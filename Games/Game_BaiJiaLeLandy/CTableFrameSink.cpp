#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
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

#include "proto/bjl.Message.pb.h"


#include <muduo/base/Mutex.h>

#include "BjlAlgorithm.h"

#include "json/json.h"

using namespace bjl;
using namespace std;

#include "CTableFrameSink.h"

#define MIN_LEFT_CARDS_RELOAD_NUM  24
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0

#define TEST	false

// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/bjl/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("bjl");
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

        if(access("./log/bjl", 0) == -1)
           mkdir("./log/bjl",0777);
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
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "BjlTimerEventLoopThread")
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));
    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

    m_strRoundId = "";
	m_strResultMsg = "";
	m_ZhuangTimes = 0;
	m_XianTimes = 0;
	m_heTimes = 0;
	m_ZhuangPairTimes = 0;
	m_XianPairTimes = 0;
	m_hePairTimes = 0;

    m_Zhuang.clear();
    m_Xian.clear();
    m_He.clear();
	m_ZhuangPair.clear();
	m_XianPair.clear();
	m_HePair.clear();

    memset(m_cbCardCount, 0, sizeof(m_cbCardCount));
	memset(m_cbTableCards, 0, sizeof(m_cbTableCards));
	memset(m_cbCardPoint, 0, sizeof(m_cbCardPoint));
	memset(m_cbCardPair, 0, sizeof(m_cbCardPair));
    memset(m_cbCardTypeCount, 0, sizeof(m_cbCardTypeCount));
    m_IsEnableOpenlog = false;
    m_IsXuYaoRobotXiaZhu = false;
    m_WinScoreSystem = 0;
    m_nPlaceJettonTime = 0;
	m_nWashCardTime = 0;
	m_nSendCardTime = 0;
	m_fUpdateGameMsgTime = 0;
    m_nGameTimeJettonBroadcast = 0;
    m_nGameEndTime = 0;
	m_fBupaiTime = 0;
	m_fOpenCardTime = 0;
    m_iShenSuanZiUserId = 0;
    m_UserInfo.clear();
    m_SortUserInfo.clear();
    m_openRecord.clear();
    m_CardVecData.clear();
	m_vecData.clear();
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
        m_currentchips[i][5]=10000;
        m_currentchips[i][6]=50000;
    }
    m_nWinIndex = -1;
	m_fRetMul = 0;
	for (int i = 0;i < MAX_COUNT;i++)
	{
		m_fMuilt[i] = 0;
	}
	m_cbGameStatus = BJL_GAME_STATUS_INIT;
	m_bGameEnd = false;
	m_bRefreshPlayerList = false;
	m_fResetPlayerTime = 5.0f;
	m_pPlayerInfoList.clear();
	m_pPlayerInfoList_20.clear();
	memset(m_retWeight, 0, sizeof(m_retWeight));
	memset(m_notOpenCount, 0, sizeof(m_notOpenCount));
	memset(m_notOpenCount_last, 0, sizeof(m_notOpenCount_last));
	memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
	memset(m_lLimitScore, 0, sizeof(m_lLimitScore));
	memset(m_lLimitQiePai, 0, sizeof(m_lLimitQiePai));
	m_NotBetJettonKickOutCount = 0;
	m_nCardFirstDeleteCount = 0;

	m_strTableId = "";							//桌号
	m_nSendCardIndex = 0;						//当前发第几张牌
	m_nLeftCardCount = 0;						//剩余牌数
	m_nCutCardId = 0;							//切牌id
	m_nJuHaoId = 0;								//第几局
	m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_ipCode = 0;
	m_bTestGame = false;
	m_bUseCancelBet = false;

	memset(m_dAreaTestJetton, 0, sizeof(m_dAreaTestJetton));
	memset(m_dAreaWinJetton, 0, sizeof(m_dAreaWinJetton));
	m_dCurAllBetJetton = 0;
	m_dCurAllWinJetton = 0;
	m_nTestJushu = 0;
	m_nTestTime = 0;
	m_nTestSameJushu = 0;
	m_nTestChangJushu = 0;
	
	ReadConfigInformation();
	memset(m_cbTestBuCard, 0, sizeof(m_cbTestBuCard));
	loadConf();
	//CalculatWeight();

    return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
}

void CTableFrameSink::loadConf()
{
	m_TestCards.clear();
	string strfile = "./conf/bjl_case.conf";

	if (boost::filesystem::exists(strfile))
	{
		boost::property_tree::ptree pt;
		boost::property_tree::read_json(strfile, pt);
		boost::property_tree::ptree::iterator it0;
		boost::property_tree::ptree element;

		int idx = 0;
		for (boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it)
		{
            vector<uint8_t> cards(3);
			element = pt.get_child(it->first);
			idx = 0;
			for (it0 = element.begin();it0 != element.end(); it0++)
			{
				string str = it0->second.get_value<string>();
				cards[idx] = strtol(str.c_str(), nullptr, 16);
				idx++;
			}
			m_TestCards.insert(pair<int32_t, vector<uint8_t>>(stoi(it->first), cards));
		}
	}
	else
	{
		m_bTestGame = false;
	}
}


//读取配置
bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/bjl_config.ini"))
    {
        LOG_INFO << "./conf/bjl_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/bjl_config.ini", pt);

    stockWeak=  pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    //Time
    m_nPlaceJettonTime		= pt.get<int32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
	m_nGameEndTime			= pt.get<int32_t>("GAME_CONFIG.TM_GAME_END", 7);
	m_nWashCardTime			= pt.get<int32_t>("GAME_CONFIG.TM_GAME_WASHCARD", 5);
	m_nSendCardTime			= pt.get<int32_t>("GAME_CONFIG.TM_GAME_SENDCARD", 5);
	m_fUpdateGameMsgTime	= pt.get<float>("GAME_CONFIG.TM_UpdateGameMsgTime", 1.0);
	m_fBupaiTime			= pt.get<float>("GAME_CONFIG.TM_BUPAI", 1.5f);
	m_fOpenCardTime			= pt.get<float>("GAME_CONFIG.TM_OPENCARD", 2.0f);
    m_nGameTimeJettonBroadcast= pt.get<int32_t>("GAME_CONFIG.TM_JETTON_BROADCAST", 1);
	

    m_ZhuangTimes		 = pt.get<double>("GAME_CONFIG.AREA_ZHUANG", 0.95);
    m_XianTimes			 = pt.get<double>("GAME_CONFIG.AREA_XIAN", 1.0);
    m_heTimes			 = pt.get<double>("GAME_CONFIG.AREA_HEE", 8.0);
	m_ZhuangPairTimes	 = pt.get<double>("GAME_CONFIG.AREA_ZHUANGPAIT", 11.0);
	m_XianPairTimes		 = pt.get<double>("GAME_CONFIG.AREA_XIANPAIT", 11.0);
	m_hePairTimes		 = pt.get<double>("GAME_CONFIG.AREA_HEEPAIT", 75.0);

	vector<double> vfMuilt;
	vfMuilt.clear();
	vfMuilt = to_array<double>(pt.get<string>("GAME_CONFIG.AREA_FMUILT","8.0,0.95,1.0,75.0,11.0,11.0"));

	vector<int32_t> nWeight;
	nWeight.clear();
	nWeight = to_array<int32_t>(pt.get<string>("GAME_CONFIG.Area_Weight", "860,3874,3874,100,646,646"));
	for (int i=0;i<MAX_COUNT;i++)
	{
		m_fMuilt[i] = vfMuilt[i];
		m_retWeight[i] = nWeight[i];
	}


    m_lLimitPoints       = pt.get<int64_t>("GAME_CONFIG.LimitPoint", 500000);
    m_dControlkill       = pt.get<double>("GAME_CONFIG.Controlkill", 0.35);

	m_lLimitScore[0]	 = pt.get<int64_t>("GAME_AREA_MAX_JETTON.LimitScoreMin", 100);
	m_lLimitScore[1]     = pt.get<int64_t>("GAME_AREA_MAX_JETTON.LimitScoreMax", 2000000);
    m_AreaMaxJetton[0]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.HE", 1000000);
    m_AreaMaxJetton[1]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.ZHUANG", 5000000);
    m_AreaMaxJetton[2]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.XIAN", 5000000);
	m_AreaMaxJetton[3]	 = pt.get<int64_t>("GAME_AREA_MAX_JETTON.HEPAIR", 500000);
	m_AreaMaxJetton[4]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.ZHUANGPAIR", 350000);
	m_AreaMaxJetton[5]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.XIANPAIR", 350000);

    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);
    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog", 0);
    m_WinScoreSystem     = pt.get<int64_t>("GAME_CONFIG.WinScore", 500);
    m_bOpenContorl       = pt.get<bool>("GAME_CONFIG.Control", true);
	m_fResetPlayerTime	 = pt.get<float>("GAME_CONFIG.TM_RESET_PLAYER_LIST", 5.0);

	m_lLimitQiePai[0]	 = pt.get<int32_t>("GAME_CONFIG.LimitQiePaiMin", 78);
	m_lLimitQiePai[1]	 = pt.get<int32_t>("GAME_CONFIG.LimitQiePaiMax", 156);

	m_NotBetJettonKickOutCount = pt.get<int32_t>("GAME_CONFIG.NotBetJettonKickOut", 5);

	m_nCardFirstDeleteCount = pt.get<int32_t>("GAME_CONFIG.CardFirstDeleteCount", 6);

	int nTestGame		 = pt.get<float>("GAME_CONFIG.TESTGAME", 0);
	if (nTestGame==1)
		m_bTestGame = true;
	else
		m_bTestGame = false;
	
	int nUseCancelBet = pt.get<float>("GAME_CONFIG.USECANCELBET", 0);
	if (nUseCancelBet == 1)
		m_bUseCancelBet = true;
	else
		m_bUseCancelBet = false;

    vector<int64_t> scorelevel;
    vector<int64_t> chips;

    m_cbJettonMultiple.clear();
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
        m_cbJettonMultiple.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear2","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[1][i]=chips[i];
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),chips[i]);
        if(it!=m_cbJettonMultiple.end())
        {
        }
        else
        {
            m_cbJettonMultiple.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear3","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[2][i]=chips[i];
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),chips[i]);
        if(it!=m_cbJettonMultiple.end())
        {
        }
        else
        {
            m_cbJettonMultiple.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000,10000,50000"));
    for(int i=0;i<7;i++)
    {
        m_userChips[3][i]=chips[i];
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),chips[i]);
        if(it!=m_cbJettonMultiple.end())
        {
        }
        else
        {
            m_cbJettonMultiple.push_back(chips[i]);
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
	sort(m_cbJettonMultiple.begin(), m_cbJettonMultiple.end());
    for(int64_t chips : m_cbJettonMultiple)
    {
        m_Zhuang[chips] = 0;
        m_Xian[chips] = 0;
        m_He[chips] = 0;
		m_ZhuangPair[chips] = 0;
		m_XianPair[chips] = 0;
		m_HePair[chips] = 0;
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
		updateGameMsg(true);
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
    LOG(INFO) << __FILE__ << " " << __FUNCTION__;
    bool canLeft = true;
    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG(INFO) << " pIServerUserItem==NULL Left UserId:"<<userId;
        return true;
    }
	else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;
        int64_t totalxz = 0;
        uint8_t status = m_cbGameStatus; //m_pITableFrame->GetGameStatus();
        if(BJL_GAME_STATUS_PLACE_JETTON == status)
        {
            totalxz = userInfo->lCurrentPlayerJetton;
        }
        if(totalxz > 0)
            canLeft = false;
    }
    return canLeft;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool isLookon)
{
    LOG(INFO) << __FILE__ << " " << __FUNCTION__;
    shared_ptr<CServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL userId:"<<userId;
        return false;
    }
    LOG(INFO) << "OnUserEnter userId:"<<userId;
    uint32_t chairId = pIServerUserItem->GetChairId();
	auto it = m_UserInfo.find(userId);
	if (m_UserInfo.end() == it)
	{
		shared_ptr<UserInfo> userInfo(new UserInfo());
		userInfo->userId = userId;
		userInfo->chairId = chairId;
		userInfo->headerId = pIServerUserItem->GetHeaderId();
		userInfo->nickName = pIServerUserItem->GetNickName();
		userInfo->location = pIServerUserItem->GetLocation();
		userInfo->Isplaying = true;
		userInfo->isAndroidUser = pIServerUserItem->IsAndroidUser();
		m_UserInfo[userId] = userInfo;
	}
	pIServerUserItem->SetUserStatus(sPlaying);
	/*int typeID[12] = { 2,1,1,1,2,1,1,0,0,2,1,1};
	for (int i = 0;i < 12;i++)
	{
		m_openRecord.push_back(typeID[i]);
	}
	m_nGoodRouteType = m_GameLogic.getGoodRouteType(m_openRecord);*/
    if(GAME_STATUS_INIT == m_cbGameStatus)
    {
		m_BjlAlgorithm.SetRoomMsg(m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
        m_iShenSuanZiUserId = pIServerUserItem->GetUserId();
        m_startTime = chrono::system_clock::now();
		m_gameStateStartTime = chrono::system_clock::now();
        m_strRoundId = m_pITableFrame->GetNewRoundId();
		m_replay.gameinfoid = m_strRoundId;

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
		m_nCutCardId = m_random.betweenInt(m_lLimitQiePai[0], m_lLimitQiePai[1]).randInt_mt(true); //1.5副到3副牌之间
		m_nSendCardIndex = 0;
		m_nLeftCardCount = MAX_CARDS;
		m_nJuHaoId = 0;
        m_dwReadyTime = (uint32_t)time(NULL);
		// 首次开始,先洗牌,然后发牌
		m_TimerIdWashCard = m_TimerLoopThread->getLoop()->runAfter(m_nWashCardTime, boost::bind(&CTableFrameSink::OnTimerSendCard, this));
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
		m_cbGameStatus = BJL_GAME_STATUS_WASHCARD;
        m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
		memset(m_notOpenCount, 0, sizeof(m_notOpenCount));
		memset(m_notOpenCount_last, 0, sizeof(m_notOpenCount_last));
		//updateGameMsg();
		if (TEST)
		{
			
			int64_t curBet = 0;
			int64_t curWin = 0;
			int8_t cardPair[3] = { 0 };
			string stsRsult = "";
			JudgmentIsValidAllCard();
			for (int time = 0;time < 30;time++)
			{
				m_nTestTime = time;
				m_nTestJushu = 0;
				m_nTestSameJushu = 0;
				m_nTestChangJushu = 0;
				m_dCurAllBetJetton = 0;
				m_dCurAllWinJetton = 0;
				memset(m_dAreaTestJetton, 0, sizeof(m_dAreaTestJetton));
				memset(m_dAreaWinJetton, 0, sizeof(m_dAreaWinJetton));
				m_dAreaTestJetton[2] = 100;
				//m_dAreaTestJetton[5] = 100;
				memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
                for (int i = 0;i < 10000;i++)
				{
					LOG(INFO) << "OnUserEnter GetOpenCards i = " << i;
					m_nTestJushu++;
					curBet = 0;
					for (int j = 0; j < MAX_COUNT; j++)
					{
						curBet += m_dAreaTestJetton[j];
						if (m_dAreaTestJetton[j] > 0)
						{
							openLog("第[%d]局,押注区域:%d,押注:%d ", m_nTestJushu, j, m_dAreaTestJetton[j]);
						}
					}
					m_dCurAllBetJetton += curBet;
					openLog("第[%d]局开始,本局押注:%d,累计总押注:%d ", m_nTestJushu, curBet, m_dCurAllBetJetton);
					GetOpenCards();
					for (int j = 0;j < 3;j++)
					{
						cardPair[j] = m_cbCardPair[j];
					}
					for (int j = 0; j < MAX_COUNT; j++)
						m_BjlAlgorithm.SetBetting(j, m_dAreaTestJetton[j]);

					curWin = m_BjlAlgorithm.CalculatTestWinScore(m_nWinIndex, cardPair);
					m_dCurAllWinJetton += curWin;
					//下注项累计开奖的次数
					m_winOpenCount[m_nWinIndex]++;
					for (int j = 0;j < 3;j++)
					{
						if (m_cbCardPair[j])
						{
							m_winOpenCount[j + 3]++;
						}
					}
					stsRsult = "";
					for (int j = 0;j < MAX_COUNT;j++)
					{
						stsRsult += to_string(j) + "_" + to_string(m_winOpenCount[j]) + " ";
					}
					openLog("开奖结果累计 stsRsult = %s", stsRsult.c_str());
					openLog("第[%d]局结束,本局押注:%d,本局赢分:%d,累计总押注:%d,累计总赢:%d,总进-总出 = %d \n", m_nTestJushu, curBet, curWin, m_dCurAllBetJetton, m_dCurAllWinJetton, m_dCurAllBetJetton - m_dCurAllWinJetton);
				}
			}
			
			return true;
		}
		else
		{
			m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, CALLBACK_0(CTableFrameSink::updateGameMsg, this, false));
		}
		//去掉前面6涨
		for (int i = 0; i < m_nCardFirstDeleteCount; ++i)
		{
			m_CardVecData.erase(m_CardVecData.begin());
			m_nSendCardIndex++;
			m_nLeftCardCount--;
		}

		LOG(INFO) << "游戏首次开始:m_strRoundId=" << m_strRoundId << ",进入洗牌状态：" << m_cbGameStatus;
		openLog("游戏首次开始:m_strRoundId=%s,m_strTableId=%s,m_nLeftCardCount=%d,m_nCutCardId=%d", m_strRoundId.c_str(), m_strTableId.c_str(), m_nLeftCardCount, m_nCutCardId);
		openLog("本局[%d]游戏开始洗牌:m_strRoundId=%s,m_nCutCardId=%d", m_nJuHaoId, m_strRoundId.c_str(), m_nCutCardId);
    }
    for(int i=0;i<7;i++)
    {
        if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[0] && pIServerUserItem->GetUserScore()< m_userScoreLevel[1])
        {
            m_currentchips[chairId][i]=m_userChips[0][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[1] && pIServerUserItem->GetUserScore()< m_userScoreLevel[2])
        {
            m_currentchips[chairId][i]=m_userChips[1][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[2] && pIServerUserItem->GetUserScore()< m_userScoreLevel[3])
        {
            m_currentchips[chairId][i]=m_userChips[2][i];
        }
        else
        {
            m_currentchips[chairId][i]=m_userChips[3][i];
        }
    }

	for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
	{
		if ((*it)->userId == userId)
		{
			m_pPlayerInfoList.erase(it);
			m_pPlayerInfoList.push_back(m_UserInfo[userId]);
			break;
		}
	}
	for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
	{
		if ((*it).userId == userId)
		{
			m_pPlayerInfoList_20.erase(it);
			m_pPlayerInfoList_20.push_back((*m_UserInfo[userId]));
			break;
		}
	}
	openLog("OnUserEnter chairId:%d, userId:%d, m_strRoundId=%s, m_strTableId=%s ", chairId, userId, m_strRoundId.c_str(), m_strTableId.c_str());
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
    int cbGameStatus = m_cbGameStatus; //m_pITableFrame->GetGameStatus();
    int64_t userId = pIServerUserItem->GetUserId();
    LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << (int)cbGameStatus;
    switch(cbGameStatus)
    {
		case BJL_GAME_STATUS_WASHCARD:		//洗牌状态
		case BJL_GAME_STATUS_SENDCARD:		//发牌状态
		case BJL_GAME_STATUS_PLACE_JETTON:	//下注状态
        case BJL_GAME_STATUS_START:			//游戏状态
        {
            SendOpenRecord(chairId);
			SendGameStateMsg(chairId);
            return true;
			break;
        }
        case BJL_GAME_STATUS_END:
        {
            SendOpenRecord(chairId);
            //SendEndMsg(chairId);
			SendGameStateEnd(chairId);
			return true;
			break;
        }
    }
    //效验错误
    assert(false);
    return false;
}
// 查询当前游戏状态
void CTableFrameSink::QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
	if (!player)
		return;
	bjl::CMD_C_CurState curStateReq;
	curStateReq.ParseFromArray(data, datasize);

	int32_t userid = curStateReq.dwuserid();

	LOG(WARNING) << "**********查询当前游戏状态**********" << userid ;

	//OnEventGameScene(chairId, false);
}
void CTableFrameSink::SendOpenRecord(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " SendOpenRecord pIServerUserItem==NULL chairId:"<<chairId;
        return;
    }
	uint8_t cbGameStatus = m_cbGameStatus; //m_pITableFrame->GetGameStatus();
    uint32_t size = m_openRecord.size();
	/*if(cbGameStatus != BJL_GAME_STATUS_END)
	{
		size = m_openRecord.size();
	}
	else
	{
		if(m_openRecord.size() > 0)
			size = m_openRecord.size() - 1;
	}*/

    CMD_S_GameRecord record;

	for (int i = 0; i < MAX_COUNT; ++i)
	{
		record.add_winindexcount(m_winOpenCount[i]);
	}
    for(int i = 0; i < size; ++i)
    {
        record.add_record(m_openRecord[i]);
    }

    std::string data = record.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, bjl::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendGameStateMsg(uint32_t chairId)
{
	shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
	int64_t userId = pIServerUserItem->GetUserId();
	shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

	CMD_Scene_StatusStart pGameStateMsg;

	pGameStateMsg.set_ncurstate(m_cbGameStatus);

	GameDeskInfo* pInfo = pGameStateMsg.mutable_deskdata();
	for (int i = 0;i < 7;i++)
	{
		pInfo->add_userchips(m_currentchips[chairId][i]);
	}
	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
		alljitton->set_dwuserid(0);
		alljitton->set_ljettonscore(m_lAllJettonScore[i]);
		alljitton->set_ljettonarea(i);
	}

	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
		userjetton->set_dwuserid(userId);
		userjetton->set_ljettonarea(i);
		userjetton->set_ljettonscore(userInfo->dAreaJetton[i]);
	}

	//divineJettonScore
	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
		divineJetton->set_dwuserid(m_iShenSuanZiUserId);
		shared_ptr<IServerUserItem> pUserItem = m_pITableFrame->GetUserItemByUserId(m_iShenSuanZiUserId);
		if (pUserItem)
			divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
		else
			divineJetton->set_ljettonscore(0);
		divineJetton->set_ljettonarea(i);
	}

	pInfo->set_lapplybankercondition(0);
	pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);

	uint32_t n = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &userInfoItem : m_SortUserInfo)
	{
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			nowScore = 0;
		else
		{
			xzScore = userInfoItem->lCurrentPlayerJetton;
			nowScore = pUserItem->GetUserScore() - xzScore;
		}
		PlayerListItem* palyer = pInfo->add_players();
		palyer->set_dwuserid(userInfoItem->userId);
		palyer->set_headerid(userInfoItem->headerId);
		palyer->set_nickname(userInfoItem->nickName);
		palyer->set_luserscore(nowScore);
		palyer->set_ltwentywinscore(userInfoItem->lTwentyWinScore);
		palyer->set_ltwentywincount(userInfoItem->lTwentyWinCount);
		int shensuanzi = userInfoItem->userId == m_iShenSuanZiUserId ? 1 : 0;
		palyer->set_isdivinemath(shensuanzi);
		palyer->set_index(n + 1);
		if (shensuanzi == 1)
			palyer->set_index(0);
		palyer->set_isbanker(0);
		palyer->set_isapplybanker(0);
		palyer->set_applybankermulti(0);
		palyer->set_ljetton(0);
		palyer->set_szlocation(userInfoItem->location);

		if (++n > size)
			break;
	}

	pInfo->set_wbankerchairid(0);
	pInfo->set_cbbankertimes(0);
	pInfo->set_lbankerwinscore(0);
	pInfo->set_lbankerscore(0);
	pInfo->set_benablesysbanker(true);
	pInfo->set_szgameroomname("bjl");
	
	for (auto &jetton : m_He)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(0);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_Zhuang)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(1);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_Xian)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(2);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_HePair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(3);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_ZhuangPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(4);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_XianPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(5);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}

	for (int i = 0;i < MAX_COUNT;i++)
	{
		pInfo->add_notopencount(m_notOpenCount_last[i]);
	}
	//限红
	for (int i = 0;i < 2;i++)
	{
		pInfo->add_llimitscore(m_lLimitScore[i]);
	}
	pInfo->set_tableid(m_strTableId);
	pInfo->set_nleftcardcount(m_nLeftCardCount);
	pInfo->set_ncutcardid(m_nCutCardId);
	pInfo->set_njuhaoid(m_nJuHaoId+1);

	pInfo->set_lonlinepopulation(m_UserInfo.size());

	int32_t allTime = 0;
	if (m_cbGameStatus == 1) // 洗牌时间
		allTime = m_nWashCardTime;
	else if (m_cbGameStatus == 2) //发牌时间
		allTime = m_nSendCardTime;
	else if (m_cbGameStatus == 3) //下注时间
		allTime = m_nPlaceJettonTime;
	else //结束时间
	{
		allTime = m_nGameEndTime;
		//补牌时间
		float bupaiTime = 0;
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			bupaiTime += m_fBupaiTime;
		}
		if (m_cbCardCount[INDEX_PLAYER] == 3)
		{
			bupaiTime += m_fBupaiTime;
		}
		allTime += ceil(bupaiTime);
	}

	pGameStateMsg.set_roundid(m_strRoundId);
	pGameStateMsg.set_cbplacetime(allTime);
	chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
	int32_t leaveTime = allTime - durationTime.count();
	pGameStateMsg.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

	xzScore = userInfo->lCurrentPlayerJetton;
	nowScore = pIServerUserItem->GetUserScore() - xzScore;
	int64_t maxscore = nowScore >= 50 ? nowScore : 0;
	pGameStateMsg.set_lusermaxscore(maxscore);
	pGameStateMsg.set_luserscore(nowScore);
	pGameStateMsg.set_notbetallcount(m_NotBetJettonKickOutCount);
	pGameStateMsg.set_curnotbetcount(userInfo->NotBetJettonCount);

	std::string data = pGameStateMsg.SerializeAsString();
	m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), bjl::SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
}

void CTableFrameSink::SendGameStateEnd(uint32_t chairId)
{
	shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
	int64_t userId = pIServerUserItem->GetUserId();
	shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

	CMD_S_StatusEnd gameStatusEnd;
	GameDeskInfo* pInfo = gameStatusEnd.mutable_deskdata();
	for (int i = 0;i < 7;i++)
	{
		pInfo->add_userchips(m_currentchips[chairId][i]);
	}
	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
		alljitton->set_dwuserid(0);
		alljitton->set_ljettonarea(i);
		alljitton->set_ljettonscore(m_lAllJettonScore[i]);
	}

	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
		userjetton->set_dwuserid(userId);
		userjetton->set_ljettonarea(i);
		userjetton->set_ljettonscore(userInfo->dAreaJetton[i]);
	}
	//divineJettonScore
	for (int i = 0; i < MAX_COUNT; ++i)
	{
		PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
		divineJetton->set_dwuserid(m_iShenSuanZiUserId);
		shared_ptr<IServerUserItem> pUserItem;
		pUserItem = m_pITableFrame->GetUserItemByUserId(m_iShenSuanZiUserId);
		if (pUserItem)
			divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
		else
			divineJetton->set_ljettonscore(0);
		divineJetton->set_ljettonarea(i);
	}

	pInfo->set_lapplybankercondition(0);
	pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);

	bool isVIP = false;
	uint32_t n = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	shared_ptr<IServerUserItem> pUserItem;
	for (auto &userInfoItem : m_SortUserInfo)
	{
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
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
		int shensuanzi = userInfoItem->userId == m_iShenSuanZiUserId ? 1 : 0;
		palyer->set_isdivinemath(shensuanzi);
		palyer->set_index(n);
		if (shensuanzi == 1)
			palyer->set_index(0);
		palyer->set_isbanker(0);
		palyer->set_isapplybanker(0);
		palyer->set_applybankermulti(0);
		palyer->set_ljetton(0);
		palyer->set_szlocation(userInfoItem->location);

		PlayerScoreValue * score = gameStatusEnd.add_gameendscore();
		score->set_userid(userInfoItem->userId);
		score->set_returnscore(userInfoItem->m_EndWinScore);
		score->set_userscore(nowScore);

		if (userId == userInfoItem->userId) //vip
			isVIP = true;

		if (++n > size)
			break;
	}

	pInfo->set_wbankerchairid(0);
	pInfo->set_cbbankertimes(0);
	pInfo->set_lbankerwinscore(0);
	pInfo->set_lbankerscore(0);
	pInfo->set_benablesysbanker(true);
	pInfo->set_szgameroomname("bjl");

	for (auto &jetton : m_He)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(0);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_Zhuang)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(1);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_Xian)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(2);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_HePair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(3);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_ZhuangPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(4);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_XianPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(5);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}

	pInfo->set_lonlinepopulation(m_UserInfo.size());

	for (int i = 0;i < MAX_COUNT;i++)
	{
		pInfo->add_notopencount(m_notOpenCount_last[i]);
	}
	//限红
	for (int i = 0;i < 2;i++)
	{
		pInfo->add_llimitscore(m_lLimitScore[i]);
	}
	pInfo->set_tableid(m_strTableId);
	pInfo->set_nleftcardcount(m_nLeftCardCount);
	pInfo->set_ncutcardid(m_nCutCardId);
	pInfo->set_njuhaoid(m_nJuHaoId+1);

	gameStatusEnd.set_roundid(m_strRoundId);
	int32_t allTime = m_nGameEndTime;
	//补牌时间
	float bupaiTime = 0;
	if (m_cbCardCount[INDEX_BANKER] == 3)
	{
		bupaiTime += m_fBupaiTime;
	}
	if (m_cbCardCount[INDEX_PLAYER] == 3)
	{
		bupaiTime += m_fBupaiTime;
	}
	allTime += ceil(bupaiTime);

	gameStatusEnd.set_cbplacetime(allTime);
	chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
	int32_t leaveTime = allTime - durationTime.count();
	gameStatusEnd.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

	nowScore = pIServerUserItem->GetUserScore() + userInfo->m_EndWinScore;
	int64_t maxscore = nowScore >= 50 ? nowScore : 0;
	gameStatusEnd.set_lusermaxscore(maxscore);
	gameStatusEnd.set_luserscore(nowScore);
	gameStatusEnd.set_lrevenue(userInfo->m_Revenue);

	//LOG(INFO) << "Revenue:"<<userInfo->m_Revenue;

	if (!isVIP)  //myself
	{
		PlayerScoreValue* score = gameStatusEnd.add_gameendscore();
		score->set_userid(userId);
		score->set_returnscore(userInfo->m_EndWinScore);
		score->set_userscore(nowScore);
	}
	gameStatusEnd.set_wintag(m_nWinIndex);
	gameStatusEnd.set_wincardgrouptype(0);
	//系统开牌
	for (int i = 0; i < 2; ++i)
	{
		CardGroup* card = gameStatusEnd.add_cards();
		card->set_group_id(i + 1);
		card->set_cards(&(m_cbTableCards[i])[0], m_cbCardCount[i]);
		card->set_cardpoint(m_cbCardPoint[i]);
		if (m_cbCardCount[i] == 3)
		{
			card->set_bneedcard(true);
		}
		else
		{
			card->set_bneedcard(false);
		}
	}
	for (int i = 0;i < 3;i++)
	{
		gameStatusEnd.add_bcardpair(m_cbCardPair[i]);
	}
	int curState = getGameEndCurState();
	gameStatusEnd.set_ncurstate(curState);
	gameStatusEnd.set_notbetallcount(m_NotBetJettonKickOutCount);
	gameStatusEnd.set_curnotbetcount(userInfo->NotBetJettonCount);

	string data = gameStatusEnd.SerializeAsString();
	m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), bjl::SUB_S_GAME_STATUSEND, (uint8_t*)data.c_str(), data.size());
}
void CTableFrameSink::SendStartJettonMsg(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_StartPlaceJetton pStartPlaceJetton;

    pStartPlaceJetton.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pStartPlaceJetton.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

	int64_t xzScore = userInfo->lCurrentPlayerJetton;
    bool isRobot = pIServerUserItem->IsAndroidUser();
	int64_t nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pStartPlaceJetton.set_lusermaxscore(maxscore);
    pStartPlaceJetton.set_luserscore(nowScore);

    std::string data = pStartPlaceJetton.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), bjl::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
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
	for (int i = 0; i < MAX_COUNT; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 0; i < MAX_COUNT; ++i)
    {
        PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
        userjetton->set_dwuserid(userId);
        userjetton->set_ljettonarea(i);
		userjetton->set_ljettonscore(userInfo->dAreaJetton[i]);
    }
     //divineJettonScore
    for(int i = 0; i < MAX_COUNT; ++i)
    {
        PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
        divineJetton->set_dwuserid(m_iShenSuanZiUserId);
        shared_ptr<IServerUserItem> pUserItem;
        pUserItem = m_pITableFrame->GetUserItemByUserId(m_iShenSuanZiUserId);
        if(pUserItem)
            divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
        else
            divineJetton->set_ljettonscore(0);
        divineJetton->set_ljettonarea(i);
    }

    pInfo->set_lapplybankercondition(0);
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);

    bool isVIP = false;
    uint32_t n = 0;
    uint32_t size = 6;
    int64_t nowScore = 0;
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
        int shensuanzi = userInfoItem->userId == m_iShenSuanZiUserId ? 1 : 0;
        palyer->set_isdivinemath(shensuanzi);
        palyer->set_index(n);
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

        if(++n > size)
            break;
    }

    pInfo->set_wbankerchairid(0);
    pInfo->set_cbbankertimes(0);
    pInfo->set_lbankerwinscore(0);
    pInfo->set_lbankerscore(0);
    pInfo->set_benablesysbanker(true);
    pInfo->set_szgameroomname("bjl");

    for(auto &jetton : m_He)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(0);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Zhuang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(1);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Xian)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(2);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
	for (auto &jetton : m_HePair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(3);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_ZhuangPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(4);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for (auto &jetton : m_XianPair)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(5);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}

    pInfo->set_lonlinepopulation(m_UserInfo.size());

	for (int i = 0;i < MAX_COUNT;i++)
	{
		pInfo->add_notopencount(m_notOpenCount_last[i]);
	}
	//限红
	for (int i = 0;i < 2;i++)
	{
		pInfo->add_llimitscore(m_lLimitScore[i]);
	}
	pInfo->set_tableid(m_strTableId);
	pInfo->set_nleftcardcount(m_nLeftCardCount);
	pInfo->set_ncutcardid(m_nCutCardId);
	pInfo->set_njuhaoid(m_nJuHaoId+1);

    gamend.set_roundid(m_strRoundId);
	int32_t allTime = m_nGameEndTime;
	//补牌时间
	float bupaiTime = 0;
	if (m_cbCardCount[INDEX_BANKER] == 3)
	{
		bupaiTime += m_fBupaiTime;
	}
	if (m_cbCardCount[INDEX_PLAYER] == 3)
	{
		bupaiTime += m_fBupaiTime;
	}
	allTime += ceil(bupaiTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
    int32_t leaveTime = allTime - durationTime.count();
    gamend.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    nowScore = pIServerUserItem->GetUserScore() + userInfo->m_EndWinScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    gamend.set_lusermaxscore(maxscore);
    gamend.set_luserscore(nowScore);
    gamend.set_lrevenue(userInfo->m_Revenue);

    //LOG(INFO) << "Revenue:"<<userInfo->m_Revenue;

    if(!isVIP)  //myself
    {
        PlayerScoreValue* score = gamend.add_gameendscore();
        score->set_userid(userId);
        score->set_returnscore(userInfo->m_EndWinScore);
        score->set_userscore(nowScore);
    }
	gamend.set_wintag(m_nWinIndex);
	gamend.set_wincardgrouptype(0);
	//系统开牌
	for (int i = 0; i < 2; ++i)
	{
		CardGroup* card = gamend.add_cards();
		card->set_group_id(i + 1);
		card->set_cards(&(m_cbTableCards[i])[0], m_cbCardCount[i]);
		card->set_cardpoint(m_cbCardPoint[i]);
		if (m_cbCardCount[i] == 3)
		{
			card->set_bneedcard(true);
		}
		else
		{
			card->set_bneedcard(false);
		}
	}
	for (int i = 0;i < 3;i++)
	{
		gamend.add_bcardpair(m_cbCardPair[i]);
	}
	int curState = getGameEndCurState();
	gamend.set_ncurstate(curState);

    string data = gamend.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), bjl::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG(INFO) << __FILE__ << " " << __FUNCTION__;
    bool bClear = false;
    shared_ptr<CServerUserItem> pServerUserItem;
    pServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pServerUserItem)
    {
        LOG(INFO) << " pIServerUserItem==NULL Left UserId:"<<userId;
    }
	else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;
        int64_t totalxz = 0;
        uint8_t status = m_cbGameStatus; //m_pITableFrame->GetGameStatus();
        if(BJL_GAME_STATUS_PLACE_JETTON == status)
        {
            totalxz = userInfo->lCurrentPlayerJetton;
        }
        pServerUserItem->setOffline();
        if(totalxz == 0)
        {
			m_UserInfo.erase(userId);
            m_pITableFrame->ClearTableUser(chairId);
            bClear = true;
        }
		else
        {
            pServerUserItem->SetTrustship(true);
        }
    }

    if(userId == m_iShenSuanZiUserId)
    {
        m_iShenSuanZiUserId = 0;
    }
	if (bClear)
	{
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
			if ((*it)->userId == userId)
			{
				m_pPlayerInfoList.erase(it);
				break;
			}
		}
		for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
			if ((*it).chairId == userId)
			{
				m_pPlayerInfoList_20.erase(it);
				break;
			}
		}
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
    LOG(INFO) << __FILE__ << " " << __FUNCTION__;

    return true;
}

//根据系统输赢发牌
void CTableFrameSink::GetOpenCards()
{
	//概率获取牌对
	int32_t wholeweight = 0;
	int32_t weights[MAX_COUNT] = { 0 };
	int32_t pairTag = 0;
	int8_t cardPair[3] = { 0 };
	for (int i = 0;i < MAX_COUNT;i++)
	{
		wholeweight += m_retWeight[i];
		weights[i] = m_retWeight[i];
	}
	int32_t pribility = m_random.betweenInt(0, wholeweight).randInt_mt(true);
	int32_t resWeight = 0;
	for (int i = 0;i < 3;i++)
	{
		pribility = m_random.betweenInt(0, wholeweight).randInt_mt(true);
		resWeight = 0;
		for (int j = 0;j < MAX_COUNT;j++)
		{
			resWeight += weights[j];
			if (pribility - resWeight <= 0)
			{
				if (j == i + 3)
				{
					cardPair[i] = 1;
				}
				break;
			}
		}
	}
	
	double playerMuiltcle[MAX_COUNT] = { 0 };
	for (int i=0;i<MAX_COUNT;i++)
	{
		playerMuiltcle[i] = m_fMuilt[i];
	}
    tagStorageInfo storageInfo;
    int8_t winFlag = 2;//0-he 1-zhuang 2-hu
	
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
            int64_t allJetton=m_UserInfo[user->GetUserId()]->dAreaJetton[0]+m_UserInfo[user->GetUserId()]->dAreaJetton[1]+m_UserInfo[user->GetUserId()]->dAreaJetton[2];
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
        //难度干涉值 m_difficulty
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        openLog("后台设置的系统难度 :%d",m_difficulty);
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
        if(randomRet<m_difficulty)
        {
            m_pITableFrame->GetStorageScore(storageInfo);
			//设置上下限,当前利润,干预难度,超红判断值
            m_BjlAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,playerMuiltcle, m_lLimitPoints);
            m_BjlAlgorithm.SetMustKill(-1);
            for(int i=0;i<MAX_COUNT;i++)
				m_BjlAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值
            m_BjlAlgorithm.GetOpenCard(winFlag, cardPair);
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
					int64_t allJetton = m_UserInfo[user->GetUserId()]->lCurrentPlayerJetton;

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
                       for(int j=0;j<3;j++)
                       {
                           if(m_lTrueUserJettonScore[j]>0)
                           {
                               probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->dAreaJetton[j])/((float)m_lTrueUserJettonScore[0]+(float)m_lTrueUserJettonScore[1]+(float)m_lTrueUserJettonScore[2]);
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
				//设置上下限,当前利润,干预难度,超红判断值
                m_BjlAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,playerMuiltcle, m_lLimitPoints);
                for(int i=0;i<MAX_COUNT;i++)
					m_BjlAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值
                if(isMustKill)
                {
                    m_BjlAlgorithm.BlackGetMustkillOpenCard(winFlag,probilityAll,m_UserInfo[mustBeKillId]->dAreaJetton, cardPair,anyCase);
                }
                else
                {
                     m_BjlAlgorithm.BlackGetOpenCard(winFlag,probilityAll, cardPair);
                }
            }
            else
            {

                m_pITableFrame->GetStorageScore(storageInfo);
				//设置上下限,当前利润,干预难度,超红判断值
                m_BjlAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,playerMuiltcle, m_lLimitPoints);
                for(int i=0;i<MAX_COUNT;i++)
					m_BjlAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值
                m_BjlAlgorithm.GetOpenCard(winFlag, cardPair);
            }
        }
    }
    /*double res=0.0;
    res = m_BjlAlgorithm.CurrentProfit();
    if(res>0)
    {
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    LOG(ERROR)<<" systemReduceRatio: "<<m_pITableFrame->GetGameRoomInfo()->systemReduceRatio;
    m_pITableFrame->UpdateStorageScore(res);
    m_pITableFrame->GetStorageScore(storageInfo);*/
	if (m_bTestGame)
	{
		vector<uint8_t> cards;
		int id = 0;
		for (auto &it : m_TestCards)
		{
			cards = it.second;
			for (int i = 0;i < 2;i++)
			{
				m_cbTableCards[id][i] = cards[i];
			}
			if (cards.size() == 3)
			{
				m_cbTestBuCard[id] = cards[2];
			}
			id++;
		}
		int8_t cbBPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_BANKER], 2);
		int8_t cbPPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_PLAYER], 2);
		if (cbBPoint == cbPPoint)
			winFlag = 0;
		else if(cbBPoint > cbPPoint)
			winFlag = 1;
		else
			winFlag = 2;
	}
    if(winFlag==0)
    {
        openLog("本局开奖结果是: 和");
    }
    else if(winFlag==1)
    {
        openLog("本局开奖结果是: 庄");
    }
    else
    {
        openLog("本局开奖结果是: 闲");
    }
    
	string strJetton;
	for (int i=0;i<MAX_COUNT;i++)
	{
		strJetton += to_string(i) + "_" + to_string(m_lTrueUserJettonScore[i]) + "  ";
	}
	openLog("真人下注 %s", strJetton.c_str());

	/*if (winFlag != 0)
	{
		cardPair[0] = 0;
	}*/
    LOG(INFO) << "本局需求:winFlag: "<< (int)winFlag <<  " 牌对:" << (int)cardPair[0] << ":" << (int)cardPair[1] <<  ":" << (int)cardPair[2];
	openLog("本局需求 winFlag: %d, 牌对:%d %d %d ", (int)winFlag, (int)cardPair[0], (int)cardPair[1], (int)cardPair[2]);

	memset(m_cbTableCards, 0, sizeof(m_cbTableCards));

     //设置扑克
	uint8_t cbZhuangCardDate[2] = { 0 };
	uint8_t cbXianCardDate[2] = { 0 };

	//取牌
	//避免算法结果开和，前两张是和，结果都是和的情况
	int8_t chooseWinFlag = 0;
	if (!m_bTestGame && winFlag == 0 && cardPair[0] == 0 && cardPair[1] == 0 && cardPair[2] == 0)
	{
		//chooseWinFlag = m_random.betweenInt(1,2).randInt_re(true);
		bool bChoose = false;
		int8_t cbBankerValue = 0;
		int8_t cbPlayerValue = 0;
		for (int8_t i = 1;i < 3;i++)
		{
			GetSearchCards(i, cardPair, cbZhuangCardDate, cbXianCardDate);
			cbBankerValue = m_GameLogic.GetCardListPip(cbZhuangCardDate, 2);
			cbPlayerValue = m_GameLogic.GetCardListPip(cbXianCardDate, 2);
			if (cbBankerValue <= 5 && cbPlayerValue <= 5)
			{
				bChoose = true;
				//openLog("开和,洗牌庄闲成功: %d, 总局数:%d", m_nTestSameJushu, m_nTestJushu);
				break;
			}
		}
		if (!bChoose)
		{
			//openLog("开和,洗牌庄闲失败: %d, 总局数:%d", m_nTestSameJushu, m_nTestJushu);
			GetSearchCards(winFlag, cardPair, cbZhuangCardDate, cbXianCardDate);
		}
	} 
	else
	{
		GetSearchCards(winFlag, cardPair, cbZhuangCardDate, cbXianCardDate);
	}

	m_nSendCardIndex += 4;
	m_nLeftCardCount -= 4;

	int8_t cbBankerPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	int8_t cbPlayerTwoCardPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

	string strZhuang = "庄家牌值:" + GlobalFunc::converToHex(m_cbTableCards[INDEX_BANKER], 2);
	strZhuang += (" value:" + to_string(cbBankerPoint));
	string strXian = "闲家牌值:" + GlobalFunc::converToHex(m_cbTableCards[INDEX_PLAYER], 2);
	strXian += (" value:" + to_string(cbPlayerTwoCardPoint));
	LOG(INFO) << "取牌结果:winFlag: " << (int)winFlag << " 牌对:" << (int)m_cbCardPair[0] << ":" << (int)m_cbCardPair[1] << ":" << (int)m_cbCardPair[2] << " 庄家 " << strZhuang << " 闲家 " << strXian;
	openLog("取牌结果 winFlag: %d, 牌对:%d %d %d ,%s,%s", (int)winFlag, (int)m_cbCardPair[0], (int)m_cbCardPair[1], (int)m_cbCardPair[2], strZhuang.c_str(), strXian.c_str());
	
	JudgmentRepairCard(winFlag);
	JudgmentIsValidCard();	
	int8_t winIndex = 0;
	if (m_cbCardPoint[INDEX_BANKER] == m_cbCardPoint[INDEX_PLAYER])
	{
		winIndex = 0;
	}
	else if (m_cbCardPoint[INDEX_BANKER] > m_cbCardPoint[INDEX_PLAYER])
	{
		winIndex = 1;
	}
	else
	{
		winIndex = 2;
	}
	
	strZhuang = "牌值:" + GlobalFunc::converToHex(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	strZhuang += (" value:" + to_string(m_cbCardPoint[INDEX_BANKER]));
	strXian = "牌值:" + GlobalFunc::converToHex(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
	strXian += (" value:" + to_string(m_cbCardPoint[INDEX_PLAYER]));
	
	LOG(INFO) << "本局结果:winFlag: " << (int)winIndex << " 牌对:" << (int)m_cbCardPair[0] << ":" << (int)m_cbCardPair[1] << ":" << (int)m_cbCardPair[2] << " 庄家 " << strZhuang << " 闲家 " << strXian;
	openLog("本局结果 winFlag: %d, 牌对:%d %d %d 庄家%s,闲家%s", (int)winIndex, (int)m_cbCardPair[0], (int)m_cbCardPair[1], (int)m_cbCardPair[2],strZhuang.c_str(), strXian.c_str());

	if (TEST)
	{
		if (winIndex != winFlag)
		{
			int kk = 0;
		}
		else
		{
			m_nTestSameJushu++;
		}
		openLog("算法需求和最终结果相同的局数: %d, 总局数:%d", m_nTestSameJushu, m_nTestJushu);
		m_nWinIndex = winIndex;
		if (m_nLeftCardCount <= m_nCutCardId)
		{
			m_CardVecData.clear();
			m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
			random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
			m_nCutCardId = m_random.betweenInt(m_lLimitQiePai[0], m_lLimitQiePai[1]).randInt_mt(true); //1.5副到3副牌之间
			memset(m_cbCardTypeCount, 0, sizeof(m_cbCardTypeCount));
			m_nSendCardIndex = 0;
			m_nLeftCardCount = MAX_CARDS;
			m_nJuHaoId = 0;
			//clear open record
			m_openRecord.clear();
			//去掉前面6涨
			for (int i = 0; i < m_nCardFirstDeleteCount; ++i)
			{
				m_CardVecData.erase(m_CardVecData.begin());
				m_nSendCardIndex++;
				m_nLeftCardCount--;
			}
			LOG(INFO) << "本轮发牌结束,开始下一轮";
			openLog("游戏开始新一轮:m_strRoundId=%s,m_strTableId=%s,m_nLeftCardCount=%d,m_nCutCardId=%d", m_strRoundId.c_str(), m_strTableId.c_str(), m_nLeftCardCount, m_nCutCardId);
			JudgmentIsValidAllCard();
		}
	}
}

void CTableFrameSink::GetSearchCards(int8_t winFlag, int8_t cardPair[3], uint8_t cbZhuangCardDate[2], uint8_t cbXianCardDate[2])
{
	int pkSize = m_CardVecData.size();
	m_vecData.clear();
	for (int i = 0; i < pkSize; i++)
	{
		if (IsMoreCard(m_CardVecData[i]))
			m_vecData.push_back(m_CardVecData[i]);
	}
	uint8_t zhuangValue = 0;
	int zhuangIndex = 0;
	int xianIndex = 0;
	getTwoCard(cbZhuangCardDate, cardPair[1], zhuangIndex, m_vecData, pkSize, true);
	zhuangValue = m_GameLogic.GetCardListPip(cbZhuangCardDate, 2);
	int xianType = 0;
	int xianValue = 0;
	int washCount = 0;
	int8_t hePair = -1;
	switch (winFlag)
	{
	case 0:
		if (cardPair[0] == 1)
		{
			xianIndex = 0;
			pkSize = m_vecData.size();
			getHePairCard(cbZhuangCardDate, cbXianCardDate, cardPair[0], xianIndex, m_vecData, pkSize);
		}
		else
		{
			do
			{
				bool isOK = false;
				pkSize = m_vecData.size();
				for (xianIndex = 0; xianIndex < pkSize - 1; ++xianIndex)
				{
					getTwoCard(cbXianCardDate, cardPair[2], xianIndex, m_vecData, pkSize);
					xianValue = m_GameLogic.GetCardListPip(cbXianCardDate, 2);
					hePair = m_GameLogic.IsCardPair(cbZhuangCardDate, cbXianCardDate, 2, ID_HEPAIR);
					if ((zhuangValue == xianValue && hePair == cardPair[0]) || washCount >= 100)
					{
						isOK = true;
						eraseVecCard(cbXianCardDate, 2);
						openLog("取牌成功 VecSize:%d, winFlag: %d,washCount:%d,zhuangValue:%d,xianValue:%d", pkSize, (int)winFlag, washCount, zhuangValue, xianValue);
						break;
					}
				}

				if (xianIndex >= pkSize - 1) //not find the same card value
				{
					washCount++;
					if (washCount >= m_vecData.size())
					{
						openLog("取牌失败 VecSize:%d, winFlag: %d,washCount:%d", m_vecData.size(), (int)winFlag, washCount);
						eraseVecCard(cbXianCardDate, 2);
						break;
					}
					zhuangIndex = washCount;
					m_vecData.clear();
					pkSize = m_CardVecData.size();
					for (int i = 0; i < pkSize; i++)
					{
						if (IsMoreCard(m_CardVecData[i]))
							m_vecData.push_back(m_CardVecData[i]);
					}
					getTwoCard(cbZhuangCardDate, cardPair[1], zhuangIndex, m_vecData, pkSize, true);
					zhuangValue = m_GameLogic.GetCardListPip(cbZhuangCardDate, 2);
					openLog("取牌失败 VecSize:%d, winFlag: %d,washCount:%d", m_CardVecData.size(), (int)winFlag, washCount);
				}
				else if (isOK)
				{
					break;
				}
			} while (true);
		}
		break;
	case 1:
		if (cardPair[0] == 1)
		{
			xianIndex = 0;
			pkSize = m_vecData.size();
			getHePairCard(cbZhuangCardDate, cbXianCardDate, cardPair[0], xianIndex, m_vecData, pkSize);
		}
		else
		{
			do
			{
				bool isOK = false;
				pkSize = m_vecData.size();
				for (xianIndex = 0; xianIndex < pkSize - 1; ++xianIndex)
				{
					getTwoCard(cbXianCardDate, cardPair[2], xianIndex, m_vecData, pkSize);
					xianValue = m_GameLogic.GetCardListPip(cbXianCardDate, 2);
					hePair = m_GameLogic.IsCardPair(cbZhuangCardDate, cbXianCardDate, 2, ID_HEPAIR);
					if (zhuangValue == xianValue)
						continue;
					else if ((zhuangValue > xianValue && hePair == cardPair[0]) || washCount >= 100)
					{
						isOK = true;
						eraseVecCard(cbXianCardDate, 2);
						openLog("取牌成功 VecSize:%d, winFlag: %d,washCount:%d,zhuangValue:%d,xianValue:%d,xianIndex=%d", pkSize, (int)winFlag, washCount, zhuangValue, xianValue, xianIndex);
						break;
					}
				}
				if (xianIndex >= pkSize - 1) //not find the same card value
				{
					washCount++;
					if (washCount >= m_vecData.size())
					{
						openLog("取牌失败 VecSize:%d, winFlag: %d,washCount:%d", m_vecData.size(), (int)winFlag, washCount);
						eraseVecCard(cbXianCardDate, 2);
						break;
					}
					zhuangIndex = washCount;
					m_vecData.clear();
					pkSize = m_CardVecData.size();
					for (int i = 0; i < pkSize; i++)
					{
						if (IsMoreCard(m_CardVecData[i]))
							m_vecData.push_back(m_CardVecData[i]);
					}
					getTwoCard(cbZhuangCardDate, cardPair[1], zhuangIndex, m_vecData, pkSize, true);
					zhuangValue = m_GameLogic.GetCardListPip(cbZhuangCardDate, 2);
					openLog("取牌失败 VecSize:%d,  winFlag: %d,washCount:%d", m_CardVecData.size(), (int)winFlag, washCount);
				}
				else if (isOK)
				{
					break;
				}
			} while (true);
		}
		break;
	case 2:
		if (cardPair[0] == 1)
		{
			xianIndex = 0;
			pkSize = m_vecData.size();
			getHePairCard(cbZhuangCardDate, cbXianCardDate, cardPair[0], xianIndex, m_vecData, pkSize);
		}
		else
		{
			do
			{
				bool isOK = false;
				pkSize = m_vecData.size();
				for (xianIndex = 0; xianIndex < pkSize - 1; ++xianIndex)
				{
					getTwoCard(cbXianCardDate, cardPair[2], xianIndex, m_vecData, pkSize);
					xianValue = m_GameLogic.GetCardListPip(cbXianCardDate, 2);
					hePair = m_GameLogic.IsCardPair(cbZhuangCardDate, cbXianCardDate, 2, ID_HEPAIR);
					if (zhuangValue == xianValue)
						continue;
					else if ((zhuangValue < xianValue && hePair == cardPair[0]) || washCount >= 100)
					{
						isOK = true;
						eraseVecCard(cbXianCardDate, 2);
						openLog("取牌成功 VecSize:%d, winFlag: %d,washCount:%d,zhuangValue:%d,xianValue:%d", pkSize, (int)winFlag, washCount, zhuangValue, xianValue);
						break;
					}
				}
				if (xianIndex >= pkSize - 1) //not find the same card value
				{
					washCount++;
					if (washCount >= m_vecData.size())
					{
						openLog("取牌失败 VecSize:%d, winFlag: %d,washCount:%d", m_vecData.size(), (int)winFlag, washCount);
						eraseVecCard(cbXianCardDate, 2);
						break;
					}
					zhuangIndex = washCount;
					m_vecData.clear();
					pkSize = m_CardVecData.size();
					for (int i = 0; i < pkSize; i++)
					{
						if (IsMoreCard(m_CardVecData[i]))
							m_vecData.push_back(m_CardVecData[i]);
					}
					getTwoCard(cbZhuangCardDate, cardPair[1], zhuangIndex, m_vecData, pkSize, true);
					zhuangValue = m_GameLogic.GetCardListPip(cbZhuangCardDate, 2);
					openLog("取牌失败 VecSize:%d, winFlag: %d,washCount:%d", m_CardVecData.size(), (int)winFlag, washCount);
				}
				else if (isOK)
				{
					break;
				}
			} while (true);
		}
		break;
	default:
		LOG(INFO) << "......WinFlag:" << winFlag << " Error!";
	}

	/*m_nSendCardIndex += 4;
	m_nLeftCardCount -= 4;*/
	m_cbCardCount[INDEX_BANKER] = 2;
	m_cbCardCount[INDEX_PLAYER] = 2;
	if (m_bTestGame)
	{
		vector<uint8_t> cards;
		int id = 0;
		for (auto &it : m_TestCards)
		{
			cards = it.second;
			for (int i = 0;i < 2;i++)
			{
				m_cbTableCards[id][i] = cards[i];
			}
			id++;
		}
	} 
	else
	{
		//避免每次开奖前两张结果就是最终的结果
		if (cardPair[0] == 0 && cardPair[1] == 0 && cardPair[2] == 0)
		{
			int32_t randomRet = m_random.betweenInt(0, 1000).randInt_mt(true);
			if (zhuangValue <= 5 && xianValue <= 5)
			{
				if (randomRet < 500)
				{
					for (int i = 0;i < 2;i++)
					{
						m_cbTableCards[INDEX_BANKER][i] = cbXianCardDate[i];
						m_cbTableCards[INDEX_PLAYER][i] = cbZhuangCardDate[i];
					}
					if (TEST)
						m_nTestChangJushu++;
				}
				else
				{
					for (int i = 0;i < 2;i++)
					{
						m_cbTableCards[INDEX_BANKER][i] = cbZhuangCardDate[i];
						m_cbTableCards[INDEX_PLAYER][i] = cbXianCardDate[i];
					}
				}
			} 
			else if ((zhuangValue <= 7 && xianValue <= 5) || (xianValue <= 7 && zhuangValue <= 5))
			{
				if (randomRet < 500)
				{
					for (int i = 0;i < 2;i++)
					{
						m_cbTableCards[INDEX_BANKER][i] = cbXianCardDate[i];
						m_cbTableCards[INDEX_PLAYER][i] = cbZhuangCardDate[i];
					}
					if (TEST) 
						m_nTestChangJushu++;
				}
				else
				{
					for (int i = 0;i < 2;i++)
					{
						m_cbTableCards[INDEX_BANKER][i] = cbZhuangCardDate[i];
						m_cbTableCards[INDEX_PLAYER][i] = cbXianCardDate[i];
					}
				}
			}
			else
			{
				for (int i = 0;i < 2;i++)
				{
					m_cbTableCards[INDEX_BANKER][i] = cbZhuangCardDate[i];
					m_cbTableCards[INDEX_PLAYER][i] = cbXianCardDate[i];
				}
			}
			if (TEST)
				openLog("前两张庄闲调换的局数: %d, 总局数:%d", m_nTestChangJushu, m_nTestJushu);
		} 
		else
		{
			for (int i = 0;i < 2;i++)
			{
				m_cbTableCards[INDEX_BANKER][i] = cbZhuangCardDate[i];
				m_cbTableCards[INDEX_PLAYER][i] = cbXianCardDate[i];
			}
		}
	}
	
	//是否牌对
	m_cbCardPair[ID_HEPAIR] = m_GameLogic.IsCardPair(m_cbTableCards[INDEX_BANKER], m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_BANKER], ID_HEPAIR);
	m_cbCardPair[ID_BANKERPAIR] = m_GameLogic.IsCardPair(m_cbTableCards[INDEX_BANKER], m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_BANKER], ID_BANKERPAIR);
	m_cbCardPair[ID_PLAYERPAIR] = m_GameLogic.IsCardPair(m_cbTableCards[INDEX_BANKER], m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_BANKER], ID_PLAYERPAIR);
}

//判断是否补牌
bool CTableFrameSink::JudgmentRepairCard(int8_t winFlag)
{
	for (int i = 0; i < 2; ++i)
	{
		for (vector<uint8_t>::iterator it = m_CardVecData.begin(); it != m_CardVecData.end(); it++)
		{
			if (m_cbTableCards[INDEX_BANKER][i] == (*it))
			{
				m_CardVecData.erase(it);
				break;
			}
		}
		for (vector<uint8_t>::iterator it = m_CardVecData.begin(); it != m_CardVecData.end(); it++)
		{
			if (m_cbTableCards[INDEX_PLAYER][i] == (*it))
			{
				m_CardVecData.erase(it);
				break;
			}
		}
		assert(m_GameLogic.IsCardValid(m_cbTableCards[INDEX_BANKER][i]));
		assert(m_GameLogic.IsCardValid(m_cbTableCards[INDEX_PLAYER][i]));
	}
	//计算点数
	int8_t cbBankerPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	int8_t cbPlayerTwoCardPoint = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);	
	
	int32_t pkSize = m_CardVecData.size();
	m_vecData.clear();
	for (int i = 0; i < pkSize; i++)
	{
		if (IsMoreCard(m_CardVecData[i]))
			m_vecData.push_back(m_CardVecData[i]);
	}
	int32_t playerNeedIndex = -1;
	int32_t bankerNeedIndex = -1;
	bool playerNeedCard = false;
	int8_t winFlagChang = winFlag;
	//闲家补牌
	int32_t cbPlayerThirdCardPoint = 0;	//第三张牌点数	
	if (cbPlayerTwoCardPoint <= 5 && cbBankerPoint < 8)
	{
		//计算点数
		m_nSendCardIndex++;
		m_cbCardCount[INDEX_PLAYER]++;
		m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
		cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
		playerNeedCard = true;
		if (m_bTestGame && m_cbTestBuCard[INDEX_PLAYER] != 0)
		{
			m_cbTableCards[INDEX_PLAYER][2] = m_cbTestBuCard[INDEX_PLAYER];
			cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
		}
	}
	pkSize = m_vecData.size();
	int8_t cbPlayerAllCardPoint = 0;
	//庄家补牌
	if (cbPlayerTwoCardPoint < 8 && cbBankerPoint < 8)
	{
		switch (cbBankerPoint)
		{
		case 0:
		case 1:
		case 2:
		{
			m_nSendCardIndex++;
			m_cbCardCount[INDEX_BANKER]++;
			cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
			m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
			break;
		}
		case 3:
		{
			if (winFlagChang == winFlag)
			{
				//补牌后结果没变
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 8) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else if (m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint == 8)
			{
				//补牌后结果改变了
				int chooseCount = 0;
				do
				{
					m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
					cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
					chooseCount++;
					if (chooseCount > 200)
					{
						openLog("补牌后结果改变了 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
					if (winFlagChang == winFlag)
					{
						openLog("补牌后结果不变退出 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
				} while (cbPlayerThirdCardPoint == 8);

				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 8) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 8) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			break;
		}
		case 4:		
		{
			if (winFlagChang == winFlag)
			{
				//补牌后结果没变
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else if ((m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardPoint == 1 || cbPlayerThirdCardPoint == 8 || cbPlayerThirdCardPoint == 9 || cbPlayerThirdCardPoint == 0)))
			{
				//补牌后结果改变了
				int chooseCount = 0;
				do
				{
					m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
					cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
					chooseCount++;
					if (chooseCount > 200)
					{
						openLog("补牌后结果改变了 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
					if (winFlagChang == winFlag)
					{
						openLog("补牌后结果不变退出 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
				} while (cbPlayerThirdCardPoint == 1 || cbPlayerThirdCardPoint == 8 || cbPlayerThirdCardPoint == 9 || cbPlayerThirdCardPoint == 0);

				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			break;
		}
		case 5:
		{
			if (winFlagChang == winFlag)
			{
				//补牌后结果没变
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 2 && cbPlayerThirdCardPoint != 3 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardPoint == 1 || cbPlayerThirdCardPoint == 2 || cbPlayerThirdCardPoint == 3 || cbPlayerThirdCardPoint == 8 || cbPlayerThirdCardPoint == 9 || cbPlayerThirdCardPoint == 0))
			{
				//补牌后结果改变了
				int chooseCount = 0;
				do
				{
					m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
					cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
					chooseCount++;
					if (chooseCount > 200)
					{
						openLog("补牌后结果改变了 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
					if (winFlagChang == winFlag)
					{
						openLog("补牌后结果不变退出 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
				} while (cbPlayerThirdCardPoint == 1 || cbPlayerThirdCardPoint == 2 || cbPlayerThirdCardPoint == 3 || cbPlayerThirdCardPoint == 8 || cbPlayerThirdCardPoint == 9 || cbPlayerThirdCardPoint == 0);

				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 2 && cbPlayerThirdCardPoint != 3 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 1 && cbPlayerThirdCardPoint != 2 && cbPlayerThirdCardPoint != 3 && cbPlayerThirdCardPoint != 8 && cbPlayerThirdCardPoint != 9 && cbPlayerThirdCardPoint != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			break;
		}
		case 6:
		{
			if (winFlagChang == winFlag)
			{
				//补牌后结果没变
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardPoint == 6 || cbPlayerThirdCardPoint == 7))
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else if (m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardPoint != 6 && cbPlayerThirdCardPoint != 7)
			{
				//补牌后结果改变了
				int chooseCount = 0;
				do
				{
					m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
					cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
					chooseCount++;
					if (chooseCount > 200)
					{
						openLog("补牌后结果改变了 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
					if (winFlagChang == winFlag)
					{
						openLog("补牌后结果不变退出 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
				} while (cbPlayerThirdCardPoint != 6 && cbPlayerThirdCardPoint != 7);

				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardPoint == 6 || cbPlayerThirdCardPoint == 7))
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			else
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardPoint == 6 || cbPlayerThirdCardPoint == 7))
				{
					m_nSendCardIndex++;
					m_cbCardCount[INDEX_BANKER]++;
					cbPlayerAllCardPoint = (cbPlayerTwoCardPoint + cbPlayerThirdCardPoint) % 10;
					m_cbTableCards[INDEX_BANKER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerAllCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex, true);
				}
			}
			break;
		}
		//不须补牌
		case 7:
		{
			if (winFlagChang != winFlag)
			{
				//补牌后结果改变了
				int chooseCount = 0;
				do
				{
					m_cbTableCards[INDEX_PLAYER][2] = getOneCard(winFlag, cbBankerPoint, cbPlayerTwoCardPoint, winFlagChang, bankerNeedIndex, playerNeedIndex);
					cbPlayerThirdCardPoint = m_GameLogic.GetCardPip(m_cbTableCards[INDEX_PLAYER][2]);
					chooseCount++;
					if (chooseCount > 200)
					{
						openLog("补牌后结果改变了 VecSize:%d, winFlag: %d,cbBankerPoint:%d,chooseCount:%d", pkSize, (int)winFlagChang, cbBankerPoint, chooseCount);
						break;
					}
				} while (winFlagChang != winFlag);
			}
			break;
		}
		case 8:
		case 9:			
			break;
		default:
			break;
		}
	}
	if (m_GameLogic.IsCardValid(m_cbTableCards[INDEX_BANKER][2]) && m_cbCardCount[INDEX_BANKER] != 3)
	{
		m_cbCardCount[INDEX_BANKER] = 3;
	}
	if (m_GameLogic.IsCardValid(m_cbTableCards[INDEX_PLAYER][2]) && m_cbCardCount[INDEX_PLAYER] != 3)
	{
		m_cbCardCount[INDEX_PLAYER] = 3;
	}
	m_cbCardPoint[INDEX_BANKER] = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	m_cbCardPoint[INDEX_PLAYER] = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
	
	if (m_cbCardCount[INDEX_BANKER]==3)
	{
		m_nLeftCardCount--;
		for (vector<uint8_t>::iterator it = m_CardVecData.begin(); it != m_CardVecData.end(); it++)
		{
			if (m_cbTableCards[INDEX_BANKER][2] == (*it))
			{
				m_CardVecData.erase(it);
				break;
			}
		}
	}
	if (m_cbCardCount[INDEX_PLAYER] == 3)
	{
		m_nLeftCardCount--;
		for (vector<uint8_t>::iterator it = m_CardVecData.begin(); it != m_CardVecData.end(); it++)
		{
			if (m_cbTableCards[INDEX_PLAYER][2] == (*it))
			{
				m_CardVecData.erase(it);
				break;
			}
		}
	}
	return true;
}
//判断牌是否有效
bool CTableFrameSink::IsMoreCard(uint8_t card)
{
	uint8_t value = m_GameLogic.GetCardValue(card);
	uint8_t color = m_GameLogic.GetCardColor(card);
	if (value >= 1 && value <= 13 && color >= 0 && color <= 3)
	{
		if ((m_cbCardTypeCount[color][value - 1] + 1) > 8)
		{
			openLog("牌值重复了,card=0x%02x;", card);
			return false;
		}
	}
	else
	{
		openLog("牌值无效出错了,card=0x%02x;", card);
		return false;
	}

	return true;
}
bool CTableFrameSink::JudgmentIsValidCard()
{
	//庄家牌
	for (int i = 0;i < m_cbCardCount[INDEX_BANKER];i++)
	{
		uint8_t value = m_GameLogic.GetCardValue(m_cbTableCards[INDEX_BANKER][i]);
		uint8_t color = m_GameLogic.GetCardColor(m_cbTableCards[INDEX_BANKER][i]);
		if (value >= 1 && value <= 13 && color >= 0 && color <= 3)
		{
			m_cbCardTypeCount[color][value - 1]++;
			if (m_cbCardTypeCount[color][value - 1] > 8)
			{
				openLog("庄家牌值重复了,card=0x%02x;", m_cbTableCards[INDEX_BANKER][i]);
				if(!m_bTestGame)
					assert(m_cbCardTypeCount[color][value - 1] <= 8);
			}
		}
		else
		{
			openLog("庄家牌值无效出错了,card=0x%02x;", m_cbTableCards[INDEX_BANKER][i]);
			assert(m_GameLogic.IsCardValid(m_cbTableCards[INDEX_BANKER][i]));
		}
	}
	//闲家牌
	for (int i = 0;i < m_cbCardCount[INDEX_PLAYER];i++)
	{
		uint8_t value = m_GameLogic.GetCardValue(m_cbTableCards[INDEX_PLAYER][i]);
		uint8_t color = m_GameLogic.GetCardColor(m_cbTableCards[INDEX_PLAYER][i]);
		if (value >= 1 && value <= 13 && color >= 0 && color <= 3)
		{
			m_cbCardTypeCount[color][value - 1]++;
			if (m_cbCardTypeCount[color][value - 1] > 8)
			{
				openLog("闲家牌值重复了,card=0x%02x;", m_cbTableCards[INDEX_PLAYER][i]);
				if (!m_bTestGame)
					assert(m_cbCardTypeCount[color][value - 1] <= 8);
			}
		}
		else
		{
			openLog("闲家牌值无效出错了,card=0x%02x;", m_cbTableCards[INDEX_PLAYER][i]);
			assert(m_GameLogic.IsCardValid(m_cbTableCards[INDEX_PLAYER][i]));
		}
	}
}
bool CTableFrameSink::JudgmentIsValidAllCard()
{
	for (int i = 0;i < m_CardVecData.size();i++)
	{
		uint8_t value = m_GameLogic.GetCardValue(m_CardVecData[i]);
		uint8_t color = m_GameLogic.GetCardColor(m_CardVecData[i]);
		if (value >= 1 && value <= 13 && color >= 0 && color <= 3)
		{
			m_cbCardTypeCount[color][value - 1]++;
			if (m_cbCardTypeCount[color][value - 1] > 8)
			{
				openLog("校验牌值重复了,card=0x%02x;", m_CardVecData[i]);
				if (!m_bTestGame)
					assert(m_cbCardTypeCount[color][value - 1] <= 8);
			}
		}
		else
		{
			openLog("校验牌值无效出错了,card=0x%02x;", m_CardVecData[i]);
		}
	}
	memset(m_cbCardTypeCount, 0, sizeof(m_cbCardTypeCount));
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
        int64_t curJetton = userInfoItem->dAreaJetton[0]+userInfoItem->dAreaJetton[1]+userInfoItem->dAreaJetton[2];
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
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason  << ", m_strRoundId = " << m_strRoundId ;
	openLog("CTableFrameSink::OnEventGameConclude onlineUserCount: %d ", m_UserInfo.size());
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();     //open cards
		int32_t zhuangValue = 0;
		int32_t xianValue = 0;
		string zhuangCardName = "";
		string xianCardName = "";
		for (int i = 0;i < m_cbCardCount[INDEX_BANKER];i++)
		{
			zhuangCardName += GetCardValueName(m_cbTableCards[INDEX_BANKER][i]) + " ";
		}
		for (int i = 0;i < m_cbCardCount[INDEX_PLAYER];i++)
		{
			xianCardName += GetCardValueName(m_cbTableCards[INDEX_PLAYER][i]) + " ";
		}
		zhuangValue = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		xianValue = m_GameLogic.GetCardListPip(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
		LOG(INFO) << "Open Card ZhuangCard:" << zhuangCardName << " XianCard:" << xianCardName;
		m_nWinIndex = -1;
		if (zhuangValue > xianValue)
			m_nWinIndex = 1;
		else if (zhuangValue < xianValue)
			m_nWinIndex = 2;
		else
			m_nWinIndex = 0;
		
		// 庄闲牌
		m_strResultMsg = "";
		m_strResultMsg = GlobalFunc::converToHex(m_cbTableCards[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]) + GlobalFunc::converToHex(m_cbTableCards[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
		// 庄闲牌数目
		m_strResultMsg += ("|" + to_string(m_cbCardCount[INDEX_BANKER]) + ":" + to_string(m_cbCardCount[INDEX_PLAYER]));
		// 庄闲牌点数
		m_strResultMsg += ("|" + to_string(m_cbCardPoint[INDEX_BANKER]) + ":" + to_string(m_cbCardPoint[INDEX_PLAYER]));
		// 开奖结果[0和、1庄、2闲]赢
		m_strResultMsg += ("|" + to_string(m_nWinIndex));
		// 是否对子【0和对、1庄对、2闲对】、【0:否 1:是】
		m_strResultMsg += ("|" + to_string(m_cbCardPair[ID_HEPAIR]) + ":" + to_string(m_cbCardPair[ID_BANKERPAIR]) + ":" + to_string(m_cbCardPair[ID_PLAYERPAIR]));

		m_replay.addStep((uint32_t)time(NULL)-m_dwReadyTime, m_strResultMsg,-1,opShowCard,-1,-1);
		double res=0.0;
		//结算
		m_fRetMul = m_fMuilt[m_nWinIndex];
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<UserInfo> userInfoItem;
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem)
				continue;
			//设置玩家倍率
			userInfoItem->SetPlayerMuticl(m_nWinIndex, m_fRetMul, m_fMuilt, m_cbCardPair);
			//税收比例
			userInfoItem->Culculate(m_fTaxRate, m_nWinIndex);
			if (!pUserItem->IsAndroidUser())
			{
				res -= userInfoItem->m_EndWinScore;
			}
		}        
		LOG(INFO) << "本局开奖结果: m_strRoundId = " << m_strRoundId << ",m_nWinIndex=" << m_nWinIndex << " m_strResultMsg:" << m_strResultMsg;
		openLog("本局[%d]游戏结束:m_strRoundId=%s,m_strTableId=%s,开奖结果=%d,m_nSendCardIndex=%d,m_nCutCardId=%d,m_strResultMsg=%s", m_nJuHaoId,m_strRoundId.c_str(), m_strTableId.c_str(), m_nWinIndex, m_nSendCardIndex, m_nCutCardId, m_strResultMsg.c_str());
		tagStorageInfo storageInfo;
		if (res > 0)
		{
			res = res * (1000.0 - m_pITableFrame->GetGameRoomInfo()->systemReduceRatio) / 1000.0;
		}
		LOG(ERROR) << " systemReduceRatio: " << m_pITableFrame->GetGameRoomInfo()->systemReduceRatio;
		m_pITableFrame->UpdateStorageScore(res);
		m_pITableFrame->GetStorageScore(storageInfo);
		openLog("本次系统赢分是：    %d", (int64_t)res);
		openLog("系统库存上限是：    %ld", storageInfo.lUplimit);
		openLog("系统库存下限是：    %ld", storageInfo.lLowlimit);
		openLog("系统当前库存是：    %ld \n", storageInfo.lEndStorage);
		m_pPlayerInfoList.clear();
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			int64_t curJetton = userInfoItem->dAreaJetton[0] + userInfoItem->dAreaJetton[1] + userInfoItem->dAreaJetton[2];
			//统计完以后把本次下注记录为玩家上次下注值
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem || !userInfoItem->Isplaying)
				continue;
			m_pPlayerInfoList.push_back(userInfoItem);
			if (pUserItem->IsAndroidUser() || m_nWinIndex == 0 || curJetton == 0)
				continue;

			userInfoItem->lLastJetton = curJetton;
			userInfoItem->lLastWinOrLost = userInfoItem->m_EndWinScore >= 0 ? 1 : 0;

			LOG(ERROR) << "curJetton = " << curJetton << "    lLastWinOrLost =" << userInfoItem->lLastWinOrLost;
		}
		m_bGameEnd = true;
		m_bRefreshPlayerList = false;
		PlayerRichSorting();
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

	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (pUserItem)
		{
			if (m_pGameRoomInfo->serverStatus != SERVER_RUNNING || (pUserItem->GetUserStatus() == sOffline ||
				(pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < min_score)))
			{
				pUserItem->setOffline();
				clearUserIdVec.push_back(userInfoItem->userId);
				clearChairIdVec.push_back(userInfoItem->chairId);
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
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
			if (clearUserIdVec[i] == (*it)->userId)
			{
				m_pPlayerInfoList.erase(it);
				break;
			}
		}
		for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
			if (clearUserIdVec[i] == (*it).userId)
			{
				m_pPlayerInfoList_20.erase(it);
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
			/*if (pUserItem->IsAndroidUser())
				continue;
			if (userInfoItem->lCurrentPlayerJetton == 0)
				continue;*/
			allBetScore = 0;
			userid = pUserItem->GetUserId();
            scoreInfo.clear();
			if (pUserItem->IsAndroidUser())
			{
				bool bChoose = false;
				totalBetScore = 0;
				youxiaotouzhu = 0;
				for (int betIdx = 0;betIdx < MAX_COUNT; betIdx++)
				{
					// 结算时记录
					int64_t addScore = userInfoItem->dAreaWin[betIdx];
					int64_t betScore = userInfoItem->dAreaJetton[betIdx];
					// 押分情况
					scoreInfo.cellScore.push_back(betScore);
					//牌型
					/*if (betScore > 0) {
						m_replay.addResult(userInfoItem->chairId, betIdx, betScore, addScore, m_strResultMsg, false);
					}*/
					totalBetScore += betScore;
					allBetScore += betScore;
					if (m_nWinIndex == (int)WinTag::eHe)
					{
						if (betIdx != (int)WinTag::eZhuang && betIdx != (int)WinTag::eXian)
						{
							youxiaotouzhu += betScore;
						}
					}
					else
					{
						if (betIdx == (int)WinTag::eHe || betIdx > (int)WinTag::eXian)
						{
							youxiaotouzhu += betScore;
						}
						else if (!bChoose)
						{
							int zhuang = (int)WinTag::eZhuang;
							int xian = (int)WinTag::eXian;
							youxiaotouzhu += abs(userInfoItem->dAreaJetton[zhuang] - userInfoItem->dAreaJetton[xian]);
							bChoose = true;
						}
					}
				}
			}
			else
			{
				bool bChoose = false;
				totalBetScore = 0;
				youxiaotouzhu = 0;
				for (int betIdx = 0;betIdx < MAX_COUNT; betIdx++)
				{
					// 结算时记录
					int64_t addScore = userInfoItem->dAreaWin[betIdx];
					int64_t betScore = userInfoItem->dAreaJetton[betIdx];
					// 押分情况
					scoreInfo.cellScore.push_back(betScore);
					//牌型
					if (betScore > 0) {
						m_replay.addResult(userInfoItem->chairId, betIdx, betScore, addScore, m_strResultMsg, false);
					}
					totalBetScore += betScore;
					allBetScore += betScore;
					if (m_nWinIndex == (int)WinTag::eHe)
					{
						if (betIdx != (int)WinTag::eZhuang && betIdx != (int)WinTag::eXian)
						{
							youxiaotouzhu += betScore;
						}
					}
					else
					{
						if (betIdx == (int)WinTag::eHe || betIdx > (int)WinTag::eXian)
						{
							youxiaotouzhu += betScore;
						}
						else if (!bChoose)
						{
							int zhuang = (int)WinTag::eZhuang;
							int xian = (int)WinTag::eXian;
							youxiaotouzhu += abs(userInfoItem->dAreaJetton[zhuang] - userInfoItem->dAreaJetton[xian]);
							bChoose = true;
						}
					}
				}
			}
			
            if(totalBetScore > 0)
            {
				scoreInfo.chairId = userInfoItem->chairId;
				scoreInfo.betScore = totalBetScore;
				scoreInfo.rWinScore = youxiaotouzhu;
				scoreInfo.addScore = userInfoItem->m_EndWinScore;
				scoreInfo.revenue = userInfoItem->m_Revenue;
				scoreInfo.cardValue = m_strResultMsg;  // GetCardValueName(0) + " vs "+GetCardValueName(1);
				scoreInfo.startTime = m_startTime;
				scoreInfoVec.push_back(scoreInfo);
            }
            // 统计连续未下注的次数
            if (m_NotBetJettonKickOutCount > 0 && totalBetScore <= 0)
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
		totalJetton = userInfoItem->lCurrentPlayerJetton;
		if (userInfoItem->m_JettonScore.size() >= 20)
			userInfoItem->m_JettonScore.pop_front();
		userInfoItem->m_JettonScore.push_back(totalJetton);
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
    m_iShenSuanZiUserId = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        if(userInfoItem)
        {
            if(m_iShenSuanZiUserId == 0)
            {
                m_iShenSuanZiUserId = userInfoItem->userId;
            }
            else if(userInfoItem->lTwentyWinCount > m_UserInfo[m_iShenSuanZiUserId]->lTwentyWinCount)
            {
                m_iShenSuanZiUserId = userInfoItem->userId;
            }
            userInfoItem->isDivineMath=false;
        }
    }
    if(m_iShenSuanZiUserId!=0)
    {
        m_UserInfo[m_iShenSuanZiUserId]->isDivineMath=true;
        m_SortUserInfo.emplace(m_UserInfo[m_iShenSuanZiUserId]);
    }
    bool bFirst = true;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem||m_iShenSuanZiUserId==userInfoItem->userId)
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
void CTableFrameSink::OnTimerWashCard()
{
	//去掉前面6涨
	for (int i = 0; i < m_nCardFirstDeleteCount; ++i)
	{
		m_CardVecData.erase(m_CardVecData.begin());
		m_nSendCardIndex++;
		m_nLeftCardCount--;
	}
	m_cbGameStatus = BJL_GAME_STATUS_WASHCARD;
	m_gameStateStartTime = chrono::system_clock::now();
	LOG(INFO) << "游戏:m_strRoundId=" << m_strRoundId << ",进入洗牌牌状态：" << m_cbGameStatus;
	openLog("本局[%d]游戏开始洗牌:m_strRoundId=%s,m_nCutCardId=%d", m_nJuHaoId, m_strRoundId.c_str(), m_nCutCardId);
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		CMD_S_WashCard pWashCard;
		GameDeskInfo* pInfo = pWashCard.mutable_deskdata();
		for (int i = 0;i < 7;i++)
		{
			pInfo->add_userchips(m_currentchips[userInfoItem->chairId][i]);
		}
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
			alljitton->set_dwuserid(0);
			alljitton->set_ljettonscore(m_lAllJettonScore[i]);
			alljitton->set_ljettonarea(i);
		}
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
			userjetton->set_dwuserid(userInfoItem->userId);
			userjetton->set_ljettonarea(i);
			userjetton->set_ljettonscore(userInfoItem->dAreaJetton[i]);
		}
		//divineJettonScore
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
			divineJetton->set_dwuserid(m_iShenSuanZiUserId);
			shared_ptr<IServerUserItem> pUserItem = m_pITableFrame->GetUserItemByUserId(m_iShenSuanZiUserId);
			if (pUserItem)
				divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
			else
				divineJetton->set_ljettonscore(0);
			divineJetton->set_ljettonarea(i);
		}
		pInfo->set_lapplybankercondition(0);
		pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);

		uint32_t n = 0;
		uint32_t size = 6;
		int64_t nowScore = 0;
		int64_t xzScore = 0;
		shared_ptr<IServerUserItem> pSortUserItem;
		shared_ptr<UserInfo> sortUserInfoItem;
		for (auto &sortUserInfoItem : m_SortUserInfo)
		{
			pSortUserItem = m_pITableFrame->GetTableUserItem(sortUserInfoItem->chairId);
			if (!pSortUserItem)
				nowScore = 0;
			else
			{
				xzScore = sortUserInfoItem->lCurrentPlayerJetton;
				nowScore = pSortUserItem->GetUserScore() - xzScore;
			}

			PlayerListItem* palyer = pInfo->add_players();
			palyer->set_dwuserid(sortUserInfoItem->userId);
			palyer->set_headerid(sortUserInfoItem->headerId);
			palyer->set_nickname(sortUserInfoItem->nickName);
			palyer->set_luserscore(nowScore);
			palyer->set_ltwentywinscore(sortUserInfoItem->lTwentyWinScore);
			palyer->set_ltwentywincount(sortUserInfoItem->lTwentyWinCount);
			int shensuanzi = sortUserInfoItem->userId == m_iShenSuanZiUserId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(n + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
			palyer->set_szlocation(sortUserInfoItem->location);

			if (++n > size)
				break;
		}

		pInfo->set_wbankerchairid(0);
		pInfo->set_cbbankertimes(0);
		pInfo->set_lbankerwinscore(0);
		pInfo->set_lbankerscore(0);
		pInfo->set_benablesysbanker(true);
		pInfo->set_szgameroomname("bjl");

		for (auto &jetton : m_He)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(1);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_Zhuang)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(2);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_Xian)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(3);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_HePair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(3);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_ZhuangPair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(4);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_XianPair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(5);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (int i = 0;i < MAX_COUNT;i++)
		{
			pInfo->add_notopencount(m_notOpenCount_last[i]);
		}
		//限红
		for (int i = 0;i < 2;i++)
		{
			pInfo->add_llimitscore(m_lLimitScore[i]);
		}
		pInfo->set_tableid(m_strTableId);
		pInfo->set_nleftcardcount(m_nLeftCardCount);
		pInfo->set_ncutcardid(m_nCutCardId);
		pInfo->set_njuhaoid(m_nJuHaoId+1);
		pInfo->set_lonlinepopulation(m_UserInfo.size());

		pWashCard.set_cbwashcardtime(m_nWashCardTime);
		chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
		int32_t leaveTime = m_nWashCardTime - durationTime.count();
		pWashCard.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

		xzScore = userInfoItem->lCurrentPlayerJetton;
		bool isRobot = pUserItem->IsAndroidUser();
		nowScore = pUserItem->GetUserScore() - xzScore;
		int64_t maxscore = nowScore >= 50 ? nowScore : 0;
		pWashCard.set_lusermaxscore(maxscore);
		pWashCard.set_luserscore(nowScore);

		std::string data = pWashCard.SerializeAsString();
		m_pITableFrame->SendTableData(userInfoItem->chairId, bjl::SUB_S_WASH_CARD, (uint8_t*)data.c_str(), data.size());
	}
	m_dwReadyTime = (uint32_t)time(NULL);
	m_TimerIdWashCard = m_TimerLoopThread->getLoop()->runAfter(m_nWashCardTime, boost::bind(&CTableFrameSink::OnTimerSendCard, this));
}

void CTableFrameSink::OnTimerSendCard()
{
	m_cbGameStatus = BJL_GAME_STATUS_SENDCARD;
	m_gameStateStartTime = chrono::system_clock::now();
	LOG(INFO) << "游戏:m_strRoundId=" << m_strRoundId << ",进入发牌牌状态：" << m_cbGameStatus;
	openLog("本局[%d]游戏开始发牌:m_strRoundId=%s,m_nSendCardIndex=%d,m_nCutCardId=%d", m_nJuHaoId, m_strRoundId.c_str(), m_nSendCardIndex, m_nCutCardId);
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		//userInfoItem->Isplaying = true;
		CMD_S_StartSendCard pStartSendCard;
		GameDeskInfo* pInfo = pStartSendCard.mutable_deskdata();
		for (int i = 0;i < 7;i++)
		{
			pInfo->add_userchips(m_currentchips[userInfoItem->chairId][i]);
		}
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
			alljitton->set_dwuserid(0);
			alljitton->set_ljettonscore(m_lAllJettonScore[i]);
			alljitton->set_ljettonarea(i);
		}
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
			userjetton->set_dwuserid(userInfoItem->userId);
			userjetton->set_ljettonarea(i);
			userjetton->set_ljettonscore(userInfoItem->dAreaJetton[i]);
		}
		//divineJettonScore
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
			divineJetton->set_dwuserid(m_iShenSuanZiUserId);
			shared_ptr<IServerUserItem> pUserItem = m_pITableFrame->GetUserItemByUserId(m_iShenSuanZiUserId);
			if (pUserItem)
				divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
			else
				divineJetton->set_ljettonscore(0);
			divineJetton->set_ljettonarea(i);
		}
		pInfo->set_lapplybankercondition(0);
		pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);

		uint32_t n = 0;
		uint32_t size = 6;
		int64_t nowScore = 0;
		int64_t xzScore = 0;
		shared_ptr<IServerUserItem> pSortUserItem;
		shared_ptr<UserInfo> sortUserInfoItem;
		for (auto &sortUserInfoItem : m_SortUserInfo)
		{
			pSortUserItem = m_pITableFrame->GetTableUserItem(sortUserInfoItem->chairId);
			if (!pSortUserItem)
				nowScore = 0;
			else
			{
				xzScore = sortUserInfoItem->lCurrentPlayerJetton;
				nowScore = pSortUserItem->GetUserScore() - xzScore;
			}

			PlayerListItem* palyer = pInfo->add_players();
			palyer->set_dwuserid(sortUserInfoItem->userId);
			palyer->set_headerid(sortUserInfoItem->headerId);
			palyer->set_nickname(sortUserInfoItem->nickName);
			palyer->set_luserscore(nowScore);
			palyer->set_ltwentywinscore(sortUserInfoItem->lTwentyWinScore);
			palyer->set_ltwentywincount(sortUserInfoItem->lTwentyWinCount);
			int shensuanzi = sortUserInfoItem->userId == m_iShenSuanZiUserId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(n + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
			palyer->set_szlocation(sortUserInfoItem->location);

			if (++n > size)
				break;
		}

		pInfo->set_wbankerchairid(0);
		pInfo->set_cbbankertimes(0);
		pInfo->set_lbankerwinscore(0);
		pInfo->set_lbankerscore(0);
		pInfo->set_benablesysbanker(true);
		pInfo->set_szgameroomname("bjl");
	
		for (auto &jetton : m_He)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(1);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_Zhuang)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(2);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_Xian)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(3);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_HePair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(3);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_ZhuangPair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(4);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (auto &jetton : m_XianPair)
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(5);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
		for (int i = 0;i < MAX_COUNT;i++)
		{
			pInfo->add_notopencount(m_notOpenCount_last[i]);
		}
		//限红
		for (int i = 0;i < 2;i++)
		{
			pInfo->add_llimitscore(m_lLimitScore[i]);
		}
		pInfo->set_tableid(m_strTableId);
		pInfo->set_nleftcardcount(m_nLeftCardCount);
		pInfo->set_ncutcardid(m_nCutCardId);
		pInfo->set_njuhaoid(m_nJuHaoId+1);
		pInfo->set_lonlinepopulation(m_UserInfo.size());
		pStartSendCard.set_roundid(m_strRoundId);
		pStartSendCard.set_cbsendcardtime(m_nSendCardTime);
		chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
		int32_t leaveTime = m_nSendCardTime - durationTime.count();
		pStartSendCard.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

		xzScore = userInfoItem->lCurrentPlayerJetton;
		bool isRobot = pUserItem->IsAndroidUser();
		nowScore = pUserItem->GetUserScore() - xzScore;
		int64_t maxscore = nowScore >= 50 ? nowScore : 0;
		pStartSendCard.set_lusermaxscore(maxscore);
		pStartSendCard.set_luserscore(nowScore);

		std::string data = pStartSendCard.SerializeAsString();
		m_pITableFrame->SendTableData(userInfoItem->chairId, bjl::SUB_S_START_SENDCARD, (uint8_t*)data.c_str(), data.size());
	}
	m_dwReadyTime = (uint32_t)time(NULL);
	m_TimerIdSendCard = m_TimerLoopThread->getLoop()->runAfter(m_nSendCardTime, boost::bind(&CTableFrameSink::OnTimerStartPlaceJetton, this));
}

void CTableFrameSink::OnTimerStartPlaceJetton()
{
	m_bGameEnd = false;
	m_pITableFrame->SetGameStatus(GAME_STATUS_START);
	m_cbGameStatus = BJL_GAME_STATUS_PLACE_JETTON;
	m_gameStateStartTime = chrono::system_clock::now();
	m_nWinIndex = -1;
	LOG(INFO) << "OnTimerStartPlaceJetton UserCount:" << m_UserInfo.size() << ", m_strRoundId = " << m_strRoundId;
	openLog("本局[%d]游戏开始下注:m_strRoundId=%s,m_nSendCardIndex=%d,m_nCutCardId=%d", m_nJuHaoId, m_strRoundId.c_str(), m_nSendCardIndex, m_nCutCardId);

	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		SendStartJettonMsg(userInfoItem->chairId);
	}
	m_dwReadyTime = (uint32_t)time(NULL);
	//设置时间
	m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
	m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast, boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
	//openLog("OnTimerStartPlaceJetton 启动广播定时器:  playerCount=%d", m_UserInfo.size());
}

void CTableFrameSink::OnTimerJetton()
{
    m_endTime = chrono::system_clock::now();
	m_gameStateStartTime = chrono::system_clock::now();
    //if(--m_nPlaceJettonLeveTime < 1)
    {
		m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);
		//GameTimerJettonBroadcast();
		//结束游戏
		OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

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
		m_nJuHaoId++;
		int32_t allTime = m_nGameEndTime;
		m_fResetPlayerTime = m_fOpenCardTime * 2 + 0.5f;
		//补牌时间
		float bupaiTime = 0;
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			bupaiTime += m_fBupaiTime;
			m_fResetPlayerTime += m_fBupaiTime;
		}
		if (m_cbCardCount[INDEX_PLAYER] == 3)
		{
			bupaiTime += m_fBupaiTime;
			m_fResetPlayerTime += m_fBupaiTime;
		}
		allTime += ceil(bupaiTime);
		m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(allTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));
		m_TimerIdResetPlayerList = m_TimerLoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(CTableFrameSink::RefreshPlayerList, this, true));

        WriteUserScore();
        ChangeUserInfo();
        SendWinSysMessage();

        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_END);
		m_cbGameStatus = BJL_GAME_STATUS_END;
		m_gameStateStartTime = chrono::system_clock::now();
        m_replay.clear();
        //删除玩家信息wwwwwww
        clearTableUser();  //sOffline  m_cbPlayStatus==false
        ChangeVipUserInfo();

        //ClearTableData();
		//openLog("end usercoount=%d ",m_UserInfo.size());
		LOG(INFO) << "GameEnd UserCount:" << m_UserInfo.size();
    }
}

void CTableFrameSink::OnTimerEnd()
{
    //连续5局未下注踢出房间
	if (m_NotBetJettonKickOutCount > 0)
	{
		CheckKickOutUser();
	}
	m_bGameEnd = false;
	if (m_nLeftCardCount <= m_nCutCardId)
	{
		openLog("游戏本轮发牌结束,开始下一轮:m_strRoundId=%s,m_nLeftCardCount=%d,m_nCutCardId=%d", m_strRoundId.c_str(), m_nLeftCardCount, m_nCutCardId);
	}
    m_strRoundId = m_pITableFrame->GetNewRoundId();
	m_strTableId = to_string(m_ipCode) + "-" + to_string(m_pITableFrame->GetTableId() + 1);	//桌号
    m_pITableFrame->SetGameStatus(GAME_STATUS_START);
	
    m_startTime = chrono::system_clock::now();
	m_gameStateStartTime = chrono::system_clock::now();
    m_replay.gameinfoid = m_strRoundId;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	m_nWinIndex = -1;

	clearTableUser();  //sOffline  m_cbPlayStatus==false
	ClearTableData();
	ChangeVipUserInfo();
	ReadConfigInformation();//重新读取配置
	memset(m_cbTestBuCard, 0, sizeof(m_cbTestBuCard));
	loadConf();
	//结束游戏
	bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
	if (!IsContinue)
		return;

	LOG(INFO) << "GameStart UserCount:" << m_UserInfo.size() << ", m_strRoundId = " << m_strRoundId;

	bool bRestart = false;
	if (m_nLeftCardCount <= m_nCutCardId)
	{
		m_CardVecData.clear();
		m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
		random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
		m_nCutCardId = m_random.betweenInt(m_lLimitQiePai[0], m_lLimitQiePai[1]).randInt_mt(true); //1.5副到3副牌之间
		m_nSendCardIndex = 0;
		m_nLeftCardCount = MAX_CARDS;
		m_nJuHaoId = 0;
		//clear open record
		m_openRecord.clear();
		bRestart = true;
		LOG(INFO) << "本轮发牌结束,开始下一轮";	
		openLog("游戏开始新一轮:m_strRoundId=%s,m_strTableId=%s,m_nLeftCardCount=%d,m_nCutCardId=%d", m_strRoundId.c_str(), m_strTableId.c_str(), m_nLeftCardCount, m_nCutCardId);
		m_cbGameStatus = BJL_GAME_STATUS_WASHCARD;
		memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
		memset(m_cbCardTypeCount, 0, sizeof(m_cbCardTypeCount));
		memset(m_notOpenCount, 0, sizeof(m_notOpenCount));
		memset(m_notOpenCount_last, 0, sizeof(m_notOpenCount_last));
		if (bRestart)
		{
			shared_ptr<IServerUserItem> pUserItem;
			shared_ptr<UserInfo> userInfoItem;
			for (auto &it : m_UserInfo)
			{
				userInfoItem = it.second;
				pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
				if (!pUserItem)
					continue;
				//userInfoItem->Isplaying = true;
				SendOpenRecord(userInfoItem->chairId);
			}
		}
		OnTimerWashCard();
	}
	else
	{
		OnTimerSendCard();
	}
}

void CTableFrameSink::ClearTableData()
{
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		if (userInfoItem->lLastJetton == 0)
		{
			userInfoItem->clear();
			continue;
		}
		// 否则把重押分数记录
		for (int j = 0; j < BET_ITEM_COUNT; j++)
		{
			userInfoItem->dLastJetton[j] = userInfoItem->dAreaJetton[j];
		}
		userInfoItem->clear();
	}

    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));
    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

	m_Zhuang.clear();
	m_Xian.clear();
	m_He.clear();
	m_ZhuangPair.clear();
	m_XianPair.clear();
	m_HePair.clear();
    for(int64_t chips : m_cbJettonMultiple)
    {
        m_Zhuang[chips] = 0;
        m_Xian[chips] = 0;
        m_He[chips] = 0;
		m_ZhuangPair[chips] = 0;
		m_XianPair[chips] = 0;
		m_HePair[chips] = 0;
    }
    memset(m_cbTableCards, 0, sizeof(m_cbTableCards)); 
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
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //::muduo::MutexLockGuard lock(m_mutexLock);
    //LOG(INFO) << "CTableFrameSink::OnGameMessage wChairID:" << wChairID << ", wSubCmdID:" << wSubCmdID << ", wDataSize:" << wDataSize;

    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if (!pIServerUserItem)
    {
        LOG(INFO) << "OnGameMessage pServerOnLine is NULL  wSubCmdID:" << wSubCmdID;
		return false;
    }

	switch (wSubCmdID)
	{
		case bjl::SUB_C_QUERY_CUR_STATE: //获取当前状态
		{
			QueryCurState(wChairID, wSubCmdID, pData, wDataSize);
			break;
		}
        case bjl::SUB_C_PLACE_JETTON:			//用户下注
		{
            //变量定义
            CMD_C_PlaceJet  pJETTON;
            pJETTON.ParseFromArray(pData,wDataSize);

			//消息处理
            return OnUserAddScore(wChairID, pJETTON.cbjettonarea(), pJETTON.ljettonscore());
		}
		case bjl::SUB_C_USER_REPEAT_JETTON:
		{
			Rejetton(wChairID, wSubCmdID, pData, wDataSize); //续押
			return true;
		}
		case bjl::SUB_C_USER_DOUBLEORCANCEL_BET:
		{
			DoubleOrCanceljetton(wChairID, wSubCmdID, pData, wDataSize); //加倍或取消投注
			return true;
		}
        case SUB_C_QUERY_PLAYERLIST:
        {
			GamePlayerQueryPlayerList(wChairID, wSubCmdID, pData, wDataSize);
            return true;
        }
        case bjl::SUB_C_SYNC_TIME:
        {
            int32_t status = m_cbGameStatus; //m_pITableFrame->GetGameStatus();
            CMD_S_SyncTime_Res res;

            res.set_userid(pIServerUserItem->GetUserId());
            res.set_status(status);

            chrono::system_clock::time_point curTime = chrono::system_clock::now();
            int32_t leaveTime = 0;
            switch (status)
            {
            case GAME_STATUS_START:
                leaveTime = m_nPlaceJettonTime - (chrono::duration_cast<chrono::seconds>(curTime - m_gameStateStartTime)).count();
                break;
            case GAME_STATUS_END:
				//补牌时间
				int32_t allTime = m_nGameEndTime;
				float bupaiTime = 0;
				if (m_cbCardCount[INDEX_BANKER] == 3)
				{
					bupaiTime += m_fBupaiTime;
				}
				if (m_cbCardCount[INDEX_PLAYER] == 3)
				{
					bupaiTime += m_fBupaiTime;
				}
				allTime += ceil(bupaiTime);
                leaveTime = allTime - (chrono::duration_cast<chrono::seconds>(curTime - m_gameStateStartTime)).count();
				break;
            }
            res.set_timeleave(leaveTime >0 ? leaveTime : 0);
            std::string data = res.SerializeAsString();
            m_pITableFrame->SendTableData(wChairID, bjl::SUB_S_SYNC_TIME, (uint8_t*)data.c_str(), data.size());
        }
        break;
        return false;
    }
}

//加注事件
bool CTableFrameSink::OnUserAddScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore,bool bReJetton)
{
    //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    if(m_cbGameStatus != BJL_GAME_STATUS_PLACE_JETTON)
    {
        LOG_ERROR << " Game Status is not GameStart ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
        return true;
    }
    //=====================================
    bool bFind = false;
	if (bReJetton)
	{
		int leng = m_cbJettonMultiple.size();
		for (int i = 0;i < leng;i++)
		{
			if (m_cbJettonMultiple[i] == lJettonScore)
			{
				bFind = true;
				break;
			}
		}
	} 
	else
	{
		for (int i = 0;i < 7;i++)
		{
			if (m_currentchips[wChairID][i] == lJettonScore)
			{
				bFind = true;
				break;
			}
		}
	}
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<lJettonScore;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 8);
        return true;
    }

    if(cbJettonArea < 0 || cbJettonArea >= BET_ITEM_COUNT)
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
    //if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
		int64_t lTotalJettonScore = userInfo->lCurrentPlayerJetton;//userInfo->dAreaJetton[1] + userInfo->dAreaJetton[2] + userInfo->dAreaJetton[3];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Real User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
            return true;
        }

        if(userInfo->dAreaJetton[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
            return true;
        }

        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        userInfo->dAreaJetton[cbJettonArea] += lJettonScore;
		userInfo->lCurrentPlayerJetton += lJettonScore;
		if (!pIServerUserItem->IsAndroidUser())
		{
			m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
			if (lTotalJettonScore == 0)
			{
				m_replay.addPlayer(userId, pIServerUserItem->GetAccount(), lUserScore, wChairID);
			}
			m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(lJettonScore), -1, opBet, wChairID, cbJettonArea);
		}

    }

    if(m_iShenSuanZiUserId)
    {
        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_iShenSuanZiUserId];
        if(SSZUserInfo->chairId == wChairID)
            m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }
	if (!pIServerUserItem->IsAndroidUser())
		LOG(INFO) << "下注成功: tableID = " << m_pITableFrame->GetTableId() << "  userid = "<< userId << " area = " << (int)cbJettonArea << " score = " << lJettonScore << " userPlayer = " << m_UserInfo.size();
    //openLog("下注成功: userid=%d area=%d score=%d  userPlayer=%d", userId, (int)cbJettonArea,lJettonScore, m_UserInfo.size());
    //成功返回
    if(cbJettonArea == 0)
    {
        m_He[lJettonScore]++;
    }
	else if(cbJettonArea == 1)
    {
        m_Zhuang[lJettonScore]++;
    }
	else if(cbJettonArea == 2)
    {
        m_Xian[lJettonScore]++;
    }
	else if (cbJettonArea == 3)
	{
		m_HePair[lJettonScore]++;
	}
	else if (cbJettonArea == 4)
	{
		m_ZhuangPair[lJettonScore]++;
	}
	else if (cbJettonArea == 5)
	{
		m_XianPair[lJettonScore]++;
	}
    
	int32_t playerCount = 0;
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

        int64_t xzScore = userInfo->lCurrentPlayerJetton;
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
            jettonScore = userInfoItem->dAreaJetton[i];
            userJetton->set_ljettonscore(jettonScore);
        }
        std::string data = Jetton.SerializeAsString();
        m_pITableFrame->SendTableData(sendChairId, bjl::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
		/*if (!Robot)
		{
			openLog("下注发送给真人: chairId=%d, userid=%d area=%d score=%d", userInfoItem->chairId, userInfoItem->userId, (int)cbJettonArea, lJettonScore);
		}*/
    };

    if(!isInVipList(wChairID) && userId != m_iShenSuanZiUserId && m_SortUserInfo.size() >= 8)
    {
		userInfoItem = m_UserInfo[userId];
		playerCount++;
		//openLog("下注发送notVipList: userid=%d area=%d score=%d", userId, (int)cbJettonArea, lJettonScore);
        push(wChairID, userId, pIServerUserItem->IsAndroidUser());
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
            if(!pUserItem)
                continue;
			playerCount++;
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }
	//openLog("下注发送完成:  playerCount=%d", playerCount);
    return true;
}

void CTableFrameSink::AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score, bool bCancel)
{
    assert(chairId<GAME_PLAYER &&areaId<MAX_COUNT);
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
    for(int i = 0; i < MAX_COUNT; ++i)
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

            for (int i = 0; i < MAX_COUNT; ++i)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[i]);
               jetton_broadcast.add_tmpjettonscore(m_array_tmparea_jetton[i] - m_array_jetton[userInfoItem->chairId].areaJetton[i]);
            }
            memset(m_array_jetton[userInfoItem->chairId].areaJetton, 0, sizeof(m_array_jetton[userInfoItem->chairId].areaJetton));
            std::string data = jetton_broadcast.SerializeAsString();
            m_pITableFrame->SendTableData(userInfoItem->chairId, bjl::SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
        }
        memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
		//openLog("GameTimerJettonBroadcast:  playerCount=%d", m_UserInfo.size());
    }
    if(m_cbGameStatus == BJL_GAME_STATUS_PLACE_JETTON)
    {
		//openLog("GameTimerJettonBroadcast 启动广播定时器:  playerCount=%d", m_UserInfo.size());
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
     m_pITableFrame->SendTableData(wChairID, bjl::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
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
            //sprintf(formate,"恭喜V%d玩家%s在庄闲中一把赢得%.2f元!",pUserItem->GetVipLevel(),pUserItem->GetNickName(),m_EndWinScore[i]);
            //m_pITableFrame->SendGameMessage(INVALID_CARD,formate,SMT_GLOBAL|SMT_SCROLL);
           m_pITableFrame->SendGameMessage(userInfoItem->chairId, "", SMT_GLOBAL|SMT_SCROLL, userInfoItem->m_EndWinScore);
        }
    }
}

string CTableFrameSink::GetCardValueName(uint8_t card)
{
    string strName;

    //获取花色
    uint8_t cbColor = m_GameLogic.GetCardColor(card);
    uint8_t cbValue = m_GameLogic.GetCardValue(card);
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

void  CTableFrameSink::openLog(const char *p, ...)
{
    if (m_IsEnableOpenlog)
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
		if (TEST)
		{
			sprintf(Filename, "./log/bjl/bjl_%d%d%d_%d_%d_TestTime_%d.log", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId(), m_nTestTime);
		} 
		else
		{
			sprintf(Filename, "./log/bjl/bjl_%d%d%d_%d_%d.log", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
		}
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
        if (userInfoItem->NotBetJettonCount >= m_NotBetJettonKickOutCount)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(userInfoItem->userId);            
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);            
            m_pITableFrame->ClearTableUser(userInfoItem->chairId,true,true);
            if(userInfoItem->userId == m_iShenSuanZiUserId)
            {
                m_iShenSuanZiUserId = 0;
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
	bjl::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分
	
	//系统开牌
	for (int i = 0; i < 2; ++i)
	{
		CardGroup* card = details.add_syscard();
		card->set_group_id(i+1);
		card->set_cards(&(m_cbTableCards[i])[0], m_cbCardCount[i]);
		card->set_cardpoint(m_cbCardPoint[i]);
		if (m_cbCardCount[i] == 3)
		{
			card->set_bneedcard(true);
		}
		else
		{
			card->set_bneedcard(false);
		}
	}

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		//和 庄 闲 和对 庄对 闲对: 0 1 2 3 4 5
		int32_t nWinIndex = 0;
		for (int betIdx = 0;betIdx < MAX_COUNT; betIdx++)
		{
			// 结算时记录
            int64_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->dAreaJetton[betIdx];
			nWinIndex = 0;
			if (betIdx == m_nWinIndex)
			{
				nWinIndex = 1;
			}
			if (betIdx >= 3)
			{
				if (m_cbCardPair[betIdx - 3] == 1)
				{
					nWinIndex = 1;
				}
			}
			//if (betScore > 0) 
			{
				bjl::BetAreaRecordDetail* detail = details.add_detail();
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

// 重押
void CTableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
	if (!Usr) { LOG(WARNING) << "----查找玩家不在----"; return; }

	if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
	{
		LOG(WARNING) << "-------------续押失败(玩家状态不对)-------------";
		return;
	}

	LOG(WARNING) << "-----------Rejetton续押-------------";
	// 游戏状态判断
	if (BJL_GAME_STATUS_PLACE_JETTON != m_cbGameStatus)
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_cbGameStatus << " " << BJL_GAME_STATUS_PLACE_JETTON;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[BET_ITEM_COUNT] = { 0 };
	int64_t userid = Usr->GetUserId();

	bjl::CMD_C_ReJetton reJetton;
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
		bjl::CMD_S_PlaceJettonFail placeJetFail;

		placeJetFail.set_dwuserid(userid);
		placeJetFail.set_cbjettonarea(3);
		placeJetFail.set_lplacescore(0);
		placeJetFail.set_cberrorcode(4);
		placeJetFail.set_cbandroid(Usr->IsAndroidUser());

		std::string sendData = placeJetFail.SerializeAsString();
		m_pITableFrame->SendTableData(Usr->GetChairId(), bjl::SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
		LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
		return;
	}

	for (int i = 0; i < BET_ITEM_COUNT; i++)
	{
		// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
		if (m_AreaMaxJetton[i] < m_UserInfo[userid]->dAreaJetton[i] + m_UserInfo[userid]->dLastJetton[i]) 
		{
			bjl::CMD_S_PlaceJettonFail placeJetFail;
			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i);
			placeJetFail.set_lplacescore(m_UserInfo[userid]->dLastJetton[i]);
			placeJetFail.set_cberrorcode(5);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());

			std::string data = placeJetFail.SerializeAsString();
			//if (m_bTest==0)
			m_pITableFrame->SendTableData(Usr->GetChairId(), bjl::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
			LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->dAreaJetton[i] << "+" << m_UserInfo[userid]->dLastJetton[i];
			return;
		}
	}
	// LOG(ERROR) << "---------重押成功--------";
	for (int i = 0; i < BET_ITEM_COUNT; i++)
	{
		if (areajetton[i] <= 0)
		{
			continue;
		}

		while (areajetton[i] > 0)
		{
			int jSize = m_cbJettonMultiple.size();
			for (int j = jSize - 1;j >= 0;j--)
			{
				if (areajetton[i] >= m_cbJettonMultiple[j])
				{
                    int64_t tmpVar = m_cbJettonMultiple[j];
					OnUserAddScore(chairid, i, tmpVar,true);
					areajetton[i] -= tmpVar;
					break;
				}
			}
		}
	}
}

//加倍或取消投注
void CTableFrameSink::DoubleOrCanceljetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
	if (!Usr) { LOG(WARNING) << "----DoubleOrCanceljetton 查找玩家不在----"; return; }

	if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
	{
		LOG(WARNING) << "-------------加倍或取消投注失败(玩家状态不对)-------------";
		return;
	}
	// 游戏状态判断
	if (BJL_GAME_STATUS_PLACE_JETTON != m_cbGameStatus)
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_cbGameStatus << " " << BJL_GAME_STATUS_PLACE_JETTON;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[MAX_COUNT] = { 0 };
	int64_t userid = Usr->GetUserId();

	bjl::CMD_C_DoubleOrCancelBet pDoubleOrCancelBet;
	pDoubleOrCancelBet.ParseFromArray(data, datasize);
	int64_t userId = pDoubleOrCancelBet.dwuserid();
	int32_t betType = pDoubleOrCancelBet.ibettype();
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
			return ;
		for (int i = 0; i < MAX_COUNT; i++)
		{
			if (areajetton[i] <= 0)
				continue;
			while (areajetton[i] > 0)
			{
				int jSize = m_cbJettonMultiple.size();
				for (int j = jSize - 1;j >= 0;j--)
				{
					if (areajetton[i] >= m_cbJettonMultiple[j])
					{
                        int64_t tmpVar = m_cbJettonMultiple[j];
						OnUserCancelPlaceScore(chairid, i, tmpVar, true);
						areajetton[i] -= tmpVar;
						break;
					}
				}
			}
		}
		break;
	}
	case 1://当前已下注翻倍
	{
		LOG(WARNING) << "-----------DoubleOrCanceljetton 加倍投注-------------";
		bool bHaveBet = false;
		for (int i = 0; i < MAX_COUNT; i++)
		{
			alljetton += m_UserInfo[userid]->dAreaJetton[i];
			areajetton[i] = m_UserInfo[userid]->dAreaJetton[i];
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
			bjl::CMD_S_PlaceJettonFail placeJetFail;

			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(3);
			placeJetFail.set_lplacescore(0);
			placeJetFail.set_cberrorcode(4);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());

			std::string sendData = placeJetFail.SerializeAsString();
			m_pITableFrame->SendTableData(Usr->GetChairId(), bjl::SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
			LOG(WARNING) << "------------------加倍投注失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
			return;
		}

		for (int i = 0; i < BET_ITEM_COUNT; i++)
		{
			// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
			if (m_AreaMaxJetton[i] < m_UserInfo[userid]->dAreaJetton[i] + areajetton[i])
			{
				//bjl::CMD_S_PlaceJettonFail placeJetFail;
				//placeJetFail.set_dwuserid(userid);
				//placeJetFail.set_cbjettonarea(i);
				//placeJetFail.set_lplacescore(m_UserInfo[userid]->dAreaJetton[i]);
				//placeJetFail.set_cberrorcode(5);
				//placeJetFail.set_cbandroid(Usr->IsAndroidUser());
				//std::string data = placeJetFail.SerializeAsString();
				////if (m_bTest==0)
				//m_pITableFrame->SendTableData(Usr->GetChairId(), bjl::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
				areajetton[i] = 0;
				LOG(WARNING) << "---------加倍投注失败(超过区域最大押分)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->dAreaJetton[i] << "+" << areajetton[i] << "  area: " << i;
				//return;
			}
		}
		// LOG(ERROR) << "---------加倍投注成功--------";
		for (int i = 0; i < MAX_COUNT; i++)
		{
			if (areajetton[i] <= 0)
			{
				continue;
			}

			while (areajetton[i] > 0)
			{
				int jSize = m_cbJettonMultiple.size();
				for (int j = jSize - 1;j >= 0;j--)
				{
					if (areajetton[i] >= m_cbJettonMultiple[j])
					{
                        int64_t tmpVar = m_cbJettonMultiple[j];
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
	if (m_cbGameStatus != BJL_GAME_STATUS_PLACE_JETTON)
	{
		LOG_ERROR << " Game Status is not GameStart ";
		//SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
		return true;
	}
	//=====================================
	bool bFind = false;
	if (bCancelJetton)
	{
		int leng = m_cbJettonMultiple.size();
		for (int i = 0;i < leng;i++)
		{
			if (m_cbJettonMultiple[i] == lJettonScore)
			{
				bFind = true;
				break;
			}
		}
	}
	else
	{
		for (int i = 0;i < 7;i++)
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

	if (cbJettonArea < 0 || cbJettonArea >= BET_ITEM_COUNT)
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
	if(lJettonScore > 0 && userInfo->lCurrentPlayerJetton > 0 && userInfo->dAreaJetton[cbJettonArea] >= lJettonScore)  //real user
	{
		//已经下的总注
		int64_t lTotalJettonScore = userInfo->lCurrentPlayerJetton;//userInfo->dAreaJetton[1] + userInfo->dAreaJetton[2] + userInfo->dAreaJetton[3];
		/*if (lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
		{
			LOG_ERROR << " Real User Score is not enough ";
			SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
			return true;
		}

		if (userInfo->dAreaJetton[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
		{
			LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
			SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
			return true;
		}*/
		m_lAllJettonScore[cbJettonArea] -= lJettonScore;
		userInfo->dAreaJetton[cbJettonArea] -= lJettonScore;
		userInfo->lCurrentPlayerJetton -= lJettonScore;
		if (!pIServerUserItem->IsAndroidUser())
		{
			m_lTrueUserJettonScore[cbJettonArea] -= lJettonScore;
			m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(lJettonScore), -1, opAddBet, wChairID, cbJettonArea);
		}

	}

	if (m_iShenSuanZiUserId)
	{
		shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_iShenSuanZiUserId];
		if (SSZUserInfo->chairId == wChairID && m_lShenSuanZiJettonScore[cbJettonArea] >= lJettonScore)
			m_lShenSuanZiJettonScore[cbJettonArea] -= lJettonScore;
	}
	LOG(INFO) << "取消下注成功:userid = " << userId << " area = " << (int)cbJettonArea << " score = " << lJettonScore;
	openLog("取消下注:userid=%d area=%d score=%f", userId,cbJettonArea,lJettonScore);
	//成功返回
	if (lJettonScore > 0 && userInfo->dAreaJetton[cbJettonArea] >= lJettonScore)  //real user
	{
		if (cbJettonArea == 0)
		{
			if(m_He[lJettonScore] > 0)
				m_He[lJettonScore]--;
		}
		else if (cbJettonArea == 1)
		{
			if (m_Zhuang[lJettonScore] > 0)
				m_Zhuang[lJettonScore]--;
		}
		else if (cbJettonArea == 2)
		{
			if (m_Xian[lJettonScore] > 0)
				m_Xian[lJettonScore]--;
		}
		else if (cbJettonArea == 3)
		{
			if (m_HePair[lJettonScore] > 0)
				m_HePair[lJettonScore]--;
		}
		else if (cbJettonArea == 4)
		{
			if (m_ZhuangPair[lJettonScore] > 0)
				m_ZhuangPair[lJettonScore]--;
		}
		else if (cbJettonArea == 5)
		{
			if (m_XianPair[lJettonScore] > 0)
				m_XianPair[lJettonScore]--;
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

		int64_t xzScore = userInfo->lCurrentPlayerJetton;
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
			jettonScore = userInfoItem->dAreaJetton[i];
			userJetton->set_ljettonscore(jettonScore);
		}
		std::string data = pCanclePlaceJet.SerializeAsString();
		m_pITableFrame->SendTableData(sendChairId, bjl::SUB_S_CANCELPLACE_JET, (uint8_t*)data.c_str(), data.size());
	};

	if (!isInVipList(wChairID) && userId != m_iShenSuanZiUserId && m_SortUserInfo.size() >= 8)
	{
		userInfoItem = m_UserInfo[userId];
		push(wChairID, userId, pIServerUserItem->IsAndroidUser());
		AddPlayerJetton(wChairID, cbJettonArea, lJettonScore,true);
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

void CTableFrameSink::GamePlayerQueryPlayerList(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd)
{
	shared_ptr<CServerUserItem> pIIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
	if (!pIIServerUserItem)
		return;

	m_pPlayerInfoList.clear();

	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		if (!userInfoItem->Isplaying)
		{
			continue;
		}
		m_pPlayerInfoList.push_back(userInfoItem);
	}

	bjl::CMD_C_PlayerList querylist;
	querylist.ParseFromArray(data, datasize);
	int32_t limitcount = querylist.nlimitcount();
	int32_t beginindex = querylist.nbeginindex();
	int32_t resultcount = 0;
	int32_t endindex = 0;
	bjl::CMD_S_PlayerList playerlist;
	int32_t wMaxPlayer = m_pITableFrame->GetMaxPlayerCount();
	if (!limitcount)
		limitcount = wMaxPlayer;
	limitcount = 20;
	int index = 0;
	PlayerRichSorting(false);
	SortPlayerList();
	for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
	{
		shared_ptr<CServerUserItem> pUser = this->m_pITableFrame->GetTableUserItem((*it).chairId);
		if (!pUser)
			continue;
		if (!(*it).Isplaying)
			continue;
		if (pUser->GetHeaderId() > 12)
		{
			LOG_INFO << " ========== 2 userId " << pUser->GetUserId() << " == headerId:" << pUser->GetHeaderId();
		}
		bjl::PlayerListItem *item = playerlist.add_players();
		item->set_dwuserid(pUser->GetUserId());
		item->set_headerid(pUser->GetHeaderId());
		item->set_nviplevel(0); //pUser->GetVipLevel());
		item->set_nickname(pUser->GetNickName());
		item->set_headboxid(0); //pUser->GetHeadBoxID());
		if (!m_bGameEnd)
		{
			item->set_luserscore(pUser->GetUserScore() - (*it).lLastJetton);
		}
		else
		{
			item->set_luserscore(pUser->GetUserScore());
		}
		item->set_isdivinemath(0);
		if (pUser->GetUserId() == m_iShenSuanZiUserId)
		{
			item->set_isdivinemath(1);
		}
		item->set_index(index);
		item->set_ltwentywinscore((*it).GetTwentyWinScore());
		item->set_ltwentywincount((*it).GetTwentyWin());

		item->set_gender(0); //pUser->GetGender());
		item->set_luserwinscore((*it).m_EndWinScore);
		endindex = index + 1;
		index++;
		resultcount++;
		if (resultcount >= limitcount)
		{
			break;
		}
	}
	playerlist.set_nendindex(endindex);
	playerlist.set_nresultcount(resultcount);
	std::string lstData = playerlist.SerializeAsString();
	m_pITableFrame->SendTableData(pIIServerUserItem->GetChairId(), bjl::SUB_S_QUERY_PLAYLIST, (uint8_t *)lstData.c_str(), lstData.size());
	PlayerRichSorting();
}

// 玩家财富排序
void CTableFrameSink::PlayerRichSorting(bool iChooseShenSuanZi)
{
	if (m_pPlayerInfoList.size() > 1)
	{
		if (iChooseShenSuanZi)
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions);
			sort(++m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
			m_iShenSuanZiUserId = m_pPlayerInfoList[0]->userId;
		}
		else
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
		}
	}

	if (m_bRefreshPlayerList)
	{
		SortPlayerList();
	}
}

void CTableFrameSink::SortPlayerList()
{
	for (int i = 0;i < m_pPlayerInfoList_20.size();i++)
	{
		for (int j = i + 1;j < m_pPlayerInfoList_20.size();j++)
		{
			UserInfo userinfo;
			if (m_pPlayerInfoList_20[i].GetTwentyWinScore() < m_pPlayerInfoList_20[j].GetTwentyWinScore())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyWinScore() == m_pPlayerInfoList_20[j].GetTwentyWinScore()
				&& m_pPlayerInfoList_20[i].GetTwentyJetton() < m_pPlayerInfoList_20[j].GetTwentyJetton())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyJetton() == m_pPlayerInfoList_20[j].GetTwentyJetton()
				&& m_pPlayerInfoList_20[i].GetTwentyWin() < m_pPlayerInfoList_20[j].GetTwentyWin())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
		}

	}
}

void CTableFrameSink::RefreshPlayerList(bool isGameEnd)
{
	m_pPlayerInfoList_20.clear();
	shared_ptr<IServerUserItem> pServerUserItem;
	for (int i = 0;i < m_pPlayerInfoList.size();i++)
	{
		if (!m_pPlayerInfoList[i])
			continue;
		pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
		if (!pServerUserItem)
			continue;
		m_pPlayerInfoList[i]->GetTwentyWinScore();
		m_pPlayerInfoList[i]->RefreshGameRusult(isGameEnd);
	}

	for (int i = 0;i < m_pPlayerInfoList.size();i++)
	{
		if (!m_pPlayerInfoList[i])
			continue;
		pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
		if (!pServerUserItem)
			continue;
		m_pPlayerInfoList_20.push_back((*m_pPlayerInfoList[i]));
		/*LOG(ERROR) << "old二十局赢分:" << (m_pPlayerInfoList[i]->GetTwentyWinScore());
		LOG(ERROR) << "二十局赢分:" << (m_pPlayerInfoList_20[i].GetTwentyWinScore());*/
	}
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
	m_openRecord.push_back(m_nWinIndex);

	m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_nGoodRouteType = m_GameLogic.getGoodRouteType(m_openRecord);
	m_strGoodRouteName = m_GameLogic.getGoodRouteName(m_nGoodRouteType);

	m_bRefreshPlayerList = true;
	PlayerRichSorting();
	//游戏记录
	updateResultList(m_nWinIndex);
	//下注项累计开奖的次数
	m_winOpenCount[m_nWinIndex]++;
	for (int i = 0;i < 3;i++)
	{
		if (m_cbCardPair[i])
		{
			m_winOpenCount[i + 3]++;
		}
	}
	string stsRsult = "";
	for (int i = 0;i < MAX_COUNT;i++)
	{
		stsRsult += to_string(i) + "_" + to_string(m_winOpenCount[i]) + " ";
	}
	openLog("开奖结果累计 stsRsult = %s", stsRsult.c_str());
	//更新开奖结果
	memcpy(m_notOpenCount_last, m_notOpenCount, sizeof(m_notOpenCount_last));

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

//更新本轮开奖结果信息
void CTableFrameSink::updateResultList(int resultId)
{
	for (int i = 0;i < MAX_COUNT;++i)
	{
		if (i<3)
		{
			if (i == resultId)
			{
				m_notOpenCount[i] = 0;
			}
			else
			{
				m_notOpenCount[i]++;
			}
		} 
		else
		{
			if (m_cbCardPair[i-3] > 0)
			{
				m_notOpenCount[i] = 0;
			}
			else
			{
				m_notOpenCount[i]++;
			}
		}
	}
}
//获取一张牌
uint8_t CTableFrameSink::getOneCard(int8_t winTag, int32_t bankerPoint, int32_t playerPoint, int8_t &winTagChang, int32_t &bankerIndex, int32_t &playerIndex, bool bBanker)
{
	uint8_t card = 0;
	int32_t pkSize = m_vecData.size();
	int32_t bankerAllPoint = 0;
	int32_t playerAllPoint = 0;
	switch (winTag)
	{
	case 0:
		do
		{
			if (!bBanker) // 闲家补牌
			{				
				playerIndex = m_random.betweenInt(0, pkSize-1).randInt_mt(true);
				card = m_vecData[playerIndex];
				playerAllPoint = (playerPoint + m_GameLogic.GetCardPip(card)) % 10;
				if (bankerPoint == playerAllPoint)
					winTagChang = 0;
				else if(bankerPoint > playerAllPoint)
					winTagChang = 1;
				else
					winTagChang = 2;

				if (m_GameLogic.IsCardValid(card))
				{
					openLog("闲家补牌成功 VecSize:%d, winFlag: %d, winTagChang: %d, bankerPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, (int)winTagChang, bankerPoint, playerAllPoint, card);
					break;
				}
			}
			else
			{
				int32_t  i = 0;
				bool isOK = false;
				for (i = 0; i < pkSize; i++)
				{
					if (i == playerIndex) continue;
					bankerIndex = i;
					card = m_vecData[i];
					bankerAllPoint = (bankerPoint + m_GameLogic.GetCardPip(card)) % 10;
					if (bankerAllPoint == playerPoint)
					{
						isOK = true;
						openLog("庄家补牌成功 VecSize:%d, winFlag: %d,bankerAllPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, bankerAllPoint, playerPoint, card);
						break;
					}
				}
				if (isOK || i == pkSize)
				{
					if (i == pkSize)
						openLog("庄家补牌失败 VecSize:%d, winFlag: %d,i:%d", pkSize, (int)winTag, i);
					break;
				}
			}
		} while (true);
		break;
	case 1:
		do
		{
			if (!bBanker) // 闲家补牌
			{
				playerIndex = m_random.betweenInt(0, pkSize - 1).randInt_mt(true);
				card = m_vecData[playerIndex];
				playerAllPoint = (playerPoint + m_GameLogic.GetCardPip(card)) % 10;
				if (bankerPoint == playerAllPoint)
					winTagChang = 0;
				else if (bankerPoint > playerAllPoint)
					winTagChang = 1;
				else
					winTagChang = 2;

				if (m_GameLogic.IsCardValid(card) && playerAllPoint < 8)
				{
					openLog("闲家补牌成功 VecSize:%d, winFlag: %d, winTagChang: %d, bankerPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, (int)winTagChang, bankerPoint, playerAllPoint, card);
					break;
				}
			}
			else
			{
				int32_t  i = 0;
				bool isOK = false;
				for (i = 0; i < pkSize; i++)
				{
					if (i == playerIndex) continue;
					bankerIndex = i;
					card = m_vecData[i];
					bankerAllPoint = (bankerPoint + m_GameLogic.GetCardPip(card)) % 10;
					if (bankerAllPoint > playerPoint)
					{
						isOK = true;
						openLog("庄家补牌成功 VecSize:%d, winFlag: %d,bankerAllPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, bankerAllPoint, playerPoint, card);
						break;
					}
				}
				if (isOK || i == pkSize)
				{
					if (i == pkSize)
						openLog("庄家补牌失败 VecSize:%d, winFlag: %d,i:%d", pkSize, (int)winTag, i);
					break;
				}
			}
		} while (true);
		break;
	case 2:
		do
		{
			if (!bBanker) // 闲家补牌
			{
				playerIndex = m_random.betweenInt(0, pkSize - 1).randInt_mt(true);
				card = m_vecData[playerIndex];
				playerAllPoint = (playerPoint + m_GameLogic.GetCardPip(card)) % 10;
				if (bankerPoint == playerAllPoint)
					winTagChang = 0;
				else if (bankerPoint > playerAllPoint)
					winTagChang = 1;
				else
					winTagChang = 2;

				if (m_GameLogic.IsCardValid(card) && playerAllPoint >= 7)
				{
					openLog("闲家补牌成功 VecSize:%d, winFlag: %d, winTagChang: %d, bankerPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, (int)winTagChang, bankerPoint, playerAllPoint, card);
					break;
				}
			}
			else
			{
				int32_t  i = 0;
				bool isOK = false;
				for (i = 0; i < pkSize; i++)
				{
					if (i == playerIndex) continue;
					bankerIndex = i;
					card = m_vecData[i];
					bankerAllPoint = (bankerPoint + m_GameLogic.GetCardPip(card)) % 10;
					if (bankerAllPoint < playerPoint)
					{
						isOK = true;
						openLog("庄家补牌成功 VecSize:%d, winFlag: %d,bankerAllPoint:%d,playerPoint:%d,card=0x%02x", pkSize, (int)winTag, bankerAllPoint, playerPoint, card);
						break;
					}
				}
				if (isOK || i == pkSize)
				{
					if (i == pkSize)
						openLog("庄家补牌失败 VecSize:%d, winFlag: %d,i:%d", pkSize, (int)winTag, i);
					break;
				}
			}
		} while (true);
		break;
	default:
		LOG(INFO) << "......winTag:" << winTag << " Error!";
	}

	return card;
}
void CTableFrameSink::CalculatWeight()
{
	// 重新计算权重
	int count = MAX_COUNT;
	int nAllMuilt[MAX_COUNT] = { 9,2,2,76,12,12 };
	int retWeight[MAX_COUNT] = { 0 };
	float weight = 0.0f;
	int allWeight = 0;
	string strWeight = "";
	for (int i = 0;i < count;++i)
	{
		weight = 1.0f * 10000.0f / nAllMuilt[i];
		retWeight[i] = (int)weight;
		allWeight += retWeight[i];
	}
	if (allWeight < 10000)
	{
		int lessweight = (10000 - allWeight) / count;
		int less = 10000 - allWeight - lessweight * count;
		for (int i = 0;i < count;++i)
		{
			retWeight[i] += lessweight;
		}
		retWeight[0] += less;
	}
	else
	{
		int moreNum = allWeight - 10000;
		int lessweight = 0;
		int less = 0;
		for (int i = 0;i < count;++i)
		{
			lessweight = 1.0f * moreNum * retWeight[i] / allWeight;
			if (retWeight[i] >= lessweight)
			{
				retWeight[i] -= lessweight;
				less += lessweight;
			}
			else
			{
				less += retWeight[i];
				retWeight[i] = 0;
			}
		}
		retWeight[0] -= (moreNum - less);
		for (int i = 0;i < count;++i)
		{
			if (retWeight[i] < 0)
			{
				break;
			}
		}
	}
	allWeight = 0;
	string str = "";
	for (int i = 0;i < count;++i)
	{
		allWeight += retWeight[i];
		strWeight += to_string(i) + "_" + to_string(retWeight[i]) + "  ";
		str += to_string(i) + "_" + to_string(nAllMuilt[i]) + "  ";
	}
	LOG(WARNING) << "本局开奖倍数:" << str;
	LOG(WARNING) << "本局开奖权重:" << strWeight << " 总权重:" << allWeight;
}

void CTableFrameSink::getTwoCard(uint8_t cbData[2], int nPair, int starIndex, vector<uint8_t> vecData, int cardSize, bool bBanker)
{
	//检验是否牌对
	bool IsOK = false;
	if (bBanker)
	{
		for (int i = starIndex;i < cardSize - 1;i++)
		{
			if (!m_GameLogic.IsCardValid(vecData[i])) continue;
			cbData[0] = vecData[i];
			for (int j = i + 1;j < cardSize;j++)
			{
				if (vecData[j] == 0) continue;
				cbData[1] = vecData[j];
				if (m_GameLogic.IsCardPair(cbData) == nPair)
				{
					IsOK = true;
					break;
				}
			}
			if (IsOK)
				break;
		}
		eraseVecCard(cbData, 2);
	}
	else
	{
		if (starIndex < cardSize - 1)
		{
			if (m_GameLogic.IsCardValid(vecData[starIndex]))
			{
				cbData[0] = vecData[starIndex];
				for (int j = starIndex + 1;j < cardSize;j++)
				{
					if (!m_GameLogic.IsCardValid(vecData[j]))
						continue;
					cbData[1] = vecData[j];
					if (m_GameLogic.IsCardPair(cbData) == nPair)
					{
						break;
					}
				}
			}
		}
	}
}

//获取和对
void CTableFrameSink::getHePairCard(uint8_t cbBankerData[2], uint8_t cbData[2], int nPair, int starIndex, vector<uint8_t> vecData, int cardSize)
{
	bool IsOK = false;
	int bPair = 0;
	do
	{
		IsOK = false;
		int i = 0;
		for (i = starIndex;i < cardSize - 1;i++)
		{
			cbData[0] = vecData[i];
			for (int j = i + 1;j < cardSize;j++)
			{
				cbData[1] = vecData[j];
				bPair = m_GameLogic.IsCardPair(cbBankerData, cbData, 2, ID_HEPAIR);
				if (bPair == nPair)
				{
					IsOK = true;
					break;
				}
			}
			if (IsOK)
				break;
		}
		if (i >= cardSize - 1)
		{
			IsOK = true;
			break;
		}
	} while (!IsOK);

	if (IsOK)
	{
		eraseVecCard(cbData, 2);
	}
}

void CTableFrameSink::eraseVecCard(uint8_t cbData[], int cbCount)
{
	for (int i = 0; i < cbCount; ++i)
	{
		for (vector<uint8_t>::iterator it = m_vecData.begin(); it != m_vecData.end(); it++)
		{
			if (cbData[i] == (*it))
			{
				m_vecData.erase(it);
				break;
			}
		}
	}
}
//获取游戏状态对应的剩余时间
int32_t CTableFrameSink::getGameCountTime(int gameStatus)
{
	int countTime = 0;
	int32_t leaveTime = 0;
	int32_t allTime = 0;
	if (gameStatus == 1) // 洗牌时间
	{
		allTime = m_nWashCardTime;
	}
	else if (gameStatus == 2) //发牌时间
	{
		allTime = m_nSendCardTime;
	}
	else if (gameStatus == 3) //下注时间
	{
		allTime = m_nPlaceJettonTime;
	}
	else //结束时间
	{
		allTime = m_nGameEndTime;
		//补牌时间
		float bupaiTime = 0;
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			bupaiTime += m_fBupaiTime;
		}
		if (m_cbCardCount[INDEX_PLAYER] == 3)
		{
			bupaiTime += m_fBupaiTime;
		}
		allTime += ceil(bupaiTime);
	}
	chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
    leaveTime = allTime - durationTime.count();
	countTime = (leaveTime > 0 ? leaveTime : 0);

	return countTime;
}

void CTableFrameSink::updateGameMsg(bool isInit)
{
	if (!isInit)
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdUpdateGameMsg);
	/*int typeID[16] = { 1,1,2,2,1,1,1,2,2,2,2,1,1,1,1,1 };
	for (int i = 0;i < 16;i++)
	{
		m_openRecord.push_back(typeID[i]);
	}
	m_nGoodRouteType = m_GameLogic.getGoodRouteType(m_openRecord);*/

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
	countMsg["b"] = m_winOpenCount[1];//庄数量
	countMsg["p"] = m_winOpenCount[2];//闲数量
	countMsg["tt"] = m_winOpenCount[3];//和对数量
	countMsg["bt"] = m_winOpenCount[4];//庄对数量
	countMsg["pt"] = m_winOpenCount[5];//闲对数量
	countMsg["player"] = m_pITableFrame->GetPlayerCount(true);//桌子人数

	details["count"] = countMsg;//各开奖次数
	details["routetypename"] = m_strGoodRouteName; //好路名称

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
	if (!TEST)
	{
		if (!isInit)
			m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, boost::bind(&CTableFrameSink::updateGameMsg, this, false));
	}
}

//获取游戏结束对应的发牌状态 0左翻牌 1右翻牌 2左补牌 3右补牌 4显示结果
int32_t CTableFrameSink::getGameEndCurState()
{
	int nState = 0;
	chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_gameStateStartTime);
	int passTime = durationTime.count();
	if (passTime < m_fOpenCardTime)
	{
		nState = 0;
	}
	else if (passTime < m_fOpenCardTime * 2)
	{
		nState = 1;
	}
	else
	{
		if (m_cbCardCount[INDEX_PLAYER] == 3) //闲家需补牌
		{
			if (passTime < m_fOpenCardTime * 2 + m_fBupaiTime)
			{
				nState = 2;
			}
			else
			{
				if (m_cbCardCount[INDEX_BANKER] == 3 && passTime < m_fOpenCardTime * 2 + m_fBupaiTime*2)
				{
					nState = 3;
				} 
				else
				{
					nState = 4;
				}
			}
		} 
		else
		{
			if (m_cbCardCount[INDEX_BANKER] == 3 && passTime < m_fOpenCardTime * 2 + m_fBupaiTime)
			{
				nState = 3;
			}
			else
			{
				nState = 4;
			}
		}
	}

	return nState;
}

