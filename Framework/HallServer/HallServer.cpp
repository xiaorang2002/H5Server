// Copyright (c) 2019, Landy
// All rights reserved.


#include "HallServer.h"

// declare the content.
extern int g_bisDebug;

namespace Landy
{

extern int g_EastOfUtc;

using namespace muduo;
using namespace muduo::net;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;


std::map<int, HallServer::AccessCommandFunctor>  HallServer::m_functor_map;


HallServer::HallServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, int idleSeconds, string netcardName) :
          m_server(loop, listenAddr, "HallServer")
        , m_timer_thread(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "HallTimerEventLoopThread"))
        , m_game_sqldb_thread(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "HallSqlEventLoopThread"))
        , m_pIpFinder("qqwry.dat")
        , m_netCardName(netcardName)
        , m_bInvaildServer(false)
{
    init();
    m_server.setConnectionCallback(bind(&HallServer::onConnection, this, placeholders::_1));
    m_server.setMessageCallback(bind(&HallServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));

    m_timer_thread->startLoop();
    m_game_sqldb_thread->startLoop();
}

HallServer::~HallServer()
{
    m_proxy_servers.clear();
    m_game_servers.clear();
    m_room_servers.clear();
    m_functor_map.clear();
    m_UserScore_map.clear();
	m_userId_agentId_map.clear();
	m_proxy_gameServer_ip.clear();
	m_userId_agentId_map_getroute.clear();
	m_gameType_map.clear();
    m_proxyUserMap.clear();
    proxyIPVec.clear();
    moneyTypeVec.clear();
	m_gameRoomTableCount_map.clear();
	m_userId_current_map.clear();
	m_lastTickmap.clear();
	m_msgListsmap.clear();	
    quit();
}

void HallServer::init()
{
    srand(time(nullptr));
    m_proxy_servers.clear();
    m_game_servers.clear();
    m_room_servers.clear();
    m_functor_map.clear();
    m_UserScore_map.clear();
    m_userId_agentId_map.clear();
	m_proxy_gameServer_ip.clear();
    m_moneyType_gameServer_ip.clear();
    m_moneyType_gameServer_ip.clear();
	m_userId_agentId_map_getroute.clear();
    m_user_tasks.clear();
	m_get_task_tick.clear();
	m_gameType_map.clear();
    m_bankNameMap.clear();
    m_ActiveItemMap.clear();
    m_RegPoorMap.clear();
    m_SignInItemMap.clear();
    m_PlatformCfgMap.clear();
    m_GameTypeListMap.clear();

    m_tableIpMap.clear();
	m_gameRoomTableCount_map.clear();
	m_userId_current_map.clear();
    // 轮盘游戏类
    m_LuckyGame.reset(new LuckyGame());
	for (int32_t i = 0;i <= 5;i++)
	{
		m_lastTickmap[i] = 0;
		m_msgListsmap[i].clear();
		m_LuckyGame->LoadCurrencyConfig("",i);
	}
	m_LuckyGame->LoadConfig("");
	m_lkGame_config = m_LuckyGame->GetConfig();

    m_TaskService.reset(new TaskService());

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_REQ]
            = bind(&HallServer::cmd_keep_alive_ping, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ]
            = bind(&HallServer::cmd_login_servers, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_REQ]
            = bind(&HallServer::cmd_get_game_info, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_REQ]
            = bind(&HallServer::cmd_get_match_info, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_REQ]
            = bind(&HallServer::cmd_get_playing_game_info, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_REQ]
            = bind(&HallServer::cmd_get_game_server_message, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_REQ]
            = bind(&HallServer::cmd_get_romm_player_nums_info, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);


    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL << 8) | ::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE]
            = bind(&HallServer::cmd_on_user_offline, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER << 8) | ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER]
            = bind(&HallServer::cmd_repair_hallserver, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_REQ]
            = bind(&HallServer::cmd_set_headId, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    // m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_REQ]
    //         = bind(&HallServer::cmd_set_nickname, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_REQ]
            = bind(&HallServer::cmd_get_user_score, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_REQ]
            = bind(&HallServer::cmd_get_play_record, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

	m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_RECORD_DETAIL_REQ]
		    = bind(&HallServer::cmd_get_play_record_detail, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
	m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCHRECORD_DETAIL_REQ]
		= bind(&HallServer::cmd_get_play_record_detail_match, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_REQ]
            = bind(&HallServer::cmd_get_match_record, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_REQ]
            = bind(&HallServer::cmd_get_match_best_record, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    // 轮盘处理方法
    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_REQ]
            = bind(&HallServer::cmd_get_luckyGame_logic, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_REQ]
                = bind(&HallServer::cmd_switch_luckyGame, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_TASK_LIST_REQ]
            = bind(&HallServer::cmd_get_task_list, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_AWARDS_REQ]
            = bind(&HallServer::cmd_get_task_award, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SAFE_BOX_SCORE_REQ]
            = bind(&HallServer::cmd_safe_box, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_EXTRANGE_ORDER_REQ]
            = bind(&HallServer::cmd_extrange_order, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_BAND_BANK_REQ]
            = bind(&HallServer::cmd_band_bank, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_REQ]
            = bind(&HallServer::cmd_set_nickName, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_MIN_REBATE_REQ]
            = bind(&HallServer::cmd_set_min_rebate, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);        

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_REBATE_REQ]
            = bind(&HallServer::cmd_get_rebate, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_BANK_INFO_REQ]
            = bind(&HallServer::cmd_get_banklist, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ACTIVE_REQ]
            = bind(&HallServer::cmd_get_activeItem_list, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOAD_SIGN_IN_REQ]
            = bind(&HallServer::cmd_get_signin_info, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);     
            
    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SIGN_IN_REQ]
            = bind(&HallServer::cmd_sign_in, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);     
            
     m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_KE_FU_REQ]
            = bind(&HallServer::cmd_customer_service, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);    

     m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MAILS_REQ]
            = bind(&HallServer::cmd_get_mails, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);     
            
    m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_BULLETIN_REQ]
            = bind(&HallServer::cmd_get_bulletin, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);     
            
     m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_BULLETINMAIL_REQ]
            = bind(&HallServer::cmd_set_bulletinmails_status, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);  

    int32_t cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_CARD_DEFUALT_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_set_card_default, this);  

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_Platform_INFO_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_platform_info, this);                         

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_VIP_REWORD_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_vip_rewardcolumn, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECEIVE_REWORD_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_receive_vip_reward, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_INFO_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_recharge_info, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_REG_REWARD_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_reg_reward, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_RECORD_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_recharge_record, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_ORDER_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_recharge_order, this);
    

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RED_ENVELOPE_RAIN_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_red_envelope_info , this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GRAB_ENVELOPE_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_grab_envelope, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_THIRD_PART_GAME_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_third_part_game_info , this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_DIVIDEND_LEVEL_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_dividend_level, this);

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_DIVIDEND_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_dividend, this);
    

    cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROUTE_REQ;
    m_functor_map[cmdId] = CALLBACK_4(HallServer::cmd_get_route_record, this);

   // LOG_INFO << "bindCommands done, command map size:" << m_functor_map.size();
}

// try to do some initialize.
void HallServer::OnInit()
{
    // load all the parameter.
    this->OnLoadParameter();
}

void HallServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
    m_thread_pool.stop();

//    m_score_order_thread_pool.stop();
}

void HallServer::setThreadNum(int numThreads)
{
	LOG_INFO << __FUNCTION__ << " --- *** " << "网络IO线程数 = " << numThreads;
    m_server.setThreadNum(numThreads);
}

bool HallServer::loadRSAPriKey()
{
    int len = 0;
    vector<unsigned char> vector_AESKey;
    vector_AESKey.resize(KEY.size()/2);
    Landy::Crypto::HexStringToBuffer(KEY,  (unsigned char*)&vector_AESKey[0], len);
    string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

    vector<unsigned char> vector_decrypt_pri_key_data;

    char buf[1024*3];
    memset(buf, 0, sizeof(buf));
    if(boost::filesystem::exists("prikey.bin"))
    {
        ifstream in("prikey.bin", ios::in|ios::binary);
        if(in.is_open())
        {
            in.seekg(0, ios::end);
            int size  = in.tellg();
            in.seekg(0, ios::beg);
            in.read(buf, size);
            Landy::Crypto::aesDecrypt(strAESKey, (unsigned char*)buf, KEY_SIZE, vector_decrypt_pri_key_data);
            m_pri_key.assign(vector_decrypt_pri_key_data.begin(), vector_decrypt_pri_key_data.begin()+KEY_SIZE);
//             if(m_pri_key == ORG_PRI_KEY)
//                 LOG_INFO <<"+++++++++++++++++++++OKOKOK+++++++++++++++++++++";
//             else
//                 LOG_INFO<<"+++++++++++++++++++++Error+++++++++++++++++++++";
            in.close();
            return true;
        }
    }

    //LOG_ERROR << "PRI KEY FILE not exists!";
    return false;
}

string encryDBKey()
{
    string Key = "Hhaoren@20181024";

    int len = 0;
    vector<unsigned char> vector_AESKey;
    vector_AESKey.resize(KEY.size()/2);
    Landy::Crypto::HexStringToBuffer(KEY,  (unsigned char*)&vector_AESKey[0], len);
    string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

    vector<unsigned char> source(Key.begin(), Key.end());

    vector<unsigned char> vector_encrypt_db_key_data;
    Landy::Crypto::aesEncrypt(strAESKey, source, vector_encrypt_db_key_data);

    string targetKey = Landy::Crypto::BufferToHexString((unsigned char*)&vector_encrypt_db_key_data[6], vector_encrypt_db_key_data.size()-6);

    cout<< "targetkey = "<<targetKey;
}

string HallServer::decryDBKey(string password, int db_password_real_len)
{
    //LOG_INFO << "Load DB Key...";

#if 0
    encryDBKey();
#elif 0
    password = "CEF64F22D4EEE3CA2EEF9A6D86B517BB000000000000";
    db_password_real_len = 16;
#else

    int len = 0;
    vector<unsigned char> vector_AESKey;
    vector_AESKey.resize(KEY.size()/2);
    Landy::Crypto::HexStringToBuffer(KEY,  (unsigned char*)&vector_AESKey[0], len);
    string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

    len = 0;
    vector<unsigned char> dbKey;
    dbKey.resize(password.size()/2);
    Landy::Crypto::HexStringToBuffer(password,  (unsigned char*)&dbKey[0], len);

    vector<unsigned char> vector_decrypt_db_key_data;
    Landy::Crypto::aesDecrypt(strAESKey, (unsigned char*)&dbKey[0], dbKey.size(), vector_decrypt_db_key_data);
    string strDBKey;
    strDBKey.assign(vector_decrypt_db_key_data.begin(), vector_decrypt_db_key_data.begin()+db_password_real_len);
    return strDBKey;
#endif

}

bool HallServer::loadKey()
{
    //LOG_INFO << "Load RSA PRI Key...";
    return loadRSAPriKey();
}

void HallServer::start(int threadCount)
{
	LOG_INFO << __FUNCTION__ << " --- *** " << "worker 线程数 = " << 1;

//    LOG_INFO << "starting " << threadCount << " work threads.";
//    m_thread_pool.setThreadInitCallback(boost::bind(&HallServer::threadInit, this));
//    m_thread_pool.start(threadCount);

//    m_score_order_thread_pool.setThreadInitCallback(boost::bind(&HallServer::threadInit, this));
//    m_score_order_thread_pool.start(SCORE_ORDER_THREAD_COUNT);

    // load parameter per 5 minute.
	m_timer_thread->getLoop()->runAfter(1.5f, bind(&HallServer::refreshRepairAgentInfo, this));
    m_timer_thread->getLoop()->runAfter(3, bind(&HallServer::OnLoadParameter, this));
    m_timer_thread->getLoop()->runEvery(60*5, bind(&HallServer::OnLoadParameter, this));

    m_timer_thread->getLoop()->runEvery(60*3, bind(&HallServer::OnSendNoticeMessage, this));

    m_timer_thread->getLoop()->runAfter(3, bind(&HallServer::RefreshGameInfo, this));
    m_timer_thread->getLoop()->runAfter(3, bind(&HallServer::RefreshMatchInfo, this));
    m_timer_thread->getLoop()->runAfter(3, bind(&HallServer::RefreshMatchSwitchInfo, this));
    m_timer_thread->getLoop()->runAfter(3,bind(&HallServer::InitTasks, this));
    m_timer_thread->getLoop()->runEvery(30, bind(&HallServer:: RefreshRoomPlayerNum, this));

    m_timer_thread->getLoop()->runEvery(5.0f, bind(&HallServer::OnRepairGameServerNode, this));


    // m_timer_thread->getLoop()->runAfter(2, bind(&HallServer::test, this));

    m_server.start();
}

bool HallServer::InitZookeeper(string ip)
{
    //LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;

    m_zookeeperclient.reset(new ZookeeperClient(ip));
//    boost::shared_ptr<ZookeeperClient> zookeeperclient(new ZookeeperClient("192.168.100.160:2181"));
//    m_zookeeperclient = zookeeperclient;
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&HallServer::ZookeeperConnectedHandler, this));
    if(m_zookeeperclient->connectServer())
    {
        return true;
    }else
    {
        LOG_ERROR<<"zookeeperclient->connectServer error";
        abort();
        return false;
    }
}

bool HallServer::InitRedisCluster(string ip, string password)
{
    m_redisPubSubClient.reset(new RedisClient());
    if(!m_redisPubSubClient->initRedisCluster(ip, password))
    {
        LOG_ERROR<<"redisclient connection error";
        abort();
        return false;
    }
    m_redisPassword = password;
    m_redisPubSubClient->subsreibeRefreashConfigMessage(boost::bind(&HallServer::UpdateConfig, this, boost::placeholders::_1));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_single_proxy,CALLBACK_0(HallServer::refreshRepairAgentInfo,this));

    // 更新任务通知
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_updateTask,[&](string msg){ 
        LOG_ERROR << "-------------updateTask------------"<<msg <<" "<< __FUNCTION__ ;
        {
            MutexLockGuard lock(m_userId_agentId_MutexLock);
            m_userId_agentId_map.clear();
        }
		for (int32_t i = 0;i <= 5;i++)
		{
			m_TaskService->setUpdateTask(i, true);
		}        
    }); 

    // 更新代理银行信息时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_bank_list_info,[&](string msg){ 
        MutexLockGuard lock(m_userId_agentId_MutexLock);
        m_bankNameMap.clear();
        LOG_ERROR << "-------------update bank list------------"<< msg;
    }); 

    // 更新
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_dividend_cfg,[&](string msg){ 
        MutexLockGuard lock(m_dividend_MutexLock);
        m_dividendMap.clear();
        m_dividendCfgMap.clear();
        m_dividend_fee_vectorMap.clear();
        m_dividend_receive_configMap.clear();
        LOG_ERROR << "-------------update dividend------------"<< msg;
    }); 
    
    // 更新活动内容时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_active_item_list,[&](string msg){ 
        MutexLockGuard lock(m_userId_agentId_MutexLock);
        m_ActiveItemMap.clear();
        m_RegPoorMap.clear();
        m_PicthPathMap.clear();
        LOG_ERROR << "-------------update active item list------------"<< msg;
        try
        {
            mongocxx::collection collProxyInfo = MONGODBCLIENT["gamemain"]["membership_level"];
            mongocxx::cursor cursorproxy = collProxyInfo.find({});// (query_value.view());li
            m_ProxyLevelStandard.clear();
            int32_t dbagentid = 0;
            for(auto &doc : cursorproxy)
            {
                dbagentid = doc["agentid"].get_int32();
                if(m_ProxyLevelStandard.find(dbagentid)!=m_ProxyLevelStandard.end())
                {
                    ProxyLevelInfo info;
                    info.bettingScore =doc["upgradevalue"].get_int64();   //升级所需洗码量
                    info.viplevel = doc["levelnumber"].get_int32()-1;     //级别
                    info.upgradeScore = doc["upgradereward"].get_int64(); //升级奖金
                    m_ProxyLevelStandard[dbagentid].push_back(info);
                }
                else
                {
                    vector<ProxyLevelInfo> vecInfo;
                    vecInfo.clear();
                    ProxyLevelInfo info;
                    info.bettingScore =doc["upgradevalue"].get_int64();
                    info.viplevel = doc["levelnumber"].get_int32()-1;
                    info.upgradeScore = doc["upgradereward"].get_int64();
                    m_ProxyLevelStandard[dbagentid].push_back(info);
                    vecInfo.push_back(info);
                    m_ProxyLevelStandard[dbagentid]=vecInfo;
                }
            }


            mongocxx::collection collPlatform = MONGODBCLIENT["gamemain"]["platform_activity_item"];
            mongocxx::cursor cursorplatform = collPlatform.find({});// (query_value.view());li
            m_ProxyRedStandard.clear();
            m_ProxyStartTime.clear();
            m_ProxyEndTime.clear();
            int32_t dbagenidRed = 0;
            for(auto &doc : cursorplatform)
            {
                dbagenidRed = doc["agentid"].get_int32();
                mongocxx::collection collpz = MONGODBCLIENT["gamemain"]["platform_activity"];
                auto cursorpz = collpz.find_one(document{} << "agentid" << dbagenidRed <<"type"<<1<< finalize);// (query_value.view());li
                if(cursorpz)
                {
                    bsoncxx::document::view view = cursorpz->view();
                    if(m_ProxyRedStandard.find(dbagenidRed)!=m_ProxyRedStandard.end())
                    {
                        RedEnvelopeInfo info;
                        info.beginTime = doc["begintime"].get_date()/1000;   //发红包开始时间
                        info.endTime = doc["closetime"].get_date()/1000;   //发红包开始时间
                        info.scoreMin =    doc["minamount"].get_int64(); //红包最小值
                        info.scoreMax = doc["maxamount"].get_int64();  //红包最大值
                        info.totalScore = doc["totalamount"].get_int64(); //红包总值
                        info.upScoreLimit = doc["deposit"].get_int64();     //需要的上分值
                        info.upScoreMul = doc["depositrate"].get_int32(); //需要打码量的倍数,针对的是上分所需要的打码量
                        info.needBet = doc["validbet"].get_int64();    //需要的打码量
                        info.allTimes = doc["times"].get_int32();       //次数
                        info.activityTimeStart=view["begintime"].get_date()/1000;
                        info.activityTimeEnd=view["closetime"].get_date()/1000;
                        m_ProxyRedStandard[dbagenidRed].push_back(info);
                        m_ProxyStartTime[dbagenidRed].push_back(info.beginTime);
                        m_ProxyEndTime[dbagenidRed].push_back(info.endTime);

                    }
                    else
                    {
                        vector<RedEnvelopeInfo> vecInfo;
                        vector<int64_t> vecBeginTime;
                        vector<int64_t> vecEndTime;
                        vecInfo.clear();
                        vecEndTime.clear();
                        vecBeginTime.clear();
                        RedEnvelopeInfo info;
                        info.beginTime = doc["begintime"].get_date()/1000;   //发红包开始时间
                        info.endTime = doc["closetime"].get_date()/1000;   //发红包开始时间
                        info.scoreMin =    doc["minamount"].get_int64(); //红包最小值
                        info.scoreMax = doc["maxamount"].get_int64();  //红包最大值
                        info.totalScore = doc["totalamount"].get_int64(); //红包总值
                        info.upScoreLimit = doc["deposit"].get_int64();     //需要的上分值
                        info.upScoreMul = doc["depositrate"].get_int32(); //需要打码量的倍数,针对的是上分所需要的打码量
                        info.needBet = doc["validbet"].get_int64();    //需要的打码量
                        info.allTimes = doc["times"].get_int32();       //次数
                        info.activityTimeStart = view["begintime"].get_date()/1000;
                        info.activityTimeEnd = view["closetime"].get_date()/1000;
                        vecInfo.push_back(info);
                        m_ProxyRedStandard[dbagenidRed]=vecInfo;
                        vecBeginTime.push_back(info.beginTime );
                        vecEndTime.push_back(info.endTime);
                        m_ProxyStartTime[dbagenidRed] = vecBeginTime;
                        m_ProxyEndTime[dbagenidRed] = vecEndTime;
                    }
                }

            }
            //需要排序
        }
        catch(exception &e)
        {

        }
    }); 
   // 更新第日签到内容时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_sign_in_item_list,[&](string msg){ 
        MutexLockGuard lock(m_userId_agentId_MutexLock);
        m_SignInItemMap.clear();
        LOG_ERROR << "-------------update sign in item list------------"<< msg;
    }); 

    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_customer_serevice,[&](string msg){ 
        MutexLockGuard lock(m_userId_agentId_MutexLock);
        m_KeFuCfgItemMap.clear();
        m_FQAItemMap.clear();
        m_ImgCfgItemMap.clear();
        LOG_ERROR << "-------------update sign in item list------------"<< msg;
    }); 

    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_member_level,[&](string msg){ 
        MutexLockGuard lock(m_userId_agentId_MutexLock);
        m_LevelCfgMap.clear();
        LOG_ERROR << "-------------update member level------------"<< msg;
    }); 
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_third_part_game,[&](string msg){ 
        MutexLockGuard lock(m_thirdpart_game_Lock);
        m_GameTypeListMap.clear();
        LOG_ERROR << "-------------update third part game------------"<< msg;
    });
   
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_gameserver_ip,[&](string msg){ 
        MutexLockGuard lock(m_proxy_game_ip);
        m_proxy_gameServer_ip.clear();
		m_gameserverIP_AgentIdVec.clear();
        LOG_ERROR << "-------------update proxy gameServer ip------------"<< msg;
    });


    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_gameserver_ip,[&](string msg){
        MutexLockGuard lock(m_currnecy_game_ip);
        m_moneyType_gameServer_ip.clear();
        LOG_ERROR << "-------------update money type gameServer ip------------"<< msg;
    });

    

       


//    m_redisPubSubClient->subscribeRechargeScoreMessage(boost::bind(&HallServer::notifyRechargeScoreMessage, this, _1));
//    m_redisPubSubClient->subscribeExchangeScoreMessage(boost::bind(&HallServer::notifyExchangeScoreMessage, this, _1));
    m_redisPubSubClient->startSubThread();

    m_redisIPPort = ip;

    return true;
}

bool HallServer::startThreadPool(int threadCount)
{
   // LOG_INFO << "starting " << threadCount << " work threads.";
    m_thread_pool.setThreadInitCallback(bind(&HallServer::threadInit, this));
    m_thread_pool.start(threadCount);
    return true;
}

void HallServer::threadInit()
{
    //===============RedisClient=============
    if(!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
    {
        LOG_ERROR<<"RedisClient Connection ERROR!";
        return;
    }

    ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
    mongocxx::database db = MONGODBCLIENT["gamemain"];
}

void HallServer::UpdateConfig(string msg)
{
   // LOG_INFO <<"Some Game Modify Config " << __FILE__ << " " << __FUNCTION__ ;
    RefreshGameInfo();
    RefreshMatchInfo();
    RefreshMatchSwitchInfo();
}
bool HallServer::startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize)
{
//    m_game_sqldb_thread->getLoop()->runInLoop([&]()
//    {


//    });
 //   string url = "tcp://"+host+":"+std::to_string(port);
 //   string dbPassword = /*decryDBKey(password, db_password_len);*/password;
//    bool bSucces = ConnectionPool::GetInstance()->initPool(url, name, dbPassword, dbname, maxSize);
    return true;
}
bool HallServer::InitMongoDB(string url)
{
   // LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;

    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}

bool HallServer::InitRocketMQ(string &ip)
{
#if 0
    m_ConsumerClusterScoreOrder.SetMesageCallback(std::bind(&HallServer::ConsumerClusterScoreOrderCallback, this, std::placeholders::_1));
    bool bInitOK = false;
    try
    {
        m_Producer.Start(ip, "TianXiaRet");
        m_ConsumerClusterScoreOrder.Start(ip, "TianXia", "ScoreOrder", "*", MessageModel::CLUSTERING);
        bInitOK = true;
    }catch(exception &e)
    {
        LOG_ERROR << "Init RocketMQ Error:"<<e.what();
        abort();
        return false;
    }
    return bInitOK;
#else
    return true;
#endif

}

// uint32_t HallServer::RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe)
// {
//     for(MQMessageExt mqItem : mqe)
//     {
//         LOG_ERROR<< "RocketMQBroadcastCallback:" << mqItem.getBody();
//     }
//     return 0;
// }

// uint32_t HallServer::ConsumerClusterScoreOrderCallback(const vector<MQMessageExt> &mqe)
// {
//     for(MQMessageExt mqItem : mqe)
//     {
//        // LOG_ERROR<< "ConsumerClusterScoreOrderCallback:" << mqItem.getBody();
//         string tag = mqItem.getTags();
//         string key = mqItem.getKeys();
//         string body = mqItem.getBody();
//         if(key == "Add")
//             m_thread_pool.run(bind(&HallServer::ProcessScoreOrderAdd, this,tag,key, body));
//         else if(key == "Sub")
//             m_thread_pool.run(bind(&HallServer::ProcessScoreOrderSub, this,tag,key, body));
//     }
//     return 0;
// }

void HallServer::stopConsumerClusterScoreOrder()
{
#if 0
    m_ConsumerClusterScoreOrder.shutdownConsumer();
#endif
}
 
void HallServer::ProcessScoreOrderAdd(string &tag,string &key, string body)
{
    int64_t userId = 0, score = 0;
    string account = "", orderId = "";
    uint32_t agentId = 0;

    Json::Reader reader;
    Json::Value root;
    if(reader.parse(body, root))
    {
        userId = root["userid"].asInt64();
        account = root["account"].asString();
        agentId = root["agentid"].asUInt();
        score = root["score"].asInt64();
        orderId = root["orderid"].asString();

        int32_t bRet = AddOrderScore(userId, account, agentId, score, orderId);

        Json::Value sendRoot;
        sendRoot["orderid"] = orderId;
        sendRoot["userid"] = userId;
        sendRoot["status"] = bRet;
        Json::FastWriter writer;
        string msg = writer.write(sendRoot);
        //LOG_ERROR << "ProcessScoreOrderAdd RET:"<<msg;
        // m_Producer.SendData("ScoreOrderRet", tag, "", msg);
        //后台要求，返回不用mq
        //REDISCLIENT.set(key,msg,ONE_DAY_SECOND);
    }
}

int32_t HallServer::AddOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId)
{
     
    return 0;
}

void HallServer::ProcessScoreOrderSub(string &tag,string &key, string body)
{
    int64_t userId = 0, score = 0;
    string account = "", orderId = "";
    uint32_t agentId = 0;

    Json::Reader reader;
    Json::Value root;
    if(reader.parse(body, root))
    {
        userId = root["userid"].asInt64();
        account = root["account"].asString();
        agentId = root["agentid"].asUInt();
        score = root["score"].asInt64();
        orderId = root["orderid"].asString();
        //m_zookeeperclient->createNode(m_szUserOrderScorePath, "UserOrderScore");

        int32_t bRet = SubOrderScore(userId, account, agentId, score, orderId);

        Json::Value sendRoot;
        sendRoot["orderid"] = orderId;
        sendRoot["userid"] = userId;
        sendRoot["status"] = bRet;
        Json::FastWriter writer;
        string msg = writer.write(sendRoot);
       // LOG_ERROR << "ProcessScoreOrderSub RET:"<<msg;
        // m_Producer.SendData("ScoreOrderRet", tag," ", msg);
        //后台要求，返回不用mq
        //REDISCLIENT.set(key,msg,ONE_DAY_SECOND);
    }
}

int32_t HallServer::SubOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId)
{ 
    return 0;
}

bool HallServer::CanSubMoney(int64_t userId)
{
    bool bExist = true;
    if(userId > 0)
    {
        bExist = REDISCLIENT.ExistsUserOnlineInfo(userId);
    }
    return !bExist;
}

void HallServer::ZookeeperConnectedHandler()
{
	LOG_INFO << __FUNCTION__;
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");

    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServers"))
        m_zookeeperclient->createNode("/GAME/HallServers", "HallServers");

    m_szUserOrderScorePath="/GAME/UserOrderScore";
    if(ZNONODE == m_zookeeperclient->existsNode(m_szUserOrderScorePath))
        m_zookeeperclient->createNode(m_szUserOrderScorePath, "UserOrderScore");

    vector<string> vec;
    boost::algorithm::split( vec, m_server.ipPort(), boost::is_any_of( ":" ));
    string ip;
    GlobalFunc::getIP(m_netCardName, ip);
    m_szNodeValue = ip + ":" + vec[1];
    m_szNodePath  = "/GAME/HallServers/" + m_szNodeValue;
    m_szInvalidNodePath = "/GAME/HallServersInvaild/" +m_szNodeValue;
    m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);

    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/ProxyServers"))
        m_zookeeperclient->createNode("/GAME/ProxyServers", "ProxyServers");

    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServers"))
        m_zookeeperclient->createNode("/GAME/GameServers", "GameServers");

    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServersInvalid"))
        m_zookeeperclient->createNode("/GAME/GameServersInvalid", "GameServersInvalid");


    GetChildrenWatcherHandler getProxyServersChildrenWatcherHandler = std::bind(&HallServer::GetProxyServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);
    vector<string> proxyServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/ProxyServers", proxyServers, getProxyServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_proxy_servers_mutex);
        m_proxy_servers.assign(proxyServers.begin(), proxyServers.end());
    }

    // init to dump the game servers.
//     for(string ip : proxyServers)
//     {
//        // LOG_INFO << " >>> Init GET GameServers :" << ip;
//     }


    GetChildrenWatcherHandler getGameServersChildrenWatcherHandler = bind(&HallServer::GetGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);
    vector<string> gameServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/GameServers", gameServers, getGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_game_servers_mutex);
        m_game_servers.assign(gameServers.begin(), gameServers.end());
    }
    //MutexLockGuard lock((m_room_servers_mutex);
    WRITE_LOCK(m_room_player_num_mutex);
    m_room_servers.clear();
    for(string ip : gameServers)
    {
       // LOG_INFO << "GET GameServers :"<<ip;

        vector<string> vec;
        boost::algorithm::split(vec, ip, boost::is_any_of( ":" ));
        uint32_t roomId=stoi(vec[0]);
        if(m_room_servers.find(roomId)==m_room_servers.end())
        {
            vector<string> vecIp;
            m_room_servers.emplace(roomId,vecIp);
        }
        m_room_servers[roomId].push_back(ip);
    }

    GetChildrenWatcherHandler getInvaildGameServersChildrenWatcherHandler = bind(&HallServer::GetInvaildGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);
    vector<string> InvaildGameServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/GameServersInvalid", InvaildGameServers, getInvaildGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_game_servers_mutex);
        m_invaild_game_server.assign(InvaildGameServers.begin(), InvaildGameServers.end());
    }
//     for(string ip : m_invaild_game_server)
//     {
//        // LOG_INFO << "GET InvaildGameServers :"<<ip;
//     }


    //matchServer
    GetChildrenWatcherHandler getMatchServersChildrenWatcherHandler = bind(&HallServer::GetMatchServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);
    vector<string> matchServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/MatchServers", matchServers, getMatchServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_match_servers_mutex);
        m_match_servers.assign(matchServers.begin(), matchServers.end());
    }

    GetChildrenWatcherHandler getInvaildMatchServersChildrenWatcherHandler = bind(&HallServer::GetInvaildMatchServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);
    vector<string> InvaildMatchServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/MatchServersInvalid", InvaildMatchServers, getInvaildMatchServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_match_servers_mutex);
        m_invaild_match_server.assign(InvaildMatchServers.begin(), InvaildMatchServers.end());
    }
//     for(string ip : m_invaild_match_server)
//     {
//        // LOG_INFO << "GET InvaildMatchServers :"<<ip;
//     }


}

void HallServer::GetProxyServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                       const string &path, void *context)
{
    LOG_INFO << " >>> zookeeper callback, type:" << type << ", state:" << state;

    GetChildrenWatcherHandler getProxyServersChildrenWatcherHandler = bind(&HallServer::GetProxyServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> proxy_servers;
    string proxyPath = "/GAME/ProxyServers";
    if(ZOK == m_zookeeperclient->getClildren(proxyPath, proxy_servers, getProxyServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_proxy_servers_mutex);
        m_proxy_servers.assign(proxy_servers.begin(), proxy_servers.end());
    }

//     for(string ip : proxy_servers)
//     {
//       //  LOG_INFO << " GetProxyServersChildrenWatcherHandler :"<<ip;
//     }

//     LOG_INFO << "type:" <<type;
//     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
//     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
//     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
//     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
//     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
//     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HallServer::GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                       const string &path, void *context)
{

    GetChildrenWatcherHandler getGameServersChildrenWatcherHandler = bind(&HallServer::GetGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> gameServers;
    string gamePath = "/GAME/GameServers";
    if(ZOK == m_zookeeperclient->getClildren(gamePath, gameServers, getGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_game_servers_mutex);
        m_game_servers.assign(gameServers.begin(), gameServers.end());
    }
    WRITE_LOCK(m_room_player_num_mutex);
    m_room_servers.clear();//填充房间服务器列表
    for(string ip : gameServers)
    {
        //LOG_INFO << "GET GameServers :"<<ip;

        vector<string> vec;
        boost::algorithm::split(vec, ip, boost::is_any_of( ":" ));
        uint32_t roomId=stoi(vec[0]);
        if(m_room_servers.find(roomId)==m_room_servers.end())
        {
            vector<string> vecIp;
            m_room_servers.emplace(roomId,vecIp);
        }
        m_room_servers[roomId].push_back(ip);
    }

//     LOG_INFO << "type:" <<type;
//     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
//     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
//     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
//     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
//     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
//     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HallServer::GetInvaildGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
{
    GetChildrenWatcherHandler getInvaildGameServersChildrenWatcherHandler = bind(&HallServer::GetInvaildGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> gameServersInvaild;
    string gamePath = "/GAME/GameServersInvalid";
    if(ZOK == m_zookeeperclient->getClildren(gamePath, gameServersInvaild, getInvaildGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_game_servers_mutex);
        m_invaild_game_server.assign(gameServersInvaild.begin(), gameServersInvaild.end());
        
		m_tableIpMap.clear();
    }

//     for(string ip : m_invaild_game_server)
//     {
//         //LOG_INFO << "GET m_invaild_game_server :"<<ip;
//     }

//     LOG_INFO << "type:" <<type;
//     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
//     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
//     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
//     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
//     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
//     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HallServer::GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                       const string &path, void *context)
{

    GetChildrenWatcherHandler getMatchServersChildrenWatcherHandler = bind(&HallServer::GetMatchServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> matchaServers;
    string gamePath = "/GAME/MatchServers";
    if(ZOK == m_zookeeperclient->getClildren(gamePath, matchaServers, getMatchServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_match_servers_mutex);
        m_match_servers.assign(matchaServers.begin(), matchaServers.end());
    }
    WRITE_LOCK(m_room_player_num_mutex);
    //m_room_servers.clear();//填充房间服务器列表
//     for(string ip : matchaServers)
//     {
//         LOG_INFO << "GET matchaServers :"<<ip;
//     }

//     LOG_INFO << "type:" <<type;
//     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
//     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
//     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
//     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
//     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
//     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HallServer::GetInvaildMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
{
    GetChildrenWatcherHandler getInvaildMatchServersChildrenWatcherHandler = bind(&HallServer::GetInvaildMatchServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> matchServersInvaild;
    string gamePath = "/GAME/MatchServersInvalid";
    if(ZOK == m_zookeeperclient->getClildren(gamePath, matchServersInvaild, getInvaildMatchServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_match_servers_mutex);
        m_invaild_match_server.assign(matchServersInvaild.begin(), matchServersInvaild.end());
    }
//     for(string ip : matchServersInvaild)
//     {
//         LOG_INFO << "GET m_invaild_match_server :"<<ip;
//     }

//     LOG_INFO << "type:" <<type;
//     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
//     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
//     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
//     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
//     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
//     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

// do the function to repair the content.
void HallServer::OnRepairGameServerNode()
{
    if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath))
    {
		LOG_DEBUG << m_szNodePath;
        m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
    }
}

void HallServer::RefreshGameInfo()
{
	LOG_INFO << __FUNCTION__;
    WRITE_LOCK(m_game_info_mutex);
    m_response.clear_header();
	m_response.clear_gamemessage();
    m_proxyInfoMap.clear();
	m_gameType_map.clear();
    m_proxyInfoSendMap.clear();
    m_crrentInfoSendMap.clear();
	m_gameRoomTableCount_map.clear();
    try
    {
        uint32_t dbGameId = 0, dbRevenueRatio = 0, dbGameSortId = 0, dbGameType = 0, dbGameIsHot = 0;
        string dbGameName;
        int32_t dbGameStatus = 0;

        uint32_t dbRoomId = 0, dbTableCount = 0,  dbMinPlayerNum = 0, dbMaxPlayerNum = 0, dbRoomStatus = 0;
        string dbRoomName;

        int64_t dbFloorScore, dbCeilScore, dbEnterMinScore, dbEnterMaxScore, dbMaxJettonScore;


        //////////////
        mongocxx::collection collgames = MONGODBCLIENT["gamemain"]["proxy_gameserver"];
        mongocxx::cursor info = collgames.find({});
        //添加第一个默认值，假如玩家没有找到代理就用默认值发送配置
        ::HallServer::GetGameMessageResponse zeroRep;
        zeroRep.clear_gamemessage();
        zeroRep.clear_header();
        m_proxyInfoSendMap[0].CopyFrom(zeroRep);
        for(auto &doc : info)
        {

            int32_t agentid= doc["agentid"].get_int32();
            ::HallServer::GetGameMessageResponse resp;
            resp.clear_gamemessage();
            resp.clear_header();
            m_proxyInfoSendMap[agentid].CopyFrom(resp);

        }
        mongocxx::collection collcurrent = MONGODBCLIENT["gamemain"]["currency_gameserver"];
        mongocxx::cursor currentinfo = collcurrent.find({});
        for(auto &doc : currentinfo)
        {
            int32_t currency= doc["currency"].get_int32();
            ::HallServer::GetGameMessageResponse resp;
            resp.clear_gamemessage();
            resp.clear_header();
            m_crrentInfoSendMap[currency].CopyFrom(resp);
        }
        mongocxx::collection proxyinfo = MONGODBCLIENT["gamemain"]["proxy_info"];
        mongocxx::cursor pinfo = proxyinfo.find({});
        map<int32_t,string> mapGameGroupid;
        mapGameGroupid.clear();
        for(auto &doc : pinfo)
        {
            string groupid = doc["gamegroupid"].get_utf8().value.to_string();
            int32_t agentid= doc["agentid"].get_int32();
            mapGameGroupid[agentid]=groupid;
        }

        mongocxx::collection colllist = MONGODBCLIENT["gamemain"]["proxy_info_gamesgroup"];
        mongocxx::cursor userresult = colllist.find({});

        std::map<int32_t,int32_t> m_proxyNotInMap;
        m_proxyNotInMap.clear();
        for(auto &doc : userresult)
        {
            for(auto &group:mapGameGroupid)
            {
                string _id=doc["_id"].get_oid().value.to_string();
                if(_id==group.second)
                {
                    int32_t proxyid = group.first;
                    if(m_proxyInfoSendMap.find(proxyid)==m_proxyInfoSendMap.end())
                    {
                        m_proxyNotInMap[proxyid]=proxyid;
                    }
                    tagProxyGamesInfo &proxyinf=m_proxyInfoMap[proxyid];

                    auto games = doc["gameids"].get_array();
                    for(auto &game:games.value)
                    {
                        proxyinf.GamesVec.push_back(game.get_int32());
                    }
                    auto rooms = doc["roomids"].get_array();
                    for(auto &room:rooms.value)
                    {
                        proxyinf.roomsVec.push_back(room.get_int32());
                    }


                }
            }
        }



        //        bsoncxx::document::element elem;
        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["game_kind"];
        mongocxx::cursor cursor = coll.find({document{} << "agentid" << 0 << finalize});// (query_value.view());li
        for(auto &doc : cursor)
        {
            dbGameId = doc["gameid"].get_int32();
            dbGameType = doc["type"].get_int32();
            m_gameType_map.insert(make_pair(dbGameId, dbGameType));
        }

       for (std::map<int32_t, ::HallServer::GetGameMessageResponse>::const_iterator it = m_proxyInfoSendMap.begin();
                it != m_proxyInfoSendMap.end(); ++it)
        {

            mongocxx::cursor cursor1 = coll.find(document{} << "agentid" << it->first << finalize);// (query_value.view());li
            for(auto &doc : cursor1)
            {
                LOG_INFO<<" 找到配置的代理号agentid"<<it->first;
                dbGameId = doc["gameid"].get_int32();
                vector<int32_t>::iterator iem;
                iem=find(m_proxyInfoMap[it->first].GamesVec.begin(),m_proxyInfoMap[it->first].GamesVec.end(),dbGameId);
                if(iem==m_proxyInfoMap[it->first].GamesVec.end()&&it->first!=0)
                {
                   continue;
                }
                dbGameName = doc["gamename"].get_utf8().value.to_string();
                dbGameSortId = doc["sort"].get_int32();
                dbGameType = doc["type"].get_int32();
                dbGameIsHot = doc["ishot"].get_int32();
                dbGameStatus = doc["status"].get_int32();

                ::HallServer::GameMessage *gameMsg =m_proxyInfoSendMap[it->first].add_gamemessage(); /*m_proxyInfoSendMap[it->first].add_gamemessage();*/
                gameMsg->set_gameid(dbGameId);
                gameMsg->set_gamename(dbGameName);
                gameMsg->set_gamesortid(dbGameSortId);
                gameMsg->set_gametype(dbGameType);
                gameMsg->set_gameishot(dbGameIsHot);
                gameMsg->set_gamestatus(dbGameStatus);


                auto rooms = doc["rooms"].get_array();
                for(auto &roomDoc : rooms.value)
                {
                    dbRoomId = roomDoc["roomid"].get_int32();

                    vector<int32_t>::iterator iem;
                    iem=find(m_proxyInfoMap[it->first].roomsVec.begin(),m_proxyInfoMap[it->first].roomsVec.end(),dbRoomId);
                    if(iem==m_proxyInfoMap[it->first].roomsVec.end()&&it->first!=0)
                    {
                        continue;
                    }
                    dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();
                    dbTableCount = roomDoc["tablecount"].get_int32();
                    dbFloorScore = roomDoc["floorscore"].get_int64();
                    dbCeilScore = roomDoc["ceilscore"].get_int64();
                    dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                    dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();
                    dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                    dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();
                    dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();
                    dbRoomStatus = roomDoc["status"].get_int32();

                    ::HallServer::GameRoomMessage *roomMsg = gameMsg->add_gameroommsg();
                    roomMsg->set_roomid(dbRoomId);
                    roomMsg->set_roomname(dbRoomName);
                    roomMsg->set_tablecount(dbTableCount);
                    roomMsg->set_floorscore(dbFloorScore);
                    roomMsg->set_ceilscore(dbCeilScore);
                    roomMsg->set_enterminscore(dbEnterMinScore);
                    roomMsg->set_entermaxscore(dbEnterMaxScore);
                    roomMsg->set_minplayernum(dbMinPlayerNum);
                    roomMsg->set_maxplayernum(dbMaxPlayerNum);
                    roomMsg->set_maxjettonscore(dbMaxJettonScore);
                    roomMsg->set_status(dbRoomStatus);

                    bsoncxx::types::b_array jettons;
                    bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                    if(jettons_obj.type() == bsoncxx::type::k_array)
                       jettons = jettons_obj.get_array();
                    else
                       jettons = roomDoc["jettons"]["_v"].get_array();
                    for(auto &jetton : jettons.value)
                    {
    //                    LOG_DEBUG << "jettos:"<<jetton.get_int64();
                        roomMsg->add_jettons(jetton.get_int64());
                    }

                    roomMsg->set_playernum(0);
					if (dbGameType == GameType_BaiRen)
					{
						if (!m_gameRoomTableCount_map.count(dbRoomId))
						{
							m_gameRoomTableCount_map.insert(make_pair(dbRoomId, dbTableCount));
						}
					}
                }
                //m_gameType_map.insert(make_pair(dbGameId, dbGameType));
            }
        }

       for(std::map<int32_t, int32_t>::const_iterator it = m_proxyNotInMap.begin();
           it != m_proxyNotInMap.end(); ++it)
       {

           ::HallServer::GetGameMessageResponse resp;
           resp.clear_gamemessage();
           resp.clear_header();
           m_proxyInfoSendMap[it->first].CopyFrom(resp);

           mongocxx::cursor cursor1 = coll.find(document{} << "agentid" << (int32_t)0 << finalize);// (query_value.view());li
           for(auto &doc : cursor1)
           {
               LOG_INFO<<" 找到配置的代理号agentid"<<it->first;
               dbGameId = doc["gameid"].get_int32();
               vector<int32_t>::iterator iem;
               iem=find(m_proxyInfoMap[it->first].GamesVec.begin(),m_proxyInfoMap[it->first].GamesVec.end(),dbGameId);
               if(iem==m_proxyInfoMap[it->first].GamesVec.end()&&it->first!=0)
               {
                  continue;
               }
               dbGameName = doc["gamename"].get_utf8().value.to_string();
               dbGameSortId = doc["sort"].get_int32();
               dbGameType = doc["type"].get_int32();
               dbGameIsHot = doc["ishot"].get_int32();
               dbGameStatus = doc["status"].get_int32();

               ::HallServer::GameMessage *gameMsg =m_proxyInfoSendMap[it->first].add_gamemessage(); /*m_proxyInfoSendMap[it->first].add_gamemessage();*/
               gameMsg->set_gameid(dbGameId);
               gameMsg->set_gamename(dbGameName);
               gameMsg->set_gamesortid(dbGameSortId);
               gameMsg->set_gametype(dbGameType);
               gameMsg->set_gameishot(dbGameIsHot);
               gameMsg->set_gamestatus(dbGameStatus);


               auto rooms = doc["rooms"].get_array();
               for(auto &roomDoc : rooms.value)
               {
                   dbRoomId = roomDoc["roomid"].get_int32();

                   vector<int32_t>::iterator iem;
                   iem=find(m_proxyInfoMap[it->first].roomsVec.begin(),m_proxyInfoMap[it->first].roomsVec.end(),dbRoomId);
                   if(iem==m_proxyInfoMap[it->first].roomsVec.end()&&it->first!=0)
                   {
                       continue;
                   }
                   dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();
                   dbTableCount = roomDoc["tablecount"].get_int32();
                   dbFloorScore = roomDoc["floorscore"].get_int64();
                   dbCeilScore = roomDoc["ceilscore"].get_int64();
                   dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                   dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();
                   dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                   dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();
                   dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();
                   dbRoomStatus = roomDoc["status"].get_int32();

                   ::HallServer::GameRoomMessage *roomMsg = gameMsg->add_gameroommsg();
                   roomMsg->set_roomid(dbRoomId);
                   roomMsg->set_roomname(dbRoomName);
                   roomMsg->set_tablecount(dbTableCount);
                   roomMsg->set_floorscore(dbFloorScore);
                   roomMsg->set_ceilscore(dbCeilScore);
                   roomMsg->set_enterminscore(dbEnterMinScore);
                   roomMsg->set_entermaxscore(dbEnterMaxScore);
                   roomMsg->set_minplayernum(dbMinPlayerNum);
                   roomMsg->set_maxplayernum(dbMaxPlayerNum);
                   roomMsg->set_maxjettonscore(dbMaxJettonScore);
                   roomMsg->set_status(dbRoomStatus);

                   bsoncxx::types::b_array jettons;
                   bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                   if(jettons_obj.type() == bsoncxx::type::k_array)
                      jettons = jettons_obj.get_array();
                   else
                      jettons = roomDoc["jettons"]["_v"].get_array();
                   for(auto &jetton : jettons.value)
                   {
   //                    LOG_DEBUG << "jettos:"<<jetton.get_int64();
                       roomMsg->add_jettons(jetton.get_int64());
                   }

                   roomMsg->set_playernum(0);
				   if (dbGameType == GameType_BaiRen)
				   {
					   if (!m_gameRoomTableCount_map.count(dbRoomId))
					   {
						   m_gameRoomTableCount_map.insert(make_pair(dbRoomId, dbTableCount));
					   }
				   }
               }
               //m_gameType_map.insert(make_pair(dbGameId, dbGameType));
           }

       }


       for(std::map<int32_t, ::HallServer::GetGameMessageResponse>::const_iterator it = m_crrentInfoSendMap.begin();
           it != m_crrentInfoSendMap.end(); ++it)
       {
           ::HallServer::GetGameMessageResponse resp;
           resp.clear_gamemessage();
           resp.clear_header();
           m_crrentInfoSendMap[it->first].CopyFrom(resp);

           mongocxx::cursor cursor1 = coll.find(document{} << "currency" << (int32_t)it->first << finalize);// (query_value.view());li
           for(auto &doc : cursor1)
           {
               LOG_INFO<<" 找到配置的代理号agentid"<<it->first;
               dbGameId = doc["gameid"].get_int32();
               dbGameName = doc["gamename"].get_utf8().value.to_string();
               dbGameSortId = doc["sort"].get_int32();
               dbGameType = doc["type"].get_int32();
               dbGameIsHot = doc["ishot"].get_int32();
               dbGameStatus = doc["status"].get_int32();

               ::HallServer::GameMessage *gameMsg =m_crrentInfoSendMap[it->first].add_gamemessage(); /*m_proxyInfoSendMap[it->first].add_gamemessage();*/
               gameMsg->set_gameid(dbGameId);
               gameMsg->set_gamename(dbGameName);
               gameMsg->set_gamesortid(dbGameSortId);
               gameMsg->set_gametype(dbGameType);
               gameMsg->set_gameishot(dbGameIsHot);
               gameMsg->set_gamestatus(dbGameStatus);


               auto rooms = doc["rooms"].get_array();
               for(auto &roomDoc : rooms.value)
               {
                   dbRoomId = roomDoc["roomid"].get_int32();
                   dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();
                   dbTableCount = roomDoc["tablecount"].get_int32();
                   dbFloorScore = roomDoc["floorscore"].get_int64();
                   dbCeilScore = roomDoc["ceilscore"].get_int64();
                   dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                   dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();
                   dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                   dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();
                   dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();
                   dbRoomStatus = roomDoc["status"].get_int32();

                   ::HallServer::GameRoomMessage *roomMsg = gameMsg->add_gameroommsg();
                   roomMsg->set_roomid(dbRoomId);
                   roomMsg->set_roomname(dbRoomName);
                   roomMsg->set_tablecount(dbTableCount);
                   roomMsg->set_floorscore(dbFloorScore);
                   roomMsg->set_ceilscore(dbCeilScore);
                   roomMsg->set_enterminscore(dbEnterMinScore);
                   roomMsg->set_entermaxscore(dbEnterMaxScore);
                   roomMsg->set_minplayernum(dbMinPlayerNum);
                   roomMsg->set_maxplayernum(dbMaxPlayerNum);
                   roomMsg->set_maxjettonscore(dbMaxJettonScore);
                   roomMsg->set_status(dbRoomStatus);

                   bsoncxx::types::b_array jettons;
                   bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                   if(jettons_obj.type() == bsoncxx::type::k_array)
                      jettons = jettons_obj.get_array();
                   else
                      jettons = roomDoc["jettons"]["_v"].get_array();
                   for(auto &jetton : jettons.value)
                   {
                       roomMsg->add_jettons(jetton.get_int64());
                   }

                   roomMsg->set_playernum(0);
                   if (dbGameType == GameType_BaiRen)
                   {
                       if (!m_gameRoomTableCount_map.count(dbRoomId))
                       {
                           m_gameRoomTableCount_map.insert(make_pair(dbRoomId, dbTableCount));
                       }
                   }
               }

           }
       }

        RefreshRoomPlayerNum();


        mongocxx::collection collProxyInfo = MONGODBCLIENT["gamemain"]["membership_level"];
        mongocxx::cursor cursorproxy = collProxyInfo.find({});// (query_value.view());li
        m_ProxyLevelStandard.clear();
        int32_t dbagentid = 0;
        for(auto &doc : cursorproxy)
        {
            dbagentid = doc["agentid"].get_int32();
            if(m_ProxyLevelStandard.find(dbagentid)!=m_ProxyLevelStandard.end())
            {
                ProxyLevelInfo info;
                info.bettingScore =doc["upgradevalue"].get_int64();   //升级所需洗码量
                info.viplevel = doc["levelnumber"].get_int32()-1;     //级别
                info.upgradeScore = doc["upgradereward"].get_int64(); //升级奖金
                m_ProxyLevelStandard[dbagentid].push_back(info);
            }
            else
            {
                vector<ProxyLevelInfo> vecInfo;
                vecInfo.clear();
                ProxyLevelInfo info;
                info.bettingScore =doc["upgradevalue"].get_int64();
                info.viplevel = doc["levelnumber"].get_int32()-1;
                info.upgradeScore = doc["upgradereward"].get_int64();
                m_ProxyLevelStandard[dbagentid].push_back(info);
                vecInfo.push_back(info);
                m_ProxyLevelStandard[dbagentid]=vecInfo;
            }
        }


        mongocxx::collection collPlatform = MONGODBCLIENT["gamemain"]["platform_activity_item"];
        mongocxx::cursor cursorplatform = collPlatform.find({});// (query_value.view());li
        m_ProxyRedStandard.clear();
        m_ProxyStartTime.clear();
        m_ProxyEndTime.clear();
        int32_t dbagenidRed = 0;
        for(auto &doc : cursorplatform)
        {
            dbagenidRed = doc["agentid"].get_int32();
            mongocxx::collection collpz = MONGODBCLIENT["gamemain"]["platform_activity"];
            auto cursorpz = collpz.find_one(document{} << "agentid" << dbagenidRed <<"type"<<1<< finalize);// (query_value.view());li
            if(cursorpz)
            {
                bsoncxx::document::view view = cursorpz->view();
                if(m_ProxyRedStandard.find(dbagenidRed)!=m_ProxyRedStandard.end())
                {
                    RedEnvelopeInfo info;
                    info.beginTime = doc["begintime"].get_date()/1000;   //发红包开始时间
                    info.endTime = doc["closetime"].get_date()/1000;   //发红包开始时间
                    info.scoreMin =    doc["minamount"].get_int64(); //红包最小值
                    info.scoreMax = doc["maxamount"].get_int64();  //红包最大值
                    info.totalScore = doc["totalamount"].get_int64(); //红包总值
                    info.upScoreLimit = doc["deposit"].get_int64();     //需要的上分值
                    info.upScoreMul = doc["depositrate"].get_int32(); //需要打码量的倍数,针对的是上分所需要的打码量
                    info.needBet = doc["validbet"].get_int64();    //需要的打码量
                    info.allTimes = doc["times"].get_int32();       //次数
                    info.activityTimeStart=view["begintime"].get_date()/1000;
                    info.activityTimeEnd=view["closetime"].get_date()/1000;
                    m_ProxyRedStandard[dbagenidRed].push_back(info);
                    m_ProxyStartTime[dbagenidRed].push_back(info.beginTime);
                    m_ProxyEndTime[dbagenidRed].push_back(info.endTime);

                }
                else
                {
                    vector<RedEnvelopeInfo> vecInfo;
                    vector<int64_t> vecBeginTime;
                    vector<int64_t> vecEndTime;
                    vecInfo.clear();
                    vecEndTime.clear();
                    vecBeginTime.clear();
                    RedEnvelopeInfo info;
                    info.beginTime = doc["begintime"].get_date()/1000;   //发红包开始时间
                    info.endTime = doc["closetime"].get_date()/1000;   //发红包开始时间
                    info.scoreMin =    doc["minamount"].get_int64(); //红包最小值
                    info.scoreMax = doc["maxamount"].get_int64();  //红包最大值
                    info.totalScore = doc["totalamount"].get_int64(); //红包总值
                    info.upScoreLimit = doc["deposit"].get_int64();     //需要的上分值
                    info.upScoreMul = doc["depositrate"].get_int32(); //需要打码量的倍数,针对的是上分所需要的打码量
                    info.needBet = doc["validbet"].get_int64();    //需要的打码量
                    info.allTimes = doc["times"].get_int32();       //次数
                    info.activityTimeStart = view["begintime"].get_date()/1000;
                    info.activityTimeEnd = view["closetime"].get_date()/1000;
                    vecInfo.push_back(info);
                    m_ProxyRedStandard[dbagenidRed]=vecInfo;
                    vecBeginTime.push_back(info.beginTime );
                    vecEndTime.push_back(info.endTime);
                    m_ProxyStartTime[dbagenidRed] = vecBeginTime;
                    m_ProxyEndTime[dbagenidRed] = vecEndTime;
                }
            }

        }
        //需要排序
    }
    catch (exception &e)
    {
        LOG_ERROR << "MongoDBException: " << e.what();
    }
}

void HallServer::InitTasks()
{
     LOG_INFO << __FILE__ << " " << __FUNCTION__;
     //m_TaskService->loadTaskStatus();
}

void HallServer::RefreshMatchInfo()
{
   // LOG_INFO << __FILE__ << " " << __FUNCTION__;
    WRITE_LOCK(m_match_info_mutex);
    m_MatchResp.clear_header();
    m_MatchResp.clear_matchmessage();
    try
    {
//        bsoncxx::document::element elem;
        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["match_kind"];
//        auto query_value = document{} << finalize;
//        LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
        mongocxx::cursor cursor = coll.find({});// (query_value.view());li
        for(auto &doc : cursor)
        {
            // LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(doc);

            ::HallServer::MatchMessage *gameMsg = m_MatchResp.add_matchmessage();
            gameMsg->set_gameid(doc["gameid"].get_int32());
            gameMsg->set_gamename(doc["gamename"].get_utf8().value.to_string());
            gameMsg->set_gamesortid(doc["sort"].get_int32());
            gameMsg->set_gameishot(doc["ishot"].get_int32());
            gameMsg->set_status(doc["status"].get_int32());
            gameMsg->set_roomid(doc["matchid"].get_int32());
            gameMsg->set_matchname(doc["matchname"].get_utf8().value.to_string());
            gameMsg->set_joinneedscore(doc["joinNeedScore"].get_int32());
            gameMsg->set_playernum(doc["matchplayernum"].get_int32());
            gameMsg->set_mayneedseconds(doc["mayNeedSeconds"].get_int32());
            gameMsg->set_floorscore(doc["floorscore"].get_int64());

            bsoncxx::types::b_array awards;
            bsoncxx::document::element award_obj = doc["award"];
            if(award_obj.type() == bsoncxx::type::k_array)
               awards = award_obj.get_array();
            else
               awards = doc["award"]["_v"].get_array();
            for(auto &award : awards.value)
            {
                gameMsg->add_awardscore(award.get_int64());
            }


            bsoncxx::types::b_array jettons;
            bsoncxx::document::element jettons_obj = doc["jettons"];
            if(jettons_obj.type() == bsoncxx::type::k_array)
               jettons = jettons_obj.get_array();
            else
               jettons = doc["jettons"]["_v"].get_array();
            for(auto &jetton : jettons.value)
            {
//                    LOG_DEBUG << "jettos:"<<jetton.get_int64();
                gameMsg->add_jettons(jetton.get_int64());
            }

            bsoncxx::types::b_array opentimes;
            bsoncxx::document::element opentimes_obj = doc["openTime"];
            if(opentimes_obj)
            {
                if(opentimes_obj.type() == bsoncxx::type::k_array)
                   opentimes = opentimes_obj.get_array();
                else
                   opentimes = doc["openTime"]["_v"].get_array();
                for(auto &times : opentimes.value)
                {
    //                    LOG_DEBUG << "jettos:"<<jetton.get_int64();
                    gameMsg->add_opentimes(times.get_int64());
                }
            }
        }
    }
    catch (exception &e)
    {
        LOG_ERROR << "MongoDBException: " << e.what();
    }
}

void HallServer::RefreshMatchSwitchInfo()
{
    LOG_INFO << __FUNCTION__;
    WRITE_LOCK(m_shieldAgentIds_mutex);
    m_shieldAgentIds.clear();
	try
	{
		//读取match_switch
		mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["match_switch"];
		mongocxx::cursor cursor = coll.find({});
		for (auto& doc : cursor)
		{
            LOG_INFO << "QueryResult:" << bsoncxx::to_json(doc);
			uint32_t matchId = doc["matchid"].get_int32();
			std::vector<uint32_t>& agentIds = m_shieldAgentIds[matchId];
			agentIds.clear();
			bsoncxx::types::b_array agentids;
			bsoncxx::document::element shield = doc["shield"];
			if (shield.type() == bsoncxx::type::k_array) {
				agentids = shield.get_array();
			}
			else {
				agentids = doc["shield"]["_v"].get_array();
			}
			for (auto& agentid : agentids.value) {
				agentIds.push_back(agentid.get_int32());
                LOG_INFO << "--- *** " << "matchId[" << matchId << "] agentId = " << agentid.get_int32();
			}
		}
	}
	catch (exception & e)
	{
		LOG_ERROR << "MongoDBException: " << e.what();
	}
    
    for (std::map<int32_t, std::vector<uint32_t>>::const_iterator it = m_shieldAgentIds.begin();
        it != m_shieldAgentIds.end(); ++it) {
        uint32_t matchId = it->first;
        LOG_ERROR << "--- *** " << "matchId[" << matchId << "] 屏蔽代理\n";
        for (std::vector<uint32_t>::const_iterator ir = it->second.begin();
            ir != it->second.end(); ++ir) {
            LOG_ERROR << "--- *** " << "matchId[" << matchId << "] agentId = " << *ir;
        }
    }
}

void HallServer::RefreshRoomPlayerNum()
{

    m_thread_pool.run(bind(&HallServer::updateRoomPlayerNum, this));
}

void HallServer::updateRoomPlayerNum()
{
    // LOG_INFO << __FILE__ << " " << __FUNCTION__;
    WRITE_LOCK(m_room_player_num_mutex);
    m_roomPlayerNums.clear_header();
    m_roomPlayerNums.clear_gameplayernummessage();
    auto& gameMessage=m_proxyInfoSendMap[0].gamemessage();
    uint32_t gameId=0;
    int roomId=0;
    uint64_t playerNums=0;
    try
    {
        for(auto&gameInfo:gameMessage )
        {
            ::HallServer::SimpleGameMessage* simpleMessage=m_roomPlayerNums.add_gameplayernummessage();
            gameId=gameInfo.gameid();
            simpleMessage->set_gameid(gameId);
            auto& gameroommsg= gameInfo.gameroommsg();
            for(auto&roomInfo : gameroommsg)
            {
                ::HallServer::RoomPlayerNum* roomPlayerNum=simpleMessage->add_roomplayernum();
                roomId=roomInfo.roomid();
                playerNums=0;
                if(m_room_servers.find(roomId)!=m_room_servers.end())
                    REDISCLIENT.GetGameServerplayerNum(m_room_servers[roomId],playerNums);
                roomPlayerNum->set_roomid(roomId);
                roomPlayerNum->set_playernum(playerNums);
                const_cast<::HallServer::GameRoomMessage&>(roomInfo).set_playernum(playerNums);
                //roomInfo.set_playernum(playerNums);
//                LOG_INFO << "================roomId="<<roomId<<"=================playerNums"<<playerNums;
            }
        }
    }catch (exception &e)
    {
        LOG_ERROR << "GetGameServerplayerNum: " << e.what();
    }

}

void HallServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        int32_t num = m_connection_num.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->peerAddress().toIpPort() << "] -> 大厅服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
    }  
    else
    {
        int32_t num = m_connection_num.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->peerAddress().toIpPort() << "] -> 大厅服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
    }
}

void HallServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp receiveTime)
{
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
        // FIXME: use Buffer::peekInt32()
        const uint16_t len = buf->peekInt16();
        if(unlikely(len > PACKET_SIZE || len < sizeof(int16_t)))
        {
           LOG_ERROR << "==== len > PACKET_SIZE Invalid length ====[" << len << "]";
            if(conn)
                conn->shutdown();  // FIXME: disable reading
            break;
        }
        else if(likely(buf->readableBytes() >= len))
        {
            BufferPtr buffer(new muduo::net::Buffer(len));
            buffer->append(buf->peek(), static_cast<size_t>(len));
            buf->retrieve(len);
            TcpConnectionWeakPtr weakPtr(conn);
            m_thread_pool.run(bind(&HallServer::processRequest, this, weakPtr, buffer, receiveTime));
        }
        else
        {
            break;
        }
    }
}

void HallServer::processRequest(TcpConnectionWeakPtr &weakConn,
                                BufferPtr &buf,
                                muduo::Timestamp receiveTime)
{
    // LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;

    int32_t bufSize = buf->readableBytes();
    if(bufSize < sizeof(internal_prev_header) + sizeof(Header))
    {
        LOG_ERROR << ">>> " <<  __FUNCTION__ << " BUFFER IS NULL !!!!!!!";
        return;
    }

    // 取出从网关传过来组装的头文件
    internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

    string session(pre_header->session, sizeof(pre_header->session));
    int64_t userId = pre_header->userId;
    string aesKey(pre_header->aesKey, sizeof(pre_header->aesKey));


    // 取出从客户端发送上来的头文件
    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
    
    // 两个协议头长度
    int headLen = sizeof(internal_prev_header) + sizeof(Header);

    // 取得数据校检
    uint16_t crc = GlobalFunc::GetCheckSum((uint8_t*)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);

    // 校验数据
    if (commandHeader->len == bufSize - sizeof(internal_prev_header) 
        && commandHeader->crc == crc && commandHeader->ver == PROTO_VER 
        && commandHeader->sign == HEADER_SIGN)
    {
        //本次请求开始时间戳(微秒)
        muduo::Timestamp timestart = muduo::Timestamp::now();

        // TRACE_COMMANDID(commandHeader->mainId, commandHeader->subId);
        switch(commandHeader->mainId)
        {
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
            case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL:
            case Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER:
            {
                int mainid = commandHeader->mainId;
                int subid = commandHeader->subId;
                int id = (mainid << 8) | subid;

                switch(commandHeader->encType)
                {
                    case PUBENC__PROTOBUF_AES:
                    {
                        if(likely(m_functor_map.count(id)))
                        {
                            try
                            {
						        //解密
                                vector<uint8_t> DecryptData;
                                int32_t encodeDataLen = commandHeader->len - sizeof(Header);
                                // LOG_WARN << "===开始解密userId[" << userId << "],subid[" << subid << "],aesKey[" << aesKey << "],DataLen[" << encodeDataLen << "]";

						        int ret =  Landy::Crypto::AES_ECB_Decrypt_EX(aesKey, (unsigned char *)buf->peek()+ headLen, (int)encodeDataLen,DecryptData);
                                if (ret > 0)
                                {
                                    vector<uint8_t> DecodeData;
                                    DecodeData.resize(sizeof(Header) + DecryptData.size());
                                    memcpy(&DecodeData[0], (uint8_t *)buf->peek() + sizeof(internal_prev_header),sizeof(Header));
                                    memcpy(&DecodeData[sizeof(Header)], (uint8_t*)&DecryptData[0],DecryptData.size());

                                    AccessCommandFunctor functor = m_functor_map[id];
                                    functor(weakConn, (uint8_t*)&DecodeData[0], DecodeData.size(), pre_header);
                                }
                                else
                                    LOG_ERROR << "===<<< AES 解密失败 >>>=== ";
                            }
                            catch (exception &e)
                            {
                                LOG_ERROR << " >>> aesDecrypt Exception: " << e.what();
                                LOG_ERROR << "===开始解密userId[" << userId << "],subid[" << subid << "],aesKey[" << aesKey << "]";
                            }

                            // 打印耗时日志
                            if(subid > ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)
                            {
                                muduo::Timestamp timenow = muduo::Timestamp::now();
                                double timdiff = muduo::timeDifference(timenow, timestart); 
                                if(timdiff >  0.5)
                                   LOG_ERROR << ">>> subid["<< subid<<"],time elapse[" << timdiff<<"]";
                                else if(timdiff >  0.1)
                                   LOG_DEBUG << ">>> subid["<< subid<<"],time elapse[" << timdiff<<"]";
                            }
                            
                        }
                        else
                            LOG_ERROR <<"Not Define Command: mainId="<<mainid <<" subId="<<subid;

                        break;
                    }
                    case PUBENC__PROTOBUF_RSA:  //RSA
                    { 
                        LOG_ERROR <<"++++++++++++++PUBENC__PROTOBUF_RSA++++++++++++++ mainId=" << mainid << " subId=" << subid;
                        break;
                    }
                    case PUBENC__PROTOBUF_NONE:  //AES
                    {
                        if (subid != 1)
                            LOG_WARN << "++++++++++++++ PUBENC__PROTOBUF_NONE ++++++++++++++ mainId=" << mainid << " subId=" << subid;
                        if (likely(m_functor_map.count(id)))
                        {
                            if (subid != 1)
                                LOG_WARN << "===<<< PROTOBUF_NONE 开始处理 >>> subid[" << subid << "],aesKey[" << aesKey << "]";
                            AccessCommandFunctor functor = m_functor_map[id];
                            functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                        }
                        else
                        {
                            LOG_ERROR << "Not Define Command: mainId=" << mainid << " subId=" << subid;
                        }
                        break;
                    }
                    default:
                        break;
                    }
            }
            break;
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
               LOG_ERROR << ">>> " <<  __FUNCTION__ << " BUFFER MAINID ERROR commandHeader->mainId = " << commandHeader->mainId;
            break;
        default:
            break;
        }
    }
    else
    {
       LOG_ERROR << " " <<  __FUNCTION__ << " BUFFER LEN ERROR OR CRC ERROR! header->len="<<commandHeader->len
                 <<" bufferLen="<<bufSize - sizeof(internal_prev_header)<<" header->crc="<<commandHeader->crc<<" buffer crc="<<crc;
    }
}

//=============================work thread=========================
bool HallServer::sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data,
                          internal_prev_header *internal_header, Header *commandHeader)
{
    bool bSuccess = false;
    TcpConnectionPtr conn(weakConn.lock());
    if(likely(conn))
    {
        int size = data.size();

        // 需要加密
        if (commandHeader->encType == PUBENC__PROTOBUF_AES)
        {
            int32_t headLen = sizeof(internal_prev_header) + sizeof(Header);
            string aesKey(internal_header->aesKey, sizeof(internal_header->aesKey));

            // LOG_INFO << ">>> 加密前长度[" << size <<"],aesKey[" << aesKey << "],uid[" << internal_header->userId << "],mainid[" << commandHeader->mainId <<"],subid[" << commandHeader->subId <<"]";
            // MutexLockGuard lock(m_send_encode_data_mutex);

            vector<unsigned char> encode_data;
            int ret = Landy::Crypto::AES_ECB_Encrypt_EX(aesKey, (unsigned char *)&data[0] + headLen, (size - headLen), encode_data);
            if (ret > 0)
            {
                size_t newSize = headLen + encode_data.size();
                vector<unsigned char> senddata(newSize);
                // LOG_INFO << ">>> 加密后总长度[" << newSize <<"],加密数据长度[" << encode_data.size() <<"]";

                // 复制网关客户端协议头
                memcpy(&senddata[0], &data[0], headLen);
                memcpy(&senddata[headLen], &encode_data[0], encode_data.size());

                // 复制网关协议头
                internal_prev_header *send_internal_header = (internal_prev_header *)(&senddata[0]);
                memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
                send_internal_header->len = newSize;

                // 复制客户端协议头
                Header *commandSendHeader = (Header *)(&senddata[sizeof(internal_prev_header)]);
                memcpy(commandSendHeader, commandHeader, sizeof(Header));

                // 重设置长度
                commandSendHeader->len = newSize - sizeof(internal_prev_header);
                commandSendHeader->realSize = newSize - headLen;
                // 校验数据
                commandSendHeader->crc = GlobalFunc::GetCheckSum(&senddata[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);

                /*
                string strtemp,strbase64;
                strtemp.assign(encode_data.begin(),encode_data.end());
                Landy::Crypto::base64Encode(strtemp,strbase64);
                LOG_WARN << ">>> strbase64 <<< [" << strbase64 << "]";  
                // LOG_INFO << ">>> 加密成功 <<< realSize[" << encode_data.size() << "],crc[" << commandSendHeader->crc <<"],newSize[" << newSize <<"],len["<< commandSendHeader->len <<"]";  
                // LOG_ERROR << ">>> data.size <<< [" << senddata.size() << "]";  
                */

                // 发送数据
                conn->send(&senddata[0], senddata.size());

                bSuccess = true;
            }
            else
            {
                string strtemp, strbase64;
                strtemp.assign(encode_data.begin(), encode_data.end());
                Landy::Crypto::base64Encode(strtemp, strbase64);
                LOG_ERROR << ">>> 加密数据失败 <<< size[" << encode_data.size() << "],strbase64[" << strbase64 << "]";
            }
        }
        else if (commandHeader->encType == PUBENC__PROTOBUF_NONE)
        {
            internal_prev_header *send_internal_header = (internal_prev_header *)(&data[0]);
            memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
            send_internal_header->len = size;

            Header *commandSendHeader = (Header *)(&data[sizeof(internal_prev_header)]);
            memcpy(commandSendHeader, commandHeader, sizeof(Header));

            commandSendHeader->len = size - sizeof(internal_prev_header);
            commandSendHeader->realSize = size - sizeof(internal_prev_header) - sizeof(Header);
            commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);

            conn->send(&data[0], data.size());
            if(commandSendHeader->subId != 2) 
                LOG_INFO << ">>> 不加密数据 <<< size[" << data.size() << "],mainid[" << commandSendHeader->mainId <<"],subid[" << commandSendHeader->subId <<"]";
        }
    }

    return bSuccess;
}

string HallServer::get_random_account()
{
    return std::to_string(GlobalFunc::RandomInt64(10000000, 99999999));
}

void HallServer::cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_DEBUG << "" << __FILE__ << " "  << __FUNCTION__ ;

    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES;
    ::Game::Common::KeepAliveMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        string session = msg.session();

        ::Game::Common::KeepAliveMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        REDISCLIENT.resetExpired(session);

        response.set_retcode(0);
        response.set_errormsg("KEEP ALIVE PING OK.");

        // LOG_DEBUG << "Keep Alive Ping OK TTL:" << REDISCLIENT.TTL(session);

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
 

// 获取登录信息
void HallServer::cmd_login_servers(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    //本次请求开始时间戳(微秒) 
    muduo::Timestamp timestart = muduo::Timestamp::now();
   
    // LOG_ERROR << ">>> "<< __FUNCTION__ <<" <<<";

    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES;
    ::HallServer::LoginMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        string session = msg.session();
        string msg_session = msg.session();

        string session_header(internal_header->session, sizeof(internal_header->session));
        LOG_ERROR << "session:" << session << " header->session：" << session_header;

        ::HallServer::LoginMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        response.set_retcode(-1);
        response.set_errormsg("init");

        std::stringstream ss;
        int32_t dbHeadId = 0, dbstatus = 0, dbtxhstatus = 0, viplevel = 0, levelrate = 0, isguest = 0, minrebaterate = 0, renamecount =0,currency=0,isdtapp = 0;
        uint32_t agentId = 0, dbAgentId = 0, ip = internal_header->ip;
        int64_t userId = 0, dbUserId = 0, dbScore = 0, dbIntegralvalue = 0, superior = 0, safebox = 0, rebate = 0, totalvalidbet = 0;
        string strIP, account, dbAccount, dbNickName, dynamicPassword, strDynamicPassword, strCountry, strLocation;
        string weixinid, headurl, password, mobile, bankcardid, bankaccount, banktype, bankaddr, alipay, wxpay, invitcode;

        try
        {
            m_pIpFinder.GetAddressByIp(ntohl(ip), strLocation, strCountry);
            strIP = GlobalFunc::Uint32IPToString(ip);

            if (!Login3SCheck(session))
            {
                LOG_ERROR << "session:" << session << " " << __FUNCTION__;
                return;
            }

            do
            {
                // 1,Redis查找玩家登录记录
                if (!GetUserLoginInfoFromRedis(session, userId, account, agentId))
                {
                    LOG_ERROR << ">>> REDIS Not Find This Session:" << session;
                    response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
                    response.set_errormsg("Session Not Exist Error.");
                    DBAddUserLoginLog(userId, strIP, strLocation, 4, dbAgentId);
                    break;
                }

                // 2,数据库查找玩家信息
                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                auto maybe_result = coll.find_one(document{} << "userid" << userId << finalize);
                if (!maybe_result)
                {
                    LOG_ERROR << ">>> DB Not Find this User Session:" << session << " userId:" << dbUserId << " account:" << account << " agentId:" << agentId;
                    response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
                    response.set_errormsg("userId account Not Exist Error.");
                    // DBAddUserLoginLog(userId, strIP, strLocation, 3, dbAgentId);
                    break;
                }

                // 2.1 取字段
                bsoncxx::document::view view = maybe_result->view();
                dbUserId = view["userid"].get_int64();
                dbAccount = view["account"].get_utf8().value.to_string();
                dbAgentId = view["agentid"].get_int32();
                dbHeadId = view["headindex"].get_int32();
                dbNickName = view["nickname"].get_utf8().value.to_string();
                dbScore = view["score"].get_int64();
                dbstatus = view["status"].get_int32();
                dbtxhstatus = view["txhstatus"].get_int32();
                dbIntegralvalue = view["integralvalue"].get_int64();
                strIP = view["lastloginip"].get_utf8().value.to_string();
                //增加多币数支持
                if (view["currency"]) 
                    currency = view["currency"].get_int32();

                if (view["isdtapp"]) 
                    isdtapp = view["isdtapp"].get_int32();
               
                // 3,验证判断帐号合法
                if (userId != dbUserId || account != dbAccount || agentId != dbAgentId)
                {
                    LOG_DEBUG << ">>> This Session:" << session << " userId:" << userId << " dbUserId:" << dbUserId << " account:" << account << " dbAccount:" << dbAccount << " agentId:" << agentId << " dbAgentId:" << dbAgentId;
                    response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
                    response.set_errormsg("userId Not Exist Error.");
                    // DBAddUserLoginLog(userId, strIP, strLocation, 2, dbAgentId);
                    break;
                }

                //4,冻结帐号则不给登录
                if (dbstatus == 1)
                {
                    response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_SEAL_ACCOUNTS);
                    response.set_errormsg("Freeze Error.");                    
                    ss << "account[" << dbAccount << "],agentId[" << dbAgentId << "],userid[" << dbUserId << "] freeze!";
                    LOG_ERROR << "---玩家已经被冻结，不允许登录----msg[" << ss.str() << "]";
                    break;
                }
				m_userId_agentId_map[userId] = agentId;
				m_userId_current_map[userId] = currency;
                // 计算登录天数
                std::chrono::system_clock::time_point lastTime = view["lastlogintime"].get_date();
                int64_t days = (chrono::duration_cast<chrono::seconds>(lastTime.time_since_epoch())).count() / 3600 / 24;


                // 读取每个玩家自己玩过的游戏(数量有限，否则此方法不可取)
                std::string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_incr_gameTime) + "_" + std::to_string(userId);
                std::map<string, int32_t> values;
                if (REDISCLIENT.hgetallEx(keyName, values) && (!values.empty()))
                {
                    //把map中元素转存到vector中
                    vector<pair<string, int32_t>> gameId_vec(values.begin(), values.end());
                    std::sort(gameId_vec.begin(), gameId_vec.end(), GlobalFunc::cmpValue);

                    for (auto item : gameId_vec)
                    {
                        // LOG_INFO << "---hot game [" << item.second << " " << item.first << "]";
                        response.add_hotgameid(atoi(item.first.c_str()));
                    }
                    // LOG_INFO << ">>> hgetallEx,keyName[" << keyName << "]";
                }

                //====================================================
                // string session(internal_header->session, sizeof(internal_header->session));
                Json::Value root;
                root["userId"] = userId;
                root["session"] = session_header;//session;
                Json::FastWriter writer;
                string msg = writer.write(root);
                // 广播登录信息
                REDISCLIENT.publishUserLoginMessage(msg);
                // 存储用户对应的session
                REDISCLIENT.SetUserLoginInfo(userId, "msg_session", msg_session);
                // 设置过期时间
                REDISCLIENT.resetExpired(session);
                // 保存玩家与代理关系
                REDISCLIENT.SetProxyUser(dbUserId, REDIS_UID_AGENTID, to_string(dbAgentId), 600000);
                // 保存玩家的币种信息
                REDISCLIENT.SetCurrencyUser(dbUserId, REDIS_UID_CURRENCY, to_string(currency), 60000000);
                // 有鸟毛用？
                dynamicPassword = Landy::Crypto::generateAESKey();
                strDynamicPassword = Landy::Crypto::BufferToHexString((unsigned char *)dynamicPassword.c_str(), dynamicPassword.size());
                REDISCLIENT.SetUserLoginInfo(userId, "dynamicPassword", strDynamicPassword);
                // 更新玩家登录时间
                REDISCLIENT.SetUserLoginInfo(userId, "LoginDateTime", to_string(chrono::system_clock::to_time_t(chrono::system_clock::now())));
                //====================================================
                // 获取玩家对应幸运转盘的配置
				tagConfig lkGame_config;
				memset(&lkGame_config, 0, sizeof(lkGame_config));
				lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

                response.set_userid(dbUserId);
                response.set_account(dbAccount);
                response.set_agentid(dbAgentId);
                response.set_nickname(dbNickName);
                response.set_headid(dbHeadId);
                response.set_currency(currency);
                
                response.set_score(dbScore);
                response.set_jifenvalue(dbIntegralvalue / (lkGame_config.nExChangeRate * COIN_RATE)); //100 写个全局的币率
                response.set_gamepass(dynamicPassword);
                response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_OK);
                response.set_errormsg("User Login OK.");

                // 更新玩家信息
                DBUpdateUserLoginInfo(userId, strIP, days);
                DBAddUserLoginLog(userId, strIP, strLocation, 0, dbAgentId);
                
                internal_header->nOK = 1;

                // 删除用户登录记录信息
                deleteUserLoginInfo(userId);
                // 更新跳转记录
                if (dbtxhstatus != 0)
                     LOG_INFO << "---跳转记录,玩家[" << userId << "],游戏["<< dbtxhstatus <<"]";


                // 大唐APP才读取
                if (isdtapp == eDTapp::op_NO){
                    // 大唐APP才往后走下面流程
                    break;
                }

                mongocxx::collection user_bank_info_coll = MONGODBCLIENT["gamemain"]["user_bank_info"];
                mongocxx::cursor info = user_bank_info_coll.find(document{} << "userid" << userId << finalize);
                for (auto &doc : info)
                {
                    string _cardtype, _cardid, _cardaddr, _qrcode, _cardname;
                    ::HallServer::CardItemInfo *carditem = response.add_carditem();
                    if (doc["cardtype"] && doc["cardtype"].type() != bsoncxx::type::k_null)
                        _cardtype = doc["cardtype"].get_utf8().value.to_string();
                    if (doc["cardid"] && doc["cardid"].type() != bsoncxx::type::k_null)
                        _cardid = doc["cardid"].get_utf8().value.to_string();
                    if (doc["cardaddr"] && doc["cardaddr"].type() != bsoncxx::type::k_null)
                        _cardaddr = doc["cardaddr"].get_utf8().value.to_string();
                    if (doc["qrcode"] && doc["qrcode"].type() != bsoncxx::type::k_null)
                        _qrcode = doc["qrcode"].get_utf8().value.to_string();
                    if (doc["cardname"] && doc["cardname"].type() != bsoncxx::type::k_null)
                        _cardname = doc["cardname"].get_utf8().value.to_string();

                    carditem->set_type(doc["type"].get_int32());               //账户类型（建索引,1=银行卡,0=支付宝,2=微信,3=USDT ）
                    carditem->set_cardtype(_cardtype);                         //银行名称/空/空/虚拟币协议
                    carditem->set_cardid(_cardid);                             //银行卡帐号/支付宝帐户/微信帐户/USDT唯一标识
                    carditem->set_firstchoice(doc["firstchoice"].get_int32()); //是否默认使用，0非默认，1默认
                    carditem->set_cardaddr(_cardaddr);                         //支行地址/空/空/虚拟币
                    carditem->set_qrcode(_qrcode);                             //二维码（base64）
                    carditem->set_cardname(_cardname);                         //银行卡帐户/支付宝收款人姓名/微信收款人姓名/虚拟币名称
                }

                if (view["weixinid"])  weixinid = view["weixinid"].get_utf8().value.to_string();
                if (view["headurl"]) headurl = view["headurl"].get_utf8().value.to_string();
                if (view["totalvalidbet"]) totalvalidbet = view["totalvalidbet"].get_int64();
                if (view["mobile"]) mobile = view["mobile"].get_utf8().value.to_string();
                if (view["invitcode"])  invitcode = view["invitcode"].get_utf8().value.to_string();
                if (view["superior"])  superior = view["superior"].get_int64();
                if (view["rebate"]) rebate = view["rebate"].get_int64();
                if (view["viplevel"]) viplevel = view["viplevel"].get_int32();
                if (view["isguest"]) isguest = view["isguest"].get_int32();
                if (view["safebox"])  safebox = view["safebox"].get_int64();
                if (view["minrebaterate"])  minrebaterate = view["minrebaterate"].get_int32();
                if (view["renamecount"])  renamecount = view["renamecount"].get_int32();

                response.set_weixinid(weixinid);
                response.set_headurl(headurl);
                response.set_password(password);
                response.set_mobile(mobile);
                response.set_isguest(isguest);
                response.set_safebox(safebox);
                response.set_invitcode(invitcode);
                response.set_superior(superior);
                response.set_rebate(rebate);
                response.set_minrebate(minrebaterate);
                response.set_rename(renamecount);

                // 找到缓存信息
                int32_t curLevel = 0, aid = (int32_t)dbAgentId;
                getLevelInfo(totalvalidbet, aid, curLevel, levelrate);
                LOG_INFO << "---viplevel[" << viplevel << "],curLevel[" << curLevel << "],isguest[" << isguest <<"],isdtapp[" << isdtapp << "]";
                if (viplevel != curLevel) //数据库与计算出来的不一样
                {
                    viplevel = curLevel;
                    if (!coll.update_one(make_document(kvp("userid", userId)), make_document(kvp("$set", make_document(kvp("viplevel", viplevel))))))
                        LOG_ERROR << "---更新玩家等级失败,viplevel[" << viplevel << "]";
                }

                // 设置玩家等级
                response.set_level(viplevel);
                response.set_lvlrate(levelrate);

                // 设置免费领取送金
                markGetFreeCoin(userId);

            } while (0);
        }
        catch (exception &e)
        {
            LOG_ERROR << " >>> SQLException: " << e.what() << ", session:" << session << ", userid: " << userId << ", account:" << account;
            response.set_retcode(::HallServer::LoginMessageResponse::LOGIN_NETBREAK);
            response.set_errormsg("Database Update Error.");
        }

        // 返回登录用户ID
        internal_header->userId = userId;

        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }

        // 打印日志
        muduo::Timestamp timenow = muduo::Timestamp::now();
        double timdiff = muduo::timeDifference(timenow, timestart);
        LOG_WARN << ">>> 登录耗时(秒)[" << timdiff << "],userId[" << userId << "],account[" << account << "],agentId[" << agentId << "],session[" << session << "]";
    }
    else
    {
        LOG_ERROR << ">>> 解析包出错 <<<";
    }
}


// 如果没有记录则记录
void HallServer::markGetFreeCoin(int64_t uid)
{
    // 北京时间
    time_t curstamp = time(nullptr);  // UTC秒数
    tm *tp = localtime(&curstamp);  

    redis::RedisValue values;
    string fields[2] = {timestampKey, countKey};

    string strKeyName = makeRedisKey(uid,eRedisKey::has_user_info);


    // UserData[UserEnum::lasttimestamp];

    auto resetKeyValue = [&]() {
        int32_t gettimes = 3;
        int64_t lasttimestamp = (int64_t)getZeroTimeStamp(tp);
        redis::RedisValue redisValue;
        redisValue[timestampKey] = std::to_string(lasttimestamp);
        redisValue[countKey] = std::to_string(gettimes);
        int32_t timeout = 30 * 24 * 3600; // 保留一个月
        if (!REDISCLIENT.hmset(strKeyName, redisValue, timeout))
        {
            LOG_ERROR << " --- " << __FUNCTION__ << " 设置免费领取金币记录失败,uid[" << uid << "]";
        }
        LOG_INFO << " --- " << __FUNCTION__ << ",keyName["<< strKeyName <<"] 重置时间。";
    };

    if (!REDISCLIENT.exists(strKeyName))
    {
        resetKeyValue();
        return;
    }

    bool res = REDISCLIENT.hmget(strKeyName, fields, 2, values);
    if (res ) 
    {
        int64_t lasttimestamp = values[timestampKey].asInt64();
        // 两次时间戳差 (1天 86400秒) = 当前日期零点时间戳  - 上次签到日期零点时间戳
        int64_t duration = getZeroTimeStamp(tp) - lasttimestamp;
        if (duration > 0)
        {
            resetKeyValue();
        }
        LOG_INFO << " --- " << __FUNCTION__ << ",今天["<< tp->tm_mday <<"],当前时间["<< getZeroTimeStamp(tp) <<"],上次时间["<< lasttimestamp <<"],duration["<< duration <<"]";
    }
}

bool HallServer::getLevelInfo(int64_t totalvalidbet,int32_t aid,int32_t & viplevel,int32_t & levelrate)
{
    if (m_LevelCfgMap.find(aid) == m_LevelCfgMap.end())
    {
        vector<LevelCfg> LevelCfgItem;
        // 正序
        mongocxx::options::find opts = mongocxx::options::find{};
        opts.sort(document{} << "levelnumber" << 1 << finalize);

        auto filter = make_document(kvp("agentid", aid));
        mongocxx::collection level_coll = MONGODBCLIENT["gamemain"]["membership_level"];
        mongocxx::cursor level_info = level_coll.find(filter.view(), opts);
        for (auto doc : level_info)
        {
            LevelCfg levelitem;
            levelitem.levelname = doc["levelname"].get_utf8().value.to_string();
            levelitem.levelnumber = doc["levelnumber"].get_int32();
            levelitem.upgradereward = doc["upgradereward"].get_int64();
            levelitem.upgradevalue = doc["upgradevalue"].get_int64();
            LevelCfgItem.push_back(levelitem);
            // LOG_INFO << "---levelname[" << levelitem.levelname << "],levelnumber[" << levelitem.levelnumber << "],upgradevalue[" << levelitem.upgradevalue << "],size[" << LevelCfgItem.size() << "]";
        }
        {
            MutexLockGuard lock(level_mutex);
            m_LevelCfgMap[aid] = LevelCfgItem; // 缓存到内存
        }
    }
    {
        MutexLockGuard lock(level_mutex);
        auto it = m_LevelCfgMap.find(aid);
        if (it != m_LevelCfgMap.end())
        {
            LevelCfg levelCfg_ = {0};
            for (auto & levelCfg : it->second)
            {
                // LOG_INFO << "---aid["<<aid<<"],["<< totalvalidbet <<"],viplevel[" << levelCfg.levelnumber << "],levelrate[" << levelCfg.levelname << "],uv1[" << levelCfg.upgradevalue << "],uv0[" << levelCfg_.upgradevalue << "]";
                // 找出洗码量最大的级别 
                if (totalvalidbet < levelCfg.upgradevalue * COIN_RATE)
                {
                    // 获取前一等级的值
                    viplevel = levelCfg.levelnumber == 0 ? levelCfg.levelnumber : levelCfg.levelnumber - 1;
                    int64_t incVar = totalvalidbet - levelCfg_.upgradevalue * COIN_RATE;
                    levelrate = (int64_t)((incVar * 1.0f) / (levelCfg.upgradevalue - levelCfg_.upgradevalue));
                    //                      找到bet[11504850000],viplevel[12],levelrate[-11],uv1[150000000],uv0[80000000] 
                    LOG_INFO << "---找到bet["<< totalvalidbet <<"],viplevel[" << viplevel << "],levelrate[" << levelrate << "],uv1[" << levelCfg.upgradevalue << "],uv0[" << levelCfg_.upgradevalue << "]";
                    break;
                }

                // 防止超过个人洗码量无限大
                levelCfg_ = levelCfg;          //保留前一等级的配置
                // 获取前一等级的值
                viplevel = levelCfg.levelnumber == 0 ? levelCfg.levelnumber : levelCfg.levelnumber - 1;
                levelrate = 100;
                // LOG_INFO << "---2 aid["<<aid<<"],["<< totalvalidbet <<"],viplevel[" << levelCfg.levelnumber << "],viplevel[" << viplevel << "]";
            }
        }
    }

    return true;
}

// 记录玩家登录情况
// @recordType 0,记录gamehandletime;1,记录tokenhandletime
void HallServer::deleteUserLoginInfo(int64_t userid)
{
	try
	{
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_login_check_list"];
        if (!coll.delete_one(document{} << "userid" << userid << finalize))
        {
            LOG_ERROR << "---删除玩家登录情况记录失败,userid[" << userid << "] " << __FUNCTION__;
        }
        else
        {
            LOG_INFO << "---玩家登录成功,删除记录userid[" << userid << "]";
        }
    }
    catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what();
	}
}
// 跳转记录，登录成功后则更新状态
void HallServer::updatePalyingRecord(int64_t userid)
{
 
}
// 从redis获取登录信息(API写入)
bool HallServer::GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId)
{
    string value;
    bool ret = REDISCLIENT.get(session, value);
    if(ret)
    {
        Json::Reader reader;
        Json::Value root;
        if(reader.parse(value, root))
        {
            userId = root["userid"].asInt64();
            account = root["account"].asString();
            agentId = root["agentid"].asUInt();
            return true;
        }
    }
    return false;
}


bool HallServer::Login3SCheck(const string &session)
{
    string key = REDIS_LOGIN_3S_CHECK + session;
    if(REDISCLIENT.exists(key))
    {
        LOG_ERROR <<"对不起，您登陆太频繁，请稍后再试. session="<<session;
        return false;
    }else
    {
        REDISCLIENT.set(key, key, 3);
        return true;
    }
}

bool HallServer::DBAddUserLoginLog(int64_t userId, string &strIp, string &address, uint32_t status,uint32_t agentId)
{
    bool bSuccess = false;
    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["login_log"];
        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << userId
                << "loginip" << strIp
                << "address" << address
                << "status" << (int32_t)status
                << "agentid" <<(int32_t) agentId
                << "logintime" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(insert_value.view());
        if( !_result )
        {
            LOG_ERROR << "Insert error:"<< bsoncxx::to_json(insert_value);
        }

        bSuccess = true;
    }
    catch(exception &e)
    {  
        LOG_ERROR << "exception: " << e.what();
    }
    return bSuccess;
}

bool HallServer::DBUpdateUserLoginInfo(int64_t userId, string &strIp, int64_t days)
{
    bool bSuccess = false;
    int64_t nowDays = (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch())).count()/3600/24;
    try
    {
        bsoncxx::document::value findValue = document{} << "userid" << userId << finalize;
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        if(days + 1 == nowDays)
        {
            bsoncxx::document::value updateValue = document{}
                            << "$set" << open_document
                                << "lastlogintime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                //<< "lastloginip" << strIp
                                << "onlinestatus" << bsoncxx::types::b_int32{1}
                                << close_document
                            << "$inc" << open_document
                                << "activedays" << 1
                                << "keeplogindays" << 1 << close_document
                            << finalize;
           // LOG_DEBUG << "Update Document:"<<bsoncxx::to_json(updateValue);
            coll.update_one(findValue.view(), updateValue.view());
        }else if(days == nowDays)
        {
            bsoncxx::document::value updateValue = document{}
                            << "$set" << open_document
                                << "lastlogintime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                //<< "lastloginip" << strIp
                                << "onlinestatus" << bsoncxx::types::b_int32{1}
                                << close_document
                            << finalize;
           // LOG_DEBUG << "Update Document:"<<bsoncxx::to_json(updateValue);
            coll.update_one(findValue.view(), updateValue.view());
        }else
        {
            bsoncxx::document::value updateValue = document{}
                            << "$set" << open_document
                                << "lastlogintime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                //<< "lastloginip" << strIp
                                << "keeplogindays" << bsoncxx::types::b_int32{1}
                                << "onlinestatus" << bsoncxx::types::b_int32{1}
                                << close_document
                            << "$inc" << open_document
                                << "activedays" << 1 << close_document
                            << finalize;
          //  LOG_DEBUG << "Update Document:"<<bsoncxx::to_json(updateValue);
            coll.update_one(findValue.view(), updateValue.view());
        }
        bSuccess = true;
    }catch(exception &e)
    {
        LOG_ERROR << "exception: " << e.what();
    }
    return bSuccess;
}
//获取不同代理玩家登陆返回信息
void HallServer::getUserResponeLogginServer()
{

}
// 获取game_kind信息
void HallServer::cmd_get_game_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_INFO << "--- *** " << __FUNCTION__;

    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_RES;
    ::HallServer::GetGameMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::GetGameMessageResponse response;
        {
            READ_LOCK(m_game_info_mutex);
            string agentId="0";
            string currency="0";
            int64_t userId=internal_header->userId;          
            REDISCLIENT.GetCurrencyUser(userId,REDIS_UID_CURRENCY,currency);
            int64_t currenNum = atol(currency.c_str());
            if(currenNum!=0)
            {
                std::map<int32_t,::HallServer::GetGameMessageResponse>::iterator it=m_crrentInfoSendMap.find(currenNum);
                if(it!=m_crrentInfoSendMap.end())
                {
                    response.CopyFrom(m_crrentInfoSendMap[currenNum]);
                    LOG_ERROR<<" 找到币种配置   currenNum=="<<currenNum;
                }
                else
                {
                    LOG_ERROR<<" 找不到币种配置   currenNum== "<<currenNum;
                }
            }
            else
            {
                if(REDISCLIENT.GetProxyUser(userId,REDIS_UID_AGENTID,agentId))
                {
                    //int64_t aid=stoull(agentId);
                    int64_t aid = atol(agentId.c_str());
                    std::map<int32_t,::HallServer::GetGameMessageResponse>::iterator it=m_proxyInfoSendMap.find(aid);
                    if(it!=m_proxyInfoSendMap.end())
                    {
                        response.CopyFrom(m_proxyInfoSendMap[aid]);
                        LOG_ERROR<<" 找到代理配置    agentId=="<<aid;
                    }
                    else
                    {
                        response.CopyFrom(m_proxyInfoSendMap[0]);//发送默认值
                        LOG_ERROR<<" 找不到代理配置,使用零号配置";
                    }
                }
                else
                {
                    response.CopyFrom(m_proxyInfoSendMap[0]);//发送默认值
                    LOG_ERROR<<" redis找不到代理号,使用零号配置";
                }
            }


        }
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        response.set_retcode(0);
        response.set_errormsg("Get Game Message OK!");
//Cleanup:
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }

        LOG_INFO << ">>> "<< __FUNCTION__ << ",Get Game Message OK!";
    }
}

void HallServer::cmd_get_match_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_RES;
    //获取 userId ///
    int64_t userId = internal_header->userId;
    //偶尔 userId == -1 why? ///
    //assert(userId > 0);
    //获取 session ///
    std::string session = internal_header->session;
    std::string account;
    uint32_t agentId = 0;
    
// 	if (!Login3SCheck(session)) {
// 		return;
// 	}
    // LOG_INFO << "--- *** " << "userId = " << userId << " session = " << session;
    //首先根据玩家session从redis中获取所在agentId ///
    int64_t t_userId_ = 0;
	bool succ = GetUserLoginInfoFromRedis(session, t_userId_, account, agentId);
    if (!succ && userId > 0) {
        try {
            //没有则根据userId从mongdb中获取所在agentId ///
            mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(bsoncxx::builder::stream::document{} <<"userid" << 1 <<"agentid" << 1 << bsoncxx::builder::stream::finalize);
 
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
            auto query_value = document{} << "userid" << userId << finalize;
            bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view(),opts);
            if (maybe_result) {
                bsoncxx::document::view view = maybe_result->view();
                agentId = view["agentid"].get_int32();
            }
        }
        catch (const std::exception &e)
        {   
            LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
        } 
	} 
         
    LOG_INFO << "--- *** " << "userId = " << userId << " agentId = " << agentId ;
    //assert(implicit_cast<int64_t>(agentId) > 0);
    ::HallServer::GetMatchMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::HallServer::GetMatchMessageResponse response;
        //比赛系统对该agentId屏蔽
        bool shieldAll = false;
        READ_LOCK(m_shieldAgentIds_mutex);
		//对需要屏蔽的代理都不开放比赛系统 ///
		std::map<int32_t, std::vector<uint32_t> >::const_iterator it = m_shieldAgentIds.find(0);
		if (it != m_shieldAgentIds.end()) {
			//该agentId是否在比赛系统的屏蔽列表中
			std::vector<uint32_t> const& shieldAgentIds = it->second;
			if (agentId > 0 && std::find(
				std::begin(const_cast<std::vector<uint32_t>&>(shieldAgentIds)),
				std::end(const_cast<std::vector<uint32_t>&>(shieldAgentIds)), agentId) != shieldAgentIds.end()) {
				//该agentId在比赛系统的屏蔽列表中 ///
                LOG_DEBUG << "--- *** " << "userId = " << userId << " agentId = " << agentId << " 在比赛系统的屏蔽列表中";
				//response.clear_matchmessage();
                shieldAll = true;
			}
			else {
				//比赛系统针对该agentId开放 ///
                LOG_DEBUG << "--- *** " << "比赛系统对 userId = " << userId << " agentId = " << agentId << " 开放";
			}
		}
        //比赛系统对该agentId开放
        if (!shieldAll) {
            ::HallServer::GetMatchMessageResponse allMatchServers;
			{
				READ_LOCK(m_match_info_mutex);
				allMatchServers.CopyFrom(m_MatchResp);
			}
			//遍历每个比赛房间 ///
			for (int i = 0; i < allMatchServers.matchmessage_size(); ++i) {
				::HallServer::MatchMessage const& curMatch = allMatchServers.matchmessage(i);
				//当前比赛房间 ///
				uint32_t matchId = curMatch.roomid();
				//是否有要屏蔽的代理 ///
				std::map<int32_t, std::vector<uint32_t> >::const_iterator it = m_shieldAgentIds.find(matchId);
				if (it != m_shieldAgentIds.end()) {
					//assert(it->first > 0);
					//assert(it->first == matchId);
					//该agentId是否在当前比赛房间的屏蔽列表中
					std::vector<uint32_t> const& shieldAgentIds = it->second;
					if (agentId > 0 && std::find(
						std::begin(const_cast<std::vector<uint32_t>&>(shieldAgentIds)),
						std::end(const_cast<std::vector<uint32_t>&>(shieldAgentIds)), agentId) != shieldAgentIds.end()) {
						//该agentId在当前比赛房间的屏蔽列表中 ///
						LOG_INFO << "--- *** " << "userId = " << userId << " agentId = " << agentId
							<< " 在 " << curMatch.matchname() << "[" << matchId << "] 的屏蔽列表中";
					}
					else {
						//当前比赛房间对该agentId开放 ///
						response.add_matchmessage()->CopyFrom(curMatch);
                        LOG_DEBUG << "--- *** " << curMatch.matchname() << "[" << matchId << "] 对 userId = " << userId << " agentId = " << agentId << " 开放";
					}
				}
				else {
					//没有要屏蔽的代理 ///
					response.add_matchmessage()->CopyFrom(curMatch);
				}
			}
        }


        ::Game::Common::Header header = msg.header();
		::Game::Common::Header* resp_header = response.mutable_header();
		resp_header->CopyFrom(header);
        response.set_retcode(0); 
        response.set_errormsg("Get Match Game Message OK!");
//Cleanup:
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 获取玩家在线情况
void HallServer::cmd_get_playing_game_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
//    LOG_DEBUG << "" << __FILE__ << " "  << __FUNCTION__  ;

    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_RES;
    ::HallServer::GetPlayingGameInfoMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        int64_t userId = internal_header->userId;
        // LOG_DEBUG << "userId: " << userId;
        ::HallServer::GetPlayingGameInfoMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        uint32_t redisGameId = 0, redisRoomId = 0, redisTableId = 0;
        if(REDISCLIENT.GetUserOnlineInfo(userId, redisGameId, redisRoomId, redisTableId) )
        {
            response.set_gameid(redisGameId);
            response.set_roomid(redisRoomId);

            response.set_retcode(0);
            response.set_errormsg("Get Playing Game Info OK.");
            LOG_INFO << ">>> Get Playing Game Info OK,gameid " << redisGameId <<" roomid " << redisRoomId << " userId: " << userId;
        }
        else
        {
            response.set_retcode(1);
            response.set_errormsg("This User Not In Playing Game!");
            LOG_INFO << ">>> This User Not In Playing Game! " << redisGameId <<" roomid " << redisRoomId << " userId: " << userId;
        }
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}



// 检查代理线是否维护waa
void HallServer::refreshRepairAgentInfo()
{
	agent_ids.clear();
	try
	{
		LOG_WARN << "---" << __FUNCTION__ << "---------------";

		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["maintain_proxy"];
		auto findvalue = coll.find_one(document{} << "apiswitch" << 0 << finalize);
		if (findvalue)
		{
			auto findview = findvalue->view();
			auto t_agent_ids = findview["proxylist"].get_array();
			LOG_INFO << "-------t agent ids get_array--------";
			{
				MutexLockGuard lock(m_mutex_ids);
				for (auto &agent_id : t_agent_ids.value)
				{
					agent_ids.push_back(agent_id.get_int32());
					LOG_INFO << "agent ids:" << agent_id.get_int32();
				}
			}

			LOG_WARN << "---" << __FUNCTION__ << " end---------------";
		}
	}
	catch (exception &e)
	{
		LOG_ERROR << "---refresh Repair Agent Info: " << e.what();
	}
}

// 检查代理线是否维护
bool HallServer::checkAgentStatus(int32_t agentid)
{
	MutexLockGuard lock(m_mutex_ids);
    // 遍历判断
    for (int32_t _agentId : agent_ids)
    {
        if (agentid == _agentId)
        {
            LOG_WARN << "---代理线[" << agentid <<"]维护中！";
            return false;
        }
    }

	return true;
}
int HallServer::GetRedEnvelopeRainStatus(int64_t &lefttime,int32_t agentid,int64_t &allscores,int64_t &nestopentime ,int64_t &nestclosetime,int64_t &activeopentime,int64_t &activeclosetime,int64_t userid)
{
    // 获取时间点，并且判断是否已经到达时间点
    //获取目前秒时间
    time_t tx = time(NULL);
    struct tm * local;
    //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
    local = localtime(&tx);
    struct tm *now = new tm;
    now->tm_year = local->tm_year;
    now->tm_mon = local->tm_mon;
    now->tm_mday = local->tm_mday;
    now->tm_hour = local->tm_hour;
    now->tm_min = local->tm_min;
    now->tm_sec = local->tm_sec;

    LOG_ERROR<<"本地时间  年:"<< now->tm_year <<"    月:"
            <<now->tm_mon+1<<"    日:  " <<now->tm_mday<<  "   时间:"<<local->tm_hour<<"   分钟"<<now->tm_min;
    //算出当前时间戳,以及计算数据库配置的时间的时间戳
    //这个函数是时间点转换成时间戳
    time_t shijianlong= mktime(now);
     LOG_ERROR<<"mktime时间戳"<<shijianlong;
     shijianlong = tx;
     LOG_ERROR<<"本地时间戳"<<shijianlong;
    //读取配置的四个发红包时间点

    if(m_ProxyStartTime.find(agentid)!=m_ProxyStartTime.end())
    {

    }
    else
    {
        LOG_ERROR<<"读不到代理信息";
        delete now;
        return 0;
    }
    //从小到大排序一下
    //sort(m_ProxyStartTime[agentid].begin(),m_ProxyStartTime[agentid].end());
    //sort(m_ProxyEndTime[agentid].begin(),m_ProxyEndTime[agentid].end());

    for(int i=0;i<m_ProxyStartTime[agentid].size();i++)
    {
        int64_t sortInt = 0;
        for(int j=i+1;j<m_ProxyStartTime[agentid].size();j++)
        {
            if(m_ProxyStartTime[agentid][i]>=m_ProxyStartTime[agentid][j])
            {
                sortInt = m_ProxyStartTime[agentid][i];
                m_ProxyStartTime[agentid][i]=m_ProxyStartTime[agentid][j];
                m_ProxyStartTime[agentid][j] = sortInt;
            }

        }
    }
    for(int i=0;i<m_ProxyEndTime[agentid].size();i++)
    {
        int64_t sortInt = 0;
        for(int j=i+1;j<m_ProxyEndTime[agentid].size();j++)
        {
            if(m_ProxyEndTime[agentid][i]>=m_ProxyEndTime[agentid][j])
            {
                sortInt = m_ProxyEndTime[agentid][i];
                m_ProxyEndTime[agentid][i]=m_ProxyEndTime[agentid][j];
                m_ProxyEndTime[agentid][j] = sortInt;
            }

        }
    }
    if(m_ProxyRedStandard.find(agentid)!=m_ProxyRedStandard.end())
    {

    }
    else
    {
        LOG_ERROR<<"所有配置信息配置不对";
        delete now;
        return 0;
    }
    sort(m_ProxyRedStandard[agentid].begin(),m_ProxyRedStandard[agentid].end(),SortFuc);

    activeopentime = m_ProxyRedStandard[agentid][0].activityTimeStart;
    activeclosetime = m_ProxyRedStandard[agentid][0].activityTimeEnd;
    nestopentime = m_ProxyStartTime[agentid][0];
    nestclosetime = m_ProxyEndTime[agentid][0];
    allscores = m_ProxyRedStandard[agentid][0].totalScore;
    //时间戳算出年月日
    {


        for(int i=0;i<m_ProxyStartTime[agentid].size();i++)
        {
            struct tm * starttime;

            time_t txxx = (time_t)m_ProxyStartTime[agentid][i];
            starttime=gmtime(&txxx);; //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
            struct tm *t = new tm;
            t->tm_year =now->tm_year ;
            t->tm_mon = now->tm_mon;
            t->tm_mday =now->tm_mday;
            t->tm_hour = starttime->tm_hour;
            t->tm_min = starttime->tm_min;
            t->tm_sec = starttime->tm_sec;

            LOG_ERROR<<"年: "<<now->tm_year<<" 月   " <<now->tm_mon+1<<"   日"<< now->tm_mday
                    <<"    时" <<starttime->tm_hour<<"  分"<< starttime->tm_min<<"   秒"<<starttime->tm_sec;
            m_ProxyStartTime[agentid][i]= (int64_t)mktime(t);
            delete t;
        }
        for(int i=0;i<m_ProxyEndTime[agentid].size();i++)
        {
            struct tm * starttime;

            time_t txxx = (time_t)m_ProxyEndTime[agentid][i];

            starttime=gmtime(&txxx);
            //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r

           struct tm *t = new tm;


            t->tm_year = now->tm_year ;
            t->tm_mon = now->tm_mon;
            t->tm_mday = now->tm_mday;
            t->tm_hour = starttime->tm_hour;
            t->tm_min = starttime->tm_min;
            t->tm_sec = starttime->tm_sec;

            LOG_ERROR<<"结束年: "<<now->tm_year<<" 结束月   " <<now->tm_mon+1<<"  结束日"<< now->tm_mday
                    <<"    结束时" <<starttime->tm_hour<<"  结束分"<< starttime->tm_min<<"   结束秒"<<starttime->tm_sec;
            m_ProxyEndTime[agentid][i]= (int64_t)mktime(t);
            delete t ;
        }
    }
    int ret =0;
    int32_t vecsize = (int32_t)m_ProxyStartTime[agentid].size();
    if(vecsize<=1)
    {
        LOG_ERROR<<"配置不对";
        if(shijianlong<m_ProxyStartTime[agentid][0])
        {
            lefttime = m_ProxyStartTime[agentid][0]-shijianlong;
            nestopentime = m_ProxyStartTime[agentid][0];
            nestclosetime = m_ProxyEndTime[agentid][0];
            ret  = 0;
            LOG_ERROR<<"在2729行: "<<ret  <<"     shijianlong"<<shijianlong<<"    m_ProxyStartTime[agentid][0]"<<m_ProxyStartTime[agentid][0];
        }
        else if(shijianlong>=m_ProxyStartTime[agentid][0]&&shijianlong<m_ProxyEndTime[agentid][0])
        {
            lefttime = m_ProxyEndTime[agentid][0]-shijianlong;
            nestopentime = m_ProxyStartTime[agentid][0]+24*60*60;
            nestclosetime = m_ProxyEndTime[agentid][0]+24*60*60;
            ret  =1;
            LOG_ERROR<<"在2737行: "<<ret<<"    shijianlong"<<shijianlong<<"     m_ProxyEndTime[agentid][0]"<<m_ProxyEndTime[agentid][0];
            string fields[8] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET,REDIS_USER_TODAY_UPER,REDIS_USER_LEFT_TIMES,
                               REDIS_USER_SPRING_STATUS,REDIS_USER_SUMMER_STATUS,REDIS_USER_FALL_STATUS,
                               REDIS_USER_WINTER_STATUS};
            redis::RedisValue values;
            string keyUserInfo=REDIS_USER_BASIC_INFO+to_string(userid);
            int res = REDISCLIENT.hmget(keyUserInfo, fields, 8, values);
            if(res)
            {
                int32_t springRain= values[REDIS_USER_SPRING_STATUS].asInt();
                if(springRain)
                {
                    ret =2;
                    LOG_ERROR<<"在2750行: "<<ret;
                }
            }
        }
        else
        {
            lefttime =24*60*60+(shijianlong-m_ProxyEndTime[agentid][0]);
            nestopentime = m_ProxyStartTime[agentid][0];
            nestclosetime = m_ProxyEndTime[agentid][0];
            ret  = 0;
            LOG_ERROR<<"在2760行: "<<ret  <<"     shijianlong"<<shijianlong;
        }
    }
    else
    {
        LOG_ERROR<<"配置对";
        if(shijianlong<m_ProxyStartTime[agentid][0])
        {
            lefttime = m_ProxyStartTime[agentid][0]-shijianlong;
            nestopentime = m_ProxyStartTime[agentid][0];
            nestclosetime = m_ProxyEndTime[agentid][0];
            ret  = 0;
            LOG_ERROR<<"在2772行: "<<ret<< "    shijianlong"<<shijianlong<<"    lefttime"<<lefttime;
        }
        else if(shijianlong>m_ProxyEndTime[agentid][vecsize-1])
        {
            lefttime = m_ProxyStartTime[agentid][0]+24*60*60-shijianlong;
            nestopentime = m_ProxyStartTime[agentid][0]+24*60*60;
            nestclosetime = m_ProxyEndTime[agentid][0]+24*60*60;
            ret  = 0;
            LOG_ERROR<<"在2790行: "<<ret<<"      shijianlong"<<shijianlong<< "    lefttime"<<lefttime;
        }
        else
        {
            LOG_ERROR<<"今天没结束";
            for(int i=0;i<vecsize;i++)
            {
                if(i+1<=vecsize-1)
                {
                    LOG_ERROR<<"i+1<vecsize-1";
                    if(shijianlong>m_ProxyEndTime[agentid][i]&&shijianlong<m_ProxyStartTime[agentid][i+1])
                    {
                        lefttime = m_ProxyStartTime[agentid][i+1]-shijianlong;
                        nestopentime = m_ProxyStartTime[agentid][i+1];
                        nestclosetime = m_ProxyEndTime[agentid][i+1];
                        ret  = 0;
                        LOG_ERROR<<"在2805行: "<<ret<<"    lefttime"<<lefttime;
                         break;
                    }
                }

                if(shijianlong>=m_ProxyStartTime[agentid][i]&&shijianlong<=m_ProxyEndTime[agentid][i])
                {
                    LOG_ERROR<<"shijianlong>=m_ProxyStartTime[agentid][i]&&shijianlong<=m_ProxyEndTime[agentid][i]";
                    lefttime = m_ProxyEndTime[agentid][i]-shijianlong;
                    if(i+1<vecsize-1)
                    {
                        nestopentime = m_ProxyStartTime[agentid][i+1];
                        nestclosetime = m_ProxyEndTime[agentid][i+1];
                    }
                    ret  =1;
                    LOG_ERROR<<"在2819行: "<<ret<< "   lefttime"<<lefttime;
                    string fields[8] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET,REDIS_USER_TODAY_UPER,REDIS_USER_LEFT_TIMES,
                                       REDIS_USER_SPRING_STATUS,REDIS_USER_SUMMER_STATUS,REDIS_USER_FALL_STATUS,
                                       REDIS_USER_WINTER_STATUS};
                    redis::RedisValue values;
                    string keyUserInfo=REDIS_USER_BASIC_INFO+to_string(userid);
                    int res = REDISCLIENT.hmget(keyUserInfo, fields, 8, values);
                    if(res)
                    {
                        int32_t springRain= values[REDIS_USER_SPRING_STATUS].asInt();
                        int32_t sumRain= values[REDIS_USER_SUMMER_STATUS].asInt();
                        int32_t fallRain= values[REDIS_USER_FALL_STATUS].asInt();
                        int32_t winRain= values[REDIS_USER_WINTER_STATUS].asInt();
                        if(springRain&&i==0||sumRain&&i==1||fallRain&&i==2||winRain&&i==3)
                        {
                            ret =2;
                            LOG_ERROR<<"在2835行: "<<ret;
                        }
                    }
                    break;
                }

            }
        }


    }
    delete now;
    return ret;
}
//判断是否开始红包雨,可以放到时间较短的定时器中进行运行判断
int HallServer::WhetherStartRedEnvelopeRain(int32_t &status,int32_t agentid,int64_t &sendscore,int64_t userId,int64_t &needbet,int32_t &lefttimes)
{

    status =0;

    // 获取时间点，并且判断是否已经到达时间点
    //获取目前秒时间
    time_t tx = time(NULL);
    struct tm * local;
    //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
    local = localtime(&tx);
    struct tm t;
    t.tm_year = local->tm_year;
    t.tm_mon = local->tm_mon;
    t.tm_mday = local->tm_mday;
    t.tm_hour = local->tm_hour;
    t.tm_min = local->tm_min;
    t.tm_sec = local->tm_sec;
    //算出当前时间戳,以及计算数据库配置的时间的时间戳
    //这个函数是时间点转换成时间戳
    time_t shijianlong= mktime(&t);

    //从小到大排序一下
    if(m_ProxyStartTime.find(agentid)!=m_ProxyStartTime.end())
    {

    }
    else
    {
        LOG_ERROR<<"开始时间配置不对";
        status = 3;
        return 3;
    }
    for(int i=0;i<m_ProxyStartTime[agentid].size();i++)
    {

        for(int j=i+1;j<m_ProxyStartTime[agentid].size();j++)
        {
            int64_t intl= 0;
            if(m_ProxyStartTime[agentid][i]>m_ProxyStartTime[agentid][j])
            {
                intl = m_ProxyStartTime[agentid][i];
                m_ProxyStartTime[agentid][i] = m_ProxyStartTime[agentid][j];
                m_ProxyStartTime[agentid][j] = intl;
            }
        }
    }
    for(int i=0;i<m_ProxyEndTime[agentid].size();i++)
    {

        for(int j=i+1;j<m_ProxyEndTime[agentid].size();j++)
        {
            int64_t intl= 0;
            if(m_ProxyEndTime[agentid][i]>m_ProxyEndTime[agentid][j])
            {
                intl = m_ProxyEndTime[agentid][i];
                m_ProxyEndTime[agentid][i] = m_ProxyEndTime[agentid][j];
                m_ProxyEndTime[agentid][j] = intl;
            }
        }
    }

    if(m_ProxyRedStandard.find(agentid)!=m_ProxyRedStandard.end())
    {

    }
    else
    {
        LOG_ERROR<<"所有配置信息配置不对";
        status = 3;
        return 3;
    }
    sort(m_ProxyRedStandard[agentid].begin(),m_ProxyRedStandard[agentid].end(),SortFuc);
    ////////////每种红包所需要的打码量/////////////
    /// \brief springNeedBet
    ///i
    if(m_ProxyRedStandard[agentid].size()<4)
    {
        LOG_ERROR<<"配置信息不够四个      ="<<m_ProxyRedStandard[agentid].size();
        status = 3;
        return 3;
    }


    needbet = m_ProxyRedStandard[agentid][0].needBet;
    ///////////////////
    //大于开始时间，小于5分钟
    string keyUserInfo = REDIS_USER_BASIC_INFO + to_string(userId);
    //此为春雨
    if(!REDISCLIENT.ExistsUserBasicInfo(userId))
    {
        struct tm ttttt;
        ttttt.tm_year = local->tm_year ;
        ttttt.tm_mon = local->tm_mon;
        ttttt.tm_mday = local->tm_mday;
        ttttt.tm_hour = 0;
        ttttt.tm_min = 0;
        ttttt.tm_sec = 0;
        time_t shijianlong1= mktime(&ttttt);
        int64_t beginStart=(int64_t)shijianlong1;
        redis::RedisValue redisValue;
        redisValue[REDIS_USER_DATE] = to_string(beginStart);
        redisValue[REDIS_USER_TODAY_BET] = to_string(0);
        redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
        redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
        redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
        redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

        redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
        redisValue[REDIS_USER_WINTER_STATUS] = to_string(0);
        REDISCLIENT.hmset(keyUserInfo, redisValue);

        //条件不足
        status = 2;
    }

    string fields[8] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET,REDIS_USER_TODAY_UPER,REDIS_USER_LEFT_TIMES,
                       REDIS_USER_SPRING_STATUS,REDIS_USER_SUMMER_STATUS,REDIS_USER_FALL_STATUS,
                       REDIS_USER_WINTER_STATUS};
    redis::RedisValue values;
    int ret = REDISCLIENT.hmget(keyUserInfo, fields, 8, values);

    int64_t todayBet = 0;
    int64_t todayUpScore = 0;
    int32_t springStatus =  0;
    int32_t summerStatus =  0;
    int32_t fallStatus =  0;
    int32_t winterStatus =  0;
    lefttimes = 0;
    sendscore = 0;
    if(ret)
    {


        struct tm ttttt;
        ttttt.tm_year = local->tm_year ;
        ttttt.tm_mon = local->tm_mon;
        ttttt.tm_mday = local->tm_mday;
        ttttt.tm_hour = 0;
        ttttt.tm_min = 0;
        ttttt.tm_sec = 0;

        string valuesx;
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_DATE, valuesx);
        int64_t nowtime=stoll(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_TODAY_BET, valuesx);
        todayBet = stoll(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_LEFT_TIMES, valuesx);
        lefttimes = stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_TODAY_UPER, valuesx);
        todayUpScore =stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_SPRING_STATUS, valuesx);
        springStatus = stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_SUMMER_STATUS, valuesx);
        summerStatus = stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_FALL_STATUS, valuesx);
        fallStatus =  stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_WINTER_STATUS, valuesx);
        winterStatus =  stoi(valuesx);
        LOG_ERROR<<"  springStatus="<<springStatus
                 <<"  summerStatus="<<summerStatus
                 <<"  fallStatus="<<fallStatus
                 <<"  winterStatus="<<winterStatus<< "   nowtime"<<nowtime;

        time_t shijianlong1= mktime(&ttttt);
        int64_t beginStart=(int64_t)shijianlong1;

        LOG_ERROR<<"  systemtime"<<beginStart;
        if(beginStart!=nowtime)
        {
            redis::RedisValue redisValue;
            redisValue[REDIS_USER_DATE] = to_string(beginStart);
            redisValue[REDIS_USER_TODAY_BET] = to_string(0);
            redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
            redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
            redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
            redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

            redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
            redisValue[REDIS_USER_WINTER_STATUS] = to_string(0);
            REDISCLIENT.hmset(keyUserInfo, redisValue);

            status = 2;
            return status;
        }








        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_SUMMER_STATUS, valuesx);
        LOG_ERROR<<"  summerStatus  "<<  stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_FALL_STATUS, valuesx);
        LOG_ERROR<<"  fallStatus  "<<  stoi(valuesx);
        REDISCLIENT.GetUserBasicInfo(userId, REDIS_USER_WINTER_STATUS, valuesx);
         LOG_ERROR<<"  winterStatus  "<<  stoi(valuesx);
        LOG_ERROR<<"  todayBet="<<todayBet<<"    todayUpScore"<<todayUpScore;
    }
    else
    {
        LOG_ERROR<<"  读取不到redis信息";
    }

    int32_t vecsize = (int32_t)m_ProxyStartTime[agentid].size();
    LOG_ERROR<<"  m_ProxyStartTime[agentid].size()==="<<vecsize;
    int count = 0;
    for(int i=0;i<vecsize;i++)
    {
       if(shijianlong>=m_ProxyStartTime[agentid][i] &&shijianlong<m_ProxyEndTime[agentid][i])
       {
           if(todayBet<m_ProxyRedStandard[agentid][i].needBet
                   &&(todayBet<m_ProxyRedStandard[agentid][i].upScoreLimit*m_ProxyRedStandard[agentid][i].upScoreMul
                      ||todayUpScore<m_ProxyRedStandard[agentid][i].upScoreLimit))
           {
               //条件不足
               status = 2;
               if(todayUpScore<m_ProxyRedStandard[agentid][i].upScoreLimit)
               {
                   needbet = m_ProxyRedStandard[agentid][i].needBet - todayBet;
               }
               else
               {
                   if(m_ProxyRedStandard[agentid][i].upScoreLimit*m_ProxyRedStandard[agentid][i].upScoreMul<m_ProxyRedStandard[agentid][i].needBet)
                   {
                       needbet = m_ProxyRedStandard[agentid][i].upScoreLimit*m_ProxyRedStandard[agentid][i].upScoreMul - todayBet;
                   }
                   else
                   {
                       needbet =m_ProxyRedStandard[agentid][i].needBet-todayBet;
                   }
               }
               return status;
           }
           LOG_ERROR<<"  m_ProxyRedStandard[agentid][i].needBet"<<m_ProxyRedStandard[agentid][i].needBet;
           LOG_ERROR<<"  upScoreLimit*upScoreMul"<<m_ProxyRedStandard[agentid][i].upScoreLimit*m_ProxyRedStandard[agentid][i].upScoreMul;
           LOG_ERROR<<"  m_ProxyRedStandard[agentid][i].upScoreLimit"<<m_ProxyRedStandard[agentid][i].upScoreLimit;
           if((springStatus&&i==0)||(summerStatus&&i==1)||(fallStatus&&i==2)||(winterStatus&&i==3))
           {
               //已经领取
               //条件不足
               lefttimes = 0;
               status = 1;
               return status;
           }

           LOG_ERROR<<"到这里已经成功了  i="<<i  <<"   vecsize="<<vecsize;
           LOG_ERROR<<"springStatus   "<<springStatus <<"  summerStatus  "<<summerStatus<<"  fallStatus  "<<fallStatus<<"   winterStatus  "<<winterStatus;
           int64_t min = m_ProxyRedStandard[agentid][i].scoreMin;
           int64_t max = m_ProxyRedStandard[agentid][i].scoreMax;
           LOG_ERROR<<"min:   "<< min  <<"   max:  "<<max;
           int32_t random = m_random.betweenInt(1,100).randInt_mt(true);
           if(random<80)
           {
               if(min==max)
                sendscore = min;
               else if(min>max)
               {
                   int res=0;
                   res=min;
                   min=max;
                   max=res;
                   sendscore = m_random.betweenInt(min,min+(max-min)/5+1).randInt_mt(true);
               }
               else
                sendscore = m_random.betweenInt(min,min+(max-min)/5+1).randInt_mt(true);
           }
           else
           {
               if(min==max)
               {
                  sendscore = min;
               }
               else if(min>max)
               {
                  int res=0;
                  res=min;
                  min=max;
                  max=res;
                  sendscore = m_random.betweenInt(min,max).randInt_mt(true);
               }
               else
               {
                   sendscore = m_random.betweenInt(min,max).randInt_mt(true);
               }

           }
           if(i==0)
           {
               redis::RedisValue redisValue;
               redisValue[REDIS_USER_SPRING_STATUS] = to_string(1);
               REDISCLIENT.hmset(keyUserInfo, redisValue);
           }
           else if(i==1)
           {
               redis::RedisValue redisValue;
               redisValue[REDIS_USER_SUMMER_STATUS] = to_string(1);
               REDISCLIENT.hmset(keyUserInfo, redisValue);
           }
           else if(i==2)
           {
               redis::RedisValue redisValue;
               redisValue[REDIS_USER_FALL_STATUS] = to_string(1);
               REDISCLIENT.hmset(keyUserInfo, redisValue);
           }
           else if(i==3)
           {
               redis::RedisValue redisValue;
               redisValue[REDIS_USER_WINTER_STATUS] = to_string(1);
               REDISCLIENT.hmset(keyUserInfo, redisValue);
           }
           break;
       }
       else
       {
           count ++;
           continue;
       }
    }
    if(count==vecsize)
    {
        //条件不足
        status = 2;
    }
    return status;
}
// 获取登录游戏获取IP信息
void HallServer::cmd_get_game_server_message(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_WARN <<  __FUNCTION__;

    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES;
    ::HallServer::GetGameServerMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        ::HallServer::GetGameServerMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        uint32_t gameId = msg.gameid();
        uint32_t roomId = msg.roomid();
        uint32_t tableId = INVALID_TABLE;
        int32_t ipcode = 0;
        string strTableId = msg.strtableid();
        vector<string> strVec;
        strVec.clear();
        boost::algorithm::split(strVec, strTableId, boost::is_any_of("-"));
        if (strVec.size() == 2)
        {
                tableId = stoi(strVec[1]) - 1;
                ipcode = stoi(strVec[0]);
        }

		int32_t  type = -1, tableCount = 0;
        map<int32_t, int32_t>::iterator iter = m_gameType_map.find(gameId);
        if (iter != m_gameType_map.end())
        {
            type = iter->second;
			if (type == GameType_BaiRen)
			{
				if (m_gameRoomTableCount_map.count(roomId))
				{
					tableCount = m_gameRoomTableCount_map[roomId];
				}
			}
        }
        else
        {
            LOG_WARN << "判断游戏类型错误,gameId;" << gameId;
        }
        //uint32_t tableId = msg.tableid();
        string errormsg = "Unknown error";
        int32_t retcode = 1;

        do
        {
            string rsAgentid = "0";
            string rsCurrency = "0";
            REDISCLIENT.GetProxyUser(userId, REDIS_UID_AGENTID, rsAgentid);
            REDISCLIENT.GetCurrencyUser(userId, REDIS_UID_CURRENCY, rsCurrency);
            int32_t agentId = atol(rsAgentid.c_str());
            int32_t curency = atol(rsCurrency.c_str());
            LOG_INFO << "---userId[" << userId << "],gameId[" << gameId << "],roomId[" << roomId << "],ipcode[" << ipcode << "],agentId[" << agentId << "],curency["<<curency<<"]";
            int32_t keyid = 0;
			if (tableId != INVALID_TABLE)
				keyid = (roomId << 14) | (ipcode << 4) | tableId;
            string roomIp="";

            // 单线路维护过滤
            if (!checkAgentStatus(agentId))
            {
                retcode = 99;//eApiErrorCode::GameMaintenanceError;
                errormsg = "agentid is in maintenance.";
                LOG_WARN << "---玩家["<<userId<<"]对应代理在维护中---";
                break;
            }
            uint32_t redisGameId = 0, redisRoomId = 0, redisTableId = 0;
            if (REDISCLIENT.GetUserOnlineInfo(userId, redisGameId, redisRoomId, redisTableId))
            {


                string msg;
                if (tableId == INVALID_TABLE)
                {
                    //百家乐逻辑
                    if (type == GameType_BaiRen &&  tableCount > 1 && gameId == redisGameId && roomId == redisRoomId && tableId == INVALID_TABLE)
                    {
                        tableId = redisTableId;
                        keyid = (roomId << 14) | (ipcode << 4) | tableId;

                        internal_header->nOK = 1;
                        retcode = 0;
                        errormsg = "Get GameServer IP OK.";
                    }
                    else
                    {
                        if (gameId != redisGameId || roomId != redisRoomId)
                        {
                                retcode = ERROR_ENTERROOM_USERINGAME;
                                errormsg = "User in other game.";
                        }
                        else
                        {
                                internal_header->nOK = 1;
                                retcode = 0;
                                errormsg = "Get GameServer IP OK.";
                        }
                    }

                }
                else
                {

                    if (gameId != redisGameId || roomId != redisRoomId || tableId != redisTableId)
                    {
                        retcode = ERROR_ENTERROOM_USERINGAME;
                        errormsg = "User in other game.";
                    }
                    else
                    {
                        internal_header->nOK = 1;
                        retcode = 0;
                        errormsg = "Get GameServer IP OK.";
                    }


                }

                LOG_INFO << ">>> Get User Online Info OK, userid:" << userId << ", gameid:" << gameId << ", roomid:" << roomId<< ", tableId:" << tableId<< ", redisTableId:" << redisTableId;
                 LOG_INFO << ">>> Get User Online Info OK, userid:" << userId << ", redisGameId:" << redisGameId << ", redisRoomId:" << redisRoomId<< ", tableId:" << tableId<< ", redisTableId:" << redisTableId;
                // 在其它游戏中
                break;
            }

            string strIp;
            //币种为0或者没有值 默认是人民币
            if(0 == curency)
            {
                if (gameId != (uint32_t)eGameKindId::bbqznn)
                {

                    if ((roomId - gameId * 10) < 20)
                    {
                        if(type==GameType_BaiRen)
                        {
                            if(tableId != INVALID_TABLE && m_tableIpMap.count(keyid))
                            {
                                roomIp = m_tableIpMap[keyid];
                                strIp  = roomIp;
                            }
                            else
                            {
                                strIp = getRandomGameServer(gameId, roomId, agentId);
                            }
                        }
                        else
                        {
                            strIp = getRandomGameServer(gameId, roomId, agentId);
                        }

                    }
                    else
                    {
                        strIp = getRandomMatchServer(gameId, roomId);
                    }
                }
                else
                {
                    strIp = getRandomGameServer(gameId, roomId,agentId);
                }
            }
            else
            {
                //有币种就走币种通道否则找不到服务器
                if ((roomId - gameId * 10) < 20)
                {
                    strIp = getMoneyTypeGameServer(gameId, roomId, curency);
                }
                else
                {
                    strIp = getMoneyTypeGameServer(gameId, roomId,curency);
                }
            }
            //
            LOG_INFO << ">>> Get Random GameServer IP, userid:" << userId << ", gameid:" << gameId << ", roomid:" << roomId << ", ip:" << strIp;

            if (!strIp.empty())
            {
                // 此处设置玩家在线状态不严格(应该设置于玩家进入房间之后)
                // 把分配到玩家游戏服IP保存到缓存
                REDISCLIENT.SetUserOnlineInfoIP(userId, strIp);
                REDISCLIENT.SetUserOnlineInfo(userId, gameId, roomId,tableId);

                internal_header->nOK = 1;

                retcode = 0;
                errormsg = "Get Game Server IP OK.";

                LOG_INFO << ">>> Bind GameServer IP OK, userid:" << userId << ", ip:" << strIp;
            }
            else
            {
                LOG_ERROR << ">>> Get Random GameServer strIP = NULL ";

                REDISCLIENT.DelUserOnlineInfo(userId);

                LOG_ERROR << ">>> delete bind game server, ip:(null) userid:" << userId;

                retcode = ERROR_ENTERROOM_GAMENOTEXIST;
                errormsg = "Game Server Not Found!!!";
            }

        } while (0);

        response.set_retcode(retcode);
        response.set_errormsg(errormsg.c_str());

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
// 获取房间人数
void HallServer::cmd_get_romm_player_nums_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_DEBUG << "" << __FILE__ << " "  << __FUNCTION__ ;

    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_RES;
    ::HallServer::GetServerPlayerNum msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header(); 

        ::HallServer::GetServerPlayerNumResponse response;
        {
            READ_LOCK(m_room_player_num_mutex);
            response.CopyFrom(m_roomPlayerNums);
        }
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        response.set_retcode(0);
        response.set_errormsg("Get ServerPlayerNum Message OK!");

        // LOG_INFO << ">>> Get ServerPlayerNum Message OK!";

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
// 根据不同币种获取不同币种服务器
string HallServer::getMoneyTypeGameServer(uint32_t gameId, uint32_t roomId,int32_t currency)
{
    string retString = "",gameserverIp = "";

    try
    {

        MutexLockGuard lock(m_proxy_game_ip);
        proxyIPVec.clear();
        mongocxx::collection proxy_game_coll = MONGODBCLIENT["gamemain"]["proxy_gameserver"];
        mongocxx::cursor info = proxy_game_coll.find({});
        for (auto &doc : info)
        {
            string allipadress = doc["ipadress"].get_utf8().value.to_string();
            proxyIPVec.push_back(allipadress);
            for (int i=0;i<proxyIPVec.size();i++)
            {
                 LOG_INFO << "分代理的ip是:"<<proxyIPVec[i];
            }

        }
    }
    catch (exception excep)
    {

        LOG_ERROR << "---" << __FUNCTION__ << " 读取游戏服配置出错,agentid信息["  << "],what[" << excep.what() << "]";
        return retString;
    }
    // 查找代理对应游戏服IP 这里可以确定是独立线路玩家还是app 玩家
    if(getGameServer_MoneyType(currency,gameserverIp))
    {

        LOG_INFO << "---agentId["<< currency <<"].找到游戏服IP[" << gameserverIp << "]";//5002:192.168.2.96:35002

    }

    vector<string> tempVector;
    {


        for (int i=0;i<moneyTypeVec.size();i++)
        {

             LOG_INFO << "分代理的ip是:"<<moneyTypeVec[i];
        }

        for (string ip : m_game_servers)
        {
            LOG_INFO << "---m_game_servers[" << ip << "]";//5002:192.168.2.96:35002
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));
            if (stoi(vec[0]) == roomId) //vec[1] ip //vec[2]
            {
                // 兼容原来的方式
                if (gameserverIp.empty())
                {
                     LOG_ERROR << "找不到币种服务器";
                }
                else
                {
                    LOG_ERROR << "-------------------------12";
                    // 有专用游戏服
                    std::size_t found = ip.find(gameserverIp);
                    if (found != std::string::npos)
                    {
                        LOG_INFO << "---[" << gameserverIp << "] is found at ip[" << ip << "]";
                        auto it = find(m_invaild_game_server.begin(), m_invaild_game_server.end(), ip);
                        if (it == m_invaild_game_server.end()) // 房间没有被挂起
                            tempVector.push_back(ip);
                    }
                }
            }
        }
    }
    if (!tempVector.empty())
    {
        int index = GlobalFunc::RandomInt64(1, tempVector.size());
        retString = tempVector[index - 1];
    }

    return retString;
}
// 根据代理IP分配不同游戏服务
string HallServer::getRandomGameServer(uint32_t gameId, uint32_t roomId,int32_t agentId)
{
    string retString = "",gameserverIp = "";
    MutexLockGuard lock(m_currnecy_game_ip);

    try
    {


        moneyTypeVec.clear();
        mongocxx::collection currency_game_coll = MONGODBCLIENT["gamemain"]["currency_gameserver"];
        mongocxx::cursor info = currency_game_coll.find({});

        for (auto &doc : info)
        {

            int32_t currency=doc["currency"].get_int32();
            string allipadress = doc["ipadress"].get_utf8().value.to_string();
            moneyTypeVec.push_back(allipadress);
            for (int i=0;i<moneyTypeVec.size();i++)
            {

                 LOG_INFO << "分币种的ip是:"<<moneyTypeVec[i];
            }

        }
    }
    catch (exception excep)
    {

        LOG_ERROR << "---" << __FUNCTION__ << " 读取游戏服配置出错,moneytype信息[" << "],what[" << excep.what() << "]";
        return retString;
    }
    // 查找代理对应游戏服IP 这里可以确定是独立线路玩家还是app 玩家
    if(getGameServerIp(agentId,gameserverIp))
    {

        LOG_INFO << "---agentId["<< agentId <<"].找到游戏服IP[" << gameserverIp << "]";//5002:192.168.2.96:35002

    }

    vector<string> tempVector;
    tempVector.clear();
    {       
        for (int i=0;i<proxyIPVec.size();i++)
        {

             LOG_INFO << "分代理的ip是:"<<proxyIPVec[i];
        }

        for (string ip : m_game_servers)
        {
            LOG_INFO << "---m_game_servers[" << ip << "]";//5002:192.168.2.96:35002
            vector<string> vec;
            vec.clear();
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));
            if (stoi(vec[0]) == roomId) //vec[1] ip //vec[2]
            {
                auto item1 = find(moneyTypeVec.begin(), moneyTypeVec.end(), vec[1]);
                if (item1 != moneyTypeVec.end())//
                {
                    LOG_INFO << "多币种服务器过滤:"<<vec[1]<<"  ip-----------"<<ip;
                    continue;
                }
                // 兼容原来的方式
                if (gameserverIp.empty())
                {
                    LOG_ERROR << "-------------------------11";
                    //没有独立代理IP地址,是APP玩家
                    LOG_INFO << "查找的ip 是 : "<< vec[1];
                    auto item = find(proxyIPVec.begin(), proxyIPVec.end(), vec[1]);
                    if (item != proxyIPVec.end())// app玩家不能再代理表中找到
                    {
                        continue;
                    }
                    auto it = find(m_invaild_game_server.begin(), m_invaild_game_server.end(), ip);
                    if (it == m_invaild_game_server.end())// 房间没有被挂起
                    {

                        tempVector.push_back(ip);
                        LOG_INFO << ":"<<vec[1]<<"  获取到正常线路的 ip-----------"<<ip;
                    }

                }
                else
                {
                    LOG_ERROR << "-------------------------12";
                    // 有专用游戏服
                    std::size_t found = ip.find(gameserverIp);
                    if (found != std::string::npos)
                    {
                        LOG_INFO << "---[" << gameserverIp << "] is found at ip[" << ip << "]";
                        auto it = find(m_invaild_game_server.begin(), m_invaild_game_server.end(), ip);
                        if (it == m_invaild_game_server.end()) // 房间没有被挂起
                            tempVector.push_back(ip);
                    }
                }
            }
        }
    }
    if (!tempVector.empty())
    {
        int index = GlobalFunc::RandomInt64(1, tempVector.size());
        retString = tempVector[index - 1];
    }

    return retString;
}

string HallServer::getRandomMatchServer(uint32_t gameId, uint32_t roomId)
{
    string retString = "";
    vector<string> tempVector;
    {
        MutexLockGuard lock(m_match_servers_mutex);
        for(string ip : m_match_servers)
        {
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of( ":" ));
            if(stoi(vec[0]) == roomId)
            {
                auto it=find(m_invaild_match_server.begin(),m_invaild_match_server.end(),ip);

                if(it==m_invaild_match_server.end())
                    tempVector.push_back(ip);
            }
        }
    }
    if(!tempVector.empty())
    {
        int index = GlobalFunc::RandomInt64(1, tempVector.size());
        retString = tempVector[index-1];
    }
    return retString;
}

void HallServer::cmd_on_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    int32_t userId = internal_header->userId;
    DBUpdateUserOnlineStatusInfo(userId, 0);

    LOG_INFO << ">>> User Offline,userid[" << userId <<"]";

    string loginDateTime;
    if(REDISCLIENT.GetUserLoginInfo(userId, "LoginDateTime", loginDateTime))
    {
        chrono::system_clock::time_point loginTime = chrono::system_clock::from_time_t(stoull(loginDateTime));
        DBAddUserLogoutLog(userId, loginTime);
    }
 
}

void HallServer::cmd_repair_hallserver(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_DEBUG << ">>> "<< __FUNCTION__ <<" >>>>>>>>>>>>>";

    Header* common=(Header*)msgdata;

    ::Game::Common::HttpNotifyRepairServerResp  msg;
    int32_t repaireStatus=1;
    if(msg.ParseFromArray(&msgdata[sizeof(Header)], common->realSize))
    {
        int32_t status = msg.status();

        if(status==1 && !m_bInvaildServer)
        {
            if (ZOK==m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true))
            {
                m_bInvaildServer=true;
                // m_server.getLoop()->runInLoop(bind(&HallServer::stopConsumerClusterScoreOrder,this));
            }
            else
                repaireStatus=2;//node create failure

        }else
            repaireStatus=0;
    }
    msg.set_status(repaireStatus);

    size_t len = msg.ByteSize();
    vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
    if(msg.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
    {
        sendData(weakConn, data, internal_header, common);
    }
}

bool HallServer::DBUpdateUserOnlineStatusInfo(int64_t userId, int32_t status)
{
    bool bSuccess = false;
    try
    {
        bsoncxx::document::value findValue = document{} << "userid" << userId << finalize;
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        bsoncxx::document::value updateValue = document{}
                                               << "$set" << open_document
                                               << "onlinestatus" << bsoncxx::types::b_int32{status}
                                               << close_document
                                               << finalize;
        // LOG_DEBUG << "Update Document:"<<bsoncxx::to_json(updateValue);
        coll.update_one(findValue.view(), updateValue.view());
        bSuccess = true;
    }catch(exception &e)
    {
        LOG_ERROR << "exception: " << e.what();
    }
    return bSuccess;
}
void HallServer::WriteLogOutLogToMysql(int64_t userid,int64_t logintime,
                         int64_t logouttime,int32_t agentid,int64_t playseconds)
{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO logout_log(userid, logintime,logouttime, "
//                                           "agentid,playseconds)"
//                                           "VALUES(?,?,?,?,?)"));

//        pstmt->setInt64(1,userid);
//        pstmt->setDateTime(2, InitialConversion(logintime));
//        pstmt->setDateTime(3, InitialConversion(logouttime));
//        pstmt->setInt(4, agentid);
//        pstmt->setInt64(5, playseconds);
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
bool HallServer::DBAddUserLogoutLog(int64_t userId, chrono::system_clock::time_point &startTime)
{
    bool bSuccess = false;
    try
    {
        int64_t redis_userId = 0;
        uint32_t redis_agentId = 0;
        int32_t agentId = 0;
        string msg_session,account;
        REDISCLIENT.GetUserLoginInfo(userId, "msg_session", msg_session);
        bool ret = GetUserLoginInfoFromRedis(msg_session, redis_userId, account, redis_agentId);
        if( ret && redis_userId == userId) {
            agentId = redis_agentId;
        } 

        chrono::system_clock::time_point endTime = chrono::system_clock::now();
        chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(endTime - startTime);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["logout_log"];
        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << userId
                << "logintime" << bsoncxx::types::b_date(startTime)
                << "logouttime" << bsoncxx::types::b_date(endTime)
                << "agentid" << agentId
                << "playseconds" << bsoncxx::types::b_int64{durationTime.count()}
                << bsoncxx::builder::stream::finalize;

        bsoncxx::stdx::optional<mongocxx::result::insert_one> result_insert = coll.insert_one(insert_value.view());
        if ( !result_insert ) {
            LOG_ERROR << ">>> User Logout Log error,userid["<< userId <<"]";
        }
        LOG_ERROR<<"mysql插入  logout_log";
        int64_t starttime = (chrono::duration_cast<chrono::seconds>(startTime.time_since_epoch())).count();
        int64_t endtime = (chrono::duration_cast<chrono::seconds>(endTime.time_since_epoch())).count();
        int64_t timeduration = bsoncxx::types::b_int64{durationTime.count()};
        m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteLogOutLogToMysql,this, userId, starttime,
                                                                         endtime, agentId,timeduration ));
        LOG_INFO << ">>> User Logout Log,userid[" << userId << "],account[" << account << "],agentId["<<agentId<<"],msg_session["<<msg_session<<"]";

        bSuccess = true;
    }
    catch(exception &e)
    {
        LOG_ERROR << __FUNCTION__ << ",exception: " << e.what();
    }
    return bSuccess;
}

void HallServer::cmd_set_headId(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_RES;
    ::HallServer::SetHeadIdMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::SetHeadIdMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t headId = msg.headid();

        response.set_retcode(0);
        response.set_errormsg("OK.");

        try
        {
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
            coll.update_one(document{} << "userid" << userId << finalize,
                            document{} << "$set" << open_document << "headindex" << headId << close_document << finalize);

            response.set_userid(userId);
            response.set_headid(headId);
        }catch(exception &e)
        {
            LOG_ERROR<< " >>> Exception: " << e.what();
            response.set_retcode(1);
            response.set_errormsg("Database  Error.");
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

 

void HallServer::cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_RES;
    ::HallServer::GetUserScoreMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::GetUserScoreMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t viplevel = 0,levelrate = 0,agentid = 0,isdtapp = 0,curLevel = 0,txhstatus = 0, currency = 0;
        int64_t score = 0,integralvalue  = 0,totalvalidbet =0;

        response.set_userid(userId);
    
        try
        {
            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} <<  "agentid" << 1 <<"score" << 1 <<"txhstatus"<<1<<"integralvalue" << 1 <<"viplevel" << 1 << "totalvalidbet"<< 1 << "isdtapp"<< 1 << "currency" << 1 << finalize);
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
            bsoncxx::stdx::optional<bsoncxx::document::value> find_result = coll.find_one(document{} << "userid" << userId << finalize, opts);
            if( find_result )
            {
                bsoncxx::document::view view = find_result->view();
                score = view["score"].get_int64();
                agentid = view["agentid"].get_int32();
                integralvalue  = view["integralvalue"].get_int64();
                totalvalidbet = view["totalvalidbet"].get_int64();

                if (view["isdtapp"]) 
                    isdtapp = view["isdtapp"].get_int32();
                if (view["viplevel"]) 
                    viplevel = view["viplevel"].get_int32();

                if (view["txhstatus"]) 
                    txhstatus = view["txhstatus"].get_int32();

				//增加多币数支持
				if (view["currency"])
					currency = view["currency"].get_int32();

				// 获取玩家对应幸运转盘的配置
				tagConfig lkGame_config;
				memset(&lkGame_config, 0, sizeof(lkGame_config));
				lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

                integralvalue = integralvalue / (lkGame_config.nExChangeRate * COIN_RATE);//100 写个全局的币率
              
                LOG_INFO << "---"<< __FUNCTION__ <<",txhstatus[" << txhstatus <<"]" ;
            //   ---cmd_get_user_score,userid[250113699],score[9910000],jifen[6004],totalvalidbet[240000],viplevel[0],levelrate[0] 

                if (isdtapp == eDTapp::op_YES)
                {
                    getLevelInfo(totalvalidbet, agentid, curLevel, levelrate);
                    if (viplevel != curLevel) //数据库与计算出来的不一样
                    {
                        viplevel = curLevel;
                        if (!coll.update_one(make_document(kvp("userid", userId)), make_document(kvp("$set", make_document(kvp("viplevel", viplevel))))))
                            LOG_ERROR << "---更新玩家等级失败,viplevel[" << viplevel << "]";
                    }
                    // 返回进入的第三方游戏ID
                    response.set_gameid(txhstatus);
                }

                response.set_jifenscore(integralvalue);
                response.set_score(score);
                response.set_viplevel(viplevel);
                response.set_vippercent(levelrate);
                response.set_retcode(0);
                response.set_errormsg("CMD GET USER SCORE OK.");
                LOG_INFO << "---"<< __FUNCTION__ <<",userid[" << userId <<"],score["<<score << "],jifen["<<integralvalue << "],totalvalidbet[" << totalvalidbet <<"],viplevel[" << viplevel <<"],levelrate[" << levelrate <<"]" ;
            }
            else
            {
                response.set_retcode(1);
                response.set_errormsg("CMD GET USER SCORE ERROR.");
            }

        }
        catch(exception &e)
        {
            LOG_ERROR<< " >>> Exception: " << e.what();
            response.set_retcode(2);
            response.set_errormsg("MongoDB  Error.");
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
void HallServer::cmd_get_play_record_detail(TcpConnectionWeakPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header) {
	internal_header->nOK = 0;
	Header* commandHeader = (Header*)msgdata;
	commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_RECORD_DETAIL_RES;
	::HallServer::GetRecordDetailMessage msg;
	if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
	{
		::Game::Common::Header header = msg.header();

		::HallServer::GetRecordDetailResponse response;
		::Game::Common::Header* resp_header = response.mutable_header();
		resp_header->CopyFrom(header);

		int64_t userId = internal_header->userId;
        std::string gameinfoid = msg.gameroundno();
		response.set_gameroundno(gameinfoid);
		//0:百人类；1:对战类；2:其他
		int nGameType = GetGameType(gameinfoid);
		try
		{
			string strRoundId;
			int32_t roomId = 0;
			int64_t winScore = 0;
			chrono::system_clock::time_point endDateTime;
			int64_t endDateTimeCount = 0;

            std::string jsondata;
			if (nGameType==1) //对战类 GameType_Confrontation
			{
				do {
					mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];

					mongocxx::options::find opts = mongocxx::options::find{};
					opts.projection(bsoncxx::builder::stream::document{}
					<< "detail" << 1 << bsoncxx::builder::stream::finalize);

					bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
						bsoncxx::builder::stream::document{}
						<< "gameinfoid" << gameinfoid
						<< bsoncxx::builder::stream::finalize, opts);
					if (!result) {
						break;
					}
					bsoncxx::document::view view = result->view();
					if (view["detail"]) {
						switch (view["detail"].type()) {
						case bsoncxx::type::k_null:
							break;
						case bsoncxx::type::k_utf8:
							jsondata = view["detail"].get_utf8().value.to_string();
							LOG_INFO << "\n" << jsondata;
							response.set_detailinfo(jsondata);
							break;
						case bsoncxx::type::k_binary:
							response.set_detailinfo(view["detail"].get_binary().bytes, view["detail"].get_binary().size);
							break;
						}
					}
				} while (0);
			} 
			else if (nGameType == 0)//百人游戏 GameType_BaiRen
			{
				do {
					mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
					bsoncxx::document::value query_value = bsoncxx::builder::stream::document{} << "gameinfoid" << gameinfoid << finalize;
					bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(query_value.view());
					if (!result) {
						LOG_ERROR << "查找详情数据表错误:" << userId << " gameinfoid:" << gameinfoid;
						break;
					}
					bsoncxx::document::view view = result->view();
					auto detailsArr = view["bairendetails"].get_array();
					for (auto &detail : detailsArr.value)
					{
						int64_t uid = 0;
						if (detail["userid"].type() == bsoncxx::type::k_int32)
						{
							uid = detail["userid"].get_int32();
						}
						else
						{
							uid = detail["userid"].get_int64();
						}
						if (userId== uid)
						{
							switch (detail["detailData"].type()) {
							case bsoncxx::type::k_null:
								break;
							case bsoncxx::type::k_utf8:
								jsondata = detail["detailData"].get_utf8().value.to_string();
								LOG_INFO << "\n" << jsondata;
								response.set_detailinfo(jsondata);
								break;
							case bsoncxx::type::k_binary:
								response.set_detailinfo(detail["detailData"].get_binary().bytes, detail["detailData"].get_binary().size);
								break;
							}
							break;
						}
					}
				} while (0);
			}
			else
			{

			}
			response.set_retcode(0);
			response.set_errormsg("CMD GET GAME RECORD DETAIL OK.");
		}
		catch (exception & e)
		{
			LOG_ERROR << " >>> Exception: " << e.what();
			response.set_retcode(1);
			response.set_errormsg("MongoDB  Error.");
		}

		size_t len = response.ByteSize();
		vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
		if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
		{
			sendData(weakConn, data, internal_header, commandHeader);
		}
	}
}

void HallServer::cmd_get_play_record_detail_match(WeakTcpConnectionPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header) {
	internal_header->nOK = 0;
	Header* commandHeader = (Header*)msgdata;
	commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCHRECORD_DETAIL_RES;
	::HallServer::GetMatchRecordDetailMessage msg;
	if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
	{
		::Game::Common::Header header = msg.header();

		::HallServer::GetMatchRecordDetailResponse response;
		::Game::Common::Header* resp_header = response.mutable_header();
		resp_header->CopyFrom(header);

		int64_t userId = internal_header->userId;
		std::string gameinfoid = msg.gameroundno();
		response.set_gameroundno(gameinfoid);
		try
		{


			std::string jsondata;
			do {
				mongocxx::collection coll = MONGODBCLIENT["gamelog"]["match_game_replay"];

				mongocxx::options::find opts = mongocxx::options::find{};
				opts.projection(bsoncxx::builder::stream::document{}
				<< "detail" << 1 << bsoncxx::builder::stream::finalize);

				bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
					bsoncxx::builder::stream::document{}
					<< "gameinfoid" << gameinfoid
					<< bsoncxx::builder::stream::finalize, opts);
				if (!result) {
					break;
				}
				bsoncxx::document::view view = result->view();
				if (view["detail"]) {
					switch (view["detail"].type()) {
					case bsoncxx::type::k_null:
						break;
					case bsoncxx::type::k_utf8:
						jsondata = view["detail"].get_utf8().value.to_string();
						LOG_ERROR << "\n" << jsondata;
						response.set_detailinfo(jsondata);
						break;
					case bsoncxx::type::k_binary:
						response.set_detailinfo(view["detail"].get_binary().bytes, view["detail"].get_binary().size);
						break;
					}
				}
			} while (0);
			response.set_retcode(0);
			response.set_errormsg("CMD GET MATCH RECORD DETAIL OK.");
		}
		catch (exception & e)
		{
			LOG_ERROR << " >>> Exception: " << e.what();
			response.set_retcode(1);
			response.set_errormsg("MongoDB  Error.");
		}

		size_t len = response.ByteSize();
		vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
		if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
		{
			sendData(weakConn, data, internal_header, commandHeader);
		}
	}
}
void HallServer::cmd_get_play_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    //本次请求开始时间戳(微秒)
    muduo::Timestamp timestart = muduo::Timestamp::now(); 

    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_RES;
    ::HallServer::GetPlayRecordMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::GetPlayRecordMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t gameId = msg.gameid();

        response.set_gameid(gameId);

        try
        {
            // 适配幸运转盘情况 
            const char* play_record_db_str  = IsLkGame(gameId) ? "play_record_lkgame":"play_record";

            string strRoundId;
            int32_t roomId = 0;
            int64_t winScore = 0;
            chrono::system_clock::time_point endDateTime;
            int64_t endDateTimeCount = 0;

            bsoncxx::document::element elem;
            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} << "gameinfoid" << 1 << "roomid" << 1 << "winscore" << 1 << "gameendtime" << 1 << finalize);
            opts.limit(m_lkGame_config.RecordItemCount);
            opts.sort(document{} << "gameendtime" << -1 << finalize); //此处后期可以调整

             // 往前21天
            int64_t seconds = m_lkGame_config.RecordDays * ONE_DAY_SECOND; //(秒);
            auto query_value = document{} << "gameendtime" << open_document << "$gte" << bsoncxx::types::b_date(chrono::system_clock::now() - std::chrono::seconds(seconds)) << close_document
                                          << "userid" << userId << "gameid" << gameId << finalize;

            mongocxx::collection coll = MONGODBCLIENT["gamemain"][play_record_db_str];
            mongocxx::cursor cursor = coll.find(query_value.view(), opts);
            // mongocxx::cursor cursor = coll.find(document{} << "userid" << userId << "gameid" << gameId << finalize, opts);
            for(auto &doc : cursor)
            {
                strRoundId = doc["gameinfoid"].get_utf8().value.to_string();
                roomId = doc["roomid"].get_int32();
                winScore = doc["winscore"].get_int64();
                endDateTime = doc["gameendtime"].get_date();
                endDateTimeCount = (chrono::duration_cast<chrono::seconds>(endDateTime.time_since_epoch())).count();

                ::HallServer::GameRecordInfo *gameRecordInfo = response.add_detailinfo();
                gameRecordInfo->set_gameroundno(strRoundId);
                gameRecordInfo->set_roomid(roomId);
                gameRecordInfo->set_winlosescore(winScore);
                gameRecordInfo->set_gameendtime(endDateTimeCount);
            }
            response.set_retcode(0);
            response.set_errormsg("CMD GET USER SCORE OK.");
           
            // LOG_INFO << ">>> "<< __FUNCTION__ <<" success"; 
        }
        catch(exception &e)
        {
            LOG_ERROR<< " >>> Exception: " << e.what();
            response.set_retcode(1);
            response.set_errormsg("MongoDB  Error.");
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        } 
    }

     ;
    LOG_INFO << ">>> " << __FUNCTION__ << " 用时["<< muduo::timeDifference(muduo::Timestamp::now(), timestart) <<"]秒"; 
}

void HallServer::cmd_get_match_record(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
   // LOG_DEBUG << ">>> HallServer::cmd_get_match_record >>>>>>>>>>>>>";
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_RES;
    ::HallServer::GetMatchRecordMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::GetMatchRecordResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t gameId = msg.gameid();
        int32_t roomId = msg.roomid();

        response.set_gameid(gameId);

        try
        {
            string strRoundId, matchname;
            int64_t score = 0, rank = 0;
            chrono::system_clock::time_point endDateTime;
            int64_t endDateTimeCount = 0;

            bsoncxx::document::element elem;
            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} << "matchroomid" << 1 << "matchname" << 1 << "rank" << 1 << "score" << 1 << "time" << 1 << finalize);
            opts.sort(document{} << "_id" << -1 << finalize);
            opts.limit(10);
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_record"];
            //document query_value{};// =document {} <<"userid" << userId << "gameid" << gameId ;
            auto query = document{} << "userid" << userId << "gameid" << gameId << finalize;
            if (roomId && roomId != 0)
                query = document{} << "userid" << userId << "gameid" << gameId << "roomid" << roomId << finalize;

            //document{}<<"userid" << userId << "gameid" << gameId<< "roomid"<<roomId<< finalize
            mongocxx::cursor cursor = coll.find(query.view(), opts); // (query_value.view());

            for(auto &doc : cursor)
            {
                strRoundId = doc["matchroomid"].get_utf8().value.to_string();
                matchname = doc["matchname"].get_utf8().value.to_string();
                score = doc["score"].get_int64();
                rank = doc["rank"].get_int32();
                endDateTime = doc["time"].get_date();
                endDateTimeCount = (chrono::duration_cast<chrono::seconds>(endDateTime.time_since_epoch())).count();

                ::HallServer::MatchRecordInfo *matchRecordInfo = response.add_detailinfo();

                matchRecordInfo->set_matchname(matchname);
                matchRecordInfo->set_rank(rank);
                matchRecordInfo->set_winlosescore(score);
                matchRecordInfo->set_matchroundno(strRoundId);
                matchRecordInfo->set_recordtime(endDateTimeCount);
            }
            response.set_retcode(0);
            response.set_errormsg("CMD GET USER SCORE OK.");
           // LOG_DEBUG << ">>> HallServer::cmd_get_match_record >>>>>>>>>>>>>success";

        }catch(exception &e)
        {
            LOG_ERROR<< " >>> Exception: " << e.what();
            response.set_retcode(1);
            response.set_errormsg("MongoDB  Error.");
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

void HallServer::cmd_get_match_best_record(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    //LOG_DEBUG << ">>> HallServer::cmd_get_match_best_record >>>>>>>>>>>>>";
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_RES;
    ::HallServer::GetMatchBestRecordMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        ::HallServer::GetMatchBestRecordResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t gameId = msg.gameid();
        int32_t matchId = msg.matchid();

        try
        {
            chrono::system_clock::time_point dateTime;
            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} << "rank" << 1 <<"awardscore"<< 1 << "time" << 1 <<finalize);
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_best_record"];
            bsoncxx::stdx::optional<bsoncxx::document::value> result =
            coll.find_one(document{} << "userid" << userId<<"matchid" << matchId << finalize, opts);
            bool updateTime=false;
            response.set_retcode(1);
            if(result)
            {
                bsoncxx::document::view view = result->view();
                int32_t score =view["awardscore"].get_int32();
                int32_t rank =view["rank"].get_int32();
                dateTime = view["time"].get_date();
                int64_t dateTimeCount= (chrono::duration_cast<chrono::seconds>(dateTime.time_since_epoch())).count();
                 response.set_bestrank(rank);
                 response.set_matchendtime(dateTimeCount);
                 response.set_totalaward(score);
                 response.set_retcode(0);
            }

            //LOG_DEBUG << ">>> HallServer::cmd_get_match_best_record >>>>>>>>>>>>>success";

        }catch(exception &e)
        {
            LOG_ERROR<< " >>> Exception: " << e.what();
            response.set_retcode(1);
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
// 开关幸运转盘
void HallServer::cmd_switch_luckyGame(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    //LOG_DEBUG << __FILE__ << " " << __FUNCTION__;
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_RES;

    // request
    ::HallServer::GetSwitchLuckyGameRequest msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetSwitchLuckyGameResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t nOperateType   = msg.noperatetype();

        // 开放状态
        bool isOpen = m_LuckyGame->CheckIsOpen(m_userId_current_map[userId]);
        bool isEnoughJifen = false;  

       LOG_INFO << "isOpen 状态 " << isOpen << " userId " << userId <<" nOperateType "<< ((nOperateType == 0) ? "开启" : "关闭");

        //退出游戏请求
        if (nOperateType == 1)
        { 
            return;
        } 

        int32_t retCode = 1,agentId = 0, currency = 0;
        int64_t luserScore = 0;
        int64_t luserJifenScore = 0;
        string account = "",lineCode = "";
        bool isLoadOk = m_LuckyGame->IsLoadOK();
        retCode = isLoadOk ? 1 : 2; //retCode 为 2时则为加载ini失败


        try
        {
            // 查数据库
            bsoncxx::document::element elem;
            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} << "userid" << 1 << "score" << 1 <<"agentid" << 1<<"account" << 1 <<"linecode" << 1 <<"integralvalue" << 1 << "currency" << 1 << finalize);

            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
            auto query_value = document{} << "userid" << userId << finalize;

            bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view(),opts);
            if(isLoadOk && maybe_result)
            {
                retCode = 0; 

                bsoncxx::document::view view = maybe_result->view();
                int64_t dbUserId    = view["userid"].get_int64(); 
                luserScore      = view["score"].get_int64(); 
                account         = view["account"].get_utf8().value.to_string(); 
                agentId         = view["agentid"].get_int32();  
                lineCode        = view["linecode"].get_utf8().value.to_string();
                luserJifenScore = view["integralvalue"].get_int64(); //.get_int32(); //

				//增加多币数支持
				if (view["currency"])
					currency = view["currency"].get_int32();

				// 获取玩家对应幸运转盘的配置
				tagConfig lkGame_config;
				memset(&lkGame_config, 0, sizeof(lkGame_config));
				lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

                //
                luserJifenScore = luserJifenScore / (lkGame_config.nExChangeRate * COIN_RATE);//100 写个全局的币率
                // 积分是否足够
                isEnoughJifen = m_LuckyGame->IsEnoughJF(luserJifenScore, currency);
            
                // LOG_INFO << "userId 7 " << userId <<" "<< luserJifenScore;

                //3组数据
                for (size_t i = 0; i < 3; i++){
                    for (size_t k = 0; k < AWARD_COUNT; k++) {
                        response.add_tgoldlists(lkGame_config.vItemArry[i].Gold[k]);
                        response.add_ticonlists(lkGame_config.vItemArry[i].Icon[k]);
                    }
                }
                
                //押分项
                for(int betItem : lkGame_config.vBetItemList){
                    response.add_tbetlists(betItem);
                }

                // 记录返回,取10个元素(3秒一次更新)
				/*static int64_t lastTick = 0;
				static vector<string> msgLists;*/
                if(  time(nullptr) - m_lastTickmap[currency] > 2 ){
					m_lastTickmap[currency] = (int64_t)time(nullptr);
					m_msgListsmap[currency].clear();
                    // 读取redis
					LOG_INFO << "读取redis " << "userId " << userId << " currency " << currency;
					if (currency == 0)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg, m_msgListsmap[currency], lkGame_config.MsgItemCount);
					else if (currency == 1)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg_1, m_msgListsmap[currency], lkGame_config.MsgItemCount);
					else if (currency == 2)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg_2, m_msgListsmap[currency], lkGame_config.MsgItemCount);
					else if (currency == 3)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg_3, m_msgListsmap[currency], lkGame_config.MsgItemCount);
					else if (currency == 4)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg_4, m_msgListsmap[currency], lkGame_config.MsgItemCount);
					else if (currency == 5)
						REDISCLIENT.lrangeCmd(eRedisKey::lst_lkGameMsg_5, m_msgListsmap[currency], lkGame_config.MsgItemCount);
                }
                
                // 填充数据
				if (m_msgListsmap.count(currency))
				{
					for (string msgItem : m_msgListsmap[currency]) {
						response.add_trecords(msgItem);
					}
				}                
            }
        }
        catch(exception &e)
        {
            retCode = 3;
            LOG_ERROR<< " >>> Exception: " << e.what();
        }
        // 发送
        response.set_nisopen((int)isOpen);                  //是否开启
        response.set_nretcode(retCode);                     //错误返回码
        response.set_noperatetype(nOperateType);            //错误返回码	
        response.set_luserscore(luserScore);                //当前玩家金币 	 
        response.set_lcurrentjifen(luserJifenScore);        //当前玩家积分	
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
 
    }
}

// 轮盘逻辑
void HallServer::cmd_get_luckyGame_logic(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_RES;

    // request
    ::HallServer::GetLuckyGameRequest msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        ::HallServer::GetLuckyGameResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int32_t retCode = 1;  
        int64_t userId = internal_header->userId;
        int32_t nbetindex   = msg.nbetindex();
        
        // 用户信息
        UserScoreInfo uScoreItem;
        // memset(&uScoreItem, 0, sizeof(uScoreItem));

        //1,获得开奖结果和取押分值
        int nResultId = -1;
        int32_t nBetScore = 0,currency = 0;
        int64_t lWinScore = 0;
        int64_t beforeScore = 0;
        int64_t beforeJFScore = 0;
        string strAccount,strRoundId = "";

        try
        {

            do
            {
                if (nbetindex < 0 || nbetindex >= 3)
                {
                    LOG_ERROR << "userId " << userId << " betindex " << nbetindex;
                    retCode = 1;
                    break;
                }
                // 判断是否在游戏时间
                if (!m_LuckyGame->CheckIsOpen(m_userId_current_map[userId]))
                {
                    retCode = 4;
                    break;
                }

                //nResultId = m_LuckyGame->GetGameResult(nbetindex, nBetScore, lWinScore);

                // 查数据库
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(document{} << "userid" << 1 << "score" << 1 << "agentid" << 1 << "account" << 1 << "linecode" << 1 << "integralvalue" << 1 << "currency" << 1 << finalize);

                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                auto query_value = document{} << "userid" << userId << finalize;

                auto maybe_result = coll.find_one(query_value.view(), opts);
                if (!maybe_result)
                {
                    retCode = 2; //查找不到玩家
                    break;
                }

                bsoncxx::document::view view = maybe_result->view();
                beforeScore = view["score"].get_int64();
                uScoreItem.account = view["account"].get_utf8().value.to_string();
                strAccount = view["account"].get_utf8().value.to_string();
                uScoreItem.agentId = view["agentid"].get_int32();
                uScoreItem.lineCode = view["linecode"].get_utf8().value.to_string();
                beforeJFScore = view["integralvalue"].get_int64(); //.get_int32(); //

				//增加多币数支持
                if (view["currency"]) 
                    currency = view["currency"].get_int32();
				// 获取玩家对应幸运转盘的配置
				tagConfig lkGame_config;
				memset(&lkGame_config, 0, sizeof(lkGame_config));
				lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);
                //
                beforeJFScore = beforeJFScore / (lkGame_config.nExChangeRate * COIN_RATE); //100 写个全局的币率
				nResultId = m_LuckyGame->GetGameResult(nbetindex, nBetScore, lWinScore, currency);
                //玩家积分不够
                if (beforeJFScore < nBetScore)
                {
                    retCode = 3;
                    break;
                }

                uScoreItem.lUserScore = beforeScore + lWinScore;
                uScoreItem.lUserJiFenScore = beforeJFScore - nBetScore;

                //生成牌局信息
                strRoundId = GetNewRoundId(userId % 1000000, lkGame_config.nRoomId);
                int64_t tempData = nBetScore * lkGame_config.nExChangeRate * COIN_RATE;
                //3,DB更新玩家金币，积分
                UpdateUserScoreToDB(userId, tempData, lWinScore);
                //4,保存操作步骤(SaveReplay)
                {
                    MutexLockGuard lock(m_mutex_save);
                    m_replay.roomname = "幸运转盘";
                    m_replay.clear();
                    m_replay.gameinfoid = strRoundId;
                    m_replay.addPlayer(userId, strAccount, uScoreItem.lUserScore, 0);
                    m_replay.addResult(0, 0, nBetScore, lWinScore, "", false);
                    m_replay.addStep(0, "", -1, 0, 0, 0);
                    SaveReplay(m_replay);
                }
                //5,写牌局记录信息
                string cardValue = to_string(nResultId) + "," + to_string(lWinScore);
                AddUserGameInfoToDB(userId, strAccount, strRoundId, cardValue, currency, beforeScore, nBetScore, lWinScore, uScoreItem.agentId, lkGame_config.nGameId, lkGame_config.nRoomId, uScoreItem.lineCode);
                //6,保存日志game_log
                //AddUserGameLogToDB(userId, strAccount, strRoundId, lWinScore, uScoreItem.agentId, lkGame_config.nGameId, lkGame_config.nRoomId);
                //7,保存玩家分更改记录
                AddScoreChangeRecordToDB(userId, strAccount, lWinScore, uScoreItem.agentId, uScoreItem.lineCode, beforeScore, lkGame_config.nRoomId);
                // 保存玩家积分更改记录
                JiFenChangeRecordToDB(userId, strAccount, uScoreItem.agentId, (-1 * nBetScore), beforeJFScore, eJiFenChangeType::op_lucky_game, "幸运转盘");
                //7.1,广播消息
                std::string msgStr = to_string((int32_t)time(nullptr)) + "," + to_string(userId) + "," + to_string(nBetScore) + "," + to_string(lWinScore);
                REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_luckyGame, msgStr);
                //7.2存储消息
                SaveLKBoardCastMsg(msgStr, currency);
                //7.3假消息
                int32_t tempWinScore = 0;
                int32_t nBetScoreTmp = 0;
                bool isCreateFakeMsg = m_LuckyGame->GetFakeResult(nBetScoreTmp, tempWinScore, currency);
                if (isCreateFakeMsg)
                {
                    msgStr = to_string((int32_t)time(nullptr) + rand() % 100) + "," + to_string(userId + rand() % 100) + "," + to_string(nBetScoreTmp) + "," + to_string(tempWinScore);
                    REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_luckyGame, msgStr);
                    SaveLKBoardCastMsg(msgStr, currency);
                }

                retCode = 0;
            } while (0);
        }
        catch (const std::exception &e)
        {
            retCode = 0;
            std::cerr << e.what() << '\n';
        }

        //8,发送回客户端
        response.set_nretcode(retCode);                         //返回结果码	
        response.set_nretid(nResultId);                         //开奖结果下标 	 
        response.set_luserscore(uScoreItem.lUserScore);         //当前玩家金币 	 
        response.set_lcurrentjifen(uScoreItem.lUserJiFenScore); //当前玩家积分	
        response.set_lscore(lWinScore);                         //获奖多少金币
        response.set_cbroundid(strRoundId);                     //牌局Id  
        
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len)){
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
} 
// 存储广播信息
void HallServer::SaveLKBoardCastMsg(string msgStr, int32_t currency)
{
    int repeatCount = 50;//重试50次后放弃存储
    std::string lockStr = to_string(::getpid());
    do
    {
        int ret = REDISCLIENT.setnxCmd(eRedisKey::str_lockId_lkMsg, lockStr, 2);
        if (ret == -1)
        {
            LOG_ERROR << "redis连接失败";
            break;
        }
        if (ret == 1)
        {
            long long msgListLen = 0;
			LOG_INFO << "SaveLKBoardCastMsg redis currency " << currency;
			if (currency == 0)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg, msgStr, msgListLen);
			else if (currency == 1)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg_1, msgStr, msgListLen);
			else if (currency == 2)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg_2, msgStr, msgListLen);
			else if (currency == 3)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg_3, msgStr, msgListLen);
			else if (currency == 4)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg_4, msgStr, msgListLen);
			else if (currency == 5)
				REDISCLIENT.lpushCmd(eRedisKey::lst_lkGameMsg_5, msgStr, msgListLen);

            //7.3移除超出的数量
            if (msgListLen > m_lkGame_config.MsgItemCount)
            {
                for (int idx = 0; idx < (msgListLen - m_lkGame_config.MsgItemCount); idx++)
                {
                    std::string lastElement;
					LOG_INFO << "SaveLKBoardCastMsg 移除 redis currency " << currency;
					if (currency == 0)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg, lastElement);
					else if (currency == 1)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg_1, lastElement);
					else if (currency == 1)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg_2, lastElement);
					else if (currency == 1)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg_3, lastElement);
					else if (currency == 1)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg_4, lastElement);
					else if (currency == 1)
						REDISCLIENT.rpopCmd(eRedisKey::lst_lkGameMsg_5, lastElement);
                   // LOG_WARN << "存储和移除消息[" << msgStr << " " << lastElement << "]";
                }
            }

            // 解锁(不是很有效)
            if (!REDISCLIENT.delnxCmd(eRedisKey::str_lockId_lkMsg, lockStr))
            {
                LOG_ERROR << "解锁失败[" << lockStr << "]";
            }

            break;
        }
        else
        {
            usleep(10 * 1000); // 10毫秒
          //  LOG_WARN << "10毫秒延时 " << repeatCount;
        }
    } 
    while (--repeatCount > 0);
}


// task related
void HallServer::cmd_get_task_list(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_INFO <<" " <<  __FUNCTION__ ;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_TASK_LIST_RES;
    try
    {
        // request
        ::HallServer::ReqGetUserTask msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            ::HallServer::ResGetUserTask response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            int32_t tasktype  = msg.tasktype();

            MutexLockGuard lock(m_userId_agentId_MutexLock);

            int64_t userId = internal_header->userId;
			int32_t currency = 0;
			// 先查询内存
			map<int64_t, int32_t>::iterator it;
			it = m_userId_current_map.find(userId);
			if (it == m_userId_current_map.end())
			{
				mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
				bsoncxx::stdx::optional<bsoncxx::document::value> user_result = user_coll.find_one(document{} << "userid" << userId << finalize);
				if (user_result)
				{
					auto view = user_result->view();
					//增加多币数支持
					if (view["currency"])
						currency = view["currency"].get_int32();
				}
				// 内存缓存
				m_userId_current_map.insert(make_pair(userId, currency));
				LOG_INFO << "---userid currency 已经缓存 " << userId << " currency" << currency << " " << m_userId_current_map.size();
			}
			else
			{
				currency = it->second;
			}
			
            m_TaskService->loadTaskStatus(currency);

            // 是否对该代理开放
            int32_t retCode = 0;
            int32_t agentId = 0; // 本玩家的代码号
            if (m_TaskService->status == 1)
            {
                // 先查询内存
                map<int64_t, int32_t>::iterator iter;
                iter = m_userId_agentId_map.find(userId);
                if (iter == m_userId_agentId_map.end())
                {
                    mongocxx::options::find opts = mongocxx::options::find{};
                    opts.projection(document{} << "agentid" << 1 << finalize);

                    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                    auto query_value = document{} << "userid" << userId << finalize;
                    auto maybe_result = coll.find_one(query_value.view(),opts);
                    if (maybe_result)
                    {
                        bsoncxx::document::view view = maybe_result->view();
                        agentId = view["agentid"].get_int32();
                    }
                    // 内存缓存
                    m_userId_agentId_map.insert(make_pair(userId, agentId));
                    LOG_INFO << "---agentId 已经缓存 " << userId << " " << agentId << " " << m_userId_agentId_map.size();
                }
                else
                {
                    agentId = iter->second;
                    // LOG_INFO  <<" userId  " << userId <<" agentId  " << " "<< agentId << " " << m_userId_agentId_map.size() ;
                }
            }

            bool isPermitShowTask = m_TaskService->isPermitShowTask(agentId);
            // 如果代理开放，且任务功能开放
            if ((m_TaskService->status == 1) && isPermitShowTask)
            {
                map<int32_t, vector<tagUserTaskInfo>> tasks;

                // 2秒内不重新加载
                int64_t curTickTime = time(nullptr) - m_get_task_tick[userId];
                if ( curTickTime < 2 )
                {
                   tasks = m_user_tasks[userId];
                }
                else{
                    m_TaskService->getUserTasks(userId, tasks, currency);
                    m_user_tasks[userId] = tasks;
                }
                m_get_task_tick[userId] = time(nullptr);

                LOG_INFO << "---getUserTasks[" << userId << "],size[" << tasks.size() <<"],curTickTime[" << curTickTime << "],markCount["<<m_user_tasks.size()<<"] " << (( curTickTime < 10 ) ? "使用内存" : "重新加载");

                for (map<int32_t, vector<tagUserTaskInfo>>::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
                {
                    uint32_t _taskType = it->first;
                    // LOG_INFO << "--- tasks Type[" << _taskType << "]";

                    for (auto taskitem : it->second)
                    {
                        auto item=m_proxyInfoMap.begin();
                        item=m_proxyInfoMap.find(agentId);
                        if(item!=m_proxyInfoMap.end())
                        {
                            auto itemProxy=find(m_proxyInfoMap[item->first].GamesVec.begin(),m_proxyInfoMap[item->first].GamesVec.end(),taskitem.taskGameId);
                            if(itemProxy!=m_proxyInfoMap[item->first].GamesVec.end())
                            {

                            }
                            else
                            {
                                continue;
                            }
                        }

                        ::HallServer::TaskInfo *task;
                        if (_taskType == 0) {
                            task = response.add_tasklist();
                        }
                        else if (_taskType == 1) {
                            task = response.add_tasklistw();
                        }
                        else if (_taskType == 2) {
                            task = response.add_tasklistm();
                        }
                        
                        task->set_taskid(taskitem.taskId);
                        task->set_taskstatus(taskitem.taskStatus);
                        task->set_taskprogress(taskitem.taskProgress);
						//task->set_taskcurrency(taskitem.taskCurrency);
                        // 详细版本内则全部返回
                        if( tasktype == 0 ){
                            task->set_taskname(taskitem.taskName);
                            task->set_tasktype(taskitem.taskType);
                            task->set_taskgameid(taskitem.taskGameId);
                            task->set_taskroomid(taskitem.taskRoomId);
                            task->set_tasktotal(taskitem.taskQty);
                            task->set_taskawardgold(taskitem.taskAwardGold);
                            task->set_taskawardscore(taskitem.taskAwardScore);
                        }
                    }
                }
               
                retCode = 1;
            }
            else
            {
                retCode = (isPermitShowTask == false) ? 3 : 2;
            }
            
			// int64_t curStamp = GlobalFunc::GetCurrentStamp64() / 1000;
            response.set_errcode(retCode);
            // response.set_servertime(curStamp);
            // 
            // LOG_INFO << ">>> Get Current TimeStamp----" << curStamp;

            size_t len = response.ByteSize();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }
    catch (exception excep)
    {
        LOG_ERROR << "================" << __FUNCTION__ << " " << excep.what();
    }
}
// 领取任务
void HallServer::cmd_get_task_award(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_AWARDS_RES;

    try
    {
        // request
        ::HallServer::ReqGetTaskRewards msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            ::HallServer::ResGetTaskReward response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            int64_t userId = internal_header->userId;
            int32_t taskId = msg.taskid();
            int32_t cycleId =msg.cycleid(); //周期ID
            int32_t retCode = 0; //返回结果码

            LOG_WARN << "---"<< __FUNCTION__<< " userId[" << userId <<"],["<<taskId<<"],cycleId["<<cycleId <<"]";

            //TODO check task status and lock this
            MutexLockGuard lock(m_t_mutex);
            tagUserTaskInfo taskInfo = {0};
            bool isFindTask = false;

            // 1，查找用户的任务列表
            mongocxx::collection user_task_coll = MONGODBCLIENT["gamemain"]["game_user_task"];
            bsoncxx::document::value query_value = bsoncxx::builder::stream::document{} << "userId" << userId << "cycle" << cycleId << finalize;
            bsoncxx::stdx::optional<bsoncxx::document::value> find_result = user_task_coll.find_one(query_value.view());
            if ( find_result )
            {
                bsoncxx::document::view view = find_result->view();
                auto arr = view["tasks"].get_array();
                for (auto &taskDoc : arr.value)
                {
                    int32_t t_task_id = taskDoc["task_id"].get_int32();
                    // 查找玩家对应的任务
                    if (taskId == t_task_id)
                    {
                        taskInfo.taskId = t_task_id;
                        taskInfo.taskGameId = taskDoc["task_game_id"].get_int32();
                        taskInfo.taskRoomId = taskDoc["task_room_id"].get_int32();
                        taskInfo.taskType = taskDoc["task_type"].get_int32();
                        taskInfo.taskStatus = taskDoc["status"].get_int32();
                        taskInfo.taskQty = taskDoc["task_quantity"].get_int64();
                        taskInfo.taskAwardGold = taskDoc["task_award_gold"].get_int64();
                        taskInfo.taskAwardScore = taskDoc["task_award_score"].get_int64();
                        taskInfo.taskName = taskDoc["task_name"].get_utf8().value.to_string();
						if (taskDoc["currency"])
						{
							taskInfo.taskCurrency = taskDoc["currency"].get_int32();
						}
                        isFindTask = true;
                        break; // return true;
                    }
                }
            }

            // 2，如果找到任务列表
            if (isFindTask)
            {
                LOG_WARN << "---"<< __FUNCTION__<< " isFindTask[" << isFindTask <<"]";
                response.set_taskid(taskId);
                // 2.1 任务未完成
                if (taskInfo.taskStatus != TaskStatusFinished)
                {
                    retCode = 2;
                    response.set_taskstatus(taskInfo.taskStatus);
                }
                else
                { 
                    // 2.2  任务完成
					int32_t currency = 0;
                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                    bsoncxx::stdx::optional<bsoncxx::document::value> user_result = user_coll.find_one(document{} << "userid" << userId << finalize);
                    if (user_result)
                    {
                        auto view = user_result->view();
						//增加多币数支持
						if (view["currency"])
							currency = view["currency"].get_int32();

						// 获取玩家对应幸运转盘的配置
						tagConfig lkGame_config;
						memset(&lkGame_config, 0, sizeof(lkGame_config));
						lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

                        //2.3 奖励金币
                        if (taskInfo.taskAwardGold != 0)
                        {
                            user_coll.update_one(document{} << "userid" << userId << finalize,
                                                 document{} << "$inc" << open_document
                                                            << "gamerevenue" << taskInfo.taskAwardGold
                                                            << "winorlosescore" << taskInfo.taskAwardGold
                                                            << "score" << taskInfo.taskAwardGold << close_document
                                                            << finalize);

                            LOG_WARN << "---奖励金币 taskAwardGold[" << taskInfo.taskAwardGold <<"]";

                        }

                        //2.4 奖励积分
                        if (taskInfo.taskAwardScore != 0)
                        {
                            int64_t award_amount = taskInfo.taskAwardScore * lkGame_config.nExChangeRate * COIN_RATE;
                            user_coll.update_one(document{} << "userid" << userId << finalize,
                                                 document{} << "$inc" << open_document
                                                            << "integralvalue" << award_amount << close_document
                                                            << finalize);
                            LOG_WARN << "---奖励积分 taskAwardScore[" << taskInfo.taskAwardScore <<"]";
                        }

                        //2.5 更新任务状态
                        user_task_coll.update_one(bsoncxx::builder::stream::document{} << "userId" << userId << "tasks.task_id" << taskId << finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << open_document
                                                                                       << "tasks.$.status" << TaskStatusAwarded << close_document << finalize);
                        int32_t agentid = view["agentid"].get_int32();
                        string linecode = view["linecode"].get_utf8().value.to_string();
                        string account = view["account"].get_utf8().value.to_string();
                        string roundId = GetNewRoundId(userId, TASK_ROOM);
                        int64_t score = view["score"].get_int64();
                        int64_t integralvalue = view["integralvalue"].get_int64();

                        //2.6  写领取记录
                        m_TaskService->AddTaskRecord(userId, agentid, linecode, account, roundId, score, integralvalue, taskInfo);                                             

                        //2.7 整理返回数据
                        response.set_taskstatus(TaskStatusAwarded);
                        response.set_userscore(score + taskInfo.taskAwardGold);
                        response.set_userjifen(integralvalue / (lkGame_config.nExChangeRate * COIN_RATE) + taskInfo.taskAwardScore);
                        // 结果码
                        retCode = 1;
                    }
                }
            }

            // 没找到任务列表
            if (!isFindTask)
            {
                retCode = 4;
                LOG_INFO << "cannot find task [" << (int)taskId << "] for user:" << (int)userId;
            }

            // 结果码
            response.set_errcode(retCode);
            size_t len = response.ByteSize();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }
    catch (exception excep)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << excep.what();
    }
}


void HallServer::WriteGameInfoToMysql(string roundid,int64_t userid,string account,
        int32_t agentid,string linecode,int32_t gameid
       ,int32_t roomid,string cardvalue
       ,int64_t beforescore,int64_t betscore,int64_t winscore)
{


//            LOG_ERROR<<"进入mysql 线程调用 ";
//            //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//            try
//            {
//            LOG_ERROR<<"获取数据库链接前";
//            auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//            LOG_ERROR<<"获取数据库链接成功";
//            conn->setSchema(DB_RECORD);

//            LOG_ERROR<<"setSchema成功";
//            pstmt.reset(conn->prepareStatement("INSERT INTO play_record_lkgame(gameinfoid, userid,account,agentid, "
//                        "linecode,gameid,roomid,tableid,chairid,isBanker,"
//                        "cardvalue,usercount,beforescore,rwinscore,allbet,winscore,"
//                        "score,revenue,isandroid,gamestarttime,gameendtime)"
//                        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
//            LOG_ERROR<<"cardValue  : " <<cardvalue;
//            int32_t zeroNum = 0;
//            int32_t countnum = 1;
//            LOG_ERROR<<"插入命令生成成功";
//            pstmt->setString(1,roundid);
//            LOG_ERROR<<"插入1" <<roundid;
//            pstmt->setInt64(2, userid);
//            LOG_ERROR<<"插入2"<<userid;
//            pstmt->setString(3, account);
//            LOG_ERROR<<"插入3"<<account;
//            pstmt->setInt(4, agentid);
//            LOG_ERROR<<"插入4"<<agentid;
//            pstmt->setString(5, linecode);
//            LOG_ERROR<<"插入5"<<linecode;
//            pstmt->setInt(6, gameid);
//            LOG_ERROR<<"插入6"<<gameid;
//            pstmt->setInt(7, roomid);
//            LOG_ERROR<<"插入7"<<roomid;
//            pstmt->setInt(8, zeroNum);
//            LOG_ERROR<<"插入8";
//            pstmt->setInt(9, zeroNum);
//            LOG_ERROR<<"插入9";
//            pstmt->setInt(10, zeroNum);
//            LOG_ERROR<<"插入10";
//            pstmt->setString(11, cardvalue);
//            LOG_ERROR<<"插入11"<<cardvalue;
//            pstmt->setInt(12, countnum);
//            LOG_ERROR<<"插入12";
//            pstmt->setInt64(13, beforescore);
//            LOG_ERROR<<"插入13"<<beforescore;
//            pstmt->setInt64(14, betscore);
//            LOG_ERROR<<"插入14"<<betscore;
//            pstmt->setInt64(15, betscore);
//            LOG_ERROR<<"插入15"<<betscore;
//            pstmt->setInt64(16, winscore);
//            LOG_ERROR<<"插入16"<<winscore;
//            pstmt->setInt64(17, beforescore + winscore);
//            LOG_ERROR<<"插入17"<<beforescore + winscore;
//            pstmt->setInt(18, zeroNum);
//            LOG_ERROR<<"插入18";
//            pstmt->setInt(19, zeroNum);
//            LOG_ERROR<<"插入19";
//            pstmt->setDateTime(20, HallServer::InitialConversion(time(NULL)));
//            LOG_ERROR<<"插入20"<<HallServer::InitialConversion(time(NULL));
//            pstmt->setDateTime(21, HallServer::InitialConversion(time(NULL)));
//            LOG_ERROR<<"插入21"<<HallServer::InitialConversion(time(NULL));
//            pstmt->executeUpdate();
//            LOG_ERROR<<"executeUpdate成功";
//            }
//            catch (sql::SQLException &e)
//            {
//            std::cout << " (MySQL error code: " << e.getErrorCode();
//            LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//            }

 }

bool HallServer::AddUserGameInfoToDB(int64_t userId, string &account, string &strRoundId, string &cardValue,int32_t currency,int64_t beforeScore,
                                        int64_t nBetScore,int64_t lWinScore,int32_t agentId,int32_t gameId,int32_t roomId,string linecode)
{
    bool bSuccess = false;

    
    //LOG_INFO << " cardValue " << cardValue; 
    

    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
                        << "gameinfoid" << strRoundId
                        << "userid" << userId
                        << "account" << account
                        << "agentid" << agentId
                        << "linecode" << linecode
                        << "gameid" << gameId
                        << "roomid" << roomId
                        << "tableid" << (int32_t)0 //(int32_t)m_TableState.nTableID
                        << "chairid" << (int32_t)0 //(int32_t)scoreInfo->chairId
                        << "isBanker" << (int32_t)0 //(int32_t)scoreInfo->isBanker
                        << "cardvalue" << cardValue
                        << "usercount" << (int32_t)1 //userCount
                        << "beforescore" << beforeScore
                        << "rwinscore" << (int32_t)nBetScore // scoreInfo->rWinScore //2019-6-14 有效投注额
                        << "currency" << currency //币种
                        << "cellscore" << open_array;
    
    // for (int64_t &betscore : scoreInfo->cellScore)
        // insert_value = insert_value << betscore;

    auto after_array = insert_value << close_array;
    after_array
        << "allbet" << (int32_t)nBetScore //scoreInfo->betScore
        << "winscore" << lWinScore //scoreInfo->addScore 
        << "score" << beforeScore + lWinScore // scoreInfo->beforeScore + scoreInfo->addScore
        << "revenue" << (int64_t)0 // scoreInfo->revenue
        << "isandroid" << false //bAndroidUser
        << "gamestarttime" << bsoncxx::types::b_date(chrono::system_clock::now()) //scoreInfo->startTime)
        << "gameendtime" << bsoncxx::types::b_date(chrono::system_clock::now());

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    // LOG_INFO<<"cardValue  : " <<cardValue;

    //m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteGameInfoToMysql, this,strRoundId,userId,account,agentId,linecode,gameId,roomId,cardValue,beforeScore,nBetScore,lWinScore));
    //WriteGameInfoToMysql(strRoundId,userId,account,agentId,linecode,gameId,roomId,cardValue,beforeScore,nBetScore,lWinScore);

    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record_lkgame"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(doc.view());

        if (_result)
            bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}

void HallServer::SaveReplay(tagGameReplay& replay)
{ 
    if(replay.players.size() == 0 || !replay.players[0].flag) return ;
 
    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
            << "gameinfoid" << replay.gameinfoid
            << "timestamp" <<  b_date{ chrono::system_clock::now() }
            << "roomname" << replay.roomname
            << "cellscore" << replay.cellscore
            << "players" << open_array;
    for(tagReplayPlayer &player : replay.players)
    {
        if(player.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "userid" << player.userid
                                        << "account" << player.accout
                                        << "score" << player.score
                                        << "chairid" << player.chairid
                                        << bsoncxx::builder::stream::close_document;
        }
    }
    insert_value << close_array << "steps" << open_array;
    for(tagReplayStep &step : replay.steps)
    {
        if(step.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "bet" << step.bet
                                        << "time" << step.time
                                        << "ty" << step.ty
                                        << "round" << step.round
                                        << "chairid" << step.chairId
                                        << "pos" << step.pos
                                        << bsoncxx::builder::stream::close_document;
        }
    }

    insert_value << close_array << "results" << open_array;

    for(tagReplayResult &result : replay.results)
    {
        if(result.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "chairid" << result.chairId
                                        << "bet" << result.bet
                                        << "pos" << result.pos
                                        << "win" << result.win
                                        << "cardtype" << result.cardtype
                                        << "isbanker" << result.isbanker
                                        << bsoncxx::builder::stream::close_document;


        }
    }
    auto after_array = insert_value << close_array;

    auto doc = after_array << bsoncxx::builder::stream::finalize;


    // LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(doc.view());
    }
    catch (const std::exception &e)
    { 
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }
}

bool HallServer::UpdateUserScoreToDB(int64_t userId, int64_t nBetScore,int64_t addScore)
{
    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                        <<"integralvalue"<<(-nBetScore)
                        <<"winorlosescore" << addScore
                        <<"score" << addScore << close_document
                        << finalize); 
    }
    catch (const std::exception &e)
    { 
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
        return false;
    }
    return true;
}
void HallServer::WriteScoreRecordToMysql(int64_t userid,string account,
                          int32_t agentid,string linecode
                         ,int32_t roomid
                         ,int64_t addscore,int64_t beforeScore)
{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO user_score_record(userid, account, "
//                                           "agentid,linecode,changetype,changemoney,beforechange,"
//                                           "afterchange,logdate)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?)"));
//        pstmt->setInt64(1, userid);
//        pstmt->setString(2, account);
//        pstmt->setInt(3, agentid);
//        pstmt->setString(4, linecode);
//        pstmt->setInt(5, roomid);
//        pstmt->setInt64(6, addscore);
//        pstmt->setInt64(7, beforeScore);
//        pstmt->setInt64(8, beforeScore + addscore);
//        pstmt->setDateTime(9, InitialConversion(int64_t(time(NULL))));
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
bool HallServer::AddScoreChangeRecordToDB(int64_t userId,string &account,int64_t addScore,int32_t agentId,string linecode,int64_t beforeScore,int32_t roomId)
{
    bool bSuccess = false;
 
    auto insert_value = bsoncxx::builder::stream::document{}
            << "userid" << userId
            << "account" << account
            << "agentid" << agentId
            << "linecode" << linecode
            << "changetype" << roomId
            << "changemoney" << addScore
            << "beforechange" << beforeScore
            << "afterchange" << beforeScore + addScore
            << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
            << bsoncxx::builder::stream::finalize;

    // LOG_DEBUG << "Insert Score Change Record:"<<bsoncxx::to_json(insert_value);
    LOG_ERROR<<"mysql插入  user_score_record";
    m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteScoreRecordToMysql,this, userId, account,agentId,linecode,roomId,addScore,beforeScore));
    try{
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(insert_value.view());
        if(_result)
            bSuccess = true; 
    }
    catch (const std::exception &e)
    { 
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}
void HallServer::WriteGameLogToMysql(int64_t userid,string gamelogid,
                          int32_t agentid,string account,int32_t gameid,int32_t roomid
                         ,int64_t addscore,int64_t revenue)

{

//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO game_log(gamelogid, userid, "
//                                           "account,agentid,gameid,roomid,winscore,"
//                                           "revenue,logdate)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?)"));
//        pstmt->setString(1, gamelogid);
//        pstmt->setInt64(2, userid);
//        pstmt->setString(3, account);
//        pstmt->setInt(4, agentid);
//        pstmt->setInt(5, gameid);
//        pstmt->setInt(6, roomid);
//        pstmt->setInt64(7, addscore);
//        pstmt->setInt64(8, revenue);
//        pstmt->setDateTime(9, InitialConversion(int64_t(time(NULL))));
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
bool HallServer::AddUserGameLogToDB(int64_t userId,string &account,string &strRoundId,int64_t addScore,int32_t agentId,int32_t gameId,int32_t roomId)
{
    bool bSuccess = false;
    auto insert_value = bsoncxx::builder::stream::document{}
            << "gamelogid" << strRoundId << "userid" << userId
            << "account" << account << "agentid" << agentId
            << "gameid" << gameId<< "roomid" << roomId
            << "winscore" << addScore << "revenue" << 0
            << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
            << bsoncxx::builder::stream::finalize;

    // LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);
    LOG_ERROR<<"mysql插入  game_log";
    //不用lambda表达式是因为两个线程用相同变量可能导致严重问题
    m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteGameLogToMysql,this, userId, strRoundId,
                                                                   agentId, account, gameId, roomId                                                                 , addScore, 0));
    try
    {
        // mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_log"];
        // bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(insert_value.view());
        // if ( _result )
        //     bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}
  
void HallServer::reLoginHallServer(TcpConnectionWeakPtr &weakConn, internal_prev_header *internal_header)
{ 

}

void HallServer::WriteDailyBindMobileRecord(int64_t userId)
{ 
}
 

void HallServer::OnLoadParameter()
{
 
}

void HallServer::OnSendNoticeMessage()
{
    if(IsMasterHallServer())
        SendNoticeMessage();
}

bool HallServer::IsMasterHallServer()
{
    bool isMaster = false;
    vector<string> hallServers;
    if(ZOK == m_zookeeperclient->getClildrenNoWatch("/GAME/HallServers", hallServers))
    {
        if(hallServers.size() > 0 && hallServers[0] == m_szNodeValue)
            isMaster = true;
    }
    return isMaster;
}

void HallServer::SendNoticeMessage()
{ 
}

 //判断是否百人游戏
int HallServer::GetGameType(string msgStr)
{
	int type = 1;
	vector<string> vec;
	boost::algorithm::split(vec, msgStr, boost::is_any_of("-"));
	int32_t tempId = stoi(vec[0]) / 100;
	int32_t gameId = (tempId == 88 ? tempId * 10 : stoi(vec[0]) / 10);
    LOG_INFO << "得到游戏类型gameId:" << gameId;
	map<int32_t, int32_t>::iterator iter;
	iter = m_gameType_map.find(gameId);
	if (iter != m_gameType_map.end())
	{
		type = iter->second;
	}
	else
	{
        LOG_WARN << "判断游戏类型错误,gameId;" << gameId;
	}
	return type;
}

// 保险箱
void  HallServer::cmd_safe_box(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SAFE_BOX_SCORE_RES;

    try
    {
        // request
        ::HallServer::SafeBoxScoreMessage msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            ::HallServer::SafeBoxScoreResponse response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            int64_t userId = internal_header->userId;
            int64_t userId2 = msg.userid();
            int32_t opType = msg.optype(); //0 score ->safe socre , 1 safescore-> score
            int64_t trangeScore =msg.trangescore(); //兑换值
            int32_t retCode = 0;                    //返回结果码
            int64_t affterScore = 0;                //更新后玩家分	
            int64_t affterSafeBox = 0;              //更新后保险箱分

            LOG_WARN << "---"<< __FUNCTION__<< " userId[" << userId <<"],["<<opType<<"],trangeScore["<<trangeScore <<"],userId2["<<userId2 <<"]";

            try
            {
                do
                {
                    // 参数过滤
                    if ( trangeScore <= 0 || trangeScore > 9999999999 || opType < 0 || opType > 1)
                    {
                        retCode = 5;
                        break;
                    }

                    //TODO check task status and lock this
                    MutexLockGuard lock(m_t_mutex);

                    int64_t userScore = 0;
                    int64_t safescore = 0;
                    int32_t agentid = 0;
                    string strAccount, strlinecode;

                    mongocxx::options::find opts = mongocxx::options::find{};
                    opts.projection(bsoncxx::builder::stream::document{} << "score" << 1 << "safebox" << 1 << "agentid" << 1 << "account" << 1 << "linecode" << 1 << bsoncxx::builder::stream::finalize);

                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                    bsoncxx::stdx::optional<bsoncxx::document::value> user_result = user_coll.find_one(document{} << "userid" << userId << finalize, opts);
                    if (!user_result)
                    {
                        retCode = 4;
                        break;
                    }

                    auto view = user_result->view();
                    userScore = view["score"].get_int64();
                    safescore = view["safebox"].get_int64();
                    agentid = view["agentid"].get_int32();
                    strAccount = view["account"].get_utf8().value.to_string();
                    strlinecode = view["linecode"].get_utf8().value.to_string();

                    LOG_INFO << "---前 userId[" << userId << "],opType[" << opType << "],userScore[" << userScore << "],safescore[" << safescore << "],操作金额[" << trangeScore << "]";
                    // 设置需要返回修改之后的数据
                    mongocxx::options::find_one_and_update options;
                    options.return_document(mongocxx::options::return_document::k_after);

                    //score ->safe socre
                    if (opType == 0)
                    {
                        if (trangeScore > userScore)
                        {
                            retCode = 1; // score not enough
                            break;
                        }

                        auto result = user_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                                    document{} << "$inc" << open_document
                                                                               << "score" << -trangeScore
                                                                               << "safebox" << trangeScore << close_document
                                                                               << finalize,options);
                        if (result)
                        {
                            auto view = result->view();
                            affterScore = view["score"].get_int64();
                            affterSafeBox = view["safebox"].get_int64();
                        }
                    }
                    else
                    {
                        if (trangeScore > safescore)
                        {
                            retCode = 2; // score not enough
                            break;
                        }

                        auto result = user_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                                    document{} << "$inc" << open_document
                                                                               << "score" << trangeScore
                                                                               << "safebox" << -trangeScore << close_document
                                                                               << finalize, options);
                        if (result)
                        {
                            auto view = result->view();
                            affterScore = view["score"].get_int64();
                            affterSafeBox = view["safebox"].get_int64();
                        }
                    }

                    LOG_INFO << "---后 userId[" << userId << "],retCode[" << retCode << "],opType[" << opType << "],userScore[" << affterScore << "],safescore[" << affterSafeBox << "]";

                    //写帐变记录，保存玩家分更改记录
                    if(retCode == 0){
                        trangeScore = (opType == 0) ? (-trangeScore) : trangeScore;
                        AddScoreChangeRecordToDB(userId, strAccount, trangeScore, agentid, strlinecode, userScore, eUserScoreChangeType::op_safe_box_operate);
                    }

                } while (0);
            }
            catch (const std::exception &e)
            {
                retCode = 3;
                LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
            }

            // 结果码
            response.set_affterscore(affterScore);
            response.set_afftersafebox(affterSafeBox);
            response.set_retcode(retCode);
            response.set_optype(opType);
            response.set_trangescore(trangeScore);
            size_t len = response.ByteSize();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }
    catch (exception excep)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << excep.what();
    }
}

// 兑换
void HallServer::cmd_extrange_order(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_INFO << " " << __FILE__ << " " << __FUNCTION__;
    internal_header->nOK = 0;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_EXTRANGE_ORDER_RES;

    // request
    ::HallServer::ExtrangeOrderMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        ::HallServer::ExtrangeOrderResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = msg.userid();
        int32_t opType = msg.optype();               // 0-支付宝 1 银行卡 2-微信 3-USDT
        int64_t extrangescore = msg.extrangescore(); //0 score ->safe socre , 1 safescore-> score
        int32_t retCode = 1;                         //返回结果码

        LOG_WARN << "---" << __FUNCTION__ << " userId[" << userId << "],opType[" << opType << "],extrangescore[" << extrangescore << "]";

        //TODO check task status and lock this
        MutexLockGuard lock(m_t_mutex);
        //score ->safe socre
        try
        {
            do
            {
                if(extrangescore < 0 || opType < ePlaymentType::op_pay_ali || opType > ePlaymentType::op_pay_usdt){
                    retCode = 6;
                    LOG_WARN << "---参数有错误---";
                    break;
                }
                string dbaddress, dbaccount, dbaccountname, dbaccounttype, dbpaymentagreement;
                mongocxx::collection banker_coll = MONGODBCLIENT["gamemain"]["user_bank_info"];
                auto find_doc = document{} << "userid" << userId << "type" << opType << "firstchoice" << 1 << finalize;
                auto banker_result = banker_coll.find_one(find_doc.view());
                if (!banker_result)  //玩家绑定信息有错误
                {
                    retCode = 2;
                    LOG_WARN << "---玩家绑定信息有错误---";
                    break;
                }

                auto viewbaner = banker_result->view();


                 double beishu=0;
                 int32_t minStrageScore=0; // 最小提款金额
                 int32_t maxStrageScore=0; // 最大提款金额
                 int32_t timesperday=0;
                 int64_t dbadministrativefee=0;
                 double dbmandatoryfee=0;
                 double dbfrequentfee=0;

                 mongocxx::options::find opts = mongocxx::options::find{};
                 opts.projection(bsoncxx::builder::stream::document{} << "safebox" << 1 << "score" << 1 << "account" << 1  << "agentid" << 1
                                                                      << "totalvalidbet" << 1 << "grandtotalbet" << 1
                                                                      <<"basicwashbet" << 1<<"lastneedbet" << 1
                                                                      <<"subscoretimes"<< 1
                                                                      << bsoncxx::builder::stream::finalize);

                 mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                 auto user_result = user_coll.find_one(document{}<<"userid"<< userId<<finalize,opts);

                 mongocxx::collection coll = MONGODBCLIENT["gamemain"]["extrange_order"];

                 //查找待审核的
                 auto user_trage =coll.find_one(document{}<<"userid"<< userId<<"orderstatus"<<0<<finalize,opts);
                 if(user_trage)
                 {
                    retCode = 6;
                    break;
                 }


                 int64_t safeboxScore=0;
                 int64_t curscore=0;
                 string account="";
                 int32_t agentid=0;
                 int64_t dbtotalbet=0;
                 int64_t dbgrandtotalbet=0;
                 int64_t dbbasicwashbet=0;
                 int32_t dbtragehistimes=0;
                 int32_t dbcurrtragetimes = 0;
                 int64_t dblastneedbet = 0;



                 if (!user_result) //玩家信息有错误
                 {
                     retCode = 3;
                     LOG_WARN << "---" << __FUNCTION__ << " ---玩家信息有错误";
                     break;
                 }
                 else
                 {

                     auto view = user_result->view();

                     safeboxScore = view["safebox"].get_int64();

                     curscore = view["score"].get_int64();

                     account = view["account"].get_utf8().value.to_string();

                     agentid = view["agentid"].get_int32();

                     dbtotalbet = view["totalvalidbet"].get_int64();
                     dblastneedbet = view["lastneedbet"].get_int64();

                     dbgrandtotalbet = view["grandtotalbet"].get_int64(); //上次兑换记录的洗码量

                     dbbasicwashbet = view["basicwashbet"].get_int64(); //兑换需要的洗码量

                     dbtragehistimes =view["subscoretimes"].get_int32(); //兑换历史次数


                 }
                 mongocxx::options::find agentopts = mongocxx::options::find{};
                 agentopts.projection(bsoncxx::builder::stream::document{}
                                      << "washtotal" << 1
                                      << "freeWithdrawTime" << 1
                                      << "withdrawManageFee" << 1

                                      << "aliMinWithdrawMoney"<<1
                                      << "aliMaxWithdrawMoney" << 1
                                      << "aliCompulsoryWithdrawalFee"<< 1
                                      << "aliWithdrawFee"<< 1

                                      << "minWithdrawMoney" << 1
                                      << "maxWithdrawMoney"<< 1
                                      << "compulsoryWithdrawalFee"<< 1
                                      << "withdrawFee"<< 1


                                      << "wxMinWithdrawMoney" << 1
                                      << "wxMaxWithdrawMoney"<< 1
                                      << "wxCompulsoryWithdrawalFee"<< 1
                                      << "wxWithdrawFee"<< 1

                                      << "usdtMinWithdrawMoney" << 1
                                      << "usdtMaxWithdrawMoney"<< 1
                                      << "usdtCompulsoryWithdrawalFee"<< 1
                                      << "usdtWithdrawFee"<< 1
                                      << bsoncxx::builder::stream::finalize);
                 mongocxx::collection agent_coll = MONGODBCLIENT["gamemain"]["platform_withdrawconfig"];
                 auto agent_result = agent_coll.find_one(make_document(kvp("agentid", agentid)),agentopts);
                 if(!agent_result)
                 {
                     retCode = 4;
                     LOG_WARN << "---" << __FUNCTION__ << " 代理不存在";
                     break;
                 }
                 if (opType == ePlaymentType::op_pay_ali) //支付宝
                {
                    LOG_WARN << "---2op_pay_ali";
                    dbaccount = viewbaner["cardid"].get_utf8().value.to_string();
                    dbaccountname = viewbaner["cardname"].get_utf8().value.to_string();
                    // dbpaymentagreement = viewbaner["qrcode"].get_utf8().value.to_string();
                    auto viewagent = agent_result->view();

                     beishu = viewagent["washtotal"].get_double();   //提款所需打码量的倍数
                     timesperday = viewagent["freeWithdrawTime"].get_int32();  //免费取款次数
                     dbadministrativefee = viewagent["withdrawManageFee"].get_int32();  //行政费


                     minStrageScore= viewagent["aliMinWithdrawMoney"].get_int32();   //最小提款金额
                     maxStrageScore= viewagent["aliMaxWithdrawMoney"].get_int32();   //最大提款金额                                    
                     dbmandatoryfee = viewagent["aliCompulsoryWithdrawalFee"].get_double();    //洗码量不够收取的费用
                     dbfrequentfee =viewagent["aliWithdrawFee"].get_double();    //超次数收取的费用

                }
                else if (opType == ePlaymentType::op_pay_bank) //银行卡
                {
                    LOG_WARN << "---2op_pay_bank";
                    dbaddress = viewbaner["cardaddr"].get_utf8().value.to_string();
                    dbaccount = viewbaner["cardid"].get_utf8().value.to_string();
                    dbaccountname = viewbaner["cardname"].get_utf8().value.to_string();
                    if(viewbaner["cardcode"]&& viewbaner["cardcode"].type() != bsoncxx::type::k_null) 
                    dbaccounttype = viewbaner["cardcode"].get_utf8().value.to_string();
                    // dbpaymentagreement = viewbaner["qrcode"].get_utf8().value.to_string();
                    auto viewagent = agent_result->view();
                    beishu = viewagent["washtotal"].get_double();   //提款所需打码量的倍数
                    timesperday = viewagent["freeWithdrawTime"].get_int32();  //免费取款次数
                    dbadministrativefee = viewagent["withdrawManageFee"].get_int32();  //行政费

                    minStrageScore= viewagent["minWithdrawMoney"].get_int32();   //最小提款金额
                    maxStrageScore= viewagent["maxWithdrawMoney"].get_int32();   //最大提款金额
                    dbmandatoryfee = viewagent["compulsoryWithdrawalFee"].get_double();    //洗码量不够收取的费用
                    dbfrequentfee =viewagent["withdrawFee"].get_double();    //超次数收取的费用
                    LOG_WARN << "---3op_pay_bank";
                }
                else if (opType == ePlaymentType::op_pay_wx) //微信
                {
                    LOG_WARN << "---2op_pay_wx";
                    dbaccount = viewbaner["cardid"].get_utf8().value.to_string();
                    dbaccountname = viewbaner["cardname"].get_utf8().value.to_string();
                    dbpaymentagreement = viewbaner["qrcode"].get_utf8().value.to_string();
                    auto viewagent = agent_result->view();

                    beishu = viewagent["washtotal"].get_double();   //提款所需打码量的倍数
                    timesperday = viewagent["freeWithdrawTime"].get_int32();  //免费取款次数
                    dbadministrativefee = viewagent["withdrawManageFee"].get_int32();  //行政费

                    minStrageScore= viewagent["wxMinWithdrawMoney"].get_int32();   //最小提款金额
                    maxStrageScore= viewagent["wxMaxWithdrawMoney"].get_int32();   //最大提款金额
                    dbmandatoryfee = viewagent["wxCompulsoryWithdrawalFee"].get_double();    //洗码量不够收取的费用
                    dbfrequentfee =viewagent["wxWithdrawFee"].get_double();    //超次数收取的费用

                    LOG_WARN << "wx ----trage dbaccount" << dbaccount << "    dbaccountname" << dbaccountname;
                }
                else if (opType == ePlaymentType::op_pay_usdt) //ustd
                {
                    LOG_WARN << "---2op_pay_usdt";                   
                    dbaddress = viewbaner["cardaddr"].get_utf8().value.to_string();
                    dbaccount = viewbaner["cardid"].get_utf8().value.to_string();
                    dbaccountname = viewbaner["cardname"].get_utf8().value.to_string();
                    dbaccounttype = viewbaner["cardtype"].get_utf8().value.to_string();
                    dbpaymentagreement = viewbaner["qrcode"].get_utf8().value.to_string();

                    auto viewagent = agent_result->view();
                    beishu = viewagent["washtotal"].get_double();   //提款所需打码量的倍数
                    timesperday = viewagent["freeWithdrawTime"].get_int32();  //免费取款次数
                    dbadministrativefee = viewagent["withdrawManageFee"].get_int32();  //行政费

                    minStrageScore= viewagent["usdtMinWithdrawMoney"].get_int32();   //最小提款金额
                    maxStrageScore= viewagent["usdtMaxWithdrawMoney"].get_int32();   //最大提款金额
                    dbmandatoryfee = viewagent["usdtCompulsoryWithdrawalFee"].get_double();    //洗码量不够收取的费用
                    dbfrequentfee =viewagent["usdtWithdrawFee"].get_double();    //超次数收取的费用


                    LOG_WARN << "ustd ----trage dbaccounttype" << dbaccounttype << "    dbaccount" << dbaccount;
                }

                //

                if (extrangescore > maxStrageScore * COIN_RATE || extrangescore < minStrageScore * COIN_RATE)
                {
                    retCode = 5;
                    LOG_WARN << "---" << __FUNCTION__ << " ---chao chu zui da di yu zui xiao ";
                    break;
                }

                LOG_WARN << "---5";
                string strageDate = "strage" + to_string(userId);
                redis::RedisValue values;
                string fields[2] = {"date", "times"};
                int64_t datetoInt = 0;
                bool ret = REDISCLIENT.hmget(strageDate,fields,2,values);

                if(ret)
                {
                    datetoInt = values["date"].asInt();
                    dbcurrtragetimes = values["times"].asInt();
                    LOG_WARN << "---读取redis兑换次数成功: " <<dbcurrtragetimes <<"  date: "<<datetoInt;
                }
                else
                {
                    datetoInt = 0 ;
                    dbcurrtragetimes = 0;
                    LOG_WARN << "---读取redis兑换次数失败: " <<dbcurrtragetimes ;
                }
                time_t shijianlong;
                int64_t redisTime=0;
                {
                    time_t tx = time(NULL); //获取目前秒时间
                    struct tm * local;
                    local = localtime(&tx); //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
                    struct tm t;
                    t.tm_year = local->tm_year ;
                    t.tm_mon = local->tm_mon;
                    t.tm_mday = local->tm_mday;
                    t.tm_hour = 0;
                    t.tm_min = 0;
                    t.tm_sec = 0;
                    shijianlong= mktime(&t);

                }
                redisTime =(int64_t)shijianlong;
                if(datetoInt!=redisTime)
                {
                    datetoInt =(int64_t)shijianlong ;
//                    dbcurrtragetimes = 1;
                    //redis放到后面写入，玩意中途有失败情况，可以不计数，不用回退
//                    redis::RedisValue redisValue;
//                    redisValue["date"] = datetoInt;
//                    redisValue["times"] = dbcurrtragetimes;
//                    REDISCLIENT.hmset(strageDate, redisValue);
                    LOG_WARN << "---对比时间不一样redis存储时间是: " << datetoInt <<"  后去本地时间是: "<<shijianlong;
                }
                else
                {
                    datetoInt =(int64_t)shijianlong ;
//                    dbcurrtragetimes+=1;
//                    redis::RedisValue redisValue;
//                    redisValue["times"] = dbcurrtragetimes;
//                    REDISCLIENT.hmset(strageDate, redisValue);
                    LOG_WARN << "---时间一样时间累加1并且存储 ";
                }


                LOG_WARN << "---" << __FUNCTION__ << " ---[" << extrangescore << "],safeboxScore[" << safeboxScore << "]";


                if (extrangescore > safeboxScore)
                {
                    retCode = 1;
                    LOG_WARN << "---" << __FUNCTION__ << " 分数不足够";
                    break;
                }

                // 更新保险箱值  //  document{} << "$inc" << open_document << "rebate" << -rebate << close_document << finalize,options);
                mongocxx::options::find_one_and_update options;
                options.return_document(mongocxx::options::return_document::k_after);


                int64_t addgrandbet=0;
                bool isNeedFee = false;
                int32_t percentTrage = 100; //假如一块钱需要一百块洗码量,需要读取配置

                int64_t currentBetting = 0;
                //假如累计洗码量大于兑换免手续费的洗码量,不需要手续费,多出不分还可以留下次用
                int64_t needbetting = 0;


                // dblastneedbet 上次所需打码量  dbgrandtotalbet  记录上次的打码量  dbtotalbet 历史总打码量 dbbasicwashbet 所需打码量


                if(dbtotalbet-dbgrandtotalbet>=dbbasicwashbet-dblastneedbet)
                {
//                   addgrandbet=dbtotalbet-dbgrandtotalbet;
//                   currentBetting = dbtotalbet-dbgrandtotalbet; // 本次兑换实现的洗码量
//                   needbetting =  dbbasicwashbet;  // 所需洗码量为零
//                   isNeedFee =false;
                     if(extrangescore*beishu>dbbasicwashbet-dblastneedbet)
                     {
                         dblastneedbet = dbbasicwashbet-dblastneedbet;
                         addgrandbet =  dblastneedbet;
                         needbetting = dbbasicwashbet-dblastneedbet;
                     }
                     else
                     {
                         dblastneedbet =  extrangescore*beishu;
                         addgrandbet   =  dblastneedbet;
                         needbetting = dbbasicwashbet-dblastneedbet;
                     }
                     currentBetting = dbtotalbet-dbgrandtotalbet;// 本次兑换实现的洗码量
                     isNeedFee = false ;
                }
                else
                {
//                    addgrandbet = dbtotalbet-dbgrandtotalbet;
//                    needbetting = extrangescore*beishu; //所需洗码量减去提取不分的所需洗码量
//                    currentBetting = dbtotalbet-dbgrandtotalbet;// 本次兑换实现的洗码量
//                    isNeedFee = true ;
                    if(extrangescore*beishu>dbbasicwashbet-dblastneedbet)
                    {

                        dblastneedbet = dbbasicwashbet-dblastneedbet;
                        addgrandbet =  dblastneedbet;
                        needbetting = dbbasicwashbet-dblastneedbet;
                    }
                    else
                    {
                        dblastneedbet =  extrangescore*beishu;
                        addgrandbet   =  dblastneedbet;
                        needbetting = dbbasicwashbet-dblastneedbet;
                    }
                    currentBetting = dbtotalbet-dbgrandtotalbet;// 本次兑换实现的洗码量
                    isNeedFee = true ;
                }
                LOG_INFO << "---插入前并无异常" ;
                 auto updateView = user_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                            document{} << "$inc" << open_document
                                                                       << "safebox" << -extrangescore
                                                                       << "grandtotalbet"<< addgrandbet                                                                       
                                                                       << "subscoretimes"<< 1 //历史次数累加一
                                                                       << "lastneedbet"<< addgrandbet
                                                                       << close_document
                                                                       << finalize,options);

                //document{} << "$inc" << open_document << "rebate" << -rebate << close_document << finalize,options);
                if (!updateView)
                {
                    LOG_INFO << "---" << __FUNCTION__ << " 更新保险箱值失败";
                    retCode = 7;
                    break;
                }
                LOG_INFO << "---插入前并无异常" ;
                // 取更新后的值
                auto viewfind = updateView->view();
				safeboxScore = viewfind["safebox"].get_int64();  // 提取后值

                // 生成订单
                 string orderid = GlobalFunc::getBillNo(userId);
                //here need to update redis value?
                // std::time_t nowTime = std::time(NULL);
                // int64_t nowtime_num = (int64_t)nowTime;


                 int32_t administrativefee =0;  //行政费计算值
                 int32_t mandatoryfee = 0;      //洗码量不够强制收费
                 int32_t frequentfee = 0;       //超次数收费
                 //假如累计洗码量大于兑换免手续费的洗码量
                 if(!isNeedFee)
                 {
                    dbmandatoryfee = 0;
                    if(dbcurrtragetimes>=timesperday)
                    {
                        administrativefee = dbadministrativefee*COIN_RATE;
                        frequentfee = extrangescore*dbfrequentfee/COIN_RATE;

                        LOG_WARN << "---当前次数: "<<dbcurrtragetimes <<"  免费次数:"<<timesperday;
                        LOG_WARN << "---dbadministrativefee: "<<dbadministrativefee <<"  dbfrequentfee:"<<dbfrequentfee;
                        LOG_WARN << "---收取行政费: "<<administrativefee <<"  收取百分比费:"<<timesperday;
                    }
                    else
                    {
                        administrativefee = 0;
                        frequentfee = 0;
                        LOG_WARN << "没超次数: "<<dbcurrtragetimes;
                    }
                 }
                 else//要手续费
                 {
                    mandatoryfee = extrangescore*dbmandatoryfee/COIN_RATE;
                    administrativefee = dbadministrativefee*COIN_RATE;
                    frequentfee = extrangescore*dbfrequentfee/COIN_RATE;
                 }
                bsoncxx::builder::stream::document builder{};
                auto insert_value = builder
                                    << "userid" << userId
                                    << "account" << account
                                    << "ordernum"<< orderid
                                    << "withdrawalscore" << extrangescore
                                    << "dispensingscore" << 0
                                    << "orderstatus" << 0
                                    << "ordertype" << (int32_t)opType
                                    << "orderinserttime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                    << "agentid" << (int32_t)agentid
                                    << "address" << dbaddress
                                    << "accountnumber" << dbaccount
                                    << "accountname" << dbaccountname
                                    << "accounttype" << dbaccounttype
                                    << "issuestatus" << (int32_t)0
                                    << "paymentagreement" << dbpaymentagreement
                                    << "completebet" << currentBetting
                                    << "needbet" << needbetting
                                    << "administrativefee" <<administrativefee
                                    << "mandatoryfee" <<mandatoryfee
                                    << "frequentfee" <<frequentfee
                                    << "historicaltimes" << (int32_t)dbtragehistimes    //历史次数
                                    << "daytimes"<< (int32_t)(dbcurrtragetimes+1 )          //当日次数
                                    << "exceedtimes"<< (int32_t)(dbcurrtragetimes>=timesperday?1:0)          //是否超次数
                                    << "exceedbetting"<<(int32_t)(isNeedFee?0:1)       //是否到达需求打码量
                                    << bsoncxx::builder::stream::finalize;


                if (!coll.insert_one(insert_value.view()))
                {
                    retCode = 8; // score not enough
                    LOG_ERROR << "---" << __FUNCTION__ << " extrange_order insert exception: ";
                    break;
                }

                retCode = 0;

                response.set_cursafeboxscore(safeboxScore);
                response.set_curscore(curscore);
                
                redis::RedisValue redisValue;
                redisValue["date"] = datetoInt;
                redisValue["times"] = dbcurrtragetimes;
                REDISCLIENT.hmset(strageDate, redisValue);
                LOG_WARN << "---" << __FUNCTION__ << " 兑换完成，兑换后[" << safeboxScore << "],curscore[" << curscore <<"],orderid[" << orderid <<"]";

            } while (0);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
            retCode = 9;
        }

        // 结果码
        response.set_retcode(retCode);
        response.set_optype(opType);
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

/*
type	    int	    账户类型（建索引,1=银行卡,0=支付宝,2=微信,3=USDT ）
cardid	    string	银行卡帐号/支付宝帐户/微信帐户/USDT唯一标识
cardname	string	银行卡帐户/支付宝收款人姓名/微信收款人姓名/虚拟币名称
cardtype	string	银行名称/空/空/虚拟币协议
cardaddr	string	支行地址/空/空/虚拟币二维码
*/
// 绑定银行
void HallServer::cmd_band_bank(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_BAND_BANK_RES;

    // request
    ::HallServer::BandBankMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::BandBankResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t nOperateType    = msg.optype();     // 操作类型，0-支付宝 1 银行卡 2-微信 3-其它
        string cardtype         = msg.cardtype();   // 银行名称/空/空/虚拟币协议
        string cardname         = msg.cardname();   // 银行卡帐户/支付宝收款人姓名/微信收款人姓名/虚拟币名称
        string cardid           = msg.cardid();     // 银行卡帐号/支付宝帐户/微信帐户/USDT唯一标识
        string cardaddr         = msg.cardaddr();   // 支行地址/空/空/虚拟币二维码
        int32_t retCode         = 0;                // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " nOperateType[" << nOperateType << "],userId[" << userId << "],cardname[" << cardname << "],cardid[" << cardid << "],cardaddr[" << cardaddr << "],cardtype[" << cardtype << "]";

        try
        {
            do
            {
                // 值合法(0-支付宝 1 银行卡 2-微信 3-其它)
                if (nOperateType < ePlaymentType::op_pay_ali || nOperateType > ePlaymentType::op_pay_usdt)
                {
                    retCode = 4;
                    break;
                }

                string mobile;
                int32_t agentid = 0;
                // 查数据库
                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(bsoncxx::builder::stream::document{} << "mobile" << 1 << "agentid" << 1 << bsoncxx::builder::stream::finalize);

                auto maybe_result = coll.find_one(document{} << "userid" << userId << finalize, opts);
                if (!maybe_result)
                {
                    retCode = 1;
                    break;
                }

                auto view = maybe_result->view();
                agentid = view["agentid"].get_int32();
                mobile = view["mobile"].get_utf8().value.to_string();
                if (agentid == 0)
                {
                    retCode = 2;
                    break;
                }

                string cardcode = "";
                if (nOperateType == ePlaymentType::op_pay_bank)
                {
                    auto it = m_bankNameMap.find(agentid);
                    if (it != m_bankNameMap.end())
                    {
                        for (auto bankinfo : it->second)
                        {
                            // LOG_INFO << " >>> bankinfo: " << bankinfo.begin()->first << ", " << bankinfo.begin()->second;
                            if (cardtype.compare(bankinfo.begin()->first) == 0)
                                cardcode = bankinfo.begin()->second; //bankcode
                        }
                    }
                }

                // 判断卡号的合法性
                if(nOperateType == ePlaymentType::op_pay_bank)
                {
                    if(!checkCard(cardid)){
                        retCode = 6;
                        LOG_ERROR << " >>>用户[" << userId << "]银行卡号[" << cardid << "]不合规!";
                        break;
                    }
                }

                bool isHad = false;
                mongocxx::collection bankcoll = MONGODBCLIENT["gamemain"]["user_bank_info"];
                // 防止重复添加
                if ((nOperateType == ePlaymentType::op_pay_ali)||(nOperateType == ePlaymentType::op_pay_bank))
                {
                    auto query_value = document{} << "agentid" << agentid << "userid" << userId << "type" << nOperateType << "cardid" << cardid << "cardname" << cardname << finalize;
                    if (bankcoll.find_one(query_value.view()))
                    {
                        isHad = true;
                    }
                }
                else if (nOperateType == ePlaymentType::op_pay_wx)
                {
                    auto query_value = document{} << "agentid" << agentid << "userid" << userId << "type" << nOperateType << "cardid" << cardid << "cardname" << cardname << finalize;
                    if (bankcoll.find_one(query_value.view()))
                    {
                        isHad = true;
                    }
                }
                else if (nOperateType == ePlaymentType::op_pay_usdt)
                {
                    auto query_value = document{} << "agentid" << agentid << "userid" << userId << "type" << nOperateType << "cardid" << cardid << "cardaddr" << cardaddr << "cardname" << cardname << finalize;
                    if (bankcoll.find_one(query_value.view()))
                    {
                        isHad = true;
                    }
                }
                
                if (isHad)
                {
                    retCode = 5;
                    LOG_ERROR << "---帐号已经存在，不能重复添加,[" << userId << "]";
                    break;
                }

                auto query_value = document{} << "userid" << userId << "type" << nOperateType << "cardid" << cardid << finalize;
                auto seq_updateValue = document{} << "$set" << open_document << "userid" << userId << "mobile" << mobile
                                                  << "type" << nOperateType << "agentid" << agentid << "cardtype" << cardtype << "cardname" << cardname << "qrcode"
                                                  << ""
                                                  << "cardid" << cardid << "cardaddr" << cardaddr << "cardcode" << cardcode << "firstchoice" << 0 << close_document << finalize;
                //update options
                mongocxx::options::update options = mongocxx::options::update{};
                if (bankcoll.update_one(query_value.view(), seq_updateValue.view(), options.upsert(true)))
                {
                    retCode = 0;
                    response.set_cardid(cardid);
                    response.set_cardname(cardname);
                    response.set_cardaddr(cardaddr);
                    response.set_cardtype(cardtype);
                    response.set_optype(nOperateType);
                }

            } while (0);
        }
        catch (exception &e)
        {
            retCode = 3;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 昵称设置
void HallServer::cmd_set_nickName(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_RES;

    // request
    ::HallServer::SetNickNameMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::SetNickNameResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        string nickname          = msg.nickname();    	// 昵称
        int32_t retCode         = 1;                    // 结果码
       
        LOG_INFO << "---" << __FUNCTION__ << " nickname昵称[" << nickname << "],userId[" << userId <<"]";
        try
        {
            do
            {
                int32_t renamecount = 0;

                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(bsoncxx::builder::stream::document{} << "renamecount" << 1 << bsoncxx::builder::stream::finalize);

                auto findValue = document{} << "userid" << userId << finalize;

                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                auto find_doc = coll.find_one(findValue.view(), opts);
                if (find_doc)
                {
                    auto view = find_doc->view();
                    renamecount = view["renamecount"].get_int32();
                    if (renamecount > 0)
                    {
                        retCode = 3; //已经修改过，不能再修改
                        break;
                    }
                }

                LOG_INFO << "---renamecount[" << renamecount <<"],userId[" << userId <<"]";

                auto seq_updateValue = document{} << "$set" << open_document << "nickname" << nickname
                                                  << "renamecount" << 1 << close_document << finalize;

                // 更新操作
                if (coll.update_one(findValue.view(), seq_updateValue.view()))
                {
                    retCode = 0;
                }

            } while (0);
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_nickname(nickname);
        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 设置玩家保底佣金
void HallServer::cmd_set_min_rebate(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_MIN_REBATE_RES;

    // request
    ::HallServer::SetMinRebateMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::SetMinRebateResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int64_t superiorUserid  = msg.userid();    	// 下级代理值
        int32_t minrebate       = msg.minrebate();  // 保底佣金
        int32_t retCode         = 1;                // 结果码
        int32_t oldminrebate    = 0;                // 旧保底佣金
       
        LOG_INFO << "---" << __FUNCTION__ << " minrebate保底佣金[" << minrebate << "],userId[" << userId <<"],下级代理值[" << superiorUserid <<"]";
        try
        {
            do
            {

                if (minrebate < 0)
                {
                    retCode = 2;
                    break;
                }

                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(bsoncxx::builder::stream::document{} << "minrebaterate" << 1 << "userid" << 1 << bsoncxx::builder::stream::finalize);
                auto user_result = coll.find_one(document{} << "userid" << superiorUserid << finalize, opts);
                if (user_result)
                {
                    auto findview = user_result->view();
                    if (findview["minrebaterate"])
                    {
                        oldminrebate = findview["minrebaterate"].get_int32();
                    }
                }

                bsoncxx::document::value seq_updateValue = document{} << "$set" << open_document << "minrebaterate" << minrebate << close_document << finalize;
                // 更新操作
                if (coll.update_one(document{} << "userid" << superiorUserid << finalize, seq_updateValue.view()))
                {
                    retCode = 0;
                    response.set_newminrebate(minrebate);
                    response.set_oldminrebate(oldminrebate);
                }

                /* code */
            } while (0);
        }
        catch (exception &e)
        {
            retCode = 3;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 提取佣金(锁分操作)
void HallServer::cmd_get_rebate(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_REBATE_RES;

    // request
    ::HallServer::GetRebateMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRebateResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int64_t rebate          = msg.rebate();// ;    	// 佣金(分数)
        int32_t retCode         = 0;                            // 结果码
        int32_t brokerage = 0;
        int32_t type  = msg.type();
        if(type==0)
        {
            LOG_INFO << "---" << __FUNCTION__ << " rebate佣金[" << rebate << "],userId[" << userId << "]";

            
            string linecode,account;
            int32_t agentid   = 0;
            int64_t oldscore,newscore,oldrebate,newrebate = 0;

            do
            {
                // 减少无谓的DB操作
                if (rebate <= 0)
                {
                    retCode = 1;
                    break;
                }

                try
                {
                    mongocxx::options::find opts = mongocxx::options::find{};
                    opts.projection(bsoncxx::builder::stream::document{} << "rebate" << 1 << "score" << 1 << "agentid" << 1 << "linecode" << 1 << "minrebaterate" << 1
                                                                         << "account" << 1 << bsoncxx::builder::stream::finalize);
                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                    auto user_result = user_coll.find_one(document{} << "userid" << userId << finalize, opts);
                    if (user_result)
                    {
                        auto view = user_result->view();
                        oldscore = view["score"].get_int64();
                        oldrebate = view["rebate"].get_int64();
                        agentid = view["agentid"].get_int32();
                        account = view["account"].get_utf8().value.to_string();
                        linecode = view["linecode"].get_utf8().value.to_string();
                        brokerage = view["minrebaterate"].get_int32();

                        if (oldrebate == 0)
                        {
                            retCode = 2;
                            break;
                        }

                        // 防止错误输入过大
                        if (rebate > oldrebate)
                            rebate = oldrebate;
                        // 防止
                        // newrebate = oldrebate - rebate;

                        // 设置需要返回修改之后的数据
                        mongocxx::options::find_one_and_update options;
                        options.return_document(mongocxx::options::return_document::k_after);

                        auto updateView = user_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                                        document{} << "$inc" << open_document << "rebate" << -rebate << close_document << finalize, options);
                        if (updateView) //更新佣金值
                        {
                            auto viewfind = updateView->view();
                            newrebate = viewfind["rebate"].get_int64(); // 提取后值
                        }

                        // 生成订单
                        string orderid = GlobalFunc::getBillNo(userId);//to_string(userId) + to_string(GlobalFunc::GetCurrentStamp64(true));
                        // 1,增加提取交易记录
                        mongocxx::collection rebate_coll = MONGODBCLIENT["gamemain"]["rebate_pick_info"];
                        auto insert_value = bsoncxx::builder::stream::document{}
                                            << "userid" << b_int64{userId}
                                            << "account" <<  account
                                            << "agentid" <<  b_int32{agentid}
                                            << "oldrebate" << b_int64{oldrebate}
                                            << "newrebate" << b_int64{newrebate}
                                            << "pickrebate" << b_int64{rebate}
                                            << "stauts" << 0
                                            << "picktime" << b_date{chrono::system_clock::now()}
                                            << "orderid" << orderid
                                            << "operatetype" << 0 //交易方式
                                            << "audituser" << "defualt"
                                            << "audittime" << b_date{chrono::system_clock::now()}
                                            << bsoncxx::builder::stream::finalize;
                        if(!rebate_coll.insert_one(insert_value.view())){
                            LOG_ERROR << "---增加提取交易记录插入失败,userId[" << userId << "],oldrebate[" << oldrebate << "],orderid[" << orderid << "]";
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    retCode = 3;
                    LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
                }

            } while (0);

            response.set_brokerage(brokerage);
            response.set_newrebate(newrebate);
            response.set_oldrebate(oldrebate);
            response.set_curscore(oldscore);

            response.set_retcode(retCode);        //结果码
            response.set_type(type);
            size_t len = response.ByteSize();
            vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
            if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
        else//请求更新值
        {
            string linecode,account;
            int32_t agentid   = 0;
            int64_t oldscore,newscore,oldrebate,newrebate = 0;
            int32_t brokerage =0;
            try
            {
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(bsoncxx::builder::stream::document{} << "rebate" << 1 << "score" << 1 << "agentid" << 1 << "linecode" << 1  << "minrebaterate" << 1
                                                                     << "account" << 1 << bsoncxx::builder::stream::finalize);
                mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                auto user_result = user_coll.find_one(document{} << "userid" << userId << finalize, opts);
                if (user_result)
                {
                    auto view = user_result->view();
                    oldscore = view["score"].get_int64();
                    oldrebate = view["rebate"].get_int64();
                    agentid = view["agentid"].get_int32();
                    account = view["account"].get_utf8().value.to_string();
                    linecode = view["linecode"].get_utf8().value.to_string();
                    brokerage = view["minrebaterate"].get_int32();
                }
            }
            catch(exception &e)
            {
                retCode= 3;
            }
            response.set_brokerage(brokerage);
            response.set_retcode(retCode);        //结果码
            response.set_newrebate(oldrebate);
            response.set_oldrebate(oldrebate);
            response.set_curscore(oldscore);
            response.set_type(type);
            size_t len = response.ByteSize();
            vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
            if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }
}

// 获取银行列表信息
void HallServer::cmd_get_banklist(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_BANK_INFO_RES;

    // request
    ::HallServer::GetBankInfoMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetBankInfoResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 0;                   // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";
        try
        {
            // 找到缓存信息
            auto it = m_bankNameMap.find(agentid);
            if (it != m_bankNameMap.end()) 
            {
                for (auto  bankinfo : it->second) 
                {
                    response.add_bankname(bankinfo.begin()->first);//bankname
                    // LOG_ERROR << " >>> 0 bankinfo: " << bankinfo.begin()->first << ", " << bankinfo.begin()->second;
                }
            }
            else
            {
                vector<map<string,string>> bankinfo;
                mongocxx::collection bankdeploy_coll = MONGODBCLIENT["gamemain"]["bankdeploy"];
                mongocxx::cursor info = bankdeploy_coll.find({}); //document{} << "agentid" << agentid << finalize
                for (auto &doc : info)
                {
                    string bankname = doc["bankname"].get_utf8().value.to_string();
                    string bankcode = doc["bankcode"].get_utf8().value.to_string();
                   
                    map<string,string>  bankitemTmp;
                    bankitemTmp[bankname] = bankcode;

                    bankinfo.push_back(bankitemTmp);
                    response.add_bankname(bankname);
                    // LOG_INFO << "---bankname[" << bankname << "],bankcode[" << bankcode << "],size[" << bankinfo.size() << "]";
                }
                // 缓存到内存
                m_bankNameMap[agentid] = bankinfo;
            }
        }
        catch (exception &e)
        {
            retCode = 1;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
void HallServer::cmd_get_mails(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MAILS_RES;

    // request
    ::HallServer::GetMailsMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetMailsMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        //  int32_t agentid = msg.agentid(); // 代理ID
        int32_t retCode = 1;             // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " userId[" << userId << "],userId[" << userId << "]";
        try
        {
            response.set_userid(userId);
           
            mongocxx::collection q_coll = MONGODBCLIENT["gamemain"]["gameuser_mail"];
            mongocxx::cursor info = q_coll.find(document{} << "userid" << userId << finalize);
            for (auto &doc : info)
            {
                //status 状态 0-未读 1-已读  邮件状态 -1 草稿，0 未读，1 已读
                int32_t status  = doc["status"].get_int32();                    
                if(status >= 0){
                    string content  = doc["content"].get_utf8().value.to_string();  //内容
                    //string sendtime = "2020-10-14T20:26:51";//doc["sendtime"].get_date().value.to_string(); //发送日期
                    int64_t timeseconds = doc["sendtime"].get_date()/1000;
                    string sendtime = InitialConversion(timeseconds);

                    string title    = doc["title"].get_utf8().value.to_string();    //标题
                    int64_t timestamp    = doc["timestamp"].get_int64();            //时间
                    ::HallServer::Mail *mailsitem = response.add_mails();
                    mailsitem->set_status(status);
                    mailsitem->set_content(content);
                    mailsitem->set_sendtime(sendtime);
                    mailsitem->set_title(title);
                    mailsitem->set_index(timestamp);

                    int64_t shijian = doc["sendtime"].get_date();


                }
            }

            retCode = 0;
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << __FUNCTION__ << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);

        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
void HallServer::cmd_get_bulletin(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_BULLETIN_RES;

    // request
    ::HallServer::GetBullettinMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetBullettinMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
         int32_t agentid = msg.agentid(); // 代理ID
        int32_t retCode = 1;             // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " userId[" << userId << "],userId[" << userId << "]";
        try
        {
             string picPath = "http://192.168.2.214:9080";
            if (cachePicPathIP(agentid,picPath) != 0)
            {
                LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agentid << "]";
            }

            // 
            response.set_userid(userId);
           
            mongocxx::collection q_coll = MONGODBCLIENT["gamemain"]["platform_notice"];
            mongocxx::cursor info = q_coll.find(document{} << "agentid" << agentid << finalize);
            for (auto &doc : info)
            {
                int32_t status  = doc["status"].get_int32();                    //状态 0-启用 1-草稿
                if (status == eDefaultStatus::op_ON)
                {
                    int32_t type = doc["type"].get_int32();                       //0 为文字公告,1 图片公告
                    string content = doc["content"].get_utf8().value.to_string(); //内容
                    int64_t timeseconds = doc["sendtime"].get_date() / 1000;      //发送日期
                    string sendtime = InitialConversion(timeseconds);

                    // 图片公告增加地址
                    if(type == 1){
                        content = picPath + content;
                    }

                    string title = doc["title"].get_utf8().value.to_string(); //标题
                    ::HallServer::Bullettin *bulletinsitem = response.add_bulletins();
                    bulletinsitem->set_status(status);
                    bulletinsitem->set_content(content);
                    bulletinsitem->set_sendtime(sendtime);
                    bulletinsitem->set_title(title);
                    bulletinsitem->set_type(type);
                }
            }

            retCode = 0;
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << __FUNCTION__ << " >>> Exception: " << e.what();
        }

        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
void HallServer::cmd_set_bulletinmails_status(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_BULLETINMAIL_RES;

    // request
    ::HallServer::SetBullettinMailMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::SetBullettinMailResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId      = internal_header->userId;  // 玩家ID
        // int32_t typeid_     = msg.type();               // 0 公告 1 邮件
        int64_t index       = msg.index();              // 公告邮件下标 按照发送时的下标取值
        int64_t trangestatus = msg.trangestatus();      // 修改状态 0 标记为已读 1 删除
        int32_t retCode = 1;                            // 结果码

        // 邮件状态 -1 草稿，0 未读，1 已读
        LOG_INFO << "---" << __FUNCTION__ << " userId[" << userId << "],index[" << index << "],trangestatus[" << trangestatus << "]";
        try
        {
            retCode = 0;
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["gameuser_mail"];
            if (trangestatus == 0)
            {
                auto updateValue = document{} << "$set" << open_document << "status" << 1 << "viewtime" << b_date{chrono::system_clock::now()} << close_document << finalize;
                // 更新操作
                if (!coll.find_one_and_update(document{} << "userid" << userId << "timestamp" << index << finalize, updateValue.view()))
                {
                    retCode = 1;
                }
            }
            else if (trangestatus == 1)
            {
                auto deleteview = make_document(kvp("userid", userId),kvp("status", 1)); //kvp("timestamp", index)
                // LOG_DEBUG << "delete_one Document:"<<bsoncxx::to_json(deleteview);

                auto result = coll.delete_one(deleteview.view());
                if (!result)//result->deleted_count() != 1);//document{} << "userid" << userId << "index" << index << finalize))
                {
                    retCode = 1;
                }
                LOG_INFO << __FUNCTION__<< ">>> deleted_count[" << result->deleted_count() <<"],retCode[" << retCode << "]";
            }
           
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << __FUNCTION__ << " >>> Exception: " << e.what();
        }

        // 返回修改状态
        response.set_index(index);
        response.set_status(trangestatus);
        response.set_retcode(retCode);
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}


// 加载PicPath值配置,图片连接
int32_t HallServer::cachePicPathIP(int32_t agentid,string & picPath)
{
	try
	{
		auto itor = m_PicthPathMap.find(agentid);
		if (itor != m_PicthPathMap.end())
		{
			picPath = itor->second;
			return 0;
		}

		string details;
		if (get_platform_global_config(agentid, (int32_t)eAgentGlobalCfgType::op_cfg_file, details))
		{
			boost::property_tree::ptree pt;
			std::stringstream ss(details);
			read_json(ss, pt);
			picPath = pt.get<string>("fullpath");
			LOG_INFO << "---" << __FUNCTION__ << "---图片连接,agentid[" << agentid << "],picPath[" << picPath << "]";
		}
		else
			LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agentid << "]";

		m_PicthPathMap[agentid] = picPath;

		return 0;
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 加载PicPath值配置失败----" << e.what();
		return 5;
	}
}

// 
bool HallServer::cacheActiveItem(int32_t agentid)
{
    // 找到缓存信息
    if (m_ActiveItemMap.find(agentid) != m_ActiveItemMap.end())
        return true;

    try
    {
        // get pic path form map 
        string picPath = "http://192.168.2.214:9080";
        if (cachePicPathIP(agentid, picPath) != 0)
            LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理配置的图片路径信息[" << agentid << "]";

        vector<ActiveItem> activeIteminfo;
        mongocxx::collection activity_coll = MONGODBCLIENT["gamemain"]["platform_activity"];
        mongocxx::cursor info = activity_coll.find(document{} << "agentid" << agentid << finalize);
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
            if (status == eDefaultStatus::op_ON)
            {
                int32_t sortid = doc["sort"].get_int32();
                int32_t ntype = doc["type"].get_int32();
                string title = doc["title"].get_utf8().value.to_string();
                string activityurl = picPath + doc["activityurl"].get_utf8().value.to_string();

                ActiveItem ActiveItem_ = {0};
                ActiveItem_.activityurl = activityurl;
                ActiveItem_.title = title;
                ActiveItem_.sortid = sortid;
                ActiveItem_.status = status;
                ActiveItem_.type = ntype;

                activeIteminfo.push_back(ActiveItem_);
                LOG_INFO << "---ActiveItem[" << title << "],size[" << activeIteminfo.size() << "],url[" << activityurl << "]";
            }
        }

        // 缓存到内存
        if (activeIteminfo.size() > 0)
            m_ActiveItemMap[agentid] = activeIteminfo;
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}

bool HallServer::checkCard(string card)
{
    int length = card.length();

    if (!(length == 16 || length == 19))
    {
        return false;
    }

    int s = 0;
    int num = 0;
    for (int i = 0; i < length; i++)
    {
        char buff = card[i];
        num = atoi(&buff);

        if ((i + 1) % 2 == 0)
        {
            if ((num * 2) >= 10)
            {
                char buffer[3] = {0};
                // itoa(num * 2, buffer, 10);

                string tmpBuf = to_string(num * 2);
                if (tmpBuf.length() >= 2)
                {
                    buffer[0] = tmpBuf[0];
                    buffer[1] = tmpBuf[1];
                }
                // cout << "itoa == [" << tmpBuf <<"],len["<< tmpBuf.length() << "],[" <<  buffer[0] << " " << buffer[1] <<"]" << endl;

                s += (int(buffer[0]) - 48) + (int(buffer[1]) - 48);
            }
            else
                s += num * 2;
        }
        else
            s += num;
    }

    return (s % 10 == 0);
}

// 
bool HallServer::cacheRegPoorItem(int32_t agentid)
{
    // 找到缓存信息
    if (m_RegPoorMap.find(agentid) != m_RegPoorMap.end())
        return true;

    try
    { 
        mongocxx::collection activity_coll = MONGODBCLIENT["gamemain"]["platform_activity_register"];
        auto find_ret = activity_coll.find_one(document{} << "agentid" << agentid << finalize);
        if(!find_ret)
        {
            LOG_ERROR << "---" << __FUNCTION__ << " 没有配置注册送金活动";
            return false;
        }

        auto findView = find_ret->view();

        int64_t rgtReward = findView["rgtReward"].get_int64();
        int64_t rgtIntegral = findView["rgtIntegral"].get_int64();
        int64_t minCoin = findView["minCoin"].get_int64();
        int32_t handselNo = findView["handselNo"].get_int32();
        int64_t reqReward = findView["reqReward"].get_int64();

        RegPoorItem ActiveItem_ = {0};
        ActiveItem_.rgtReward = rgtReward / COIN_RATE;
        ActiveItem_.rgtIntegral = rgtIntegral; //转化之后 的积分值，使用时需要除500 再缩小100倍
        ActiveItem_.handselNo = handselNo;
        ActiveItem_.reqReward = reqReward / COIN_RATE;
        ActiveItem_.minCoin = minCoin / COIN_RATE;

        // 缓存到内存
        m_RegPoorMap[agentid] = ActiveItem_;
        LOG_INFO << "---RegPoorItem[" << agentid << "],rgtReward[" << rgtReward << "],rgtIntegral[" << rgtIntegral << "],handselNo[" << handselNo << "],reqReward[" << reqReward << "]";
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}

// 获取代理活动列表
void HallServer::cmd_get_activeItem_list(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ACTIVE_RES;

    // request
    ::HallServer::GetActiveMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetActiveResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 0;                   // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";

        do
        {
            // 读缓存
            if (!cacheActiveItem(agentid))
            {
                retCode = 1;
                break;
            }

            // 找到缓存信息
            auto it = m_ActiveItemMap.find(agentid);
            if (it != m_ActiveItemMap.end())
            {
                for (auto activeItem : it->second)
                {
                    ::HallServer::ActiveItemList *ActiveItemList_ = response.add_item();
                    ActiveItemList_->set_activityurl(activeItem.activityurl);
                    ActiveItemList_->set_sortid(activeItem.sortid);
                    ActiveItemList_->set_status(activeItem.status);
                    ActiveItemList_->set_title(activeItem.title);
                }
                LOG_INFO << "---Active Item agentid[" << agentid << "],Item size[" << it->second.size() << "]";
            }

        } while (0);

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 缓存签到信息
int32_t HallServer::cacheSignInInfo(int32_t agentid)
{
    int32_t retcode = 0;
    try
    {
        vector<DaliySignItem> signinIteminfo;
        mongocxx::collection signin_coll = MONGODBCLIENT["gamemain"]["attendance"];
        mongocxx::cursor info = signin_coll.find(document{} << "agentid" << agentid << finalize);
        for (auto &doc : info)
        {
            int32_t giftType = doc["giftType"].get_int32();
            int32_t sortid = doc["sort"].get_int32();
            int32_t giftValue = doc["giftValue"].get_int32();
            int32_t minbet = doc["minbet"].get_int32();
            string title = doc["title"].get_utf8().value.to_string();

            DaliySignItem signItem_ = {0};
            signItem_.giftType = giftType;
            signItem_.sortid = sortid;
            signItem_.giftValue = giftValue;
            signItem_.minBet = minbet;
            signItem_.title = title;

            signinIteminfo.push_back(signItem_);
            LOG_INFO << "---SignInItem giftType[" << giftType << "],giftValue[" << giftValue << "],minbet[" << minbet << "],title[" << title << "],size[" << signinIteminfo.size() << "]";
        }
        // 缓存到内存
        m_SignInItemMap[agentid] = signinIteminfo;
        /* code */
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        retcode = 2;
        LOG_ERROR << "---SignInItem agentid[" << agentid << "],e.what()[" << e.what() << "]";
    }

    return retcode;
}
// 获取每日签到列表 Get Sign In Info
void HallServer::cmd_get_signin_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOAD_SIGN_IN_RES;

    // request
    ::HallServer::GetSignInMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetSignInResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 0;                   // 结果码
        int32_t issignin = 1,lastday,today,lastmon,curmon,seriesday = 0;
        int64_t lastvalidbet  = 0; //上次流水值

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";
        try
        {
            do
            {
                // 北京时间
                time_t curstamp = time(nullptr);  // UTC秒数
                tm *tp = localtime(&curstamp);   
                int64_t curTimeStamp = getZeroTimeStamp(tp);

                seriesday = 0; //连续签到天数0
                issignin = 2;//1-已签到 2-没签到
 
                // 1，判断当天是否已经签到
                mongocxx::collection user_sign_in_coll = MONGODBCLIENT["gamemain"]["user_sign_in_info"];
                auto updateView = user_sign_in_coll.find_one(document{} << "userid" << userId << finalize);
                if (updateView) 
                {
                    auto view = updateView->view();
                    lastday                 = view["lastday"].get_int32();
                    today                   = view["today"].get_int32();
                    lastmon                 = view["lastmon"].get_int32();
                    curmon                  = view["curmon"].get_int32();
                    seriesday               = view["seriesday"].get_int32();
                    // 上次签到日期零点时间戳
                    int64_t lasttimestamp   = view["lasttimestamp"].get_int64(); 

                    // 两次时间戳差 (1天 86400秒) = 当前日期零点时间戳  - 上次签到日期零点时间戳 
                    int64_t duration = curTimeStamp - lasttimestamp;

                    LOG_INFO << "---签到,今天[" << curTimeStamp << "],昨天[" << lasttimestamp << "],两次时间戳差[" << duration << "]";

                    if (duration == 0 ){
                        issignin = 1;//1-已签到 2-没签到
                    }
                    else if (duration > ONE_DAY_SECOND || ((duration == ONE_DAY_SECOND) && (seriesday >= 7) )) // 签到天数相隔已经大于1天，或者累计达7天
                    { 
                        seriesday = 0; //连续签到天数归 0
                        user_sign_in_coll.update_one(document{} << "userid" << userId << finalize,
                                                     document{} << "$set" << open_document << "seriesday" << seriesday 
                                                     << close_document << finalize);

                        time_t _curstamp = time(nullptr);  // UTC秒数
                        tm *tp_ = localtime(&_curstamp);  
                        LOG_INFO << "---连续签到天数归零,今天[" << tp_->tm_mday << "]";

                        // 昨天
                        time_t forwardtimestamp = time_t(curstamp  - ONE_DAY_SECOND);//求出前一天的对应点的时间戳
                        tm *tpforward = localtime(&forwardtimestamp);           //求出前一天的时间结构体
                        LOG_INFO << "---昨天[" << tpforward->tm_mday << "]";

                        // 上次签到日期
                        time_t lasttimestamp_ = time_t(lasttimestamp);
                        tm *tplast = localtime(&lasttimestamp_);           //求出前一天的时间结构体
                        LOG_INFO << "---上次签到日期[" << tplast->tm_mday << "]";
                    }
                }

                // 2, 缓存签到信息
                if(cacheSignInInfo(agentid) != 0){
                    LOG_ERROR << "---缓存签到信息失败[" << agentid << "]";
                }

                // 3, 找到缓存信息
                auto it = m_SignInItemMap.find(agentid);
                if (it != m_SignInItemMap.end()) 
                {
                    for (auto signItem : it->second)
                    {
                        ::HallServer::SignItem*  SignItemList_ = response.add_subitem();
                        SignItemList_->set_sortid(signItem.sortid);
                        SignItemList_->set_gifttype(signItem.giftType);
                        SignItemList_->set_giftvalue(signItem.giftValue);
                        SignItemList_->set_minbet(signItem.minBet);
                        SignItemList_->set_title(signItem.title);
                    }
                    LOG_INFO << "---Sign In Item agentid[" << agentid << "],Item size[" << it->second.size() << "]";
                }

            } while (0);
        }
        catch (exception &e)
        {
            retCode = 1;
            LOG_ERROR << __FUNCTION__ <<  " >>> Exception: " << e.what();
        }
 
        response.set_seqday(seriesday);
        response.set_issignin(issignin);
        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 每日签到 sign In
void HallServer::cmd_sign_in(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SIGN_IN_RES;

    // request
    ::HallServer::SignInMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::SignInResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t seqdate          = msg.seriesdate();    // 当天签到第几天
        int32_t retCode          = 0;                   // 结果码 0-成功(连续签到) 1，流水不足 2 已经签到 3 异常错误 4 其它错误 5 正在结算中稍后再试
        int32_t lastday,today,lastmon,curmon,seriesday = 0;
        int64_t lastvalidbet,diff_validbet   = 0; //上次流水值,流水值差额
        DaliySignItem signInfo = {0};
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"],签到第几天[" << seqdate <<"]";

        // 做频率限制
        seriesday = seqdate;
        try
        {
            bool getSignInInfo = true;
            // 1，找到缓存代理签到信息
            {
                // 1.2, 缓存签到信息
                if(cacheSignInInfo(agentid) != 0){
                    getSignInInfo = false;
                    LOG_ERROR <<  "---" << __FUNCTION__ << "---缓存签到信息失败[" << agentid << "]";
                }

                auto signit = m_SignInItemMap.find(agentid);
                if (signit != m_SignInItemMap.end())
                {
                    for (auto signItem : signit->second)
                    {
                        //取出对应天数的奖励信息
                        if (signItem.sortid == seqdate)
                        {
                            signInfo = signItem;
                            break;
                        }
                    }
                }
                
                LOG_INFO << "---get sign Item agentid[" << agentid << "],sortid[" << signInfo.sortid << "],giftValue[" << signInfo.giftValue << "],title[" << signInfo.title << "]";
            }

            // 北京时间
            time_t curstamp = time(nullptr);  // UTC秒数
            tm *tp = localtime(&curstamp);  
            int64_t curTimeStamp = getZeroTimeStamp(tp);

            // 使用变量定义
            string account, mobile;
            int64_t totalvalidbet, beforechange, afterchange, beforechange_integral, afterchange_integral = 0,lasttimestamp = 0,currency = 0;
			
			tagConfig lkGame_config;
			memset(&lkGame_config, 0, sizeof(lkGame_config));
			lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

            do
            {
                //正在跑计划任务
                if (tp->tm_hour == 0 && tp->tm_min < 30)
                {
                    retCode = 5;
                    break;
                }
                if (userId <= 10000 || getSignInInfo == false)
                {
                    retCode = 4;
                    break;
                }
                 // 1.1 查询game_user表
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.projection(bsoncxx::builder::stream::document{} << "mobile" << 1 << "account" << 1 << "integralvalue" << 1 << "score" << 1 << "totalvalidbet" << 1 << "currency" << 1 << bsoncxx::builder::stream::finalize);
                mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                auto result = user_coll.find_one(document{} << "userid" << userId << finalize, opts);
                if (result)
                {
                    bsoncxx::document::view view = result->view();
                    beforechange            = view["score"].get_int64();
                    totalvalidbet           = view["totalvalidbet"].get_int64();
                    beforechange_integral   = view["integralvalue"].get_int64();
                    account                 = view["account"].get_utf8().value.to_string();
                    mobile                  = view["mobile"].get_utf8().value.to_string(); 
					//增加多币数支持
					if (view["currency"])
						currency = view["currency"].get_int32();
                }

				// 获取玩家对应幸运转盘的配置
				memset(&lkGame_config, 0, sizeof(lkGame_config));
				lkGame_config = m_LuckyGame->GetCurrencyConfig(currency);

                // 2，判断签到逻辑合法性
                mongocxx::collection user_sign_in_coll = MONGODBCLIENT["gamemain"]["user_sign_in_info"];
                auto updateView = user_sign_in_coll.find_one(document{} << "userid" << userId << finalize);

                // 2.1 没有记录则插入新记录
                if (!updateView) 
                {
                    time_t curstamp = time(nullptr);  // UTC秒数
                    time_t forwardtimestamp = time_t(curstamp  - ONE_DAY_SECOND);//求出前一天的对应点的时间戳
                    tm *tpforward = localtime(&forwardtimestamp);           //求出前一天的时间结构体
                    lasttimestamp = getZeroTimeStamp(tpforward);

                    auto insertValue = bsoncxx::builder::stream::document{}
                                                    << "userid" << userId  << "agentid" << agentid << "account" << account                                    
                                                    << "seriesday" << 0 << "lastday" << 0                //时间日期 //连续签到天数
                                                    << "today" << tp->tm_mday  << "lastvalidbet" << b_int64{0}   //今天签到时间 签到时的流水 totalvalidbet
                                                    << "curmon" << (tp->tm_mon + 1) << "lastmon" << 0       //今天签到时间 签到时的流水
                                                    << "lasttimestamp" <<  lasttimestamp         //当前日期零点时间戳
                                                    << "createtime" << b_date{chrono::system_clock::now()}          //创建时间
                                                    <<  bsoncxx::builder::stream::finalize;
                    if(!user_sign_in_coll.insert_one(insertValue.view())) {
                        LOG_ERROR << "---签到表增加玩家失败,userId["<< userId << ",agentid[" << agentid << "]";
                    }


                    // 需满足最低流水要求 
                    if ( totalvalidbet < signInfo.minBet)
                    {
                        retCode = 1; // 流水不足
                        diff_validbet = signInfo.minBet - totalvalidbet;  // 初次注册需要
                        LOG_INFO << "---玩家新创建，玩家流水不足,minBet[" << signInfo.minBet << "],totalvalidbet[" << totalvalidbet <<"]";
                        // break;// 退出
                    }

                    lastday                 = tpforward->tm_mday;
                    today                   = tpforward->tm_mday;
                    lastmon                 = tpforward->tm_mon;
                    curmon                  = tpforward->tm_mon;
                    seriesday               = 0;
                    lastvalidbet            = 0;
                    // lasttimestamp           = 0;
                    // LOG_INFO << "---0 tm_mon[" << tpforward->tm_mon << "],tm_mday[" << tpforward->tm_mday  << "]";
                }
                else
                {
                    // 2.2 有记录
                    auto view = updateView->view();
                    lastday                 = view["lastday"].get_int32();
                    today                   = view["today"].get_int32();
                    lastmon                 = view["lastmon"].get_int32();
                    curmon                  = view["curmon"].get_int32();
                    seriesday               = view["seriesday"].get_int32();
                    lastvalidbet            = view["lastvalidbet"].get_int64();
                    lasttimestamp           = view["lasttimestamp"].get_int64();
                }

                //两次时间戳差 (1天 86400秒)
                int64_t duration = curTimeStamp - lasttimestamp;

                LOG_INFO << "---当前时间戳[" << curTimeStamp << "],上次时间戳[" << lasttimestamp << "],时差要[" << duration <<"]";
                // 日期相同一天签到 且时间戳小于 1 天
                if (duration == 0 ){
                    retCode = 2; //已经签到，失败
                    LOG_INFO << "---玩家已经签到";
                    break;
                }

                // 流水值差额 = 要求值 - 上次增量
                diff_validbet = (totalvalidbet - lastvalidbet) - signInfo.minBet;
                LOG_INFO << "---最低流水要求,minBet[" << signInfo.minBet << "],totalvalidbet[" << totalvalidbet << "],diff_validbet[" << diff_validbet << "],lastvalidbet[" << lastvalidbet << "]";
                // 需满足最低流水要求 minBet 需要放大存储
                if ( diff_validbet < 0 )
                {
                    diff_validbet = 0 - diff_validbet;
                    retCode = 1; // 流水不足
                    LOG_INFO << "---玩家流水不足,minBet[" << signInfo.minBet << "],diff_validbet[" << diff_validbet << "],lastvalidbet[" << lastvalidbet << "],totalvalidbet[" << totalvalidbet << "]";
                    break;
                } 

                
                // 北京时间
                time_t curstamp_ = time(nullptr);  // UTC秒数
                tm *tp_tmp = localtime(&curstamp_);  

                if (duration > ONE_DAY_SECOND )
                {
                    // 连续天数记为1
                    seriesday = 1;

                    //连续签到失败，两次日期不相等 且 时间戳大于 2 天 则连续签到天数归 1
                    if (!user_sign_in_coll.update_one(document{} << "userid" << userId << finalize,
                                                            document{} << "$set" << open_document
                                                                << "seriesday" << seriesday         //连续签到天数 1
                                                                << "lastday" << tp_tmp->tm_mday         //更新当前签到时间日期
                                                                << "today" << tp_tmp->tm_mday           //更新当前签到时间日期
                                                                << "lastmon" << (tp_tmp->tm_mon + 1)          //更新当前签到时间日期
                                                                << "curmon" << (tp_tmp->tm_mon + 1)           //更新当前签到时间日期
                                                                << "lastvalidbet" << totalvalidbet  //更新当前流水值
                                                                << "lasttimestamp" << curTimeStamp  //更新当前签到时间戳
                                                                << close_document
                                                                << finalize))
                    {
                        LOG_ERROR << "---连续签到失败，两次日期不相等 last day[" << lastday << "],cur day[" << tp_tmp->tm_mday << "]";
                    }

                    LOG_WARN << "---隔两天以上签到，月份[" << (tp_tmp->tm_mon + 1) << "],今天[" << tp_tmp->tm_mday << "]";
                }
                else if (duration == ONE_DAY_SECOND ) //两天时间戳以内
                {

                    // 设置需要返回修改之后的数据
					mongocxx::options::find_one_and_update options;
            		options.return_document(mongocxx::options::return_document::k_after);
                    result = user_sign_in_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                    document{} << "$inc" << open_document
                                                                << "seriesday" << 1 << close_document  //连续签到天数 +1
                                                                << "$set" << open_document
                                                                << "lastday" << tp_tmp->tm_mday         //更新当前签到时间日期
                                                                << "today" << tp_tmp->tm_mday           //更新当前签到时间日期
                                                                << "lastmon" << (tp_tmp->tm_mon + 1)          //更新当前签到时间日期
                                                                << "curmon" << (tp_tmp->tm_mon + 1)           //更新当前签到时间日期
                                                                << "lastvalidbet" << totalvalidbet  //更新当前流水值
                                                                << "lasttimestamp" << curTimeStamp  //更新当前签到时间戳
                                                                << close_document
                                                                << finalize,options);
                    if (!result)
                    {
                        LOG_ERROR << "---连续签到失败，操作失败 last day[" << lastday << "],cur day[" << tp_tmp->tm_mday << "],cur timestamp[" << curstamp << "]";
                        break;
                    }

                    LOG_WARN << "---相邻天签到，月份[" << (tp_tmp->tm_mon + 1) << "],今天[" << tp_tmp->tm_mday << "]";

                    // 获取更新后的值
                    seriesday = result->view()["seriesday"].get_int32();
                } 
                else
                {
                    LOG_ERROR << "---sign In seq ok last day[" << lastday << "],cur day[" << tp_tmp->tm_mday << "],lastmon[" << lastmon << "],curmon[" << (tp_tmp->tm_mon + 1) << "],cur timestamp[" << curstamp << "]";
                }
                
                LOG_INFO << "---签到成功，连续签到天数["<< seriesday << "]";
                
            } while (0);
            

            // 3，签到成功,写帐变记录
            if(retCode == 0)
            {
                mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                // 分数转换
                int64_t incrscore = signInfo.giftType == eGiftType::coin ? (signInfo.giftValue * COIN_RATE) : 0;
                int64_t incrintegral = signInfo.giftType == eGiftType::integral ? signInfo.giftValue : 0;
                string lineCode;
                // 积分要转换存储 积分 * 500 * 100 缩放
                incrintegral = incrintegral * lkGame_config.nExChangeRate * COIN_RATE;

                // 设置需要返回修改之后的数据
                mongocxx::options::find_one_and_update options;
                options.return_document(mongocxx::options::return_document::k_after);

                // 3.2 更新玩家信息
                auto updateView = user_coll.find_one_and_update(document{} << "userid" << userId << finalize,
                                                                document{} << "$inc" << open_document
                                                                           << "score" << incrscore << "integralvalue" << incrintegral
                                                                           << close_document << finalize,options);
                if (updateView)
                {
                    auto viewfind           = updateView->view();
                    afterchange             = viewfind["score"].get_int64();
                    afterchange_integral    = viewfind["integralvalue"].get_int64();
                    lineCode                = viewfind["linecode"].get_utf8().value.to_string();
                }

                LOG_INFO << "---金币或者积分帐变记录,userId[" << userId << ",奖励类型[" << ((signInfo.giftType == eGiftType::coin)?"金币":"积分")  << "]";

                // 3.2 金币或者积分帐变记录 奖励类型 1  金币 0  积分
                stringstream reason;
                if(signInfo.giftType == eGiftType::coin)
                {
                    reason << account << " 第" << to_string(signInfo.sortid) << "天签到,奖" << signInfo.giftValue << "金币,[" << incrscore << "]";
                    LOG_INFO << "---" << reason.str();
                    //写帐变记录，保存玩家分更改记录
                    if(!AddScoreChangeRecordToDB(userId, account, incrscore, agentid, lineCode, beforechange, eUserScoreChangeType::op_sign_reward))
                        LOG_ERROR << "---账变记录失败,userId[" << userId << ",reason[" << reason.str() << "]";
                }
                else
                {
                    reason << account << " 第" << to_string(signInfo.sortid) << "天签到,奖" << signInfo.giftValue << "积分,[" << incrintegral << "]";
                     // 写积分记录
                    JiFenChangeRecordToDB(userId,account,agentid,incrintegral,beforechange_integral,eJiFenChangeType::op_sign_reward,reason.str());
                    LOG_INFO << "---" << reason.str();
                }

                     
                //奖励值
                response.set_giftvalue(signInfo.giftType == eGiftType::coin ? incrscore :signInfo.giftValue);    
                 //奖励类型    
                response.set_gifttype(signInfo.giftType);       
            }

            //连续签到天数 
            response.set_seqday(seriesday);  

            LOG_INFO << "---签到结果retCode["<< retCode << "],上次[" << lastday << "],今天[" << tp->tm_mday << "],当前第几天[" << seriesday << "],签到第几天[" << seqdate << "]";
        }
        catch (exception &e)
        {
            retCode = 3;
            LOG_ERROR << __FUNCTION__ <<  " >>> Exception: " << e.what();
        }
        // 流水差值
        response.set_needbetval(diff_validbet); 
        // 4，返回结果
        response.set_retcode(retCode);        //结果码
        
        
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 客服中心 customer service
void HallServer::cmd_customer_service(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_KE_FU_RES;

    // request
    ::HallServer::CustomerServiceMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::CustomerServiceResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 1;                   // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";
        try
        {
            // 找到缓存信息
            auto it = m_FQAItemMap.find(agentid);
            if (it != m_FQAItemMap.end())
            {
                for (auto fqaitem : it->second)
                {
                    ::HallServer::QandAItem *qaitem = response.add_qaitem();
                    qaitem->set_sortid(fqaitem.sortid);
                    qaitem->set_answer(fqaitem.answer);
                    qaitem->set_question(fqaitem.question);
                }
                LOG_INFO << "---FQA Item agentid[" << agentid << "],Item size[" << it->second.size() << "]";
            }
            else
            {
                vector<FQAItem> fqaIteminfo;
                mongocxx::collection q_coll = MONGODBCLIENT["gamemain"]["qestion_and_answer_info"];
                mongocxx::cursor info = q_coll.find(document{} << "agentid" << agentid << finalize);
                for (auto &doc : info)
                {
                    int32_t status = doc["status"].get_int32();
                    if (status == 0) //0显示 其它不显示
                    {
                        int32_t sortid  = doc["sortid"].get_int32();
                        string answer   = doc["answer"].get_utf8().value.to_string();
                        string question = doc["question"].get_utf8().value.to_string();
                        ::HallServer::QandAItem *qaitem = response.add_qaitem();
                        qaitem->set_sortid(sortid);
                        qaitem->set_question(question);
                        qaitem->set_answer(answer);

                        FQAItem fqaitem_ = {0};
                        fqaitem_.question   = question;
                        fqaitem_.sortid     = sortid;
                        fqaitem_.answer     = answer;
                        fqaIteminfo.push_back(fqaitem_);
                        LOG_INFO << "---FQAItem question[" << question << "],answer[" << answer << "],size[" << fqaIteminfo.size() << "]";
                    }
                }

                // 缓存到内存
                m_FQAItemMap[agentid] = fqaIteminfo;
            }

            // 找到缓存信息
            auto kfit = m_KeFuCfgItemMap.find(agentid);
            if (kfit != m_KeFuCfgItemMap.end())
            {
                for (auto kefucfg : kfit->second)
                {
                    ::HallServer::HerfItem*  herfitem_ = response.add_herfitem();
                    herfitem_->set_isjump(kefucfg.isjump);
                    herfitem_->set_weight(kefucfg.weight);
                    herfitem_->set_type(kefucfg.type);
                    herfitem_->set_viewtype(kefucfg.viewtype);
                    herfitem_->set_titlename(kefucfg.titlename);
                    herfitem_->set_href(kefucfg.href);
                }
                LOG_INFO << "---Kefu Item agentid[" << agentid << "],Item size[" << kfit->second.size() << "]";
            }
            else
            {
                //客服配置表
                vector<KeFuCfgItem> kefucfgItem;
                mongocxx::collection contact_coll = MONGODBCLIENT["gamemain"]["platform_contact"]; 
                mongocxx::cursor info_ = contact_coll.find(document{} << "agentid" << agentid << finalize);
                for (auto &doc : info_)//0显示 其它不显示
                {
                    int32_t status      = doc["status"].get_int32();
                    int32_t viewtype    = doc["viewtype"].get_int32();  // 是否在网页上显示
                    int32_t type        = doc["type"].get_int32();      // 客服类型
                    if (status == 0 && viewtype == 0 && type == 4)
                    {
                        string href;
                        int32_t isjump      = doc["isjump"].get_int32();    // 是否跳转
                        int32_t weight      = doc["weight"].get_int32();    // 跳转权重
                        
                        string titlename    = doc["typename"].get_utf8().value.to_string(); // 客服连接名字
                        if(doc["href"]) href  = doc["href"].get_utf8().value.to_string();     // 跳转连接
                        ::HallServer::HerfItem*  herfitem_ = response.add_herfitem();

                        herfitem_->set_isjump(isjump);
                        herfitem_->set_weight(weight);
                        herfitem_->set_type(type);
                        herfitem_->set_viewtype(viewtype);
                        herfitem_->set_titlename(titlename);
                        herfitem_->set_href(href);

                        KeFuCfgItem kefucfg = {0};
                        kefucfg.href        = href;
                        kefucfg.titlename   = titlename;
                        kefucfg.isjump      = isjump;
                        kefucfg.type        = type;
                        kefucfg.viewtype    = viewtype;
                        kefucfg.weight      = weight;
                        kefucfgItem.push_back(kefucfg);
                        LOG_INFO << "---KeFuCfgItem href[" << href << "],titlename[" << titlename << "],isjump[" << isjump << "],type[" << type << "],size[" << kefucfgItem.size() << "]";
                    }
                }
                // 缓存到内存
                m_KeFuCfgItemMap[agentid] = kefucfgItem;
            }

             // 返回其它图片（推广连接、返佣比例）连接信息,找到缓存信息
            auto imgit = m_ImgCfgItemMap.find(agentid);
            if (imgit != m_ImgCfgItemMap.end())
            {
                for (auto imgcfg : imgit->second)
                {
                    ::HallServer::ImgHerfItem*  imgherfitem_ = response.add_imgherf();
                    imgherfitem_->set_type(imgcfg.type);
                    imgherfitem_->set_href(imgcfg.href);
                }
                LOG_INFO << "---Img Item agentid[" << agentid << "],Item size[" << imgit->second.size() << "]";
            }
            else
            {
                string details, picPath = "http://192.168.2.214:9080";
                if (!get_platform_global_config(agentid, (int32_t)eAgentGlobalCfgType::op_cfg_file, details))
                {
                    LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agentid << "]";
                }
                else
                {
                    boost::property_tree::ptree pt;
                    std::stringstream ss(details);
                    read_json(ss, pt);
                    picPath = pt.get<string>("fullpath");

                    //客服配置表
                    vector<ImgCfgItem> imgcfgItemVec;
                    mongocxx::collection img_coll = MONGODBCLIENT["gamemain"]["platform_webcom_config"];
                    mongocxx::cursor img_info_ = img_coll.find(document{} << "agentid" << agentid << finalize);
                    for (auto &doc : img_info_) //0显示 其它不显示
                    {
                        int32_t status = doc["status"].get_int32(); // 状态
                        if (status == eDefaultStatus::op_ON)
                        {
                            int32_t type_id = doc["typeid"].get_int32();                        // 状态
                            string pictureurl = doc["pictureurl"].get_utf8().value.to_string(); // 连接名字
                            
                            ImgCfgItem imgcfg{type_id, picPath + pictureurl};
                            imgcfgItemVec.push_back(imgcfg);

                            ::HallServer::ImgHerfItem *imgherfitem_ = response.add_imgherf();
                            imgherfitem_->set_type(imgcfg.type);
                            imgherfitem_->set_href(imgcfg.href);
                        }
                    }

                    m_ImgCfgItemMap[agentid] = imgcfgItemVec;
                    LOG_INFO << "---增加其它图片连接,agentid[" << agentid << "],Item size[" << m_ImgCfgItemMap[agentid].size() << "]";
                }
            }

            // 完成
            retCode = 0;
        }
        catch (exception &e)
        {
            retCode = 1;
            LOG_ERROR << __FUNCTION__ <<  " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 设置默认卡类型
void HallServer::cmd_set_card_default(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_CARD_DEFUALT_RES;

    // request
    ::HallServer::SetCardDefaultMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::SetCardDefaultResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        string cardid           = msg.cardid();    	// 银行卡帐号/支付宝帐户/微信帐户/USDT唯一标识
        int32_t type            = msg.type();    	// 账户类型（建索引,1=银行卡,0=支付宝,2=微信,3=USDT ）
        int32_t retCode         = 1;                // 结果码
       
        LOG_INFO << "---" << __FUNCTION__ << " type[" << type << "],cardid[" << cardid << "],userId[" << userId <<"]";
        try
        {
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_bank_info"];
            // 
            auto choicechanged = make_document(kvp("firstchoice", 0));

            bsoncxx::builder::basic::document update_doc;
            update_doc.append(kvp("$set", choicechanged.view()));

            auto find = make_document(kvp("userid", userId), kvp("type", type));
            if (coll.update_many(find.view(), update_doc.view()))
            {
                // 给当前游客绑定帐号/手机号/密码/绑定状态
                auto seq_updateValue = document{} << "$set" << open_document << "firstchoice" << 1 << close_document << finalize;
                // auto seq_findValue = document{} << "userid" << userId << "type" << type  << "cardid" << cardid << finalize;
                auto seq_findValue = make_document(kvp("userid", userId), kvp("type", type), kvp("cardid", cardid));

                // 更新操作
                if (coll.update_one(seq_findValue.view(), seq_updateValue.view()))
                {
                    retCode = 0;
                    response.set_cardid(cardid);
                    response.set_type(type);
                }
            }
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
//计算出等级，以及本次升级获得的奖励
void HallServer::CalculatePlayerLevel(int64_t histrybet ,int32_t lastLevel ,int64_t &score,int32_t &viplevel,vector<int64_t> betlev,map<int32_t,int64_t> levscore,int64_t &upgradeamount)
{
    for(int i=0;i<betlev.size()-1;i++)
    {
        LOG_ERROR << "---betlev[i]  "<<betlev[i]<<"  betlev[i=1]  "<<betlev[i+1] << "  histrybet"<<histrybet;
        if(histrybet<betlev[0]*COIN_RATE)
        {
            viplevel=0;
            upgradeamount = betlev[0]*COIN_RATE-histrybet; //还需要多少洗码量升级
            break;
        }
        if(histrybet>=betlev[i]*COIN_RATE&&histrybet<betlev[i+1]*COIN_RATE)
        {
            viplevel = i+1;
            upgradeamount = betlev[i+1]*COIN_RATE-histrybet;
            LOG_ERROR << "---viplevel"<<viplevel << "  betlev[i]  "<<betlev[i]<<"  betlev[i=1]  "<<betlev[i+1];
            LOG_ERROR << "---histrybet="<<histrybet << "    upgradeamount="<<upgradeamount;
            break;
        }

    }
    if(viplevel-lastLevel>0)
    {
        for(int i =lastLevel;i<viplevel;i++ )
        {
            score+=levscore[i];
            LOG_ERROR << "---score"<<score << "  levscore[i]  "<<levscore[i];
        }


    }
    else
    {
        if(lastLevel<1)
        {
            score=0;
        }
        else
        {
            score=levscore[lastLevel];
        }

    }

}

// 获取提款配置信息
int32_t HallServer::getExchangeInfo(int32_t agentid, int32_t &bank_min, int32_t &bank_bonus, int32_t &zfb_min,
                                    int32_t &zfb_bonus, int32_t &exchangefreetime, int32_t &exchangefee, int32_t &forceFee, int32_t &aliforceFee)
{
    int32_t retCode = 0;

    try
    {
        mongocxx::collection withdraw_coll = MONGODBCLIENT["gamemain"]["platform_withdrawconfig"];
        auto withdraw_result = withdraw_coll.find_one(document{} << "agentid" << agentid << finalize);
        if (!withdraw_result)
        {
            retCode = 1;
            return retCode;
        }

        // 1，取出最小提款额度和手续费率
        auto viewfind = withdraw_result->view();
        bank_min = viewfind["minWithdrawMoney"].get_int32();
        bank_bonus = (int32_t)(viewfind["withdrawFee"].get_double() * COIN_RATE);
        zfb_min = viewfind["aliMinWithdrawMoney"].get_int32();
        zfb_bonus = (int32_t)(viewfind["aliWithdrawFee"].get_double() * COIN_RATE);
        exchangefreetime = viewfind["freeWithdrawTime"].get_int32();//免费次数
        exchangefee = viewfind["withdrawManageFee"].get_int32(); //管理/手续费
        forceFee = (int32_t)viewfind["compulsoryWithdrawalFee"].get_double();//银行强收手续费
        aliforceFee = (int32_t)viewfind["aliCompulsoryWithdrawalFee"].get_double(); //支付强收手续费

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],bm[" << bank_min << "],zm[" << zfb_min << "],bb[" << bank_bonus << "],zb[" << zfb_bonus << "],免费次数[" << exchangefreetime << "],手续费[" << exchangefee << "]";
    }
    catch (const std::exception &e)
    {
        retCode = 2;
        LOG_ERROR << __FUNCTION__ << " e.what[" << e.what() << "]";
    }

    return retCode;
}
// 获取平台配置信息
// {"bulletinMin":10,"zfb_bonus":2,"zfb_min":2,"bank_bonus":2,"bank_min":2,"reg_reward":0,"poor_reward":0,"sign_reward":0,"hby":0}
void HallServer::cmd_get_platform_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_Platform_INFO_RES;

    // 获取平台配置信息
    ::HallServer::GetPlatformMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetPlatformResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid = msg.agentid();
        int32_t retCode = 0;


        int32_t bulletinMin = 1000,zfb_bonus = 0,zfb_min = 0,bank_bonus=0,bank_min = 0,exchangefreetime = 0,exchangefee = 0;
        int32_t reg_reward = 0,poor_reward = 0,sign_reward = 0,hby = 0,forceFee = 0,aliforceFee = 0,use_wx = 0,use_vip = 0;
        string jsonString,details;
        std::stringstream ss;
        boost::property_tree::ptree root;
        root.put("exchangefreetime", ":free");    //免费提现次数
        root.put("exchangefee", ":fee");          //提现手续费
        root.put("bulletinMin", ":bMin");       //公告要求最低金币值
        root.put("zfb_bonus", ":zfbb");         //支付宝手续费
        root.put("zfb_min", ":zfbm");           //支付宝最低提现额度
        root.put("bank_bonus", ":bankb");       //银行卡手续费
        root.put("bank_min", ":bankm");         //银行卡最低提现额度
        root.put("forceFee", ":forceFee");         //银行卡强收手续费
        root.put("aliforceFee", ":aliforceFee");   //支付宝强收手续费

        root.put("reg_reward", ":reg");         //注册送金开关
        root.put("poor_reward", ":poor");       //低保送金开关
        root.put("sign_reward", ":sign");       //每日签到开关
        root.put("hby", ":hby");                //红包雨开关

        root.put("use_wx", ":use_wx");          //是否启用微信开关 use_wx:0不启用微信，1启用微信
        root.put("use_vip", ":use_vip");          //是否启用VIP开关 use_vip:0不启用，1启用
        //
        bulletinMin = m_lkGame_config.bulletinMin;

        try
        {
            do
            {
                if(agentid <= 0 ) {
                    retCode = 5;
                    break;
                }

                // 获取兑换配置信息
                if(retCode = getExchangeInfo(agentid,bank_min,bank_bonus,zfb_min,zfb_bonus,exchangefreetime,exchangefee,forceFee,aliforceFee) > 0)
                    break;

                // 2，低保送金等活动开关
                if(!cacheActiveItem(agentid)){  // 读缓存
                    retCode = 3;
                    break;
                }

                if(!cacheRegPoorItem(agentid)){  // 读缓存
                    retCode = 4;
                    break;
                }

                // 功能开关(读取不到默认关掉微信)
                if (get_platform_global_config(agentid, (int32_t)eAgentGlobalCfgType::op_cfg_funcSwitch,details))
                {
                    try
                    {
                        boost::property_tree::ptree pt;
                        std::stringstream ss(details);
                        read_json(ss, pt);
                        use_wx = pt.get<int32_t>("use_wx",0);
                        use_vip = pt.get<int32_t>("use_vip",0);
                    }
                    catch(const std::exception& e)
                    {
                         LOG_ERROR << "---" << __FUNCTION__ << ",what["<< e.what() << "]";
                    }
                }
                
                RegPoorItem regPoorItem;
                auto regit = m_RegPoorMap.find(agentid);
                if (regit != m_RegPoorMap.end())
                    regPoorItem = regit->second;

                // 找到活动缓存信息
                auto it = m_ActiveItemMap.find(agentid);
                if (it != m_ActiveItemMap.end()) 
                {
                    for (auto activeItem : it->second)
                    {
                        if(activeItem.type == (int32_t)eActiveType::op_reg_poor)
                        {
                            // 设置有注册送金大于0
                            if(regPoorItem.rgtReward > 0 )
                                reg_reward = 1;
                             
                            // 设置低保送金大于0
                            if(regPoorItem.handselNo > 0 )
                                poor_reward = 1;
                        }
                        
                        // 每日签到开关
                        if(activeItem.type == (int32_t)eActiveType::op_sign_in)
                            sign_reward = 1;

                        // 红包雨开关
                        if(activeItem.type == (int32_t)eActiveType::op_hby)
                            hby = 1;
                    }
                }

            } while (0);

            boost::property_tree::json_parser::write_json(ss, root, false);
            jsonString = ss.str();
            // 兑换信息
            GlobalFunc::replace(jsonString, ":free", std::to_string(exchangefreetime));
            GlobalFunc::replace(jsonString, ":fee", std::to_string(exchangefee * COIN_RATE));
            GlobalFunc::replace(jsonString, ":bMin", std::to_string(bulletinMin * COIN_RATE));
            GlobalFunc::replace(jsonString, ":zfbb", std::to_string(zfb_bonus));
            GlobalFunc::replace(jsonString, ":zfbm", std::to_string(zfb_min * COIN_RATE));
            GlobalFunc::replace(jsonString, ":bankb", std::to_string(bank_bonus));
            GlobalFunc::replace(jsonString, ":bankm", std::to_string(bank_min * COIN_RATE));
            GlobalFunc::replace(jsonString, ":forceFee", std::to_string(forceFee * COIN_RATE)); //银行卡强收手续费
            GlobalFunc::replace(jsonString, ":aliforceFee", std::to_string(aliforceFee * COIN_RATE));//支付宝强收手续费

            // 低保送金等活动开关
            GlobalFunc::replace(jsonString, ":reg", std::to_string(reg_reward));
            GlobalFunc::replace(jsonString, ":poor", std::to_string(poor_reward));
            GlobalFunc::replace(jsonString, ":sign", std::to_string(sign_reward));
            GlobalFunc::replace(jsonString, ":hby", std::to_string(hby));

            GlobalFunc::replace(jsonString, ":use_wx", std::to_string(use_wx));
            GlobalFunc::replace(jsonString, ":use_vip", std::to_string(use_vip));

            boost::replace_all<std::string>(jsonString, "\\", "");
            jsonString = boost::regex_replace(jsonString, boost::regex("\n|\r"), "");
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            retCode=6;
        }

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "],retCode[" << retCode << "],jsonString[" << jsonString << "]";

        response.set_retcode(retCode);
        response.set_jsonstr(jsonString);
        
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 请求vip奖项信息
void HallServer::cmd_get_vip_rewardcolumn(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_VIP_REWORD_RES;

    // request
    ::HallServer::GetRewardMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRewardMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        int64_t dbuserId = internal_header->userId;
        int32_t dbagentid=0;
        int64_t dbscore = 0;
        int32_t dbtype = 0;
        int64_t dbtopup = 0;
        int32_t dbstatus = 0;
        int64_t  dbuserbetting = 0;

        int32_t dblastlevel = 0;

        string dbaccount = "";

        int32_t dbNowLevel = 0;


        int32_t retcodes= 0;


        int64_t monlevel  =0 ;
        int64_t monstatus  =0 ;
        int64_t montype  =0 ;
        int64_t monscore  =0 ;


        int64_t weeklevel  =0 ;
        int64_t weekstatus  =0 ;
        int64_t weektype  =0 ;
        int64_t weekscore  =0 ;
        int64_t dbupgradeamount = 0;
        int32_t percentage = 0;
        try
        {

            mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
            auto user_result = user_coll.find_one(document{} << "userid" << dbuserId << finalize);
            if (user_result)
            {
                auto viewfind           = user_result->view();
                dbuserbetting             = viewfind["totalvalidbet"].get_int64();
                dbagentid                 = viewfind["agentid"].get_int32();
                dbaccount                 = viewfind["account"].get_utf8().value.to_string();
            }

            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["memberlevel_welfare"];
            auto updateView = coll.find_one(document{} << "userid" << dbuserId <<"cycletype"<<int32_t(3)<< finalize);
            // db
            LOG_ERROR << "---读取玩家的vip表   ";
            // 2.1 没有记录则插入新记录
            if (!updateView)
            {
                LOG_ERROR << "  ---读取玩家的vip表失败   ";
                auto insertValue = bsoncxx::builder::stream::document{}
                                                << "agentid" << dbagentid
                                                << "userid"  << dbuserId
                                                << "welfarescore"  << (int64_t)dbscore
                                                << "cycletype" << (int32_t)3
                                                << "addscore" <<(int64_t)dbtopup
                                                << "status" << dbstatus
                                                << "memberlevel" << dblastlevel
                                                << "account" <<dbaccount
                                                <<  bsoncxx::builder::stream::finalize;
                if(!coll.insert_one(insertValue.view()))
                {
                    LOG_ERROR << "---,userId["<< dbuserId << ",agentid[" << dbagentid << "]";
                }
            }
            //假如有记录
            else
            {

                LOG_ERROR << "---读取玩家的vip表成功   ";
                auto view = updateView->view();
                dblastlevel   = view["memberlevel"].get_int32();
                dbstatus      = view["status"].get_int32();
                dbtype      = view["cycletype"].get_int32();
                dbscore     = view["welfarescore"].get_int64();
                LOG_ERROR << "---读取玩家的vip表成功   "<<dbscore<<"  dbstatus"<<dbstatus<<" dbscore"<<dbscore<<"   dblastlevel"<<dblastlevel;
            }
            if(m_ProxyLevelStandard.find(dbagentid)!=m_ProxyLevelStandard.end())
            {
                vector<int64_t> betscore;
                betscore.clear();
                map<int32_t, int64_t> levscore;
                levscore.clear();
                vector<ProxyLevelInfo>::iterator it = m_ProxyLevelStandard[dbagentid].begin();
                for (; it != m_ProxyLevelStandard[dbagentid].end(); it++)
                {
                    betscore.push_back((*it).bettingScore);
                    levscore[(*it).viplevel] = (*it).upgradeScore;

                    LOG_INFO << "---levscore[(*it).viplevel]    " << (*it).upgradeScore << "   (*it).viplevel" << (*it).viplevel << "  bettingScore" << (*it).bettingScore;
                }
                LOG_ERROR << "---betscore-size" << betscore.size() << "   levscore-size" << levscore.size() << "  dbuserbetting " << dbuserbetting;
                sort(betscore.begin(), betscore.end());
                CalculatePlayerLevel(dbuserbetting, dblastlevel, dbscore, dbNowLevel, betscore, levscore, dbupgradeamount);
                if (dbNowLevel < levscore.size() - 1)
                {
                    if (dbNowLevel >= 1)
                    {
                        percentage = (dbuserbetting - betscore[dbNowLevel - 1] * COIN_RATE) / ((betscore[dbNowLevel] * COIN_RATE - betscore[dbNowLevel - 1] * COIN_RATE) / 100);
                    }
                    else
                    {
                        percentage = (dbuserbetting) / ((betscore[0] * COIN_RATE) / 100);
                    }
                }
                else
                {
                    percentage = 100;
                }

                LOG_INFO << "---percentage"<<percentage<< "    dblastlevel"<<dblastlevel <<"  dbNowLevel"<<dbNowLevel;
                //  LOG_ERROR << "---percentage"<<percentage;

                if(dblastlevel!=dbNowLevel)
                {
                    dbstatus = 1;
                    LOG_ERROR << "---dbstatus"<<dbstatus<<" userid "<<dbuserId;
                    coll.update_one(document{} << "userid" << dbuserId << "cycletype"<<(int32_t)3<<finalize,
                                    document{} << "$set" << open_document
                                               << "status" << dbstatus
                                               <<close_document
                                               << finalize);
                }
            }
            //上面已经算出了本次玩家是否升级是否有奖励等,是否可以领取

            //还要读出玩家周奖励和月奖励数据，返回给玩家

            updateView = coll.find_one(document{} << "userid" << dbuserId <<"cycletype"<<int32_t(1)<< finalize);
            if(updateView)
            {
                auto view = updateView->view();
                weeklevel   = view["memberlevel"].get_int32();
                weekstatus      = view["status"].get_int32();
                weektype      = view["cycletype"].get_int32();
                weekscore     = view["welfarescore"].get_int64();
            }

            updateView = coll.find_one(document{} << "userid" << dbuserId <<"cycletype"<<int32_t(2)<< finalize);
            if(updateView)
            {
                auto view = updateView->view();
                monlevel   = view["memberlevel"].get_int32();
                monstatus      = view["status"].get_int32();
                montype      = view["cycletype"].get_int32();
                monscore     = view["welfarescore"].get_int64();
            }
        }
        catch(exception &e)
        {
            retcodes =1  ;
        }

        resp_header->CopyFrom(header);
        //vip等级信息,周福利信息,月福利信息
        response.set_retcode(retcodes);
        response.set_viplevel(dbNowLevel);
        response.set_levelmark(dbstatus);
        response.set_levelreward(dbscore*COIN_RATE);
        response.set_monmark(monstatus);
        response.set_percentage(percentage);
        response.set_weekmark(weekstatus);
        response.set_weekreward(weekscore);
        response.set_monreward(monscore);
        response.set_upgradeamount(dbupgradeamount);
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }


}
void HallServer::WriteVipToMysql(int64_t userid,int32_t agentid,
                         int64_t dbwelfarescore,int64_t dbrewardType ,int64_t dbaddscore ,int32_t dbnewlevel,
                         int64_t dbvalidbet ,string dbaccount )

{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO play_vip_record(userid, agentid, "
//                                           "welfarescore,welftype,addscore,menberlevel,validbet,"
//                                           "account,inserttime)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?)"));
//        pstmt->setInt64(1, userid);
//        pstmt->setInt(2, agentid);
//        pstmt->setInt64(3, dbwelfarescore);
//        pstmt->setInt(4, dbrewardType);
//        pstmt->setInt64(5, dbaddscore);
//        pstmt->setInt64(6, dbnewlevel);
//        pstmt->setInt64(7, dbvalidbet);
//        pstmt->setString(8, dbaccount);
//        pstmt->setDateTime(9, InitialConversion(int64_t(time(NULL))));
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
//领取奖励
void HallServer::cmd_receive_vip_reward(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECEIVE_REWORD_RES;

    // request
    ::HallServer::ReceiveRewardMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::ReceiveRewardMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);
        int32_t dbrewardType = msg.rewardtype();
        int32_t dbrewardScore = msg.rewardscore();

        int64_t userId = internal_header->userId;
        string strlinecode;

        int32_t recode = 0;
        int64_t dbaddscore = 0;
        int32_t   dbagentid = 0;
        int32_t   dbmenberlevel = 0;
        int32_t   dbnewlevel = 0;
        int32_t  dbstatus = 0;
        int64_t dbwelfarescore = 0;

        int64_t dbvalidbet = 0;
        int64_t dbuserscore = 0;

        int64_t  dbuserbetting =0;

        string dbaccount = "";

        auto opts = mongocxx::options::find{};
        opts.projection(document{} << "totalvalidbet" << 1 << "score" << 1 << "account" << 1 << "linecode" << 1 << finalize);
        mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto user_result = user_coll.find_one(document{} << "userid" << internal_header->userId << finalize);
        if (user_result)
        {
            auto viewfind = user_result->view();
            dbuserbetting = viewfind["totalvalidbet"].get_int64();
            dbuserscore = viewfind["score"].get_int64();
            dbvalidbet = dbuserbetting;
            dbaccount = viewfind["account"].get_utf8().value.to_string();
            strlinecode = viewfind["linecode"].get_utf8().value.to_string();
        }
        //领取周奖励
        LOG_INFO << "---,dbrewardType" << dbrewardType;

        if (dbrewardType == 1)
        {
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["memberlevel_welfare"];
            auto updateView = coll.find_one(document{} << "userid" << internal_header->userId << "cycletype" << int32_t(1) << finalize);
            if (!updateView)
            {
                recode = 1;
            }
            else
            {
                auto vew = updateView.value().view();
                dbaddscore = vew["addscore"].get_int64();
                dbagentid = vew["agentid"].get_int32();
                dbmenberlevel = vew["memberlevel"].get_int32();
                dbstatus = vew["status"].get_int32();
                dbwelfarescore = vew["welfarescore"].get_int64();
            }
        }
        else if (dbrewardType == 2) //领取月奖励
        {
            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["memberlevel_welfare"];
            auto updateView = coll.find_one(document{} << "userid" << internal_header->userId << "cycletype" << int32_t(2) << finalize);
            if (!updateView)
            {
                recode = 1;
            }
            else
            {
                auto vew = updateView.value().view();
                dbaddscore = vew["addscore"].get_int64();
                dbagentid = vew["agentid"].get_int32();
                dbmenberlevel = vew["memberlevel"].get_int32();
                dbstatus = vew["status"].get_int32();
                dbwelfarescore = vew["welfarescore"].get_int64();
            }
        }
        else if (dbrewardType == 3) //领取升级奖励
        {

            LOG_ERROR << "---,领取升级奖励";

            mongocxx::collection coll = MONGODBCLIENT["gamemain"]["memberlevel_welfare"];
            auto updateView = coll.find_one(document{} << "userid" << internal_header->userId << "cycletype" << int32_t(3) << finalize);
            if (!updateView)
            {
                recode = 1;
                LOG_ERROR << "---!updateView";
            }
            else
            {
                LOG_ERROR << "---读取memberlevel_welfare成功";
                auto vew = updateView.value().view();
                dbaddscore = vew["addscore"].get_int64();
                dbagentid = vew["agentid"].get_int32();
                dbmenberlevel = vew["memberlevel"].get_int32();
                dbstatus = vew["status"].get_int32();
                dbwelfarescore = vew["welfarescore"].get_int64();

                if (dbstatus == 0)
                {
                    recode = 1;
                    LOG_ERROR << "---,dbwelfarescore==0||dbstatus==0";
                }
                else
                {
                    dbstatus = 0;

                    int32_t dbNowLevel = 0;
                    if (m_ProxyLevelStandard.find(dbagentid) != m_ProxyLevelStandard.end())
                    {

                        vector<int64_t> betscore;
                        betscore.clear();
                        map<int32_t, int64_t> levscore;
                        levscore.clear();
                        vector<ProxyLevelInfo>::iterator it = m_ProxyLevelStandard[dbagentid].begin();
                        for (; it != m_ProxyLevelStandard[dbagentid].end(); it++)
                        {
                            betscore.push_back((*it).bettingScore);
                            levscore[(*it).viplevel] = (*it).upgradeScore;

                            LOG_INFO << "---levscore[(*it).viplevel]    " << (*it).upgradeScore << "   (*it).viplevel" << (*it).viplevel << "  bettingScore" << (*it).bettingScore;
                        }
                        sort(betscore.begin(), betscore.end());

                        int64_t dbupgradeamount;
                        CalculatePlayerLevel(dbuserbetting, dbmenberlevel, dbwelfarescore, dbnewlevel, betscore, levscore, dbupgradeamount);

                        dbwelfarescore = dbwelfarescore * COIN_RATE;
                    }
                    coll.update_one(document{} << "userid" << internal_header->userId << "cycletype" << 3 << finalize,
                                    document{} << "$set" << open_document << "status" << (int32_t)0
                                               << "memberlevel" << dbNowLevel << close_document
                                               << finalize);
                    LOG_ERROR << "---,领取升级奖励,status strage";
                }
            }
        }

        //成功了，就更新数据库
        if (recode == 0)
        {
            try
            {
                bsoncxx::document::value findValue = document{} << "userid" << internal_header->userId << "cycletype" << dbrewardType << finalize;
                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["memberlevel_welfare"];
                bsoncxx::document::value updateValue = document{}
                                                       << "$set" << open_document
                                                       << "status" << 0
                                                       << "welfarescore" << (int64_t)0
                                                       << "memberlevel" << dbnewlevel
                                                       << close_document
                                                       << finalize;
                // LOG_DEBUG << "Update Document:"<<bsoncxx::to_json(updateValue);
                coll.update_one(findValue.view(), updateValue.view());

                //插入vip福利注单表
                bsoncxx::document::value insert_value = document{}
                                                        << "userid" << internal_header->userId
                                                        << "agentid" << dbagentid
                                                        << "welfarescore" << (int64_t)dbwelfarescore
                                                        << "welftype" << dbrewardType
                                                        << "addscore" << (int64_t)dbaddscore
                                                        << "menberlevel" << dbnewlevel
                                                        << "validbet" << (int64_t)dbvalidbet
                                                        << "account" << dbaccount
                                                        << "inserttime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                                        << finalize;
                //在这里写入注单
                mongocxx::collection coll1 = MONGODBCLIENT["gamemain"]["play_vip_record"];
                if (!coll1.insert_one(insert_value.view()))
                {
                    LOG_ERROR << "play_record insert exception ";
                }
                LOG_ERROR<<"mysql插入  play_vip_record";
                //之所以不用lamber表达式时因为直接使用局部变量会导致原线程值被删除了，使用一个被删除的值进行计算
                m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteVipToMysql,this, internal_header->userId, dbagentid,
                                                                           dbwelfarescore, dbrewardType , dbaddscore ,dbnewlevel,
                                                                          dbvalidbet , dbaccount ));


                user_coll.update_one(document{} << "userid" << internal_header->userId << finalize,
                                     document{} << "$inc" << open_document << "score" << (int64_t)dbwelfarescore
                                                << close_document
                                                << finalize);
                LOG_INFO << "---,领取升级奖励,并且写分数 dbmenberlevel" << dbmenberlevel << "      dbwelfarescore" << dbwelfarescore << "    userid" << internal_header->userId;

                // 帐变类型
                int32_t changeTypeId = eUserScoreChangeType::op_game_record + 1;
                switch (dbrewardType)
                {
                //月俸禄
                case 2:  changeTypeId = eUserScoreChangeType::op_PlatformMonthBonus;break;
                //周俸禄
                case 1: changeTypeId = eUserScoreChangeType::op_PlatformWeekBonus;break;
                //VIP 升级 
                case 3: changeTypeId = eUserScoreChangeType::op_PlatformVipUpgrade;break;
                default:
                    LOG_ERROR << __FUNCTION__ << "---帐变类型未知--- "<< changeTypeId;
                    break;
                }
                // 写帐变记录
                AddScoreChangeRecordToDB(userId, dbaccount, dbwelfarescore, dbagentid, strlinecode, dbuserscore,changeTypeId);
            }
            catch (exception &e)
            {
                recode = 1;
                LOG_ERROR << "---exception &e";
            }
        }
        response.set_retcode(recode);
        response.set_rewardscore(dbwelfarescore);
        response.set_rewardtype(dbrewardType);
        response.set_usercore(dbuserscore);
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 获取注册送金请求
void HallServer::cmd_get_reg_reward(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_REG_REWARD_RES;

    // request
    ::HallServer::GetRegRewardMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRegRewardResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid         = msg.agentid();    // 代理ID
        int32_t reqtype         = msg.reqtype();    // 代理ID
        int32_t retCode         = 0; // 结果码
        int32_t regReward       = 0; //注册送金(首次注册送金数量,当值为0时则该功能不启用)
        int32_t reqCount        = 0; //今天可领取次数(今日可领取 3 次)
        int32_t reqReward       = 0; //领取金币数量（立即领取 2 枚免费金币）
        int32_t minCoin         = 0; //金币不足 1 枚时可领取(当值为0时则该功能不启用)
        int32_t maxCoin         = 0; //最高可提款100金币，只限一次
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";

        try
        {
            //获取注册送金配置
            retCode = getRewardConfig(agentid, regReward, reqCount, reqReward, minCoin, maxCoin);

            do
            {
                if (retCode != 0)
                    break;

                redis::RedisValue values;
                string fields[2] = {timestampKey, countKey};

                // enumToString(UserEnum::lasttimestamp);

                string strKeyName = makeRedisKey(userId,eRedisKey::has_user_info);


                bool res = REDISCLIENT.hmget(strKeyName, fields, 2, values);
                if (!res)
                {
                    LOG_INFO << " --- " << __FUNCTION__ << ",key Name[" << strKeyName << "] res is false";
                    retCode = 3;
                    break;
                }

                int64_t lasttimestamp = values[timestampKey].asInt64();
                reqCount = values[countKey].asInt();

                LOG_INFO << " --- " << __FUNCTION__ << ",key Name[" << strKeyName << "],timestampKey[" << lasttimestamp << "],countKey[" << reqCount << "]";

                //领取
                if (reqtype == 1)
                {
                    string strAccount, lineCode;
                    int64_t beforeScore = 0,safeboxScore = 0;
                    int32_t isguest = 0,txhstatus = 0;

                    if (reqCount <= 0)
                    {
                        retCode = 4;
                        break;
                    }

                    auto opts = mongocxx::options::find{};
                    opts.projection(document{} << "isguest" << 1 << "score" << 1 << "linecode" << 1 << "account" << 1 <<"safebox" << 1 << "txhstatus" << 1 << finalize);
                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];

                    auto find_doc = make_document(kvp("userid", userId));
                    auto find_result = user_coll.find_one(find_doc.view(), opts);
                    if (find_result)
                    {
                        auto find_view = find_result->view();
                        strAccount = find_view["account"].get_utf8().value.to_string();
                        lineCode = find_view["linecode"].get_utf8().value.to_string();
                        beforeScore = find_view["score"].get_int64();
                        safeboxScore = find_view["safebox"].get_int64();
                        isguest = find_view["isguest"].get_int32();
                        txhstatus = find_view["txhstatus"].get_int32();
                    }

                    LOG_INFO << "---" << __FUNCTION__ << " 第三方游戏[" << txhstatus << "],已经有分数 [" << beforeScore << "],保险箱["<< safeboxScore <<"],minCoin * COIN_RATE[" << minCoin * COIN_RATE << "]";

                    // 判断下玩家是否有跳转第三方游戏
                    if( txhstatus > 0 )
                    {
                        retCode = 6;
                        break;
                    }
                   
                    // 已经有分数
                    if ((beforeScore + safeboxScore) >= minCoin * COIN_RATE)
                    {
                        retCode = 5;
                        LOG_INFO << "---" << __FUNCTION__ << " 已经有分数 [" << beforeScore << "],userId[" << userId << "]";
                        break;
                    }

                    // 更新玩家分
                    if (isguest == eGuestMode::formal)
                    {
                        int64_t rewardScore = reqReward * COIN_RATE;
                        if (user_coll.update_one(find_doc.view(), make_document(kvp("$inc", make_document(kvp("score", rewardScore))))))
                        {
                            //写帐变记录，保存玩家分更改记录
                            AddScoreChangeRecordToDB(userId, strAccount, rewardScore, agentid, lineCode, beforeScore, eUserScoreChangeType::op_poor_reward);
                            LOG_INFO << "---" << __FUNCTION__ << " 领取金币 [" << rewardScore << "],userId[" << userId << "]";
                        }

                        int64_t current = 0;
                        if (REDISCLIENT.hincrby(strKeyName, countKey, -1, &current))
                        {
                            reqCount = current < 0 ? 0 : current;
                            LOG_INFO << "---" << __FUNCTION__ << " 剩余领取次数 [" << reqCount << "],userId[" << userId << "],领取前[" << beforeScore << "],当前分数[" << beforeScore + rewardScore << "]";
                        }

                        response.set_curscore(beforeScore + rewardScore);
                    }
                }
                else
                {
                    auto opts = mongocxx::options::find{};
                    opts.projection(document{} << "score" << 1 << finalize);
                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                    auto find_result = user_coll.find_one(make_document(kvp("userid", userId)), opts);
                    if (find_result)
                        response.set_curscore(find_result->view()["score"].get_int64());
                }

            } while (0);
        }
        catch (const std::exception &e)
        {
             retCode = 6;
            std::cerr << e.what() << '\n';
            LOG_ERROR << "---" << __FUNCTION__ << " 领取送金异常,e.what()[" << e.what() << "]";
        }

        // 功能被禁用
        if(retCode == -1){
            retCode = 0;
        }

        response.set_regreward(regReward); 
        response.set_reqreward(reqReward); 
        response.set_reqcount(reqCount); 
        response.set_mincoin(minCoin); 
        response.set_maxcoin(maxCoin); 

        response.set_reqtype(reqtype); //操作码
        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

bool HallServer::get_platform_global_config(int32_t agent_id,int32_t type_id,string & details)
{
	try
	{
		//1, 从用户表中查询出会员层级
		mongocxx::collection global_config_coll = MONGODBCLIENT["gamemain"]["platform_global_config"];
		auto find_result = global_config_coll.find_one(document{} << "agentid" << agent_id << "typeid" << type_id << finalize);
		if (!find_result)
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agent_id << "]";
			return false;
		}

		// 1.1 取出结果
		bsoncxx::document::view view = find_result->view();
		int32_t status = view["status"].get_int32();
		details = view["details"].get_utf8().value.to_string();
		if (status != eDefaultStatus::op_ON || details.empty())
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 代理没开放此功能[" << agent_id << "]";
			return false;
		}
	}
	catch(const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 玩家创建邮件失败----" << e.what();
		return false;
	}

	return true;
}

// 从用户表中查询出会员层级
int32_t HallServer::getRewardConfig(int32_t agentid,int32_t &regReward,int32_t &reqCount,int32_t &reqReward,int32_t &minCoin,int32_t &maxCoin)
{
    int32_t retCode = 0;
    do
    {
        // 读缓存
        if (!cacheRegPoorItem(agentid))
        {
            retCode = 1;
            break;
        }

        RegPoorItem regPoorItem;
        auto regit = m_RegPoorMap.find(agentid);
        if (regit != m_RegPoorMap.end())
            regPoorItem = regit->second;

        regReward = regPoorItem.rgtReward;
        reqCount = regPoorItem.handselNo;
        reqReward = regPoorItem.reqReward;
        minCoin = regPoorItem.minCoin;
        maxCoin = 100; // regPoorItem.;

        LOG_INFO << "---" << __FUNCTION__ << ",regReward [" << regReward << "],reqCount[" << reqCount << "],reqReward [" << reqReward << "],minCoin[" << minCoin << "],maxCoin[" << maxCoin << "]";

    } while (0);

    return retCode;
}

// 获取充值信息（弃用,把此功能移到 PayServer实现了）
void HallServer::cmd_get_recharge_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_INFO_RES;

    // request
    ::HallServer::GetRechargeInfoMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRechargeInfoResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t retCode         = 0;                // 结果码
        int32_t agentid         = msg.agentid();    // 代理ID
        int32_t paytype         = 1;// 1 为移动端 0　为PC端
        int32_t level           = 1; //会员层级
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";

        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 充值订单请求  会员存款记录数据结构
void HallServer::cmd_recharge_order(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_ORDER_RES;

    // request
    ::HallServer::RechargeOrderMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::RechargeOrderResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid         = msg.agentid();            // 代理ID
        int32_t ntype           = msg.type();               //汇款方式  ATM 手机  支付宝 柜台转等
        int64_t amount          = msg.amount();             // 汇款金额(分)
        string payNameC         = msg.payname();            //银行名称
        string accountNameC     = msg.accountname();        //收款账户
        string accountNumberC   = msg.accountnumber();      //收款账号
        string details          = msg.details();            //存款人备注信息
        int32_t retCode         = 0;                        // 结果码

        double integral = 0;                        /// 积分分值(分)  从数据库里取出优惠比例 有就给
        int64_t yhAmount,cjAmount,betAmount = 0;    /// 打码量(分)  取出打码量最低取款条件
        int32_t status,todayTimes,isFirst = 0;      /// 是否首存  game_user 上分次数判断
        string payName,payNumber,type,ttype,accountName,accountNumber,bankName,bankAddress,audituser,description; 

        payName = payNameC;
        accountName = accountNameC;
        accountNumber = accountNumberC;
        description = details;
        type = to_string(ntype);
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "],amount[" << amount << "],payName[" << payName << "],accountName[" << accountName << "],accountNumber[" << accountNumber << "],type[" << type << "]";
        try
        {
            // 同一层级各个支付渠道优惠
            do
            {

                if (ntype <= 0 || ntype > 6)
                {
                    retCode = 6;
                    LOG_ERROR << "---" << __FUNCTION__ << " 数据非法,ntype[" << ntype << "]";
                    break;
                }

                // 非法数据过滤
                if(amount < 0 || amount >= 1073741823){
                    retCode = 1;
                    LOG_ERROR << "---" << __FUNCTION__ << " 数据非法[" << amount << "]";
                    break;
                }

                auto getPayTypeByIdx = [](int32_t type_ )
                {
                    switch(type_){
                        case 1: return "银行转帐";
                        case 2: return "支付宝";
                        case 3: return "ATM柜员机转帐";
                        case 4: return "ATM柜员机现金";
                        case 5: return "银行柜台";
                        case 6: return "云闪付支付";
                        default : return "未知方式";
                    };
                };
                // 1, 从用户表中查询出相关信息
                auto opts = mongocxx::options::find{};
                opts.projection(document{} << "addscoretimes" << 1 << "status" << 1 << "account" << 1 << finalize);
                mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];

                auto find_doc = make_document(kvp("userid", userId));
                auto find_result = user_coll.find_one(find_doc.view(), opts);
                if (!find_result)
                {
                    retCode = 1;
                    LOG_ERROR << "---" << __FUNCTION__ << " 找不到玩家[" << userId << "]";
                    break;
                }

                // 1.1 取出结果
                bsoncxx::document::view view = find_result->view();
                string account = view["account"].get_utf8().value.to_string(); 
                int32_t addscoretimes = view["addscoretimes"].get_int32();
                int32_t status = view["status"].get_int32();
                if (status == (int32_t)ePlayerStatus::op_NO)
                {
                    retCode = 1;
                    break;
                }

                // 2, 取银行卡信息
                mongocxx::collection bank_coll = MONGODBCLIENT["gamemain"]["platform_bank"];
                find_result = bank_coll.find_one(make_document(kvp("agentid", agentid),kvp("banknumber", accountNumber)));
                if (!find_result)
                {
                    retCode = 2;
                    LOG_ERROR << "---" << __FUNCTION__ << " platform_bank 找不到平台银行卡信息[" << agentid << "]";
                    break;
                }

                view = find_result->view();
                bankName = view["bankname"].get_utf8().value.to_string();        //银行卡名称
                accountName = view["bankaccount"].get_utf8().value.to_string();   //银行卡帐户
                bankAddress = view["bankaddress"].get_utf8().value.to_string();   //银行卡支行地址

                // 3, 取平台优惠信息
                mongocxx::collection withdrawcfg_coll = MONGODBCLIENT["gamemain"]["platform_withdrawconfig"];
                find_result = withdrawcfg_coll.find_one(make_document(kvp("agentid", agentid)));
                if (!find_result)
                {
                    retCode = 3;
                    LOG_ERROR << "---" << __FUNCTION__ << " platform_withdrawconfig 找不到平台信息[" << agentid << "]";
                    break;
                }

                view = find_result->view();
                double companyintegral = view["companyintegral"].get_double();   //积分优惠比例
                double companyFavorable = view["companyFavorable"].get_double(); //入款优惠比例(已经取消)
                double giftBonusProp = view["giftBonusProp"].get_double();       //彩金优惠比例
                double washtotal = view["washtotal"].get_double();               //洗码量的倍数
                int32_t giftBonusLimit = view["giftBonusLimit"].get_int32();    //充值赠送彩金的上限
                // 转成分
                giftBonusLimit = giftBonusLimit * COIN_RATE;

                yhAmount = amount * companyFavorable / 100;     //入款优惠
                cjAmount = amount * giftBonusProp / 100;        //彩金优惠

                // 充值赠送彩金的上限值 0 为不限制
                if(giftBonusLimit > 0){
                   cjAmount = cjAmount > giftBonusLimit ? giftBonusLimit : cjAmount;
                }
                // 积分优惠
                double nIntegral =  (amount / 100.0 ) * (companyintegral / 100.0);// * m_lkGame_config.nExChangeRate;
                // 打码量要求(总上分+各种优惠送分) * 倍数要求
                betAmount = (cjAmount + amount + yhAmount) * washtotal;
                // 是否首充
                isFirst = addscoretimes == 0 ? 0 : 1;
                // 充值类型
                ttype = getPayTypeByIdx(ntype);


                string ordernumber ="CK" + GlobalFunc::getBillNo(userId);  /// 订单号  唯一
                //1,插入memberdeposit_order表
                bsoncxx::builder::stream::document builder{};
                builder << "agentid" << b_int32{agentid}; /// 平台ID
                builder << "userid" << b_int64{userId};   /// 玩家ID
                builder << "account" << account;          /// 玩家账号
                builder << "amount" << amount;            /// 汇款金额(分)
                builder << "yhAmount" << yhAmount;        /// 优惠金额(分)  从数据库里取出优惠比例 有就给
                builder << "cjAmount" << cjAmount;        /// 彩金金额(分)  从数据库里取出优惠比例 有就给
                builder << "integral" << b_double{nIntegral};  /// 积分分值(分)  从数据库里取出优惠比例 有就给
                builder << "betAmount" << betAmount;      /// 打码量(分)  取出打码量最低取款条件
                builder << "todayTimes" << todayTimes;    /// 当日次数 
                builder << "isFirst" << isFirst;          /// 是否首存  game_user 上分次数判断
                builder << "times" << addscoretimes;      /// 上分次数
                builder << "payName" << payName;          /// 汇款人姓名  玩家银行卡名称 可选
                builder << "payNumber" << payNumber;      /// 汇款账号  存款信息获取
                builder << "type" << type;                ///  汇款方式  ATM 手机  支付宝 柜台转等
                builder << "ttype" << ttype;              /// 汇款方式 对应名字
                builder << "accountName" << accountName;  /// 收款人姓名 从数据库获取出来写进去
                builder << "accountNumber" << accountNumber; /// 收款账号 从数据库获取出来写进去
                builder << "bankName" << bankName; /// 银行名称
                builder << "bankAddress" << bankAddress; /// 支行地址
                builder << "status" << status; /// 订单状态  0 待审核,1 审核通过,2 审核不通过
                builder << "ordernumber" << ordernumber; /// 订单号  唯一 
                builder << "createuser" << account; /// 创建者  user account
                builder << "createtime" << b_date{chrono::system_clock::now()}; /// 创建时间
                builder << "audituser" << audituser; /// 审核玩家   user account
                builder << "audittime" << b_date{chrono::system_clock::now()}; /// 审核时间  当前时间
                builder << "description" << description; /// 备注 写空窜
                //插入文档
                mongocxx::collection deposit_order_coll = MONGODBCLIENT["gamemain"]["memberdeposit_order"];
                if (!deposit_order_coll.insert_one(builder.view()))
                {
                    LOG_ERROR << "---" << __FUNCTION__ << "---创建充值订单失败----userId[" << userId << "]";
                    retCode = 4;
                    break;
			    }
              
            }while(0);

        }
        catch (exception &e)
        {
            retCode = 5;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}
void HallServer::cmd_get_red_envelope_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RED_ENVELOPE_RAIN_RES;

    // request
    ::HallServer::EvelopeRainMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::EvelopeRainResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();

        int64_t userid = internal_header->userId;
        int32_t agentid = 0;
        int64_t shijian = 0;

        int64_t activeOpenTime = 0;
        int64_t activeEndTime = 0;
        int64_t nestOpenTime = 0;
        int64_t nestEndTime = 0;
        int64_t allScores = 0;
        resp_header->CopyFrom(header);

        auto opts = mongocxx::options::find{};
        opts.projection(document{} <<"agentid" << 1<< finalize);
        mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto find_doc = make_document(kvp("userid", userid));
        auto find_result = user_coll.find_one(find_doc.view(), opts);
        if(find_result)
        {
            bsoncxx::document::view view = find_result->view();
           agentid = view["agentid"].get_int32();
        }    
        //大于开始时间，小于5分钟
        string keyUserInfo = REDIS_USER_BASIC_INFO + to_string(userid);
        //此为春雨

        time_t tx = time(NULL);
        struct tm * local;
        //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
        local = localtime(&tx);
        struct tm t;
        t.tm_year = local->tm_year;
        t.tm_mon = local->tm_mon;
        t.tm_mday = local->tm_mday;
        t.tm_hour = local->tm_hour;
        t.tm_min = local->tm_min;
        t.tm_sec = local->tm_sec;
        //算出当前时间戳,以及计算数据库配置的时间的时间戳
        //这个函数是时间点转换成时间戳
        if(!REDISCLIENT.ExistsUserBasicInfo(userid))
        {
            struct tm ttttt;
            ttttt.tm_year = local->tm_year ;
            ttttt.tm_mon = local->tm_mon;
            ttttt.tm_mday = local->tm_mday;
            ttttt.tm_hour = 0;
            ttttt.tm_min = 0;
            ttttt.tm_sec = 0;
            time_t shijianlong1= mktime(&ttttt);
            int64_t beginStart=(int64_t)shijianlong1;
            redis::RedisValue redisValue;
            redisValue[REDIS_USER_DATE] = to_string(beginStart);
            redisValue[REDIS_USER_TODAY_BET] = to_string(0);
            redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
            redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
            redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
            redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

            redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
            redisValue[REDIS_USER_WINTER_STATUS] = to_string(0);
            REDISCLIENT.hmset(keyUserInfo, redisValue);
        }
        string fields[8] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET,REDIS_USER_TODAY_UPER,REDIS_USER_LEFT_TIMES,
                           REDIS_USER_SPRING_STATUS,REDIS_USER_SUMMER_STATUS,REDIS_USER_FALL_STATUS,
                           REDIS_USER_WINTER_STATUS};
        redis::RedisValue values;
        int ret = REDISCLIENT.hmget(keyUserInfo, fields, 8, values);

        int64_t todayBet = 0;
        int64_t todayUpScore = 0;
        int32_t springStatus =  0;
        int32_t summerStatus =  0;
        int32_t fallStatus =  0;
        int32_t winterStatus =  0;
        if(ret)
        {
            struct tm ttttt;
            ttttt.tm_year = local->tm_year ;
            ttttt.tm_mon = local->tm_mon;
            ttttt.tm_mday = local->tm_mday;
            ttttt.tm_hour = 0;
            ttttt.tm_min = 0;
            ttttt.tm_sec = 0;

            string valuesx;
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_DATE, valuesx);
            int64_t nowtime=stoll(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_TODAY_BET, valuesx);
            todayBet = stoll(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_TODAY_UPER, valuesx);
            todayUpScore =stoi(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_SPRING_STATUS, valuesx);
            springStatus = stoi(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_SUMMER_STATUS, valuesx);
            summerStatus = stoi(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_FALL_STATUS, valuesx);
            fallStatus =  stoi(valuesx);
            REDISCLIENT.GetUserBasicInfo(userid, REDIS_USER_WINTER_STATUS, valuesx);
            winterStatus =  stoi(valuesx);

            time_t shijianlong1= mktime(&ttttt);
            int64_t beginStart=(int64_t)shijianlong1;

            LOG_ERROR<<"  systemtime"<<beginStart;
            if(beginStart!=nowtime)
            {
                redis::RedisValue redisValue;
                redisValue[REDIS_USER_DATE] = to_string(beginStart);
                redisValue[REDIS_USER_TODAY_BET] = to_string(0);
                redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
                redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
                redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
                redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

                redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
                redisValue[REDIS_USER_WINTER_STATUS] = to_string(0);
                REDISCLIENT.hmset(keyUserInfo, redisValue);
            }

        }
        int status = GetRedEnvelopeRainStatus(shijian,agentid,allScores,nestOpenTime , nestEndTime ,activeOpenTime,activeEndTime ,userid);

        string activetimestart,activetimeend,nestopentime,nestendtime;
        activetimestart=InitialConversionNoTime(activeOpenTime);
        activetimeend=InitialConversionNoTime(activeEndTime);
        nestopentime=InitialConversionNoDate(nestOpenTime);
        nestendtime=InitialConversionNoDate(nestEndTime);
        LOG_ERROR<<"activetimestart:"<<activetimestart<<"       activetimeend"<<activetimeend
                    <<"       nestopentime"<<nestopentime
                    <<"       nestendtime"<<nestendtime          <<"     shijian"<<shijian;
        response.set_lefttime(shijian);
        response.set_status(status);
        response.set_activestarttime(activetimestart);
        response.set_activeendtime(activetimeend);
        response.set_allscores(allScores);
        response.set_nestopentime(nestopentime);
        response.set_nestendtime(nestendtime);

        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }


}
void HallServer::WriteEvelopeToMysql(int64_t userid,int32_t agentid,
                           int64_t hbScore,string dbaccount)
{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO play_envelope_record(userid, agentid,welftype, "
//                                           "score,addscore,account,inserttime)"
//                                           "VALUES(?,?,?,?,?,?,?)"));
//        pstmt->setInt64(1,userid);
//        pstmt->setInt(2, agentid);
//        pstmt->setInt(3, 10);
//        pstmt->setInt64(4, hbScore);
//        pstmt->setInt64(5, hbScore);
//        pstmt->setString(6, dbaccount);
//        pstmt->setDateTime(7, InitialConversion(time(NULL)));
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
//抢红包
void HallServer::cmd_get_grab_envelope(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GRAB_ENVELOPE_RES;

    // request
    ::HallServer::GrabEvelopeMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GrabEvelopeResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);
        int32_t status = 0;
        int64_t hbScore = 0;
        int32_t agentid=0;
        string dbaccount="";
        int64_t userid = internal_header->userId;
        string strlinecode="";
        int64_t dbuserscore =0;
        auto opts = mongocxx::options::find{};
        opts.projection(document{} <<"agentid" << 1<<"account"<<1<<"linecode"<<1<<"score"<<1<< finalize);
        mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto find_doc = make_document(kvp("userid", userid));
        auto find_result = user_coll.find_one(find_doc.view(), opts);
        if(find_result)
        {
            bsoncxx::document::view view = find_result->view();
            agentid = view["agentid"].get_int32();
            dbaccount = view["account"].get_utf8().value.to_string();
            strlinecode = view["linecode"].get_utf8().value.to_string();
            dbuserscore = view["score"].get_int64();
        }
        int32_t openTimes=1;
        int64_t needBet = 0;
        WhetherStartRedEnvelopeRain(status,agentid,hbScore,userid,needBet,openTimes);
        //领取红包成功 ,修改玩家分值，插入订单，插入账变记录
        if(status==0)
        {
            openTimes=0;
            //插入vip福利注单表
            bsoncxx::document::value insert_value = document{}
                                                    << "userid" << internal_header->userId
                                                    << "agentid" << agentid
                                                    << "welftype" << 10
                                                    << "score" << (int64_t)hbScore
                                                    << "addscore" << (int64_t)hbScore
                                                    << "account" << dbaccount
                                                    << "inserttime" << bsoncxx::types::b_date(chrono::system_clock::now())
                                                    << finalize;
            //在这里写入注单
            mongocxx::collection coll1 = MONGODBCLIENT["gamemain"]["play_envelope_record"];
            if (!coll1.insert_one(insert_value.view()))
            {
                LOG_ERROR << "play_envelope_record insert exception ";
            }
            // LOG_DEBUG << "Insert Score Change Record:"<<bsoncxx::to_json(insert_value);
            LOG_ERROR<<"mysql插入  play_envelope_record";
            m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteEvelopeToMysql,this, internal_header->userId, agentid,
                                                                           hbScore, dbaccount));
            user_coll.update_one(document{} << "userid" << internal_header->userId << finalize,
                                 document{} << "$inc" << open_document << "score" << (int64_t)hbScore
                                            << close_document
                                            << finalize);
            LOG_INFO << "---,领取红包钱数,并且写分数 hbScore" << hbScore << "    userid" << internal_header->userId;

            // 写帐变记录
            AddScoreChangeRecordToDB(userid, dbaccount, hbScore, agentid, strlinecode, dbuserscore,eUserScoreChangeType::op_activity_hby);

        }
        response.set_hbscores(hbScore);
        response.set_retcode(status); 
        response.set_lefttimes(openTimes);
        response.set_needbet(needBet);
        response.set_scores(dbuserscore+hbScore);

        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 获取充值记录信息
void HallServer::cmd_get_recharge_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_RECORD_RES;

    // request
    ::HallServer::RechargeRecordMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::RechargeRecordResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid         = msg.agentid();    // 代理ID
        int32_t retCode         = 0;                // 结果码
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";
        try
        {
            // 同一层级各个支付渠道优惠
            do
            {
                int32_t count = 0;
                //1, 从用户表中查询出会员层级
                auto opts = mongocxx::options::find{};
                opts.projection(document{} << "type" << 1 << "status" << 1 << "amount" << 1 << "createtime" << 1 << "ordernumber" << 1 <<"ttype" << 1 << finalize);
                opts.sort(document{} << "createtime" << -1 << finalize);
                mongocxx::collection order_coll = MONGODBCLIENT["gamemain"]["memberdeposit_order"];
                mongocxx::cursor cursor = order_coll.find(make_document(kvp("userid", userId)), opts); 
                for(auto &doc : cursor)
                {
                    int32_t status = doc["status"].get_int32();
                    string payType = doc["type"].get_utf8().value.to_string();
                    int64_t money = doc["amount"].get_int64();
                    int64_t createtime = doc["createtime"].get_date();

                    string orderid = doc["ordernumber"].get_utf8().value.to_string();
                    string ttype = doc["ttype"].get_utf8().value.to_string();

                    ::HallServer::RechargeRecordItem* recorditem = response.add_recorditem();
                    recorditem->set_status(status);         //订单状态 0 待支付 1 已支付 2 处理中 3 支付失败 4 取消支付
                    recorditem->set_paytype(atol(payType.c_str()));       //支付类型
                    recorditem->set_money(money);           //订单金额(放大100倍整数返回前端)
                    recorditem->set_orderid(orderid);       //订单号
                    recorditem->set_createtime(createtime/1000); //创建时间
                    recorditem->set_channlename(ttype);//支付方式

                    count++;
                }

                LOG_INFO << "---" << __FUNCTION__ << " 银行订单,count[" << count << "]";
                count = 0;
                //2, 从用户表中查询出会员层级
                opts = mongocxx::options::find{};
                opts.projection(document{} << "paytype" << 1 << "channelname" << 1 << "tradestatus" << 1 << "orderamount" << 1 << "ordertime" << 1 << "ordernumber" << 1 << finalize);
                opts.sort(document{} << "ordertime" << -1 << finalize);
                order_coll = MONGODBCLIENT["gamemain"]["onlinerecharge_order"];
                cursor = order_coll.find(make_document(kvp("userid", userId)), opts); 
                for(auto &doc : cursor)
                {
                    int32_t status = doc["tradestatus"].get_int32();
                    string channelname = doc["channelname"].get_utf8().value.to_string(); //channelname
                    // int32_t payType = doc["paytype"].get_int32(); 

                    int64_t money = doc["orderamount"].get_int64();
                    int64_t createtime = doc["ordertime"].get_date();
                    string orderid = doc["ordernumber"].get_utf8().value.to_string(); 
                   
                    ::HallServer::RechargeRecordItem* recorditem = response.add_recorditem();
                    recorditem->set_status(status);         //订单状态 0 待支付 1 已支付 2 处理中 3 支付失败 4 取消支付
                    // recorditem->set_paytype(0);//atol(payType.c_str()));       //支付类型
                    recorditem->set_channlename(channelname);//支付方式
                    recorditem->set_money(money);           //订单金额(放大100倍整数返回前端)
                    recorditem->set_orderid(orderid);       //订单号
                    recorditem->set_createtime(createtime/1000); //创建时间
                    count++;
                }

                LOG_INFO << "---" << __FUNCTION__ << " 在线订单,count[" << count << "]";

            }
            while(0);
        }
        catch (exception &e)
        {
            retCode = 1;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

void HallServer::cmd_get_third_part_game_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_THIRD_PART_GAME_RES;

    // request
    ::HallServer::GetThirdPartGameMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetThirdPartGameResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t retCode         = 0;                // 结果码
        int32_t agentid         = msg.agentid();    // 代理ID
        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";
        
        do
        {
            if (!cacheThirdPartGame(agentid))
            {
                retCode = 1;
                break;
            }

            for (auto iter = m_GameTypeListMap.begin(); iter != m_GameTypeListMap.end(); iter++)
            {
                ::HallServer::GameTypeList *gametypeitem = response.add_gametypeitem();
                gametypeitem->set_gametype(iter->first);

                for (auto &item : iter->second)
                {
                    ::HallServer::GameItemInfo *gameitem = gametypeitem->add_gameitem();
                    gameitem->set_gameid(item.gameid);//111021001
                    gameitem->set_gamename(item.gamename); //真人百家乐
                    gameitem->set_repairstatus(item.repairstatus);  //维护状态
                    gameitem->set_companycode(item.companyCode);//IG
                    gameitem->set_tid(item.tid);
                  
                    // LOG_INFO << "---" << __FUNCTION__ << " gameid[" << item.gameid << "],gamename[" << item.gamename << "],gamecode[" << item.gamecode << "],gametype[" << iter->first << "]";
                }
            }
           
        } while (0);

        response.set_retcode(retCode); //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 获取分状态(一定要做缓存,用时间判断)
int32_t HallServer::get_user_dividend_status(int32_t agentid,int64_t userId,platform_user_dividends & user_dividends)
{
    int32_t dividend_stauts = 0; //没领取（没有领取记录）
    try
    {
        do
        {
            time_t tx = time(NULL);
            struct tm *local = localtime(&tx);
            int64_t curDate = getZeroTimeStamp(local);
            int64_t monday = (curDate - local->tm_wday  * ONE_DAY_SECOND + ONE_DAY_SECOND ) - 7 * ONE_DAY_SECOND ;//算出上星期一
            LOG_INFO << "---" << __FUNCTION__ << " monday[" << monday << "],curDate[" << curDate << "],tm_wday[" << local->tm_wday << "]";

            char temp[64];
            struct tm * timeSet = gmtime(&monday);
            strftime(temp,50, "当前期数:%Y-%m-%d %H:%M:%S", timeSet);
            LOG_INFO << temp;

            // 查询玩家领取分红状态
            // 1,先读取user_dividends表有没有 ,有则继续第2步,否则返回过期未领取
            // 2,再读取platform_dividend_record表获取
            mongocxx::collection dividend_user_coll = MONGODBCLIENT["gamemain"]["user_dividends"];
            auto dividend_user_doc = dividend_user_coll.find_one(make_document(kvp("agentid", agentid), kvp("userid", userId), kvp("dateTime", monday)));
            if (!dividend_user_doc)
            {
                dividend_stauts = 1; //没领取（没有资格）
                LOG_INFO << "---" << __FUNCTION__ << "用户[" << userId << "],没领取（没有资格）,分红表中没有查到有分红记录";
                break;
            }

            auto doc_view = dividend_user_doc->view();
            user_dividends.selfShares = doc_view["selfShares"].get_double();       //个人份额
            user_dividends.userProfit = doc_view["userProfit"].get_int64();       //个人分红
            user_dividends.dateTime = doc_view["dateTime"].get_int64();           //期号
            user_dividends.moneyPerShare = doc_view["moneyPerShare"].get_double(); //每个份额价值
            user_dividends.level = doc_view["level"].get_int32();                 //每个份额价值
            LOG_INFO << "---" << __FUNCTION__ << " 个人份额[" << user_dividends.selfShares << "],个人分红[" << user_dividends.userProfit << "],每个份额价值[" << user_dividends.moneyPerShare << "],期号[" << user_dividends.dateTime << "]";

            mongocxx::options::find opts = mongocxx::options::find{};
            opts.projection(document{} << "serialId" << 1 << finalize);
            opts.sort(document{} << "serialId" << -1 << finalize);
            opts.limit(1);

            int64_t dateTime_monday = 0;
            mongocxx::collection dividend_rec_coll = MONGODBCLIENT["gamemain"]["platform_dividend_record"];
            mongocxx::cursor rec_list = dividend_rec_coll.find(make_document(kvp("agentid", agentid), kvp("userid", userId)), opts);
            for (auto &rec_doc : rec_list)
            {
                // 找到记录
                dateTime_monday = rec_doc["serialId"].get_int64();
                if (dateTime_monday == monday)
                    dividend_stauts = 4; //已经领取过(显示分红明细)

                timeSet = gmtime(&dateTime_monday);
                strftime(temp,50, "上次领取记录期数:%Y-%m-%d %H:%M:%S", timeSet);
                LOG_INFO << temp;
            }

            if (dividend_stauts == 0)
            {
                LOG_INFO << "---" << __FUNCTION__ << " 没有找到领取记录";
            }
            else
                LOG_INFO << "---" << __FUNCTION__ << " 找到领取记录,领取时间[" << dateTime_monday << "],当前时间[" << monday << "]";

        } while (0);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return dividend_stauts;
}

// 获取百人游戏实时路单信息
void HallServer::cmd_get_route_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROUTE_RES;

    // request
    ::HallServer::GetRouteMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRouteResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t gameid           = msg.gameid();    	    // gameID
        int32_t roomid           = msg.roomid();    	    // roomID
        int32_t tableid          = msg.tableid();       // tableID
		int32_t retCode          = 0;                   // 结果码
		int32_t ipCodein		 = 0;
		string strTableId		 = msg.tableno();
        int32_t currency =0;
        string strCurrency="";
        REDISCLIENT.GetCurrencyUser(userId, REDIS_UID_CURRENCY, strCurrency);
        currency = atol(strCurrency.c_str());


		vector<string> strVec;
		strVec.clear();
		boost::algorithm::split(strVec, strTableId, boost::is_any_of("-"));
		if (strVec.size() == 2)
			ipCodein = stoi(strVec[0]);
		
		int32_t find_agentid = agentid;
		if (agentid == 0) 
		{
			// H5版本时赋值，APP不用出来
			getGameServerIp_agentidVec(userId);
			if(m_userId_agentId_map_getroute.count(userId))
				agentid = m_userId_agentId_map_getroute[userId];
		}
        LOG_INFO << "---"<<" currency:"<<currency << __FUNCTION__ << " find_agentid[" << find_agentid << "],get_agentid[" << agentid << "],userId[" << userId <<"],gameid[" << gameid << "],roomid[" << roomid <<"],tableid[" << tableid <<"]" << "],ipCodein[" << ipCodein << "]";
        try
        {

            if(currency!=0)
            {
                auto inc_doc = make_document(kvp("currency", currency), kvp("gameid", gameid));
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.sort(document{} << "ipcode" << 1 << "roomid" << 1 << "tableid" << 1 << finalize);
                mongocxx::collection route_coll = MONGODBCLIENT["gamemain"]["room_route_record"];
                mongocxx::cursor rec_list = route_coll.find(inc_doc.view(), opts);
                for (auto &rec_doc : rec_list)
                {
                    string details = rec_doc["details"].get_utf8().value.to_string();
                    int32_t counttime = rec_doc["time"].get_int32();
                    int32_t gamestatus = rec_doc["gamestatus"].get_int32();
                    int32_t status = rec_doc["status"].get_int32();
                    int32_t goodroutetype = rec_doc["goodroutetype"].get_int32();  // 好路类型
                    int32_t ipcode = rec_doc["ipcode"].get_int32();             //ipcode
                    int32_t tid = rec_doc["tableid"].get_int32();               //tableid
                    int32_t rid = rec_doc["roomid"].get_int32();                //roomid
                    int32_t gameCurrency = rec_doc["currency"].get_int32();     //currency
                    string roomIp = rec_doc["ip"].get_utf8().value.to_string(); //ip，游戏服的IP地址信息
                    string routetypename = rec_doc["routetypename"].get_utf8().value.to_string(); //好路类型名称

                    //IP为空，过滤
                    if (ipcode == 0 || roomIp == "")

                        continue;

                    //组装Key值
                    int32_t keyid = (rid << 14) | ipcode << 4 | tid;
                    // 有值则不重写,若游戏服IP变化，则会新增加新的Key值,所以不用每次都更新它
                    if (!m_tableIpMap.count(keyid)){
                        m_tableIpMap[keyid] = roomIp;
                        LOG_INFO << "--- keyid[" << keyid << "],tableid[" << tid << "],roomid[" << rid << "],roomIp[" << roomIp << "]";
                    }

                    //请求所有桌子时,桌子状态为禁用,就发这个桌子信息给前端
                    if (tableid == -1 && status == SERVER_STOPPED)
                        continue;

                    // 上线时要删除的日志
                    //LOG_INFO << "--- tableid[" << tid << "],status[" << status << "],gamestatus[" << gamestatus << "],counttime[" << counttime << "],details[" << details << "]";
                    if (tableid == -1)
                    {
                        if (roomid == -1)
                        {
                            // 路单详情
                            ::HallServer::RouteItem *routeItem = response.add_routeitem();

                            routeItem->set_details(details);
                            routeItem->set_tableid(tid);
                            routeItem->set_time(counttime);
                            routeItem->set_gamestatus(gamestatus);
                            routeItem->set_status(status);
                            routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                            routeItem->set_roomid(rid);
                        }
                        else if (roomid == rid)
                        {
                            // 路单详情
                            ::HallServer::RouteItem *routeItem = response.add_routeitem();

                            routeItem->set_details(details);
                            routeItem->set_tableid(tid);
                            routeItem->set_time(counttime);
                            routeItem->set_gamestatus(gamestatus);
                            routeItem->set_status(status);
                            routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                            routeItem->set_roomid(rid);
                        }
                    }
                    else if (tableid == tid && roomid == rid && ipcode == ipCodein)
                    {
                        // 路单详情
                        ::HallServer::RouteItem *routeItem = response.add_routeitem();

                        routeItem->set_details(details);
                        routeItem->set_tableid(tid);
                        routeItem->set_time(counttime);
                        routeItem->set_gamestatus(gamestatus);
                        routeItem->set_status(status);
                        routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                        routeItem->set_roomid(rid);
                    }

                    // 填充好路结果
                    if( goodroutetype > 0 )
                    {
                        if (status != SERVER_RUNNING)
                            continue;
                        // 好路结果
                        ::HallServer::GoodRouteTypeItme *routetypeitem = response.add_routetypeitem();

                        routetypeitem->set_tableid(tid);
                        routetypeitem->set_routetype(goodroutetype);
                        routetypeitem->set_routetypename(routetypename);
                        routetypeitem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                        routetypeitem->set_roomid(rid);
                    }

                }

            }
            else
            {


                //(tableid > -1) ? make_document(kvp("agentid", agentid), kvp("ipcode", ipCode), kvp("gameid", gameid), kvp("roomid", roomid), kvp("tableid", tableid)) :
                // (tableid > -1) 请求单个，否则请求所有
                auto inc_doc = make_document(kvp("agentid", agentid), kvp("gameid", gameid));
                mongocxx::options::find opts = mongocxx::options::find{};
                opts.sort(document{} << "ipcode" << 1 << "roomid" << 1 << "tableid" << 1 << finalize);
                //if (roomid != -1) //查询大厅单个房间所有的桌子
                //{
                //	inc_doc = make_document(kvp("agentid", agentid), kvp("gameid", gameid), kvp("roomid", roomid));
                //	opts.sort(document{} << "ipcode" << 1 << "tableid" << 1 << finalize);
                //}
                //else //排序所有个房间所有的桌子
                //{
                //	opts.sort(document{} << "ipcode" << 1 << "roomid" << 1 << "tableid" << 1 << finalize);
                //}
                // 读取配置表
                mongocxx::collection route_coll = MONGODBCLIENT["gamemain"]["room_route_record"];
                mongocxx::cursor rec_list = route_coll.find(inc_doc.view(), opts);
                for (auto &rec_doc : rec_list)
                {
                    string details = rec_doc["details"].get_utf8().value.to_string();
                    int32_t counttime = rec_doc["time"].get_int32();
                    int32_t gamestatus = rec_doc["gamestatus"].get_int32();
                    int32_t status = rec_doc["status"].get_int32();
                    int32_t goodroutetype = rec_doc["goodroutetype"].get_int32();  // 好路类型
                    int32_t ipcode = rec_doc["ipcode"].get_int32();             //ipcode
                    int32_t tid = rec_doc["tableid"].get_int32();               //tableid
                    int32_t rid = rec_doc["roomid"].get_int32();                //roomid
                    int32_t gameCurrency = rec_doc["currency"].get_int32();     //currency
                    string roomIp = rec_doc["ip"].get_utf8().value.to_string(); //ip，游戏服的IP地址信息
                    string routetypename = rec_doc["routetypename"].get_utf8().value.to_string(); //好路类型名称

                    if(currency!=gameCurrency)
                    {
                        continue;
                    }
                    //IP为空，过滤
                    if (ipcode == 0 || roomIp == "")
                        continue;

                    //组装Key值
                    int32_t keyid = (rid << 14) | ipcode << 4 | tid;
                    // 有值则不重写,若游戏服IP变化，则会新增加新的Key值,所以不用每次都更新它
                    if (!m_tableIpMap.count(keyid)){
                        m_tableIpMap[keyid] = roomIp;
                        LOG_INFO << "--- keyid[" << keyid << "],tableid[" << tid << "],roomid[" << rid << "],roomIp[" << roomIp << "]";
                    }

                    //请求所有桌子时,桌子状态为禁用,就发这个桌子信息给前端
                    if (tableid == -1 && status == SERVER_STOPPED)
                        continue;

                    // 上线时要删除的日志
                    //LOG_INFO << "--- tableid[" << tid << "],status[" << status << "],gamestatus[" << gamestatus << "],counttime[" << counttime << "],details[" << details << "]";
                    if (tableid == -1)
                    {
                        if (roomid == -1)
                        {
                            // 路单详情
                            ::HallServer::RouteItem *routeItem = response.add_routeitem();

                            routeItem->set_details(details);
                            routeItem->set_tableid(tid);
                            routeItem->set_time(counttime);
                            routeItem->set_gamestatus(gamestatus);
                            routeItem->set_status(status);
                            routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                            routeItem->set_roomid(rid);
                        }
                        else if (roomid == rid)
                        {
                            // 路单详情
                            ::HallServer::RouteItem *routeItem = response.add_routeitem();

                            routeItem->set_details(details);
                            routeItem->set_tableid(tid);
                            routeItem->set_time(counttime);
                            routeItem->set_gamestatus(gamestatus);
                            routeItem->set_status(status);
                            routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                            routeItem->set_roomid(rid);
                        }
                    }
                    else if (tableid == tid && roomid == rid && ipcode == ipCodein)
                    {
                        // 路单详情
                        ::HallServer::RouteItem *routeItem = response.add_routeitem();

                        routeItem->set_details(details);
                        routeItem->set_tableid(tid);
                        routeItem->set_time(counttime);
                        routeItem->set_gamestatus(gamestatus);
                        routeItem->set_status(status);
                        routeItem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                        routeItem->set_roomid(rid);
                    }

                    // 填充好路结果
                    if( goodroutetype > 0 )
                    {
                        if (status != SERVER_RUNNING)
                            continue;
                        // 好路结果
                        ::HallServer::GoodRouteTypeItme *routetypeitem = response.add_routetypeitem();

                        routetypeitem->set_tableid(tid);
                        routetypeitem->set_routetype(goodroutetype);
                        routetypeitem->set_routetypename(routetypename);
                        routetypeitem->set_tableno(to_string(ipcode) + "-" + to_string(tid + 1));
                        routetypeitem->set_roomid(rid);
                    }

                }

            }
        }
        catch (exception &e)
        {
            retCode = 2;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}



/*
GetDividendLevelMessage
status字段意义：
0：表示可领取，显示“领取”按钮;
1：没领取（没有资格），显示“过期未领”按钮,
2：没领取（玩家真没有操作领取），显示“过期未领”按钮,
3：过期未领，显示“过期未领”按钮,
4：表示已经领取，显示“分红明细”;
*/
// 获取分红等级信息
void HallServer::cmd_get_dividend_level(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_DIVIDEND_LEVEL_RES;

    // request
    ::HallServer::GetDividendLevelMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetDividendLevelResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 0;                   // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";
        try
        {
            do
            {
                // 找到缓存信息(分红会员等级信息说明)
                if (m_dividendMap.find(agentid) == m_dividendMap.end())
                {
                    vector<dividendItem> dividendinfo;
                    mongocxx::collection dividend_coll = MONGODBCLIENT["gamemain"]["platform_dividend_level"];
                    mongocxx::cursor info = dividend_coll.find(make_document(kvp("agentid", agentid)));
                    for (auto &doc : info)
                    {
                        int32_t level = doc["level"].get_int32();
                        int32_t share = doc["share"].get_int32();
                        string alias = doc["alias"].get_utf8().value.to_string();
                        string name = doc["name"].get_utf8().value.to_string();

                        dividendItem itemTmp;
                        itemTmp.level = level;
                        itemTmp.share = share;
                        itemTmp.title = alias;
                        itemTmp.name = name;
                        dividendinfo.push_back(itemTmp);
                        LOG_INFO << " >>> agentid[" << agentid << "],dividendItem: " << level << ", " << share << ", " << name<< ", " << alias;
                    }
                    
                    // 缓存到内存
                    if(dividendinfo.size() == 0)
                    {
                        retCode = 1;
                        break;
                    }
                
                    MutexLockGuard lock(m_dividend_MutexLock);
                    m_dividendMap[agentid] = dividendinfo;
                }

                auto it = m_dividendMap.find(agentid);
                if (it != m_dividendMap.end())
                {
                    for (auto item : it->second)
                    {
                        ::HallServer::DividendLvlItem *dividendItem = response.add_item();
                        dividendItem->set_level(item.level);
                        dividendItem->set_share(item.share);
                        dividendItem->set_info(item.title);
                        dividendItem->set_name(item.name);
                    }
                }

                // 分红功能配置表
                if (m_dividendCfgMap.find(agentid) == m_dividendCfgMap.end())
                {
                    // 读取配置表
                    mongocxx::collection dividend_config_coll = MONGODBCLIENT["gamemain"]["platform_dividend_config"];
                    auto cfg_doc = dividend_config_coll.find_one(make_document(kvp("agentid", agentid), kvp("status", 0)));
                    if (!cfg_doc)
                    {
                        /* code */
                        retCode = 2;
                        LOG_ERROR << "---" << __FUNCTION__ << "  没有找到代理[" << agentid << "]领取分红红配置功能";
                        break;
                    }

                    // 是否开启
                    auto cfg_view = cfg_doc->view();
                    int32_t status = cfg_view["status"].get_int32();
                    if(status != 0){
                        LOG_WARN << "---" << __FUNCTION__ << " 代理[" << agentid << "] 没有开启领取分红配置功能";
                        retCode = 3;
                        break;
                    }

                    // 读取配置
                    dividendCfg diviCfg = {0};
                    diviCfg.limit = cfg_view["minPerformance"].get_int64()/COIN_RATE;
                    diviCfg.rate = cfg_view["dividendScale"].get_double();//分红百分比
                    {
                        MutexLockGuard lock(m_dividend_MutexLock);
                        m_dividendCfgMap[agentid] = diviCfg;
                    }

                    // 抽取费率(默认往上五级代理，超出或者不足则用map返回的值统为0判断)
                    vector<tollscaleCfg> vec_toll;
                    vec_toll.clear();
                    double tollscale = cfg_view["tollscale"].get_double();
                    auto details = cfg_view["details"].get_array();
                    for (auto &itemDoc : details.value)
                    {
                        int32_t level = itemDoc["level"].get_int32();
                        int32_t keyId = (agentid << 8) | level;
                        // 等级对应值
                        tollscaleCfg tempCfg = {0};
                        tempCfg.tollscale = tollscale;
                        tempCfg.level = level;
                        tempCfg.scale = itemDoc["scale"].get_double();
                        tempCfg.levelname = itemDoc["levelname"].get_utf8().value.to_string();
                        // 存放对应的配置
                        m_dividend_receive_configMap[keyId] = tempCfg;
                        // 存放到容器
                        vec_toll.push_back(tempCfg);

                        LOG_INFO << "---" << __FUNCTION__ << " level[" << level << "],levelname[" << tempCfg.levelname <<"],scale[" << tempCfg.scale <<"],scale[" << tempCfg.tollscale <<"]";
                    }
                    
                    {
                        MutexLockGuard lock(m_dividend_MutexLock);
                        // 按代理存放到对应容器
                        m_dividend_fee_vectorMap[agentid] = vec_toll;
                    }
                }

                auto iter = m_dividendCfgMap.find(agentid);
                if (iter != m_dividendCfgMap.end())
                {
                    response.set_limit(iter->second.limit);
                    response.set_rate(iter->second.rate);
                    LOG_INFO << "---" << __FUNCTION__ << " limit[" << iter->second.limit << "],rate[" << iter->second.rate <<"]";
                }

                // 返回分红手续费收取规则
                auto iterFee = m_dividend_fee_vectorMap.find(agentid);
                if (iterFee != m_dividend_fee_vectorMap.end())
                {
                    int32_t tollscale = 0;
                    for (auto item : iterFee->second)
                    {
                        ::HallServer::DividendFeeItem *dividendFeeItem = response.add_feeitem();
                        dividendFeeItem->set_level(item.level);
                        dividendFeeItem->set_scale(item.scale);
                        dividendFeeItem->set_levelname(item.levelname);

                        // 每个Item中存储了相同的tollscale
                        tollscale = item.tollscale;
                    }

                    // 收取手续费比率
                    response.set_tollscale(tollscale);
                }

            } while (0);

            // 
            platform_user_dividends user_dividends;
            int32_t dividend_stauts = get_user_dividend_status(agentid,userId,user_dividends);
         
            // 1 没领取（没有资格）;2 没领取（玩家真没有操作领取）;3 领取过期;4 已经领取过(分红明细)
            response.set_status(dividend_stauts);        //领取结果码

            LOG_INFO << "---" << __FUNCTION__ << " 领取状态[" << dividend_stauts << "](0 可领取(显示领取); 1 没领取（没有资格）;2 没领取（玩家真没有操作领取）;3 领取过期;4 已经领取过(分红明细))";

        }
        catch (exception &e)
        {
            retCode = 4;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 领取分红信息
void HallServer::cmd_get_dividend(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_DIVIDEND_RES;

    // request
    ::HallServer::GetDividendMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetDividendResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t agentid          = msg.agentid();    	// 代理ID
        int32_t retCode          = 0;                   // 结果码

        LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId <<"]";
        try
        {
            do
            {
                platform_user_dividends user_dividends;
                int32_t dividend_stauts = get_user_dividend_status(agentid,userId,user_dividends);
                if(dividend_stauts != 0)
                {
                    retCode = 1; //状态错误，已经领取了
                    LOG_ERROR << "---" << __FUNCTION__ << " 状态错误，已经领取了";
                    break;
                }

                // 获取对应代理等级比例
                if( m_dividend_fee_vectorMap[agentid].size()  == 0 ) 
                {
                    retCode = 2; //没有配置好领取规则
                    LOG_ERROR << "---" << __FUNCTION__ <<" 没有配置好领取规则" ;
                    break;
                }

                if(user_dividends.userProfit > 0)
                {
                    string account ,lineCode;
                    int64_t selfRealProfit = 0, superior_uid = 0,beforechange = 0;
                    mongocxx::options::find opts = mongocxx::options::find{};
			        opts.projection(bsoncxx::builder::stream::document{} << "superior" << 1 <<"account" << 1 <<"linecode" << 1 << "score" << 1 << bsoncxx::builder::stream::finalize);
 
                    mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
                    auto find_doc = user_coll.find_one(make_document(kvp("userid", userId)),opts);
                    if(!find_doc){
                        retCode = 3; //玩家不存在，怎么可能呢
                        break;
                    }
                    
                    // 取值
                    auto update_view = find_doc->view();
                    superior_uid = update_view["superior"].get_int64();
                    account = update_view["account"].get_utf8().value.to_string();
                    lineCode = update_view["linecode"].get_utf8().value.to_string();
                    beforechange = update_view["score"].get_int64(); 

                    // 手续费收费层级
                    int32_t feeItemCount = 0;
                    {
                        MutexLockGuard lock(m_dividend_MutexLock);
                        feeItemCount = m_dividend_fee_vectorMap[agentid].size();
                    }

                    stringstream details;
                    int32_t pickCount = 0;
                    if(superior_uid > 0 && feeItemCount > 0) {
                        details << "层级:"<<  feeItemCount << ",";
                        pickDividendfee(superior_uid,agentid,user_dividends.userProfit,pickCount,selfRealProfit,feeItemCount,details);//提取的分红值,默认最大5级
                    }

                    // 真正自己拿到的分红额
                    int64_t userProfit = user_dividends.userProfit - selfRealProfit;
                    userProfit = userProfit < 0 ? 0 : userProfit;
                 
                    // 更新玩家
                    auto update_doc = user_coll.update_one(make_document(kvp("userid", userId)), make_document(kvp("$inc", make_document(kvp("score", userProfit)))));
                    if (update_doc)
                    {
                        // 返回玩家分数
                        response.set_curscore(beforechange + userProfit);
                        response.set_beforescore(beforechange);

                        // 写领取记录
                        auto insertValue = bsoncxx::builder::stream::document{}
                                    << "userid" << b_int64{userId}                      //玩家ID
                                    << "agentid" << agentid                             //代理ID
                                    << "account" << account                             //玩家账号
                                    << "serialId" <<  user_dividends.dateTime           //期号
                                    << "personalshare" << user_dividends.selfShares     //个人份额
                                    << "persharevalue" << user_dividends.moneyPerShare  //每个份额价值
                                    << "dividends" << userProfit                        //个人分红
                                    << "fee" << selfRealProfit                          //手续费
                                    << "createtime" << b_date{chrono::system_clock::now()}  //创建时间
                                    << "details" << details.str()                         //手续费详情
                                    << bsoncxx::builder::stream::finalize;
                        // 写记录
                        MONGODBCLIENT["gamemain"]["platform_dividend_record"].insert_one(insertValue.view());

                        //写帐变记录，保存玩家分更改记录
                        AddScoreChangeRecordToDB(userId, account,userProfit, agentid, lineCode, beforechange, eUserScoreChangeType::op_dividend);

                        LOG_DEBUG << "手续费层级["<< feeItemCount<<"],总分红[" << user_dividends.userProfit << "],分剩自己[" << userProfit << "],分摊部分[" << selfRealProfit << "],更新前[" << beforechange <<"],更新后[" << (beforechange + userProfit) <<"]";
                    }
                }

            } while (0);

        }
        catch (exception &e)
        {
            retCode = 4;
            LOG_ERROR << " >>> Exception: " << e.what();
        }

        response.set_retcode(retCode);        //结果码
        size_t len = response.ByteSize();
        vector<unsigned char> data(len+sizeof(internal_prev_header)+sizeof(Header));
        if(response.SerializeToArray(&data[sizeof(internal_prev_header)+sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }
    }
}

// 代理团队抽取佣金手续费计算
void HallServer::pickDividendfee(int64_t uid,int32_t agentid,int64_t _userProfit,int32_t & pickCount,int64_t & selfProfit,int32_t feeItemCount,stringstream & ss)
{
	try
	{
        // 默认最大5级
        if(pickCount++ > feeItemCount){
            LOG_DEBUG << "---递归次数["<< pickCount <<"],递归结束";
            return;
        }

        if( feeItemCount == 0 )
        {
			LOG_ERROR << "---该抽取手续费等级不存在[" << pickCount << "],递归结束,uid[" << uid << "]";
            return;
        }

        int32_t keyId = (agentid << 8) | pickCount;

        // 对应等级分配比例
        tollscaleCfg tollsCfg = m_dividend_receive_configMap[keyId];
        // 第[pickCount]等级 分得手续费
        int64_t profit = (_userProfit * tollsCfg.tollscale * 0.01) * tollsCfg.scale * 0.01;//提取的分红值,tollscale也为 *0.01，但

        ss << pickCount << " 级 " << uid << ":" << profit <<";";
        
        LOG_INFO << "---递归玩家[" << uid << "],费用层级(次数)[" << pickCount << "],分红值[" << _userProfit << "],手续费用[" << profit << "],百分比[" << tollsCfg.tollscale << "],占比[" << tollsCfg.scale << "]";

        // 当手续费设置为0时则中断
        if( profit <= 0 )
        {
            LOG_ERROR << "---递归次数["<< profit <<"],当手续费设置为0时则中断递归";
            return;
        }

        // 更新玩家
		mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
		auto update_doc = user_coll.find_one_and_update(make_document(kvp("userid", uid)), make_document(kvp("$inc", make_document(kvp("score", profit)))));
		if (!update_doc)
		{
			LOG_ERROR << "---递归找不到玩家[" << uid << "],增加费用失败，费用[" << profit <<"],费用等级[" << pickCount << "]";
			return;
		}
        // 取值
		auto update_view = update_doc->view();
        string account = update_view["account"].get_utf8().value.to_string();
        string lineCode = update_view["linecode"].get_utf8().value.to_string();
        int64_t beforechange = update_view["score"].get_int64(); 

        //写帐变记录，保存玩家分更改记录
        if(!AddScoreChangeRecordToDB(uid, account, profit, agentid, lineCode, beforechange, eUserScoreChangeType::op_dividend_fee))
            LOG_ERROR << "---账变记录失败,userId[" << uid << "";

        // 统计分出去的手续费
        selfProfit += profit;

        // 遍历下一个玩家
		if (update_view["superior"])
		{
			auto superiorid = update_view["superior"].get_int64();
            if(superiorid > 0)
			    pickDividendfee(superiorid,agentid,_userProfit,pickCount,selfProfit,feeItemCount,ss);
            else
                LOG_DEBUG << "---递归次数["<< pickCount <<"],最后递归玩家[" << uid << "],递归结束";
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << "递归失败----" << e.what();
	}
}

// 缓存代理游戏服IP
bool HallServer::getGameServerIp(int32_t agentid,string & gameserverIp)
{
    if (m_proxy_gameServer_ip.find(agentid) == m_proxy_gameServer_ip.end())
    {
        try
        {

            MutexLockGuard lock(m_proxy_game_ip);
            m_proxy_gameServer_ip.clear();
            proxyIPVec.clear();
            mongocxx::collection proxy_game_coll = MONGODBCLIENT["gamemain"]["proxy_gameserver"];
            mongocxx::cursor info = proxy_game_coll.find({});

            for (auto &doc : info)
            {

                int32_t proxyid=doc["agentid"].get_int32();
                string allipadress = doc["ipadress"].get_utf8().value.to_string();
                if(proxyid==agentid)
                {

                    int32_t status = doc["status"].get_int32();
                    if (status == eDefaultStatus::op_ON)
                    {
                        m_proxy_gameServer_ip[agentid].push_back(allipadress);
                        LOG_INFO << "---" << __FUNCTION__ << ",agentid[" << agentid << "],size[" << m_proxy_gameServer_ip.size() << "],ipadress[" << allipadress << "]";
                    }
                }

                proxyIPVec.push_back(allipadress);
                for (int i=0;i<proxyIPVec.size();i++)
                {

                     LOG_INFO << "分代理的ip是:"<<proxyIPVec[i];
                }

            } 
        }
        catch (exception excep)
        {

            LOG_ERROR << "---" << __FUNCTION__ << " 读取游戏服配置出错,agentid信息[" << agentid << "],what[" << excep.what() << "]";
            return false;
        }
    }

    // 找缓存信息
    auto proxyip = m_proxy_gameServer_ip.find(agentid);
    if(proxyip==m_proxy_gameServer_ip.end())
    {
		LOG_ERROR << "---" << __FUNCTION__ << " 缓存信息没有找到游戏服,agentid信息[" << agentid << "],gameserverIp[" << gameserverIp << "]";
         return false;
    }
    if (proxyip->second.size()<=0)
    {
        LOG_ERROR << "-------------------------5-5";
        LOG_ERROR << "---" << __FUNCTION__ << " 找到缓存信息没有找到游戏服,agentid信息[" << agentid << "],gameserverIp[" << gameserverIp << "]";
        return false;
    }

    // 有Game Server IP
    int index = GlobalFunc::RandomInt64(1, proxyip->second.size());
    gameserverIp = proxyip->second[index - 1];
    LOG_INFO << "---" << __FUNCTION__ << " agentid信息[" << agentid << "],gameserverIp[" << gameserverIp << "]";

    return true;
}
// 缓存币种游戏服务器
bool HallServer::getGameServer_MoneyType(int32_t moneytype,string & gameserverIp)
{
    if (m_moneyType_gameServer_ip.find(moneytype) == m_moneyType_gameServer_ip.end())
    {
        try
        {

            MutexLockGuard lock(m_currnecy_game_ip);
            m_moneyType_gameServer_ip.clear();
            moneyTypeVec.clear();
            mongocxx::collection currency_game_coll = MONGODBCLIENT["gamemain"]["currency_gameserver"];
            mongocxx::cursor info = currency_game_coll.find({});

            for (auto &doc : info)
            {

                int32_t currency=doc["currency"].get_int32();
                string allipadress = doc["ipadress"].get_utf8().value.to_string();
                if(currency==moneytype)
                {

                    int32_t status = doc["status"].get_int32();
                    if (status == eDefaultStatus::op_ON)
                    {
                        m_moneyType_gameServer_ip[moneytype].push_back(allipadress);
                        LOG_INFO << "---" << __FUNCTION__ << ",currency[" << moneytype << "],size[" << m_moneyType_gameServer_ip.size() << "],ipadress[" << allipadress << "]";
                    }
                }

                moneyTypeVec.push_back(allipadress);
                for (int i=0;i<moneyTypeVec.size();i++)
                {

                     LOG_INFO << "分币种的ip是:"<<moneyTypeVec[i];
                }

            }
        }
        catch (exception excep)
        {

            LOG_ERROR << "---" << __FUNCTION__ << " 读取游戏服配置出错,moneytype信息[" << moneytype << "],what[" << excep.what() << "]";
            return false;
        }
    }

    // 找缓存信息
    auto currencyip = m_moneyType_gameServer_ip.find(moneytype);
    if(currencyip==m_moneyType_gameServer_ip.end())
    {
        LOG_ERROR << "---" << __FUNCTION__ << " 缓存信息没有找到游戏服,moneytype信息[" << moneytype << "],gameserverIp[" << gameserverIp << "]";
         return false;
    }
    if (currencyip->second.size()<=0)
    {
        LOG_ERROR << "-------------------------5-5";
        LOG_ERROR << "---" << __FUNCTION__ << " 找到缓存信息没有找到游戏服,moneytype信息[" << moneytype << "],gameserverIp[" << gameserverIp << "]";
        return false;
    }

    // 有Game Server IP
    int index = GlobalFunc::RandomInt64(1, currencyip->second.size());
    gameserverIp = currencyip->second[index - 1];
    LOG_INFO << "---" << __FUNCTION__ << " moneytype信息[" << moneytype << "],gameserverIp[" << gameserverIp << "]";

    return true;
}
// 缓存代理游戏服代理id
bool HallServer::getGameServerIp_agentidVec(int64_t userid)
{
	if (0 == m_gameserverIP_AgentIdVec.size())// 玩家在独享服务代理表中找到
	{
		try
		{
			MutexLockGuard lock(m_proxy_game_ip);
			m_userId_agentId_map_getroute.clear();
			mongocxx::collection proxy_game_coll = MONGODBCLIENT["gamemain"]["proxy_gameserver"];
			mongocxx::cursor info = proxy_game_coll.find({});
			
			for (auto &doc : info)
			{
				int32_t proxyid = doc["agentid"].get_int32();
				int32_t status = doc["status"].get_int32();
				if (status == eDefaultStatus::op_ON)
				{
					int32_t proxyidEx = doc["agentidEx"].get_int32();
					if (proxyidEx > 0)
					{
						m_gameserverIP_AgentIdVec[proxyid] = proxyidEx;
					}
					else
					{
						m_gameserverIP_AgentIdVec[proxyid] = proxyid;
					}
					LOG_INFO << "分代理独享服务器 agentid:" << proxyid << " proxyidEx:" << m_gameserverIP_AgentIdVec[proxyid];
				}
			}
		}
		catch (exception excep)
		{

			LOG_ERROR << "---" << __FUNCTION__ << " 读取分代理独享服务器游戏服配置出错,userid信息[" << userid << "],what[" << excep.what() << "]";
			return false;
		}
	}

	//记录代理独享服务器时玩家对应的代理
	int32_t agentId = m_userId_agentId_map[userid];
	auto item = m_gameserverIP_AgentIdVec.find(agentId);
	if (item != m_gameserverIP_AgentIdVec.end())// 玩家在独享服务代理表中找到
	{
		m_userId_agentId_map_getroute[userid] = m_gameserverIP_AgentIdVec[agentId];
	}
	else
	{
		m_userId_agentId_map_getroute[userid] = 0;
	}

	return true;
}

// 
bool HallServer::cacheThirdPartGame(int32_t agentid)
{
    // 找到缓存信息
    if (m_GameTypeListMap.size() > 0)
        return true;
    
    LOG_INFO << "---" << __FUNCTION__ << " agentid信息[" << agentid << "]";

    try
    {
        MutexLockGuard lock(m_thirdpart_game_Lock);
        m_GameTypeListMap.clear();

        mongocxx::collection third_part_game_coll = MONGODBCLIENT["gameconfig"]["third_part_game_config"];
        mongocxx::cursor info = third_part_game_coll.find({});
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
            if (status == eDefaultStatus::op_ON)
            {
                int32_t gametype = doc["gametype"].get_int32();
                // 添加
                GameItemInfo gameItem;
                gameItem.repairstatus = abs(doc["repairstatus"].get_int32());
                gameItem.gameid = doc["gameid"].get_int32();
                gameItem.gamecode = doc["gamecode"].get_utf8().value.to_string();
                gameItem.gamename = doc["gamename"].get_utf8().value.to_string();
                gameItem.companyNo = doc["companyNo"].get_int32();
                gameItem.companyCode = doc["companyCode"].get_utf8().value.to_string();
                gameItem.companyName = doc["companyName"].get_utf8().value.to_string();
                gameItem.tid = getTidByGameid(gameItem.gameid);

                // 分公司进行装游戏项目
                m_GameTypeListMap[gametype].push_back(gameItem);

                LOG_INFO << "---GameItemInfo[" << gameItem.companyNo << "],size[" << m_GameTypeListMap.size() << "],companyName[" << gameItem.companyName << "],companyCode[" << gameItem.companyCode << "]";
            }
        }
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}
void HallServer::WriteIntegralToMysql(int64_t userid,string account,
                          int32_t agentid,int32_t changetype
                         ,int64_t incrintegral,int64_t before_integral
                         ,string  reason)
{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        //shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO user_integral_record(userid, agentid,account, "
//                                           "changetype,changeamount,beforechange,afterchange,"
//                                           "logdate,changereason)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?)"));
//        pstmt->setInt64(1,userid);
//        LOG_ERROR<<"mysql插入1  "<<userid;
//        pstmt->setInt(2, agentid);
//        LOG_ERROR<<"mysql插入2  "<<agentid;
//        pstmt->setString(3, account);
//        LOG_ERROR<<"mysql插入3  "<<account;
//        pstmt->setInt(4, int32_t(changetype));
//        LOG_ERROR<<"mysql插入4  "<<int32_t(changetype);
//        pstmt->setInt64(5, incrintegral);
//        LOG_ERROR<<"mysql插入5  "<<incrintegral;
//        pstmt->setInt64(6, before_integral);
//        LOG_ERROR<<"mysql插入6  "<<before_integral;
//        pstmt->setInt64(7, before_integral  + incrintegral);
//        LOG_ERROR<<"mysql插入7  "<<before_integral  + incrintegral;
//        pstmt->setDateTime(8, InitialConversion(time(NULL)));
//        LOG_ERROR<<"mysql插入8  "<<before_integral  + incrintegral;
//        pstmt->setString(9, reason);
//        LOG_ERROR<<"mysql插入9  "<<reason;
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//        LOG_ERROR<<"mysql插入异常  error code: "<< e.getErrorCode();
//    }
}
bool HallServer::JiFenChangeRecordToDB(int64_t userId,string account,int32_t agentId,int64_t incrintegral,int32_t before_integral,eJiFenChangeType changetype,string reason)
{
    bool bSuccess = false;

    auto insertValue = bsoncxx::builder::stream::document{}
                       << "userid" << userId                                   //玩家ID
                       << "agentid" << agentId                                 //代理ID
                       << "account" << account                                 //玩家账号
                       << "changetype" << (int32_t)changetype                            //账变类型
                       << "changeamount" << incrintegral                       //账变额度signInfo.giftValue
                       << "beforechange" << before_integral                     //变前额度
                       << "afterchange" << b_int32{before_integral  + incrintegral}               //变后额度
                       << "logdate" << b_date{chrono::system_clock::now()}      //账变时间
                       << "changereason" << reason                              //账变原因
                       << bsoncxx::builder::stream::finalize;

    // LOG_DEBUG << "Insert Score Change Record:"<<bsoncxx::to_json(insert_value);
    LOG_ERROR<<"mysql插入  user_integral_record";

    m_game_sqldb_thread->getLoop()->runInLoop(bind(&HallServer::WriteIntegralToMysql,this, userId, account,agentId, (int32_t)changetype, incrintegral, before_integral, reason));
    try
    {
        mongocxx::collection integral_coll = MONGODBCLIENT["gamemain"]["user_integral_record"];
        if (!integral_coll.insert_one(insertValue.view()))
            LOG_INFO << "---账变记录失败,userId[" << userId << ",reason[" << reason << "]";
        else
             bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}





void HallServer::test()
{
    // return;
    int64_t userId = 0;
    int32_t retCode = 0;      // 结果码
    int32_t agentid = 111228; // 代理ID
    LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";

    LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "]";
    try
    {
        do
        {
            // 找到缓存信息
            if (m_dividendMap.find(agentid) == m_dividendMap.end())
            {
                vector<dividendItem> dividendinfo;
                mongocxx::collection dividend_coll = MONGODBCLIENT["gamemain"]["platform_dividend_level"];
                mongocxx::cursor info = dividend_coll.find(make_document(kvp("agentid", agentid)));
                for (auto &doc : info)
                {
                    int32_t level = doc["level"].get_int32();
                    int32_t share = doc["share"].get_int32();
                    string alias = doc["alias"].get_utf8().value.to_string();

                    dividendItem itemTmp;
                    itemTmp.level = level;
                    itemTmp.share = share;
                    itemTmp.title = alias;
                    dividendinfo.push_back(itemTmp);
                    LOG_INFO << " >>> agentid[" << agentid << "],dividendItem: " << level << ", " << share << ", " << alias;
                }

                // 缓存到内存
                if (dividendinfo.size() == 0)
                {
                    retCode = 1;
                    break;
                }

                MutexLockGuard lock(m_dividend_MutexLock);
                m_dividendMap[agentid] = dividendinfo;
            }

            auto it = m_dividendMap.find(agentid);
            if (it != m_dividendMap.end())
            {
                for (auto item : it->second)
                {
                    LOG_INFO << "---" << __FUNCTION__ << " level[" << item.level << "],share[" << item.share << "],title[" << item.title << "]";
                }
            }

            if (m_dividendCfgMap.find(agentid) == m_dividendCfgMap.end())
            {
                // 读取配置表
                mongocxx::collection dividend_config_coll = MONGODBCLIENT["gamemain"]["platform_dividend_config"];
                auto cfg_doc = dividend_config_coll.find_one(make_document(kvp("agentid", agentid), kvp("status", 0)));
                if (!cfg_doc)
                {
                    /* code */
                    retCode = 2;
                    break;
                }

                dividendCfg diviCfg = {0};
                auto cfg_view = cfg_doc->view();
                diviCfg.limit = cfg_view["minPerformance"].get_int64();
                diviCfg.rate = cfg_view["dividendScale"].get_double();

                MutexLockGuard lock(m_dividend_MutexLock);
                m_dividendCfgMap[agentid] = diviCfg;
            }

            auto iter = m_dividendCfgMap.find(agentid);
            if (iter != m_dividendCfgMap.end())
            {
                LOG_INFO << "---" << __FUNCTION__ << " limit[" << iter->second.limit << "],rate[" << iter->second.rate << "]";
            }

        } while (0);
    }
    catch (exception &e)
    {
        retCode = 3;
        LOG_ERROR << " >>> Exception: " << e.what();
    }
}
}


//  auto updateView = user_coll.find_one_and_update(make_document(kvp("userid", userId)),
//                  make_document(kvp("$inc", make_document(kvp("safebox",100))))), options);
//                 //  document{} << "$inc" << open_document << "rebate" << -rebate << close_document << finalize,options);
