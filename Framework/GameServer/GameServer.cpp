#include "GameServer.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dlfcn.h>

#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>

#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>


#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

#include "json/json.h"
#include "crypto/crypto.h"

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/GlobalFunc.h"
// #include "public/GameGlobalFunc.h"
#include "ConnectionPool.h"

#include "public/ITableFrameSink.h"

#include "AndroidUserManager.h"
#include "ServerUserManager.h"
#include "GameTableManager.h"
#include "TableFrame.h"

#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/ThreadLocalSingletonRedisClient.h"
#include "proto/Game.Common.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "TraceMsg/FormatCmdId.h"

// is debug server now.
extern int g_bisDebug;

namespace Landy
{

extern int g_EastOfUtc;

using namespace muduo;
using namespace muduo::net;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

map<int, GameServer::AccessCommandFunctor> GameServer::m_functor_map;

//// timer clock internal value.
//double g_fTickSysClock = 1.0f;
//timeval g_lastTickTime = { 0 };

// dump the data content value now.
void dumpbin(void *datain, int size)
{
    if (!g_bisDebug)
        return;

    string strValue;
    char buf[32] = {0};
    unsigned char *data = (unsigned char *)datain;
    for (int i = 0; i < size; ++i)
    {
        snprintf(buf, sizeof(buf), "%02X ", data[i]);
        strValue += buf;
    }
    LOG_DEBUG << " >>> bin:" << strValue;
}

GameServer::GameServer(muduo::net::EventLoop *loop,
                       const muduo::net::InetAddress &listenAddr,
                       int idleSeconds, string netcardName)
    : m_server(loop, listenAddr, "GameServer-GameServer"),
 m_game_logic_thread(new EventLoopThread(bind(&GameServer::singleThreadInit, this, placeholders::_1), "GameLogicEventLoopThread")),
 m_game_sqldb_thread(new EventLoopThread(bind(&GameServer::singleThreadInit1, this, placeholders::_1), "GameBDEventLoopThread")),m_netCardName(netcardName)
{
    init();
    m_server.setConnectionCallback(bind(&GameServer::onConnection, this, placeholders::_1));
    m_server.setMessageCallback(bind(&GameServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));


    //    m_timer_thread->startLoop();
}

GameServer::~GameServer()
{

#if 0
    m_invalidGameServerIps.clear();
#endif

    m_functor_map.clear();
}

void GameServer::init()
{
    srand(time(nullptr));
#if 0
    m_invalidGameServerIps.clear();
#endif

    m_functor_map.clear();

    m_myIP = "";
    m_myPort = 0;
    m_proxyId = 0;

    m_bStopServer = false;
    m_bInvaildServer = false;
    m_UserIdProxyConnectionInfoMap.clear();
    m_IPProxyMap.clear();
    // 实例化
    TaskService::get_mutable_instance();

    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::Game::Common::SUBID::GET_AES_KEY_MESSAGE_REQ] = bind(&GameServer::cmd_get_aes_key, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::Game::Common::SUBID::GET_MOBILE_VERIFY_CODE_MESSAGE_REQ] = bind(&GameServer::cmd_get_verify_code, this, _1, _2, _3, _4);
    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::Game::Common::KEEP_ALIVE_REQ] = bind(&GameServer::cmd_keep_alive_ping, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::SUB_C2S_ENTER_ROOM_REQ] = bind(&GameServer::cmd_enter_room, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::SUB_C2S_USER_READY_REQ] = bind(&GameServer::cmd_user_ready, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::SUB_C2S_USER_LEFT_REQ] = bind(&GameServer::cmd_user_left, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
    m_functor_map[(::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER << 8) | ::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE] = bind(&GameServer::cmd_user_offline, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    m_functor_map[(::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER << 8) | ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER] = bind(&GameServer::cmd_notifyRepairServerResp, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::Message::SUBID::LOGIN_GAME_SERVER_MESSAGE_REQ] = bind(&GameServer::cmd_login_servers, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::Game::Common::GET_USER_SCORE_MESSAGE_REQ] =  bind(&GameServer::cmd_get_user_score, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::SUB_C2S_CHANGE_TABLE_REQ] = bind(&GameServer::cmd_user_change_table, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::Message::SUBID::SUB_C2S_CHANGE_TABLE_SIT_CHAIR_REQ] = bind(&GameServer::cmd_user_change_table_sit_chair, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::GameServer::SUBID::SUB_C2S_USER_RECHARGE_OK_REQ] = bind(&GameServer::cmd_user_recharge_ok, this, _1, _2, _3, _4);
    //    m_functor_map[(::Game::Common::MSGGS_MAIN_FRAME << 8) | ::GameServer::Message::SUBID::SUB_C2S_USER_EXCHANGE_FAIL_REQ] = bind(&GameServer::cmd_user_exchange_fail, this, _1, _2, _3, _4);

    m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC << 8)] = bind(&GameServer::cmd_game_message, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
}

void GameServer::quit()
{
    m_zookeeperclient->closeServer();
    //    m_redisClient->
    //    m_thread_pool.stop();
}

void GameServer::setThreadNum(int numThreads)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "网络IO线程数 = " << numThreads;
    m_server.setThreadNum(numThreads);
}

bool GameServer::loadRSAPriKey()
{
    int len = 0;
    vector<unsigned char> vector_AESKey;
    vector_AESKey.resize(KEY.size() / 2);
    Landy::Crypto::HexStringToBuffer(KEY, (unsigned char *)&vector_AESKey[0], len);
    string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

    vector<unsigned char> vector_decrypt_pri_key_data;

    char buf[1024 * 3];
    memset(buf, 0, sizeof(buf));
    if (boost::filesystem::exists("prikey.bin"))
    {
        ifstream in("prikey.bin", ios::in | ios::binary);
        if (in.is_open())
        {
            in.seekg(0, ios::end);
            int size = in.tellg();
            in.seekg(0, ios::beg);
            in.read(buf, size);
            Landy::Crypto::aesDecrypt(strAESKey, (unsigned char *)buf, KEY_SIZE, vector_decrypt_pri_key_data);
            m_pri_key.assign(vector_decrypt_pri_key_data.begin(), vector_decrypt_pri_key_data.begin() + KEY_SIZE);
            //             if(m_pri_key == ORG_PRI_KEY)
            //                 LOG_INFO <<"+++++++++++++++++++++OKOKOK+++++++++++++++++++++";
            //             else
            //                 LOG_INFO<<"+++++++++++++++++++++Error+++++++++++++++++++++";
            in.close();
            return true;
        }
    }

    LOG_ERROR << "PRI KEY FILE not exists!";
    return false;
}

string GameServer::decryDBKey(string password, int db_password_real_len)
{
    //LOG_INFO << "Load DB Key...";

    int len = 0;
    vector<unsigned char> vector_AESKey;
    vector_AESKey.resize(KEY.size() / 2);
    Landy::Crypto::HexStringToBuffer(KEY, (unsigned char *)&vector_AESKey[0], len);
    string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

    len = 0;
    vector<unsigned char> dbKey;
    dbKey.resize(password.size() / 2);
    Landy::Crypto::HexStringToBuffer(password, (unsigned char *)&dbKey[0], len);

    vector<unsigned char> vector_decrypt_db_key_data;
    Landy::Crypto::aesDecrypt(strAESKey, (unsigned char *)&dbKey[0], dbKey.size(), vector_decrypt_db_key_data);
    string strDBKey;
    strDBKey.assign(vector_decrypt_db_key_data.begin(), vector_decrypt_db_key_data.begin() + db_password_real_len);
    return strDBKey;
}

bool GameServer::loadKey()
{
    //LOG_INFO << "Load RSA PRI Key...";
    return loadRSAPriKey();
}
bool GameServer::startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize)
{
//    string url = "tcp://"+host+":"+std::to_string(port);
//    string dbPassword = /*decryDBKey(password, db_password_len);*/password;
//    bool bSucces = ConnectionPool::GetInstance()->initPool(url, name, dbPassword, dbname, maxSize);
//    return bSucces;
}

bool GameServer::InitRedisCluster(string ip, string password)
{
    LOG_INFO << "InitRedisCluster m_redisPubSubClient... ";
    m_redisPubSubClient.reset(new RedisClient());
    if (!m_redisPubSubClient->initRedisCluster(ip, password))
    {
        LOG_FATAL << "redisclient connection error";
        return false;
    }

//    muduo::Timestamp  timex1 = muduo::Timestamp::now();
//    for(int i=0;i<1000;i++)
//    {
//        m_redisPubSubClient->SetUserOnlineInfo(201+i, 200, 2001);
//    }
//    double SetUserOnlineInfoitmes=muduo::timeDifference(muduo::Timestamp::now(), timex1);
//    LOG_ERROR << "耗时时间是-----------------------   "<<SetUserOnlineInfoitmes<<"-----执行次数----- "<<1000;
    m_redisPassword = password;

    m_redisPubSubClient->subscribeStopGameServerMessage(bind(&GameServer::notifyStopGameServerMessage, this, placeholders::_1));
    m_redisPubSubClient->subscribeRechargeScoreToGameServerMessage(bind(&GameServer::notifyRechargeScoreToGameServerMessage, this, placeholders::_1));
    m_redisPubSubClient->subscribeExchangeScoreMessage(bind(&GameServer::notifyExchangeScoreToGameServerMessage, this, placeholders::_1));
    m_redisPubSubClient->subsreibeRefreashConfigMessage(bind(&GameServer::notifyRefreashConfig, this, placeholders::_1));

    // 更新任务通知
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_updateTask, [&](string msg) {
        TaskService::get_mutable_instance().setUpdateTask(true);
        LOG_ERROR << "-------------updateTask------------" << msg << " " << __FUNCTION__;
    });

    m_redisPubSubClient->startSubThread();

    m_redisIPPort = ip;

    return true;
}

bool GameServer::InitZookeeper(string ip, uint32_t gameId, uint32_t roomId)
{
    m_roomId = roomId;
    m_gameId = gameId;
    m_zookeeperclient.reset(new ZookeeperClient(ip));
    //    shared_ptr<ZookeeperClient> zookeeperclient(new ZookeeperClient("192.168.100.160:2181"));
    //    m_zookeeperclient = zookeeperclient;
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&GameServer::ZookeeperConnectedHandler, this));
    if (m_zookeeperclient->connectServer())
    {
        return true;
    }
    else
    {
        LOG_FATAL << "zookeeperclient->connectServer error";
        return false;
    }
}

bool GameServer::InitMongoDB(string url)
{
    LOG_INFO << " " << __FILE__ << " " << __FUNCTION__;

    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;
    ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
    return true;
}
void GameServer::singleThreadInit1(EventLoop *ep)
{
    if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
    {
        LOG_FATAL << "RedisClient Connection ERROR!";
        return;
    }
    if(m_GameRoomInfo.roomId== SGJ_ROOM_ID)
    {
        REDISCLIENT.hset(REDIS_CUR_STOCKS+to_string(m_GameRoomInfo.agentId), to_string(SGJ_ROOM_ID), to_string(m_GameRoomInfo.totalStock));
    }
    else
    {
         REDISCLIENT.hset(REDIS_CUR_STOCKS+to_string(m_GameRoomInfo.agentId), to_string(m_GameRoomInfo.roomId), to_string(m_GameRoomInfo.totalStock));
    }

    LOG_ERROR << "--- *** 当前库存[" << m_GameRoomInfo.totalStock << "]";

    mongocxx::database db = MONGODBCLIENT["gamemain"];


    LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__;
}

void GameServer::singleThreadInit(EventLoop *ep)
{
    if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
    {
        LOG_FATAL << "RedisClient Connection ERROR!";
        return;
    }
    if(m_GameRoomInfo.roomId== SGJ_ROOM_ID)
    {
        REDISCLIENT.hset(REDIS_CUR_STOCKS+to_string(m_GameRoomInfo.agentId), to_string(SGJ_ROOM_ID), to_string(m_GameRoomInfo.totalStock));
    }
    else
    {
         REDISCLIENT.hset(REDIS_CUR_STOCKS+to_string(m_GameRoomInfo.agentId), to_string(m_GameRoomInfo.roomId), to_string(m_GameRoomInfo.totalStock));
    }

    LOG_ERROR << "--- *** 当前库存[" << m_GameRoomInfo.totalStock << "]";

    mongocxx::database db = MONGODBCLIENT["gamemain"];


    LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__;
}

void GameServer::start(int threadCount)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "worker 线程数 = " << 1;
    //    m_thread_pool.setThreadInitCallback(bind(&GameServer::threadInit, this));
    //    m_thread_pool.start(1);
    m_game_sqldb_thread->startLoop();
    m_game_logic_thread->startLoop();
    if (m_GameRoomInfo.bEnableAndroid)
    {
        if (m_GameInfo.gameType == GameType_BaiRen)
        {
            m_game_logic_thread->getLoop()->runEvery(3.0, bind(&CAndroidUserManager::OnTimerCheckUserIn, &(CAndroidUserManager::get_mutable_instance())));
            auto pUser = make_shared<CServerUserItem>();
			for (uint32_t idx = 0; idx < m_GameRoomInfo.tableCount; ++idx)
			{
                dynamic_pointer_cast<CTableFrame>(CGameTableManager::get_mutable_instance().FindNormalSuitTable(pUser, INVALID_TABLE,idx)); //百人场默认一个桌子可用
			}
            CAndroidUserManager::get_mutable_instance().Hourtimer(m_game_logic_thread);
        }
        else if (m_GameInfo.gameType == GameType_Confrontation)
            m_game_logic_thread->getLoop()->runEvery(0.1f, bind(&CAndroidUserManager::OnTimerCheckUserIn, &(CAndroidUserManager::get_mutable_instance())));
    }

    //    m_timer_thread->getLoop()->runEvery(5.0f, bind(&GameServer::OnRepairGameServerNode, this));
    if (m_GameRoomInfo.updatePlayerNumTimes)
    {
        m_game_logic_thread->getLoop()->runEvery(m_GameRoomInfo.updatePlayerNumTimes,
                                                 bind(&CGameTableManager::UpdataPlayerCount2Redis, &(CGameTableManager::get_mutable_instance()), m_szNodeValue,m_GameInfo));
        m_game_logic_thread->getLoop()->runEvery(m_GameRoomInfo.updatePlayerNumTimes * 2,
                                                 bind(&GameServer::updateAgentPlayers, this));
    }

    m_server.start();

    m_server.getLoop()->runEvery(5.0f, bind(&GameServer::OnRepairGameServerNode, this));
}

void GameServer::OnRepairGameServerNode()
{
    if (!m_bStopServer) // && ZNONODE == m_zookeeperclient->existsNode(m_szNodePath))
    {
        if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath) && ZNONODE == m_zookeeperclient->existsNode(m_szInvalidNodePath))
        {
            LOG_DEBUG << m_szNodePath;
            m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
        }
    }
}

void GameServer::ZookeeperConnectedHandler()
{
    LOG_INFO << __FUNCTION__;
    if (ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServers"))
        m_zookeeperclient->createNode("/GAME/GameServers", "GameServers");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServersInvalid"))
        m_zookeeperclient->createNode("/GAME/GameServersInvalid", "GameServersInvalid");

    vector<string> vec;
    boost::algorithm::split(vec, m_server.ipPort(), boost::is_any_of(":"));
    string ip;
    GlobalFunc::getIP(m_netCardName, ip);
    m_myIP = ip;
    m_myPort = stoi(vec[1]);
    m_szNodeValue = to_string(m_roomId) + ":" + ip + ":" + vec[1];
    m_szNodePath = "/GAME/GameServers/" + m_szNodeValue;
    m_szInvalidNodePath = "/GAME/GameServersInvalid/" + m_szNodeValue;
    m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);

	LOG_WARN << " >>> ZookeeperConnectedHandler.m_szNodeValue:" << m_szNodeValue;

#if 0
    GetChildrenWatcherHandler getInvalidGameServersChildrenWatcherHandler = std::bind(&GameServer::getInvalidGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> invalidGameServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/GameServersInvalid", invalidGameServers, getInvalidGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_invalidGameServerIps_mutex);
        m_invalidGameServerIps.assign(invalidGameServers.begin(), invalidGameServers.end());
    }

    // init to dump the game servers.
    for(string ip : invalidGameServers)
    {
        LOG_INFO << " >>> Init GET GameServers :" << ip;
    }
#endif
}

bool GameServer::LoadGameRoomKindInfo(uint32_t gameId, uint32_t roomId)
{
	vector<string> vec;
	boost::algorithm::split(vec, m_server.ipPort(), boost::is_any_of(":"));
	string ip;
	GlobalFunc::getIP(m_netCardName, ip);
	m_myIP = ip;
	m_myPort = stoi(vec[1]);
    bool bFind = false;
    try
    {

        uint32_t dbGameId = 0, dbRevenueRatio = 0, dbGameSortId = 0, dbGameType = 0, dbGameIsHot = 0, dbMatchData = 0;
        string dbGameName, dbGameServiceName;
        int32_t dbGameStatus = 0;

        uint32_t dbRoomId = 0, dbTableCount = 0, dbMinPlayerNum = 0, dbMaxPlayerNum = 0, dbRoomStatus = 0, dbRealChangeAndroid = 0;
        string dbRoomName;

        int64_t dbFloorScore, dbCeilScore, dbEnterMinScore, dbEnterMaxScore, dbBroadcastScore, dbMaxJettonScore;

        int64_t dbTotalStock, dbTotalStockLowerLimit, dbTotalStockHighLimit;
        int32_t dbSystemKillAllRatio, dbSystemReduceRatio, dbChangeCardRatio;

        uint32_t dbAndroidCount = 0, dbAndroidMaxUserCount = 0, dbBEnableAndroid = 0, dbUpdatePlayerNumTimes = 30;

        int64_t dbBaseCoefficient = 0;
        double dbCoeHighLimit=0.0,dbCoeLowLimit=0.0,dbPersonalRatio=0.0,dbAgentRatio=0.0;
        vector<int64_t> jettonsVec;
        mongocxx::collection currencycoll = MONGODBCLIENT["gamemain"]["currency_gameserver"];
        auto query_currency = document{} << "ipadress" << m_myIP << finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> currency_result = currencycoll.find_one(query_currency.view());
        if(currency_result)
        {
            bsoncxx::document::view view = currency_result->view();
            m_currency = view["currency"].get_int32();

             LOG_ERROR<<"读到这个币种下的currency"<<m_currency;
        }
        else
        {
            m_currency = 0;
            LOG_ERROR<<"读不到这个币种下的currency_gameserver ,只能默认为0 人民币种";
        }
        if(m_currency==0)
        {
            mongocxx::collection proxycoll = MONGODBCLIENT["gamemain"]["proxy_gameserver"];
            auto query_proxy = document{} << "ipadress" << m_myIP << finalize;
            bsoncxx::stdx::optional<bsoncxx::document::value> proxy_result = proxycoll.find_one(query_proxy.view());
            if(proxy_result)
            {
                 bsoncxx::document::view view = proxy_result->view();
                 m_proxyId = view["agentid"].get_int32();
            }
            else
            {
                 m_proxyId = 0;
                 LOG_ERROR<<"读不到这个代理下的proxy_gameserver ,只能默认为0 号代理";
            }
            mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["game_kind"];

            auto query_value = document{} << "gameid" << (int32_t)gameId <<"agentid"<<(int32_t)m_proxyId<< finalize;
            // LOG_DEBUG << "Query:" << bsoncxx::to_json(query_value);

            bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
            if (maybe_result)
            {
                LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(maybe_result->view());

                bsoncxx::document::view view = maybe_result->view();
                dbGameId = view["gameid"].get_int32();
                dbGameName = view["gamename"].get_utf8().value.to_string();
                dbGameSortId = view["sort"].get_int32();
                dbGameServiceName = view["servicename"].get_utf8().value.to_string();
                dbRevenueRatio = view["revenueRatio"].get_int32();
                dbGameType = view["type"].get_int32();
                dbGameIsHot = view["ishot"].get_int32();
                dbGameStatus = view["status"].get_int32();
                dbUpdatePlayerNumTimes = view["updatePlayerNum"].get_int32();
                dbMatchData = view["matchmask"].get_int32(); //0b000010;

                auto rooms = view["rooms"].get_array();
                for (auto &roomDoc : rooms.value)
                {
                    dbRoomId = roomDoc["roomid"].get_int32();

                    if (dbRoomId == roomId)
                    {
                        dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();

                        dbTableCount = roomDoc["tablecount"].get_int32();

                        dbFloorScore = roomDoc["floorscore"].get_int64();
                        dbCeilScore = roomDoc["ceilscore"].get_int64();
                        dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                        dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();

                        dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                        dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();

                        dbBroadcastScore = roomDoc["broadcastscore"].get_int64();

                        dbBEnableAndroid = roomDoc["enableandroid"].get_int32();
                        dbAndroidCount = roomDoc["androidcount"].get_int32();
                        dbAndroidMaxUserCount = roomDoc["androidmaxusercount"].get_int32();

                        dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();

                        dbTotalStock = roomDoc["totalstock"].get_int64();
                        dbTotalStockLowerLimit = roomDoc["totalstocklowerlimit"].get_int64();
                        dbTotalStockHighLimit = roomDoc["totalstockhighlimit"].get_int64();

                        dbSystemKillAllRatio = roomDoc["systemkillallratio"].get_int32();
                        dbSystemReduceRatio = roomDoc["systemreduceratio"].get_int32();
                        dbChangeCardRatio = roomDoc["changecardratio"].get_int32();
                        dbRoomStatus = roomDoc["status"].get_int32();

                        //add by xiaorang for personal ctr
                        if (roomDoc["basiccoe"] && roomDoc["basiccoe"].type() == bsoncxx::type::k_int64)
                            dbBaseCoefficient = roomDoc["basiccoe"].get_int64();
                        if (roomDoc["highlimit"] && roomDoc["highlimit"].type() == bsoncxx::type::k_int64)
                            dbCoeHighLimit = roomDoc["highlimit"].get_int64();
                        if (roomDoc["lowlimit"] && roomDoc["lowlimit"].type() == bsoncxx::type::k_int64)
                            dbCoeLowLimit = roomDoc["lowlimit"].get_int64();
                        if (roomDoc["personalratio"] && roomDoc["personalratio"].type() == bsoncxx::type::k_int64)
                            dbPersonalRatio = roomDoc["personalratio"].get_int64();
                        if (roomDoc["agentratio"] && roomDoc["agentratio"].type() == bsoncxx::type::k_int64)
                            dbAgentRatio = roomDoc["agentratio"].get_int64();

                        bsoncxx::types::b_array jettons;
                        bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                        if (jettons_obj.type() == bsoncxx::type::k_array)
                            jettons = jettons_obj.get_array();
                        else
                            jettons = roomDoc["jettons"]["_v"].get_array();
                        for (auto &jetton : jettons.value)
                        {
                            jettonsVec.push_back(jetton.get_int64());
                        }
                        if (dbGameType == GameType_BaiRen)
                        {
                            // 消除
                            m_GameRoomInfo.enterAndroidPercentage.clear();

                            dbRealChangeAndroid = roomDoc["realChangeAndroid"].get_int32();
                            bsoncxx::types::b_array androidPercentage = roomDoc["androidPercentage"]["_v"].get_array();
                            for (auto &percent : androidPercentage.value)
                            {
                                m_GameRoomInfo.enterAndroidPercentage.push_back(percent.get_double());
                            }
                            //
                            LOG_WARN << "---enter Android count [" << m_GameRoomInfo.enterAndroidPercentage.size() << "]";
                        }

                        bFind = true;
                        break;
                    }
                }

                if (bFind)
                {
                    m_GameInfo.gameId = dbGameId;
                    m_GameInfo.gameName = dbGameName;
                    m_GameInfo.sortId = dbGameSortId;
                    m_GameInfo.gameServiceName = dbGameServiceName;
                    m_GameInfo.revenueRatio = dbRevenueRatio;
                    m_GameInfo.gameType = dbGameType;
                    for (int i = 0; i < MTH_MAX; ++i)
                    {
                        m_GameInfo.matchforbids[i] = ((dbMatchData & (1 << i)) != 0);
                        m_GameRoomInfo.roomforbids[i] = ((dbMatchData & (1 << i)) != 0);
                    }
                    if (m_GameInfo.gameType == GameType_BaiRen)
                    {
                        m_GameRoomInfo.serverIP = to_string(m_roomId) + ":" + m_myIP + ":" + to_string(m_myPort);
                        LOG_WARN << " >>> m_GameRoomInfo.serverIP:" << m_GameRoomInfo.serverIP;
                    }

                    m_GameRoomInfo.gameId = dbGameId;
                    m_GameRoomInfo.roomId = dbRoomId;
                    m_GameRoomInfo.roomName = dbRoomName;
                    m_GameRoomInfo.tableCount = dbTableCount;
                    m_GameRoomInfo.floorScore = dbFloorScore;
                    m_GameRoomInfo.ceilScore = dbCeilScore;
                    m_GameRoomInfo.enterMinScore = dbEnterMinScore;
                    m_GameRoomInfo.enterMaxScore = dbEnterMaxScore;

                    m_GameRoomInfo.minPlayerNum = dbMinPlayerNum;
                    m_GameRoomInfo.maxPlayerNum = dbMaxPlayerNum;

                    m_GameRoomInfo.broadcastScore = dbBroadcastScore;
                    m_GameRoomInfo.maxJettonScore = dbMaxJettonScore;

                    m_GameRoomInfo.androidCount = dbAndroidCount;
                    m_GameRoomInfo.androidMaxUserCount = dbAndroidMaxUserCount;

                    m_GameRoomInfo.totalStock = dbTotalStock;
                    m_GameRoomInfo.totalStockLowerLimit = dbTotalStockLowerLimit;
                    m_GameRoomInfo.totalStockHighLimit = dbTotalStockHighLimit;

                    m_GameRoomInfo.systemKillAllRatio = dbSystemKillAllRatio;
                    m_GameRoomInfo.systemReduceRatio = dbSystemReduceRatio;
                    m_GameRoomInfo.changeCardRatio = dbChangeCardRatio;

                    m_GameRoomInfo.serverStatus = dbRoomStatus;
                    m_GameRoomInfo.realChangeAndroid = dbRealChangeAndroid;
                    m_GameRoomInfo.bEnableAndroid = dbBEnableAndroid;

                    m_GameRoomInfo.jettons = jettonsVec;

                    m_GameRoomInfo.updatePlayerNumTimes = dbUpdatePlayerNumTimes;


                    //新增三个个人库存控制系数,输赢基础值（房间信息）流水赢系数（房间信息）流水输系数（房间信息）
                    m_GameRoomInfo.betHighLimit = dbCoeHighLimit/1000.0;
                    m_GameRoomInfo.betLowerLimit = dbCoeLowLimit/1000.0;
                    m_GameRoomInfo.personalInventory = dbBaseCoefficient;
                    m_GameRoomInfo.personalratio = dbPersonalRatio/1000.0;
                    m_GameRoomInfo.agentRatio  = dbAgentRatio/1000.0;
                    m_GameRoomInfo.agentId = m_proxyId ;

                    LOG_WARN << " >>> totalStock:" << dbTotalStock;
                    LOG_WARN << " >>> totalStockLowerLimit:" << dbTotalStockLowerLimit;
                    LOG_WARN << " >>> totalStockHighLimit:" << dbTotalStockHighLimit;

                    //        m_GamePlayKind.nServerStatus    = dbGameKindStatus;         //
                    //        m_GamePlayKind.bisDynamicJoin   = dbDynamicJoin;            //
                    //        m_GamePlayKind.bisAutoReady     = dbAutoReady;              //
                    //        m_GamePlayKind.bisEnterIsReady  = dbIsEnterIsReady;         //
                    //        m_GamePlayKind.bisQipai         = dbQipai;                  //
                    //        m_GamePlayKind.bisKeepAndroidin = dbKeepAndroidIn;          //
                    //        m_GamePlayKind.bisLeaveAnyTime  = dbLeaveAnyTime;           //
                    //        m_GamePlayKind.nServerStatus    = SERVER_STAT_ENABLE;       // enable.

                    tagStockInfo stockinfo = {0};
                    stockinfo.nStorageControl = dbTotalStock;
                    stockinfo.nStorageLowerLimit = dbTotalStockLowerLimit;
                    //        stockinfo.nAndroidStorage = nAndroidStock;
                    //        stockinfo.nAndroidStorageLowerLimit = nAndroidStockLowerLimit;
                    stockinfo.wSystemAllKillRatio = dbSystemKillAllRatio;
                    CGameTableManager::get_mutable_instance().SetTableStockInfo(stockinfo);

                    CAndroidUserManager::get_mutable_instance().SetAndroidUserCount(dbAndroidCount, dbAndroidMaxUserCount);

                    LOG_WARN << " >>> bisEnableAndroid:" << dbBEnableAndroid;
                }
            }
            else
            {
                m_proxyId = 0;
                auto query = document{} << "gameid" << (int32_t)gameId <<"agentid"<<(int32_t)m_proxyId<< finalize;


                bsoncxx::stdx::optional<bsoncxx::document::value> maybe = coll.find_one(query.view());

                if(maybe)
                {
                    LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(maybe->view());

                    bsoncxx::document::view view = maybe->view();
                    dbGameId = view["gameid"].get_int32();
                    dbGameName = view["gamename"].get_utf8().value.to_string();
                    dbGameSortId = view["sort"].get_int32();
                    dbGameServiceName = view["servicename"].get_utf8().value.to_string();
                    dbRevenueRatio = view["revenueRatio"].get_int32();
                    dbGameType = view["type"].get_int32();
                    dbGameIsHot = view["ishot"].get_int32();
                    dbGameStatus = view["status"].get_int32();
                    dbUpdatePlayerNumTimes = view["updatePlayerNum"].get_int32();
                    dbMatchData = view["matchmask"].get_int32(); //0b000010;

                    auto rooms = view["rooms"].get_array();
                    for (auto &roomDoc : rooms.value)
                    {
                        dbRoomId = roomDoc["roomid"].get_int32();

                        if (dbRoomId == roomId)
                        {
                            dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();

                            dbTableCount = roomDoc["tablecount"].get_int32();

                            dbFloorScore = roomDoc["floorscore"].get_int64();
                            dbCeilScore = roomDoc["ceilscore"].get_int64();
                            dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                            dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();

                            dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                            dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();

                            dbBroadcastScore = roomDoc["broadcastscore"].get_int64();

                            dbBEnableAndroid = roomDoc["enableandroid"].get_int32();
                            dbAndroidCount = roomDoc["androidcount"].get_int32();
                            dbAndroidMaxUserCount = roomDoc["androidmaxusercount"].get_int32();

                            dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();

                            dbTotalStock = roomDoc["totalstock"].get_int64();
                            dbTotalStockLowerLimit = roomDoc["totalstocklowerlimit"].get_int64();
                            dbTotalStockHighLimit = roomDoc["totalstockhighlimit"].get_int64();

                            dbSystemKillAllRatio = roomDoc["systemkillallratio"].get_int32();
                            dbSystemReduceRatio = roomDoc["systemreduceratio"].get_int32();
                            dbChangeCardRatio = roomDoc["changecardratio"].get_int32();
                            dbRoomStatus = roomDoc["status"].get_int32();

                            //add by xiaorang for personal ctr
                            if (roomDoc["basiccoe"] && roomDoc["basiccoe"].type() == bsoncxx::type::k_int64)
                                dbBaseCoefficient = roomDoc["basiccoe"].get_int64();
                            if (roomDoc["highlimit"] && roomDoc["highlimit"].type() == bsoncxx::type::k_int64)
                                dbCoeHighLimit = roomDoc["highlimit"].get_int64();
                            if (roomDoc["lowlimit"] && roomDoc["lowlimit"].type() == bsoncxx::type::k_int64)
                                dbCoeLowLimit = roomDoc["lowlimit"].get_int64();
                            if (roomDoc["personalratio"] && roomDoc["personalratio"].type() == bsoncxx::type::k_int64)
                                dbPersonalRatio = roomDoc["personalratio"].get_int64();
                            if (roomDoc["agentratio"] && roomDoc["agentratio"].type() == bsoncxx::type::k_int64)
                                dbAgentRatio = roomDoc["agentratio"].get_int64();

                            bsoncxx::types::b_array jettons;
                            bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                            if (jettons_obj.type() == bsoncxx::type::k_array)
                                jettons = jettons_obj.get_array();
                            else
                                jettons = roomDoc["jettons"]["_v"].get_array();
                            for (auto &jetton : jettons.value)
                            {
                                jettonsVec.push_back(jetton.get_int64());
                            }
                            if (dbGameType == GameType_BaiRen)
                            {
                                // 消除
                                m_GameRoomInfo.enterAndroidPercentage.clear();

                                dbRealChangeAndroid = roomDoc["realChangeAndroid"].get_int32();
                                bsoncxx::types::b_array androidPercentage = roomDoc["androidPercentage"]["_v"].get_array();
                                for (auto &percent : androidPercentage.value)
                                {
                                    m_GameRoomInfo.enterAndroidPercentage.push_back(percent.get_double());
                                }
                                //
                                LOG_WARN << "---enter Android count [" << m_GameRoomInfo.enterAndroidPercentage.size() << "]";
                            }

                            bFind = true;
                            break;
                        }
                    }

                    if (bFind)
                    {
                        m_GameInfo.gameId = dbGameId;
                        m_GameInfo.gameName = dbGameName;
                        m_GameInfo.sortId = dbGameSortId;
                        m_GameInfo.gameServiceName = dbGameServiceName;
                        m_GameInfo.revenueRatio = dbRevenueRatio;
                        m_GameInfo.gameType = dbGameType;
                        for (int i = 0; i < MTH_MAX; ++i)
                        {
                            m_GameInfo.matchforbids[i] = ((dbMatchData & (1 << i)) != 0);
                            m_GameRoomInfo.roomforbids[i] = ((dbMatchData & (1 << i)) != 0);
                        }
                        if (m_GameInfo.gameType == GameType_BaiRen)
                        {
                            m_GameRoomInfo.serverIP = to_string(m_roomId) + ":" + m_myIP + ":" + to_string(m_myPort);
                            LOG_WARN << " >>> m_GameRoomInfo.serverIP:" << m_GameRoomInfo.serverIP;
                        }
                        m_GameRoomInfo.gameId = dbGameId;
                        m_GameRoomInfo.roomId = dbRoomId;
                        m_GameRoomInfo.roomName = dbRoomName;
                        m_GameRoomInfo.tableCount = dbTableCount;
                        m_GameRoomInfo.floorScore = dbFloorScore;
                        m_GameRoomInfo.ceilScore = dbCeilScore;
                        m_GameRoomInfo.enterMinScore = dbEnterMinScore;
                        m_GameRoomInfo.enterMaxScore = dbEnterMaxScore;

                        m_GameRoomInfo.minPlayerNum = dbMinPlayerNum;
                        m_GameRoomInfo.maxPlayerNum = dbMaxPlayerNum;

                        m_GameRoomInfo.broadcastScore = dbBroadcastScore;
                        m_GameRoomInfo.maxJettonScore = dbMaxJettonScore;

                        m_GameRoomInfo.androidCount = dbAndroidCount;
                        m_GameRoomInfo.androidMaxUserCount = dbAndroidMaxUserCount;

                        m_GameRoomInfo.totalStock = dbTotalStock;
                        m_GameRoomInfo.totalStockLowerLimit = dbTotalStockLowerLimit;
                        m_GameRoomInfo.totalStockHighLimit = dbTotalStockHighLimit;

                        m_GameRoomInfo.systemKillAllRatio = dbSystemKillAllRatio;
                        m_GameRoomInfo.systemReduceRatio = dbSystemReduceRatio;
                        m_GameRoomInfo.changeCardRatio = dbChangeCardRatio;

                        m_GameRoomInfo.serverStatus = dbRoomStatus;
                        m_GameRoomInfo.realChangeAndroid = dbRealChangeAndroid;
                        m_GameRoomInfo.bEnableAndroid = dbBEnableAndroid;

                        m_GameRoomInfo.jettons = jettonsVec;

                        m_GameRoomInfo.updatePlayerNumTimes = dbUpdatePlayerNumTimes;


                        //新增三个个人库存控制系数,输赢基础值（房间信息）流水赢系数（房间信息）流水输系数（房间信息）
                        m_GameRoomInfo.betHighLimit = dbCoeHighLimit/1000.0;
                        m_GameRoomInfo.betLowerLimit = dbCoeLowLimit/1000.0;
                        m_GameRoomInfo.personalInventory = dbBaseCoefficient;
                        m_GameRoomInfo.personalratio = dbPersonalRatio/1000.0;
                        m_GameRoomInfo.agentRatio  = dbAgentRatio/1000.0;
                        m_GameRoomInfo.agentId = m_proxyId ;

                        LOG_WARN << " >>> totalStock:" << dbTotalStock;
                        LOG_WARN << " >>> totalStockLowerLimit:" << dbTotalStockLowerLimit;
                        LOG_WARN << " >>> totalStockHighLimit:" << dbTotalStockHighLimit;

                        //        m_GamePlayKind.nServerStatus    = dbGameKindStatus;         //
                        //        m_GamePlayKind.bisDynamicJoin   = dbDynamicJoin;            //
                        //        m_GamePlayKind.bisAutoReady     = dbAutoReady;              //
                        //        m_GamePlayKind.bisEnterIsReady  = dbIsEnterIsReady;         //
                        //        m_GamePlayKind.bisQipai         = dbQipai;                  //
                        //        m_GamePlayKind.bisKeepAndroidin = dbKeepAndroidIn;          //
                        //        m_GamePlayKind.bisLeaveAnyTime  = dbLeaveAnyTime;           //
                        //        m_GamePlayKind.nServerStatus    = SERVER_STAT_ENABLE;       // enable.

                        tagStockInfo stockinfo = {0};
                        stockinfo.nStorageControl = dbTotalStock;
                        stockinfo.nStorageLowerLimit = dbTotalStockLowerLimit;
                        //        stockinfo.nAndroidStorage = nAndroidStock;
                        //        stockinfo.nAndroidStorageLowerLimit = nAndroidStockLowerLimit;
                        stockinfo.wSystemAllKillRatio = dbSystemKillAllRatio;
                        CGameTableManager::get_mutable_instance().SetTableStockInfo(stockinfo);

                        CAndroidUserManager::get_mutable_instance().SetAndroidUserCount(dbAndroidCount, dbAndroidMaxUserCount);

                        LOG_WARN << " >>> bisEnableAndroid:" << dbBEnableAndroid;
                    }
                }
                LOG_ERROR<<"读不到这个代理下的任何配置，完犊子，没配好，请检查 proxy_gameserver  和 game_kind 表 ";
            }

            if( dbGameId == (int32_t)eGameKindId::xsgj)
            {
                 bFind = false;
                 mongocxx::collection collxsgj = MONGODBCLIENT["gameconfig"]["game_kind"];
                 auto query_valuexsgj = document{}<<"agentid"<< m_proxyId<< "gameid" << (int32_t)eGameKindId::sgj << finalize;
                // LOG_DEBUG << "Query:" << bsoncxx::to_json(query_value);
                 bsoncxx::stdx::optional<bsoncxx::document::value> maybe_resultxsgj = collxsgj.find_one(query_valuexsgj.view());
                 if(maybe_resultxsgj)
                 {
                     bsoncxx::document::view view = maybe_resultxsgj->view();
                     auto rooms = view["rooms"].get_array();
                     for (auto &roomDoc : rooms.value)
                     {

                         dbRoomId = roomDoc["roomid"].get_int32();

                         if (dbRoomId == SGJ_ROOM_ID)
                         {
                             dbTotalStock = roomDoc["totalstock"].get_int64();
                             dbTotalStockLowerLimit = roomDoc["totalstocklowerlimit"].get_int64();
                             dbTotalStockHighLimit = roomDoc["totalstockhighlimit"].get_int64();
                             bFind = true;
                         }
                     }
                 }
                 if(bFind)
                 {
                     m_GameRoomInfo.totalStock = dbTotalStock;
                     m_GameRoomInfo.totalStockLowerLimit = dbTotalStockLowerLimit;
                     m_GameRoomInfo.totalStockHighLimit = dbTotalStockHighLimit;
                 }

            }

        }
        else
        {
            mongocxx::collection currencycoll1 = MONGODBCLIENT["gamemain"]["currency_gameserver"];
            auto currency_proxy1 = document{} << "ipadress" << m_myIP << finalize;
            bsoncxx::stdx::optional<bsoncxx::document::value> curency_result1 = currencycoll1.find_one(currency_proxy1.view());
            if(curency_result1)
            {
                 bsoncxx::document::view view = curency_result1->view();
                 m_currency = view["currency"].get_int32();
            }
            else
            {
                 m_currency = 0;
                 LOG_ERROR<<"读不到这个代理下的proxy_gameserver ,只能默认为0 号代理";
                 return false;
            }
            mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["game_kind"];

            auto query_value = document{} << "gameid" << (int32_t)gameId <<"currency"<<(int32_t)m_currency<< finalize;
            // LOG_DEBUG << "Query:" << bsoncxx::to_json(query_value);

            bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
            if (maybe_result)
            {
                LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(maybe_result->view());

                bsoncxx::document::view view = maybe_result->view();
                m_proxyId= view["agentid"].get_int32();
                dbGameId = view["gameid"].get_int32();
                dbGameName = view["gamename"].get_utf8().value.to_string();
                dbGameSortId = view["sort"].get_int32();
                dbGameServiceName = view["servicename"].get_utf8().value.to_string();
                dbRevenueRatio = view["revenueRatio"].get_int32();
                dbGameType = view["type"].get_int32();
                dbGameIsHot = view["ishot"].get_int32();
                dbGameStatus = view["status"].get_int32();
                dbUpdatePlayerNumTimes = view["updatePlayerNum"].get_int32();
                dbMatchData = view["matchmask"].get_int32(); //0b000010;

                auto rooms = view["rooms"].get_array();
                for (auto &roomDoc : rooms.value)
                {
                    dbRoomId = roomDoc["roomid"].get_int32();

                    if (dbRoomId == roomId)
                    {
                        dbRoomName = roomDoc["roomname"].get_utf8().value.to_string();

                        dbTableCount = roomDoc["tablecount"].get_int32();

                        dbFloorScore = roomDoc["floorscore"].get_int64();
                        dbCeilScore = roomDoc["ceilscore"].get_int64();
                        dbEnterMinScore = roomDoc["enterminscore"].get_int64();
                        dbEnterMaxScore = roomDoc["entermaxscore"].get_int64();

                        dbMinPlayerNum = roomDoc["minplayernum"].get_int32();
                        dbMaxPlayerNum = roomDoc["maxplayernum"].get_int32();

                        dbBroadcastScore = roomDoc["broadcastscore"].get_int64();

                        dbBEnableAndroid = roomDoc["enableandroid"].get_int32();
                        dbAndroidCount = roomDoc["androidcount"].get_int32();
                        dbAndroidMaxUserCount = roomDoc["androidmaxusercount"].get_int32();

                        dbMaxJettonScore = roomDoc["maxjettonscore"].get_int64();

                        dbTotalStock = roomDoc["totalstock"].get_int64();
                        dbTotalStockLowerLimit = roomDoc["totalstocklowerlimit"].get_int64();
                        dbTotalStockHighLimit = roomDoc["totalstockhighlimit"].get_int64();

                        dbSystemKillAllRatio = roomDoc["systemkillallratio"].get_int32();
                        dbSystemReduceRatio = roomDoc["systemreduceratio"].get_int32();
                        dbChangeCardRatio = roomDoc["changecardratio"].get_int32();
                        dbRoomStatus = roomDoc["status"].get_int32();

                        //add by xiaorang for personal ctr
                        if (roomDoc["basiccoe"] && roomDoc["basiccoe"].type() == bsoncxx::type::k_int64)
                            dbBaseCoefficient = roomDoc["basiccoe"].get_int64();
                        if (roomDoc["highlimit"] && roomDoc["highlimit"].type() == bsoncxx::type::k_int64)
                            dbCoeHighLimit = roomDoc["highlimit"].get_int64();
                        if (roomDoc["lowlimit"] && roomDoc["lowlimit"].type() == bsoncxx::type::k_int64)
                            dbCoeLowLimit = roomDoc["lowlimit"].get_int64();
                        if (roomDoc["personalratio"] && roomDoc["personalratio"].type() == bsoncxx::type::k_int64)
                            dbPersonalRatio = roomDoc["personalratio"].get_int64();
                        if (roomDoc["agentratio"] && roomDoc["agentratio"].type() == bsoncxx::type::k_int64)
                            dbAgentRatio = roomDoc["agentratio"].get_int64();

                        bsoncxx::types::b_array jettons;
                        bsoncxx::document::element jettons_obj = roomDoc["jettons"];
                        if (jettons_obj.type() == bsoncxx::type::k_array)
                            jettons = jettons_obj.get_array();
                        else
                            jettons = roomDoc["jettons"]["_v"].get_array();
                        for (auto &jetton : jettons.value)
                        {
                            jettonsVec.push_back(jetton.get_int64());
                        }
                        if (dbGameType == GameType_BaiRen)
                        {
                            // 消除
                            m_GameRoomInfo.enterAndroidPercentage.clear();

                            dbRealChangeAndroid = roomDoc["realChangeAndroid"].get_int32();
                            bsoncxx::types::b_array androidPercentage = roomDoc["androidPercentage"]["_v"].get_array();
                            for (auto &percent : androidPercentage.value)
                            {
                                m_GameRoomInfo.enterAndroidPercentage.push_back(percent.get_double());
                            }
                            //
                            LOG_WARN << "---enter Android count [" << m_GameRoomInfo.enterAndroidPercentage.size() << "]";
                        }

                        bFind = true;
                        break;
                    }
                }

                if (bFind)
                {
                    m_GameInfo.gameId = dbGameId;
                    m_GameInfo.gameName = dbGameName;
                    m_GameInfo.sortId = dbGameSortId;
                    m_GameInfo.gameServiceName = dbGameServiceName;
                    m_GameInfo.revenueRatio = dbRevenueRatio;
                    m_GameInfo.gameType = dbGameType;
                    for (int i = 0; i < MTH_MAX; ++i)
                    {
                        m_GameInfo.matchforbids[i] = ((dbMatchData & (1 << i)) != 0);
                        m_GameRoomInfo.roomforbids[i] = ((dbMatchData & (1 << i)) != 0);
                    }
                    if (m_GameInfo.gameType == GameType_BaiRen)
                    {
                        m_GameRoomInfo.serverIP = to_string(m_roomId) + ":" + m_myIP + ":" + to_string(m_myPort);
                        LOG_WARN << " >>> m_GameRoomInfo.serverIP:" << m_GameRoomInfo.serverIP;
                    }

                    m_GameRoomInfo.gameId = dbGameId;
                    m_GameRoomInfo.roomId = dbRoomId;
                    m_GameRoomInfo.roomName = dbRoomName;
                    m_GameRoomInfo.tableCount = dbTableCount;
                    m_GameRoomInfo.floorScore = dbFloorScore;
                    m_GameRoomInfo.ceilScore = dbCeilScore;
                    m_GameRoomInfo.enterMinScore = dbEnterMinScore;
                    m_GameRoomInfo.enterMaxScore = dbEnterMaxScore;

                    m_GameRoomInfo.minPlayerNum = dbMinPlayerNum;
                    m_GameRoomInfo.maxPlayerNum = dbMaxPlayerNum;

                    m_GameRoomInfo.broadcastScore = dbBroadcastScore;
                    m_GameRoomInfo.maxJettonScore = dbMaxJettonScore;

                    m_GameRoomInfo.androidCount = dbAndroidCount;
                    m_GameRoomInfo.androidMaxUserCount = dbAndroidMaxUserCount;

                    m_GameRoomInfo.totalStock = dbTotalStock;
                    m_GameRoomInfo.totalStockLowerLimit = dbTotalStockLowerLimit;
                    m_GameRoomInfo.totalStockHighLimit = dbTotalStockHighLimit;

                    m_GameRoomInfo.systemKillAllRatio = dbSystemKillAllRatio;
                    m_GameRoomInfo.systemReduceRatio = dbSystemReduceRatio;
                    m_GameRoomInfo.changeCardRatio = dbChangeCardRatio;

                    m_GameRoomInfo.serverStatus = dbRoomStatus;
                    m_GameRoomInfo.realChangeAndroid = dbRealChangeAndroid;
                    m_GameRoomInfo.bEnableAndroid = dbBEnableAndroid;

                    m_GameRoomInfo.jettons = jettonsVec;

                    m_GameRoomInfo.updatePlayerNumTimes = dbUpdatePlayerNumTimes;


                    //新增三个个人库存控制系数,输赢基础值（房间信息）流水赢系数（房间信息）流水输系数（房间信息）
                    m_GameRoomInfo.betHighLimit = dbCoeHighLimit/1000.0;
                    m_GameRoomInfo.betLowerLimit = dbCoeLowLimit/1000.0;
                    m_GameRoomInfo.personalInventory = dbBaseCoefficient;
                    m_GameRoomInfo.personalratio = dbPersonalRatio/1000.0;
                    m_GameRoomInfo.agentRatio  = dbAgentRatio/1000.0;
                    m_GameRoomInfo.agentId = m_proxyId ;
                    m_GameRoomInfo.currency = m_currency;
                    LOG_WARN << " >>> totalStock:" << dbTotalStock;
                    LOG_WARN << " >>> totalStockLowerLimit:" << dbTotalStockLowerLimit;
                    LOG_WARN << " >>> totalStockHighLimit:" << dbTotalStockHighLimit;

                    //        m_GamePlayKind.nServerStatus    = dbGameKindStatus;         //
                    //        m_GamePlayKind.bisDynamicJoin   = dbDynamicJoin;            //
                    //        m_GamePlayKind.bisAutoReady     = dbAutoReady;              //
                    //        m_GamePlayKind.bisEnterIsReady  = dbIsEnterIsReady;         //
                    //        m_GamePlayKind.bisQipai         = dbQipai;                  //
                    //        m_GamePlayKind.bisKeepAndroidin = dbKeepAndroidIn;          //
                    //        m_GamePlayKind.bisLeaveAnyTime  = dbLeaveAnyTime;           //
                    //        m_GamePlayKind.nServerStatus    = SERVER_STAT_ENABLE;       // enable.

                    tagStockInfo stockinfo = {0};
                    stockinfo.nStorageControl = dbTotalStock;
                    stockinfo.nStorageLowerLimit = dbTotalStockLowerLimit;
                    //        stockinfo.nAndroidStorage = nAndroidStock;
                    //        stockinfo.nAndroidStorageLowerLimit = nAndroidStockLowerLimit;
                    stockinfo.wSystemAllKillRatio = dbSystemKillAllRatio;
                    CGameTableManager::get_mutable_instance().SetTableStockInfo(stockinfo);

                    CAndroidUserManager::get_mutable_instance().SetAndroidUserCount(dbAndroidCount, dbAndroidMaxUserCount);

                    LOG_WARN << " >>> bisEnableAndroid:" << dbBEnableAndroid;
                }
        }
    }
    }
    catch (exception &e)
    {
        LOG_ERROR << "MongoDB exception: " << e.what();
    }
    return bFind;
}

bool GameServer::LoadLibraryEx(PFNCreateTableSink &CreateTableFrameSink)
{
    //开始加载机器人模块
    char path[1024] = {0};
    if (!getcwd(path, sizeof(path)))
    {
        strcpy(path, ".");
    }
    // build the full android path.
    string strDllPath = path;
    strDllPath.append("/");

    // std::string strDllName(pGameKind->szServerDLL);
    string strDllName = GlobalFunc::clearDllPrefix(m_GameInfo.gameServiceName);
    // try to build the full android dll file name.
    strDllName.insert(0, "./lib");
    strDllName.append(".so");
    strDllName.insert(0, strDllPath);

    LOG_DEBUG << "Load Game Logic so:" << strDllName;
    // try to load the special library from the database.
    void *lib_dl = dlopen(strDllName.c_str(), RTLD_LAZY);
    if (!lib_dl)
    {
        LOG_ERROR << "==dlopen error=====" << dlerror();
        return false;
    }

    // try to get the special function pointer for create the special table frame sink item data now.
    CreateTableFrameSink = (shared_ptr<ITableFrameSink>(*)(void))dlsym(lib_dl, PFNNAME_CREATE_SINK);
    if (!CreateTableFrameSink)
    {
        dlclose(lib_dl);
        LOG_ERROR << "==dlsym function: " << PFNNAME_CREATE_SINK << " error=====" << dlerror();
        return false;
    }

    return true;
}

bool GameServer::InitServer(uint32_t gameId, uint32_t roomId)
{

    PFNCreateTableSink pfnCreateSink = nullptr;
    if ((LoadGameRoomKindInfo(gameId, roomId)) &&
        LoadLibraryEx(pfnCreateSink))
    {
        // initialize the special room kind value content data item now.
        CServerUserManager::get_mutable_instance().Init(&m_GameRoomInfo);
        CGameTableManager::get_mutable_instance().InitAllTable(m_GameRoomInfo.tableCount, &m_GameInfo, &m_GameRoomInfo, pfnCreateSink, m_game_logic_thread,m_game_sqldb_thread, this);

        //LOG_WARN << " >>> InitAllTable called, nTableCount:" << m_GameRoomInfo.tableCount;
        // load benefit config.
        //        LoadBenefitConfig();
        // check if current is enable android.
        if (m_GameRoomInfo.bEnableAndroid)
        {
            CAndroidUserManager::get_mutable_instance().Init(&m_GameInfo, &m_GameRoomInfo, this);
            CAndroidUserManager::get_mutable_instance().LoadAndroidParam(m_proxyId,m_currency);

            //            LoadAndroidParam();
            //            m_timer_thread->getLoop()->runEvery(TIME_RELOADPARAM, bind(&GameServer::OnReLoadParameter, this));
            //            m_game_logic_thread->getLoop()->runEvery(3, bind(&CAndroidUserManager::OnTimerCheckUserIn, &(CAndroidUserManager::get_mutable_instance())));
        }
        else
        {
            //LOG_WARN << "LoadLibrary android disabled!, bisEnableAndroid:" << m_GameRoomInfo.bEnableAndroid;
        }

        //        // initialize the time value content.
        //        gettimeofday(&g_lastTickTime, NULL);
        //        g_fTickSysClock = 1.0f;
        //        if(m_GamePlayKind.bisDynamicJoin)
        //        {
        //            g_fTickSysClock = 0.1f;
        //        }

        // GAME_BR tick interval is 100 ms. IDI_TIME_SYS_CLOCK
        // try to initialize the special timer interval for one second tick arrived data content now.
        //        m_server.getLoop()->runEvery(g_fTickSysClock,  bind(&GameServer::OnTimerTick, this));
        //        LOG_INFO << " >>> OnTimeTick interval:" << g_fTickSysClock;

        // check if current is enable android.
        //        if(m_GamePlayKind.bisEnableAndroid)
        //        {
        // check the android data in.
        //            double fTickAndroid   = 1.0f;
        //            double fTickAndroidIn = 3.0f;
        //            if(m_GamePlayKind.bisDynamicJoin)
        //            {
        //                fTickAndroid  = 0.1f;
        //            }

        //#if 0
        //            // ddz qipai, load interval is.
        //            if (m_GamePlayKind.bisQipai)
        //            {
        //                fTickAndroidIn= 2.0f; // modify by James for test only.
        //            }
        //#endif

        //            // run the special load android tick for insert all the android user to the game scene value now.
        //            m_server.getLoop()->runEvery(fTickAndroid,   bind(&GameServer::OnAndroidTick,    this));
        //            m_server.getLoop()->runEvery(fTickAndroidIn, bind(&GameServer::OnAndroidCheckIn, this));
        //            LOG_INFO << " >>> OnAndroidTick interval:" << fTickAndroid << ", OnAndroidCheckIn interval:" << fTickAndroidIn;

        // try to run the special time tick to reload the special parameter from the database of ini files.
        //			m_server.getLoop()->runEvery(TIME_RELOADPARAM, bind(&GameServer::OnReLoadParameter, this));
        //        }

        return true;
    }
    else
    {
        return false;
    }
}

void GameServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        TcpConnectionWeakPtr weakPtr(conn);
        {
            WRITE_LOCK(m_IPProxyMapMutex);
            // m_IPProxyMap[conn->peerAddress().toIp()] = weakPtr;
            m_IPProxyMap[conn->peerAddress().toIpPort()] = weakPtr;
        }
        uint32_t num = m_connection_num.incrementAndGet();
        LOG_INFO << __FUNCTION__ << " --- *** "
                 << "网关服[" << conn->peerAddress().toIpPort() << "] -> 游戏服["
                 << conn->localAddress().toIpPort() << "] "
                 << (conn->connected() ? "UP" : "DOWN") << " " << num;
    }
    else
    {
        {
            WRITE_LOCK(m_IPProxyMapMutex);
            // m_IPProxyMap.erase(conn->peerAddress().toIp());
            m_IPProxyMap.erase(conn->peerAddress().toIpPort());
        }

        uint32_t num = m_connection_num.decrementAndGet();
        LOG_INFO << __FUNCTION__ << " --- *** "
                 << "网关服[" << conn->peerAddress().toIpPort() << "] -> 游戏服["
                 << conn->localAddress().toIpPort() << "] "
                 << (conn->connected() ? "UP" : "DOWN") << " " << num;
    }
}

void GameServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp receiveTime)
{
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
        // FIXME: use Buffer::peekInt32()
        const uint16_t len = buf->peekInt16();
        if (unlikely(len > PACKET_SIZE) || (len < sizeof(int16_t)))
        {
            LOG_ERROR << "Invalid length " << len;
            if (conn)
                conn->shutdown(); // FIXME: disable reading
            break;
        }
        else if (likely(buf->readableBytes() >= len))
        {
            BufferPtr buffer(new muduo::net::Buffer(len));
            buffer->append(buf->peek(), static_cast<size_t>(len));
            buf->retrieve(len);
            TcpConnectionWeakPtr weakPtr(conn);
            //            m_thread_pool.run(bind(&GameServer::processRequest, this, weakPtr, buffer, receiveTime));
            m_game_logic_thread->getLoop()->runInLoop(bind(&GameServer::processRequest, this, weakPtr, buffer, receiveTime));
        }
        else
        {
            break;
        }
    }
}

void GameServer::processRequest(TcpConnectionWeakPtr &weakConn,
                                BufferPtr &buf,
                                muduo::Timestamp receiveTime)
{
    int32_t size = buf->readableBytes();
    if (size < sizeof(internal_prev_header) + sizeof(Header))
    {
        LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER IS NULL !!!!!!!";
        return;
    }

    internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

    string session(pre_header->session, sizeof(pre_header->session));
    int64_t userId = pre_header->userId;
    string aesKey(pre_header->aesKey, sizeof(pre_header->aesKey));

    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
    int headLen = sizeof(internal_prev_header) + sizeof(Header);
    uint16_t crc = GlobalFunc::GetCheckSum((uint8_t *)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);
    if (commandHeader->len == size - sizeof(internal_prev_header) && commandHeader->crc == crc && commandHeader->ver == PROTO_VER && commandHeader->sign == HEADER_SIGN)
    {
        TRACE_COMMANDID(commandHeader->mainId, commandHeader->subId);
        switch (commandHeader->mainId)
        {
        case Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER:
        case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
        case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
        {
            switch (commandHeader->encType)
            {
            case PUBENC__PROTOBUF_NONE:
            {

                int mainId = commandHeader->mainId;
                int subId = commandHeader->subId;

                int id = mainId << 8;
                if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER || commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    id = id | subId;

                if (likely(m_functor_map.count(id)))
                {

                    AccessCommandFunctor functor = m_functor_map[id];
                    functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                }
                else
                {
                    LOG_WARN << "Not Define Command: mainId=" << mainId << " subId=" << subId;
                }
                break;
            }
            case PUBENC__PROTOBUF_RSA: //RSA
            {
                break;
            }
            case PUBENC__PROTOBUF_AES: //AES
            {
                break;
            }
            default:
                break;
            }
        }
        break;
        case Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER:
        {
            switch (commandHeader->encType)
            {
            case PUBENC__PROTOBUF_NONE:
            {
                int mainId = commandHeader->mainId;
                int subId = commandHeader->subId;
                int id = mainId << 8;
                if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER || commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    id = id | subId;

                if (likely(m_functor_map.count(id)))
                {
                    AccessCommandFunctor functor = m_functor_map[id];
                    functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                }
                else
                {
                    LOG_WARN << "Not Define Command: mainId=" << mainId << " subId=" << subId;
                }
                break;
            }
            case PUBENC__PROTOBUF_RSA: //RSA
            {

                break;
            }
            case PUBENC__PROTOBUF_AES: //AES
            {

                break;
            }
            default:
                break;
            }
        }
        break;
        case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
        case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER MAINID ERROR commandHeader->mainId = " << commandHeader->mainId;
            break;
        }
    }
    else
    {
        LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER LEN ERROR OR CRC ERROR! header->len=" << commandHeader->len
                  << " bufferLen=" << size - sizeof(internal_prev_header) << " header->crc=" << commandHeader->crc << " buffer crc=" << crc;
    }
}

boost::tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>> GameServer::GetProxyConnectionWeakPtrFromUserId(int64_t userId)
{
    string strProxy;
    shared_ptr<UserProxyConnectionInfo> userProxyConnInfo;
    shared_ptr<internal_prev_header> header;
    TcpConnectionWeakPtr weakConn;

    if (userId > 0)
    {
        {
            READ_LOCK(m_UserIdProxyConnectionInfoMapMutex);
            auto it = m_UserIdProxyConnectionInfoMap.find(userId);
            if (it != m_UserIdProxyConnectionInfoMap.end())
            {
                userProxyConnInfo = it->second;
                strProxy = userProxyConnInfo->strProxyIP;
            }
        }
        if (!strProxy.empty())
        {
            READ_LOCK(m_IPProxyMapMutex);
            auto it = m_IPProxyMap.find(strProxy);
            if (it != m_IPProxyMap.end())
            {
                weakConn = it->second;
            }
        }
        if (userProxyConnInfo && userProxyConnInfo->internal_header)
            header = userProxyConnInfo->internal_header;
    }

    return boost::make_tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>>(weakConn, header);
}

bool GameServer::sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data,
                          internal_prev_header *internal_header, Header *commandHeader)
{
    //LOG_INFO << __FILE__ << __FUNCTION__;
    bool bSuccess = true;
    TcpConnectionPtr conn(weakConn.lock());
    if (likely(conn))
    {
        int size = data.size();
        internal_prev_header *send_internal_header = (internal_prev_header *)(&data[0]);
        memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
        send_internal_header->len = size;

        Header *commandSendHeader = (Header *)(&data[sizeof(internal_prev_header)]);
        memcpy(commandSendHeader, commandHeader, sizeof(Header));
        commandSendHeader->encType = PUBENC__PROTOBUF_NONE;
        commandSendHeader->len = size - sizeof(internal_prev_header);
        commandSendHeader->realSize = size - sizeof(internal_prev_header) - sizeof(Header);
        commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);

        TRACE_COMMANDID(commandSendHeader->mainId, commandSendHeader->subId);
        conn->send(&data[0], data.size());
    }
    else
    {
        // check userid.
        int64_t userId = 0;
        if (internal_header)
        {
            userId = internal_header->userId;
        }

        LOG_WARN << "GameServer::sendData conn(weakConn.lock()) failed, userId:" << userId;
    }
    return (bSuccess);
}

bool GameServer::GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId)
{
    string value;
    bool ret = REDISCLIENT.get(session, value);
    if (ret)
    {
        Json::Reader reader;
        Json::Value root;
        if (reader.parse(value, root))
        {
            userId = root["userid"].asInt64();
            account = root["account"].asString();
            agentId = root["agentid"].asUInt();
            return true;
        }
    }
    return false;
}

//=====================================work thread=================================
void GameServer::cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    // LOG_DEBUG << " "  << __FUNCTION__ ;
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES;
    ::Game::Common::KeepAliveMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        string session = msg.session();

        ::Game::Common::KeepAliveMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        // 如果是玩家则处理
        if (!session.empty()) {
            int32_t ret = 0;
            int64_t userId = 0;
            string account;
            uint32_t agentId = 0;

            if (GetUserLoginInfoFromRedis(session, userId, account, agentId))
            {
                if (REDISCLIENT.ResetExpiredUserOnlineInfo(userId)) {
                    response.set_errormsg("KEEP ALIVE PING OK.");
                }
                else {
                    ret = 2;//response.set_retcode(2);
                    response.set_errormsg("KEEP ALIVE PING Error UserId Not Find!");
                }
            }
            else {
                ret = 1;//response.set_retcode(1);
                response.set_errormsg("KEEP ALIVE PING Error Session Not Find!");
            }

            response.set_retcode(ret);
            size_t len = response.ByteSizeLong();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len)){
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }
}
bool GameServer::SetBlackeListInfo(string insert,map<string,int16_t> &usermap)
{
    usermap.clear();
    vector<string> vec;
    vec.clear();
    if(insert=="")
    {
        //LOG_ERROR<<"balcklist has no key";
        return false;
    }
    string covers=insert;
    covers.erase(std::remove(covers.begin(), covers.end(), '"'), covers.end());
    boost::algorithm::split( vec,covers, boost::is_any_of( "|" ));
    if(vec.size()==0)
    {
        LOG_ERROR<<"balcklist vec.size()==0 has no key";
    }
    for(int i=0;i<(int)vec.size();i++)
    {
        vector<string> vec1;
        vec1.clear();
        boost::algorithm::split( vec1,vec[i] , boost::is_any_of( "," ));
        if(vec1.size()==2)
        {
            usermap.insert(pair<string,int16_t>(vec1[0],stoi(vec1[1])));
        }
    }
    return true;
}
void GameServer::cmd_enter_room(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::GameServer::SUB_S2C_ENTER_ROOM_RES;
    ::GameServer::MSG_C2S_UserEnterMessage msg;
    LOG_INFO << "user enter room  subId:" << commandHeader->subId << "    realSize:" << commandHeader->realSize << "   msgdataLen:" << msgdataLen;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        int64_t userId = internal_header->userId;
        uint32_t userIp = internal_header->ip;
        do
        {
            ::Game::Common::Header header = msg.header();

            ::GameServer::MSG_S2C_UserEnterMessageResponse response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            uint32_t gameId = msg.gameid();
            uint32_t roomId = msg.roomid();
            string password = msg.dynamicpassword();
			uint32_t tableId = INVALID_TABLE;
			string strTableId = msg.strtableid();
			vector<string> strVec;
			strVec.clear();
			boost::algorithm::split(strVec, strTableId, boost::is_any_of("-"));
			if (strVec.size() == 2)
                tableId = stoi(strVec[1]) - 1;

            LOG_INFO << ">>> gameId:" << gameId << " roomId:" << roomId << " tableId:" << tableId << " userid:" << userId;

            response.set_retcode(0);
            response.set_errormsg("InitError!");


            string currency="0";
            REDISCLIENT.GetCurrencyUser(userId,REDIS_UID_CURRENCY,currency);
            int64_t currenNum = atol(currency.c_str());

            if(currenNum!=m_currency)
            {
                LOG_ERROR << "玩家币种和服务器不匹配. userid:" << userId;
                //                    response.set_retcode(::GameServer::Message::LoginGameServerMessageResponse::LOGIN_RESTART);
                //                    response.set_errormsg("User LoginInfo TimeOut, Restart Login.");
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_GAME_IS_END, "Currency Not Exists!", internal_header, commandHeader);
                break;
            }
            // 判断玩家是否在线
            if (REDISCLIENT.ExistsUserOnlineInfo(userId))
            {
                string dynamicPassword;
                if (REDISCLIENT.GetUserLoginInfo(userId, REDIS_DYNAMIC_PWD, dynamicPassword))
                {
                    string strUserPassword = Landy::Crypto::BufferToHexString((unsigned char *)password.c_str(), password.size());
                    if (dynamicPassword == strUserPassword)
                    {
                        //LOG_WARN << ">>> cmd_enter_room dynamicPassword == strUserPassword ok, userid[" << userId << "],password[" << strUserPassword << "]";
                        {
                            TcpConnectionPtr conn(weakConn.lock());
                            internal_prev_header *head = new internal_prev_header();
                            memcpy(head, internal_header, sizeof(internal_prev_header));
                            shared_ptr<internal_prev_header> shared_iternal(head);
                            shared_ptr<UserProxyConnectionInfo> userProxyConnInfo(new UserProxyConnectionInfo());
                            // userProxyConnInfo->strProxyIP = conn->peerAddress().toIp();
                            userProxyConnInfo->strProxyIP = conn->peerAddress().toIpPort();
                            userProxyConnInfo->internal_header = shared_iternal;
                            {
                                WRITE_LOCK(m_UserIdProxyConnectionInfoMapMutex);
                                m_UserIdProxyConnectionInfoMap[userId] = userProxyConnInfo;
                            }
                        }
                    }
                    else
                    {
                        LOG_ERROR << "user password Error, LOGIN_PASSWORD_ERCOR. userid : " << userId << ", client:" << strUserPassword << ", stored:" << dynamicPassword;
                        //                            response.set_retcode(::GameServer::Message::LoginGameServerMessageResponse::LOGIN_PASSWORD_ERCOR);
                        //                            response.set_errormsg("User Password Error.");
                        SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_PASSWORD_ERROR, "Password Error!", internal_header, commandHeader);
                        break;
                    }
                }
                else
                {
                    LOG_ERROR << "m_redisClient get account Error, LOGIN_ACCOUNTS_NOT_EXIST.";
                    //                        response.set_retcode(::GameServer::Message::LoginGameServerMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
                    //                        response.set_errormsg("User Account Not Exist Error.");
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_NOSESSION, "User Login Info Not Exists!", internal_header, commandHeader);
                    break;
                }
            }
            else
            {
                LOG_ERROR << "m_redisClient existsUserIdInfo Error on login, LOGIN_RESTART. userid:" << userId;
                //                    response.set_retcode(::GameServer::Message::LoginGameServerMessageResponse::LOGIN_RESTART);
                //                    response.set_errormsg("User LoginInfo TimeOut, Restart Login.");
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_GAME_IS_END, "Room Info Not Exists!", internal_header, commandHeader);
                break;
            }

            // find if user already exist.
            {
                shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
                if (pIServerUserItem && pIServerUserItem->isValided())
                {
                    LOG_DEBUG << " >>> cmd_enter_room, find exist old pIServerUserItem, do SwitchUserItem. userId:" << userId;
                    shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());
                    //断线重连进入游戏判断
                    if (pITableFrame && pITableFrame->CanJoinTable(pIServerUserItem))
                    {
                        if (!SwitchUserItem(pIServerUserItem, weakConn, internal_header, commandHeader))
                        {
                            SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_USERNOTEXIST, "Enter Room Error!", internal_header, commandHeader);
                            return;
                        }
                    }
                    else
                    {
                        LOG_WARN << "user can't enter room, because ERROR_ENTERROOM_GAME_IS_END, userId:" << userId;
                        SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_GAME_IS_END, "ERROR_ENTERROOM_GAME_IS_END!", internal_header, commandHeader);
                        return;
                    }
                    //LOG_DEBUG << "after SwitchUserItem(pIServerUserItem, weakConn, internal_header)";

                    //LOG_WARN << "void GameServer::cmd_enter_room Finished";
                    break;
                }
            }

            //卡线检测.
            bool bCanEnter = CheckUserCanEnterRoom(userId, weakConn, internal_header, commandHeader, tableId);
            if (!bCanEnter)
            {
                LOG_WARN << " >>> user can't enter room, because in other room, userId:" << userId;
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_PLAY_IN_OTHERROOM, ERROR_ENTERROOM_USERINGAME, "User In Other Room!", internal_header, commandHeader);
                break;
            }

            GSUserBaseInfo userInfo;
            // get user base info value.
            if (weakConn.lock())
            {
                bool bExist = GetUserBaseInfo(userId, userInfo);
                if (!bExist)
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    LOG_DEBUG << " >>> can't find user base info, userId:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_USERNOTEXIST, "In DB,UserId Not Exists!", internal_header, commandHeader);
                    break;
                }
            }
            else
            {
                REDISCLIENT.DelUserOnlineInfo(userId);
                LOG_DEBUG << " >>> weakConn.lock() == NULL, user id:" << userId;
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_NOSESSION, "Android Cannot Login!", internal_header, commandHeader);
                break;
            }

            //是否满足最小进入条件.
            if (m_GameRoomInfo.enterMinScore > 0 && userInfo.userScore < m_GameRoomInfo.enterMinScore)
            {
                REDISCLIENT.DelUserOnlineInfo(userId);
                LOG_DEBUG << " >>> enter room failed, reason: nUserScore < GetUserEnterMinScore(). userId:" << userId;
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_SCORENOENOUGH, "nUserScore < GetUserEnterMinScore()", internal_header, commandHeader);
                break;
            }

            //最大进入条件.
            if (m_GameRoomInfo.enterMaxScore > 0 && userInfo.userScore > m_GameRoomInfo.enterMaxScore)
            {
                REDISCLIENT.DelUserOnlineInfo(userId);
                LOG_DEBUG << " >>> enter room failed, reason: nUserScore > Room UserEnterMaxScore(). userId:" << userId;
                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_SCORELIMIT, "nUserScore > Room UserEnterMaxScore()", internal_header, commandHeader);
                break;
            }

            // lock the special table item now.
            {
                bool inQuarantine = REDISCLIENT.exists(REDIS_QUARANTINE + to_string(userId));
                shared_ptr<CTableFrame> pTableFrame;

                shared_ptr<CServerUserItem> pIServerUserItem;
                pIServerUserItem = CServerUserManager::get_mutable_instance().GetUserItem(userId);
                if (!pIServerUserItem)
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    LOG_ERROR << "insert or create user item failed!, userId:" << userId;
                    break;
                }
                
                REDISCLIENT.GetMatchBlockList(userId, pIServerUserItem->m_sMthBlockList);
                pIServerUserItem->inQuarantine = inQuarantine;
                if (m_GameInfo.matchforbids[MTH_BLACKLIST])
                {
                    redis::RedisValue status;

                    string fields[5] = {REDIS_FIELD_STATUS, REDIS_FIELD_RATE, REDIS_FIELD_TOTAL, REDIS_FIELD_CONTROL, REDIS_FIELD_CURRENT};
                    bool res = REDISCLIENT.hmget(REDIS_BLACKLIST + to_string(userId), fields, 5, status);

                    if (res)
                    {
                        LOG_INFO << "user  is blacklist";
                        SetBlackeListInfo(status[REDIS_FIELD_CONTROL].asString(), pIServerUserItem->GetBlacklistInfo().listRoom);
                        auto it = pIServerUserItem->GetBlacklistInfo().listRoom.find(to_string(gameId));
                        if (it != pIServerUserItem->GetBlacklistInfo().listRoom.end())
                        {
                            pIServerUserItem->GetBlacklistInfo().weight = it->second; //
                        }
                        else
                        {
                            pIServerUserItem->GetBlacklistInfo().weight = 0;
                        }
                        pIServerUserItem->GetBlacklistInfo().status = status[REDIS_FIELD_STATUS].asInt();
                        //pIServerUserItem->GetBlacklistInfo().weight = status[REDIS_FIELD_RATE].asInt();
                        pIServerUserItem->GetBlacklistInfo().total = status[REDIS_FIELD_TOTAL].asLong();
                        pIServerUserItem->GetBlacklistInfo().current = status[REDIS_FIELD_CURRENT].asLong();
                    }
                    else
                    {
                        LOG_ERROR << "user  is  not  blacklist";
                        pIServerUserItem->GetBlacklistInfo().status = 0;
                    }
                }
                else
                {
                    pIServerUserItem->GetBlacklistInfo().status = 0;
                }

                //获取炸金花黑屋控制参数
                if (gameId == (int64_t)eGameKindId::zjh)
                {
                    redis::RedisValue status;
                    string fields[4] = {"current", "controlTimes", "roomWin", "status"};
                    bool res = REDISCLIENT.hmget(REDIS_BLACKROOM + to_string(userId) + "_" + to_string(roomId), fields, 4, status);
                    // bool res=REDISCLIENT.BlackRoomListHget(REDIS_BLACKROOMLIST+to_string(userId),"roomcontrol:"+to_string(roomId),status,pIServerUserItem->GetBlackRoomlistInfo().blackroom_Info);
                    if (res)
                    {
                        pIServerUserItem->GetBlackRoomlistInfo().current = status["current"].asInt();
                        pIServerUserItem->GetBlackRoomlistInfo().controlTimes = status["controlTimes"].asInt();
                        pIServerUserItem->GetBlackRoomlistInfo().roomWin = status["roomWin"].asInt();
                        pIServerUserItem->GetBlackRoomlistInfo().status = status["status"].asInt();
                    }
                    else
                    {
                        pIServerUserItem->GetBlackRoomlistInfo().status = 0;
                    }
                }

                userInfo.ip = internal_header->ip;
                pIServerUserItem->SetUserBaseInfo(userInfo);
                // 个人库存
                PersonalProfit(pIServerUserItem);

                pTableFrame = dynamic_pointer_cast<CTableFrame>(CGameTableManager::get_mutable_instance().FindNormalSuitTable(pIServerUserItem, INVALID_TABLE, tableId));
                if (!pTableFrame)
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    LOG_INFO << " >>> user can't enter room, because table all full, userId:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, ::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_TABLE_FULL, "table all full", internal_header, commandHeader);
                    break;
                }

                bool bDistribute = false;
                if (pTableFrame->RoomSitChair(pIServerUserItem, commandHeader, bDistribute))
                {
                    // 新旧水果机只记录一个水果机记录
                    int64_t _gameid = m_GameRoomInfo.gameId;
                    if (_gameid == (int64_t)eGameKindId::xsgj)
                    {
                        _gameid = (int64_t)eGameKindId::sgj;
                    }

                    // 记录游戏数量2592000
                    int timeout = 1728000; //20天过期时间
                    std::string fieldName = std::to_string(_gameid);
                    std::string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_incr_gameTime) + "_" + std::to_string(userId);
                    if (REDISCLIENT.hset(keyName, fieldName, std::to_string(time(nullptr)), timeout))
                    {
                        LOG_INFO << "---hset gameTime keyName[" << keyName << "],fieldName[" << fieldName << "],Count[" << time(nullptr) << "]";
                    }
                }
            }
        } while (0);
    }
    else
    {
        LOG_DEBUG << "command: (100, 2) cmd_enter_room parse data failed!";
    }
}

void GameServer::PersonalProfit( shared_ptr<CServerUserItem> & pIServerUserItem )
{
    // 获取引用
    tagPersonalProfit &_personalProfit = pIServerUserItem->GetPersonalPrott();

    if (!m_GameInfo.matchforbids[MTH_PERSONALSTOCK])
    {
        _personalProfit.clear();
        return;
    }

    // bool res = false, res1 = false;
    // string totalbet="0", totalprofit="0", agenttendayPro="0";
    // string key = REDIS_SCORE_PREFIX + to_string(pIServerUserItem->GetUserId()) + to_string(m_GameRoomInfo.roomId);
    // res = REDISCLIENT.hget(key, REDIS_PERSONALBET, totalbet);
    // res = REDISCLIENT.hget(key, REDIS_PERSONALPRO, totalprofit);
    // key = REDIS_AGENT_INFO + to_string(pIServerUserItem->GetUserBaseInfo().agentId);
    // //res1 = REDISCLIENT.hget(key, REDIS_AGENTPRO, agenttendayPro);
    // if (res) // && res1)
    // {

    string key = REDIS_SCORE_PREFIX + to_string(pIServerUserItem->GetUserId()) + to_string(m_GameRoomInfo.roomId);

    int64_t totalbet = 0, totalprofit = 0, agenttendayPro = 0;

    string fields[2] = {REDIS_PERSONALBET, REDIS_PERSONALPRO};
    redis::RedisValue values;
    bool ret = REDISCLIENT.hmget(key, fields, 2, values);

    if (ret && !values.empty())
    {
        totalbet = values[REDIS_PERSONALBET].asInt64();
        totalprofit = values[REDIS_PERSONALPRO].asInt64();

        LOG_INFO << ">>> " << __FUNCTION__ << " UserId[" << pIServerUserItem->GetUserId() << "],totalbet[" << totalbet << "],totalprofit[" << totalprofit << "]";

        _personalProfit.playerAllProfit = totalprofit;//stod(totalprofit);      //get from redis
        _personalProfit.agentTendayProfit = agenttendayPro;//stod(agenttendayPro); //get from redis
        
        if (_personalProfit.agentTendayProfit < 1000 * COIN_RATE)//00) //设计要这样子,小于1000 就按1000算
        {
            _personalProfit.agentTendayProfit = 1000 * COIN_RATE;//  00;
        }
         
        _personalProfit.playerALlbet = totalbet;//stod(totalbet); //get from redis
        _personalProfit.agentRatio = m_GameRoomInfo.agentRatio;
        _personalProfit.playerBaseValue = m_GameRoomInfo.personalInventory;
        _personalProfit.playerBetLoseRatio = m_GameRoomInfo.betLowerLimit;
        _personalProfit.playerBetWinRatio = m_GameRoomInfo.betHighLimit;
        _personalProfit.playerInterferenceRatio = m_GameRoomInfo.personalratio;
        _personalProfit.isOpenPersonalAction = 0;

        _personalProfit.playerHighLimit = _personalProfit.playerALlbet * _personalProfit.playerBetWinRatio + _personalProfit.playerBaseValue + _personalProfit.agentTendayProfit * _personalProfit.agentRatio;
        _personalProfit.playerLowerLimit = _personalProfit.playerALlbet * _personalProfit.playerBetLoseRatio - _personalProfit.playerBaseValue - _personalProfit.agentTendayProfit * _personalProfit.agentRatio;

        LOG_ERROR << "playerALlbet: " << totalbet << "agentRatio: " << m_GameRoomInfo.agentRatio
                  << "playerBaseValue :" << m_GameRoomInfo.personalInventory << "playerBetLoseRatio"
                  << m_GameRoomInfo.betLowerLimit << "playerBetWinRatio:" << m_GameRoomInfo.betHighLimit
                  << "playerInterferenceRatio:" << m_GameRoomInfo.personalratio
                  <<"_personalProfit.playerAllProfit:"<<_personalProfit.playerAllProfit;

        if (_personalProfit.playerAllProfit > _personalProfit.playerHighLimit)
        {
            int64_t fanweizhi = _personalProfit.playerHighLimit - _personalProfit.playerLowerLimit;
            if (fanweizhi == 0)
                fanweizhi = 1;
            if ((double)(_personalProfit.playerAllProfit - _personalProfit.playerHighLimit) / (double)fanweizhi > 1)
            {
            }
            else
            {
                _personalProfit.playerInterferenceRatio = _personalProfit.playerInterferenceRatio * ((double)(_personalProfit.playerAllProfit - _personalProfit.playerHighLimit) / (double)fanweizhi);
            }
            _personalProfit.isOpenPersonalAction = 1; //玩家高于上限,杀分
        }
        if (_personalProfit.playerAllProfit < _personalProfit.playerLowerLimit)
        {
            int64_t fanweizhi = _personalProfit.playerHighLimit - _personalProfit.playerLowerLimit;
            if (fanweizhi == 0)
                fanweizhi = 1;
            if ((double)(_personalProfit.playerLowerLimit - _personalProfit.playerAllProfit) / (double)fanweizhi > 1)
            {
            }
            else
            {
                _personalProfit.playerInterferenceRatio = _personalProfit.playerInterferenceRatio * ((double)(_personalProfit.playerLowerLimit - _personalProfit.playerAllProfit) / (double)fanweizhi);
            }
            _personalProfit.isOpenPersonalAction = -1; //玩家低于下限了,放分
        }
        LOG_ERROR << "_personalProfit.playerInterferenceRatio:" << _personalProfit.playerInterferenceRatio
                  << "_personalProfit.isOpenPersonalAction:"<<_personalProfit.isOpenPersonalAction;
    }
    else
    {
        //读不到就不起作用
        _personalProfit.clear();
    }
}

bool GameServer::SendGameErrorCode(TcpConnectionWeakPtr &weakConn, uint8_t mainId, uint8_t subId, uint32_t nRetCode, string errMsg,
                                   internal_prev_header *internal_header, Header *commandHeader)
{
    bool bSuccess = false;
    do
    {
        ::GameServer::MSG_S2C_UserEnterMessageResponse response;
        Game::Common::Header *header = response.mutable_header();
        header->set_sign(HEADER_SIGN);
        response.set_retcode(nRetCode);
        response.set_errormsg(errMsg);

        commandHeader->mainId = mainId;
        commandHeader->subId = subId;

        size_t len = response.ByteSizeLong();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
            bSuccess = false;
        }
        else
        {
            LOG_ERROR << " >>> SendGameErrorCode serial to array failed.";
        }

    } while (0);
    //Cleanup:
    return (bSuccess);
}

bool GameServer::SwitchUserItem(shared_ptr<CServerUserItem> &pIServerUserItem,
                                TcpConnectionWeakPtr &conn,
                                internal_prev_header *internal_header, Header *commandHeader)
{
    //LOG_DEBUG << __FILE__ <<__FUNCTION__;
    do
    {
        if (!pIServerUserItem)
        {
            LOG_DEBUG << __FILE__ << __FUNCTION__ << "pIServerUserItem == NULL";
            break;
        }

        //LOG_DEBUG << __FILE__ << __FUNCTION__ << "TABLEID="<<pIServerUserItem->GetTableId();

        shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());
        //        if(pITableFrame && pITableFrame->CanJoinTable(0))
        //        {
        //            MutexLockGuard lock(pITableFrame->m_table_mutex);
        //            muduo::RecursiveLockGuard lock(pITableFrame->m_RecursiveMutextLockEnterLeave);

        if (!pIServerUserItem || pIServerUserItem->GetUserId() <= 0 || pIServerUserItem->GetChairId() == INVALID_CHAIR)
        {
            LOG_DEBUG << __FILE__ << __FUNCTION__ << "pIServerUserItem == NULL OR pIServerUserItem->GetUserID() <= 0";
            return false;
        }
        pIServerUserItem->SetTrustship(false);
        pIServerUserItem->setClientReady(true);

        //            shared_ptr<IServerUserItem> pItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);
        pITableFrame->OnUserEnterAction(pIServerUserItem, false);

        //            if(m_GamePlayKind.bisQipai && (pITableFrame->GetGameStatus() == GAME_STATUS_FREE))
        //            {
        //                if(pITableFrame->CheckGameStart())
        //                {
        //                    pITableFrame->GameRoundStartDeploy();

        //                    // try to start the game.
        //                    pITableFrame->StartGame();
        //                }
        //            }

        int64_t userId = pIServerUserItem->GetUserId();
        uint32_t tableId = pIServerUserItem->GetTableId();
        uint32_t chairId = pIServerUserItem->GetChairId();
        int64_t userScore = pIServerUserItem->GetUserScore();
        //LOG_INFO << ">>> SwitchUserItem userId: " << userId << " at tableId(" << tableId
        //         << "," << chairId << "), score:" << userScore << ", pIServerUserItem:" << pIServerUserItem.get();
        //        }else
        //        {
        //            LOG_DEBUG << __FILE__ << __FUNCTION__ <<"pITableFrame == NULL";
        //        }

    } while (0);
    //Cleanup:
    return true;
}

bool GameServer::CheckUserCanEnterRoom(int64_t userId, TcpConnectionWeakPtr &weakPtr, internal_prev_header *internal_header, Header *commandHeader, uint32_t tableId)
{
    uint32_t redisGameId = 0, redisRoomId = 0, redisTableId = 0;
    if (REDISCLIENT.GetUserOnlineInfo(userId, redisGameId, redisRoomId, redisTableId) )
    {
		if (INVALID_TABLE == redisTableId)
		{
			if (m_GameInfo.gameId != redisGameId || m_GameRoomInfo.roomId != redisRoomId)
			{
				::GameServer::MSG_S2C_PlayInOtherRoom otherroom;
				::Game::Common::Header *header = otherroom.mutable_header();
				header->set_sign(HEADER_SIGN);

				commandHeader->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
				commandHeader->subId = ::GameServer::SUB_S2C_PLAY_IN_OTHERROOM;

				otherroom.set_gameid(redisGameId);
				otherroom.set_roomid(redisRoomId);

				size_t bytesize = otherroom.ByteSizeLong();
				vector<unsigned char> data(bytesize + sizeof(internal_prev_header) + sizeof(Header));
				if (otherroom.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], bytesize))
				{
					sendData(weakPtr, data, internal_header, commandHeader);
				}

				LOG_WARN << "user can't enter room, because in other room, userId:" << userId
					<< ", nGameId:" << redisGameId << ", nRoomId:" << redisRoomId;

				return false;
			}
		} 
		else
		{
             //百家乐逻辑
             if (m_GameInfo.gameType == GameType_BaiRen && m_GameRoomInfo.tableCount > 1 && tableId == INVALID_TABLE)
             {
                //  可能是重拉进游戏
                 return true;
             }

            if (m_GameInfo.gameId != redisGameId || m_GameRoomInfo.roomId != redisRoomId || tableId != redisTableId)
			{
				::GameServer::MSG_S2C_PlayInOtherRoom otherroom;
				::Game::Common::Header *header = otherroom.mutable_header();
				header->set_sign(HEADER_SIGN);

				commandHeader->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
				commandHeader->subId = ::GameServer::SUB_S2C_PLAY_IN_OTHERROOM;

				otherroom.set_gameid(redisGameId);
				otherroom.set_roomid(redisRoomId);

				size_t bytesize = otherroom.ByteSizeLong();
				vector<unsigned char> data(bytesize + sizeof(internal_prev_header) + sizeof(Header));
				if (otherroom.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], bytesize))
				{
					sendData(weakPtr, data, internal_header, commandHeader);
				}

				LOG_WARN << "user can't enter room, because in other room, userId:" << userId
					<< ", nGameId:" << redisGameId << ", nRoomId:" << redisRoomId;

				return false;
			}
		}
        
    }
    return (true);
}

void GameServer::cmd_user_ready(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    int64_t userId = internal_header->userId;
    LOG_DEBUG << __FUNCTION__ << " userId " << userId;

    //    Header *commandHeader = (Header*)msgdata;
    //    commandHeader->subId = ::Game::Common::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES;
    //    ::HallServer::GetGameServerMessage msg;
    //    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    //    {
    //        ::Game::Common::Header header = msg.header();
    //        ::HallServer::GetGameServerMessageResponse response;
    //        ::Game::Common::Header *resp_header = response.mutable_header();
    //        resp_header->CopyFrom(header);

    //        int64_t userId = internal_header->userId;
    //        uint32_t gameId = msg.gameid();
    //        uint32_t roomId = msg.roomid();

    //    ::GameServer::Message::MSG_CS_UserReady ready;
    //    if (ready.ParseFromArray(&msgdata[0], msgdataLen))
    //    {
    //        int32_t userid = ready.userid();
    //        shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userid);
    //        if(pIServerUserItem)
    //        {
    //            word tableid = pIServerUserItem->GetTableID();
    //            shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(tableid);
    //            if(pITableFrame)
    //            {
    //                pITableFrame->SetUserReady(pIServerUserItem->GetChairID());
    //            }
    //        }else
    //        {
    //            LOG_INFO << __FILE__ << __FUNCTION__ << " UserId Is GetOut UserId:"<<to_string(userid);
    //        }
    //    }
}

void GameServer::cmd_user_left(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen,
                               internal_prev_header *internal_header)
{
    LOG_DEBUG << __FUNCTION__;

    Header *commandHeader = (Header *)msgdata;
    commandHeader->subId = ::GameServer::SUB_S2C_USER_LEFT_RES;
    ::GameServer::MSG_C2S_UserLeftMessage msg;
    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        int64_t userId = internal_header->userId;

        ::GameServer::MSG_C2S_UserLeftMessageResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        uint32_t gameId = msg.gameid();
        uint32_t roomId = msg.roomid();
        int32_t type = msg.type();

        response.set_gameid(gameId);
        response.set_roomid(roomId);
        response.set_type(type);

        LOG_WARN << " >>> On User Left userId:" << userId << ", gameId:" << gameId << ",roomId:" << roomId << ",type:" << type;

        shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
        if (pIServerUserItem)
        {
            LOG_WARN << " >>> OnUserLeft userId:" << userId << ", score:" << pIServerUserItem->GetUserScore();
            shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());
            if (pITableFrame)
            {
                //                KickUserIdToProxyOffLine(userId, KICK_GS|KICK_CLOSEONLY);

                bool bRet = false;
                if (pITableFrame->CanLeftTable(userId))
                {   ///特殊处理转盘和任务在游戏中点击时玩家不退出游戏
                    if(type!=4)
                    {
                        bRet = pITableFrame->OnUserLeft(pIServerUserItem, true, false);
                    }
                    if (bRet||type==4)
                    {
                        response.set_retcode(0);
                        response.set_errormsg("OK");
                    }
                    else
                    {
                        response.set_retcode(1);
                        response.set_errormsg("Error");
                    }
                }
                else
                {
                    response.set_retcode(2);
                    response.set_errormsg("正在游戏中，不能离开");
                }
            }
            else
            {
                response.set_retcode(0);
                response.set_errormsg(" >>> OnUserLeft find pITableFrame failed! <<< ");
            }
        }
        else
        {
            response.set_retcode(0);
            response.set_errormsg(" >>> OnUserLeft find user failed! <<< ");
            LOG_ERROR << " >>> OnUserLeft find user failed!, userid:" << userId;
        }

        size_t len = response.ByteSizeLong();
        vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
        if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, data, internal_header, commandHeader);
        }

    }
}

void GameServer::cmd_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    //LOG_DEBUG << __FILE__ << __FUNCTION__;

    //    muduo::MutexLockGuard lock(m_lockEnter);

    int64_t userId = internal_header->userId;

    do
    {
        // LOG_DEBUG << __FUNCTION__ << " userId:"<<userId;

        shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
        if (!pIServerUserItem)
        {
            LOG_INFO << "OnUserOffline, user not exist, skipped. userId:" << userId;
            break;
        }
        //            WRITE_LOCK(pIServerUserItem->m_list_mutex);

        LOG_DEBUG << __FUNCTION__ << "pIServerUserItem userId:" << pIServerUserItem->GetUserId();

        uint32_t table_id = pIServerUserItem->GetTableId();
        shared_ptr<ITableFrame> pTableFrame = CGameTableManager::get_mutable_instance().GetTable(table_id);
        if (pTableFrame)
        {
            //                muduo::RecursiveLockGuard lock(pTableFrame->m_RecursiveMutextLockEnterLeave);
            //                shared_ptr<IServerUserItem> pItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);
            clearUserIdProxyConnectionInfo(userId);
            pTableFrame->OnUserOffline(pIServerUserItem);
        }
        else
        {
            LOG_INFO << "GetTable(table_id); user not exist, skipped. userId:" << userId << " table_id:" << table_id;
            return;
        }
    } while (0);

    //        SavePlayGameTime(userId);
    return;
}

void GameServer::clearUserIdProxyConnectionInfo(int64_t userId)
{
    WRITE_LOCK(m_UserIdProxyConnectionInfoMapMutex);
    m_UserIdProxyConnectionInfoMap.erase(userId);
}

void GameServer::cmd_game_message(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    Header *commandHeader = (Header *)msgdata;
    uint8_t mainId = commandHeader->mainId;
    uint8_t subId = commandHeader->subId;
    ::GameServer::MSG_CSC_Passageway msg;

    if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();

        int64_t userId = internal_header->userId;

        // try to get the pass data value.
        string str = msg.passdata();
        const uint8_t *buf = (const uint8_t *)str.c_str();
        uint32_t byteSize = str.size();

        // try to get the pointer content item value data now try to get the pointer content item value data.
        shared_ptr<CServerUserItem> serverUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
        if (serverUserItem)
        {
            uint32_t tableId = serverUserItem->GetTableId();
            uint32_t chairId = serverUserItem->GetChairId();
            // LOG_INFO << " >>>>>>>> process_game_message chairid:" << chairId;
            shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(tableId);
            if (pITableFrame)
            {
                //                long tick = Utility::gettimetick();
                // try to call the special table frame on game event now.
                pITableFrame->OnGameEvent(chairId, subId, buf, byteSize);
                //                long sub = Utility::gettimetick() - tick;
                //                if(sub >= TIMEOUT_PERFRAME)
                //                {
                //                    LOG_ERROR << " >>> processRequest timeout, OnGameEvent mainid:" << mainId << ", subid:" << subId
                //                              << ", tableid:" << tableId << ", chairid:"  << chairId  << ", userid:" << userid;
                //                }
            }
        }
        else
        {
            LOG_ERROR << " >>> process_game_message find user item failed! userId:" << userId;
        }
    }
}

void GameServer::notifyStopGameServerMessage(string msg)
{
    LOG_DEBUG << "--- *** "
              << "MSG = " << msg;
}

void GameServer::notifyRefreashConfig(string msg)
{
    LOG_WARN << "--- *** "
             << "MSG = " << msg;
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(msg, root))
    {
        uint32_t gameId = root["GameId"].asInt();
        uint32_t roomId = root["RoomId"].asInt();
        if ((!gameId && !roomId) || (gameId == m_gameId && roomId == m_roomId))
        {
            LOG_WARN << __FUNCTION__ << "  " << msg;
            if (LoadGameRoomKindInfo(m_gameId, m_roomId))
            {
                // 更新库存
                m_game_logic_thread->getLoop()->runInLoop([&]() {

                    int rooomid=m_GameRoomInfo.roomId;
                    //特殊处理新水果机和经典水果机的共同库存问题
                    if(rooomid == XSGJ_ROOM_ID)
                    {
                        rooomid = SGJ_ROOM_ID;
                    }
                    // 水果机共用一个库存
                    REDISCLIENT.hset(REDIS_CUR_STOCKS+to_string(m_GameRoomInfo.agentId), to_string(rooomid), to_string(m_GameRoomInfo.totalStock));
                    // REDISCLIENT.hset(REDIS_CUR_STOCKS, to_string(m_GameRoomInfo.roomId), to_string(m_GameRoomInfo.totalStock));
                    LOG_WARN << "--- *** 更新当前库存[" << m_GameRoomInfo.totalStock << "],rooomid[" << rooomid <<"]";
                });
            }
        }
    }
}

void GameServer::notifyRechargeScoreToGameServerMessage(string msg)
{
    LOG_DEBUG << "--- *** "
              << "MSG = " << msg;
}

void GameServer::notifyExchangeScoreToGameServerMessage(string msg)
{
    LOG_DEBUG << "--- *** "
              << "MSG = " << msg;
}

void GameServer::cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
}

// get the special user base info.
bool GameServer::GetUserBaseInfo(int64_t userId, GSUserBaseInfo &baseInfo)
{
    try
    {
        bsoncxx::document::element elem;
        mongocxx::options::find opts = mongocxx::options::find{};
        opts.projection(document{} << "userid" << 1 << "account" << 1 << "agentid" << 1 << "linecode" << 1 << "headindex" << 1 << "nickname" << 1 << "registertime" << 1
                                   << "score" << 1 << "status" << 1 << "alladdscore" << 1 << "allsubscore" << 1 << "winorlosescore" << 1 << finalize);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto query_value = document{} << "userid" << userId << finalize;
        // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view(),opts);
        if (maybe_result)
        {
            //LOG_DEBUG<< "Login User DB Info" << bsoncxx::to_json(*maybe_result);

            bsoncxx::document::view view = maybe_result->view();
            baseInfo.userId = view["userid"].get_int64();
            baseInfo.account = view["account"].get_utf8().value.to_string();
            baseInfo.agentId = view["agentid"].get_int32();
            baseInfo.lineCode = view["linecode"].get_utf8().value.to_string();
            baseInfo.headId = view["headindex"].get_int32();
            baseInfo.nickName = view["nickname"].get_utf8().value.to_string();
            baseInfo.userScore = view["score"].get_int64();
            baseInfo.status = view["status"].get_int32();
            //        REDISCLIENT.exists()
            int64_t totalAddScore = view["alladdscore"].get_int64();
            int64_t totalSubScore = view["allsubscore"].get_int64();
            int64_t totalWinScore = view["winorlosescore"].get_int64();
            int64_t totalBet = totalAddScore - totalSubScore;
            
            //注册秒数
            int64_t registertime = view["registertime"].get_date()/1000; 
            int64_t timeseconds = time(nullptr) - registertime;
            baseInfo.regSecond = timeseconds > 0 ? timeseconds : 0;
            
            LOG_DEBUG<< "user["<< baseInfo.userId <<"] registertime " << timeseconds << "s," << registertime;

            REDISCLIENT.hset(REDIS_SCORE_PREFIX + to_string(userId), REDIS_SUBSCORE, to_string(view["allsubscore"].get_int64()));
            REDISCLIENT.hset(REDIS_SCORE_PREFIX + to_string(userId), REDIS_ADDSCORE, to_string(view["alladdscore"].get_int64()));
            REDISCLIENT.hset(REDIS_SCORE_PREFIX + to_string(userId), REDIS_WINSCORE, to_string(view["winorlosescore"].get_int64()));
            if (totalBet == 0)
            {
                totalBet = 1;
            }
            int64_t per = totalWinScore * 100 / totalBet;
            if (totalWinScore > 0)
            {
                string key = to_string(userId);
                if (per > 200 || per < 0)
                {
                    REDISCLIENT.AddQuarantine(userId);
                }
                else
                {
                    REDISCLIENT.RemoveQuarantine(userId);
                }
            }
            return true;
        }
        else
        {
            LOG_ERROR << "GameServer:GetUserBaseInfo data error!, userId:" << userId;
        }
    }
    catch (exception &e)
    {
        LOG_ERROR << "MongoDB exception: " << e.what();
    }
    return (false);
}

void GameServer::DBWriteRecordDailyUserOnline(shared_ptr<RedisClient> &redisClient, int nOSType, int nChannelId, int nPromoterId)
{
    //    char sql[1024]={0};
}

// status = 0 offline.
// status = configid (in which game default=0).
void GameServer::ChangeGameOnLineStatus(shared_ptr<RedisClient> &redisClient, int64_t userId, int configId)
{
    char sql[128] = {0};
    snprintf(sql, sizeof(sql), "UPDATE db_account.user_extendinfo SET isonline_game=%d WHERE userid=%d", configId, userId);
    redisClient->PushSQL(sql);
    LOG_WARN << "Change Game Online status=" << configId << " userid:" << userId;
}

void GameServer::cmd_user_change_table(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_DEBUG << __FILE__ << __FUNCTION__;
}

void GameServer::cmd_user_recharge_ok(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
{
    LOG_DEBUG << "cmd_user_recharge_ok  command.";
}

void GameServer::cmd_notifyRepairServerResp(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header)
{

    Header *common = (Header *)data;
    int32_t repaireStatus = 1;
    ::Game::Common::HttpNotifyRepairServerResp msg;
    if (msg.ParseFromArray(&data[sizeof(Header)], common->realSize))
    {
        int32_t status = msg.status();
        if (status == 0 && m_bInvaildServer && ZOK == m_zookeeperclient->deleteNode(m_szInvalidNodePath))
        {
            m_bInvaildServer = false;
            m_GameRoomInfo.serverStatus = SERVER_RUNNING;
        }
        else if (status == 1 && !m_bInvaildServer && ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, to_string(1), true))
        {
            m_bInvaildServer = true;
        }
        else if (status == 2)
        {
            m_GameRoomInfo.serverStatus = SERVER_REPAIRING;
            if (!m_bInvaildServer && ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, to_string(2), true))
            {
                m_bInvaildServer = true;
            }
            else if (ZOK == m_zookeeperclient->setNodeValue(m_szInvalidNodePath, to_string(2)))
            {
            }
        }
        else
            repaireStatus = 0;
    }
    msg.set_status(repaireStatus);

    size_t len = msg.ByteSize();
    vector<unsigned char> rdata(len + sizeof(internal_prev_header) + sizeof(Header));
    if (msg.SerializeToArray(&rdata[sizeof(internal_prev_header) + sizeof(Header)], len))
    {
        sendData(weakConn, rdata, internal_header, common);
    }
}

void GameServer::SavePlayGameTime(int64_t userId)
{
}

void GameServer::KickUserIdToProxyOffLine(int64_t userId, int32_t nKickType)
{
    LOG_DEBUG << "--- *** "
              << "userid = " << userId << " kickType = " << nKickType;
}

void GameServer::updateAgentPlayers()
{
    map<uint32_t, uint32_t> mapAgent_Players;
    CServerUserManager::get_mutable_instance().GetAgentPlayersMap(mapAgent_Players);
    Json::Value value;
    for (auto &info : mapAgent_Players)
    {
        value[to_string(info.first)] = info.second;
    }
    Json::FastWriter write;
    string str = write.write(value);
    REDISCLIENT.set("agentid:" + m_szNodeValue, str, 120);
}

void GameServer::DistributeTable()
{
    LOG_ERROR << "DistributeTable NO IMPL yet!";
}

bool GameServer::DelGameUserToProxy(int64_t userId, muduo::net::TcpConnectionWeakPtr pproxySock)
{
    LOG_ERROR << "DelGameUserToProxy NO IMPL yet!";
    return true;
}

} // namespace Landy
