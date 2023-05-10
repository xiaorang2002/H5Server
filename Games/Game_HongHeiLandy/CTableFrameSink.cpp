#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string> 
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/HongHei.Message.pb.h"

#include "redblackalgorithm.h"
#include "GameLogic.h"
//#include "HistoryScore.h"
//#include "IAicsRedxBlack.h"

using namespace HongHei;

#include "CTableFrameSink.h"
#include "json/json.h"

#define MIN_LEFT_CARDS_RELOAD_NUM  24
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0


// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/honghei/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("honghei");
        // google::SetStderrLogging(google::GLOG_ERROR);
        google::SetStderrLogging(google::GLOG_INFO);
    }

    virtual ~CGLogInit()
    {
        google::ShutdownGoogleLogging();
    }
};

CGLogInit glog_init;
//struct RomInfo
//{
//    IAicsRedxBlackEngine* sp ;
//    tagRedSettings gs  ;
//    int status ;
//   int nRoomID;
//};

class RomMgr
{
public:
    RomMgr()
    {
        bisInit = false;

        if(access("./log/honghei", 0) == -1)
           mkdir("./log/honghei",0777);
    }

    virtual ~RomMgr()
    {
         google::ShutdownGoogleLogging();
    }

public:
    static RomMgr& Singleton()
    {
        static RomMgr* sp = 0;
        if (!sp)
        {
             sp = new RomMgr();
        }
    //Cleanup:
        return (*sp);
    }

public:
//     int setInit(int nMaxTable,int roomID)
//     {
//         RomInfo rominfo = {0};
//         rominfo.status  = -1;
//         if (bisInit) return sOk;
//         do
//         {
//             rominfo.sp=GetAicsRedEngine();
//             rominfo.gs.nToubbili      = 1;
//             rominfo.gs.nMaxPlayer     = GAME_PLAYER;
//             rominfo.gs.nMaxTable	   = nMaxTable;
//             rominfo.status= rominfo.sp->Init(&rominfo.gs);
//             if (sOk != rominfo.status) {
//                 break;
//             }

//             // try to select the special room information now.
//             rominfo.status = rominfo.sp->SetRoomID(roomID);
//             if (sOk == rominfo.status)
//             {
//                 bisInit = true;
//             }
//         } while (0);
     //Cleanup:
//         return 0/*(rominfo.status)*/;
//     }

//     int getinfo(tagRedGateInfo &info)
//     {
//         int status = -1;
//         IAicsRedxBlackEngine* sp = GetAicsRedEngine();
//         if (sp)
//         {
//             status = sp->redBlackOnebk(0,0,0,&info);
//         }
//     //Cleanup:
//         return (status);
//     }
private:
     bool bisInit;
};


//////////////////////////////////////////////////////////////////////////

//静态变量
//const int			CTableFrameSink::m_wPlayerCount = GAME_PLAYER;		      //游戏人数
//const uint8_t		CTableFrameSink::m_GameStartMode=START_MODE_DELAY_START;	//开始模式


//#define TIME_GAME_COMPAREEND				6000			//结束定时器
//#define TIME_GAME_OPENEND					500				//结束定时器

int64_t						m_lStockScore    = 0;			//库存
int64_t						m_lInScoreVal    = 10000;		//吸分值
int64_t						m_lOutScoreVal   = 20000;		//吐分值
int64_t						m_lStorageDeduct = 1;			//回扣变量

int64_t                     openCount = 1;

//////////////////////////////////////////////////////////////////////////

//MAX_CARDS = 52 * 8
uint8_t CTableFrameSink::m_cbTableCard[MAX_CARDS] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

};


//构造函数
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "HongHeiTimerEventLoopThread")
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));


    m_strRoundId = "";
	m_vAllchips.clear();
    m_Hong.clear();
    m_Hei.clear();
    m_Special.clear();

    memset(m_cbTableCardArray, 0, sizeof(m_cbTableCardArray));

    m_IsEnableOpenlog = false;
    m_IsXuYaoRobotXiaZhu = false;
    m_WinScoreSystem = 0;

    m_nPlaceJettonTime = 0;
    m_nPlaceJettonLeveTime = 0;
    m_ngameTimeJettonBroadcast=0;
    m_nGameEndTime = 0;
    m_nGameEndLeveTime = 0;

    m_ShenSuanZiId = 0;

    m_UserInfo.clear();
    m_SortUserInfo.clear();

    m_openRecord.clear();



    m_CardVecData.clear();
    m_pGameRoomInfo = nullptr;
    srandom(time(NULL));

    m_duiZiCount=0;
    m_shunZiCount=0;
    m_jinHuaCount=0;
    m_shunJinCount=0;
    m_baoZiCount=0;
    m_gameRoundCount=0;
    m_gameWinCount=0;
    m_winPercent=0.0f;
    m_curSystemWin=0;
    m_dControlkill=0.0;
    m_lLimitPoints=0;
    stockWeak = 0.0f;
    memset(m_nMuilt, 0, sizeof(m_nMuilt));
    memset(m_bAreaIsWin, 0, sizeof(m_bAreaIsWin));
    for(int i=0;i<GAME_PLAYER;i++)
    {
        m_currentchips[i][0]=100;
        m_currentchips[i][1]=200;
        m_currentchips[i][2]=500;
        m_currentchips[i][3]=1000;
        m_currentchips[i][4]=5000;
        /*m_currentchips[i][5]=10000;
        m_currentchips[i][6]=50000;*/
    }
    //ReadConfigInformation();

	m_bGameEnd = false;
	m_fUpdateGameMsgTime = 0;
	m_fResetPlayerTime = 5.0f;
	m_cbGameStatus = GAME_STATUS_INIT;
	m_strTableId = "";		//桌号
	memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
	memset(m_lLimitScore, 0, sizeof(m_lLimitScore));
	m_ipCode = 0;
	m_bAddRecord = false;
    return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
    /*
    if( m_pServerContro )
    {
        delete m_pServerContro;
        m_pServerContro = NULL;
    }

    if( m_hControlInst )
    {
        FreeLibrary(m_hControlInst);
        m_hControlInst = NULL;
    }
    */
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
}

//读取配置
bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/honghei_config.ini"))
    {
        LOG_INFO << "./conf/honghei_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/honghei_config.ini", pt);

    stockWeak=pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    //Time
    m_nPlaceJettonTime   = pt.get<int64_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 7);
    m_ngameTimeJettonBroadcast= pt.get<float>("GAME_CONFIG.TM_JETTON_BROADCAST", 1);

	m_fUpdateGameMsgTime = pt.get<float>("GAME_CONFIG.TM_UpdateGameMsgTime", 1.0);
	m_fResetPlayerTime = pt.get<float>("GAME_CONFIG.TM_RESET_PLAYER_LIST", 5.0);

    m_HongTimes          = pt.get<int64_t>("GAME_CONFIG.AREA_LONG", 1);
    m_HeiTimes            = pt.get<int64_t>("GAME_CONFIG.AREA_HU", 1);

    m_SpecialTimes[0]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_0", 0);
    m_SpecialTimes[1]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_1", 1);
    m_SpecialTimes[2]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_2", 2);
    m_SpecialTimes[3]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_3", 3);
    m_SpecialTimes[4]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_4", 5);
    m_SpecialTimes[5]            = pt.get<int64_t>("GAME_CONFIG.RXBPKTYPE_5", 10);

    m_AreaMaxJetton[1]=pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".RED",1000000);
    m_AreaMaxJetton[2]=pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".BLACK",1000000);
    m_AreaMaxJetton[0]=pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".SPECIAL",1000000);

	m_lLimitPoints = pt.get<int64_t>("GAME_CONFIG.LimitPoint" + to_string(m_pGameRoomInfo->roomId % 100), 500000);
    m_dControlkill=pt.get<double>("GAME_CONFIG.CotrolKill",0.45);


    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog",0);
    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);
    m_WinScoreSystem     = pt.get<int64_t>("GAME_CONFIG.WinScore", 500);
    m_bControl           = pt.get<int64_t>("GAME_CONFIG.CONTROL", 0);

    vector<int64_t> scorelevel;
    vector<int64_t> chips;

    //vector<int> m_vAllchips;

	string strCHIP_CONFIGURATION = "";
	strCHIP_CONFIGURATION = "CHIP_CONFIGURATION_" + to_string(m_pGameRoomInfo->roomId);
	m_lLimitScore[0] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMin", 100);
	m_lLimitScore[1] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMax", 2000000);

	for (int64_t chips : m_vAllchips)
	{
		m_Hong[chips] = 0;
		m_Hei[chips] = 0;
		m_Special[chips] = 0;
	}
    m_vAllchips.clear();
    scorelevel.clear();
    chips.clear();
    useIntelligentChips=pt.get<int64_t>(strCHIP_CONFIGURATION + ".useintelligentchips", 0);
    scorelevel = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear1","100,200,500,1000,5000"));
    for(int i=0;i<4;i++)
    {
        m_userScoreLevel[i]=scorelevel[i];
    }
    for(int i=0;i<CHIP_COUNT;i++)
    {
        m_userChips[0][i]=chips[i];
        m_vAllchips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear2","100,200,500,1000,5000"));
    for(int i=0;i<CHIP_COUNT;i++)
    {
        m_userChips[1][i]=chips[i];
        vector<int64_t>::iterator it=find(m_vAllchips.begin(),m_vAllchips.end(),chips[i]);
        if(it!=m_vAllchips.end())
        {

        }
        else
        {
            m_vAllchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear3","100,200,500,1000,5000"));
    for(int i=0;i<CHIP_COUNT;i++)
    {
        m_userChips[2][i]=chips[i];
        vector<int64_t>::iterator it=find(m_vAllchips.begin(),m_vAllchips.end(),chips[i]);
        if(it!=m_vAllchips.end())
        {

        }
        else
        {
            m_vAllchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear4","100,200,500,1000,5000"));
    for(int i=0;i<CHIP_COUNT;i++)
    {
        m_userChips[3][i]=chips[i];
        vector<int64_t>::iterator it=find(m_vAllchips.begin(),m_vAllchips.end(),chips[i]);
        if(it!=m_vAllchips.end())
        {

        }
        else
        {
            m_vAllchips.push_back(chips[i]);
        }
    }

    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear4","100,200,500,1000,5000"));
    if(!useIntelligentChips)
    {
        for(int i=0;i<CHIP_COUNT;i++)
        {
             m_userChips[0][i]=chips[i];
             m_userChips[1][i]=chips[i];
             m_userChips[2][i]=chips[i];
             m_userChips[3][i]=chips[i];
        }
    }
    for(int64_t chips : m_vAllchips)
    {
        m_Hong[chips] = 0;
        m_Hei[chips] = 0;
        m_Special[chips] = 0;
    }
}

bool CTableFrameSink::ReadCardConfig()
{
    return false;
    m_listTestCardArray.clear();
    if (!boost::filesystem::exists("./conf/cards_hh.json"))
        return false;
    boost::property_tree::ptree pt;
    try{
        boost::property_tree::read_json("./conf/cards_hh.json",pt);
    }catch(...){
        LOG(INFO)<<"cards.json firmat error !!!!!!!!!!!!!!!";
        return false;
    }

    boost::property_tree::ptree  pCard_weave=pt.get_child("card_weave");
    boost::property_tree::ptree cards;
    for(auto& weave :pCard_weave){
       cards=weave.second.get_child("red");
       for(auto& it:cards){
           m_listTestCardArray.push_back(stod(it.second.data()));
       }
       cards=weave.second.get_child("black");
       for(auto& it:cards){
           m_listTestCardArray.push_back(stod(it.second.data()));
       }
    }
    string str="./conf/cards";
    str+= time((time_t*)NULL);
    str+="_hh.json";
    boost::filesystem::rename("./conf/cards_hh.json",str);
}

bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
//    int status = RomMgr::Singleton().setInit(pTableFrame->GetGameRoomInfo()->tableCount, pTableFrame->GetGameRoomInfo()->roomId);
//    if (m_bControl&&sOk != status)
//    {
//        int roomid = pTableFrame->GetGameRoomInfo()->roomId;
//        openLog("RomMgr::Singleton().setInit failed, roomid:[%d], status:[%d]",roomid,status);
//        LOG_ERROR << "RomMgr::Singleton().setInit failed, status:" << status;
//        abort();
//    }

    LOG(INFO) << "CTableFrameSink::SetTableFrame pTableFrame:" << pTableFrame;

    m_pITableFrame = pTableFrame;
    if (pTableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata

		ReadConfigInformation();
        return true;
    }else
        return false;
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    LOG(INFO) << "OnGameStart" ;
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
        return true;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            totalxz = userInfo->m_lUserJettonScore[SPECIAL] + userInfo->m_lUserJettonScore[BLACK] + userInfo->m_lUserJettonScore[RED];
        }

        if(totalxz > 0)
            canLeft = false;
    }

    return canLeft;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    shared_ptr<CServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL userId:"<<userId;
        return false;
    }
    LOG_DEBUG << "OnUserEnter userId:"<<userId;
    uint32_t chairId = pIServerUserItem->GetChairId();
	m_cbGameStatus = m_pITableFrame->GetGameStatus();
    if(GAME_STATUS_INIT == m_pITableFrame->GetGameStatus())
    {
		string ip = m_pITableFrame->GetGameRoomInfo()->serverIP;
		m_ipCode = 0;
		vector<string> addrVec;
		addrVec.clear();
		boost::algorithm::split(addrVec, ip, boost::is_any_of(":"));
		if (addrVec.size() == 3)
		{
			vector<string> ipVec;
			ipVec.clear();
			boost::algorithm::split(ipVec, addrVec[1], boost::is_any_of("."));
			for (int i = 0; i < ipVec.size();i++)
			{
				m_ipCode += stoi(ipVec[i]);
			}
		}
		m_strTableId = to_string(m_ipCode) + "-" + to_string(m_pITableFrame->GetTableId() + 1);	//桌号

		m_Algorithm.SetRoomMsg(m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());

        m_ShenSuanZiId = pIServerUserItem->GetUserId();
//        openLog("11111shensunzi=%d",m_ShenSuanZiId);
        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
        m_nGameEndLeveTime = m_nGameEndTime;
        m_startTime = chrono::system_clock::now();
        m_strRoundId = m_pITableFrame->GetNewRoundId();

        m_replay.gameinfoid = m_strRoundId;
        m_dwReadyTime = (uint32_t)time(NULL);

        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
//        openLog("m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON, 1000, -1, 0L)");
        m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

		m_cbGameStatus = GAME_STATUS_START;
		m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, CALLBACK_0(CTableFrameSink::updateGameMsg, this, false));

//        for(int64_t jetton : m_pGameRoomInfo->jettons)
//        {
//            m_Hong[jetton] = 0;
//            m_Hei[jetton] = 0;
//            m_Special[jetton] = 0;
//        }
    }


    for(int i=0;i<CHIP_COUNT;i++)
    {

        if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[0]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[1])
        {
            m_currentchips[chairId][i]=m_userChips[0][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[1]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[2])
        {
            m_currentchips[chairId][i]=m_userChips[1][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[2]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[3])
        {
            m_currentchips[chairId][i]=m_userChips[2][i];
        }
        else
        {
            m_currentchips[chairId][i]=m_userChips[3][i];
        }
    }
    auto it = m_UserInfo.find(userId);
    if(m_UserInfo.end() == it)
    {
        shared_ptr<UserInfo> userInfo(new UserInfo());
        userInfo->userId = userId;
        userInfo->chairId = chairId;
        userInfo->headerId = pIServerUserItem->GetHeaderId();
        userInfo->nickName = pIServerUserItem->GetNickName();
        userInfo->location = pIServerUserItem->GetLocation();

        m_UserInfo[userId] = userInfo;
    }
//    m_cbPlayStatus[chairId] = true;
    pIServerUserItem->SetUserStatus(sPlaying);
    //m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
    //m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
    //m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
    OnEventGameScene(chairId, false);
    return true;
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL chairId:"<<chairId;
        return false;
    }

    uint8_t cbGameStatus = m_pITableFrame->GetGameStatus();
    int64_t userId = pIServerUserItem->GetUserId();

    LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << (int)cbGameStatus;

    switch(cbGameStatus)
    {
        case GAME_STATUS_FREE:		//空闲状态
        {
            //构造数据
          // CMD_S_GameStart pGameStartScene;
          // std::string data = pGameStartScene.SerializeAsString();
          // pGameStartScene.set_cbtimeleave(10);
          // m_pITableFrame->SendTableData(pIServerUserItem->GetChairID(), HongHei::SUB_S_GAME_START, data.c_str(), data.size());
            return true;
        }
        break;
//     	  case GAME_STATUS_PLAY:	//游戏状态
        case GAME_STATUS_START:	//游戏状态
//        case GAME_STATUS_JETTON:
        {
            SendOpenRecord(chairId);
            SendStartMsg(chairId);
            return true;
        }
        break;
        case GAME_STATUS_END:
        {
            SendOpenRecord(chairId);
            SendEndMsg(chairId);
            return true;
        }
        break;
    }
    //效验错误
    assert(false);
    return false;
}

void CTableFrameSink::SendOpenRecord(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " SendOpenRecord pIServerUserItem==NULL chairId:"<<chairId;
        return;
    }

    uint8_t cbGameStatus = m_pITableFrame->GetGameStatus();
    uint32_t size = m_openRecord.size();
    /*if(cbGameStatus == GAME_STATUS_START)
    {
        size = m_openRecord.size();
    }else
    {
        if(m_openRecord.size() > 0)
            size = m_openRecord.size() - 1;
    }*/

    CMD_S_GameRecord record;
    int heigWin = 0;
    int hongWin = 0;
    int xinyunWin = 0;

    for(int i = 0; i < size; ++i)
    {
        HongHeiGameRecord* precord = record.add_record();
        precord->set_winarea(m_openRecord[i]);
        precord->set_cardgrouptype(m_openRecordType[i]);
        if(m_openRecord[i] == BLACK)
            heigWin++;
        else if(m_openRecord[i] == RED)
            hongWin++;

        if(m_openRecordType[i]>=CT_SINGLE)
            xinyunWin++;
    }
    record.set_lheicount(heigWin);
    record.set_lhongcount(hongWin);
    record.set_lallcount(xinyunWin);

    std::string data = record.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, HongHei::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendStartMsg(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
    if (!userInfo)
        return;

    CMD_S_StartPlaceJetton pGamePlayScene;
    GameDeskInfo* pInfo = pGamePlayScene.mutable_deskdata();

    for(int i=0;i < CHIP_COUNT;i++)
    {
        pInfo->add_userchips(m_currentchips[chairId][i]);
    }
    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
        alljitton->set_ljettonarea(i);
    }

    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
        userjetton->set_dwuserid(userId);
        userjetton->set_ljettonarea(i);

        if(pIServerUserItem->IsAndroidUser())
            userjetton->set_ljettonscore(userInfo->m_lRobotUserJettonScore[i]);
        else
            userjetton->set_ljettonscore(userInfo->m_lUserJettonScore[i]);
    }

    //divineJettonScore
    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
        divineJetton->set_dwuserid(m_ShenSuanZiId);
        shared_ptr<IServerUserItem> pUserItem = m_pITableFrame->GetUserItemByUserId(m_ShenSuanZiId);
        if(pUserItem)
            divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
        else
            divineJetton->set_ljettonscore(0);
        divineJetton->set_ljettonarea(i);
    }

    pInfo->set_lapplybankercondition(0);
    pInfo->set_larealimitscore(100000000);
    for(int i = 0; i < 2; ++i)
    {
        CardGroup* card =pInfo->add_cards();
        card->add_cards(m_cbTableCardArray[i*3+1]);
        card->add_cards(m_cbTableCardArray[i*3+2]);
        card->add_cards(m_cbTableCardArray[i*3+0]);
        card->set_group_id(i+1);
        card->set_cardtype(0);
    }

   uint32_t i = 0;
   uint32_t size = 6;
   int64_t nowScore = 0;
   int64_t xzScore = 0;
   shared_ptr<IServerUserItem> pUserItem;
   shared_ptr<UserInfo> userInfoItem;
   for(auto &userInfoItem : m_SortUserInfo)
   {
//       userInfoItem = it.second;
       pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
       if(!pUserItem)
           nowScore = 0;
       else
       {
           xzScore = 0;
           if(pUserItem->IsAndroidUser())
               xzScore = userInfoItem->m_lRobotUserJettonScore[SPECIAL] + userInfoItem->m_lRobotUserJettonScore[BLACK] + userInfoItem->m_lRobotUserJettonScore[RED];
           else
               xzScore = userInfoItem->m_lUserJettonScore[SPECIAL] + userInfoItem->m_lUserJettonScore[BLACK] + userInfoItem->m_lUserJettonScore[RED];

            nowScore = pUserItem->GetUserScore()- xzScore;
       }

       PlayerListItem* palyer = pInfo->add_players();
       palyer->set_dwuserid(userInfoItem->userId);
       palyer->set_headerid(userInfoItem->headerId);
       palyer->set_nickname(userInfoItem->nickName);

       palyer->set_luserscore(nowScore);

       palyer->set_ltwentywinscore(userInfoItem->lTwentyWinScore);
       palyer->set_ltwentywincount(userInfoItem->lTwentyWinCount);
       int shensuanzi = userInfoItem->userId == m_ShenSuanZiId ? 1 : 0;
       palyer->set_isdivinemath(shensuanzi);
       palyer->set_index(i+1);
       if(shensuanzi == 1)
           palyer->set_index(0);
       palyer->set_isbanker(0);
       palyer->set_isapplybanker(0);
       palyer->set_applybankermulti(0);
       palyer->set_ljetton(0);
       palyer->set_szlocation(userInfoItem->location);

       if(++i > size)
           break;
   }

    pInfo->set_wbankerchairid(0);
    pInfo->set_cbbankertimes(0);
    pInfo->set_lbankerwinscore(0);
    pInfo->set_lbankerscore(0);
    pInfo->set_benablesysbanker(true);
    pInfo->set_szgameroomname("HongHei");
    if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);

    pInfo->set_wincardgrouptype(m_winType);// add_wincardgrouptype(0);

    for(auto &jetton : m_Special)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(SPECIAL);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hong)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(RED);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(BLACK);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }

    pInfo->set_lonlinepopulation(m_UserInfo.size());
	//限红
	for (int i = 0;i < 2;i++)
	{
		pInfo->add_llimitscore(m_lLimitScore[i]);
	}
	pInfo->set_tableid(m_strTableId);

    pGamePlayScene.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_startTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pGamePlayScene.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    xzScore = 0;
    bool isRobot = pIServerUserItem->IsAndroidUser();
    if(isRobot)
        xzScore = userInfo->m_lRobotUserJettonScore[SPECIAL] + userInfo->m_lRobotUserJettonScore[BLACK]+ userInfo->m_lRobotUserJettonScore[RED];
    else
        xzScore = userInfo->m_lUserJettonScore[SPECIAL] + userInfo->m_lUserJettonScore[BLACK] + userInfo->m_lUserJettonScore[RED];

    nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pGamePlayScene.set_lusermaxscore(maxscore);
    pGamePlayScene.set_luserscore(nowScore);
    if(userInfo->m_lPreUserJettonScore[SPECIAL]||userInfo->m_lPreUserJettonScore[BLACK]||userInfo->m_lPreUserJettonScore[RED])
        pGamePlayScene.set_brejetton(true);
    else
        pGamePlayScene.set_brejetton(false);


    pGamePlayScene.set_verificationcode(0);
    pGamePlayScene.set_roundid(m_strRoundId);
    std::string data = pGamePlayScene.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), HongHei::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
}


void CTableFrameSink::SendEndMsg(uint32_t wChairID)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_GameEnd gamend;
    GameDeskInfo* pInfo = gamend.mutable_deskdata();

    for(int i=0;i < CHIP_COUNT;i++)
    {
        pInfo->add_userchips(m_currentchips[wChairID][i]);
    }
    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
        userjetton->set_dwuserid(userId);
        userjetton->set_ljettonarea(i);

        if(pIServerUserItem->IsAndroidUser())
            userjetton->set_ljettonscore(userInfo->m_lRobotUserJettonScore[i]);
        else
            userjetton->set_ljettonscore(userInfo->m_lUserJettonScore[i]);
    }
     //divineJettonScore
    for(int i = 0; i < 3; ++i)
    {
        PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
        divineJetton->set_dwuserid(m_ShenSuanZiId);
        shared_ptr<IServerUserItem> pUserItem;
        pUserItem = m_pITableFrame->GetUserItemByUserId(m_ShenSuanZiId);
        if(pUserItem)
            divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
        else
            divineJetton->set_ljettonscore(0);
        divineJetton->set_ljettonarea(i);
    }

    pInfo->set_lapplybankercondition(0);
    pInfo->set_larealimitscore(10000000);
    for(int i = 0; i < 2; ++i)
    {
        CardGroup* card = pInfo->add_cards();
        card->add_cards(m_cbTableCardArray[i*3+0]);
        card->add_cards(m_cbTableCardArray[i*3+1]);
        card->add_cards(m_cbTableCardArray[i*3+2]);
        card->set_group_id(i+1);
        card->set_cardtype(m_TableCardType[i]);
    }

    bool isVIP = false;
    uint32_t i = 0;
    uint32_t size = 6;
    int64_t nowScore = 0;
    int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pUserItem;
//    shared_ptr<UserInfo> userInfoItem;
    for(auto &userInfoItem : m_SortUserInfo)
    {
//        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            nowScore = 0;
        else
        {
            xzScore = 0;
            if(pUserItem->IsAndroidUser())
                xzScore = userInfoItem->m_lRobotUserJettonScore[SPECIAL] + userInfoItem->m_lRobotUserJettonScore[BLACK] + userInfoItem->m_lRobotUserJettonScore[RED];
            else
                xzScore = userInfoItem->m_lUserJettonScore[SPECIAL] + userInfoItem->m_lUserJettonScore[BLACK] + userInfoItem->m_lUserJettonScore[RED];

             nowScore = pUserItem->GetUserScore() + userInfoItem->m_EndWinScore ;//- xzScore;
        }

        PlayerListItem* palyer = pInfo->add_players();
        palyer->set_dwuserid(userInfoItem->userId);
        palyer->set_headerid(userInfoItem->headerId);
        palyer->set_nickname(userInfoItem->nickName);

        palyer->set_luserscore(nowScore);

        palyer->set_ltwentywinscore(userInfoItem->lTwentyWinScore);
        palyer->set_ltwentywincount(userInfoItem->lTwentyWinCount);
        int shensuanzi = userInfoItem->userId == m_ShenSuanZiId ? 1 : 0;
        palyer->set_isdivinemath(shensuanzi);
        palyer->set_index(i);
        if(shensuanzi == 1)
            palyer->set_index(0);
        palyer->set_isbanker(0);
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(userInfoItem->location);


        PlayerScoreValue * score = gamend.add_gameendscore();
        score->set_userid(userInfoItem->userId);
        score->set_returnscore(userInfoItem->m_EndWinScore);
        score->set_userscore(nowScore);

        if(userId == userInfoItem->userId) //vip
            isVIP = true;

        if(++i > size)
            break;
    }

    pInfo->set_wbankerchairid(0);
    pInfo->set_cbbankertimes(0);
    pInfo->set_lbankerwinscore(0);
    pInfo->set_lbankerscore(0);
    pInfo->set_benablesysbanker(true);
    pInfo->set_szgameroomname("HongHei");

    /*if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);
    
    else
    {
        m_openRecord.push_back(rand()%2+1);
        pInfo->add_wintag(m_openRecord[m_openRecord.size()-1]);
    }
    */
	if (m_bGameEnd)
	{
		pInfo->set_wintag(m_winFlag);
	}
	else
	{
		if (m_openRecord.size() > 0)
			pInfo->set_wintag(m_openRecord[m_openRecord.size() - 1]);
		else
			pInfo->set_wintag(0);
	}
	
    pInfo->set_wincardgrouptype(m_winType);
    pInfo->set_blucky(m_winType > CT_SINGLE&&m_winType<6 &&(m_winType!=CT_DOUBLE||m_wincardValue>8));

    for(auto &jetton : m_Special)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(SPECIAL);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hong)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(RED);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(BLACK);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }

    pInfo->set_lonlinepopulation(m_UserInfo.size());

	//限红
	for (int i = 0;i < 2;i++)
	{
		pInfo->add_llimitscore(m_lLimitScore[i]);
	}
	pInfo->set_tableid(m_strTableId);

    gamend.set_cbplacetime(m_nGameEndTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_endTime);
    int32_t leaveTime = m_nGameEndTime - durationTime.count();
    gamend.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    nowScore = pIServerUserItem->GetUserScore()+userInfo->m_EndWinScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    gamend.set_lusermaxscore(maxscore);
    gamend.set_luserscore(nowScore);
    gamend.set_lrevenue(userInfo->m_Revenue);
    gamend.set_verificationcode(0);
    gamend.set_roundid(m_strRoundId);
    LOG_DEBUG << "Revenue:"<<userInfo->m_Revenue;

    if(!isVIP)  //myself
    {
        PlayerScoreValue* score = gamend.add_gameendscore();
        score->set_userid(userId);
        score->set_returnscore(userInfo->m_EndWinScore);
        score->set_userscore(nowScore);
    }
	gamend.set_baddrecord(m_bAddRecord);

    string data = gamend.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), HongHei::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
}

bool CTableFrameSink::isInVipList(int64_t chairid)
{
    int listNum=0;
    for(auto userInfo:m_SortUserInfo)
    {
        if(userInfo->chairId==chairid)
        {
            return true;
        }
       if( ++listNum==10)
            break;
    }
    return false;
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool bClear = false;
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

        uint32_t chairId = userInfo->chairId;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            totalxz = userInfo->m_lUserJettonScore[SPECIAL] + userInfo->m_lUserJettonScore[BLACK] + userInfo->m_lUserJettonScore[RED];
        }
        pIServerUserItem->setOffline();
        if(totalxz == 0)
        {
            {
                //WRITE_LOCK(m_mutex);
                m_UserInfo.erase(userId);
            }
            m_pITableFrame->ClearTableUser(chairId);
            bClear=true;
        }else
        {
            pIServerUserItem->SetTrustship(true);
        }
        //pIServerUserItem->SetUserStatus(sGetout);
        //m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
        //        m_cbPlayStatus[chairId] = false;
    }

    if(userId == m_ShenSuanZiId)
    {
        m_ShenSuanZiId = 0;
    }

    //pIServerUserItem->SetUserStatus(sOffline);
    //m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
    return bClear;
}

bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    return true;
}

//根据系统输赢发牌
void CTableFrameSink::GetOpenCards()
{
//    tagRedGateInfo info = { 0 };
//    if(m_IsXuYaoRobotXiaZhu == 1)
//    {
//        for(int i = 0; i < RXBGATE_MAX; ++i)
//        {
//            tagRedGateIn& in = info.inf[i];
//            in.lscorePlaced = m_lAllJettonScore[i]/100;
//            in.sign = AICSRED_SIGN;
//        }
//    }else if(m_IsXuYaoRobotXiaZhu == 0)
//    {
//        for (int i = 0; i < RXBGATE_MAX; ++i)
//        {
//            tagRedGateIn& in = info.inf[i];
//            in.lscorePlaced = m_lTrueUserJettonScore[(i+1)%3]/100;  //1 2 0
//            in.sign = AICSRED_SIGN;
//            LOG_ERROR << "lscorePlaced: " << in.lscorePlaced;
//            openLog("RXBGATE_MAX:[%d],lscorePlaced:[%0.02lf]",(i+1)%3,in.lscorePlaced);
//        }
//    }
    m_winFlag = random()%2+1;//1- 黑 2-红

    int gap= random()%1000 +1;
    if (0<=gap&&gap<700)
        m_winType =CT_SINGLE;
    else if(700<=gap&&gap<890)
        m_winType =CT_DOUBLE;
    else if(890<=gap&&gap<940)
        m_winType =CT_SHUN_ZI;
    else if(940<=gap&&gap<970)
        m_winType =CT_JIN_HUA;
    else if(970<=gap&&gap<990)
        m_winType =CT_SHUN_JIN;
    else if(990<=gap&&gap<1000)
        m_winType =CT_BAO_ZI;


    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    if (m_bControl)
    {

        //难度干涉值
        //m_difficulty
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,命中概率系统必须赚钱
        if(randomRet<m_difficulty)
        {
            m_lStockScore=storageInfo.lEndStorage;
            m_Algorithm.SetBetting(0 ,m_lTrueUserJettonScore[0]);//设置押注
            m_Algorithm.SetBetting(1 ,m_lTrueUserJettonScore[1]);//设置押注
            m_Algorithm.SetBetting(2 ,m_lTrueUserJettonScore[2]);//设置押注
            uint8_t beishu[6]={0};
            for(int i=0;i<6;i++)
            {
                if(i==0)
                 beishu[i]=1;
                else
                 beishu[i]=m_SpecialTimes[i];
            }
            m_Algorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,beishu, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            uint8_t tmpcbTableCardArray[2][3];
            m_Algorithm.SetMustKill(-1);//顺序不可换
            m_Algorithm.GetOpenCard(tmpcbTableCardArray,m_winFlag,m_winType);
            memcpy(m_cbTableCardArray, tmpcbTableCardArray, sizeof(m_cbTableCardArray));

            uint8_t  heiType =   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], 3);
            uint8_t  hongType=   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], 3);
            uint8_t heiCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BLACK-1], 3,heiType);
            uint8_t hongCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RED-1],3,hongType);


            if(m_Algorithm.m_GameLogic.CompareCard(tmpcbTableCardArray[BLACK-1],tmpcbTableCardArray[RED-1],3))
            {
                m_winFlag = BLACK ;
                m_winType=heiType;
                m_wincardValue=heiCard;
            }else
            {
                m_winFlag = RED ;
                m_winType=hongType;
                m_wincardValue=hongCard;
            }
            m_TableCardType[BLACK-1] = heiType;
            m_TableCardType[RED-1] = hongType;
        }
        else
        {
            int  isHaveBlackPlayer=0;

            for(int i=0;i<GAME_PLAYER;i++)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(!user||!user->GetBlacklistInfo().status)
                {
                    continue;
                }
                int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[0]
                +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2];
                if(!user||user->IsAndroidUser()||allJetton==0) continue;
                if(user->GetBlacklistInfo().listRoom.size()>0)
                {
                    auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                    if(it != user->GetBlacklistInfo().listRoom.end())
                    {
                        if(it->second>0)
                        {
                            isHaveBlackPlayer=it->second;//
                        }
                        openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                        openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
                    }
                }


            }
            if(isHaveBlackPlayer>0)
            {
                float probilityAll[3]={0.0f};

                bool mustKill = false ;
                int64_t mustKillId = 0;
                //user probilty count
                for(int i=0;i<GAME_PLAYER;i++)
                {
                    shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                    if(!user||user->IsAndroidUser())
                    {
                        continue;
                    }
                    int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[0]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2];

                    if(allJetton==0) continue;

                    if(user->GetBlacklistInfo().status == 1 )
                    {
                        int shuzhi=0;
                        auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                        if(it != user->GetBlacklistInfo().listRoom.end())
                        {
                            shuzhi=it->second;
                        }
                        else
                        {
                            shuzhi=0;
                        }
                        float gailv=(float)shuzhi/1000;
                       for(int j=0;j<MAX_COUNT;j++)
                       {
                           if(m_lTrueUserJettonScore[j]>0)
                           {
                               probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->m_lUserJettonScore[j])/((float)m_lTrueUserJettonScore[0]+(float)m_lTrueUserJettonScore[1]+(float)m_lTrueUserJettonScore[2]);
                           }

                       }
                       //100%启动必杀
                       if(shuzhi==1000)
                       {
                           mustKillId = user->GetUserId();
                           mustKill =true;
                           break;
                       }
                    }
                }
                m_lStockScore=storageInfo.lEndStorage;
                m_Algorithm.SetBetting(0 ,m_lTrueUserJettonScore[0]);//设置押注
                m_Algorithm.SetBetting(1 ,m_lTrueUserJettonScore[1]);//设置押注
                m_Algorithm.SetBetting(2 ,m_lTrueUserJettonScore[2]);//设置押注
                uint8_t beishu[6]={0};
                for(int i=0;i<6;i++)
                {
                    if(i==0)
                     beishu[i]=1;
                    else
                     beishu[i]=m_SpecialTimes[i];
                }
                m_Algorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,beishu, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
                uint8_t tmpcbTableCardArray[2][3];

                if(mustKill)
                {
                    m_Algorithm.BlackGetMustKillOpenCard(tmpcbTableCardArray,m_winFlag,m_winType,probilityAll,m_UserInfo[mustKillId]->m_lUserJettonScore);
                }
                else
                {
                    m_Algorithm.BlackGetOpenCard(tmpcbTableCardArray,m_winFlag,m_winType,probilityAll);
                }



                memcpy(m_cbTableCardArray, tmpcbTableCardArray, sizeof(m_cbTableCardArray));

                uint8_t  heiType =   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], 3);
                uint8_t  hongType=   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], 3);
                uint8_t heiCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BLACK-1], 3,heiType);
                uint8_t hongCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RED-1],3,hongType);

                if(m_Algorithm.m_GameLogic.CompareCard(tmpcbTableCardArray[BLACK-1],tmpcbTableCardArray[RED-1],3))
                {
                    m_winFlag = BLACK ;
                    m_winType=heiType;
                    m_wincardValue=heiCard;
                }else
                {
                    m_winFlag = RED ;
                    m_winType=hongType;
                    m_wincardValue=hongCard;
                }
                m_TableCardType[BLACK-1] = heiType;
                m_TableCardType[RED-1] = hongType;

            }
            else
            {

                m_lStockScore=storageInfo.lEndStorage;
                m_Algorithm.SetBetting(0 ,m_lTrueUserJettonScore[0]);//设置押注
                m_Algorithm.SetBetting(1 ,m_lTrueUserJettonScore[1]);//设置押注
                m_Algorithm.SetBetting(2 ,m_lTrueUserJettonScore[2]);//设置押注
                uint8_t beishu[6]={0};
                for(int i=0;i<6;i++)
                {
                    if(i==0)
                     beishu[i]=1;
                    else
                     beishu[i]=m_SpecialTimes[i];
                }
                m_Algorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,beishu, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
                uint8_t tmpcbTableCardArray[2][3];
                m_Algorithm.GetOpenCard(tmpcbTableCardArray,m_winFlag,m_winType);
                memcpy(m_cbTableCardArray, tmpcbTableCardArray, sizeof(m_cbTableCardArray));

                uint8_t  heiType =   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], 3);
                uint8_t  hongType=   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], 3);
                uint8_t heiCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BLACK-1], 3,heiType);
                uint8_t hongCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RED-1],3,hongType);


                if(m_Algorithm.m_GameLogic.CompareCard(tmpcbTableCardArray[BLACK-1],tmpcbTableCardArray[RED-1],3))
                {
                    m_winFlag = BLACK ;
                    m_winType=heiType;
                    m_wincardValue=heiCard;
                }else
                {
                    m_winFlag = RED ;
                    m_winType=hongType;
                    m_wincardValue=hongCard;
                }
                m_TableCardType[BLACK-1] = heiType;
                m_TableCardType[RED-1] = hongType;

            }

        }



    }
    else
    {
        uint8_t tmpcbTableCardArray[2][3];
        m_Algorithm.m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        uint8_t  heiType =   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], 3);
        uint8_t  hongType=   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], 3);
        m_TableCardType[BLACK-1] = heiType;
        m_TableCardType[RED-1] = hongType;

        uint8_t heiCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BLACK-1], 3,heiType);
        uint8_t hongCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RED-1],3,hongType);
        if(m_Algorithm.m_GameLogic.CompareCard(tmpcbTableCardArray[BLACK-1],tmpcbTableCardArray[RED-1],3))
        {
            m_winFlag = BLACK ;
            m_winType=heiType;
            m_wincardValue=heiCard;
        }else
        {
            m_winFlag = RED ;
            m_winType=hongType;
            m_wincardValue=hongCard;
        }
    }

    bool isOpenCardEnd = false;
    LOG(INFO) << "......winFlag:"<< m_winFlag << "......";

    openLog("winFlag:[%d],winType:[%d]",m_winFlag,m_winType);
    if (m_listTestCardArray.size() ==0){
        ReadCardConfig();
    }
    if (m_listTestCardArray.size()>5)
    {
        uint8_t tmpcbTableCardArray[2][3];
        for(int i = 0 ; i < 6 ; i ++ ){
            m_cbTableCardArray[i]=*(m_listTestCardArray.begin());
            m_listTestCardArray.pop_front();
        }
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        uint8_t  heiType =   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], 3);
        uint8_t  hongType=   m_Algorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], 3);
        m_TableCardType[BLACK-1] = heiType;
        m_TableCardType[RED-1] = hongType;

        uint8_t heiCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BLACK-1], 3,heiType);
        uint8_t hongCard = m_Algorithm.m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RED-1],3,hongType);
        if(m_Algorithm.m_GameLogic.CompareCard(tmpcbTableCardArray[BLACK-1],tmpcbTableCardArray[RED-1],3))
        {
            m_winFlag = BLACK ;
            m_winType=heiType;
            m_wincardValue=heiCard;
        }else
        {
            m_winFlag = RED ;
            m_winType=hongType;
            m_wincardValue=hongCard;
        }

    }
    m_curSystemWin=0;
    m_curSystemWin=m_Algorithm.GetCurProfit();
    m_gameRoundCount++;
    if(m_winType==0)
    {

    }
    else if(m_winType==1)
    {
        m_duiZiCount++;
    }
    else if(m_winType==2)
    {
        m_shunZiCount++;
    }
    else if(m_winType==3)
    {
        m_jinHuaCount++;
    }
    else if(m_winType==4)
    {
        m_shunJinCount++;
    }
    else if(m_winType==5)
    {
        m_baoZiCount++;
    }
    if(m_curSystemWin>0)
    {
        m_gameWinCount++;
    }
    if(m_curSystemWin>0)
    {
        m_curSystemWin=(m_curSystemWin*(1000-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000);
    }
    m_pITableFrame->UpdateStorageScore(m_curSystemWin);
    m_pITableFrame->GetStorageScore(storageInfo);
    m_lStockScore=storageInfo.lEndStorage;
    openLog("对子次数[%d]",m_duiZiCount);
    openLog("顺子次数[%d]",m_shunZiCount);
    openLog("金花次数[%d]",m_jinHuaCount);
    openLog("顺金次数[%d]",m_shunJinCount);
    openLog("豹子次数[%d]",m_baoZiCount);
    openLog("胜场次数[%d]",m_gameWinCount);
    openLog("总共场次[%d]",m_gameRoundCount);
    openLog("胜率[%02f]",m_winPercent);
    openLog("本次赢输[%d]",m_curSystemWin);
    openLog("当前库存[%d]",m_lStockScore);
    openLog("库存上限[%d]",storageInfo.lUplimit);
    openLog("库存下限[%d]",storageInfo.lLowlimit);

    openLog("card1=0x%02x,0x%02x,0x%02x-----card2=0x%02x,0x%02x,0x%02x",m_cbTableCardArray[0],m_cbTableCardArray[1],
            m_cbTableCardArray[2],m_cbTableCardArray[3],m_cbTableCardArray[4],m_cbTableCardArray[5]);
}


//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();     //open cards
        m_replay.addStep((uint32_t)time(NULL)-m_dwReadyTime,GlobalFunc::converToHex(m_cbTableCardArray, 6),-1,opShowCard,-1,-1);
        //结算
        auto cul_android = [&](shared_ptr<UserInfo> userItem,int winarea,int lostarea,int64_t times){
            if(winarea!=-1 && userItem->m_lRobotUserJettonScore[winarea] > 0)
            {
                //税收
                userItem->m_Revenue = m_pITableFrame->CalculateRevenue(userItem->m_lRobotUserJettonScore[winarea] * times);
                userItem->m_EndWinScore += (userItem->m_lRobotUserJettonScore[winarea]*times - userItem->m_Revenue);
                LOG_DEBUG << " robot Id:"<<userItem->userId<<" SystemTax:"<<userItem->m_Revenue;
            }

            if(lostarea!=-1 && userItem->m_lRobotUserJettonScore[lostarea] > 0)
            {
                userItem->m_EndWinScore -= userItem->m_lRobotUserJettonScore[lostarea];
            }

            LOG_DEBUG << " robot Id:"<<userItem->userId<<" SystemTax:"<<userItem->m_Revenue<<" TotalWin:"<<userItem->m_EndWinScore;
        };
        auto cul_real_user = [&](shared_ptr<UserInfo> userItem,int winarea,int lostarea,int64_t times)
        {
            if(winarea!=-1 &&userItem->m_lUserJettonScore[winarea] > 0)
            {
                //税收
                userItem->m_Revenue = m_pITableFrame->CalculateRevenue(userItem->m_lUserJettonScore[winarea] * times);
                userItem->m_EndWinScore += (userItem->m_lUserJettonScore[winarea]*times - userItem->m_Revenue);
                m_replay.addResult(userItem->chairId,winarea,userItem->m_lUserJettonScore[winarea],(userItem->m_lUserJettonScore[winarea]*times - userItem->m_Revenue),"",false);
                LOG_DEBUG << " robot Id:"<<userItem->userId<<" SystemTax:"<<userItem->m_Revenue;
				userItem->dAreaWin[winarea] = (userItem->m_lUserJettonScore[winarea] * times - userItem->m_Revenue);
            }

            if(lostarea!=-1 && userItem->m_lUserJettonScore[lostarea] > 0)
            {
                userItem->m_EndWinScore -= userItem->m_lUserJettonScore[lostarea];
                m_replay.addResult(userItem->chairId,lostarea,userItem->m_lUserJettonScore[lostarea],-userItem->m_lUserJettonScore[lostarea],"",false);
				userItem->dAreaWin[lostarea] = -userItem->m_lUserJettonScore[lostarea];
            }

            LOG_DEBUG << " robot Id:"<<userItem->userId<<" SystemTax:"<<userItem->m_Revenue<<" TotalWin:"<<userItem->m_EndWinScore;
        };

        LOG_DEBUG<<" WinFlag BLACK 1 RED 2"<<m_winFlag;
        if(m_winFlag==BLACK )
        {
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    cul_android(userInfoItem,BLACK,RED, m_HeiTimes);
                }else   //if(!pUserItem->IsAndroidUser())
                {
                    cul_real_user(userInfoItem,BLACK,RED,m_HeiTimes);
                }
            }
        }else if(m_winFlag==RED){
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    cul_android(userInfoItem,RED,BLACK,m_HongTimes);
                }else   //if(!pUserItem->IsAndroidUser())
                {
                    cul_real_user(userInfoItem,RED,BLACK,m_HongTimes);
                }
            }
        }
        else{
            LOG_DEBUG<<" Conclude failure !!!!!!!!!!!!!!!!!!!";
        }
		m_nMuilt[BLACK] = m_HeiTimes;
		m_nMuilt[RED] = m_HongTimes;
		m_bAreaIsWin[m_winFlag] = 1;
        //special type
        if(m_winType > CT_SINGLE&&m_winType<6 &&(m_winType!=CT_DOUBLE||m_wincardValue>8)) //duizi 1 ~baozi 5
        {
           shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    cul_android(userInfoItem,SPECIAL,-1,m_SpecialTimes[m_winType]);
                }else   //if(!pUserItem->IsAndroidUser())
                {
                    cul_real_user(userInfoItem,SPECIAL,-1,m_SpecialTimes[m_winType]);
                }
            }
			m_nMuilt[SPECIAL] = m_SpecialTimes[m_winType];
			m_bAreaIsWin[SPECIAL] = 1;
        }
        else
        {
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    cul_android(userInfoItem,-1,SPECIAL,1);
                }else   //if(!pUserItem->IsAndroidUser())
                {
                    cul_real_user(userInfoItem,-1,SPECIAL,1);
                }
            }
			m_nMuilt[SPECIAL] = 0;
        }

        ChangeUserInfo();

         /*m_openRecord.push_back(m_winFlag);
         m_openRecordType.push_back(m_winType);*/

        LOG(INFO) << "OnEventGameConclude wrong Conclude is NULL  wChairID = " << chairId;
        return true;
    }
    return false;
}

void CTableFrameSink::clearTableUser()
{
    int64_t min_score = MIN_PLAYING_ANDROID_USER_SCORE;
    if(m_pGameRoomInfo)
        min_score = std::max(m_pGameRoomInfo->enterMinScore, min_score);

    vector<uint32_t> clearUserIdVec;
    {
        //WRITE_LOCK(m_mutex);

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(pUserItem)
            {
                if(m_pGameRoomInfo->serverStatus !=SERVER_RUNNING|| (pUserItem->GetUserStatus() == sOffline ||
                   (pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < min_score)))
                {
                    pUserItem->setOffline();
                    clearUserIdVec.push_back(it.first);
                }
            }
        }
    }

    uint32_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_pITableFrame->ClearTableUser(m_UserInfo[clearUserIdVec[i]]->chairId);
        m_UserInfo.erase(clearUserIdVec[i]);
        for(auto it = m_SortUserInfo.begin(); it != m_SortUserInfo.end(); ++it)
        {
            if(clearUserIdVec[i] == (*it)->userId)
            {
                m_SortUserInfo.erase(it);
                break;
            }
        }
    }
}


//保存玩家结算结果
void CTableFrameSink::WriteUserScore()
{
    vector<tagScoreInfo> scoreInfoVec;

    tagScoreInfo scoreInfo;
    int64_t totalBetScore = 0;

	int64_t allBetScore = 0;
	int64_t userid = 0;
	m_replay.detailsData = "";
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(pUserItem)
        {
			allBetScore = 0;
			userid = pUserItem->GetUserId();
            scoreInfo.clear();
            if(pUserItem->IsAndroidUser())
            {
                totalBetScore = userInfoItem->m_lRobotUserJettonScore[RED] + userInfoItem->m_lRobotUserJettonScore[BLACK] + userInfoItem->m_lRobotUserJettonScore[SPECIAL];
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[RED]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[BLACK]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[SPECIAL]);
            }else
            {
                totalBetScore = userInfoItem->m_lUserJettonScore[RED] + userInfoItem->m_lUserJettonScore[BLACK] + userInfoItem->m_lUserJettonScore[SPECIAL];
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[RED]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[BLACK]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[SPECIAL]);
				allBetScore = totalBetScore;
            }
            if(totalBetScore > 0)
            {
    //                if(userInfoItem->m_EndWinScore)
                {
                    scoreInfo.chairId = userInfoItem->chairId;
                    scoreInfo.betScore = totalBetScore;
                    scoreInfo.rWinScore = totalBetScore;
                    scoreInfo.addScore = userInfoItem->m_EndWinScore;
                    scoreInfo.revenue = userInfoItem->m_Revenue;
                    scoreInfo.cardValue = GlobalFunc::converToHex(m_cbTableCardArray, 6);  // GetCardValueName(0) + " vs "+GetCardValueName(1);
                    scoreInfo.startTime = m_startTime;
                    scoreInfoVec.push_back(scoreInfo);
                }
            }
            // 统计连续未下注的次数
            if (totalBetScore<=0)
            {
                userInfoItem->NotBetJettonCount++;
            }
            else
            {            
                userInfoItem->NotBetJettonCount = 0;
            }
			if (allBetScore > 0)
			{
				//对局记录详情
				SetGameRecordDetail(userid, userInfoItem->chairId, pUserItem->GetUserScore(), userInfoItem);
			}
        }

		
    }

    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
    m_pITableFrame->SaveReplay(m_replay);
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    int64_t totalJetton = 0;

    m_ShenSuanZiId = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        //添加下注信息
        if(pUserItem->IsAndroidUser())
        {
            totalJetton = userInfoItem->m_lRobotUserJettonScore[RED] + userInfoItem->m_lRobotUserJettonScore[SPECIAL] + userInfoItem->m_lRobotUserJettonScore[BLACK];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }else
        {
            totalJetton = userInfoItem->m_lUserJettonScore[RED] + userInfoItem->m_lUserJettonScore[SPECIAL] + userInfoItem->m_lUserJettonScore[BLACK];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }
        totalJetton = 0;
        totalJetton = accumulate(userInfoItem->m_JettonScore.begin(), userInfoItem->m_JettonScore.end(), totalJetton);

        //修改输赢 更新输赢总局数 获取神算子
        if(userInfoItem->m_WinScore.size() >= 20)
            userInfoItem->m_WinScore.pop_front();
         userInfoItem->m_WinScore.push_back(userInfoItem->m_EndWinScore);

         //输赢总局数
         uint32_t winCount = 0;
         int64_t totalWin = 0;
         for(auto iter : userInfoItem->m_WinScore)
         {
            totalWin += iter;
            if(iter > 0)
                winCount++;
         }

         userInfoItem->lTwentyAllUserScore = totalJetton;
         userInfoItem->lTwentyWinScore = totalWin;
         userInfoItem->lTwentyWinCount = winCount;
    }
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeVipUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    int64_t nLow = 0;
    m_SortUserInfo.clear();
    m_ShenSuanZiId = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem||!userInfoItem)
            continue;

        if(m_ShenSuanZiId == 0)
        {
            m_ShenSuanZiId = userInfoItem->userId;
        }
        else if(userInfoItem->lTwentyWinCount > m_UserInfo[m_ShenSuanZiId]->lTwentyWinCount)
        {
            m_ShenSuanZiId = m_ShenSuanZiId = userInfoItem->userId;
        }

        userInfoItem->isDivineMath=false;
    }

    if(m_ShenSuanZiId!=0)
    {
        m_UserInfo[m_ShenSuanZiId]->isDivineMath=true;
        m_SortUserInfo.emplace(m_UserInfo[m_ShenSuanZiId]);
    }

    bool bFirst = true;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem||m_ShenSuanZiId==userInfoItem->userId)
            continue;

         if(unlikely(bFirst))
         {
            bFirst = false;
            nLow = userInfoItem->lTwentyAllUserScore;
            m_SortUserInfo.emplace(userInfoItem);
         }else
         {
             if(m_SortUserInfo.size() < 20)
                 m_SortUserInfo.emplace(userInfoItem);
             else if(userInfoItem->lTwentyAllUserScore > nLow)
             {
                 auto it = m_SortUserInfo.end();
                 it--;
                 m_SortUserInfo.erase(it);
                 it--;
                 nLow = (*it)->lTwentyAllUserScore;
                 m_SortUserInfo.emplace(userInfoItem);
             }
         }
    }


}

//定时器事件
void CTableFrameSink::OnTimerJetton()
{
    //if(--m_nPlaceJettonLeveTime < 1)
    //{
        m_endTime =chrono::system_clock::now();
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdJetton);
        m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);
        //GameTimerJettonBroadcast();
		m_bGameEnd = true;
        //结束游戏
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

        m_nGameEndLeveTime = m_nGameEndTime;

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);

            if(!pUserItem)
                continue;
            SendEndMsg(userInfoItem->chairId);
        }
        SendWinSysMessage();
        m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(m_nGameEndTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));
		m_TimerIdResetPlayerList = m_TimerLoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(CTableFrameSink::RefreshPlayerList, this, true));
        WriteUserScore();
        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_END);
		m_cbGameStatus = GAME_STATUS_END;
        //删除玩家信息
        clearTableUser();  //sOffline  m_cbPlayStatus==false
        ChangeVipUserInfo();
//        ClearTableData();
//        openLog("  ");
//        openLog("end usercoount=%d ",m_UserInfo.size());
        LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();

//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
    //}
}

void CTableFrameSink::OnTimerEnd()
{
      //连续5局未下注踢出房间
      //CheckKickOutUser();
	m_bGameEnd = false;
      m_startTime = chrono::system_clock::now();
//    if (--m_nGameEndLeveTime < 1)
//    {
//        m_TimerIdEnd.stop();
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
        ClearTableData();
        ChangeVipUserInfo();

//        openLog("  ");
        //结束游戏
        bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
        if(!IsContinue)
            return;

//         openLog("  ");
//         openLog("start usercoount=%d ",m_UserInfo.size());

        LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();

//         for(auto iterinfo = m_UserInfo.begin(); iterinfo != m_UserInfo.end(); ++iterinfo)
//         {
//             openLog("vipuserid=%d ",iterinfo->dwUserID);
//         }

         bool bRestart = false;
         //if(m_openRecord.size()>100)//record last 20
         //{

         //    //clear open record
         //    m_openRecord.pop_front();
         //    m_openRecordType.pop_front();
         //    //bRestart = true;  // no use now
         //}

		 m_pITableFrame->SetGameStatus(GAME_STATUS_START);
		 m_cbGameStatus = GAME_STATUS_START;
         m_strRoundId = m_pITableFrame->GetNewRoundId();
		 m_strTableId = to_string(m_ipCode) + "-" + to_string(m_pITableFrame->GetTableId() + 1);	//桌号
		 m_bAddRecord = false;
         m_replay.gameinfoid = m_strRoundId;
         m_dwReadyTime = (uint32_t)time(NULL);
         m_replay.clear();
         shared_ptr<IServerUserItem> pUserItem;
         shared_ptr<UserInfo> userInfoItem;
		 m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata		 
         for(auto &it : m_UserInfo)
         {
             if (!it.second)
                 continue;
             userInfoItem = it.second;
             pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
             if(!pUserItem)
                 continue;
             if(bRestart)
                SendOpenRecord(userInfoItem->userId);

             SendStartMsg(userInfoItem->chairId);
         }
		 
        //m_pITableFrame->SetGameStatus(GAME_STATUS_START);

        //设置时间
//        m_TimerJetton.start(1000, bind(&CTableFrameSink::OnTimerJetton, this, std::placeholders::_1), NULL, m_nPlaceJettonLeveTime, false);
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(m_ngameTimeJettonBroadcast, boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    //}
}

void CTableFrameSink::ClearTableData()
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));
    memset(m_arr_jetton,0,sizeof(m_arr_jetton));
    memset(m_iarr_tmparea_jetton,0,sizeof(m_iarr_tmparea_jetton));
    memset(tmpJttonList,0,sizeof(tmpJttonList));
    memset(iarr_tmparea_jetton,0,sizeof(iarr_tmparea_jetton));
    memset(m_cbTableCardArray, 0, sizeof(m_cbTableCardArray));
    for(int64_t jetton : m_vAllchips)
    {
        m_Hong[jetton] = 0;
        m_Hei[jetton] = 0;
        m_Special[jetton] = 0;
    }


    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
		// 否则把重押分数记录
		for (int j = 0; j < MAX_COUNT; j++)
		{
			userInfoItem->dLastJetton[j] = userInfoItem->m_lUserJettonScore[j];
		}
        userInfoItem->clear();
    }
    ReadConfigInformation();
	memset(m_bAreaIsWin, 0, sizeof(m_bAreaIsWin));
}

bool  CTableFrameSink::OnMakeDir()
{
	std::string dir="./Log";  
    if(access(dir.c_str(), 0) == -1)
    {  
        if(mkdir(dir.c_str(), 0755)== 0)
		{ 
			dir="./Log/zjh";  

            if(access(dir.c_str(), 0) == -1)
			{  
                return (mkdir(dir.c_str(), 0755)== 0);
			}  
		}
    }  	

	return false;
}

//游戏消息处理
//bool  CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem)
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //::muduo::MutexLockGuard lock(m_mutexLock);
//    LOG(INFO) << "CTableFrameSink::OnGameMessage wChairID:" << wChairID << ", wSubCmdID:" << wSubCmdID << ", wDataSize:" << wDataSize;

    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if (!pIServerUserItem)
    {
        LOG(INFO) << "OnGameMessage pServerOnLine is NULL  wSubCmdID:" << wSubCmdID;
		return false;
    }

	switch (wSubCmdID)
	{
        case SUB_C_PLACE_JETTON:			//用户下注
		{
//            LOG(INFO) << " >>> OnUserJETTON wChairID:" << wChairID;

            //用户效验
            /*
            //WORD wChairID = pIServerUserItem->GetChairID();
            if (!pIServerUserItem->isPlaying())
            {
                return true;
            }
            */

//            if(pIServerUserItem->GetUserID() == 15935)
//                 openLog("chairID=%d",wChairID);
			//状态判断
//            assert(m_cbPlayStatus[wChairID]);
//            if(!m_cbPlayStatus[wChairID])
        CMD_C_PlaceJet  pJETTON;
        pJETTON.ParseFromArray(pData,wDataSize);

        //消息处理
        return OnUserAddScore(wChairID, pJETTON.cbjettonarea(), pJETTON.ljettonscore());
    }
    case SUB_C_QUERY_PLAYERLIST:
    {
//            LOG(INFO) << " >>> OnUserJETTON wChairID:" << wChairID;

        //用户效验
        /*
        WORD wChairID = pIServerUserItem->GetChairID();
        if (!pIServerUserItem->isPlaying())
        {
            return true;
//                return false;

            //变量定义
            }
            */

            //状态判断
//            assert(m_cbPlayStatus[wChairID]);
//            if(!m_cbPlayStatus[wChairID])
//                return false;

//            OnPlayerList(wChairID);
            return true;
        }
    case SUB_C_REJETTON :
        {
            //CMD_C_ReJetton
            //return OnUserRejetton(wChairID);
			Rejetton(wChairID, wSubCmdID, pData, wDataSize); //续押
			return true;
        }
        return false;
    }
}


//加注事件
bool CTableFrameSink::OnUserAddScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, bool bRejetton)
{
     //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    //LOG(INFO)<<"cbJettonArea="<<cbJettonArea<<"lJettonScore"<<lJettonScore;
    if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
          string err = "当前不是下注时间";
          SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, err);
          return true;
    }
    //=====================================
    bool bFind = false;
    for(int i=0;i<CHIP_COUNT;i++)
    {
        if(m_currentchips[wChairID][i]==lJettonScore)
        {
            bFind = true;
            break;
        }
    }
//    for(auto it : m_pGameRoomInfo->jettons)
//    {
//        if(it == lJettonScore)
//        {
//            bFind = true;
//            break;
//        }
//    }
    //if (bRejetton) bFind = true;
    if(!bFind||cbJettonArea<0||cbJettonArea>2 )
    {
        string err = "下注金额不正确";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore,err);
        return true;
    }
    //======================================
	//校验是否当前用户
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pIServerUserItem)
    {
        string err = "玩家不存在";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore,err);
        return true;
    }
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();

    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lUserJettonScore[SPECIAL] + userInfo->m_lUserJettonScore[BLACK] + userInfo->m_lUserJettonScore[RED];
        if(m_AreaMaxJetton[cbJettonArea]<userInfo->m_lUserJettonScore[cbJettonArea]+lJettonScore)//
        {
            string err = "下注已达区域上限";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, err);
            return true;
        }
        if (lUserScore - lTotalJettonScore-lJettonScore < 0)
        {
            string err = "筹码不足";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, err);
            return true;
        }
        if(lTotalJettonScore == 0)
       {
           m_replay.addPlayer(userId,pIServerUserItem->GetAccount(),lUserScore,wChairID);
       }
       m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,wChairID,cbJettonArea);
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
//        m_lUserJettonScore[cbJettonArea][wChairID] += lJettonScore;
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
    }else //android suer
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lRobotUserJettonScore[SPECIAL] + userInfo->m_lRobotUserJettonScore[BLACK] + userInfo->m_lRobotUserJettonScore[RED];
        if(m_AreaMaxJetton[cbJettonArea]<userInfo->m_lRobotUserJettonScore[cbJettonArea]+lJettonScore)
        {
            string err = "下注已达区域上限";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, err);
            return true;
        }
        if (lUserScore - lTotalJettonScore-lJettonScore < 0){
            string err = "筹码不足";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, err);
            return true;
        }
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lRobotserJettonAreaScore[cbJettonArea] += lJettonScore;
//        m_lRobotUserJettonScore[cbJettonArea][wChairID] += lJettonScore;
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
    }

    if(m_ShenSuanZiId==userId)
    {
//        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
//        if(SSZUserInfo->chairId == wChairID)
            m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }

    //openLog("userid=%d area=%d score=%f",pIServerUserItem->GetUserID(),cbJettonArea,lJettonScore);
    //成功返回
    if(cbJettonArea == SPECIAL)
    {
        m_Special[lJettonScore]++;
    }else if(cbJettonArea == RED)
    {
        m_Hong[lJettonScore]++;
    }else if(cbJettonArea == BLACK)
    {
        m_Hei[lJettonScore]++;
    }
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    CMD_S_PlaceJetSuccess Jetton;
    auto push=[&](uint32_t iChairId,int64_t sendUserId ,bool Robot)
    {
        Jetton.Clear();
        Jetton.set_dwuserid(userId);
        Jetton.set_cbjettonarea(cbJettonArea);
        Jetton.set_ljettonscore(lJettonScore);
        bool isRobot = pIServerUserItem->IsAndroidUser();
        Jetton.set_bisandroid(isRobot);
        int64_t xzScore = 0;
        if(isRobot)
            xzScore = userInfo->m_lRobotUserJettonScore[SPECIAL] + userInfo->m_lRobotUserJettonScore[BLACK] + userInfo->m_lRobotUserJettonScore[RED];
        else
            xzScore = userInfo->m_lUserJettonScore[SPECIAL] + userInfo->m_lUserJettonScore[BLACK] + userInfo->m_lUserJettonScore[RED];
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 0; i < MAX_COUNT; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 0; i < MAX_COUNT; ++i)
        {
            PlaceJettonScore* userJetton = Jetton.add_selfjettonscore();
            userJetton->set_dwuserid(sendUserId);
            userJetton->set_ljettonarea(i);
            int64_t jettonScore = 0;
            if(Robot)
                jettonScore =  userInfoItem->m_lRobotUserJettonScore[i];
             else
                jettonScore =  userInfoItem->m_lUserJettonScore[i];
             userJetton->set_ljettonscore(jettonScore);
        }

        std::string data = Jetton.SerializeAsString();
        m_pITableFrame->SendTableData(iChairId, HongHei::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };

    if ( !isInVipList(wChairID) && userId!=m_ShenSuanZiId && m_UserInfo.size() >= 8)
    {
        userInfoItem=m_UserInfo[userId];
        push(wChairID,userId,pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }else
    {
        //LOG(INFO)<<"no in list jetton userid = "<<userId;
        for(auto &it : m_UserInfo)
        {
            if (!it.second)
                continue;

            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem||pUserItem->IsAndroidUser())
                continue;
//            int64_t sendUserId = userInfoItem->userId;
//            uint32_t sendChairId = userInfoItem->chairId;
//            bool Robot = pUserItem->IsAndroidUser();
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}

bool CTableFrameSink::OnUserRejetton(uint32_t chairId)
{
    shared_ptr<CServerUserItem>User=m_pITableFrame->GetTableUserItem(chairId);
    shared_ptr<UserInfo> userinfo=m_UserInfo[User->GetUserId()];
    if(!User)
        return false;
    if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
          string err = "当前不是下注时间";
          SendPlaceBetFail(chairId, 4, 0,err);
          return true;
    }
    int64_t alljetton=0;
    int64_t areajetton[MAX_COUNT]={0};
    for(int i=0;i<MAX_COUNT;i++)
    {
        alljetton+=userinfo->m_lPreUserJettonScore[i];
        areajetton[i]=userinfo->m_lPreUserJettonScore[i];
    }
    int64_t lTotalJettonScore = userinfo->m_lRobotUserJettonScore[SPECIAL] + userinfo->m_lRobotUserJettonScore[BLACK] + userinfo->m_lRobotUserJettonScore[RED];
    if(alljetton<0||User->GetUserScore()<lTotalJettonScore+alljetton)
    {
        string err="筹码不足";
        SendPlaceBetFail(chairId,4,alljetton,err);
        return false;
    }
    for(int i=0;i<MAX_COUNT;i++)
    {
        if(m_AreaMaxJetton[i]<areajetton[i]+userinfo->m_lUserJettonScore[i])
        {
            string err="下注已达区域上限";
            SendPlaceBetFail(chairId,4,alljetton,err);
            return false;
        }
    }

    for(int i=0;i<MAX_COUNT;i++)
    {
        if(areajetton[i]==0)
        {
            continue;
        }
        OnUserAddScore(chairId,i,areajetton[i],true);
//        while(areajetton[i]>=1)
//        {
//            if(areajetton[i]>=m_pGameRoomInfo->jettons[5])
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[5]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[5];
//            }
//            else if(areajetton[i]>=m_pGameRoomInfo->jettons[4])
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[4]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[4];
//            }
//            else if(areajetton[i]>=m_pGameRoomInfo->jettons[3])
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[3]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[3];
//            }
//            else if(areajetton[i]>=m_pGameRoomInfo->jettons[2])
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[2]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[2];
//            }
//            else if(areajetton[i]>=m_pGameRoomInfo->jettons[1])
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[1]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[1];
//            }
//            else
//            {
//                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[0]);
//                areajetton[i]-=m_pGameRoomInfo->jettons[0];
//            }
//        }
    }
    return true;
}

void CTableFrameSink::AddPlayerJetton(int64_t chairid, int areaid, int score)
{
    assert(chairid<GAME_PLAYER &&areaid<3);

    m_arr_jetton[chairid].bJetton =true; //special black red 0 1 2
    m_arr_jetton[chairid].iAreaJetton[areaid] +=score;
    m_iarr_tmparea_jetton[areaid]+=score;
}

void CTableFrameSink::GameTimerJettonBroadcast()
{
    memset(tmpJttonList,0,sizeof(tmpJttonList));
    memset(iarr_tmparea_jetton,0,sizeof(iarr_tmparea_jetton));
    {
        swap(m_arr_jetton,tmpJttonList);
        swap(m_iarr_tmparea_jetton,iarr_tmparea_jetton);
    }

    HongHei::CMD_S_Jetton_Broadcast jetton_broadcast;

    //int iJettonCount =0;
   // double alljetton=0.0;
    int listNum=0;
    bool havejetton=false;
    for (int x=0; x<MAX_COUNT; x++)
    {
        if (iarr_tmparea_jetton[x] > 0)
        {
            havejetton = true;
            break;
        }
    }
    if (havejetton)//check jetton
    {
        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem||pUserItem->IsAndroidUser())
                continue;
            jetton_broadcast.Clear();

            for (int x=0; x<MAX_COUNT; x++)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[x]);
               jetton_broadcast.add_tmpjettonscore(iarr_tmparea_jetton[x]- tmpJttonList[userInfoItem->chairId].iAreaJetton[x]);
            }
            std::string data = jetton_broadcast.SerializeAsString();
            m_pITableFrame->SendTableData(userInfoItem->chairId, HongHei::SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
        }
    }
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    {
        m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(m_ngameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::SendPlaceBetFail(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore,string str_err)
{
    CMD_S_PlaceJettonFail Jetton;

    shared_ptr<IServerUserItem> pUserItem;
    pUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pUserItem)
        return;
     int64_t userid = pUserItem->GetUserId();
     bool isRobot = pUserItem->IsAndroidUser();
     Jetton.set_dwuserid(userid);
     Jetton.set_cbjettonarea(cbJettonArea);
     Jetton.set_lplacescore(lJettonScore);
     Jetton.set_cbandroid(isRobot);
     Jetton.set_errmsg(str_err);
     std::string data = Jetton.SerializeAsString();
     m_pITableFrame->SendTableData(wChairID, HongHei::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendWinSysMessage()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if(userInfoItem->m_EndWinScore >= m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
            //char formate[2048]={0};
            //sprintf(formate,"恭喜V%d玩家%s在龙虎中一把赢得%.2f元!",pUserItem->GetVipLevel(),pUserItem->GetNickName(),m_EndWinScore[i]);
            //m_pITableFrame->SendGameMessage(INVALID_CARD,formate,SMT_GLOBAL|SMT_SCROLL);
            m_pITableFrame->SendGameMessage(userInfoItem->chairId, "", SMT_GLOBAL|SMT_SCROLL, userInfoItem->m_EndWinScore);
        }
    }
}

string CTableFrameSink::GetCardValueName(int index)
{
    string strName;

    //获取花色
    uint8_t cbColor = m_Algorithm.m_GameLogic.GetCardColor(m_cbTableCardArray[0]);
    uint8_t cbValue = m_Algorithm.m_GameLogic.GetCardValue(m_cbTableCardArray[0]);
    switch(cbColor)
    {
        case 3:
            strName = "黑";
            break;
        case 2:
            strName = "红";
            break;
        case 1:
            strName = "梅";
            break;
        case 0:
            strName = "方";
            break;
        default:
            break;
    }
    switch(cbValue)
    {
        case 1:
            strName += "A";
            break;
        case 2:
            strName += "2";
            break;
        case 3:
            strName += "3";
            break;
        case 4:
            strName += "4";
            break;
        case 5:
            strName += "5";
            break;
        case 6:
            strName += "6";
            break;
        case 7:
            strName += "7";
            break;
        case 8:
            strName += "8";
            break;
        case 9:
            strName += "9";
            break;
        case 10:
            strName += "10";
            break;
        case 11:
            strName += "J";
            break;
        case 12:
            strName += "Q";
            break;
        case 13:
            strName += "K";
            break;
        default:
            break;
    }
    return strName;
}

////黑白名单
//void CTableFrameSink::ListControl()
//{

//}


// try to create the special table frame sink item now.
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
    pSink.reset();
}



//bool CTableFrameSink::SaveTableLog()
//{
//	if (NULL == m_pITableFrame) return false;

//	CString strFileName;
//	int ran = rand() % 10000 + 1000;
//	long num = time(NULL);
//    strFileName.Format(TEXT("./log/%s/%d_%ld_%d"), "HongHei", m_pITableFrame->GetTableID(), num, ran);
//	CLog log;

//	if (log.OpenLog(strFileName))
//	{
//		log.TraceString(m_GameRecord);
//		log.CloseLog();
//	}

//	return true;
//}

//bool CTableFrameSink::WriteStorage(WORD wWinner, google::protobuf::RepeatedField<SCORE>& lGameScore)
//{

//	return true;
//}


void  CTableFrameSink::openLog(const char *p, ...)
{
    if (m_IsEnableOpenlog)
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/honghei/HongHei_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

void CTableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo){
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (userInfoItem->NotBetJettonCount>=5)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(userInfoItem->userId);            
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);            
            m_pITableFrame->ClearTableUser(userInfoItem->chairId,true,true);
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->chairId<<" userId="<<userInfoItem->userId;          
        } 
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_UserInfo.erase(clearUserIdVec[i]);
    }
}

//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
	HongHei::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分
	//系统开牌
	for (int i = 0; i < 2; ++i)
	{
        SysCards* card = details.add_syscard();
		card->set_cards(&(m_cbTableCardArray)[i * 3], MAX_COUNT);
        card->set_ty(m_TableCardType[i]);
		card->set_multi(m_SpecialTimes[m_TableCardType[i]]);
	}

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		//special black red 0 1 2
		for (int betIdx = 0;betIdx < MAX_COUNT; betIdx++)
		{
			// 结算时记录
            int64_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->m_lUserJettonScore[betIdx];
			int32_t	nMuilt = m_nMuilt[betIdx];// 倍率表m_nMuilt
			//if (betScore > 0) 
			{
				HongHei::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//区域倍数
				detail->set_dareaidmul(nMuilt);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
				//本区域是否赢[0:否;1:赢]
				detail->set_dareaiswin(m_bAreaIsWin[betIdx]);
			}
		}
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}


void CTableFrameSink::updateGameMsg(bool isInit)
{
	if (!isInit)
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdUpdateGameMsg);

	Json::FastWriter writer;
	Json::Value gameMsg;

	gameMsg["tableid"] = m_pITableFrame->GetTableId(); //桌子ID
	gameMsg["gamestatus"] = m_cbGameStatus; //游戏状态
	gameMsg["time"] = getGameCountTime(m_cbGameStatus); //倒计时
	gameMsg["goodroutetype"] = 0; //好路类型
	gameMsg["routetypename"] = ""; //好路名称

	string gameinfo = writer.write(gameMsg);

	Json::Value details;
	Json::Value countMsg;
	Json::Value routeMsg;
	Json::Value routeTypeMsg;

	//details["routetype"] = m_nGoodRouteType;//好路类型
	//countMsg["t"] = m_winOpenCount[0];//幸运一击数量
	countMsg["b"] = m_winOpenCount[BLACK];//黑数量
	countMsg["r"] = m_winOpenCount[RED];//红数量
	//countMsg["tt"] = m_winOpenCount[3];//和对数量
	//countMsg["bt"] = m_winOpenCount[4];//庄对数量
	//countMsg["pt"] = m_winOpenCount[5];//闲对数量
	countMsg["player"] = m_pITableFrame->GetPlayerCount(true);//桌子人数

	details["count"] = countMsg;//各开奖次数
	//details["routetypename"] = m_strGoodRouteName; //好路名称
	details["limitmin"] = m_lLimitScore[0];//限红低
	details["limitmax"] = m_lLimitScore[1];//限红高

	//开奖记录
	routeMsg = Json::arrayValue;
	if (m_openRecord.size() > 0)
	{
		for (int i = 0;i < m_openRecord.size();i++)
		{
			routeMsg.append(m_openRecord[i]);
		}
	}
	details["route"] = routeMsg;
	//开奖牌型记录
	routeTypeMsg = Json::arrayValue;
	int TypeSize = m_openRecordType.size();
	if (TypeSize <= 15)
	{
		for (int i = 0;i < m_openRecordType.size();i++)
		{
			routeTypeMsg.append(m_openRecordType[i]);
		}
	}
	else
	{
		for (int i = TypeSize - 15;i < TypeSize;i++)
		{
			routeTypeMsg.append(m_openRecordType[i]);
		}
	}
	details["routetype"] = routeTypeMsg;

	string detailsinfo = writer.write(details);
	/*GlobalFunc::replaceChar(detailsinfo, '\\n');
	GlobalFunc::replaceChar(detailsinfo, '\\');*/
	//LOG(INFO) << "updateGameMsg gameinfo " << gameinfo << " detailsinfo " << detailsinfo;
	// 写数据库
	m_pITableFrame->CommonFunc(eCommFuncId::fn_id_1, gameinfo, detailsinfo);
	if (!isInit)
		m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, boost::bind(&CTableFrameSink::updateGameMsg, this, false));
}

//获取游戏状态对应的剩余时间
int32_t CTableFrameSink::getGameCountTime(int gameStatus)
{
	int countTime = 0;
	int32_t leaveTime = 0;
	int32_t allTime = 0;
	chrono::system_clock::time_point gameStateStartTime;
	if (gameStatus == GAME_STATUS_INIT) // 初始化时间
	{
		allTime = m_nPlaceJettonTime;
		gameStateStartTime = m_startTime;
	}
	else if (gameStatus == GAME_STATUS_START) //下注时间
	{
		allTime = m_nPlaceJettonTime;
		gameStateStartTime = m_startTime;
	}
	else //结束时间
	{
		allTime = m_nGameEndTime;
		gameStateStartTime = m_endTime;
	}
	chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - gameStateStartTime);
	leaveTime = allTime - durationTime.count();
	countTime = (leaveTime > 0 ? leaveTime : 0);

	return countTime;
}

void CTableFrameSink::RefreshPlayerList(bool isGameEnd)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
	
	m_openRecord.push_back(m_winFlag);
	m_openRecordType.push_back(m_winType);
	if (m_openRecord.size() > 100)//record last 20
	{
		m_openRecord.pop_front();
		m_openRecordType.pop_front();
	}
	m_bAddRecord = true;
	int size = m_openRecord.size();
	int heigWin = 0;
	int hongWin = 0;
	int xinyunWin = 0;

	for (int i = 0; i < size; ++i)
	{
		if (m_openRecord[i] == BLACK)
			heigWin++;
		else if (m_openRecord[i] == RED)
			hongWin++;

		if (m_openRecordType[i] > CT_SINGLE)
			xinyunWin++;
	}

	//下注项累计开奖的次数
	m_winOpenCount[BLACK] = heigWin;
	m_winOpenCount[RED] = hongWin;
	m_winOpenCount[SPECIAL] = xinyunWin;

	string stsRsult = "";
	for (int i = 0;i < MAX_COUNT;i++)
	{
		stsRsult += to_string(i) + "_" + to_string(m_winOpenCount[i]) + " ";
	}
	openLog("开奖结果累计 stsRsult = %s", stsRsult.c_str());

	if (isGameEnd && m_bGameEnd)
	{
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<UserInfo> userInfoItem;
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem)
				continue;
			SendOpenRecord(userInfoItem->chairId);
		}
	}
}

// 重押
void CTableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
	if (!Usr) { LOG(WARNING) << "----查找玩家不在----"; return; }

	/*if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
	{
		LOG(WARNING) << "-------------续押失败(玩家状态不对)-------------";
		return;
	}*/

	LOG(WARNING) << "-----------Rejetton续押-------------";
	// 游戏状态判断
	if (GAME_STATUS_START != m_cbGameStatus)
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_cbGameStatus << " " << GAME_STATUS_START;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[MAX_COUNT] = { 0 };
	int64_t userid = Usr->GetUserId();

	CMD_C_ReJetton reJetton;
	reJetton.ParseFromArray(data, datasize);
	int reSize = reJetton.ljettonscore_size();
	for (int i = 0;i < reSize;i++)
	{
		m_UserInfo[userid]->dLastJetton[i] = reJetton.ljettonscore(i);
		if (m_UserInfo[userid]->dLastJetton[i] < 0)
		{
			LOG(ERROR) << "---续押押分有效性检查出错--->" << i << " " << m_UserInfo[userid]->dLastJetton[i];
		}
	}
	for (int i = 0; i < MAX_COUNT; i++)
	{
		alljetton += m_UserInfo[userid]->dLastJetton[i];
		areajetton[i] = m_UserInfo[userid]->dLastJetton[i];
		LOG(WARNING) << "--dLastJetton-" << areajetton[i];
		if (areajetton[i] < 0)
		{
			alljetton = 0;
			break;
		}
	}

	// 续押失败
	if (alljetton == 0 || Usr->GetUserScore() < alljetton)
	{
		CMD_S_PlaceJettonFail placeJetFail;

		placeJetFail.set_dwuserid(userid);
		placeJetFail.set_cbjettonarea(3);
		placeJetFail.set_lplacescore(0);
		placeJetFail.set_cbandroid(Usr->IsAndroidUser());
		placeJetFail.set_errmsg("筹码不足");
		std::string sendData = placeJetFail.SerializeAsString();
		m_pITableFrame->SendTableData(Usr->GetChairId(), HongHei::SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
		LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
		return;
	}

	for (int i = 0; i < MAX_COUNT; i++)
	{
		// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
		if (m_AreaMaxJetton[i] < m_UserInfo[userid]->m_lUserJettonScore[i] + m_UserInfo[userid]->dLastJetton[i])
		{
			CMD_S_PlaceJettonFail placeJetFail;
			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i);
			placeJetFail.set_lplacescore(m_UserInfo[userid]->dLastJetton[i]);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());
			placeJetFail.set_errmsg("下注已达区域上限");
			std::string data = placeJetFail.SerializeAsString();
			//if (m_bTest==0)
			m_pITableFrame->SendTableData(Usr->GetChairId(), HongHei::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
			LOG(WARNING) << "---------续押失败(下注已达区域上限)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->m_lUserJettonScore[i] << "+" << m_UserInfo[userid]->dLastJetton[i];
			return;
		}
	}
	// LOG(ERROR) << "---------重押成功--------";
	for (int i = 0; i < MAX_COUNT; i++)
	{
		if (areajetton[i] <= 0)
		{
			continue;
		}

		while (areajetton[i] > 0)
		{
			int jSize = CHIP_COUNT;
			for (int j = jSize - 1;j >= 0;j--)
			{
				if (areajetton[i] >= m_currentchips[chairid][j])
				{
                    int64_t tmpVar = m_currentchips[chairid][j];
					OnUserAddScore(chairid, i, tmpVar, true);
					areajetton[i] -= tmpVar;
					break;
				}
			}
            if(areajetton[i]<m_currentchips[chairid][0])
            {
                break;
            }
		}
	}
}


//void CTableFrameSink::WriteLosterUserScore(WORD wChairID)
//{
//	if (INVALID_CHAIR == wChairID) return;

//	//修改积分
//    tagScoreInfo ScoreInfoArray[GAME_PLAYER]={0};
//    ScoreInfoArray[wChairID].nRevenue = 0;

//    /*
//    if (m_cbPlayStatus[wChairID] == PLAYER_DESPERATE)
//	{
//        ScoreInfoArray[wChairID].nAddScore = Double_Round(-m_lUserMaxScore[wChairID] / D_TIMES, 2, 1);
//	}
//	else
//	{
//        ScoreInfoArray[wChairID].nAddScore = Double_Round(-m_lTableScore[wChairID] / D_TIMES, 2, 1);
//	}

//    m_GameRecord.AppendFormat(TEXT("\nP%ld	Score=%.2lf	lRevenue=%.2lf#"), wChairID, ScoreInfoArray[wChairID].nAddScore, ScoreInfoArray[wChairID].nRevenue);
//*/
//	m_pITableFrame->WriteUserScore(ScoreInfoArray, CountArray(ScoreInfoArray));
//}

//void CTableFrameSink::InitAddScore(uint32_t chairId)
//{
//    shared_ptr<IServerUserItem> user = m_pITableFrame->GetTableUserItem(chairId);
//    if(user)
//    {
//        if(user->IsAndroidUser())
//        {
//            m_lRobotUserJettonScore[0][chairId] = 0;
//            m_lRobotUserJettonScore[1][chairId] = 0;
//            m_lRobotUserJettonScore[2][chairId] = 0;
//            m_lRobotUserJettonScore[3][chairId] = 0;
//            m_EndWinScore[chairId]=0;
//        }
//        else
//        {
//            m_lUserJettonScore[0][chairId] = 0;
//            m_lUserJettonScore[1][chairId] = 0;
//            m_lUserJettonScore[2][chairId] = 0;
//            m_lUserJettonScore[3][chairId] = 0;
//            m_EndWinScore[chairId]=0;
//        }
//    }
//}
//CString CTableFrameSink::TransformCardInfo( uint8_t cbCardData )
//{
//	CString str = TEXT("");

//    uint8_t cbCardValue = (cbCardData & LOGIC_MASK_VALUE);
//	switch(cbCardValue)
//	{
//	case 0x01:
//		{
//			str += TEXT("A");
//			break;
//		}
//	case 0x02:
//	case 0x03:
//	case 0x04:
//	case 0x05:
//	case 0x06:
//	case 0x07:
//	case 0x08:
//	case 0x09:
//		{
//			str.Format( TEXT("%d"),cbCardValue);
//			break;
//		}
//	case 0x0A:
//		{
//			str += TEXT("10");
//			break;
//		}
//	case 0x0B:
//		{
//			str += TEXT("J");
//			break;
//		}
//	case 0x0C:
//		{
//			str += TEXT("Q");
//			break;
//		}
//	case 0x0D:
//		{
//			str += TEXT("K");
//			break;
//		}
//	default:
//        // ASSERT(FALSE);
//        str = "未知";
//    }

//    uint8_t cbCardColor = (cbCardData & LOGIC_MASK_COLOR);
//	switch( cbCardColor )
//	{
//	case 0x00:
//		str += TEXT("方块");
//		break;
//	case 0x10:
//		str += TEXT("梅花");
//		break;
//	case 0x20:
//		str += TEXT("红桃");
//		break;
//	case 0x30:
//		str += TEXT("黑桃");
//		break;
//	default:
//        // ASSERT(FALSE);
//        str = "未知";
//	}
//	return str;
//}
////定时器事件
//bool CTableFrameSink::OnTimerMessage(DWORD wTimerID, WPARAM wBindParam)
//{
//    // ::muduo::MutexLockGuard lock(m_mutexLock);
//    if(wTimerID == IDI_PLACE_JETTON)
//	{
//        if(--m_nPlaceJettonLeveTime < 1)
//		{
//            m_pITableFrame->KillGameTimer(IDI_PLACE_JETTON);

//            //结束游戏
//            OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

//            //openLog("  ");
//            //  openLog("  ");
//            //openLog("  ");
//            m_nGameEndLeveTime= m_nGameEndTime;
//            for(int i = 0; i < GAME_PLAYER; ++i)
//            {
//                IServerUserItem * pUserItem = nullptr;
//                pUserItem = m_pITableFrame->GetTableUserItem(i);
//                if(!pUserItem)
//                    continue;
//                SendEndMsg(i);
//            }
//            SendWinSysMessage();
//            //设置状态
//            m_pITableFrame->SetGameStatus(GAME_STATUS_END);

//            //删除玩家信息
//            clearTableUser();  //sOffline  m_cbPlayStatus==false
//            ChangeVipUserInfo();
//            openLog("  ");
//            openLog("end usercoount=%d ",m_UserInfo.size());

//            //设置时间
//            m_pITableFrame->SetGameTimer(IDI_GAME_END, 1000, -1, 0L);
//		}
//		return true;
//	}
//    else if(IDI_GAME_END == wTimerID)
//	{
//        if (--m_nGameEndLeveTime < 1)
//		{
//            m_pITableFrame->KillGameTimer(IDI_GAME_END);

//            //结束返回
//            //修改数据库
//            //保存税收
//            ChangeVipUserInfo();
//            openLog("  ");
//            m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
//            openLog("start usercoount=%d ",m_UserInfo.size());
//            for(auto iterinfo = m_UserInfo.begin(); iterinfo != m_UserInfo.end(); ++iterinfo)
//            {
//                openLog("vipuserid=%d ",iterinfo->dwUserID);
//            }
//            //结束游戏
//             m_pITableFrame->ConcludeGame(GAME_STATUS_START);

//             memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
//             memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
//             memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));

//             memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
//             memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

//             memset(m_CalculateRevenue, 0, sizeof(m_CalculateRevenue));
//             memset(m_CalculateAgentRevenue, 0, sizeof(m_CalculateAgentRevenue));

//             memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));
//             memset(m_EndWinScore, 0, sizeof(m_EndWinScore));
//             memset(m_TotalGain, 0, sizeof(m_TotalGain));
//             memset(m_EndRealWinScore, 0, sizeof(m_EndRealWinScore));

//             memset(m_Hong, 0, sizeof(m_Hong));
//             memset(m_Hei, 0, sizeof(m_Hei));
//             memset(m_he, 0, sizeof(m_he));


//             //openLog("  ");
//             //openLog("  ");
//            // openLog("  ");
//            if(m_CardVecData.size() < MIN_LEFT_CARDS_RELOAD_NUM)
//            {
//                m_CardVecData.clear();
//                m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
//                random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

//                m_openRecord.clear();
//                for(int i = 0; i < GAME_PLAYER; ++i)
//                {
//                    if(!m_cbPlayStatus[i])
//                        continue;

//                    IServerUserItem * pUserItem = nullptr;
//                    pUserItem = m_pITableFrame->GetTableUserItem(i);
//                    if(!pUserItem)
//                        continue;
//                    SendOpenRecord(pUserItem->GetUserID());
//                }
//            }

//            for(int i = 0; i < GAME_PLAYER; ++i)
//            {
//                if(!m_cbPlayStatus[i])
//                    continue;

//                IServerUserItem *  pUserItem = nullptr;
//                pUserItem = m_pITableFrame->GetTableUserItem(i);
//                if(!pUserItem)
//                    continue;
//                SendStartMsg(i);
//            }
//            m_pITableFrame->SetGameStatus(GAME_STATUS_START);
//            openLog("start.......................");

//            openLog("start end.......................");

//            //设置时间
//            m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON, 1000, -1, 0L);
//		}
//        return true;
//    }
//	return false;
//}
//bool CTableFrameSink::OnTimerMessage(DWORD wTimerID, WPARAM wBindParam)
//{
//    // ::muduo::MutexLockGuard lock(m_mutexLock);
//    if(wTimerID == IDI_PLACE_JETTON)
//    {
//        if(--m_nPlaceJettonLeveTime < 1)
//        {
////            m_pITableFrame->KillGameTimer(IDI_PLACE_JETTON);

//            //设置时间
////            m_pITableFrame->SetGameTimer(IDI_GAME_END, m_nGameEndTime*1000, -1, 0L);

//            //结束游戏
//            OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

//            //openLog("  ");
//            //  openLog("  ");
//            //openLog("  ");
////            m_nGameEndLeveTime = m_nGameEndTime;

//            for(int i = 0; i < GAME_PLAYER; ++i)
//            {
//                shared_ptr<IServerUserItem> pUserItem;
//                pUserItem = m_pITableFrame->GetTableUserItem(i);
//                if(!pUserItem)
//                    continue;
//                SendEndMsg(i);
//            }
//            SendWinSysMessage();
//            //设置状态
//            m_pITableFrame->SetGameStatus(GAME_STATUS_END);

//            //删除玩家信息
//            clearTableUser();  //sOffline  m_cbPlayStatus==false
//            ChangeVipUserInfo();
//            openLog("  ");
//            openLog("end usercoount=%d ",m_UserInfo.size());

//            //结束返回
//            //修改数据库
//            //保存税收
//            ChangeVipUserInfo();
////            m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
////            m_nGameEndLeaveTime= m_nGameEndTime;
//        }
//        return true;
//    }else if(IDI_GAME_END == wTimerID)
//    {
////        if (--m_nGameEndLeveTime < 1)
//        {
////            m_pITableFrame->KillGameTimer(IDI_GAME_END);
//            m_nPlaceJettonLeveTime = m_nPlaceJettonTime;

//            //结束游戏
//            bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
//            if(false == IsContinue)
//                return true;

//             openLog("  ");
//             openLog("start usercoount=%d ",m_UserInfo.size());
//             for(auto iterinfo = m_UserInfo.begin(); iterinfo != m_UserInfo.end(); ++iterinfo)
//             {
//                 openLog("vipuserid=%d ",iterinfo->dwUserID);
//             }


//             if(m_CardVecData.size() < MIN_LEFT_CARDS_RELOAD_NUM)
//             {
//                 m_CardVecData.clear();
//                 m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
//                 random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

//                 m_bRefreshAllCards = true;
//             }

//            if(m_bRefreshAllCards)
//            {
//                m_openRecord.clear();
//                for(int i = 0; i < GAME_PLAYER; ++i)
//                {
//                    if(!m_cbPlayStatus[i])
//                        continue;

//                    shared_ptr<IServerUserItem> pUserItem;
//                    pUserItem = m_pITableFrame->GetTableUserItem(i);
//                    if(!pUserItem)
//                        continue;
//                    SendOpenRecord(pUserItem->GetUserID());
//                }
//            }

//            for(int i = 0; i < GAME_PLAYER; ++i)
//            {
//                if(!m_cbPlayStatus[i])
//                    continue;

//                shared_ptr<IServerUserItem> pUserItem;
//                pUserItem = m_pITableFrame->GetTableUserItem(i);
//                if(!pUserItem)
//                    continue;
//                SendStartMsg(i);
//            }

//            m_pITableFrame->SetGameStatus(GAME_STATUS_START);

//            ClearTableData();

//            openLog("start end.......................");

//            //设置时间
////            m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON, 1000, -1, 0L);
//        }
//        return true;
//    }
//    return false;
//}

//pPlayerList
//bool CTableFrameSink::OnPlayerList(uint32_t wChairID)
//{
//    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
//    if(!pIServerUserItem)
//        return false;

//    list<PlayerInfo> playerInfo;
//    {
//        READ_LOCK(m_mutex);
//        playerInfo = m_UserInfo;
//    }

//    CMD_S_PlayerList pPlayerList;
//    int size = playerInfo.size() > 50 ? 50 : playerInfo.size();

//    pPlayerList.set_nendindex(0);
//    pPlayerList.set_nresultcount(size);

//    openLog("play list count=%d",playerInfo.size());

//    list<PlayerInfo>::iterator iterinfo = playerInfo.begin();
//    for(int i = 0; i < size && iterinfo != playerInfo.end(); ++i, ++iterinfo)
//    {
//        PlayerListItem* playre = pPlayerList.add_players();
//        playre->set_dwuserid(iterinfo->dwUserID);
//        playre->set_headerid(iterinfo->headerID);
////        playre->set_nviplevel(iterinfo->nVipLevel);
//        playre->set_nickname(iterinfo->nickName);
////        playre->set_gender(0);
//        //playre->set_luserscore(playerInfo[i].lUserScore);
//        // playre->set_ltwentywinscore(playerInfo[i].lTwentyWinScore);
//        shared_ptr<IServerUserItem> pUserItem;
//        pUserItem = m_pITableFrame->GetTableUserItem(iterinfo->ChairID);
////        int headboxID = 0;
//        if(!pUserItem)
//        {
//            playre->set_luserscore(0);
//        }else
//        {
//            playre->set_luserscore(pUserItem->GetUserScore());
////            headboxID =  pUserItem->GetHeadBoxID();
//        }
////        playre->set_headboxid(headboxID);
//        playre->set_szlocation(iterinfo->location);

//        playre->set_ltwentywinscore(iterinfo->lTwentyAllUserScore);
//        playre->set_ltwentywincount(iterinfo->lTwentyWinCount);
//        int shensuanzi =iterinfo->dwUserID==m_ShenSuanZiId ? 1 : 0;
//        playre->set_isdivinemath(shensuanzi);
//        playre->set_index(i);
//        playre->set_isbanker(0);
//        playre->set_isapplybanker(0);
//        playre->set_applybankermulti(0);
//        playre->set_ljetton(0);
//    }

//    std::string data = pPlayerList.SerializeAsString();
//    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), HongHei::SUB_S_QUERY_PLAYLIST, (uint8_t*)data.c_str(), data.size());
//    return true;
//}

////游戏状态
//bool CTableFrameSink::IsUserPlaying(uint32_t chairId)
//{
//    assert(chairId < m_wPlayerCount);
//    if (chairId >= m_wPlayerCount)
//        return false;

//    return m_cbPlayStatus[chairId];
//}
