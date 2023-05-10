#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
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
#include <functional>
#include <chrono>

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

#include "proto/Longhu.Message.pb.h"

#include "GameLogic.h"
//#include "HistoryScore.h"
// #include "IAicsdragon.h"

#include <muduo/base/Mutex.h>

#include "LongHuAlgorithm.h"

using namespace Longhu;

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
        FLAGS_log_dir = "./log/longhu/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("longhu");
        // google::SetStderrLogging(google::GLOG_ERROR);
        google::SetStderrLogging(google::GLOG_INFO);
    }

    virtual ~CGLogInit()
    {
        google::ShutdownGoogleLogging();
    }
};

CGLogInit glog_init;

class RomMgr
{
public:
    RomMgr()
    {
        bisInit = false;

        if(access("./log/longhu", 0) == -1)
           mkdir("./log/longhu",0777);
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
        return (*sp);
    }
private:
     bool bisInit;
};


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

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

};


//构造函数
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "LongHuTimerEventLoopThread")
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

//    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
//    memset(m_long, 0, sizeof(m_long));
//    memset(m_hu, 0, sizeof(m_hu));
//    memset(m_he, 0, sizeof(m_he));

    m_strRoundId = "";

    m_Long.clear();
    m_Hu.clear();
    m_He.clear();

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
    m_dControlkill=0.0;
    m_lLimitPoints=0;
    stockWeak = 0.0;
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
    m_nWinIndex = 0;

	m_bGameEnd = false;
	m_fUpdateGameMsgTime = 0;
	m_fResetPlayerTime = 5.0f;
	m_cbGameStatus = GAME_STATUS_INIT;
	m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_strTableId = "";		//桌号
	memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
	memset(m_lLimitScore, 0, sizeof(m_lLimitScore));
	m_ipCode = 0;
	m_bUseCancelBet = false;

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
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/longhu_config.ini"))
    {
        LOG_INFO << "./conf/longhu_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/longhu_config.ini", pt);

    stockWeak=  pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    //Time
    m_nPlaceJettonTime   = pt.get<int64_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 7);
    m_nGameTimeJettonBroadcast= pt.get<float>("GAME_CONFIG.TM_JETTON_BROADCAST", 1);
	m_fUpdateGameMsgTime = pt.get<float>("GAME_CONFIG.TM_UpdateGameMsgTime", 1.0);
	m_fResetPlayerTime   = pt.get<float>("GAME_CONFIG.TM_RESET_PLAYER_LIST", 5.0);

    m_LongTimes          = pt.get<int64_t>("GAME_CONFIG.AREA_LONG", 1);
    m_huTimes            = pt.get<int64_t>("GAME_CONFIG.AREA_HU", 1);
    m_heTimes            = pt.get<int64_t>("GAME_CONFIG.AREA_HEE", 8);

    m_lLimitPoints       = pt.get<int64_t>("GAME_CONFIG.LimitPoint" + to_string(m_pGameRoomInfo->roomId % 100), 500000);
    m_dControlkill       = pt.get<double>("GAME_CONFIG.Controlkill", 0.35);

    m_AreaMaxJetton[1]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".HE", 1000000);
    m_AreaMaxJetton[2]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".LONG", 1000000);
    m_AreaMaxJetton[3]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON_" + to_string(m_pGameRoomInfo->roomId) + ".HU", 1000000);

    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);
    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog", 0);
    m_WinScoreSystem     = pt.get<int64_t>("GAME_CONFIG.WinScore", 500);
    m_bOpenContorl       = pt.get<bool>("GAME_CONFIG.Control", true);

	int nUseCancelBet	 = pt.get<float>("GAME_CONFIG.USECANCELBET", 0);
	if (nUseCancelBet == 1)
		m_bUseCancelBet = true;
	else
		m_bUseCancelBet = false;

    vector<int64_t> scorelevel;
    vector<int64_t> chips;
	string strCHIP_CONFIGURATION = "";
	strCHIP_CONFIGURATION = "CHIP_CONFIGURATION_" + to_string(m_pGameRoomInfo->roomId);

	m_lLimitScore[0] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMin", 100);
	m_lLimitScore[1] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMax", 2000000);

    allchips.clear();
    scorelevel.clear();
    chips.clear();
    useIntelligentChips=pt.get<int64_t>(strCHIP_CONFIGURATION + ".useintelligentchips", 0);
    scorelevel = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear1","100,200,500,1000,5000"));
    for(int i=0;i<4;i++)
    {
        m_userScoreLevel[i]=scorelevel[i];
    }
    for(int i=0;i < CHIP_COUNT;i++)
    {
        m_userChips[0][i]=chips[i];
        allchips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear2","100,200,500,1000,5000"));
    for(int i=0;i < CHIP_COUNT;i++)
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
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear3","100,200,500,1000,5000"));
    for(int i=0;i < CHIP_COUNT;i++)
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
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear4","100,200,500,1000,5000"));
    for(int i=0;i < CHIP_COUNT;i++)
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
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000"));
    if(!useIntelligentChips)
    {
        for(int i=0;i < CHIP_COUNT;i++)
        {
             m_userChips[0][i]=chips[i];
             m_userChips[1][i]=chips[i];
             m_userChips[2][i]=chips[i];
             m_userChips[3][i]=chips[i];
        }
    }
    for(int64_t chips : allchips)
    {
        m_Long[chips] = 0;
        m_Hu[chips] = 0;
        m_He[chips] = 0;
    }
}

bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{

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
            totalxz = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
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

		m_LongHuAlgorithm.SetRoomMsg(m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
        m_ShenSuanZiId = pIServerUserItem->GetUserId();
//        openLog("11111shensunzi=%d",m_ShenSuanZiId);
//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
//        m_nGameEndLeveTime = m_nGameEndTime;
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
//            m_Long[jetton] = 0;
//            m_Hu[jetton] = 0;
//            m_He[jetton] = 0;
//        }
    }
    for(int i=0;i < CHIP_COUNT;i++)
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
           return true;
        }
        break;
        case GAME_STATUS_START:	//游戏状态
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
    /*if(cbGameStatus == GAME_STATUS_START)
    {
        size = m_openRecord.size();
    }else
    {
        if(m_openRecord.size() > 0)
            size = m_openRecord.size() - 1;
    }*/
	size = m_openRecord.size();

    CMD_S_GameRecord record;
    uint32_t longWin = 0;
    uint32_t huWin = 0;
    uint32_t heWin = 0;
    for(int i = 0; i < size; ++i)
    {
        record.add_record(m_openRecord[i]);
        if(m_openRecord[i] == 2)
            longWin++;
        else if(m_openRecord[i] == 3)
            huWin++;
        else
            heWin++;
    }

    record.set_lcount(longWin);
    record.set_hucount(huWin);
    record.set_hecount(heWin);

    std::string data = record.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, Longhu::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendStartMsg(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_StartPlaceJetton pGamePlayScene;
    GameDeskInfo* pInfo = pGamePlayScene.mutable_deskdata();
    for(int i=0;i < CHIP_COUNT;i++)
    {
        pInfo->add_userchips(m_currentchips[chairId][i]);
    }
    for(int i = 1; i < 4; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
        alljitton->set_ljettonarea(i);
    }

    for(int i = 1; i < 4; ++i)
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
    for(int i = 1; i < 4; ++i)
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
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);
    for(int i = 0; i < 2; ++i)
    {
        CardGroup* card =pInfo->add_cards();
        card->add_cards(m_cbCurrentRoundTableCards[i]);
        card->set_group_id(i+2);
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
       pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
       if(!pUserItem)
           nowScore = 0;
       else
       {
           xzScore = 0;
           if(pUserItem->IsAndroidUser())
               xzScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3];
           else
               xzScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3];

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
    pInfo->set_szgameroomname("Longhu");
    if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);

    pInfo->set_wincardgrouptype(0);// add_wincardgrouptype(0);

    for(auto &jetton : m_He)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(1);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Long)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(2);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hu)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(3);
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

    pGamePlayScene.set_roundid(m_strRoundId);
    pGamePlayScene.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_startTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pGamePlayScene.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    xzScore = 0;
    bool isRobot = pIServerUserItem->IsAndroidUser();
    if(isRobot)
        xzScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2]+ userInfo->m_lRobotUserJettonScore[3];
    else
        xzScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];

    nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pGamePlayScene.set_lusermaxscore(maxscore);
    pGamePlayScene.set_luserscore(nowScore);

    pGamePlayScene.set_verificationcode(0);

    std::string data = pGamePlayScene.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Longhu::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
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
    for(int i = 1; i < 4; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 1; i < 4; ++i)
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
    for(int i = 1; i < 4; ++i)
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
    for(int i = 0; i < 2; ++i)
    {
        CardGroup* card = pInfo->add_cards();
        card->add_cards(m_cbCurrentRoundTableCards[i]);
        card->set_group_id(i+2);
        card->set_cardtype(0);
    }

    bool isVIP = false;
    uint32_t i = 0;
    uint32_t size = 6;
    int64_t nowScore = 0;
//    int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pUserItem;
    for(auto &userInfoItem : m_SortUserInfo)
    {
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            nowScore = 0;
        else
            nowScore = pUserItem->GetUserScore() + userInfoItem->m_EndWinScore;

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
    pInfo->set_szgameroomname("Longhu");

	if (m_bGameEnd)
	{
		pInfo->set_wintag(m_nWinIndex);
	} 
	else
	{
		if (m_openRecord.size() > 0)
			pInfo->set_wintag(m_openRecord[m_openRecord.size() - 1]);
		else
			pInfo->set_wintag(0);
	}
    /*
    else
    {
        m_openRecord.push_back(rand()%2+1);
        pInfo->add_wintag(m_openRecord[m_openRecord.size()-1]);
    }
    */

    pInfo->set_wincardgrouptype(0);

    for(auto &jetton : m_He)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(1);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Long)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(2);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hu)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(3);
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

    gamend.set_roundid(m_strRoundId);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_endTime);
    int32_t leaveTime = m_nGameEndTime - durationTime.count();
    gamend.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    nowScore = pIServerUserItem->GetUserScore() + userInfo->m_EndWinScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    gamend.set_lusermaxscore(maxscore);
    gamend.set_luserscore(nowScore);
    gamend.set_lrevenue(userInfo->m_Revenue);
    gamend.set_verificationcode(0);

    LOG_DEBUG << "Revenue:"<<userInfo->m_Revenue;

    if(!isVIP)  //myself
    {
        PlayerScoreValue* score = gamend.add_gameendscore();
        score->set_userid(userId);
        score->set_returnscore(userInfo->m_EndWinScore);
        score->set_userscore(nowScore);
    }

    string data = gamend.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Longhu::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool bClear = false;

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
            totalxz = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
        }

        pServerUserItem->setOffline();
        if(totalxz == 0)
        {
            {
//                WRITE_LOCK(m_mutex);
                m_UserInfo.erase(userId);
            }
            m_pITableFrame->ClearTableUser(chairId);
            bClear = true;
        }else
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

//根据系统输赢发牌
void CTableFrameSink::GetOpenCards()
{

    int8_t playerMuiltcle[3]={8,1,1};
    playerMuiltcle[0]=m_heTimes;
    playerMuiltcle[1]=m_LongTimes;
    playerMuiltcle[2]=m_huTimes;

    tagStorageInfo storageInfo;
    int8_t winFlag = 2;//1-he 2-long 3-hu
    if(true)
    {
        int  isHaveBlackPlayer=0;
        for(int i=0;i<GAME_PLAYER;i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
            {
                continue;
            }
            int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]
            +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[3];
            if(allJetton==0) continue;
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
        openLog("后台设置的系统难度 :%d",m_difficulty);
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
        if(randomRet<m_difficulty)
        {
            m_pITableFrame->GetStorageScore(storageInfo);

            m_LongHuAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
            ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            //
            m_LongHuAlgorithm.SetMustKill(-1);
            for(int i=0;i<3;i++)
             m_LongHuAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i+1]);//设置每门真人押注值
            m_LongHuAlgorithm.GetOpenCard(winFlag);
        }
        else
        {
            //判断玩家是不是倍投
            JudgmentUserIsDoubleCast();
            //存在黑名单控制名单
            if(isHaveBlackPlayer)
            {
                float probilityAll[3]={0.0f};

                bool isMustKill = false;
                bool anyCase=true;
                int64_t mustBeKillId = 0;
                //user probilty count
                for(int i=0;i<GAME_PLAYER;i++)
                {
                    shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                    if(!user||user->IsAndroidUser())
                    {
                        continue;
                    }
                    int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2]+m_UserInfo[user->GetUserId()]->m_lUserJettonScore[3];

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
                           if(m_lTrueUserJettonScore[j+1]>0)
                           {
                               probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->m_lUserJettonScore[j+1])/((float)m_lTrueUserJettonScore[1]+(float)m_lTrueUserJettonScore[2]+(float)m_lTrueUserJettonScore[3]);
                           }

                       }
                       //既是黑名单，又是倍投玩家
                       if(m_UserInfo[user->GetUserId()]->isDoubleCast)
                       {
                           isMustKill = true ;
                           anyCase = false;
                           mustBeKillId = user->GetUserId();
                            LOG(ERROR) << "开始既是黑名单，又是倍投判断 ";
                           break;
                       }
                       if(shuzhi==1000)
                       {
                           isMustKill = true ;
                           anyCase = true;
                           mustBeKillId = user->GetUserId();
                           break;
                       }
                    }
                }

                m_pITableFrame->GetStorageScore(storageInfo);

                m_LongHuAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
                ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值

                for(int i=0;i<3;i++)
                 m_LongHuAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i+1]);//设置每门真人押注值

                if(isMustKill)
                {
                    m_LongHuAlgorithm.BlackGetMustkillOpenCard(winFlag,probilityAll,m_UserInfo[mustBeKillId]->m_lUserJettonScore,anyCase);
                }
                else
                {
                     m_LongHuAlgorithm.BlackGetOpenCard(winFlag,probilityAll);
                }
            }
            else
            {

                m_pITableFrame->GetStorageScore(storageInfo);

                m_LongHuAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
                ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值

                for(int i=0;i<3;i++)
                 m_LongHuAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i+1]);//设置每门真人押注值
                m_LongHuAlgorithm.GetOpenCard(winFlag);
            }

        }

    }
    double res=0.0;
    res = m_LongHuAlgorithm.CurrentProfit();
    if(res>0)
    {
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    LOG(ERROR)<<" systemReduceRatio: "<<m_pITableFrame->GetGameRoomInfo()->systemReduceRatio;
    m_pITableFrame->UpdateStorageScore(res);
    m_pITableFrame->GetStorageScore(storageInfo);
    if(winFlag==1)
    {
        openLog("本局开奖结果过是：和");
    }
    else if(winFlag==2)
    {
        openLog("本局开奖结果过是：龙");
    }
    else
    {
        openLog("本局开奖结果过是：虎");
    }
    openLog("本次系统赢分是：    %d",m_LongHuAlgorithm.CurrentProfit());
    openLog("系统库存上限是：    %ld",storageInfo.lUplimit);
    openLog("系统库存下限是：    %ld",storageInfo.lLowlimit);
    openLog("系统当前库存是：    %ld",storageInfo.lEndStorage);
    openLog("和门真人下注：    %d",m_lTrueUserJettonScore[1]);
    openLog("龙门真人下注：    %d",m_lTrueUserJettonScore[2]);
    openLog("虎门真人下注：    %d",m_lTrueUserJettonScore[3]);


    if(winFlag >= 1 && winFlag <= 3)

    LOG_DEBUG << "......winFlag:"<< winFlag <<  "......";

    m_cbCurrentRoundTableCards[0] = 0;
    m_cbCurrentRoundTableCards[1] = 0;
//    memset(m_EndWinScore, 0, sizeof(m_EndWinScore));

     //设置扑克
    int longIndex = -1;
    int huIndex = -1;

    int pkSize = m_CardVecData.size();
    longIndex = rand() % pkSize;
    uint8_t longType = m_CardVecData[longIndex];
    uint8_t longValue = m_GameLogic.GetCardValue(longType);

    uint8_t huType = 0;
    uint8_t huValue = 0;

    switch(winFlag)
    {
        case 1:
            do
            {
                for(huIndex = pkSize - 1; huIndex >= 0; --huIndex)
                {
                    if(huIndex != longIndex)
                    {
                        huType = m_CardVecData[huIndex];
                        huValue =m_GameLogic.GetCardValue(huType);
                        if(longValue == huValue)
                        {
                            m_cbCurrentRoundTableCards[0] = longType;
                            m_cbCurrentRoundTableCards[1] = huType;
                            break;
                        }
                    }
                }

                if(huIndex < 0) //not find the same card value
                {
                    longIndex = rand() % pkSize;
                    longType = m_CardVecData[longIndex];
                    longValue =m_GameLogic.GetCardValue(longType);
                }else
                {
                    break;
                }
            }while(true);
            break;
        case 2:
            do
            {
                for(huIndex = pkSize - 1; huIndex >= 0; --huIndex)
                {
                    if(huIndex != longIndex)
                    {
                        huType = m_CardVecData[huIndex];
                        huValue =m_GameLogic.GetCardValue(huType);
                        if(longValue == huValue)
                            continue;
                        else if(longValue > huValue)
                        {
                            m_cbCurrentRoundTableCards[0] = longType;
                            m_cbCurrentRoundTableCards[1] = huType;
                        }else
                        {
                            m_cbCurrentRoundTableCards[0] = huType;
                            m_cbCurrentRoundTableCards[1] = longType;
                        }
                        break;
                    }
                }
                if(huIndex < 0) //not find the small card value
                {
                    longIndex = rand() % pkSize;
                    longType = m_CardVecData[longIndex];
                    longValue =m_GameLogic.GetCardValue(longType);
                }else
                {
                    break;
                }
            }while(true);
            break;
        case 3:
            do
            {
                for(huIndex = pkSize - 1; huIndex >= 0; --huIndex)
                {
                    if(huIndex != longIndex)
                    {
                        huType = m_CardVecData[huIndex];
                        huValue =m_GameLogic.GetCardValue(huType);
                        if(longValue == huValue)
                            continue;
                        else if(longValue < huValue)
                        {
                            m_cbCurrentRoundTableCards[0] = longType;
                            m_cbCurrentRoundTableCards[1] = huType;
                        }else
                        {
                            m_cbCurrentRoundTableCards[0] = huType;
                            m_cbCurrentRoundTableCards[1] = longType;
                        }
                        break;
                    }
                }
                if(huIndex < 0) //not find the large card value
                {
                    longIndex = rand() % pkSize;
                    longType = m_CardVecData[longIndex];
                    longValue =m_GameLogic.GetCardValue(longType);
                }else
                {
                    break;
                }
            }while(true);
            break;
        default:
            LOG_DEBUG << "......WinFlag:"<<winFlag<<" Error!";
    }

    if(longIndex > huIndex)
    {
        m_CardVecData.erase(m_CardVecData.begin() + longIndex);
        m_CardVecData.erase(m_CardVecData.begin() + huIndex);
    }else
    {
        m_CardVecData.erase(m_CardVecData.begin() + huIndex);
        m_CardVecData.erase(m_CardVecData.begin() + longIndex);
    }
}

//判断玩家是不是倍投
void CTableFrameSink::JudgmentUserIsDoubleCast()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem||pUserItem->IsAndroidUser())
            continue;
        int64_t curJetton = userInfoItem->m_lUserJettonScore[1]+userInfoItem->m_lUserJettonScore[2]+userInfoItem->m_lUserJettonScore[3];
        if(curJetton<1)
        {
            continue;
        }

        //上次玩家整体是赢钱
        if(userInfoItem->lLastWinOrLost)
        {
           //上次赢了的本局操作不影响倍投统计
        }
        else
        {
            userInfoItem->iLoseTimes++;
            //输了之后倍投1.8-3倍, 倍投基本不大于1.8到三局之后就即使中也亏钱了
            if(curJetton>userInfoItem->lLastJetton*180/100
                    &&curJetton<userInfoItem->lLastJetton*250/100)
            {
                userInfoItem->iDoubleCastTimes++;
                //出现倍投次数超过5次避免误打误撞倍投,当然还有黑名单做过滤，5次以上概率就不再是碰巧,输了之后基本都是倍投，这种操作概率超过百分至七十的
                if(userInfoItem->iLoseTimes>5&&(float)userInfoItem->iDoubleCastTimes/(float)userInfoItem->iLoseTimes>0.8)
                {
                    userInfoItem->isDoubleCast =true;
                }
            }
           LOG(ERROR) << "curJetton: "<<curJetton<<"     userInfoItem->lLastJetton:"<<userInfoItem->lLastJetton;
           LOG(ERROR) << "iLoseTimes: "<<userInfoItem->iLoseTimes<<"    isDoubleCast: "<<userInfoItem->isDoubleCast
                      <<"      userInfoItem->iDoubleCastTimes: "<<userInfoItem->iDoubleCastTimes;
        }

    }
}
//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;
//    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
//    int64_t userId = pIServerUserItem->GetUserId();
//    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();     //open cards
		m_replay.addStep((uint32_t)time(NULL)-m_dwReadyTime,GlobalFunc::converToHex(m_cbCurrentRoundTableCards, 2),-1,opShowCard,-1,-1);
        uint8_t longValue = m_GameLogic.GetCardValue(m_cbCurrentRoundTableCards[0]);
        uint8_t huValue = m_GameLogic.GetCardValue(m_cbCurrentRoundTableCards[1]);

        string longCardName = GetCardValueName(0);
        string huCardName = GetCardValueName(1);
        LOG_DEBUG << "Open Card LongCard:"<<longCardName<<" HuCard:"<<huCardName;

        //结算
        if(longValue > huValue)
        {
            LOG_DEBUG<<"......Long Win......";
			m_nWinIndex = 2;
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
                    if(userInfoItem->m_lRobotUserJettonScore[2] > 0)
                    {
                        //税收
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[2] * m_LongTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[2]*m_LongTimes - userInfoItem->m_Revenue);
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[3] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[3];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[1] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[1];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }else   //if(!pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lUserJettonScore[2] > 0)
                    {
                        //税收
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[2] * m_LongTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[2] * m_LongTimes - userInfoItem->m_Revenue);
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,2,userInfoItem->m_lUserJettonScore[2],(userInfoItem->m_lUserJettonScore[2]*m_LongTimes - userInfoItem->m_Revenue),"",false);
						userInfoItem->dAreaWin[2] = (userInfoItem->m_lUserJettonScore[2] * m_LongTimes - userInfoItem->m_Revenue);
                    }

                    if(userInfoItem->m_lUserJettonScore[3]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[3];
                        m_replay.addResult(userInfoItem->chairId,3,userInfoItem->m_lUserJettonScore[3],-userInfoItem->m_lUserJettonScore[3],"",false);
						userInfoItem->dAreaWin[3] = -userInfoItem->m_lUserJettonScore[3];
                    }

                    if(userInfoItem->m_lUserJettonScore[1]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[1];
                        m_replay.addResult(userInfoItem->chairId,1,userInfoItem->m_lUserJettonScore[1],-userInfoItem->m_lUserJettonScore[1],"",false);
						userInfoItem->dAreaWin[1] = -userInfoItem->m_lUserJettonScore[1];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }else if(longValue < huValue)
        {
            LOG_DEBUG<<"......Hu Win......";
			m_nWinIndex = 3;
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
                    if(userInfoItem->m_lRobotUserJettonScore[2] > 0)
                    {
                        //税收
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[2];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[3] > 0)
                    {
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[3] * m_huTimes);
//                        openLog("robotId=%d Tax=%f",pUserItem->GetUserId(), userInfoItem->m_Revenue);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[3] * m_huTimes - userInfoItem->m_Revenue);
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[1] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[1];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }else
                {
                    if(userInfoItem->m_lUserJettonScore[2] > 0)
                    {
                        //税收
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[2];
                        m_replay.addResult(userInfoItem->chairId,2,userInfoItem->m_lUserJettonScore[2],-userInfoItem->m_lUserJettonScore[2],"",false);
						userInfoItem->dAreaWin[2] = -userInfoItem->m_lUserJettonScore[2];
                    }

                    if(userInfoItem->m_lUserJettonScore[3] > 0)
                    {
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[3] * m_huTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[3] * m_huTimes - userInfoItem->m_Revenue);
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,3,userInfoItem->m_lUserJettonScore[3],(userInfoItem->m_lUserJettonScore[3]*m_huTimes - userInfoItem->m_Revenue),"",false);
						userInfoItem->dAreaWin[3] = (userInfoItem->m_lUserJettonScore[3] * m_huTimes - userInfoItem->m_Revenue);
                    }

                    if(userInfoItem->m_lUserJettonScore[1] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[1];
                        m_replay.addResult(userInfoItem->chairId,1,userInfoItem->m_lUserJettonScore[1],-userInfoItem->m_lUserJettonScore[1],"",false);
						userInfoItem->dAreaWin[1] = -userInfoItem->m_lUserJettonScore[1];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }else
        {
            LOG_DEBUG<<"......He Win......";
			m_nWinIndex = 1;
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
                    if(userInfoItem->m_lRobotUserJettonScore[1] > 0)
                    {
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[1] * m_heTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[1] * m_heTimes - userInfoItem->m_Revenue);
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }
                }else
                {
                    if(userInfoItem->m_lUserJettonScore[1] >= 0)
                    {
                        userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[1] * m_heTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[1] * m_heTimes - userInfoItem->m_Revenue);
                        m_replay.addResult(userInfoItem->chairId,1,userInfoItem->m_lUserJettonScore[1],(userInfoItem->m_lUserJettonScore[1]*m_heTimes - userInfoItem->m_Revenue),"",false);
						userInfoItem->dAreaWin[1] = (userInfoItem->m_lUserJettonScore[1] * m_heTimes - userInfoItem->m_Revenue);
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }

        int result = 0;
        if(longValue > huValue)
            result = 2;
        else if(longValue < huValue)
            result = 3;
        else
            result = 1;
		m_nWinIndex = result;
         //m_openRecord.push_back(result);
    {

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            int64_t curJetton = userInfoItem->m_lUserJettonScore[1]+userInfoItem->m_lUserJettonScore[2]+userInfoItem->m_lUserJettonScore[3];
            //统计完以后把本次下注记录为玩家上次下注值
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem||pUserItem->IsAndroidUser()||result==1||curJetton==0)
                continue;

            userInfoItem->lLastJetton = curJetton;
            userInfoItem->lLastWinOrLost = userInfoItem->m_EndWinScore>=0?1:0;

            LOG(ERROR) << "curJetton = " << curJetton << "    lLastWinOrLost ="<<userInfoItem->lLastWinOrLost;
        }
    }
        LOG(INFO) << "OnEventGameConclude chairId = " << chairId;
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
                if(m_pGameRoomInfo->serverStatus !=SERVER_RUNNING|| (pUserItem->GetUserStatus() == sOffline ||
                   (pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < min_score)))
                {
//                    m_JettonScore[userInfoIter->ChairID].clear();
//                    m_WinScore[userInfoIter->ChairID].clear();
//                    InitAddScore(userInfoIter->ChairID);
//                    openLog("11111111 clear userId=%d", userInfoIter->dwUserID);
                    pUserItem->setOffline();
                    clearUserIdVec.push_back(userInfoItem->userId);
                    clearChairIdVec.push_back(userInfoItem->chairId);
//                    userInfoIter = m_UserInfo.erase(userInfoIter);
//                    continue;
                }
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
    int64_t youxiaotouzhu = 0;

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
//            memset(&scoreInfo, 0, sizeof(tagScoreInfo));
			allBetScore = 0;
			userid = pUserItem->GetUserId();
            scoreInfo.clear();
            if(pUserItem->IsAndroidUser())
            {
                totalBetScore = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3];
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[1]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[2]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[3]);
                if(m_nWinIndex==1)
                {
                    youxiaotouzhu = userInfoItem->m_lRobotUserJettonScore[1];
                }
                else
                {
                    youxiaotouzhu = totalBetScore;
                }

            }else
            {
                totalBetScore = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3];
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[1]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[2]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[3]);
				allBetScore = totalBetScore;
                if(m_nWinIndex==1)
                {
                    youxiaotouzhu = userInfoItem->m_lUserJettonScore[1];
                }
                else
                {
                    youxiaotouzhu = totalBetScore;
                }
            }
            if(totalBetScore > 0)
            {

                {
                    scoreInfo.chairId = userInfoItem->chairId;
                    scoreInfo.betScore = totalBetScore;
                    scoreInfo.rWinScore = youxiaotouzhu;
                    scoreInfo.addScore = userInfoItem->m_EndWinScore;
                    scoreInfo.revenue = userInfoItem->m_Revenue;
                    scoreInfo.cardValue = GlobalFunc::converToHex(m_cbCurrentRoundTableCards, 2);  // GetCardValueName(0) + " vs "+GetCardValueName(1);
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

    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        //添加下注信息
        if(pUserItem->IsAndroidUser())
        {
            totalJetton = userInfoItem->m_lRobotUserJettonScore[1] + userInfoItem->m_lRobotUserJettonScore[2] + userInfoItem->m_lRobotUserJettonScore[3];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }else
        {
            totalJetton = userInfoItem->m_lUserJettonScore[1] + userInfoItem->m_lUserJettonScore[2] + userInfoItem->m_lUserJettonScore[3];
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
        if(userInfoItem)
        {
            if(m_ShenSuanZiId == 0)
            {
                m_ShenSuanZiId = userInfoItem->userId;
            }
            else if(userInfoItem->lTwentyWinCount > m_UserInfo[m_ShenSuanZiId]->lTwentyWinCount)
            {
                m_ShenSuanZiId = userInfoItem->userId;
            }
            userInfoItem->isDivineMath=false;
        }
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
    m_endTime = chrono::system_clock::now();
//    if(--m_nPlaceJettonLeveTime < 1)
    {
//        m_TimerLoopThread->getLoop()->cancel(m_TimerIdJetton);
        m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);
//        GameTimerJettonBroadcast();
		m_bGameEnd = true;
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
		m_TimerIdResetPlayerList = m_TimerLoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(CTableFrameSink::RefreshPlayerList, this, true));
        WriteUserScore();
        ChangeUserInfo();
        SendWinSysMessage();

        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_END);
		m_cbGameStatus = GAME_STATUS_END;
        m_replay.clear();
        //删除玩家信息wwwwwww
        clearTableUser();  //sOffline  m_cbPlayStatus==false
        ChangeVipUserInfo();

        ClearTableData();

//        openLog("  ");
//        openLog("end usercoount=%d ",m_UserInfo.size());
        LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();

//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
    }
}

void CTableFrameSink::OnTimerEnd()
{
    //连续5局未下注踢出房间
    //CheckKickOutUser();
	m_bGameEnd = false;
    m_strRoundId = m_pITableFrame->GetNewRoundId();
    m_pITableFrame->SetGameStatus(GAME_STATUS_START);
	m_cbGameStatus = GAME_STATUS_START;
    m_startTime = chrono::system_clock::now();
    m_replay.gameinfoid = m_strRoundId;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	m_strTableId = to_string(m_ipCode) + "-" + to_string(m_pITableFrame->GetTableId() + 1);	//桌号
	m_nWinIndex = 0;
    {

        ClearTableData();
        ChangeVipUserInfo();
        ReadConfigInformation();//重新读取配置
        //结束游戏
        bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
        if(!IsContinue)
            return;

        LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();

         bool bRestart = false;
         if(m_CardVecData.size() < MIN_LEFT_CARDS_RELOAD_NUM)
         {
             m_CardVecData.clear();
             m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
             random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

             //clear open record
             m_openRecord.clear();
             bRestart = true;
			 memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
         }

         shared_ptr<IServerUserItem> pUserItem;
         shared_ptr<UserInfo> userInfoItem;
         for(auto &it : m_UserInfo)
         {
             userInfoItem = it.second;
             pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
             if(!pUserItem)
                 continue;
             if(bRestart)
                SendOpenRecord(userInfoItem->chairId);
             SendStartMsg(userInfoItem->chairId);
         }
        m_dwReadyTime = (uint32_t)time(NULL);
        //设置时间
//        m_TimerJetton.start(1000, bind(&CTableFrameSink::OnTimerJetton, this, std::placeholders::_1), NULL, m_nPlaceJettonLeveTime, false);
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::ClearTableData()
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

//    memset(m_long, 0, sizeof(m_long));
//    memset(m_hu, 0, sizeof(m_hu));
//    memset(m_he, 0, sizeof(m_he));
//    for(int64_t jetton : m_pGameRoomInfo->jettons)
//    {
//        m_Long[jetton] = 0;
//        m_Hu[jetton] = 0;
//        m_He[jetton] = 0;
//    }
    for(int64_t chips : allchips)
    {
        m_Long[chips] = 0;
        m_Hu[chips] = 0;
        m_He[chips] = 0;
    }
    memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));

//    memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
//    memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
//    memset(m_CalculateRevenue, 0, sizeof(m_CalculateRevenue));
//    memset(m_CalculateAgentRevenue, 0, sizeof(m_CalculateAgentRevenue));
//    memset(m_EndWinScore, 0, sizeof(m_EndWinScore));

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;

        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
		// 否则把重押分数记录
		for (int j = 1; j < 4; j++)
		{
			userInfoItem->dLastJetton[j] = userInfoItem->m_lUserJettonScore[j];
		}
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
            m_pITableFrame->SendTableData(wChairID, Longhu::SUB_S_SYNC_TIME, (uint8_t*)data.c_str(), data.size());
        }
        break;
		case SUB_C_USER_REPEAT_JETTON:
		{
			Rejetton(wChairID, wSubCmdID, pData, wDataSize); //续押
			return true;
		}
		case SUB_C_USER_DOUBLEORCANCEL_BET:
		{
			DoubleOrCanceljetton(wChairID, wSubCmdID, pData, wDataSize); //加倍或取消投注
			return true;
		}
        return false;
    }
}

//加注事件
bool CTableFrameSink::OnUserAddScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, bool bReJetton)
{
    //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    if(m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
        LOG_ERROR << " Game Status is not GameStart ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
        return true;
    }
    //=====================================
    bool bFind = false;
    for(int i=0;i < CHIP_COUNT;i++)
    {
        if(m_currentchips[wChairID][i]==lJettonScore)
        {
            bFind = true;
            break;
        }
    }
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<lJettonScore;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 8);
        return true;
    }

    if(cbJettonArea < 1 || cbJettonArea > 3)
    {
        LOG_ERROR << " Can not find jettons Area:"<<cbJettonArea;
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
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();
    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Real User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
            return true;
        }

        if(userInfo->m_lUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
            return true;
        }

        if(lTotalJettonScore == 0)
        {
            m_replay.addPlayer(userId,pIServerUserItem->GetAccount(),lUserScore,wChairID);
        }
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,wChairID,cbJettonArea);
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
    }else //android suer
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2] + userInfo->m_lRobotUserJettonScore[3];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50)
        {
            LOG_ERROR << " Android User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 6);
            return true;
        }
        if(userInfo->m_lRobotUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Android User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 7);
            return true;
        }

        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lRobotserJettonAreaScore[cbJettonArea] += lJettonScore;
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
    }

    if(m_ShenSuanZiId)
    {
        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
        if(SSZUserInfo->chairId == wChairID)
            m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }

    //openLog("userid=%d area=%d score=%f",pIServerUserItem->GetUserID(),cbJettonArea,lJettonScore);
    //成功返回
    if(cbJettonArea == 1)
    {
        m_He[lJettonScore]++;
    }else if(cbJettonArea == 2)
    {
        m_Long[lJettonScore]++;
    }else if(cbJettonArea == 3)
    {
        m_Hu[lJettonScore]++;
    }

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    CMD_S_PlaceJetSuccess Jetton;
    auto push = [&](uint32_t sendChairId, int64_t sendUserId, bool Robot)
    {
        Jetton.Clear();
        Jetton.set_dwuserid(userId);
        Jetton.set_cbjettonarea(cbJettonArea);
        Jetton.set_ljettonscore(lJettonScore);
        bool isRobot = pIServerUserItem->IsAndroidUser();
        Jetton.set_bisandroid(isRobot);

        int64_t xzScore = 0;
        if(isRobot)
            xzScore = userInfo->m_lRobotUserJettonScore[1] + userInfo->m_lRobotUserJettonScore[2] + userInfo->m_lRobotUserJettonScore[3];
        else
            xzScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 1; i < 4; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 1; i < 4; ++i)
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
        m_pITableFrame->SendTableData(sendChairId, Longhu::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };

    if(!isInVipList(wChairID) && userId != m_ShenSuanZiId && m_SortUserInfo.size() >= 8)
    {
        userInfoItem = m_UserInfo[userId];
        push(wChairID, userId, pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }else
    {
        for(auto &it : m_UserInfo)
        {
            if (!it.second)
                continue;

            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem)
                continue;
//            int64_t sendUserId = userInfoItem->userId;
//            uint32_t sendChairId = userInfoItem->chairId;
//            bool Robot = pUserItem->IsAndroidUser();
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}

void CTableFrameSink::AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score, bool bCancel)
{
    assert(chairId<GAME_PLAYER &&areaId<4);

	if (bCancel)
		m_array_jetton[chairId].bJetton = false;
	else
		m_array_jetton[chairId].bJetton = true;
    m_array_jetton[chairId].areaJetton[areaId] +=score;
    m_array_tmparea_jetton[areaId] += score;
}

void CTableFrameSink::GameTimerJettonBroadcast()
{
    CMD_S_Jetton_Broadcast jetton_broadcast;

    bool havejetton = false;
    for(int i = 1; i < 4; ++i)
    {
        if(m_array_tmparea_jetton[i] > 0)
        {
            havejetton = true;
            break;
        }
    }
    if(havejetton)
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

            for (int i = 1; i < 4; ++i)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[i]);
               jetton_broadcast.add_tmpjettonscore(m_array_tmparea_jetton[i] - m_array_jetton[userInfoItem->chairId].areaJetton[i]);
            }
            memset(m_array_jetton[userInfoItem->chairId].areaJetton, 0, sizeof(m_array_jetton[userInfoItem->chairId].areaJetton));
            std::string data = jetton_broadcast.SerializeAsString();
            m_pITableFrame->SendTableData(userInfoItem->chairId, SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
        }
        memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
    }
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    {
        m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::SendPlaceBetFail(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, int32_t errCode)
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
     Jetton.set_cberrorcode(errCode);

     std::string data = Jetton.SerializeAsString();
     m_pITableFrame->SendTableData(wChairID, Longhu::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
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
    uint8_t cbColor = m_GameLogic.GetCardColor(m_cbCurrentRoundTableCards[index]);
    uint8_t cbValue = m_GameLogic.GetCardValue(m_cbCurrentRoundTableCards[index]);
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
//    strFileName.Format(TEXT("./log/%s/%d_%ld_%d"), "longhu", m_pITableFrame->GetTableID(), num, ran);
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
        sprintf(Filename, "./log/longhu/longhu_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
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
            if(userInfoItem->userId == m_ShenSuanZiId)
            {
                m_ShenSuanZiId = 0;
            }
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
	Longhu::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分
	//系统开牌
	for (int i = 0; i < 2; ++i)
	{
		SysCards* card = details.add_syscard();
		card->set_cards(&(m_cbCurrentRoundTableCards)[i], 1);
	}

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		//和龙虎: 1 2 3
		int32_t nWinIndex = 0;
		for (int betIdx = 1;betIdx < 4; betIdx++)
		{
			// 结算时记录
            int64_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->m_lUserJettonScore[betIdx];
			nWinIndex = 0;
			if (betIdx == m_nWinIndex)
			{
				nWinIndex = 1;
			}
			//if (betScore > 0) 
			{
				Longhu::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//本区域是否赢[0:否;1:赢]
				detail->set_dareaiswin(nWinIndex);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
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
	gameMsg["goodroutetype"] = m_nGoodRouteType; //好路类型
	gameMsg["routetypename"] = m_strGoodRouteName; //好路名称

	string gameinfo = writer.write(gameMsg);

	Json::Value details;
	Json::Value countMsg;
	Json::Value routeMsg;

	details["routetype"] = m_nGoodRouteType;//好路类型
	countMsg["t"] = m_winOpenCount[0];//和数量
	countMsg["b"] = m_winOpenCount[1];//龙数量
	countMsg["p"] = m_winOpenCount[2];//虎数量
	//countMsg["tt"] = m_winOpenCount[3];//和对数量
	//countMsg["bt"] = m_winOpenCount[4];//庄对数量
	//countMsg["pt"] = m_winOpenCount[5];//闲对数量
	countMsg["player"] = m_pITableFrame->GetPlayerCount(true);//桌子人数

	details["count"] = countMsg;//各开奖次数
	details["routetypename"] = m_strGoodRouteName; //好路名称
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
	//m_pPlayerInfoList_20.clear();
	//shared_ptr<IServerUserItem> pServerUserItem;
	//for (int i = 0;i < m_pPlayerInfoList.size();i++)
	//{
	//	if (!m_pPlayerInfoList[i])
	//		continue;
	//	pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
	//	if (!pServerUserItem)
	//		continue;
	//	m_pPlayerInfoList[i]->GetTwentyWinScore();
	//	m_pPlayerInfoList[i]->RefreshGameRusult(isGameEnd);
	//}

	//for (int i = 0;i < m_pPlayerInfoList.size();i++)
	//{
	//	if (!m_pPlayerInfoList[i])
	//		continue;
	//	pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
	//	if (!pServerUserItem)
	//		continue;
	//	m_pPlayerInfoList_20.push_back((*m_pPlayerInfoList[i]));
	//	/*LOG(ERROR) << "old二十局赢分:" << (m_pPlayerInfoList[i]->GetTwentyWinScore());
	//	LOG(ERROR) << "二十局赢分:" << (m_pPlayerInfoList_20[i].GetTwentyWinScore());*/
	//}
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
	m_openRecord.push_back(m_nWinIndex);

	m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_nGoodRouteType = m_GameLogic.getGoodRouteType(m_openRecord);
	m_strGoodRouteName = m_GameLogic.getGoodRouteName(m_nGoodRouteType);

	//m_bRefreshPlayerList = true;
	//PlayerRichSorting();
	//游戏记录
	//updateResultList(m_nWinIndex);
	//下注项累计开奖的次数
	m_winOpenCount[m_nWinIndex-1]++;
	/*for (int i = 0;i < 3;i++)
	{
		if (m_cbCardPair[i])
		{
			m_winOpenCount[i + 3]++;
		}
	}*/
	string stsRsult = "";
	for (int i = 0;i < MAX_COUNT;i++)
	{
		stsRsult += to_string(i) + "_" + to_string(m_winOpenCount[i]) + " ";
	}
	openLog("开奖结果累计 stsRsult = %s", stsRsult.c_str());
	//更新开奖结果
	//memcpy(m_notOpenCount_last, m_notOpenCount, sizeof(m_notOpenCount_last));

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
    int64_t areajetton[4] = { 0 };
	int64_t userid = Usr->GetUserId();

	CMD_C_ReJetton reJetton;
	reJetton.ParseFromArray(data, datasize);
	int reSize = reJetton.ljettonscore_size();
	for (int i = 1;i < reSize;i++)
	{
		m_UserInfo[userid]->dLastJetton[i] = reJetton.ljettonscore(i);
		if (m_UserInfo[userid]->dLastJetton[i] < 0)
		{
			LOG(ERROR) << "---续押押分有效性检查出错--->" << i << " " << m_UserInfo[userid]->dLastJetton[i];
		}
	}
	for (int i = 1; i < 4; i++)
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
		placeJetFail.set_cberrorcode(4);
		placeJetFail.set_cbandroid(Usr->IsAndroidUser());

		std::string sendData = placeJetFail.SerializeAsString();
		m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
		LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
		return;
	}

	for (int i = 1; i < 4; i++)
	{
		// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
		if (m_AreaMaxJetton[i] < m_UserInfo[userid]->m_lUserJettonScore[i] + m_UserInfo[userid]->dLastJetton[i])
		{
			CMD_S_PlaceJettonFail placeJetFail;
			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i);
			placeJetFail.set_lplacescore(m_UserInfo[userid]->dLastJetton[i]);
			placeJetFail.set_cberrorcode(5);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());

			std::string data = placeJetFail.SerializeAsString();
			//if (m_bTest==0)
			m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
			LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->m_lUserJettonScore[i] << "+" << m_UserInfo[userid]->dLastJetton[i];
			return;
		}
	}
	// LOG(ERROR) << "---------重押成功--------";
	for (int i = 1; i < 4; i++)
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

//加倍或取消投注
void CTableFrameSink::DoubleOrCanceljetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
	if (!Usr) { LOG(WARNING) << "----DoubleOrCanceljetton 查找玩家不在----"; return; }

	/*if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
	{
		LOG(WARNING) << "-------------加倍或取消投注失败(玩家状态不对)-------------";
		return;
	}*/
	// 游戏状态判断
	if (GAME_STATUS_START != m_cbGameStatus)
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_cbGameStatus << " " << GAME_STATUS_START;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[MAX_COUNT] = { 0 };
	int64_t userid = Usr->GetUserId();

	CMD_C_DoubleOrCancelBet pDoubleOrCancelBet;
	pDoubleOrCancelBet.ParseFromArray(data, datasize);
	int64_t userId = pDoubleOrCancelBet.dwuserid();
    int64_t betType = pDoubleOrCancelBet.ibettype();
	if (userId != Usr->GetUserId())
	{
		LOG(WARNING) << "----DoubleOrCanceljetton 传过来的玩家不对 userId:  " << userId;
		return;
	}
	switch (betType)
	{
	case 0://取消所有下注
	{
		LOG(WARNING) << "-----------DoubleOrCanceljetton 取消投注-------------";
		if (!m_bUseCancelBet)
			return;
		for (int i = 1; i < 4; i++)
		{
			if (areajetton[i] <= 0)
				continue;
			while (areajetton[i] > 0)
			{
				int jSize = CHIP_COUNT;
				for (int j = jSize - 1;j >= 0;j--)
				{
					if (areajetton[i] >= m_currentchips[chairid][j])
					{
                        int64_t tmpVar = m_currentchips[chairid][j];
						OnUserCancelPlaceScore(chairid, i, tmpVar, true);
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
		break;
	}
	case 1://当前已下注翻倍
	{
		LOG(WARNING) << "-----------DoubleOrCanceljetton 加倍投注-------------";
		bool bHaveBet = false;
		for (int i = 1; i < 4; i++)
		{
			alljetton += m_UserInfo[userid]->m_lUserJettonScore[i];
			areajetton[i] = m_UserInfo[userid]->m_lUserJettonScore[i];
			LOG(WARNING) << "--dhaveBetJetton-" << areajetton[i];
			if (areajetton[i] < 0)
			{
				alljetton = 0;
				break;
			}
			else if (areajetton[i] > 0)
			{
				bHaveBet = true;
			}
		}
		if (!bHaveBet)
		{
			LOG(WARNING) << "-----------DoubleOrCanceljetton 玩家未投注不能加倍 -------------";
			return;
		}

		// 加倍投注失败
		if (alljetton == 0 /*|| Usr->GetUserScore() < alljetton * 2*/)
		{
			CMD_S_PlaceJettonFail placeJetFail;

			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(3);
			placeJetFail.set_lplacescore(0);
			placeJetFail.set_cberrorcode(4);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());

			std::string sendData = placeJetFail.SerializeAsString();
			m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
			LOG(WARNING) << "------------------加倍投注失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
			return;
		}

		for (int i = 1; i < 4; i++)
		{
			// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
			if (m_AreaMaxJetton[i] < m_UserInfo[userid]->m_lUserJettonScore[i] + areajetton[i])
			{
				//CMD_S_PlaceJettonFail placeJetFail;
				//placeJetFail.set_dwuserid(userid);
				//placeJetFail.set_cbjettonarea(i);
				//placeJetFail.set_lplacescore(m_UserInfo[userid]->m_lUserJettonScore[i]);
				//placeJetFail.set_cberrorcode(5);
				//placeJetFail.set_cbandroid(Usr->IsAndroidUser());
				//std::string data = placeJetFail.SerializeAsString();
				////if (m_bTest==0)
				//m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
				areajetton[i] = 0;
				LOG(WARNING) << "---------加倍投注失败(超过区域最大押分)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->m_lUserJettonScore[i] << "+" << areajetton[i] << "  area: " << i;
				//return;
			}
		}
		// LOG(ERROR) << "---------加倍投注成功--------";
		for (int i = 1; i < 4; i++)
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
			}
		}
		break;
	}
	default:
		LOG(ERROR) << "----DoubleOrCanceljetton 不存在这种操作 betType:  " << betType;
		break;
	}
}

//取消下注事件
bool CTableFrameSink::OnUserCancelPlaceScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, bool bCancelJetton)
{
	//openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
	if (m_cbGameStatus != GAME_STATUS_START)
	{
		LOG_ERROR << " Game Status is not GameStart ";
		//SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
		return true;
	}
	//=====================================
	bool bFind = false;
	/*if (bCancelJetton)
	{
		int leng = m_currentchips[chairid].size();
		for (int i = 0;i < leng;i++)
		{
			if (m_currentchips[chairid][i] == lJettonScore)
			{
				bFind = true;
				break;
			}
		}
	}
	else*/
	{
		for (int i = 0;i < CHIP_COUNT;i++)
		{
			if (m_currentchips[wChairID][i] == lJettonScore)
			{
				bFind = true;
				break;
			}
		}
	}
	if (!bFind)
	{
		LOG_ERROR << " Jettons Error:" << lJettonScore;
		//SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 8);
		return true;
	}

	if (cbJettonArea < 1 || cbJettonArea > 3)
	{
		LOG_ERROR << " Can not find jettons Area:" << cbJettonArea;
		//SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 2);
		return true;
	}
	//======================================
	//校验是否当前用户
	shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
	if (!pIServerUserItem)
	{
		LOG_ERROR << " User is not Valid ";
		//SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 3);
		return true;
	}
	int64_t userId = pIServerUserItem->GetUserId();
	shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

	//玩家积分
	int64_t lUserScore = pIServerUserItem->GetUserScore();
	int64_t lCurrentPlayerJetton = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
	if (lJettonScore > 0 && lCurrentPlayerJetton > 0 && userInfo->m_lUserJettonScore[cbJettonArea] >= lJettonScore)  //real user
	{
		//已经下的总注
		int64_t lTotalJettonScore = lCurrentPlayerJetton;//userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
		/*if (lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
		{
			LOG_ERROR << " Real User Score is not enough ";
			SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
			return true;
		}

		if (userInfo->m_lUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
		{
			LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
			SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
			return true;
		}*/
		m_lAllJettonScore[cbJettonArea] -= lJettonScore;
		userInfo->m_lUserJettonScore[cbJettonArea] -= lJettonScore;
		//userInfo->lCurrentPlayerJetton -= lJettonScore;
		if (!pIServerUserItem->IsAndroidUser())
		{
			m_lTrueUserJettonScore[cbJettonArea] -= lJettonScore;
			m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(lJettonScore), -1, opAddBet, wChairID, cbJettonArea);
		}

	}

	if (m_ShenSuanZiId)
	{
		shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
		if (SSZUserInfo->chairId == wChairID && m_lShenSuanZiJettonScore[cbJettonArea] >= lJettonScore)
			m_lShenSuanZiJettonScore[cbJettonArea] -= lJettonScore;
	}
	LOG(INFO) << "取消下注成功:userid = " << userId << " area = " << (int)cbJettonArea << " score = " << lJettonScore;
	openLog("取消下注:userid=%d area=%d score=%f", userId, cbJettonArea, lJettonScore);
	//成功返回
	if (lJettonScore > 0 && userInfo->m_lUserJettonScore[cbJettonArea] >= lJettonScore)  //real user
	{
		if (cbJettonArea == 1)
		{
			if (m_He[lJettonScore] > 0)
				m_He[lJettonScore]--;
		}
		else if (cbJettonArea == 2)
		{
			if (m_Long[lJettonScore] > 0)
				m_Long[lJettonScore]--;
		}
		else if (cbJettonArea == 3)
		{
			if (m_Hu[lJettonScore] > 0)
				m_Hu[lJettonScore]--;
		}
	}
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;

	CMD_S_CanclePlaceJet pCanclePlaceJet;
	auto push = [&](uint32_t sendChairId, int64_t sendUserId, bool Robot)
	{
		pCanclePlaceJet.Clear();
		pCanclePlaceJet.set_dwuserid(userId);
		pCanclePlaceJet.set_cbjettonarea(cbJettonArea);
		pCanclePlaceJet.set_ljettonscore(lJettonScore);
		bool isRobot = pIServerUserItem->IsAndroidUser();
		pCanclePlaceJet.set_bisandroid(isRobot);

		int64_t xzScore = userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];;
		pCanclePlaceJet.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* areaJetton = pCanclePlaceJet.add_alljettonscore();
			areaJetton->set_dwuserid(0);
			areaJetton->set_ljettonarea(i);
			areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
		}
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* userJetton = pCanclePlaceJet.add_selfjettonscore();
			userJetton->set_dwuserid(sendUserId);
			userJetton->set_ljettonarea(i);
			int64_t jettonScore = 0;
			jettonScore = userInfoItem->m_lUserJettonScore[i];
			userJetton->set_ljettonscore(jettonScore);
		}
		std::string data = pCanclePlaceJet.SerializeAsString();
		m_pITableFrame->SendTableData(sendChairId, SUB_S_CANCELPLACE_JET, (uint8_t*)data.c_str(), data.size());
	};

	if (!isInVipList(wChairID) && userId != m_ShenSuanZiId && m_SortUserInfo.size() >= 8)
	{
		userInfoItem = m_UserInfo[userId];
		push(wChairID, userId, pIServerUserItem->IsAndroidUser());
		AddPlayerJetton(wChairID, cbJettonArea, lJettonScore, true);
	}
	else
	{
		for (auto &it : m_UserInfo)
		{
			if (!it.second)
				continue;

			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem)
				continue;
			push(userInfoItem->chairId, userInfoItem->userId, pUserItem->IsAndroidUser());
		}
	}

	return true;
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

//             memset(m_long, 0, sizeof(m_long));
//             memset(m_hu, 0, sizeof(m_hu));
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
//    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Longhu::SUB_S_QUERY_PLAYLIST, (uint8_t*)data.c_str(), data.size());
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
