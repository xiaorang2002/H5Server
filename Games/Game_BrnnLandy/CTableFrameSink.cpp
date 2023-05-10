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
#include "proto/Brnn.Message.pb.h"

#include "BrnnAlgorithm.h"
//#include "HistoryScore.h"
//#include "IAicsdragon.h"

#include "CTableFrameSink.h"

using namespace Brnn;

#define MIN_LEFT_CARDS_RELOAD_NUM  10
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0
#define MAX_MULITIPLE  3 //牛牛的最大倍数

//天地玄黄输赢
#define	WIN_LOSE_MASK_VALUE			0x0F								//数值掩码
#define LOSE_MASK_TIAN              0x07                                //天输标识位
#define LOSE_MASK_DI                0x0B                                //地输标识位
#define LOSE_MASK_XUAN              0x0D                                //玄输标识位
#define LOSE_MASK_HUANG             0x0E                                //黄输标识位

int64_t CTableFrameSink::m_llStorageControl = 0;
int64_t CTableFrameSink::m_llStorageLowerLimit = 0;
int64_t CTableFrameSink::m_llStorageHighLimit = 0;
int64_t CTableFrameSink::m_llStorageAverLine = 0;
int64_t CTableFrameSink::m_llGap = 0;
// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/brnn/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }

        google::InitGoogleLogging("brnn");
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
//    IAicsDragonEngine* sp ;
//    tagDGSettings gs  ;
//    int status ;
//    int nRoomID;
//};

//class RomMgr
//{
//public:
//    RomMgr()
//    {
//        bisInit = false;

//        if(access("./log/longhu", 0) == -1)
//           mkdir("./log/longhu",0777);
//    }

//    virtual ~RomMgr()
//    {
//        google::ShutdownGoogleLogging();
//    }

//public:
//    RomMgr& Singleton()
//    {
//        static RomMgr* sp = 0;
//        if (!sp)
//        {
//             sp = new RomMgr();
//        }
//    //Cleanup:
//        return (*sp);
//    }

//public:
//    void setInit(int nMaxTable,int roomID)
//    {
//        if (!bisInit)
//        {
//            usleep(15000000);
//            RomInfo rominfo;
//            rominfo.sp=GetAicsDragonEngine();
//            rominfo.gs.nToubbili      = 1;
//            rominfo.gs.nMaxPlayer     = GAME_PLAYER;
//            rominfo.gs.nMaxTable	  = nMaxTable;
//            rominfo.status= rominfo.sp->Init(&rominfo.gs);
//            rominfo.status = rominfo.sp->SetRoomID(roomID);
//            if (sOk == rominfo.status)
//            {
//                bisInit = true;
//            }
//        }
//    }

//     void  getinfo(tagDGGateInfo &info)
//     {
//         IAicsDragonEngine* sp = GetAicsDragonEngine();
//         if (sp)
//         {
//             int status = sp->dragonOnebk(0,0,0,&info);
//         }

//     }
//private:
//     bool bisInit;
//};

//RomMgr romMgr;



//bool PlayerInfoSort (PlayerInfo a,PlayerInfo b) { return (a.lUserScore > b.lUserScore); }
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
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "LongHuTimerEventLoopThread")
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
	memset(m_lBankerAllEndScore, 0, sizeof(m_lBankerAllEndScore));
    memset(m_allCardType, 0, sizeof(m_allCardType));
    memset(m_allMultical, 0, sizeof(m_allMultical));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

//    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
//    memset(m_long, 0, sizeof(m_long));
//    memset(m_hu, 0, sizeof(m_hu));
//    memset(m_he, 0, sizeof(m_he));

    m_strRoundId = "";

    m_Tian.clear();
    m_Di.clear();
    m_Xuan.clear();
    m_Huang.clear();

    memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));

    m_IsEnableOpenlog = false;
    m_IsXuYaoRobotXiaZhu = false;
    m_WinScoreSystem = 0;

    m_nPlaceJettonTime = 0;
//    m_nPlaceJettonLeveTime = 0;
    m_nGameTimeJettonBroadcast = 0;
    m_nGameEndTime = 0;
//    m_nGameEndLeveTime = 0;

    m_ShenSuanZiId = 0;

    m_UserInfo.clear();
    m_SortUserInfo.clear();

    m_openRecord.clear();



    m_CardVecData.clear();
    m_pGameRoomInfo = nullptr;
    srandom(time(NULL));

    stockWeak = 0.0;
    m_dControlkill=0.3;
    m_lLimitPoints=500000;

    for(int i=0;i<GAME_PLAYER;i++)
    {
        m_currentchips[i][0]=1;
        m_currentchips[i][1]=2;
        m_currentchips[i][2]=5;
        m_currentchips[i][3]=10;
        m_currentchips[i][4]=50;
        m_currentchips[i][5]=100;
        m_currentchips[i][6]=500;
    }
    useIntelligentChips=1;

	m_UserBankerList.clear();
	m_wBankerUser = INVALID_CHAIR;
	m_wBankerLimitCount = 0;
	m_wBankerContinuityCount = 0;
	m_wBankerLimitScore = 0;
	m_wBankerApplyLimitCount = 0;
	m_buserCancelBanker = false;
	m_bOpenBankerApply = 0;

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
}

//读取配置
bool CTableFrameSink::ReadConfigInformation()
{


    string ConfPath = "./conf/brnn_config.ini";

    if(m_pGameRoomInfo->roomId == 9301)
    {
        ConfPath= "./conf/brnn_config3.ini";
    }
    else
    {
        ConfPath= "./conf/brnn_config10.ini";
    }


    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists(ConfPath.c_str()))
    {
        LOG_INFO << "./conf/brnn_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(ConfPath.c_str(), pt);

    stockWeak = pt.get<float>("GAME_CONFIG.STOCK_WEAK", 1.0);

    m_dControlkill = pt.get<float>("GAME_CONFIG.CotrolKill", 0.3);

    m_lLimitPoints = pt.get<int64_t>("GAME_CONFIG.LiMiPoint", 500000);
	//限制坐庄的最大局数
	m_wBankerLimitCount = pt.get<int32_t>("GAME_CONFIG.BankerLimitCount", 5);
    m_wBankerLimitScore = pt.get<int64_t>("GAME_CONFIG.BankerLimitScore", 5000000);
	m_wBankerApplyLimitCount = pt.get<int32_t>("GAME_CONFIG.BankerApplyLimitCount", 8);
	m_bOpenBankerApply = pt.get<int32_t>("GAME_CONFIG.bOpenBankerApply", 0);
    //Time
    m_nPlaceJettonTime   = pt.get<int64_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
//    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 7);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 13); // -modify by guansheng
    m_nGameTimeJettonBroadcast= pt.get<float>("GAME_CONFIG.TM_JETTON_BROADCAST", 7);

    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);

    m_WinScoreSystem     = pt.get<int64_t>("GAME_CONFIG.WinScore", 500);
    m_interfere_rate = pt.get<int64_t>("GAME_CONFIG.INTERFER_RATE", 1000);
    m_interfere_line = pt.get<int64_t>("GAME_CONFIG.INTERFER_LINE", 500000);
    m_winRates = to_array<int32_t>(pt.get<string>("GAME_CONFIG.WIN_RATES","70,800,1000"));
    m_lostRates = to_array<int32_t>(pt.get<string>("GAME_CONFIG.LOSE_RATES","70,800,1000"));
    if(m_winRates.size() < 3 )
    {
        m_winRates.push_back(70);
        m_winRates.push_back(800);
        m_winRates.push_back(1000);
        LOG(ERROR) << "config error.";
    }
    if(m_lostRates.size() < 3)
    {
        m_lostRates.push_back(70);
        m_lostRates.push_back(800);
        m_lostRates.push_back(1000);
        LOG(ERROR) << "config error.";
    }

    vector<int64_t> scorelevel;
    vector<int64_t> chips;

    vector<int64_t> allchips;
    allchips.clear();
    scorelevel.clear();
    chips.clear();
    useIntelligentChips=pt.get<int64_t>("CHIP_CONFIGURATION.useintelligentchips", 0);
    scorelevel = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear1","100,200,500,1000,5000,10000,50000"));

    for(int i=0;i<4;i++)
    {
        m_userScoreLevel[i]=scorelevel[i];
    }
    for(int i=0;i<7;i++)
    {
        m_userChips[0][i]=chips[i];
        allchips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear2","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[1][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear3","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[2][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[3][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000,10000,50000"));
    if(!useIntelligentChips)
    {
        for(int i=0;i<7;i++)
        {
             m_userChips[0][i]=chips[i];
             m_userChips[1][i]=chips[i];
             m_userChips[2][i]=chips[i];
             m_userChips[3][i]=chips[i];
        }
    }
    for(int64_t chips : allchips)
    {
        m_Tian[chips] = 0;
        m_Di[chips] = 0;
        m_Xuan[chips] = 0;
        m_Huang[chips] = 0;
    }
    m_IsEnableOpenlog = true;
}

bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
//    romMgr.setInit(pTableFrame->GetGameKindInfo()->nTableCount, pTableFrame->GetGameKindInfo()->nConfigId);

    LOG(INFO) << "CTableFrameSink::SetTableFrame pTableFrame:" << pTableFrame;

    m_pITableFrame = pTableFrame;
    if (pTableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        ReadStorageInfo();

        ReadConfigInformation();
        return true;
    }else
        return false;
}

//游戏开始
void CTableFrameSink::OnGameStart()
{

}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;
	bool bBankerCanleft = true;
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
            totalxz = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3]+ userInfo->m_lUserJettonScore[4];			
        }
		if (status >= GAME_STATUS_START)
		{
			if (INVALID_CHAIR != m_wBankerUser && chairId == m_wBankerUser)
			{
				bBankerCanleft = false;
			}
		}
        if(totalxz > 0 || !bBankerCanleft)
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
	uint32_t chairId = pIServerUserItem->GetChairId();
	LOG(INFO) << "OnUserEnter userId:" << userId << " chairId:" << chairId;

    if(GAME_STATUS_INIT == m_pITableFrame->GetGameStatus())
    {
        m_ShenSuanZiId = pIServerUserItem->GetUserId();
        m_startTime = chrono::system_clock::now();
        m_dwReadyTime = (uint32_t)time(NULL);
        m_strRoundId = m_pITableFrame->GetNewRoundId();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_replay.gameinfoid = m_strRoundId;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
        m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
//        for(int64_t jetton : m_pGameRoomInfo->jettons)
//        {
//            m_Tian[jetton] = 0;
//            m_Di[jetton] = 0;
//            m_Xuan[jetton] = 0;
//            m_Huang[jetton] = 0;
//        }
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
    for(auto it0 = m_SortUserInfo.begin(); it0 != m_SortUserInfo.end(); it0++)
    {
        if((*it0)->userId == userId)
        {
            m_SortUserInfo.erase(it0);
            m_SortUserInfo.emplace(m_UserInfo[userId]);
            break;
        }
    }
    pIServerUserItem->SetUserStatus(sPlaying);

    for(int i=0;i<7;i++)
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


//    m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
//    m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
//    m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
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

    LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << cbGameStatus;

    switch(cbGameStatus)
    {
        case GAME_STATUS_FREE:		//空闲状态
        {
            //构造数据
          // CMD_S_GameStart pGameStartScene;
          // std::string data = pGameStartScene.SerializeAsString();
          // pGameStartScene.set_cbtimeleave(10);
          // m_pITableFrame->SendTableData(pIServerUserItem->GetChairID(), Longhu::SUB_S_GAME_START, data.c_str(), data.size());
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
    uint32_t size = 0;
    if(cbGameStatus == GAME_STATUS_START)
    {
        size = m_openRecord.size();
    }else
    {
        if(m_openRecord.size() > 0)
            size = m_openRecord.size() - 1;
    }

    CMD_S_GameRecord record;
    uint32_t TianWin = 0;
    uint32_t DiWin = 0;
    uint32_t XuanWin = 0;
    uint32_t HuangWin = 0;
    for(int i = 0; i < size; ++i)
    {
        record.add_record(m_openRecord[i]);
        if(m_openRecord[i] & 0x08) //天赢
            TianWin++;
        if(m_openRecord[i] & 0x04) //地赢
            DiWin++;
        if(m_openRecord[i] & 0x02) //玄赢
            XuanWin++;
        if(m_openRecord[i] & 0x01) //黄赢
            HuangWin++;
    }

    record.set_tiancount(TianWin);
    record.set_dicount(DiWin);
    record.set_xuancount(XuanWin);
    record.set_huangcount(HuangWin);

    std::string data = record.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, Brnn::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
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

    for(int i=0;i<7;i++)
    {
        pInfo->add_userchips(m_currentchips[chairId][i]);
    }
    for(int i = 1; i < 5; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
        alljitton->set_ljettonarea(i);
    }

    for(int i = 1; i < 5; ++i)
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
    for(int i = 1; i < 5; ++i)
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

    pInfo->set_lapplybankercondition(m_wBankerLimitScore);
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);
    for(int i = 0; i < 5; ++i)
    {
        CardGroup* card =pInfo->add_cards();
        card->set_group_id(i+1);
        for(int j = 0; j < 5;j++)
        {
           card->add_cards(m_cbCurrentRoundTableCards[i][j]);
        }
        card->set_cardtype(0);
    }

   uint32_t i = 0;
   uint32_t size = 6;
   int64_t nowScore = 0;
   int64_t xzScore = 0;
   shared_ptr<IServerUserItem> pUserItem;
//   shared_ptr<UserInfo> userInfoItem;
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
               xzScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3] + userInfoItem->m_lRobotUserJettonScore[4];
           else
               xzScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3] + userInfoItem->m_lUserJettonScore[4];

            nowScore = pUserItem->GetUserScore()- xzScore;
       }
	   if (userInfoItem->chairId == m_wBankerUser)
	   {
		   continue;
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
	   //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
	   //{
		  // palyer->set_isbanker(1);
	   //}
       palyer->set_isapplybanker(0);
       palyer->set_applybankermulti(0);
       palyer->set_ljetton(0);
       palyer->set_szlocation(userInfoItem->location);

       if(++i > size)
           break;
   }
    
	if (m_wBankerUser == INVALID_CHAIR) // 系统庄
	{
		pInfo->set_wbankeruserid(INVALID_CHAIR);
		pInfo->set_cbbankertimes(0);
		pInfo->set_lbankerscore(100000000);
		pInfo->set_benablesysbanker(true);
	} 
	else  // 玩家坐庄
	{
		pUserItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
		pInfo->set_wbankeruserid(pUserItem->GetUserId());
		pInfo->set_cbbankertimes(m_wBankerContinuityCount+1);
		pInfo->set_lbankerscore(pUserItem->GetUserScore());
		pInfo->set_benablesysbanker(false);
        pInfo->set_szbankernickname(to_string(pUserItem->GetUserId()));
		pInfo->set_cbbankerlimitcount(m_wBankerLimitCount);
	}
    
    pInfo->set_lbankerwinscore(0);
    pInfo->set_szgameroomname("Brnn");
    if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);

    pInfo->set_wincardgrouptype(0);// add_wincardgrouptype(0);
    pInfo->set_roundid(m_strRoundId);

	// 是否在申请上庄列表里面
	shared_ptr<UserInfo> userBankerInfoItem;
    for (auto &iter : m_UserBankerList)
	{
        userBankerInfoItem = iter;
		if (userBankerInfoItem->userId == pIServerUserItem->GetUserId())
		{
			pInfo->set_binapplybanker(true);
			break;
		}
	}
	/*auto it = m_UserBankerList.find(pIServerUserItem->GetUserId());
	if (m_UserBankerList.end() != it)
	{
		pInfo->set_binapplybanker(true);
	}*/

    for(auto &jetton : m_Tian)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(1);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Di)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(2);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Xuan)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(3);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Huang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(4);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }

    pInfo->set_lonlinepopulation(m_UserInfo.size());
	pInfo->set_nopenbankerapply(m_bOpenBankerApply);

    pGamePlayScene.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_startTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pGamePlayScene.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    xzScore = 0;
    bool isRobot = pIServerUserItem->IsAndroidUser();
    if(isRobot)
        xzScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2]+ userInfo->m_lRobotUserJettonScore[3]+ userInfo->m_lRobotUserJettonScore[4];
    else
        xzScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3]+ userInfo->m_lUserJettonScore[4];

    nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pGamePlayScene.set_lusermaxscore(maxscore/3);
    pGamePlayScene.set_luserscore(nowScore);

    pGamePlayScene.set_verificationcode(0);

    std::string data = pGamePlayScene.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Brnn::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
}

void CTableFrameSink::SendEndMsg(uint32_t wChairID)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_GameEnd gamend;
    GameDeskInfo* pInfo = gamend.mutable_deskdata();

    for(int i=0;i<7;i++)
    {
        pInfo->add_userchips(m_currentchips[wChairID][i]);
    }
    for(int i = 1; i < 5; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 1; i < 5; ++i)
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
    for(int i = 1; i < 5; ++i)
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
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);
    pInfo->set_roundid(m_strRoundId);
    for(int i = 0; i < 5; i++)
    {
        CardGroup* card = pInfo->add_cards();
        int nNiuType = m_allCardType[i];
        for(int j = 0; j < 5;j++)
        {
            card->add_cards(m_cbCurrentRoundTableCards[i][j]);
        }
        card->set_group_id(i+1);
        card->set_cardtype(nNiuType);
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
//            if(pUserItem->IsAndroidUser())
//                xzScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3]+ userInfoItem->m_lRobotUserJettonScore[4];
//            else
//                xzScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3]+ userInfoItem->m_lUserJettonScore[4];


             nowScore = pUserItem->GetUserScore() + userInfoItem->m_EndWinScore;
        }
		if (userInfoItem->chairId == m_wBankerUser)
		{
			continue;
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
		//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
		//{
		//	palyer->set_isbanker(1);
		//}
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(userInfoItem->location);


        PlayerScoreValue * score = gamend.add_gameendscore();
        score->set_userid(userInfoItem->userId);
        score->set_returnscore(userInfoItem->m_EndWinScore);
        if(userInfoItem->m_EndWinScore == 0 && userInfoItem->userId == 459915)
        {
            LOG(INFO) << "jeje";
        }
        score->set_userscore(nowScore);
        for(int i = 1; i < 5; ++i)
        {
            score->add_jettonareascore(userInfoItem->m_EndWinJettonScore[i]);
        }

        if(userId == userInfoItem->userId) //vip
            isVIP = true;

        if(++i > size)
            break;
    }
	if (m_wBankerUser == INVALID_CHAIR) // 系统庄
	{
		pInfo->set_wbankeruserid(INVALID_CHAIR);
		pInfo->set_cbbankertimes(0);
		pInfo->set_lbankerscore(100000000);
		pInfo->set_benablesysbanker(true);
	}
	else  // 玩家坐庄
	{
		pUserItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
		pInfo->set_wbankeruserid(pUserItem->GetUserId());
		pInfo->set_cbbankertimes(m_wBankerContinuityCount+1);
		pInfo->set_lbankerscore(pUserItem->GetUserScore());
		pInfo->set_benablesysbanker(false);
        pInfo->set_szbankernickname(to_string(pUserItem->GetUserId()));
		pInfo->set_cbbankerlimitcount(m_wBankerLimitCount);
	}
    pInfo->set_lbankerwinscore(0);
    pInfo->set_szgameroomname("Brnn");

    if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);
    /*
    else
    {
        m_openRecord.push_back(rand()%2+1);
        pInfo->add_wintag(m_openRecord[m_openRecord.size()-1]);
    }
    */

    pInfo->set_wincardgrouptype(0);

    for(auto &jetton : m_Tian)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(1);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Di)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(2);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Xuan)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(3);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Huang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(4);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }

    pInfo->set_lonlinepopulation(m_UserInfo.size());
	pInfo->set_nopenbankerapply(m_bOpenBankerApply);

    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_endTime);
    int32_t leaveTime = m_nGameEndTime - durationTime.count();
    gamend.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    nowScore = pIServerUserItem->GetUserScore() + userInfo->m_EndWinScore;
//    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    int64_t maxscore = nowScore/MAX_MULITIPLE;
    gamend.set_lusermaxscore(maxscore);
    gamend.set_luserscore(nowScore);
    gamend.set_lrevenue(userInfo->m_Revenue);
    gamend.set_verificationcode(0);

    LOG_DEBUG << "Revenue:"<<userInfo->m_Revenue;
	openLog("userid=%d wChairID=%d EndWinScore=%d,Revenue=%d", userId, wChairID, userInfo->m_EndWinScore, userInfo->m_Revenue);

    if(!isVIP)  //myself
    {
        PlayerScoreValue* score = gamend.add_gameendscore();
        score->set_userid(userId);
        score->set_returnscore(userInfo->m_EndWinScore);
        if(userInfo->m_EndWinScore == 0 && userInfo->userId == 459915)
        {
            LOG(INFO) << "jeje";
        }
        score->set_userscore(nowScore);

        for(int i = 1; i < 5; ++i)
        {
            score->add_jettonareascore(userInfo->m_EndWinJettonScore[i]);
        }
    }

    string data = gamend.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Brnn::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool bClear = false;
	bool bBankerCanleft = true;
    shared_ptr<CServerUserItem> pServerUserItem;
    pServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pServerUserItem)
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
            totalxz = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3] + userInfo->m_lUserJettonScore[4];
			
        }
		if (GAME_STATUS_START >= status)
		{
			if (INVALID_CHAIR != m_wBankerUser && chairId == m_wBankerUser)
			{
				bBankerCanleft = false;
			}
		}
		if (bBankerCanleft)
		{
			// 是否在申请上庄列表里面
			bool bInApplyList = false;
			shared_ptr<UserInfo> userBankerInfoItem;
			vector<shared_ptr<UserInfo>>::iterator iter;//声明一个迭代器，来访问vector容器，作用：遍历或者指向vector容器的元素
			for (iter = m_UserBankerList.begin();iter != m_UserBankerList.end();iter++)
			{
				userBankerInfoItem = *iter;
				if (userBankerInfoItem->userId == pServerUserItem->GetUserId())
				{
					bInApplyList = true;
					m_UserBankerList.erase(iter); // 申请上庄列表里清除
					break;
				}
			}
			if (chairId == m_wBankerUser)
			{
				m_wBankerUser = INVALID_CHAIR;
			}
			if (bInApplyList) //刷新申请上庄列表
			{
				OnApplyBankerList();
			}
		}				
		
        pServerUserItem->setOffline();
        if(totalxz == 0 && bBankerCanleft)
        {
            {
//                WRITE_LOCK(m_mutex);
                m_UserInfo.erase(userId);
            }
            m_pITableFrame->ClearTableUser(chairId);
            bClear = true;
        }
        else
        {
            pServerUserItem->SetTrustship(true);
        }


//        m_cbPlayStatus[chairId] = false;
    }

    if(userId == m_ShenSuanZiId)
    {
        m_ShenSuanZiId = 0;
    }

    return bClear;
}

bool CTableFrameSink::isInVipList(uint32_t chairId)
{
    uint32_t listNum = 0;
    for(auto userInfo : m_SortUserInfo)
    {
        if(userInfo->chairId == chairId)
        {
            return true;
        }
       if(++listNum >= 8)
            break;
    }
    return false;
}

bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    return true;
}

void CTableFrameSink::ReadStorageInfo()
{
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;
    m_llGap = (m_llStorageHighLimit-m_llStorageLowerLimit) >> 1;
    m_llStorageAverLine = (m_llStorageHighLimit+m_llStorageLowerLimit) >> 1;
    if(m_llGap < 0)
    {
        LOG(ERROR) << "Error Limit...";
    }else{
        LOG(INFO) << "High Limit" << m_llStorageHighLimit << ",Lower Limit:" << m_llStorageLowerLimit << ",Aver Line"<<m_llStorageAverLine
                  << ", Gap" << m_llGap << ", Stock:" << m_llStorageControl;
    }

    // 当前系统库存
    LOG_ERROR << "StockScore:" << m_llStorageControl <<",StockHighLimit:"<<m_llStorageHighLimit<<",StockLowLimit:"<<m_llStorageLowerLimit;
}
//current banker win
int64_t CTableFrameSink::CurrentWin()
{
//    int64_t iTianWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1],m_cbCurrentRoundTableCards[0],5)*m_lTrueUserJettonScore[1];
//    int64_t iDiWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2],m_cbCurrentRoundTableCards[0],5)*m_lTrueUserJettonScore[2];
//    int64_t iXuanWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3],m_cbCurrentRoundTableCards[0],5)*m_lTrueUserJettonScore[3];
//    int64_t iHuangWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4],m_cbCurrentRoundTableCards[0],5)*m_lTrueUserJettonScore[4];
//    return -(iTianWin+iDiWin+iHuangWin+iXuanWin);
      return 0;
}
void  CTableFrameSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//brnn//brnn_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId);
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
     return;
    }
   va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] [TABLEID:%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,m_pITableFrame->GetTableId());
   vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);


}
//根据系统输赢发牌
void CTableFrameSink::GetOpenCards()
{

    memset(m_allCardType, 0, sizeof(m_allCardType));
    memset(m_allMultical, 0, sizeof(m_allMultical));
    memset(m_cbCurrentRoundTableCards,0,sizeof(m_cbCurrentRoundTableCards));
    int64_t shijian=time(NULL);
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);
    int  isHaveBlackPlayer=0;    
    if(m_pGameRoomInfo->roomId == 9301)
    {
       brnnAlgorithm.SetTenMutical(false);
    }
    else
    {
       brnnAlgorithm.SetTenMutical(true);
    }
	shared_ptr<UserInfo> userInfoItem;
    //for(int i=0;i<GAME_PLAYER;i++)
	for (auto &it : m_UserInfo) 
	{
		userInfoItem = it.second;
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]
        +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[3]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[4];
		
		if (m_wBankerUser == user->GetChairId())
			allJetton = m_lRobotserJettonAreaScore[1] + m_lRobotserJettonAreaScore[2] + m_lRobotserJettonAreaScore[3] + m_lRobotserJettonAreaScore[4];
		
		if (allJetton == 0) continue;

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
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        brnnAlgorithm.SetMustKill(-1);
		brnnAlgorithm.SetAlgorithemNum(storageInfo.lUplimit, storageInfo.lLowlimit, storageInfo.lEndStorage, m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
		if (m_wBankerUser == INVALID_CHAIR) // 系统庄
		{
			for (int i = 0;i < 4;i++)
				brnnAlgorithm.SetBetting(i, m_lTrueUserJettonScore[i + 1]);//设置每门真人押注值
			brnnAlgorithm.GetOpenCard(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical);
		} 
		else // 玩家坐庄
		{
			for (int i = 0;i < 4;i++)
				brnnAlgorithm.SetBetting(i, m_lRobotserJettonAreaScore[i + 1]);//设置每门AI押注值
			brnnAlgorithm.GetOpenCard_UserBancker(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical);
		}
    }
    else
    {
        if(isHaveBlackPlayer)
        {
            float probilityAll[4]={0.0f};
            //user probilty count
			for (auto &it : m_UserInfo)
			{
				userInfoItem = it.second;
				shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!user||user->IsAndroidUser())
                {
                    continue;
                }
                int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]
                +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[3]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[4];

				if (m_wBankerUser == INVALID_CHAIR) // 系统庄
				{
					if (allJetton == 0) continue;

					if (user->GetBlacklistInfo().status == 1)
					{
						int shuzhi = 0;
						auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
						if (it != user->GetBlacklistInfo().listRoom.end())
						{
							shuzhi = it->second;
						}
						else
						{
							shuzhi = 0;
						}
						float gailv = (float)shuzhi / 1000;
						for (int j = 0;j < 4;j++)
						{
							if (m_lTrueUserJettonScore[j + 1] > 0)
							{
								probilityAll[j] += (gailv*(float)m_UserInfo[user->GetUserId()]->m_lUserJettonScore[j + 1]) / m_lTrueUserJettonScore[j + 1];
							}

						}
					}
				}
				else // 玩家上庄
				{
					if (m_wBankerUser == user->GetChairId())
						allJetton = m_lRobotserJettonAreaScore[1] + m_lRobotserJettonAreaScore[2] + m_lRobotserJettonAreaScore[3] + m_lRobotserJettonAreaScore[4];

					if (allJetton == 0) continue;

					if (user->GetBlacklistInfo().status == 1)
					{
						int shuzhi = 0;
						auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
						if (it != user->GetBlacklistInfo().listRoom.end())
						{
							shuzhi = it->second;
						}
						else
						{
							shuzhi = 0;
						}
						float gailv = (float)shuzhi / 1000;
						for (int j = 0;j < 4;j++)
						{
							if (m_wBankerUser != user->GetChairId())
							{
								if (m_lTrueUserJettonScore[j + 1] > 0)
								{
									probilityAll[j] += (gailv*(float)m_UserInfo[user->GetUserId()]->m_lUserJettonScore[j + 1]) / m_lAllJettonScore[j + 1];
								}
							}
							else
							{
								if (m_lRobotserJettonAreaScore[j + 1] > 0)
								{
									probilityAll[j] -= (gailv*(float)m_lRobotserJettonAreaScore[j + 1]) / m_lAllJettonScore[j + 1];
								}
							}
						}
					}
				}
            }



            brnnAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
			if (m_wBankerUser == INVALID_CHAIR) // 系统庄
			{
				for (int i = 0;i < 4;i++)
					brnnAlgorithm.SetBetting(i, m_lTrueUserJettonScore[i + 1]);//设置每门真人押注值
				brnnAlgorithm.BlackGetOpenCard(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical, probilityAll);
			}
			else // 玩家坐庄
			{
				for (int i = 0;i < 4;i++)
					brnnAlgorithm.SetBetting(i, m_lRobotserJettonAreaScore[i + 1]);//设置每门AI押注值
				brnnAlgorithm.BlackGetOpenCard_UserBancker(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical, probilityAll);
			}
        }
        else
        {

            brnnAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
			if (m_wBankerUser == INVALID_CHAIR) // 系统庄
			{
				for (int i = 0;i < 4;i++)
					brnnAlgorithm.SetBetting(i, m_lTrueUserJettonScore[i + 1]);//设置每门真人押注值
				brnnAlgorithm.GetOpenCard(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical);
			}
			else // 玩家坐庄
			{
				for (int i = 0;i < 4;i++)
					brnnAlgorithm.SetBetting(i, m_lRobotserJettonAreaScore[i + 1]);//设置每门AI押注值
				brnnAlgorithm.GetOpenCard_UserBancker(m_cbCurrentRoundTableCards, m_allCardType, m_allMultical);
			}
        }

    }




    double res=brnnAlgorithm.CurrentProfit();
    if(res>0)
    {
        res = res * (1000.0 - m_pITableFrame->GetGameRoomInfo()->systemReduceRatio) / 1000.0;
        //res = res * (100.0 - stockWeak) / 100.0;
    }
    m_pITableFrame->UpdateStorageScore(res);//结果出来了就可以刷新库存了,当然是不算税的
    m_pITableFrame->GetStorageScore(storageInfo);
    if(m_allMultical[0]<0)
        openLog("本次天门输");
    else
        openLog("本次天门赢");
    if(m_allMultical[1]<0)
        openLog("本次地门输");
    else
        openLog("本次地门赢");
    if(m_allMultical[2]<0)
        openLog("本次玄门输");
    else
        openLog("本次玄门赢");
    if(m_allMultical[3]<0)
        openLog("本次黄门输");
    else
        openLog("本次黄门赢");
    openLog("系统库存上限------%d",storageInfo.lUplimit);
    openLog("系统库存下限------%d",storageInfo.lLowlimit);
    openLog("系统当前库存------%d",storageInfo.lEndStorage);
    openLog("本次系统赢输------%d",brnnAlgorithm.CurrentProfit());
    shijian=time(NULL)-shijian;
    openLog("调用算法总共耗时------%d",shijian);
    if(m_listTestCardArray.size() > 0)
    {
        for(int i = 0; i < 5; ++i)
        {
            for(int j = 0; j < 5; ++j)
            {
                m_cbCurrentRoundTableCards[i][j] = *(m_listTestCardArray.begin());
                m_listTestCardArray.erase(m_listTestCardArray.begin());
            }
        }
    }


}

void CTableFrameSink::SafeSwitch()
{
    LOG(WARNING) << "Trigger Safety Switch";
    if(m_llStorageControl > m_llStorageHighLimit)//over hight limit
    {
        MakeUserWin(m_winRates[1]+1);
    }else if(m_llStorageControl < m_llStorageLowerLimit)//over low limit
    {
        MakeUserLose();
    }else//normal
    {
        if(GetRandData()  >= 550)
        {
            MakeUserWin(m_winRates[1]+1);
        }else{
            MakeUserLose(m_winRates[1]+1);
        }

    }
}

void CTableFrameSink::MakeUserWin(int32_t weight)
{
    LOG(INFO) << "Win";
    //todo control not trigger safe switch
    if(weight == 0)
    {
       weight = GetRandData();
    }
    uint8_t byCardBuffer[4];
    if( weight < m_lostRates[0] )//all win
    {
        byCardBuffer[0] = 0;
        byCardBuffer[1] = 1;
        byCardBuffer[2] = 2;
        byCardBuffer[3] = 3;
        AllocationCard(byCardBuffer,4,5);
    }else if(weight < m_lostRates[1] )//3 win
    {
        byCardBuffer[0] = 0;
        byCardBuffer[1] = 1;
        byCardBuffer[2] = 2;
        byCardBuffer[3] = 4;
        AllocationCard(byCardBuffer,4,4);
    }else//2 win
    {
        byCardBuffer[0] = 0;
        byCardBuffer[1] = 1;
        byCardBuffer[2] = 3;
        byCardBuffer[3] = 4;
        AllocationCard(byCardBuffer,4,3);
    }
}

void CTableFrameSink::MakeUserLose(int32_t weight )
{
    LOG(INFO) << "Lose";
    if(weight == 0)
    {
       weight = GetRandData();
    }
    uint8_t byCardBuffer[4];
    // make sure the most betted one the last
    if( weight < m_winRates[0])//win all
    {
        byCardBuffer[0] = 1;
        byCardBuffer[1] = 2;
        byCardBuffer[2] = 3;
        byCardBuffer[3] = 4;
        AllocationCard(byCardBuffer,4,1);
    }else if(weight < m_winRates[1])//win 3
    {
        memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));
        //庄家第二概率
        for(int j = 0; j < 5; ++j)
        {
            m_cbCurrentRoundTableCards[0][j] = m_cbTempTableCards[1][j];
        }
        int i = 1;
        for(i = 1; i < 5; ++i)
        {
            if(m_lTempSortBet[4] == m_lTrueUserJettonScore[i])
            {
                break;
            }
        }
        //押注最少的玩家赢庄家
        for(int j = 0; j < 5; ++j)
        {
            m_cbCurrentRoundTableCards[i][j] = m_cbTempTableCards[0][j];
        }
        //随机分配其他三个区域
        uint8_t cbIndex = 0;
        int iIndex = 0;
        uint8_t byCardBufferBig[3] = {2,3,4};
        random_shuffle(byCardBufferBig, byCardBufferBig+3);
        for(int i = 1; i < 5; ++i)
        {
            if(m_cbCurrentRoundTableCards[i][0] == 0)
            {
                cbIndex = byCardBufferBig[iIndex++];
                for(int j = 0; j < 5; ++j)
                {
                    m_cbCurrentRoundTableCards[i][j] = m_cbTempTableCards[cbIndex][j];
                }
            }

        }
    }else//win 2
    {
        memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));
        //庄家第三概率
        for(int j = 0; j < 5; ++j)
        {
            m_cbCurrentRoundTableCards[0][j] = m_cbTempTableCards[2][j];
        }
        uint8_t byCardBuffer[2] = {3,4};
        std::vector<int> vecAreaVal;
        random_shuffle(byCardBuffer, byCardBuffer+2);
        //确定小于庄家的两个区域(就是下注最多的两个区域)
        int i = 1;
        int cnt = 0;
        for(i = 1; i < 5; ++i)
        {
            if(m_lTempSortBet[1] == m_lTrueUserJettonScore[i] || m_lTempSortBet[2] == m_lTrueUserJettonScore[i])
            {
                vecAreaVal.push_back(i);
                cnt++;
                if(cnt == 2) break;
            }
        }
        std::vector<int>::iterator  it = vecAreaVal.begin();
        i = 0;
        for (; it != vecAreaVal.end(); ++it)
        {
            uint8_t cbIndex  = byCardBuffer[i++];
            for(int j = 0; j < 5; ++j)
            {
                m_cbCurrentRoundTableCards[*it][j] = m_cbTempTableCards[cbIndex][j];
            }
        }
        //分配大于庄家的两个区域
        uint8_t byCardBufferBig[2] = {0,1};
        random_shuffle(byCardBufferBig, byCardBufferBig+2);
        i = 1;
        int iIndex = 0;
        while(i < 5)
        {
            if(m_cbCurrentRoundTableCards[i][0] == 0)
            {
                uint8_t cbIndex = byCardBufferBig[iIndex++];
                for(int j = 0; j < 5; ++j)
                {
                    m_cbCurrentRoundTableCards[i][j] = m_cbTempTableCards[cbIndex][j];
                }
            }
            i++;
        }
    }
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;

    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        bool bOpenLogFlag = false;
        GetOpenCards();     //open cards
        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        int32_t iTianTimes = 0;    //天的输赢标识
        int32_t iDiTimes = 0;      //地的输赢标识
        int32_t iXuanTimes = 0;    //玄的输赢标识
        int32_t iHuangTimes = 0;   //黄的输赢标识
        int64_t res = 0.0;
        iTianTimes = m_allMultical[0];
        iDiTimes = m_allMultical[1];
        iXuanTimes = m_allMultical[2];
        iHuangTimes = m_allMultical[3];

        int8_t tianCardType = m_allCardType[1];
        int8_t diCardType = m_allCardType[2];
        int8_t xuanCardType = m_allCardType[3];
        int8_t huangCardType = m_allCardType[4];

        stringstream ss;
        ss << "1:" << GlobalFunc::converToHex(m_cbCurrentRoundTableCards[1],5) << ":" << to_string(tianCardType) << "|";
        ss << "2:" << GlobalFunc::converToHex(m_cbCurrentRoundTableCards[2],5) << ":" << to_string(diCardType) << "|";
        ss << "3:" << GlobalFunc::converToHex(m_cbCurrentRoundTableCards[3],5) << ":" << to_string(xuanCardType) << "|";
        ss << "4:" << GlobalFunc::converToHex(m_cbCurrentRoundTableCards[4],5) << ":" << to_string(huangCardType) << "|";
        ss << "0:" << GlobalFunc::converToHex(m_cbCurrentRoundTableCards[0],5) << ":" << to_string(m_allCardType[0]);
        uint32_t nowSecs = (uint32_t)time(NULL) - m_dwReadyTime;
        m_replay.addStep(nowSecs,ss.str(),0,opShowCard,-1,-1);

		int64_t allBetScore = 0;
		int64_t userid = 0;
		m_replay.detailsData = "";
		int64_t bankerRevenue = 0;
		int64_t bankerEndWinScore = 0;
		int64_t bankerEndWinJettonScore[5] = { 0 };
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem)
                continue;

            if(pUserItem->IsAndroidUser()) //机器人的下注
            {
                if(userInfoItem->m_lRobotUserJettonScore[1] > 0) //在天区域的下注
                {
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iTianTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[1] * iTianTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[1] = (userInfoItem->m_lRobotUserJettonScore[1]*iTianTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lRobotUserJettonScore[1] * iTianTimes;
						bankerEndWinJettonScore[1] -= userInfoItem->m_lRobotUserJettonScore[1] * iTianTimes;
                    }
                    else
                    {
                        //输了没税收
//                        userInfoItem->m_Revenue = 0.0;
                        userInfoItem->m_EndWinJettonScore[1] = userInfoItem->m_lRobotUserJettonScore[1]*iTianTimes;
						
						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[1]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[1] - bRevenue);
						bankerEndWinJettonScore[1] += (-userInfoItem->m_EndWinJettonScore[1] - bRevenue);
                    }

                    userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[1]*iTianTimes - Revenue);

                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                }

                if(userInfoItem->m_lRobotUserJettonScore[2] > 0) //在地区域的下注
                {
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iDiTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[2] * iDiTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[2] = (userInfoItem->m_lRobotUserJettonScore[2]*iDiTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lRobotUserJettonScore[2] * iDiTimes;
						bankerEndWinJettonScore[2] -= userInfoItem->m_lRobotUserJettonScore[2] * iDiTimes;
                    }
                    else
                    {
                        //输了没税收
//                        userInfoItem->m_Revenue = 0.0;
                        userInfoItem->m_EndWinJettonScore[2] = (userInfoItem->m_lRobotUserJettonScore[2]*iDiTimes);

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[2]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[2] - bRevenue);
						bankerEndWinJettonScore[2] += (-userInfoItem->m_EndWinJettonScore[2] - bRevenue);
                    }
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[2]*iDiTimes - Revenue);

                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                }

                if(userInfoItem->m_lRobotUserJettonScore[3] > 0) //在玄区域的下注
                {
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iXuanTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[3] * iXuanTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[3] = (userInfoItem->m_lRobotUserJettonScore[3]*iXuanTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lRobotUserJettonScore[3] * iXuanTimes;
						bankerEndWinJettonScore[3] -= userInfoItem->m_lRobotUserJettonScore[3] * iXuanTimes;
                    }
                    else
                    {
                        //输了没税收
//                        userInfoItem->m_Revenue = 0.0;
                        userInfoItem->m_EndWinJettonScore[3] = userInfoItem->m_lRobotUserJettonScore[3]*iXuanTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[3]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[3] - bRevenue);
						bankerEndWinJettonScore[3] += (-userInfoItem->m_EndWinJettonScore[3] - bRevenue);
                    }
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[3]*iXuanTimes - Revenue);

                }

                if(userInfoItem->m_lRobotUserJettonScore[4] > 0) //在黄区域的下注
                {
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iHuangTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[4] * iHuangTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[4] = (userInfoItem->m_lRobotUserJettonScore[4]*iHuangTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lRobotUserJettonScore[4] * iHuangTimes;
						bankerEndWinJettonScore[4] -= userInfoItem->m_lRobotUserJettonScore[4] * iHuangTimes;
                    }
                    else
                    {
                        //输了没税收
//                        userInfoItem->m_Revenue = 0.0;
                        userInfoItem->m_EndWinJettonScore[4] = userInfoItem->m_lRobotUserJettonScore[4]*iHuangTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[4]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[4] - bRevenue);
						bankerEndWinJettonScore[4] += (-userInfoItem->m_EndWinJettonScore[4] - bRevenue);
                    }
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[4]*iHuangTimes - Revenue);

                }

                LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
            }
            else
            {
				allBetScore = 0;
				userid = pUserItem->GetUserId();
                if(userInfoItem->m_lUserJettonScore[1]>0) //在天区域的下注
                {
                    bOpenLogFlag = true;
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iTianTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[1] * iTianTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[1] = (userInfoItem->m_lUserJettonScore[1] * iTianTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lUserJettonScore[1] * iTianTimes;
						bankerEndWinJettonScore[1] -= userInfoItem->m_lUserJettonScore[1] * iTianTimes;
                    }
                    else
                    {
//                        userInfoItem->m_Revenue = 0;
                        userInfoItem->m_EndWinJettonScore[1] = userInfoItem->m_lUserJettonScore[1] * iTianTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[1]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[1] - bRevenue);
						bankerEndWinJettonScore[1] += (-userInfoItem->m_EndWinJettonScore[1] - bRevenue);
                    }

                    m_lStockScore -= userInfoItem->m_lUserJettonScore[1] * iTianTimes;
                    res -= userInfoItem->m_lUserJettonScore[1] * iTianTimes;
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[1] * iTianTimes - Revenue);
                    m_replay.addResult(userInfoItem->chairId,1,userInfoItem->m_lUserJettonScore[1],userInfoItem->m_EndWinJettonScore[1],to_string(tianCardType),false);
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
					allBetScore += userInfoItem->m_lUserJettonScore[1];
                }

                if(userInfoItem->m_lUserJettonScore[2]>0) //在地区域的下注
                {
                    bOpenLogFlag = true;
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iDiTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[2] * iDiTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[2] += (userInfoItem->m_lUserJettonScore[2] * iDiTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lUserJettonScore[2] * iDiTimes;
						bankerEndWinJettonScore[2] -= userInfoItem->m_lUserJettonScore[2] * iDiTimes;
                    }
                    else
                    {
//                        userInfoItem->m_Revenue = 0;
                        userInfoItem->m_EndWinJettonScore[2] += userInfoItem->m_lUserJettonScore[2] * iDiTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[2]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[2] - bRevenue);
						bankerEndWinJettonScore[2] += (-userInfoItem->m_EndWinJettonScore[2] - bRevenue);
                    }
                    m_lStockScore -= userInfoItem->m_lUserJettonScore[2] * iDiTimes;
                    res -= userInfoItem->m_lUserJettonScore[2] * iDiTimes;
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[2] * iDiTimes - Revenue);
                    m_replay.addResult(userInfoItem->chairId,2,userInfoItem->m_lUserJettonScore[2],userInfoItem->m_EndWinJettonScore[2],to_string(diCardType),false);
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
					allBetScore += userInfoItem->m_lUserJettonScore[2];
                }

                if(userInfoItem->m_lUserJettonScore[3]>0) //在玄区域的下注
                {
                    bOpenLogFlag = true;
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iXuanTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[3] * iXuanTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[3] = (userInfoItem->m_lUserJettonScore[3] * iXuanTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lUserJettonScore[3] * iXuanTimes;
						bankerEndWinJettonScore[3] -= userInfoItem->m_lUserJettonScore[3] * iXuanTimes;
                    }
                    else
                    {
//                        userInfoItem->m_Revenue = 0;
                        userInfoItem->m_EndWinJettonScore[3] = userInfoItem->m_lUserJettonScore[3] * iXuanTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[3]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[3] - bRevenue);
						bankerEndWinJettonScore[3] += (-userInfoItem->m_EndWinJettonScore[3] - bRevenue);
                    }
                    m_lStockScore -= userInfoItem->m_lUserJettonScore[3] * iXuanTimes;
                    res -= userInfoItem->m_lUserJettonScore[3] * iXuanTimes;
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[3] * iXuanTimes - Revenue);
                    m_replay.addResult(userInfoItem->chairId,3,userInfoItem->m_lUserJettonScore[3],userInfoItem->m_EndWinJettonScore[3],to_string(xuanCardType),false);
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
					allBetScore += userInfoItem->m_lUserJettonScore[3];
                }

                if(userInfoItem->m_lUserJettonScore[4]>0) //在黄区域的下注
                {
                    bOpenLogFlag = true;
                    int64_t Revenue = 0;
					int64_t bRevenue = 0;
                    if(iHuangTimes > 0)
                    {
                        //赢了有税收
                        Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[4] * iHuangTimes);
                        userInfoItem->m_Revenue += Revenue;
                        userInfoItem->m_EndWinJettonScore[4] = (userInfoItem->m_lUserJettonScore[4] * iHuangTimes - Revenue);

						bankerEndWinScore -= userInfoItem->m_lUserJettonScore[4] * iHuangTimes;
						bankerEndWinJettonScore[4] -= userInfoItem->m_lUserJettonScore[4] * iHuangTimes;
                    }
                    else
                    {
//                        userInfoItem->m_Revenue = 0;
                        userInfoItem->m_EndWinJettonScore[4] = userInfoItem->m_lUserJettonScore[4] * iHuangTimes;

						bRevenue = m_pITableFrame->CalculateRevenue(-userInfoItem->m_EndWinJettonScore[4]);
						bankerRevenue += bRevenue;
						bankerEndWinScore += (-userInfoItem->m_EndWinJettonScore[4] - bRevenue);
						bankerEndWinJettonScore[4] += (-userInfoItem->m_EndWinJettonScore[4] - bRevenue);
                    }
                    m_lStockScore -= userInfoItem->m_lUserJettonScore[4] * iHuangTimes;
                    res -= userInfoItem->m_lUserJettonScore[4] * iHuangTimes;
                    userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[4] * iHuangTimes - Revenue);
                    m_replay.addResult(userInfoItem->chairId,4,userInfoItem->m_lUserJettonScore[4],userInfoItem->m_EndWinJettonScore[4],to_string(huangCardType),false);
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
					allBetScore += userInfoItem->m_lUserJettonScore[4];
                }
                LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
				if (allBetScore > 0)
				{
					//对局记录详情
					SetGameRecordDetail(userid, userInfoItem->chairId, pUserItem->GetUserScore(), userInfoItem);
				}
            }
        }
		if (m_wBankerUser != INVALID_CHAIR) // 玩家坐庄
		{			
			shared_ptr<IServerUserItem> bankerInfoItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
			userInfoItem = m_UserInfo[bankerInfoItem->GetUserId()];
			m_replay.addPlayer(userInfoItem->userId, bankerInfoItem->GetAccount(), bankerInfoItem->GetUserScore(), m_wBankerUser);

			userInfoItem->m_Revenue = bankerRevenue;
			userInfoItem->m_EndWinScore = bankerEndWinScore;
			for (int i = 1;i < 5;i++)
			{
				m_lBankerAllEndScore[i] = bankerEndWinJettonScore[i];
				m_replay.addResult(m_wBankerUser, i, m_lRobotserJettonAreaScore[i], m_lBankerAllEndScore[i], to_string(m_allCardType[i]), true);
			}
			
			openLog("玩家坐庄:m_wBankerUser=%d,bankerRevenue=%d,bankerEndWinScore=%d", m_wBankerUser,userInfoItem->m_Revenue, userInfoItem->m_EndWinScore);
			SetGameRecordDetail(bankerInfoItem->GetUserId(), m_wBankerUser, bankerInfoItem->GetUserScore(), userInfoItem);
		}
        ChangeUserInfo();

        m_pITableFrame->SaveReplay(m_replay);

        int result = WIN_LOSE_MASK_VALUE;
        if(iTianTimes < 0) //天
            result = result&LOSE_MASK_TIAN;
        if(iDiTimes < 0) //地
            result = result&LOSE_MASK_DI;
        if(iXuanTimes < 0) //玄
            result = result&LOSE_MASK_XUAN;
        if(iHuangTimes < 0) //黄
            result = result&LOSE_MASK_HUANG;
        m_openRecord.push_back(result);

        LOG(INFO) << "OnEventGameConclude chairId =  " << chairId;
        return true;
    }
    return false;
}

void CTableFrameSink::clearTableUser()
{
    int64_t min_score = MIN_PLAYING_ANDROID_USER_SCORE;
    if(m_pGameRoomInfo)
        min_score = std::max(m_pGameRoomInfo->enterMinScore, min_score);

    vector<uint64_t> clearUserIdVec;
    vector<uint32_t> clearChairIdVec;
    {
//        WRITE_LOCK(m_mutex);

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(pUserItem)
            {
                if( m_pGameRoomInfo->serverStatus !=SERVER_RUNNING || (pUserItem->GetUserStatus() == sOffline || (pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < min_score)))
                {
                    pUserItem->setOffline();
                    clearUserIdVec.push_back(userInfoItem->userId);
                    clearChairIdVec.push_back(userInfoItem->chairId);
//                    userInfoIter = m_UserInfo.erase(userInfoIter);
//                    continue;
                }
				//else if(pUserItem->GetUserStatus() == sOffline)
				//{
				//	if (m_wBankerUser == INVALID_CHAIR) //系统庄
				//	{
				//		pUserItem->setOffline();
				//		clearUserIdVec.push_back(userInfoItem->userId);
				//		clearChairIdVec.push_back(userInfoItem->chairId);
				//	} 
				//	else if (userInfoItem->chairId != m_wBankerUser) //玩家庄,非庄家踢出
				//	{
				//		pUserItem->setOffline();
				//		clearUserIdVec.push_back(userInfoItem->userId);
				//		clearChairIdVec.push_back(userInfoItem->chairId);
				//	}
				//	else if (m_wBankerContinuityCount >= m_wBankerLimitCount) // 玩家坐庄，局数够了
				//	{
				//		pUserItem->setOffline();
				//		clearUserIdVec.push_back(userInfoItem->userId);
				//		clearChairIdVec.push_back(userInfoItem->chairId);
				//	}
				//}
            }
        }
    }

    uint32_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_pITableFrame->ClearTableUser(clearChairIdVec[i]);
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
	int64_t totleBankerWinScore = 0;
	int64_t totleBankerRevenue = 0;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(pUserItem)
        {
//            memset(&scoreInfo, 0, sizeof(tagScoreInfo));			
            scoreInfo.clear();
            if(pUserItem->IsAndroidUser())
            {
                totalBetScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2]
                        + userInfoItem->m_lRobotUserJettonScore[3]+ userInfoItem->m_lRobotUserJettonScore[4];
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[1]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[2]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[3]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[4]);
            }
            else
            {
                totalBetScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2]
                        + userInfoItem->m_lUserJettonScore[3]+ userInfoItem->m_lUserJettonScore[4];
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[1]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[2]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[3]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[4]);				
            }
            if(totalBetScore > 0)
            {
                scoreInfo.chairId = userInfoItem->chairId;
                scoreInfo.betScore = totalBetScore;
                scoreInfo.rWinScore = totalBetScore;
                scoreInfo.addScore = userInfoItem->m_EndWinScore;
                scoreInfo.revenue = userInfoItem->m_Revenue;
//                string cardValue = GlobalFunc::converToHex((uint8_t*)m_cbCurrentRoundTableCards, 25);
                string cardValue = "";
                for(int i = 1; i < 5; ++i)
                {
                    cardValue += std::to_string(i);
                    cardValue += GlobalFunc::converToHex((uint8_t*)m_cbCurrentRoundTableCards[i], 5);
                }
                cardValue += std::to_string(5);
                cardValue += GlobalFunc::converToHex((uint8_t*)m_cbCurrentRoundTableCards[0], 5);

                uint32_t size = m_openRecord.size();
                if(size > 0)
                {
                    uint8_t uValue[2];
                    if(m_openRecord[size - 1] & 0x08) //天赢
                    {
                        uValue[0] = 1;
                        cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
                    }

                    if(m_openRecord[size - 1] & 0x04) //地赢
                    {
                        uValue[0] = 2;
                        cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
                    }

                    if(m_openRecord[size - 1] & 0x02) //玄赢
                    {
                        uValue[0] = 3;
                        cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
                    }

                    if(m_openRecord[size - 1] & 0x01) //黄赢
                    {
                        uValue[0] = 4;
                        cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
                    }
                }

                scoreInfo.cardValue = cardValue;
                scoreInfo.startTime = m_startTime;
				scoreInfo.isBanker = 0;
                scoreInfoVec.push_back(scoreInfo);
				if (m_wBankerUser != INVALID_CHAIR)
				{
					totleBankerWinScore += (-scoreInfo.addScore);
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
        }
    }
	if (m_wBankerUser != INVALID_CHAIR)
	{
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			if (m_wBankerUser == userInfoItem->chairId) // 玩家是庄家
			{
				pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
				scoreInfo.cellScore.push_back(m_lRobotserJettonAreaScore[1]);
				scoreInfo.cellScore.push_back(m_lRobotserJettonAreaScore[2]);
				scoreInfo.cellScore.push_back(m_lRobotserJettonAreaScore[3]);
				scoreInfo.cellScore.push_back(m_lRobotserJettonAreaScore[4]);
				totalBetScore = m_lRobotserJettonAreaScore[1] + m_lRobotserJettonAreaScore[2] + m_lRobotserJettonAreaScore[3] + m_lRobotserJettonAreaScore[4];
				if (pUserItem)
				{
					scoreInfo.chairId = userInfoItem->chairId;
					scoreInfo.betScore = totalBetScore;
					scoreInfo.rWinScore = totalBetScore;
					scoreInfo.addScore = userInfoItem->m_EndWinScore;
					scoreInfo.revenue = userInfoItem->m_Revenue;
					scoreInfo.isBanker = 1;
					
					string cardValue = "";
					for (int i = 1; i < 5; ++i)
					{
						cardValue += std::to_string(i);
						cardValue += GlobalFunc::converToHex((uint8_t*)m_cbCurrentRoundTableCards[i], 5);
					}
					cardValue += std::to_string(5);
					cardValue += GlobalFunc::converToHex((uint8_t*)m_cbCurrentRoundTableCards[0], 5);
					uint32_t size = m_openRecord.size();
					if (size > 0)
					{
						uint8_t uValue[2];
						if (m_openRecord[size - 1] & 0x08) //天赢
						{
							uValue[0] = 1;
							cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
						}
						if (m_openRecord[size - 1] & 0x04) //地赢
						{
							uValue[0] = 2;
							cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
						}
						if (m_openRecord[size - 1] & 0x02) //玄赢
						{
							uValue[0] = 3;
							cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
						}
						if (m_openRecord[size - 1] & 0x01) //黄赢
						{
							uValue[0] = 4;
							cardValue += GlobalFunc::converToHex((uint8_t*)uValue, 1);
						}
					}
					scoreInfo.cardValue = cardValue;
					scoreInfo.startTime = m_startTime;
					scoreInfoVec.push_back(scoreInfo);
				}
				break;
			}
		}
	}
    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    int64_t totalJetton = 0;

    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        //添加下注信息
        if(pUserItem->IsAndroidUser())
        {
            totalJetton = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3]+ userInfoItem->m_lRobotUserJettonScore[4];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }else
        {
            totalJetton = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3]+ userInfoItem->m_lUserJettonScore[4];
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
    bool bFirst = true;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
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
//    shared_ptr<IServerUserItem> pUserItem;
//    shared_ptr<UserInfo> userInfoItem;
    m_ShenSuanZiId = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        if(m_ShenSuanZiId == 0)
            m_ShenSuanZiId = userInfoItem->userId;
        else if(userInfoItem->lTwentyWinCount > m_UserInfo[m_ShenSuanZiId]->lTwentyWinCount)
            m_ShenSuanZiId = userInfoItem->userId;
    }
}

//定时器事件
void CTableFrameSink::OnTimerJetton()
{
    m_endTime = chrono::system_clock::now();
//    if(--m_nPlaceJettonLeveTime < 1)
    {
//        m_TimerLoopThread->getLoop()->cancel(m_TimerIdJetton);
        m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);
        //结束游戏
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

//        m_nGameEndLeveTime = m_nGameEndTime;

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
        m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(m_nGameEndTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));

        WriteUserScore();
//        ChangeUserInfo();

        SendWinSysMessage();

        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_END);
        m_replay.clear();

        ChangeVipUserInfo();
        ReadConfigInformation();
//        ClearTableData();
        LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();

//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
    }
}

void CTableFrameSink::OnTimerEnd()
{
    //连续5局未下注踢出房间
    //CheckKickOutUser();
    m_strRoundId = m_pITableFrame->GetNewRoundId();
	if (m_wBankerUser != INVALID_CHAIR)
	{
		m_wBankerContinuityCount++;
	}
    //删除玩家信息
    clearTableUser();  //sOffline  m_cbPlayStatus==false
	OnGameChangeBanker();
    m_pITableFrame->SetGameStatus(GAME_STATUS_START);
    m_startTime = chrono::system_clock::now();
    m_dwReadyTime = (uint32_t)time(NULL);
    m_replay.gameinfoid = m_strRoundId;
    m_replay.roomname = m_pGameRoomInfo->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        ClearTableData();
        bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
        if(!IsContinue)
            return;
        LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();

        bool bRestart = false;
#if 0

#endif
         if(m_openRecord.size() < 20)
         {

         }
         else
         {
             m_CardVecData.clear();
             m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
             random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

             m_openRecord.erase(m_openRecord.begin());
             bRestart = true;
         }


         shared_ptr<IServerUserItem> pUserItem;
         shared_ptr<UserInfo> userInfoItem;
         for(int i=0;i<GAME_PLAYER;i++)
         {
             shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
             if(!user||!user->IsAndroidUser()||user->GetUserScore()>10000) continue;
             LOG_ERROR<<"所有机器人的分数==========="<<user->GetUserScore();
         }
         for(auto &it : m_UserInfo)
         {
             if (!it.second)
                 continue;
             userInfoItem = it.second;
             pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
             if(!pUserItem)
                 continue;
             if(bRestart)
                SendOpenRecord(userInfoItem->chairId);
             SendStartMsg(userInfoItem->chairId);
         }
		 OnApplyBankerList();
        //设置时间
//        m_TimerJetton.start(1000, bind(&CTableFrameSink::OnTimerJetton, this, std::placeholders::_1), NULL, m_nPlaceJettonLeveTime, false);
         ReadStorageInfo();
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));

}

void CTableFrameSink:: ClearTableData()
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
	memset(m_lBankerAllEndScore, 0, sizeof(m_lBankerAllEndScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

    for(int64_t jetton : m_pGameRoomInfo->jettons)
    {
        m_Tian[jetton] = 0;
        m_Di[jetton] = 0;
        m_Xuan[jetton] = 0;
        m_Huang[jetton] = 0;
    }

    memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;

        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        userInfoItem->clear();
    }

    memset(m_array_jetton, 0, sizeof(m_array_jetton));
    memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
}

bool  CTableFrameSink::OnMakeDir()
{
    std::string dir="./Log";
    if(access(dir.c_str(), 0) == -1)
    {
        if(mkdir(dir.c_str(), 0755)== 0)
        {
            dir="./Log/brnn";

            if(access(dir.c_str(), 0) == -1)
            {
                return (mkdir(dir.c_str(), 0755)== 0);
            }
        }
    }

    return false;
}

//游戏消息处理
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
//                return false;

            //变量定义
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
            }
            */

            //状态判断
//            assert(m_cbPlayStatus[wChairID]);
//            if(!m_cbPlayStatus[wChairID])
//                return false;

//            OnPlayerList(wChairID);
            return true;
        }
        case SUB_C_SYNC_TIME:
        {
            int32_t status = m_pITableFrame->GetGameStatus();
            CMD_S_SyncTime_Res res;

            res.set_userid(pIServerUserItem->GetUserId());
            res.set_status(status);

            chrono::system_clock::time_point curTime = chrono::system_clock::now();
            int32_t leaveTime = 0;
            switch (status)
            {
            case GAME_STATUS_START:
                leaveTime = m_nPlaceJettonTime - (chrono::duration_cast<chrono::seconds>(curTime - m_startTime)).count();
                break;
            case GAME_STATUS_END:
                leaveTime = m_nGameEndTime - (chrono::duration_cast<chrono::seconds>(curTime - m_endTime)).count();
            default:
                break;
            }
            res.set_timeleave(leaveTime >0 ? leaveTime : 0);
            std::string data = res.SerializeAsString();
            m_pITableFrame->SendTableData(wChairID, Brnn::SUB_S_SYNC_TIME, (uint8_t*)data.c_str(), data.size());

			return true;
        }
		case SUB_C_APPLY_BANKER:			//用户申请上庄
		{
			//变量定义
			Msg_ApplyBanker  pApplyBanker;
			pApplyBanker.ParseFromArray(pData, wDataSize);

			//消息处理
			return OnUserApplyBanker(wChairID, pApplyBanker.applyorcancel());
		}
		case SUB_C_CANCEL_BANKER:			//用户申请下庄
		{
			//变量定义
			/*Msg_CancelBanker  pCancelBanker;
			pCancelBanker.ParseFromArray(pData, wDataSize);*/

			//消息处理
			return OnUserCancelBanker(wChairID);
		}
		case SUB_C_APPLY_BANKERLIST:			//申请上庄玩家列表
		{
			//变量定义
			/*Msg_ApplyBankerList  ApplyBankerList;
			ApplyBankerList.ParseFromArray(pData, wDataSize);*/
			//消息处理
			return OnApplyBankerList();
		}
		break;
        return false;
    }
}


//加注事件
bool CTableFrameSink::OnUserAddScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore)
{
    //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
        LOG_ERROR << " Game Status is not GameStart ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
        return true;
    }
    //=====================================
    bool bFind = false;
    for(int i=0;i<7;i++)
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
    if(!bFind)
    {
        LOG_ERROR << " Can not find jettons Area";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 2);
        return true;
    }
    //======================================
    //校验是否当前用户
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " User is not Valid ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 3);
        return true;
    }
	int64_t lBankerTotalCanJettonScore = 0;
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();
    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3] + userInfo->m_lUserJettonScore[4];		
        int64_t canjetton=0;
        if(m_pGameRoomInfo->roomId==9301)
        {
            canjetton=lUserScore/3;
        }
        else
        {
            canjetton=lUserScore/10;
        }
        if(canjetton < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Real User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
            return true;
        }

        int64_t lAllJettonScore = userInfo->m_lUserJettonScore[cbJettonArea];
        lAllJettonScore += lJettonScore;
        if(lAllJettonScore > m_pGameRoomInfo->maxJettonScore)
        {
            LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
            return true;
        }

		// 当前玩家是庄家不能下注
		shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if (m_wBankerUser == wChairID)
		{
			LOG_ERROR << " 当前玩家是庄家不能下注 ";
			SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 9);
			return true;
		}
		// 总压注不能超过庄家的分数
		if (m_wBankerUser != INVALID_CHAIR)
		{
			lBankerTotalCanJettonScore = m_lAllJettonScore[1] + m_lAllJettonScore[2] + m_lAllJettonScore[3] + m_lAllJettonScore[4];
			shared_ptr<IServerUserItem> pIServerBankerItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
            int64_t lBankerScore = pIServerBankerItem->GetUserScore();
			canjetton = 0;
			if (m_pGameRoomInfo->roomId == 9301)
			{
				canjetton = lBankerScore / 3;
			}
			else
			{
				canjetton = lBankerScore / 10;
			}
			if (canjetton < lBankerTotalCanJettonScore + lJettonScore || lBankerScore - lBankerTotalCanJettonScore < 50.0)
			{
				LOG_ERROR << " 庄家分数不足 ";
				SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 10);
				return true;
			}
		}

        if(lTotalJettonScore == 0)
        {
            m_replay.addPlayer(userInfo->userId,pIServerUserItem->GetAccount(),lUserScore,wChairID);
        }
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,wChairID,cbJettonArea);
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
        //        m_lUserJettonScore[cbJettonArea][wChairID] += lJettonScore;
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
    }else //android suer
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2] + userInfo->m_lRobotUserJettonScore[3] + + userInfo->m_lRobotUserJettonScore[4];

        int64_t canjetton=0;
        if(m_pGameRoomInfo->roomId==9301)
        {
            canjetton=lUserScore/3;
        }
        else
        {
            canjetton=lUserScore/10;
        }
        if(canjetton < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Android User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 6);
            return true;
        }

        int64_t lAllJettonScore = userInfo->m_lRobotUserJettonScore[cbJettonArea];
        lAllJettonScore += lJettonScore;
        if(lAllJettonScore > m_pGameRoomInfo->maxJettonScore)
        {
            LOG_ERROR << " Android User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 7);
            return true;
        }
		// 总压注不能超过庄家的分数
		if (m_wBankerUser != INVALID_CHAIR)
		{
			lBankerTotalCanJettonScore = m_lAllJettonScore[1] + m_lAllJettonScore[2] + m_lAllJettonScore[3] + m_lAllJettonScore[4];
			shared_ptr<IServerUserItem> pIServerBankerItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
            int64_t lBankerScore = pIServerBankerItem->GetUserScore();
			canjetton = 0;
			if (m_pGameRoomInfo->roomId == 9301)
			{
				canjetton = lBankerScore / 3;
			}
			else
			{
				canjetton = lBankerScore / 10;
			}
			if (canjetton < lBankerTotalCanJettonScore + lJettonScore || lBankerScore - lBankerTotalCanJettonScore < 50.0)
			{
				LOG_ERROR << " 庄家分数不足 ";
				SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 10);
				return true;
			}
		}
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lRobotserJettonAreaScore[cbJettonArea] += lJettonScore;
        //        m_lRobotUserJettonScore[cbJettonArea][wChairID] += lJettonScore;
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
    }

    if(m_ShenSuanZiId == userId && m_ShenSuanZiId != 0)
    {
//        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
//        if(SSZUserInfo->chairId == wChairID)
          m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }

    //openLog("userid=%d area=%d score=%f",pIServerUserItem->GetUserID(),cbJettonArea,lJettonScore);
    //成功返回
    if(cbJettonArea == 1)
    {
        m_Tian[lJettonScore]++;
    }else if(cbJettonArea == 2)
    {
        m_Di[lJettonScore]++;
    }else if(cbJettonArea == 3)
    {
        m_Xuan[lJettonScore]++;
    }else if(cbJettonArea == 4)
    {
        m_Huang[lJettonScore]++;
    }

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    CMD_S_PlaceJetSuccess Jetton;
    auto push=[&](uint32_t iChairId,int64_t sendUserId ,bool Robot){
        Jetton.Clear();
        Jetton.set_dwuserid(userId);
        Jetton.set_cbjettonarea(cbJettonArea);
        Jetton.set_ljettonscore(lJettonScore);
        bool isRobot = pIServerUserItem->IsAndroidUser();
        Jetton.set_bisandroid(isRobot);
        int64_t xzScore = 0;
        if(isRobot)
            xzScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2] + userInfo->m_lRobotUserJettonScore[3]+ userInfo->m_lRobotUserJettonScore[4];
        else
            xzScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3] + userInfo->m_lUserJettonScore[4];
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 1; i < 5; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 1; i < 5; ++i)
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
        m_pITableFrame->SendTableData(iChairId, Brnn::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };


    if (!isInVipList(wChairID) && userId!=m_ShenSuanZiId && m_UserInfo.size() >= 8)
    {
        userInfoItem=m_UserInfo[userId];
        push(wChairID,userId,pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }
    else
    {
        for(auto &it : m_UserInfo)
        {
            if (!it.second)
                continue;

            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem||pUserItem->IsAndroidUser())
                continue;
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}

void CTableFrameSink::AddPlayerJetton(int64_t chairId,uint8_t cbJettonArea,int64_t lJettonScore)
{
    assert(chairId<GAME_PLAYER &&cbJettonArea<5); //cbJettonArea 1：天 ，2：地，3：玄，4：黄

    m_array_jetton[chairId].bJetton =true;
    m_array_jetton[chairId].iAreaJetton[cbJettonArea] +=lJettonScore;
    m_array_tmparea_jetton[cbJettonArea]+=lJettonScore;
}

void CTableFrameSink::GameTimerJettonBroadcast()
{
    Brnn::CMD_S_Jetton_Broadcast jetton_broadcast;

    bool havejetton=false;
    for (int i=1; i<5; ++i)
    {
        if (m_array_tmparea_jetton[i] > 0)
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
            if(!pUserItem || pUserItem->IsAndroidUser())
                continue;
            jetton_broadcast.Clear();

            for (int i=1; i<5; ++i)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[i]);
               jetton_broadcast.add_tmpjettonscore(m_array_tmparea_jetton[i]- m_array_jetton[userInfoItem->chairId].iAreaJetton[i]);
            }
            memset(m_array_jetton[userInfoItem->chairId].iAreaJetton, 0, sizeof(m_array_jetton[userInfoItem->chairId].iAreaJetton));
            std::string data = jetton_broadcast.SerializeAsString();
            m_pITableFrame->SendTableData(userInfoItem->chairId, Brnn::SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
        }
        memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
    }
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    {
        m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::SendPlaceBetFail(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, int32_t cbErrorcode)
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
     Jetton.set_cberrorcode(cbErrorcode);

     std::string data = Jetton.SerializeAsString();
     m_pITableFrame->SendTableData(wChairID, Brnn::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
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
        if(userInfoItem->m_EndWinScore >= m_pGameRoomInfo->broadcastScore)
        {
            //char formate[2048]={0};
            //sprintf(formate,"恭喜V%d玩家%s在龙虎中一把赢得%.2f元!",pUserItem->GetVipLevel(),pUserItem->GetNickName(),m_EndWinScore[i]);
            //m_pITableFrame->SendGameMessage(INVALID_CARD,formate,SMT_GLOBAL|SMT_SCROLL);
            m_pITableFrame->SendGameMessage(userInfoItem->chairId, "", SMT_GLOBAL|SMT_SCROLL, userInfoItem->m_EndWinScore);
        }
    }

}

int64_t CTableFrameSink::GetTotalUserJettonScore()
{
    int64_t lTrueUserJettonScore = m_lTrueUserJettonScore[1] + m_lTrueUserJettonScore[2] + m_lTrueUserJettonScore[3] + m_lTrueUserJettonScore[4];
    return lTrueUserJettonScore;
}

bool CTableFrameSink::IsUserBeted()
{
    int64_t lTrueUserJettonScore = GetTotalUserJettonScore();
    return lTrueUserJettonScore > 0 ? true : false;
}

int64_t CTableFrameSink::CalculateSafeHandicapLimit()
{
    SetSortCardInfo();
    SetSortAreaBetInfo();
    int64_t lBetHandicapValue = CalculateBetHandicap();

    int64_t lSafeHandicapLimit = m_lStockScore - lBetHandicapValue;
    return lSafeHandicapLimit;
}

uint8_t CTableFrameSink::CalculateAreaBetRatioType()
{
    uint8_t cbAreaBetType = 3;
    int64_t lTrueUserJettonScore = GetTotalUserJettonScore();
    bool bFlag = false;
    for(int i = 1; i < 5; ++i)
    {
        if(m_lTempSortBet[i] >= (lTrueUserJettonScore/2))
        {
            cbAreaBetType = 1;
            bFlag = true;
            break;
        }
    }

    if(!bFlag)
    {
        for(int i = 1; i < 5; ++i)
        {
            for(int j = i+1; j < 5; ++j)
            {
                if((m_lTempSortBet[i] + m_lTempSortBet[j]) >= (lTrueUserJettonScore*100/67))
                {
                    cbAreaBetType = 2;
                    bFlag = true;
                    break;
                }
            }
            if(bFlag) break;
        }
    }


    return cbAreaBetType;
}

void CTableFrameSink::SetSortCardInfo()
{
//    uint8_t bTempCardData[5][5] = {0};
//    memcpy(bTempCardData, m_cbCurrentRoundTableCards, sizeof(bTempCardData));
//    int nCardSort = 1;
//    //确定随机牌中的排序
//    for(int i = 0; i < 5; ++i)
//    {
//        nCardSort = 1;
//        for(int j = 0; j < 5; ++j)
//        {
//            if( j == i) continue;
//            if(m_GameLogic.CalculateMultiples(bTempCardData[j], bTempCardData[i], 5) < 0)
//                ++nCardSort;
//        }
//        m_cbSortAreaCard[i] = nCardSort;
//        int nTimes = 1;
//        int32_t iPoint = m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i], 5);
//        if(iPoint > 10)
//        {
//            nTimes = 3;
//        }
//        else if(iPoint > 7 && iPoint < 11)
//        {
//            nTimes = 2;
//        }

//        if(nCardSort == 1)
//        {
//            m_nTempTimes[4] = nTimes;
//            for(int j = 0;j < 5;++j)
//            {
//                m_cbTempTableCards[4][j] = m_cbCurrentRoundTableCards[i][j];
//            }
//        }
//        else if(nCardSort == 2)
//        {
//            m_nTempTimes[3] = nTimes;
//            for(int j = 0;j < 5;++j)
//            {
//                m_cbTempTableCards[3][j] = m_cbCurrentRoundTableCards[i][j];
//            }
//        }
//        else if(nCardSort == 3)
//        {
//            m_nTempTimes[2] = nTimes;
//            for(int j = 0;j < 5;++j)
//            {
//                m_cbTempTableCards[2][j] = m_cbCurrentRoundTableCards[i][j];
//            }
//        }
//        else if(nCardSort == 4)
//        {
//            m_nTempTimes[1] = nTimes;
//            for(int j = 0;j < 5;++j)
//            {
//                m_cbTempTableCards[1][j] = m_cbCurrentRoundTableCards[i][j];
//            }
//        }
//        else if(nCardSort == 5)
//        {
//            m_nTempTimes[0] = nTimes;
//            for(int j = 0;j < 5;++j)
//            {
//                m_cbTempTableCards[0][j] = m_cbCurrentRoundTableCards[i][j];
//            }
//        }
//    }
}

void CTableFrameSink::SetSortAreaBetInfo()
{
//    m_nTempBetIdx = {0,1,2,3,4};
    memcpy(m_lTempSortBet, m_lTrueUserJettonScore, sizeof(m_lTempSortBet));
    int i,j;
    for(i = 1; i < 5; ++i)
    {
        for(j = i+1; j < 5; ++j)
        {
            if(m_lTempSortBet[j] > m_lTempSortBet[i])
            {
                std::swap(m_lTempSortBet[j], m_lTempSortBet[i]);
//                std:swap(m_nTempBetIdx[j],m_nTempBetIdx[i]);
            }
        }
    }
}

int64_t CTableFrameSink::CalculateBetHandicap()
{
    int64_t lBetHandicapValue = (m_nTempTimes[0] * m_lTempSortBet[1] + m_nTempTimes[1] * m_lTempSortBet[2] +
            m_nTempTimes[2] * m_lTempSortBet[3] + m_nTempTimes[3] * m_lTempSortBet[4]);
//    uint8_t cbBetType = CalculateAreaBetRatioType();
//    switch (cbBetType)
//    {
//    case 1: //A
//    {
//        lBetHandicapValue = m_nTempTimes[0] * m_lTempSortBet[1];
//    }
//        break;
//    case 2: //B
//    {
//        lBetHandicapValue = (m_nTempTimes[0] * m_lTempSortBet[1] + m_nTempTimes[1] * m_lTempSortBet[2]);
//    }
//        break;
//    case 3: //C
//    {
//        lBetHandicapValue = (m_nTempTimes[1] * m_lTempSortBet[1] + m_nTempTimes[2] * m_lTempSortBet[2] + m_nTempTimes[3] * m_lTempSortBet[3]);
//    }
//        break;
//    default:
//        break;
//    }
    return lBetHandicapValue;
}

int CTableFrameSink::GetRandData()
{
    srand(time(NULL));
    return rand()%1000;
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
	int32_t iMulticle = brnnAlgorithm.GetMultiple(m_cbCurrentRoundTableCards[0]);
	Brnn::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分
	//系统开牌
	Brnn::SysCards* card = details.mutable_syscard();
	card->set_cards(&(m_cbCurrentRoundTableCards[0])[0], 5);
	card->set_ty(m_allCardType[0]);
	card->set_multi(iMulticle);

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		//1恭 2喜 3发 4财 
		for (int betIdx = 1;betIdx < 5; betIdx++)
		{
			// 结算时记录
            int64_t winScore = userInfo->m_EndWinJettonScore[betIdx];
			int64_t betScore = userInfo->m_lUserJettonScore[betIdx];
			if (m_wBankerUser != INVALID_CHAIR && userInfo->chairId == m_wBankerUser) //玩家坐庄
			{
				winScore = m_lBankerAllEndScore[betIdx];
				betScore = m_lAllJettonScore[betIdx];
			}
			int32_t	nMuilt = brnnAlgorithm.GetMultiple(m_cbCurrentRoundTableCards[betIdx]);// 倍率表m_nMuilt
			//if (betScore > 0) 
			{
				Brnn::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//手牌数据
				Brnn::SysCards* handcard = detail->mutable_handcard();
				handcard->set_cards(&(m_cbCurrentRoundTableCards[betIdx])[0], 5);
				handcard->set_ty(m_allCardType[betIdx]);
				handcard->set_multi(nMuilt);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
				//本区域是否赢[0:否;1:赢]
				if (m_allMultical[betIdx - 1]>=0)
				{
					detail->set_dareaiswin(1);
				} 
				else
				{
					detail->set_dareaiswin(0);
				}
			}
		}
	}
	if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == chairId)
	{
		details.set_isplayerbanker(true); //玩家坐庄
		details.set_wbankeruserid(userid); //玩家id
	}

	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}

//用户申请上庄
bool CTableFrameSink::OnUserApplyBanker(uint32_t chairId, bool applyOrCancel)
{
	if (m_bOpenBankerApply == 0)
	{
		return true;
	}
	LOG(INFO) << " >>> OnUserApplyBanker wChairID:" << chairId << " applyOrCancel:" << applyOrCancel ? "true" : "false";
	bool bApplySuccess = true;
	int32_t resCode = 0;

	shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
	if (!pIServerUserItem)
	{
		LOG(INFO) << "OnUserApplyBanker pIServerUserItem is NULL ";
		bApplySuccess = false;
		resCode = 1;
	}
	int64_t userId = pIServerUserItem->GetUserId();
	if (applyOrCancel) // 申请上庄
	{
		//如果当前是庄家不能上庄下庄
		if (bApplySuccess && m_wBankerUser == chairId)
		{
			bApplySuccess = false;
			resCode = 2;    //当前是庄家不能上庄
		}
		if (bApplySuccess && pIServerUserItem->GetUserScore() < m_wBankerLimitScore)
		{
			bApplySuccess = false;
			resCode = 3;          // 玩家携带分数不足
		}
		if (bApplySuccess && m_UserBankerList.size() >= m_wBankerApplyLimitCount)   
		{
			bApplySuccess = false;
			resCode = 4;          //上庄人数排队已经满了
		}
		
		//上庄前判断是否已经上庄
		if (bApplySuccess)
		{
			//auto it = m_UserBankerList.find(userId);
			bool bHaveInBanker = false;
			shared_ptr<UserInfo> userBankerInfoItem;
            vector<shared_ptr<UserInfo>>::iterator iter;//声明一个迭代器，来访问vector容器，作用：遍历或者指向vector容器的元素
            for(iter=m_UserBankerList.begin();iter!=m_UserBankerList.end();iter++)
            {
                userBankerInfoItem = *iter;
				if (userBankerInfoItem->userId == pIServerUserItem->GetUserId())
				{
					bHaveInBanker = true;
					break;
				}
			}
			if (!bHaveInBanker)
			{
				shared_ptr<UserInfo> userInfo(new UserInfo());
				userInfo->userId = userId;
				userInfo->chairId = chairId;
				userInfo->headerId = pIServerUserItem->GetHeaderId();
				userInfo->nickName = pIServerUserItem->GetNickName();
				userInfo->location = pIServerUserItem->GetLocation();

				m_UserBankerList.push_back(userInfo);
				bApplySuccess = true;    // 申请上庄成功
			}
			else 
			{
				bApplySuccess = false;
				resCode = 5;	// 已经申请过上庄
			}
		}		
	} 
	else  // 取消申请上庄
	{
		//auto it = m_UserBankerList.find(userId);
		bool bHaveInBanker = false;
		shared_ptr<UserInfo> userBankerInfoItem;
        vector<shared_ptr<UserInfo>>::iterator iter;//声明一个迭代器，来访问vector容器，作用：遍历或者指向vector容器的元素
        for(iter=m_UserBankerList.begin();iter!=m_UserBankerList.end();iter++)
        {
            userBankerInfoItem = *iter;
			if (userBankerInfoItem->userId == pIServerUserItem->GetUserId())
			{
				bHaveInBanker = true;
                m_UserBankerList.erase(iter);
				bApplySuccess = true;    // 取消申请上庄成功
				break;
			}
		}
		if (!bHaveInBanker)
		{
			bApplySuccess = false;
			resCode = 6;	// 不在申请上庄列表里面
		}
	}
	
	Msg_ApplyRes ApplyRes;

	ApplyRes.set_applyorcancel(applyOrCancel);
	ApplyRes.set_rescode(resCode);

	std::string data = ApplyRes.SerializeAsString();
	m_pITableFrame->SendTableData(chairId, Brnn::SUB_S_APPLY_RES, (uint8_t *)data.c_str(), data.size());

	OnApplyBankerList();
	return true;
}
//用户申请下庄
bool CTableFrameSink::OnUserCancelBanker(uint32_t chairId)
{
	if (m_bOpenBankerApply == 0)
	{
		return true;
	}
	LOG(INFO) << " >>> OnUserCancelBanker wChairID:" << chairId;
	bool bApplySuccess = true;
	int32_t resCode = 0;	

	shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
	if (!pIServerUserItem)
	{
		LOG(INFO) << "OnUserCancelBanker pIServerUserItem is NULL ";
		bApplySuccess = false;
		resCode = 1;
	}
	
	if (bApplySuccess && m_wBankerUser != chairId)
	{
		bApplySuccess = false;
		resCode = 2;	// 当前庄家不是玩家自己，不能下庄
	}
	
	if (resCode == 0)
	{
		m_buserCancelBanker = true;
	}
	else
	{
		m_buserCancelBanker = false;
	}
	Msg_CancelRes CancelRes;

	CancelRes.set_rescode(resCode);

	std::string data = CancelRes.SerializeAsString();
	m_pITableFrame->SendTableData(chairId, Brnn::SUB_S_CANCEL_RES, (uint8_t *)data.c_str(), data.size());

	return true;
}

//是否需要换庄
bool CTableFrameSink::OnGameChangeBanker()
{
	bool bChange = false;
	if (m_buserCancelBanker || m_wBankerContinuityCount >= m_wBankerLimitCount) {
		m_wBankerContinuityCount = 0;
		bChange = true;
	}

	if (m_wBankerUser != INVALID_CHAIR)
	{
		shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(m_wBankerUser);
		if (pIServerUserItem)
		{
			if (pIServerUserItem->GetUserScore() < m_wBankerLimitScore)
			{
				bChange = true;
			}
		}
		else
		{
			bChange = true;
		}
	}
	else if (m_wBankerUser == INVALID_CHAIR) //系统庄
	{
		bChange = true;
	}

	ClearNotEnoughScoreInApplyBankerList();

	if (bChange)
	{		
        m_wBankerUser = INVALID_CHAIR;
		shared_ptr<UserInfo> userInfoItem;
        vector<shared_ptr<UserInfo>>::iterator iter;//声明一个迭代器，来访问vector容器，作用：遍历或者指向vector容器的元素
        for(iter=m_UserBankerList.begin();iter!=m_UserBankerList.end();iter++)
        {
            userInfoItem = *iter;
			shared_ptr<CServerUserItem> pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem)
				continue;

			m_wBankerUser = userInfoItem->chairId;
			m_wBankerContinuityCount = 0;
            m_UserBankerList.erase(iter);
			break;
		}
	}
	m_buserCancelBanker = false;
	return bChange;
}

// 申请上庄玩家列表
bool CTableFrameSink::OnApplyBankerList()
{
	if (m_bOpenBankerApply == 0)
	{
		return true;
	}
	Msg_ApplyBankerListRes ApplyBankerListRes;
    ApplyBankerListRes.set_lapplybankercondition(m_wBankerLimitScore);
    ApplyBankerListRes.set_napplybankercount(m_UserBankerList.size());

	long nowScore = 0;
	long xzScore = 0;
	shared_ptr<UserInfo> userInfoItem;
	shared_ptr<IServerUserItem> pUserItem;
	for (auto &it : m_UserBankerList)
	{
		userInfoItem = it;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			nowScore = 0;
		else
		{
			xzScore = 0;
			if (pUserItem->IsAndroidUser())
				xzScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3] + userInfoItem->m_lRobotUserJettonScore[4];
			else
				xzScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3] + userInfoItem->m_lUserJettonScore[4];

			nowScore = pUserItem->GetUserScore() - xzScore;
		}

		PlayerListItem* palyer = ApplyBankerListRes.add_players();
		palyer->set_dwuserid(userInfoItem->userId);
		palyer->set_headerid(userInfoItem->headerId);
		palyer->set_nickname(userInfoItem->nickName);
		palyer->set_luserscore(nowScore);

	}
	std::string data = ApplyBankerListRes.SerializeAsString();

	for (auto &it : m_UserInfo) {
		userInfoItem = it.second;
		shared_ptr<CServerUserItem> pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		if (pUserItem->IsAndroidUser())
			continue;
		m_pITableFrame->SendTableData(userInfoItem->chairId, Brnn::SUB_S_BANKERLIST_RES, (uint8_t *)data.c_str(), data.size());
	}
	

	return true;
}

// 清理待上庄玩家列表中钱不够的玩家
void CTableFrameSink::ClearNotEnoughScoreInApplyBankerList()
{
	vector<shared_ptr<UserInfo>> tmp_UserBankerList(m_UserBankerList);
	m_UserBankerList.clear();

	shared_ptr<UserInfo> userInfoItem;
	vector<shared_ptr<UserInfo>>::iterator iter;//声明一个迭代器，来访问vector容器，作用：遍历或者指向vector容器的元素
	for (iter = tmp_UserBankerList.begin();iter != tmp_UserBankerList.end();iter++)
	{
		userInfoItem = *iter;
		shared_ptr<CServerUserItem> pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;

		if (pUserItem->GetUserScore() < m_wBankerLimitScore)
		{
            bool applyOrCancel = false;
			int resCode = 7;          // 玩家携带分数不足,踢出申请列表

			Msg_ApplyRes ApplyRes;

			ApplyRes.set_applyorcancel(applyOrCancel);
			ApplyRes.set_rescode(resCode);

			std::string data = ApplyRes.SerializeAsString();
			m_pITableFrame->SendTableData(userInfoItem->chairId, Brnn::SUB_S_APPLY_RES, (uint8_t *)data.c_str(), data.size());

			continue;
		}

        m_UserBankerList.push_back(userInfoItem);
	}

	//OnApplyBankerList();
}

void CTableFrameSink::AllocationCard(uint8_t byCardBuffer[], int nLen, int nBankerIndex)
{
    int tBankerMulp;
    int tTotalWin;
    while(true)
    {
        random_shuffle(byCardBuffer, byCardBuffer+nLen);
        tBankerMulp = m_nTempTimes[nBankerIndex-1];
        tTotalWin = 0;
        for(int i = 0;i <4;i++)
        {
            if(byCardBuffer[i] > nBankerIndex-1)
            {
                tTotalWin -= tBankerMulp*m_lTrueUserJettonScore[i+1];
            }else
            {
                tTotalWin += m_nTempTimes[byCardBuffer[i]]*m_lTrueUserJettonScore[i+1];
            }
        }
        LOG(WARNING) << "nBankerIndex:" << nBankerIndex;
        LOG(WARNING) << "BUFFERS:" << (int)byCardBuffer[0] <<","<< (int)byCardBuffer[1] <<"," << (int)byCardBuffer[2] <<","<< (int)byCardBuffer[3];
        LOG(WARNING) << "BETS:" << (int)m_lTrueUserJettonScore[1] <<","<< (int)m_lTrueUserJettonScore[2] <<"," << (int)m_lTrueUserJettonScore[3] <<","<< (int)m_lTrueUserJettonScore[4];
        LOG(WARNING) << "TIMES:" << (int)m_nTempTimes[0] <<","<< (int)m_nTempTimes[1] <<","<< (int)m_nTempTimes[2] <<"," << (int)m_nTempTimes[3] <<","<< (int)m_nTempTimes[4];
        if(nBankerIndex > 2 && (tTotalWin >= m_lMaxLose && tTotalWin >= m_interfere_line))
        {
            LOG(WARNING) << "Trigger safety switch on high limits";
            nBankerIndex--;
            if(nBankerIndex == 4)
            {
                byCardBuffer[0] = 0;
                byCardBuffer[1] = 1;
                byCardBuffer[2] = 2;
                byCardBuffer[3] = 4;
            }else if(nBankerIndex == 3)
            {
                byCardBuffer[0] = 0;
                byCardBuffer[1] = 1;
                byCardBuffer[2] = 3;
                byCardBuffer[3] = 4;
            }else if(nBankerIndex == 2)
            {
                byCardBuffer[0] = 0;
                byCardBuffer[1] = 2;
                byCardBuffer[2] = 3;
                byCardBuffer[3] = 4;
            }
        }else{
            LOG(ERROR) << "tTotalWin:" << tTotalWin;
            break;
        }
    }


    for(int j = 0; j < 5; ++j)
    {
        m_cbCurrentRoundTableCards[0][j] = m_cbTempTableCards[nBankerIndex-1][j];
    }
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            m_cbCurrentRoundTableCards[i+1][j] = m_cbTempTableCards[byCardBuffer[i]][j];
        }
    }
}

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
