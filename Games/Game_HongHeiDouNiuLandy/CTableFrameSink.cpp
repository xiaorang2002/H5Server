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

#include "proto/Hhdn.Message.pb.h"

#include "HhdnAlgorithm.h"
#include "GameLogic.h"

using namespace Hhdn;

#include "CTableFrameSink.h"

#define MIN_LEFT_CARDS_RELOAD_NUM  24
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0
//宏定义
#ifndef _UNICODE
#define myprintf    _snprintf
#else
#define myprintf    swprintf
#endif

// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/Hhdn/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("Hhdn");
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

        if(access("./log/Hhdn", 0) == -1)
           mkdir("./log/Hhdn",0777);
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
private:
     bool bisInit;
};


//////////////////////////////////////////////////////////////////////////

//静态变量
//#define TIME_GAME_COMPAREEND				6000			//结束定时器
//#define TIME_GAME_OPENEND					500				//结束定时器

int64_t						m_lStockScore    = 0;			//库存
int64_t						m_lInScoreVal    = 10000;		//吸分值
int64_t						m_lOutScoreVal   = 20000;		//吐分值
int64_t						m_lStorageDeduct = 1;			//回扣变量

int64_t                     openCount = 1;

//////////////////////////////////////////////////////////////////////////

//MAX_CARDS = 52
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

	for (int i=0;i<MAX_AREA;++i)
	{
		m_AreaJetton[i].clear();
	}

    memset(m_cbTableCardArray, 0, sizeof(m_cbTableCardArray));
	memset(m_cbOxCard, 0, sizeof(m_cbOxCard));

    m_IsEnableOpenlog = false;
    m_IsXuYaoRobotXiaZhu = false;

    m_nPlaceJettonTime = 0;
    m_nPlaceJettonLeveTime = 0;
    m_ngameTimeJettonBroadcast=0;
    m_nGameEndTime = 0;
    m_nGameEndLeveTime = 0;

    m_ShenSuanZiId = 0;

    m_UserInfo.clear();
    m_SortUserInfo.clear();

    m_openRecord.clear();
	m_openRecordType.clear();


    m_CardVecData.clear();
    m_pGameRoomInfo = nullptr;
    srandom(time(NULL));

    m_gameRoundCount=0;
    m_gameWinCount=0;
    m_winPercent=0.0f;
    m_curSystemWin=0;
    m_dControlkill=0.0;
    m_lLimitPoints=0;



    stockWeak = 0.0f;
	memset(m_nMuilt, 0.0f, sizeof(m_nMuilt));
    memset(m_bAreaIsWin, 0, sizeof(m_bAreaIsWin));
	memset(m_areaWinCount, 0, sizeof(m_areaWinCount));
	memset(m_notOpenCount, 0, sizeof(m_notOpenCount));
	memset(m_notOpenCount_last, 0, sizeof(m_notOpenCount_last));
	m_todayGameRoundCount = 0;
	m_bTestGame = 0;
	m_bWriteLvLog = false;
	m_nTestType = 0;
	m_nTestTimes = 0;
	m_bTestAgain = false;
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
    useIntelligentChips=1;
    ReadConfigInformation();
    return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdJudgeTime);
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
}

//读取配置
bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/hhdn_config.ini"))
    {
        LOG_INFO << "./conf/hhdn_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/hhdn_config.ini", pt);

    stockWeak=pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
	m_bTestGame = pt.get<int32_t>("GAME_CONFIG.TESTGAME", 0);
    m_nTestType = pt.get<int32_t>("GAME_CONFIG.NTESTTYPE", 0);
    m_nTestTimes = pt.get<int32_t>("GAME_CONFIG.TESTGAMETIMES", 0);
    //Time
    m_nPlaceJettonTime   = pt.get<int64_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 7);
    m_ngameTimeJettonBroadcast= pt.get<float>("GAME_CONFIG.TM_JETTON_BROADCAST", 1);

    m_HeTimes           = pt.get<double>("GAME_CONFIG.AREA_HE", 5.0);
    m_HongTimes          = pt.get<double>("GAME_CONFIG.AREA_LONG", 1.0);
    m_HeiTimes           = pt.get<double>("GAME_CONFIG.AREA_HU", 1.0);
	m_NiuOneTimes		 = pt.get<double>("GAME_CONFIG.AREA_NIU_1", 5.5);
    m_NormalTimes        = pt.get<double>("GAME_CONFIG.AREA_NORMALNIU", 6.0);
    m_SpecialTimes       = pt.get<double>("GAME_CONFIG.AREA_SPECIALNIU", 500.0);
	for (int i = 0; i < MAX_AREA; ++i)
	{
		if (i == 0) //和
			m_nMuilt[i] = m_HeTimes;
		else if (i == 1) //黑
			m_nMuilt[i] = m_HeiTimes;
		else if (i == 2) //红
			m_nMuilt[i] = m_HongTimes;
		else if (i == 3) //牛一
			m_nMuilt[i] = m_NiuOneTimes;
		else if (i == MAX_AREA-1) //特殊牛
			m_nMuilt[i] = m_SpecialTimes;
		else //常规牛[牛二 ~ 牛牛]
			m_nMuilt[i] = m_NormalTimes;
	}

    m_AreaMaxJetton[0]=pt.get<int64_t>("GAME_AREA_MAX_JETTON.HE",1000000);
    m_AreaMaxJetton[1]=pt.get<int64_t>("GAME_AREA_MAX_JETTON.RED",4000000);
    m_AreaMaxJetton[2]=pt.get<int64_t>("GAME_AREA_MAX_JETTON.BLACK",4000000);
    for (int i = 3; i < MAX_AREA; ++i)
    {
        if (i<MAX_AREA-1)
        {
            m_AreaMaxJetton[i]=pt.get<int64_t>("GAME_AREA_MAX_JETTON.NORMALNIU",1000000);
        }
        else
        {
            m_AreaMaxJetton[i]=pt.get<int64_t>("GAME_AREA_MAX_JETTON.SPECIALNIU",10000);
        }
    }

    m_dControlkill=pt.get<double>("GAME_CONFIG.CotrolKill",0.45);
    m_lLimitPoints=pt.get<int64_t>("GAME_CONFIG.LiMiPoint", 500000);

    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog",0);
    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);
    m_bControl           = pt.get<int64_t>("GAME_CONFIG.CONTROL", 0);

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
        for(int i=0;i<MAX_AREA;i++)
        {
            m_AreaJetton[i][chips] = 0;
        }
    }
}

bool CTableFrameSink::ReadCardConfig()
{
	if (m_bTestGame != 2)
		return false;
    m_listTestCardArray.clear();
    if (!boost::filesystem::exists("./conf/cards_hhdn.json"))
        return false;
    boost::property_tree::ptree pt;
    try{
        boost::property_tree::read_json("./conf/cards_hhdn.json",pt);
    }catch(...){
        LOG(INFO)<<"cards_hhdn.json firmat error !!!!!!!!!!!!!!!";
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
	/*string str="./conf/cards";
	str+= time((time_t*)NULL);
	str+="_hhdn.json";
	boost::filesystem::rename("./conf/cards_hhdn.json",str);*/
    return true;
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
            for (int i = 0; i < MAX_AREA; ++i)
            {
                totalxz += userInfo->m_lUserJettonScore[i];
            }
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

    if(GAME_STATUS_INIT == m_pITableFrame->GetGameStatus())
    {
        m_ShenSuanZiId = pIServerUserItem->GetUserId();
//        openLog("11111shensunzi=%d",m_ShenSuanZiId);
        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
        m_nGameEndLeveTime = m_nGameEndTime;
        m_startTime = chrono::system_clock::now();
        m_strRoundId = m_pITableFrame->GetNewRoundId();
		ReadConfigInformation();
        m_replay.gameinfoid = m_strRoundId;
        m_dwReadyTime = (uint32_t)time(NULL);
		openLog("===开始下注 gameinfoid[%s]", m_strRoundId.c_str());
        
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
        m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
		memset(m_notOpenCount, 0, sizeof(m_notOpenCount));
		memset(m_notOpenCount_last, 0, sizeof(m_notOpenCount_last));
//		for (int i = 0;i < MAX_AREA;++i)
//		{
//			for(int64_t jetton : m_pGameRoomInfo->jettons)
//			{
//				m_AreaJetton[i][jetton] = 0;
//			}
//        }

		if (m_bTestGame == 1)
		{
			tagStorageInfo storageInfo;
			m_pITableFrame->GetStorageScore(storageInfo);
			m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
			m_TimerIdTest = m_TimerLoopThread->getLoop()->runEvery(0.1, boost::bind(&CTableFrameSink::TestGameMessage, this, chairId, m_nTestType));
			openLog("本次测试%d开始;", m_nTestTimes);
			openLog("开始库存:High Limit:%d,Lower Limit:%d,,Aver Line:%d", storageInfo.lUplimit, storageInfo.lLowlimit, storageInfo.lEndStorage);
			m_bTestAgain = true;
		}
		else
		{
			m_TimerIdJudgeTime = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::ClearTodayRoundCount, this));
			if (m_bTestGame == 1)
				m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(0.1, boost::bind(&CTableFrameSink::OnTimerJetton, this));
			else
				m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
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
		//清除这个座位的押分记录
		userInfo->clearJetton();

        m_UserInfo[userId] = userInfo;
		
    }
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

   // LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << (int)cbGameStatus;

    switch(cbGameStatus)
    {
        case GAME_STATUS_FREE:		//空闲状态
        {
            //构造数据
            // CMD_S_GameStart pGameStartScene;
            // std::string data = pGameStartScene.SerializeAsString();
            // pGameStartScene.set_cbtimeleave(10);
            // m_pITableFrame->SendTableData(pIServerUserItem->GetChairID(), Hhdn::SUB_S_GAME_START, data.c_str(), data.size());
            return true;
        }
        break;
    	// case GAME_STATUS_PLAY:	//游戏状态
        case GAME_STATUS_START:	//游戏状态
        // case GAME_STATUS_JETTON:
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

    for(int i = 0; i < size; ++i)
    {
		HhdnGameRecord* precord = record.add_record();
        precord->set_winarea(m_openRecord[i]);
        precord->set_cardgrouptype(m_openRecordType[i]);
    }

	for (int i = 0;i < MAX_AREA; ++i)
	{
        AreaRecord* arearecord = record.add_arearecord();
		arearecord->set_areaindex(i);
		arearecord->set_wincount(m_areaWinCount[i]);
	}
    record.set_opencount(m_gameRoundCount);

    std::string data = record.SerializeAsString();
	if (m_bTestGame != 1)
		m_pITableFrame->SendTableData(chairId, Hhdn::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
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
    for(int i = 0; i < MAX_AREA; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
        alljitton->set_ljettonarea(i);
    }

    for(int i = 0; i < MAX_AREA; ++i)
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
    for(int i = 0; i < MAX_AREA; ++i)
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
    for(int i = 0; i < 2; ++i) //1：黑； 2：红
    {
		CardGroup* card = pInfo->add_cards();
		for (int j=0;j<MAX_COUNT;++j)
		{
			card->add_cards(m_cbTableCardArray[i*MAX_COUNT + j]);
		}
        card->set_group_id(i+1);
        card->set_cardtype(0);
		for (int j = 0;j < MAX_COUNT;++j)
		{
			card->add_cboxcards(m_cbOxCard[i][j]);
		}
    }

   uint32_t i = 0;
   uint32_t size = 6;
   int64_t nowScore = 0;
   int64_t xzScore = 0;
   int64_t lPreUserJettonScore = 0;
   shared_ptr<IServerUserItem> pUserItem;
   shared_ptr<UserInfo> userInfoItem;
   for(auto &userInfoItem : m_SortUserInfo)
   {
        // userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
           nowScore = 0;
        else
        {
            xzScore = 0;
            if(pUserItem->IsAndroidUser())
            {
                for (int j = 0; j < MAX_AREA; ++j)
                {
                    xzScore += userInfoItem->m_lRobotUserJettonScore[j];
                }
            }
            else
            {
                for (int j = 0; j < MAX_AREA; ++j)
                {
                    xzScore += userInfoItem->m_lUserJettonScore[j];
                }
            }
            nowScore = pUserItem->GetUserScore() - xzScore;
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
    pInfo->set_szgameroomname("Hhdn");
    if(m_openRecord.size() > 0)
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1]);
    else
        pInfo->set_wintag(0);

    pInfo->set_wincardgrouptype(m_winType);// add_wincardgrouptype(0);

	/*for(auto &jetton : m_Special)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(HE);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for(auto &jetton : m_Hong)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(BLACK);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}
	for(auto &jetton : m_Hei)
	{
		PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
		jetinfo->set_ljettonarea(RED);
		jetinfo->set_ljettontype(jetton.first);
		jetinfo->set_ljettoncount(jetton.second);
	}*/
	for (int j=0;j<MAX_AREA;++j)
	{
		for (auto &jetton : m_AreaJetton[j])
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(j);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
	}

    pInfo->set_lonlinepopulation(m_UserInfo.size());

	for (int i = 0;i < MAX_AREA;i++)
	{
		pInfo->add_notopenjushu(m_notOpenCount_last[i]);
	}

    pGamePlayScene.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_startTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pGamePlayScene.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    xzScore = 0;

    bool isRobot = pIServerUserItem->IsAndroidUser();
    if(isRobot)
    {
        for (int j = 0; j < MAX_AREA; ++j)
        {
            xzScore += userInfo->m_lRobotUserJettonScore[j];
        }
    }   
    else
    {
        for (int j = 0; j < MAX_AREA; ++j)
        {
            xzScore += userInfo->m_lUserJettonScore[j];
        }
    }

    nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pGamePlayScene.set_lusermaxscore(maxscore);
    pGamePlayScene.set_luserscore(nowScore);
    lPreUserJettonScore = 0;
    for (int j = 0; j < MAX_AREA; ++j)
    {
        lPreUserJettonScore += userInfo->m_lPreUserJettonScore[j];
    }
    if(lPreUserJettonScore>0)
        pGamePlayScene.set_brejetton(true);
    else
        pGamePlayScene.set_brejetton(false);


    pGamePlayScene.set_verificationcode(0);
    pGamePlayScene.set_roundid(m_strRoundId);
    std::string data = pGamePlayScene.SerializeAsString();
	if (m_bTestGame != 1)
		m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Hhdn::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
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
    for(int i = 0; i < MAX_AREA; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 0; i < MAX_AREA; ++i)
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
    for(int i = 0; i < MAX_AREA; ++i)
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
		for (int j = 0;j < MAX_COUNT;++j)
		{
			card->add_cards(m_cbTableCardArray[i*MAX_COUNT + j]);
		}
        card->set_group_id(i+1);
        card->set_cardtype(m_TableCardType[i]);
		for (int j = 0;j < MAX_COUNT;++j)
		{
			card->add_cboxcards(m_cbOxCard[i][j]);
		}
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
            {
                for (int j = 0; j < MAX_AREA; ++j)
                {
                    xzScore += userInfoItem->m_lRobotUserJettonScore[j];
                }
            }
            else
            {
                for (int j = 0; j < MAX_AREA; ++j)
                {
                    xzScore += userInfoItem->m_lUserJettonScore[j];
                }
            }
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
        palyer->set_index(i+1);
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
    pInfo->set_szgameroomname("Hhdn");

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

    pInfo->set_wincardgrouptype(m_winType);
    //pInfo->set_blucky(m_winType == (uint8_t)HE);

	for (int j = 0;j < MAX_AREA;++j)
	{
		for (auto &jetton : m_AreaJetton[j])
		{
			PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
			jetinfo->set_ljettonarea(j);
			jetinfo->set_ljettontype(jetton.first);
			jetinfo->set_ljettoncount(jetton.second);
		}
	}

    pInfo->set_lonlinepopulation(m_UserInfo.size());

	for (int i = 0;i < MAX_AREA;i++)
	{
		pInfo->add_notopenjushu(m_notOpenCount_last[i]);
	}

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

    string data = gamend.SerializeAsString();
	if (m_bTestGame != 1)
		m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Hhdn::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
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
       if( ++listNum>=7)
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
            for (int i = 0; i < MAX_AREA; ++i)
            {
                totalxz += userInfo->m_lUserJettonScore[i];
            }            
        }
		int nStatus = pIServerUserItem->GetUserStatus();
        if(totalxz == 0)
        {
			pIServerUserItem->setOffline();
			//清除这个座位的押分记录
			userInfo->clearJetton();
            m_UserInfo.erase(userId);
            m_pITableFrame->ClearTableUser(chairId);
            bClear=true;
        }else
        {
            pIServerUserItem->SetTrustship(true);
        }
    }

    if(userId == m_ShenSuanZiId)
    {
        m_ShenSuanZiId = 0;
    }

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
    //tagRedGateInfo info = { 0 };
    //if(m_IsXuYaoRobotXiaZhu == 1)
    //{
    //    for(int i = 0; i < RXBGATE_MAX; ++i)
    //    {
    //        tagRedGateIn& in = info.inf[i];
    //        in.lscorePlaced = m_lAllJettonScore[i]/100;
    //        in.sign = AICSRED_SIGN;
    //    }
    //}else if(m_IsXuYaoRobotXiaZhu == 0)
    //{
    //    for (int i = 0; i < RXBGATE_MAX; ++i)
    //    {
    //        tagRedGateIn& in = info.inf[i];
    //        in.lscorePlaced = m_lTrueUserJettonScore[(i+1)%3]/100;  //1 2 0
    //        in.sign = AICSRED_SIGN;
    //        LOG_ERROR << "lscorePlaced: " << in.lscorePlaced;
    //        openLog("RXBGATE_MAX:[%d],lscorePlaced:[%0.02lf]",(i+1)%3,in.lscorePlaced);
    //    }
    //}
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);
	uint8_t heiType = 0;
	uint8_t hongType = 0;
	uint8_t CompareIndex = 0;
	m_lStockScore = storageInfo.lEndStorage;
	for (int i = 0; i < MAX_AREA; ++i)
	{
		m_hhdnAlgorithm.SetBetting(i, m_lTrueUserJettonScore[i]);//设置押注
	}
	m_hhdnAlgorithm.SetAlgorithemNum(storageInfo.lUplimit, storageInfo.lLowlimit, storageInfo.lEndStorage, m_dControlkill, m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
    if (m_bControl)
    {
		BlackPlayerGetOpenCards(storageInfo, heiType, hongType);
    }
    else
    {
        uint8_t tmpcbTableCardArray[2][MAX_COUNT];
        m_hhdnAlgorithm.m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
		heiType = m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK - 1], MAX_COUNT);
		hongType = m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED - 1], MAX_COUNT);
		m_TableCardType[BLACK - 1] = heiType;
		m_TableCardType[RED - 1] = hongType;		
		m_hhdnAlgorithm.ConcludeWinScore(tmpcbTableCardArray, m_winFlag, m_winType);
    }
	
    if (m_listTestCardArray.size() ==0){
        ReadCardConfig();
    }
    if (m_listTestCardArray.size()>=2*MAX_COUNT)
    {
		uint8_t tmpcbTableCardArray[2][MAX_COUNT] = { 0 };
        for(int i = 0 ; i < 2*MAX_COUNT ; i ++ ){
            m_cbTableCardArray[i]=*(m_listTestCardArray.begin());
            m_listTestCardArray.pop_front();
        }
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        heiType =   m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK-1], MAX_COUNT);
        hongType=   m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED-1], MAX_COUNT);
        m_TableCardType[BLACK-1] = heiType;
        m_TableCardType[RED-1] = hongType;
		m_hhdnAlgorithm.ConcludeWinScore(tmpcbTableCardArray, m_winFlag, m_winType);
    }
	memcpy(m_cbOxCard, m_cbTableCardArray, sizeof(m_cbOxCard));
	for (int i = 0; i < 2; i++)
	{
		if (m_TableCardType[i] == CT_SIHUA_NIU || m_TableCardType[i] == CT_BOMB_NIU)
		{
			m_hhdnAlgorithm.m_GameLogic.SortCardListBH(m_cbOxCard[i], m_TableCardType[i]);
		} 
		else
		{
			m_hhdnAlgorithm.m_GameLogic.GetOxCard(m_cbOxCard[i]);
		}
	}
	
	bool isOpenCardEnd = false;
    LOG(INFO) << "......winFlag:" << (int)m_winFlag << " heiType:" << (int)m_TableCardType[BLACK - 1] << " hongType:" << (int)m_TableCardType[RED - 1];
	openLog("===winFlag:[%d],winType:[%d],heiType:[%d],hongType:[%d]", m_winFlag, m_winType, m_TableCardType[BLACK - 1], m_TableCardType[RED - 1]);
    m_curSystemWin=0;
    m_curSystemWin=m_hhdnAlgorithm.GetCurProfit();
	int64_t concludeSystemWin = m_curSystemWin;
    m_gameRoundCount++;
    m_todayGameRoundCount++;
	m_areaWinCount[m_winFlag]++;
	uint8_t winIndex = 0;
	if (m_TableCardType[BLACK - 1] > 0)
	{
		winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
		m_areaWinCount[winIndex]++;
	} 
	if (m_TableCardType[RED - 1] > 0)
	{		
		winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[RED - 1]);
		m_areaWinCount[winIndex]++;
	}
	
    if(m_curSystemWin>0)
    {
        m_gameWinCount++;
        m_curSystemWin=(m_curSystemWin*(1000-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000);
    }
    m_pITableFrame->UpdateStorageScore(m_curSystemWin);
    m_pITableFrame->GetStorageScore(storageInfo);
    m_lStockScore=storageInfo.lEndStorage;
    string str = "";
	for (int i=0;i<MAX_AREA;++i)
	{
		str += GetAreaName(i) + "_" + to_string(m_areaWinCount[i]) + " ";
	}
    openLog("===总共场次[%d],胜场次数[%d],各区域开奖次数[%s]",m_gameRoundCount, m_gameWinCount, str.c_str());
    openLog("===胜率[%02f],库存变化:结算赢输[%d],实际赢输[%d],", m_winPercent, concludeSystemWin, m_curSystemWin);
    openLog("===当前库存[%d],库存上限[%d],库存下限[%d]", m_lStockScore, storageInfo.lUplimit,storageInfo.lLowlimit);
    openLog("===黑牛=0x%02x,0x%02x,0x%02x,0x%02x,0x%02x-----红牛=0x%02x,0x%02x,0x%02x,0x%02x,0x%02x",
        m_cbTableCardArray[0],m_cbTableCardArray[1],m_cbTableCardArray[2],m_cbTableCardArray[3],m_cbTableCardArray[4]
        ,m_cbTableCardArray[5],m_cbTableCardArray[6],m_cbTableCardArray[7],m_cbTableCardArray[8],m_cbTableCardArray[9]);

	LOG(INFO) << "===库存上限:" << storageInfo.lUplimit << ",库存下限:" << storageInfo.lLowlimit << " ========当前库存:" << m_lStockScore ;
	if (m_gameRoundCount % 100 == 0)
	{
		m_bWriteLvLog = true;
		double heLv = 1.0*m_areaWinCount[HE] / (m_gameRoundCount*1.0);
		double heiLv = 1.0*m_areaWinCount[BLACK] / (m_gameRoundCount*1.0);
		double hongLv = 1.0*m_areaWinCount[RED] / (m_gameRoundCount*1.0);		
		str = "";
		for (int i = 3;i < MAX_AREA;++i)
		{
			if (i == MAX_AREA-1)
			{
				str += GetAreaName(i) + "[" + to_string(m_areaWinCount[i]) + "];";
			} 
			else
			{
				str += GetAreaName(i) + "[" + to_string(m_areaWinCount[i]) + "],";
			}			
		}
		openLog(" 总场次[%d],和胜率%d[%f],黑胜率%d[%f],红胜率%d[%f],%s", m_gameRoundCount, m_areaWinCount[HE], heLv, m_areaWinCount[BLACK], heiLv, m_areaWinCount[RED], hongLv, str.c_str());
		openLog("===当前库存[%d],库存上限[%d],库存下限[%d]", m_lStockScore, storageInfo.lUplimit, storageInfo.lLowlimit);
		m_bWriteLvLog = false;
	}
	 if (m_bTestGame==1 && m_nTestTimes>0)
	 {
	     m_nTestTimes--;
	     LOG(INFO)<< "==============剩余测试次数============== " << m_nTestTimes;
		 openLog("剩余测试次数: %d;",m_nTestTimes);
	     if (m_nTestTimes<=0)
	     {
	         m_nTestTimes = 0;
			 m_bTestGame = 0;
	         LOG(INFO)<< "==========本次测试结束===========" ;
			 openLog("==========本次测试结束=========== ");
			 m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
	         TCHAR szPath[MAX_PATH] = TEXT("");
	         TCHAR szConfigFileName[MAX_PATH] = TEXT("");
	         TCHAR OutBuf[255] = TEXT("");
	         CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
             myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/hhdn_config.ini"), szPath);
	         CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TESTGAME","0",szConfigFileName);
	         CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TESTGAMETIMES","0",szConfigFileName);
			 ReadConfigInformation();
	     }        
	 }
}
//系统控制是否有黑名单时开牌
void CTableFrameSink::BlackPlayerGetOpenCards(tagStorageInfo storageInfo, uint8_t &heiType, uint8_t &hongType)
{
    int isHaveBlackPlayer = 0;
    shared_ptr<CServerUserItem> user;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		user = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!user || user->IsAndroidUser() || !user->GetBlacklistInfo().status)
		{
			continue;
		}
		int64_t allJetton = 0;
		for (int j = 0; j < 3; ++j) //只干涉和红黑三个区域
		{
			allJetton += userInfoItem->m_lUserJettonScore[j];
		}
		if (allJetton == 0) continue;
		if (user->GetBlacklistInfo().listRoom.size() > 0)
		{
			auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
			if (it != user->GetBlacklistInfo().listRoom.end())
			{
				if (it->second > 0)
				{
					isHaveBlackPlayer = it->second;//
				}
				openLog("===黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(), m_pITableFrame->GetGameRoomInfo()->gameId, isHaveBlackPlayer);
				openLog("===控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status, user->GetBlacklistInfo().current, user->GetBlacklistInfo().total);
			}
		}
    }



	uint8_t CompareIndex = 0;
    uint8_t tmpcbTableCardArray[2][MAX_COUNT] = { 0 };
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        m_hhdnAlgorithm.SetMustKill(-1);
        m_hhdnAlgorithm.GetOpenCard(tmpcbTableCardArray, m_winFlag, m_winType);
    }
    else
    {
        if (isHaveBlackPlayer > 0)
        {
            // 只干涉和黑红区域
            float probilityAll[3] = { 0.0f };
            //user probilty count
            for (auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                user = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if (!user || user->IsAndroidUser() || !user->GetBlacklistInfo().status)
                {
                    continue;
                }
                int64_t allJetton = 0;
                for (int j = 0; j < 3; ++j)
                {
                    allJetton += userInfoItem->m_lUserJettonScore[j];
                }
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
                    float gailv = (float)shuzhi*1.0 / 1000;
                    int64_t lTrueUserAllJettonScore = 0;
                    for (int j = 0; j < 3; j++)
                    {
                        lTrueUserAllJettonScore += m_lTrueUserJettonScore[j];
                    }
                    for (int j = 0;j < 3;j++)
                    {
                        if (m_lTrueUserJettonScore[j] > 0)
                        {
                            probilityAll[j] += (gailv*(float)userInfoItem->m_lUserJettonScore[j]) / (float)lTrueUserAllJettonScore;
                        }
                    }
                }
            }
            m_hhdnAlgorithm.BlackGetOpenCard(tmpcbTableCardArray, m_winFlag, m_winType, probilityAll);
        }
        else
        {
            m_hhdnAlgorithm.GetOpenCard(tmpcbTableCardArray, m_winFlag, m_winType);
        }

    }

    memcpy(m_cbTableCardArray, tmpcbTableCardArray, sizeof(m_cbTableCardArray));
	m_TableCardType[BLACK - 1] = m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[BLACK - 1], MAX_COUNT);
	m_TableCardType[RED - 1] = m_hhdnAlgorithm.m_GameLogic.GetCardType(tmpcbTableCardArray[RED - 1], MAX_COUNT);
	heiType = m_TableCardType[BLACK - 1];
	hongType = m_TableCardType[RED - 1];
}
//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    //LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();     //open cards
        m_replay.addStep((uint32_t)time(NULL)-m_dwReadyTime,GlobalFunc::converToHex(m_cbOxCard[0], MAX_COUNT) + GlobalFunc::converToHex(m_cbOxCard[1], MAX_COUNT) + ":" + to_string(m_TableCardType[0]) + ":" + to_string(m_TableCardType[1]),-1,opShowCard,-1,-1);
        //结算
		int64_t lTrueUserWinScore[MAX_AREA] = { 0 };
		int64_t lTrueUserAllWinScore = 0;
		auto cul_user_score = [&](shared_ptr<UserInfo> userItem, int winarea, int lostarea, int64_t times, bool bAndroid)
		{
			int64_t lUserJettonScore[MAX_AREA] = { 0 };
			if (bAndroid)
			{
				memcpy(lUserJettonScore, userItem->m_lRobotUserJettonScore, sizeof(lUserJettonScore));
			} 
			else
			{
				memcpy(lUserJettonScore, userItem->m_lUserJettonScore, sizeof(lUserJettonScore));
			}
			// 和黑红区域
			if (winarea != -1 && lUserJettonScore[winarea] > 0)
			{
				//税收
				userItem->m_Revenue = m_pITableFrame->CalculateRevenue(lUserJettonScore[winarea] * times);
				userItem->m_EndWinScore += (lUserJettonScore[winarea] * times - userItem->m_Revenue);
				if (!bAndroid)
				{
					m_replay.addResult(userItem->chairId, winarea, lUserJettonScore[winarea], (lUserJettonScore[winarea] * times - userItem->m_Revenue), "", false);
					userItem->dAreaWin[winarea] += (lUserJettonScore[winarea] * times - userItem->m_Revenue);
					lTrueUserWinScore[winarea] += (lUserJettonScore[winarea] * times);
					lTrueUserAllWinScore += (lUserJettonScore[winarea] * times);
				}
				LOG_DEBUG << " robot Id:" << userItem->userId << " SystemTax:" << userItem->m_Revenue;
			}
			if (lostarea != -1 && lUserJettonScore[lostarea] > 0)
			{
				userItem->m_EndWinScore -= lUserJettonScore[lostarea];
				if (!bAndroid)
				{
					m_replay.addResult(userItem->chairId, lostarea, lUserJettonScore[lostarea], -lUserJettonScore[lostarea], "", false);
					userItem->dAreaWin[lostarea] -= lUserJettonScore[lostarea];
					lTrueUserWinScore[lostarea] -= lUserJettonScore[lostarea];
					lTrueUserAllWinScore -= lUserJettonScore[lostarea];
				}
			}
			// 其他区域
			uint8_t winIndex = 0;
			int64_t revenue = 0;
			bool bSpecialNiu = false;
			bool bOtherHaveBetScore = false;
			if (winarea == HE) //开和，红牛和黑牛的押注退还给玩家
			{
				if (m_TableCardType[BLACK - 1] > 0) //有牛 且下注了
				{
					//税收
					winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
					if (lUserJettonScore[winIndex] > 0)
					{
						revenue = m_pITableFrame->CalculateRevenue(lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
						userItem->m_Revenue += revenue;
						userItem->m_EndWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
						if (!bAndroid)
						{
							m_replay.addResult(userItem->chairId, winIndex, lUserJettonScore[winIndex], (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue), "", false);
							userItem->dAreaWin[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
							lTrueUserWinScore[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
							lTrueUserAllWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
							bOtherHaveBetScore = true;
						}
					}
				}
				//其他不中的区域
				for (uint8_t i = NiuOneGate;i < MaxGate;++i)
				{
					winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
					if (i != winIndex && lUserJettonScore[i] > 0)
					{
						userItem->m_EndWinScore -= lUserJettonScore[i];
						if (!bAndroid)
						{
							m_replay.addResult(userItem->chairId, i, lUserJettonScore[i], -lUserJettonScore[i], "", false);
							userItem->dAreaWin[i] -= lUserJettonScore[i];
							lTrueUserWinScore[i] -= lUserJettonScore[i];
							lTrueUserAllWinScore -= lUserJettonScore[i];
							bOtherHaveBetScore = true;
						}
					}
				}
				userItem->dAreaWin[BLACK] = 0;
				userItem->dAreaWin[RED] = 0;
				if (!bOtherHaveBetScore && lUserJettonScore[HeGate] == 0 && userItem->lTotalJettonScore != 0)
				{
					if (!bAndroid)
						m_replay.addResult(userItem->chairId, HeGate, 0, 0, "", false);
				}
			}
			else
			{
				userItem->dAreaWin[HE] = 0;
                if (lUserJettonScore[HE] > 0)
				{
					userItem->m_EndWinScore -= lUserJettonScore[HE];
					if (!bAndroid)
					{
						m_replay.addResult(userItem->chairId, HE, lUserJettonScore[HE], -lUserJettonScore[HE], "", false);
						userItem->dAreaWin[HE] -= lUserJettonScore[HE];
						lTrueUserWinScore[HE] -= lUserJettonScore[HE];
						lTrueUserAllWinScore -= lUserJettonScore[HE];
					}
				}
				if (m_TableCardType[BLACK - 1] > 0) //黑方，有牛且下注了
				{
					//税收
					winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
					if (lUserJettonScore[winIndex] > 0)
					{
						revenue = m_pITableFrame->CalculateRevenue(lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
						userItem->m_Revenue += revenue;
						userItem->m_EndWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
						if (!bAndroid)
						{
							m_replay.addResult(userItem->chairId, winIndex, lUserJettonScore[winIndex], (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue), "", false);
							userItem->dAreaWin[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
							lTrueUserWinScore[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
							lTrueUserAllWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
						}
					}
					if (winIndex == NiuSpecialGate)
					{
						bSpecialNiu = true;
					}
				}
				if (m_TableCardType[RED - 1] > 0) //红方，有牛且下注了
				{
					//税收
					winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[RED - 1]);
					if (lUserJettonScore[winIndex] > 0)
					{
						if (winIndex != NiuSpecialGate || !bSpecialNiu)
						{
							revenue = m_pITableFrame->CalculateRevenue(lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
							userItem->m_Revenue += revenue;
							userItem->m_EndWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
							if (!bAndroid)
							{
								m_replay.addResult(userItem->chairId, winIndex, lUserJettonScore[winIndex], (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue), "", false);
								userItem->dAreaWin[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex] - revenue);
								lTrueUserWinScore[winIndex] += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
								lTrueUserAllWinScore += (lUserJettonScore[winIndex] * m_nMuilt[winIndex]);
							}
						}						
					}
				}
				//其他不中的区域
				for (uint8_t i = NiuOneGate;i < MaxGate;++i)
				{
					uint8_t heiIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
					uint8_t hongIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[RED - 1]);
					if (i != heiIndex && i != hongIndex && lUserJettonScore[i] > 0)
					{
						userItem->m_EndWinScore -= lUserJettonScore[i];
						if (!bAndroid)
						{
							m_replay.addResult(userItem->chairId, i, lUserJettonScore[i], -lUserJettonScore[i], "", false);
							userItem->dAreaWin[i] -= lUserJettonScore[i];
							lTrueUserWinScore[i] -= lUserJettonScore[i];
							lTrueUserAllWinScore -= lUserJettonScore[i];
						}
					}
				}
			}
			LOG_DEBUG << " robot Id:" << userItem->userId << " SystemTax:" << userItem->m_Revenue << " TotalWin:" << userItem->m_EndWinScore;
			if (!bAndroid && userItem->lTotalJettonScore > 0)
			{
				string str = "";
				for (uint8_t i = 0; i < MAX_AREA; i++)
				{
					str += "[" + to_string(i) + "_" + to_string(lUserJettonScore[i]) + "_" + to_string(userItem->dAreaWin[i]) + "] ";
				}
				openLog("===玩家结算信息:userid=%d,%s,总输赢:%d;", userItem->userId, str.c_str(), userItem->m_EndWinScore);
			}
		};

        LOG_DEBUG<<" WinFlag BLACK 1 RED 2"<<m_winFlag;
		m_bAreaIsWin[m_winFlag] = 1;
		if (m_winFlag == HE)
			m_bAreaIsWin[HE] = 1;
		else
			m_bAreaIsWin[HE] = 0;

		uint8_t winIndex = 0;
		if (m_TableCardType[BLACK - 1] > 0) //黑方，有牛
		{
			winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
			m_bAreaIsWin[winIndex] = 1;
		}
		if (m_TableCardType[RED - 1] > 0) //黑方，有牛
		{
			winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[RED - 1]);
			m_bAreaIsWin[winIndex] = 1;
		}
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<UserInfo> userInfoItem;
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
			if (!pUserItem)
				continue;
			if (m_winFlag == HE)
			{
				cul_user_score(userInfoItem, HE, -1, m_HeTimes, pUserItem->IsAndroidUser());
			}
			else if (m_winFlag == BLACK)
			{
				cul_user_score(userInfoItem, BLACK, RED, m_HeiTimes, pUserItem->IsAndroidUser());
			}
            else if (m_winFlag == RED)
			{
				cul_user_score(userInfoItem, RED, BLACK, m_HongTimes, pUserItem->IsAndroidUser());
			}
		}	

		string allStr;
		if (lTrueUserAllWinScore!=0)
		{
			for (uint8_t i = 0; i < MAX_AREA; i++)
			{
				allStr += "[" + to_string(i) + "_" + to_string(m_lTrueUserJettonScore[i]) + "_" + to_string(lTrueUserWinScore[i]) + "] ";
			}
			openLog("===总结算免税信息:%s,玩家总输赢:%d;", allStr.c_str(), lTrueUserAllWinScore);
		}
		
        ChangeUserInfo();

         m_openRecord.push_back(m_winFlag);
         m_openRecordType.push_back(m_winType);

        //LOG(INFO) << "OnEventGameConclude wrong Conclude is NULL  wChairID = " << chairId;
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
        WRITE_LOCK(m_mutex);

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
		//openLog("===需踢出的玩家:Userid=%d;", clearUserIdVec[i]);
		LOG(INFO) << "===需踢出的玩家 Userid=" << clearUserIdVec[i];
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
	if (m_pGameRoomInfo->serverStatus != SERVER_RUNNING)
	{
		//m_TimerLoopThread->getLoop()->cancel(m_TimerIdJudgeTime);
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
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
			totalBetScore = 0;
			userid = pUserItem->GetUserId();
            scoreInfo.clear();
            if(pUserItem->IsAndroidUser())
            {
                for (int i = 0; i < MAX_AREA; ++i)
                {
                    totalBetScore += userInfoItem->m_lRobotUserJettonScore[i];
                    scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[i]);
                }
            }else
            {
                for (int i = 0; i < MAX_AREA; ++i)
                {
                    totalBetScore += userInfoItem->m_lUserJettonScore[i];
                    scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[i]);
                }
				allBetScore = totalBetScore;
            }
            if(totalBetScore > 0)
            {
                // if(userInfoItem->m_EndWinScore)
                {
                    scoreInfo.chairId = userInfoItem->chairId;
                    scoreInfo.betScore = totalBetScore;
                    scoreInfo.rWinScore = totalBetScore;
                    scoreInfo.addScore = userInfoItem->m_EndWinScore;
                    scoreInfo.revenue = userInfoItem->m_Revenue;
                    scoreInfo.cardValue = GlobalFunc::converToHex(m_cbTableCardArray, 2*MAX_COUNT);  // GetCardValueName(0) + " vs "+GetCardValueName(1);
                    scoreInfo.startTime = m_startTime;
                    scoreInfoVec.push_back(scoreInfo);
                }
            }
            // 统计连续未下注的次数
            if (totalBetScore<=0)
            {
                // userInfoItem->NotBetJettonCount++;
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
		totalJetton = 0;
        //添加下注信息
        if(pUserItem->IsAndroidUser())
        {
            for (int i = 0; i < MAX_AREA; ++i)
            {
                totalJetton += userInfoItem->m_lRobotUserJettonScore[i];
            }
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }else
        {
            for (int i = 0; i < MAX_AREA; ++i)
            {
                totalJetton += userInfoItem->m_lUserJettonScore[i];
            }
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
            m_ShenSuanZiId = userInfoItem->userId;
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
	
	/*for (auto &userInfoItem : m_SortUserInfo)
	{
		openLog("===UserId:%d,20局总下注:%d;", userInfoItem->userId, userInfoItem->lTwentyAllUserScore);
	}*/

}

//定时器事件
void CTableFrameSink::OnTimerJetton()
{
    m_endTime =chrono::system_clock::now();
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdJetton);
	//m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);

	openLog("===下注结束 Usercount=%d gameinfoid[%s];", m_UserInfo.size(), m_replay.gameinfoid.c_str());
    //GameTimerJettonBroadcast();
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
	if (m_bTestGame == 1)
		m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(0.1, boost::bind(&CTableFrameSink::OnTimerEnd, this));
	else
		m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(m_nGameEndTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));
    
    WriteUserScore();
    //设置状态
    m_pITableFrame->SetGameStatus(GAME_STATUS_END);
    //删除玩家信息
    clearTableUser();  //sOffline  m_cbPlayStatus==false
    ChangeVipUserInfo();
    LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();
	openLog("***结算结束 UserCount=%d*** \n", m_UserInfo.size());
	 if (m_bTestGame == 1 && m_bTestAgain )
	 {
	 	if ((m_nTestType == 1) && m_lStockScore <= 5500000 * 100) //放分
	 	{
	 		m_bTestAgain = false;
	 		openLog("=============库存下降为550W后再测试200局开始;");
	 		m_nTestTimes = 200;
	 	}
	 	else if ((m_nTestType == 2) && m_lStockScore >= 5000000 * 100) //收分
	 	{
	 		m_bTestAgain = false;
			openLog("=============库存上升到500W后再测试200局开始;");
	 		m_nTestTimes = 200;
	 	}
	 }
}

void CTableFrameSink::OnTimerEnd()
{
    //连续5局未下注踢出房间
    //CheckKickOutUser();
    m_startTime = chrono::system_clock::now();
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
    m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
	updateResultList(m_winFlag);
    ClearTableData();
    ChangeVipUserInfo();
    //结束游戏
    bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
    if(!IsContinue)
        return;

    // openLog("  ");
    LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();

    // for(auto iterinfo = m_UserInfo.begin(); iterinfo != m_UserInfo.end(); ++iterinfo)
    // {
    //     openLog("vipuserid=%d ",iterinfo->dwUserID);
    // }

     bool bRestart = false;
     if(m_openRecord.size()>100)//record last 20
     {
         //clear open record
         m_openRecord.pop_front();
         m_openRecordType.pop_front();
         //bRestart = true;  // no use now
     }

	 m_replay.clear();
     m_strRoundId = m_pITableFrame->GetNewRoundId();
     m_replay.gameinfoid = m_strRoundId;
     m_dwReadyTime = (uint32_t)time(NULL);

	 tagStorageInfo storageInfo;
	 m_pITableFrame->GetStorageScore(storageInfo);
	 openLog("===开始下注 Usercount=%d gameinfoid[%s];", m_UserInfo.size(), m_strRoundId.c_str());
	 openLog("===开始库存[%d],库存上限[%d],库存下限[%d];", storageInfo.lEndStorage, storageInfo.lUplimit, storageInfo.lLowlimit);
	 LOG(INFO) <<  "===库存上限:" << storageInfo.lUplimit << ",库存下限:" << storageInfo.lLowlimit << " 当前库存:" << storageInfo.lEndStorage;

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

    m_pITableFrame->SetGameStatus(GAME_STATUS_START);

    //设置时间
	if (m_bTestGame == 1)
		m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(0.1, boost::bind(&CTableFrameSink::OnTimerJetton, this));
	else
	{
		m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
		m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_ngameTimeJettonBroadcast, boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
	}   
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
	memset(m_cbOxCard, 0, sizeof(m_cbOxCard));
//	for (int i = 0;i < MAX_AREA;++i)
//	{
//		for (int64_t jetton : m_pGameRoomInfo->jettons)
//		{
//			m_AreaJetton[i][jetton] = 0;
//		}
//	}

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
	m_bTestGame = 0;
	m_nTestType = 0;
	m_nTestTimes = 0;
	if (m_bTestGame != 1)
	{
		ReadConfigInformation();
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
	}
	if (m_bTestGame != 2 && m_listTestCardArray.size() != 0)
		m_listTestCardArray.clear();
	memset(m_bAreaIsWin, 0, sizeof(m_bAreaIsWin));
	memset(m_TableCardType, 0, sizeof(m_TableCardType));
}

//游戏消息处理
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //::muduo::MutexLockGuard lock(m_mutexLock);
    // LOG(INFO) << "CTableFrameSink::OnGameMessage wChairID:" << wChairID << ", wSubCmdID:" << wSubCmdID << ", wDataSize:" << wDataSize;

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
            // LOG(INFO) << " >>> OnUserJETTON wChairID:" << wChairID;
            CMD_C_PlaceJet  pJETTON;
            pJETTON.ParseFromArray(pData,wDataSize);

            //消息处理
            return OnUserAddScore(wChairID, pJETTON.cbjettonarea(), pJETTON.ljettonscore());
        }
        case SUB_C_QUERY_PLAYERLIST:
        {
           // LOG(INFO) << " >>> OnUserJETTON wChairID:" << wChairID;
           // OnPlayerList(wChairID);
            return true;
        }
        case SUB_C_REJETTON :
        {
            //CMD_C_ReJetton
			OnUserRejetton(wChairID);
			return true;
        }
		case SUB_C_REJETTON_NEW:
		{
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
    for(int i=0;i<7;i++)
    {
        if(m_currentchips[wChairID][i]==lJettonScore)
        {
            bFind = true;
            break;
        }
    }
//	int32_t jettonsSize = m_pGameRoomInfo->jettons.size();
//    for(auto it : m_pGameRoomInfo->jettons)
//    {
//        if(it == lJettonScore)
//        {
//            bFind = true;
//            break;
//        }
//    }
    if (bRejetton) bFind = true;
    if(!bFind||cbJettonArea<0||cbJettonArea>=MAX_AREA || lJettonScore<0 || lJettonScore>m_currentchips[wChairID][6])
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
    // 清缓存筹码
    if (userInfo->lTotalJettonScore == 0 && !bRejetton) {
        userInfo->clearJetton();
    }
    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();
	int64_t lTotalJettonScore = 0;
    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        lTotalJettonScore = userInfo->lTotalJettonScore;
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
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
		userInfo->lTotalJettonScore += lJettonScore;
    }
	else //android suer
    {
        //已经下的总注
		lTotalJettonScore = userInfo->lTotalJettonScore;
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
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
		userInfo->lTotalJettonScore += lJettonScore;
    }

    if(m_ShenSuanZiId==userId)
    {
        // shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
        // if(SSZUserInfo->chairId == wChairID)
        m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }
	
    //openLog("===OnUserAddScore userid=%d area=%d score=%d",pIServerUserItem->GetUserId(),cbJettonArea,lJettonScore);
    //成功返回
    /*if(cbJettonArea == HE)
    {
        m_Special[lJettonScore]++;
    }else if(cbJettonArea == BLACK)
    {
        m_Hong[lJettonScore]++;
    }else if(cbJettonArea == RED)
    {
        m_Hei[lJettonScore]++;
    }*/
	for (uint8_t i=0;i<MAX_AREA;++i)
	{
		if (cbJettonArea == i)
		{
			m_AreaJetton[i][lJettonScore]++;
		}
	}
	tagBetInfo betinfo = { 0 };
	betinfo.cbJettonArea = cbJettonArea;
	betinfo.lJettonScore = lJettonScore;
	// 记录玩家押分筹码值
    if (!bRejetton)
    {
        userInfo->JettonValues.push_back(betinfo);
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
        {
            for(int i = 0; i < MAX_AREA; ++i)
            {
                xzScore += userInfo->m_lRobotUserJettonScore[i];
            }
        }
        else
        {
            for(int i = 0; i < MAX_AREA; ++i)
            {
                xzScore += userInfo->m_lUserJettonScore[i];
            }
        }
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 0; i < MAX_AREA; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 0; i < MAX_AREA; ++i)
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
		if (m_bTestGame != 1)
			m_pITableFrame->SendTableData(iChairId, Hhdn::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };
	
    if ( !isInVipList(wChairID) && userId!=m_ShenSuanZiId && m_UserInfo.size() >= 8)
    {
        userInfoItem=m_UserInfo[userId];
		if (!bRejetton)
			push(wChairID,userId,pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }
	else if (!bRejetton)
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
            // int64_t sendUserId = userInfoItem->userId;
            // uint32_t sendChairId = userInfoItem->chairId;
            // bool Robot = pUserItem->IsAndroidUser();
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}

//bool CTableFrameSink::OnUserRejetton(uint32_t chairId)
//{
//    shared_ptr<CServerUserItem>User=m_pITableFrame->GetTableUserItem(chairId);
//    shared_ptr<UserInfo> userinfo=m_UserInfo[User->GetUserId()];
//    if(!User)
//        return false;
//    if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
//    {
//          string err = "当前不是下注时间";
//          SendPlaceBetFail(chairId, 4, 0,err);
//          return true;
//    }
//    int alljetton=0;
//    int areajetton[MAX_AREA]={0};
//    for(int i=0;i<MAX_AREA;i++)
//    {
//        alljetton+=userinfo->m_lPreUserJettonScore[i];
//        areajetton[i]=userinfo->m_lPreUserJettonScore[i];
//    }
//    int64_t lTotalJettonScore = 0;//userinfo->m_lRobotUserJettonScore[HE] + userinfo->m_lRobotUserJettonScore[BLACK] + userinfo->m_lRobotUserJettonScore[RED];
//    for (int i = 0; i < MAX_AREA; ++i)
//    {
//        lTotalJettonScore += userinfo->m_lRobotUserJettonScore[i];
//    }
//    if(alljetton<0||User->GetUserScore()<lTotalJettonScore+alljetton)
//    {
//        string err="筹码不足";
//        SendPlaceBetFail(chairId,4,alljetton,err);
//        return false;
//    }
//    for(int i=0;i<MAX_AREA;i++)
//    {
//        if(m_AreaMaxJetton[i]<areajetton[i]+userinfo->m_lUserJettonScore[i])
//        {
//            string err="下注已达区域上限";
//            SendPlaceBetFail(chairId,4,alljetton,err);
//            return false;
//        }
//    }
//
//    for(int i=0;i<MAX_AREA;i++)
//    {
//        if(areajetton[i]==0)
//        {
//            continue;
//        }
//        OnUserAddScore(chairId,i,areajetton[i],true);
////        while(areajetton[i]>=1)
////        {
////            if(areajetton[i]>=m_pGameRoomInfo->jettons[5])
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[5]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[5];
////            }
////            else if(areajetton[i]>=m_pGameRoomInfo->jettons[4])
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[4]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[4];
////            }
////            else if(areajetton[i]>=m_pGameRoomInfo->jettons[3])
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[3]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[3];
////            }
////            else if(areajetton[i]>=m_pGameRoomInfo->jettons[2])
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[2]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[2];
////            }
////            else if(areajetton[i]>=m_pGameRoomInfo->jettons[1])
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[1]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[1];
////            }
////            else
////            {
////                OnUserAddScore(chairId,i,m_pGameRoomInfo->jettons[0]);
////                areajetton[i]-=m_pGameRoomInfo->jettons[0];
////            }
////        }
//    }
//    return true;
//}

bool CTableFrameSink::OnUserRejetton(uint32_t chairId)
{
	shared_ptr<CServerUserItem>User = m_pITableFrame->GetTableUserItem(chairId);
	if (!User)
		return false;
	int64_t userid = User->GetUserId();
	shared_ptr<UserInfo> userinfo = m_UserInfo[userid];
	if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
	{
		string err = "当前不是下注时间";
		SendPlaceBetFail(chairId, 4, 0, err);
		return true;
	}
    int64_t alljetton = 0;
    int64_t areajetton[MAX_AREA] = { 0 };
	for (int i = 0;i < MAX_AREA;i++)
	{
		alljetton += userinfo->m_lPreUserJettonScore[i];
		areajetton[i] = userinfo->m_lPreUserJettonScore[i];
	}
	/*int64_t lTotalJettonScore = userinfo->lTotalJettonScore;
	for (int i = 0; i < MAX_AREA; ++i)
	{
		lTotalJettonScore += userinfo->m_lRobotUserJettonScore[i];
	}*/
	//续押的所有的筹码值
	int64_t lAllRejettonScore = 0;
	int64_t lAreaRejettonScore[MAX_AREA] = { 0 };
	for (tagBetInfo betVal : userinfo->JettonValues)
	{
		lAllRejettonScore += betVal.lJettonScore;
		lAreaRejettonScore[betVal.cbJettonArea] += betVal.lJettonScore;
	}
	if (alljetton < 0 || User->GetUserScore() < alljetton || User->GetUserScore() < lAllRejettonScore)
	{
		string err = "筹码不足";
		SendPlaceBetFail(chairId, 4, alljetton, err);
		return false;
	}
	for (int i = 0;i < MAX_AREA;i++)
	{
		if (m_AreaMaxJetton[i] < (areajetton[i] + userinfo->m_lUserJettonScore[i]) || m_AreaMaxJetton[i] < lAreaRejettonScore[i])
		{
			string err = "下注已达区域上限";
			SendPlaceBetFail(chairId, 4, alljetton, err);
			return false;
		}
	}

	/*for (int i = 0;i < MAX_AREA;i++)
	{
		if (areajetton[i] == 0)
		{
			continue;
		}
		OnUserAddScore(chairId, i, areajetton[i], true);
	}*/
	//返回续押数组
	Hhdn::CMD_S_RepeatJetSuccess RepeatJet;
	RepeatJet.set_dwuserid(userid);
	RepeatJet.set_bisandroid(User->IsAndroidUser());
	RepeatJet.set_luserscore(User->GetUserScore() - alljetton);

	bool bIsJetOk = false;
	//RepeatJet
	for (tagBetInfo betVal : userinfo->JettonValues)
	{
		Hhdn::RepeatInfo *RepeatInfo = RepeatJet.add_trepeat();
		RepeatInfo->set_dwuserid(userid);
		RepeatInfo->set_ljettonscore(betVal.lJettonScore);
		RepeatInfo->set_cbjettonarea(betVal.cbJettonArea);
		bIsJetOk = OnUserAddScore(chairId, betVal.cbJettonArea, betVal.lJettonScore, true);
		if (!bIsJetOk)
		{
			openLog("===玩家续押失败:Userid=%d,cbJettonArea=%d,lJettonScore=%d;", userid, betVal.cbJettonArea, betVal.lJettonScore);
			break;
		}
		else
		{
			openLog("===玩家续押:Userid=%d,cbJettonArea=%d,lJettonScore=%d;", userid, betVal.cbJettonArea, betVal.lJettonScore);
		}
	}
	// 重押失败
	if (!bIsJetOk) {
		string err = "托管重押失败";
		SendPlaceBetFail(chairId, 4, alljetton, err);
		//return false;
	}

	//每个区域的总下分
	for (int i = 0; i < MAX_AREA; i++) {
		RepeatJet.add_alljettonscore(m_lAllJettonScore[i]);
	}

	//单个玩家每个区域的总下分
	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		if (!it.second)
			continue;
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		RepeatJet.clear_selfjettonscore();
		for (int i = 0; i < MAX_AREA; ++i)
		{
            RepeatJet.add_selfjettonscore(userinfo->m_lUserJettonScore[i]);
		}

		std::string data = RepeatJet.SerializeAsString();
		if (m_bTestGame != 1)
			m_pITableFrame->SendTableData(userInfoItem->chairId, Hhdn::SUB_S_REPEAT_JETTON_OK, (uint8_t*)data.c_str(), data.size());
	}
	LOG(ERROR) << "---------重押成功--------";
	string str;
	for (int i = 0;i < MAX_AREA;i++)
	{
		if (lAreaRejettonScore[i]>0)
			str += to_string(i) + "_" + to_string(lAreaRejettonScore[i]) + " ";
	}
	openLog("===玩家托管续押成功:Userid=%d,%s;", userid, str.c_str());

	return true;
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
	if (GAME_STATUS_START != m_pITableFrame->GetGameStatus())
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_pITableFrame->GetGameStatus() << " " << GAME_STATUS_START;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[MAX_AREA] = { 0 };
	int64_t userid = Usr->GetUserId();

	Hhdn::CMD_C_ReJettonNew reJetton;
	reJetton.ParseFromArray(data, datasize);
	int reSize = reJetton.ljettonscore_size();
	for (int i = 0;i < reSize;i++)
	{
		m_UserInfo[userid]->m_lPreUserJettonScore[i] = reJetton.ljettonscore(i);
		if (m_UserInfo[userid]->m_lPreUserJettonScore[i] < 0)
		{
			LOG(ERROR) << "---续押押分有效性检查出错--->" << i << " " << m_UserInfo[userid]->m_lPreUserJettonScore[i];
		}
	}
	for (int i = 0; i < MAX_AREA; i++)
	{
		alljetton += m_UserInfo[userid]->m_lPreUserJettonScore[i];
		areajetton[i] = m_UserInfo[userid]->m_lPreUserJettonScore[i];
		LOG(WARNING) << "--m_lPreUserJettonScore-" << areajetton[i];
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
		m_pITableFrame->SendTableData(Usr->GetChairId(), Hhdn::SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
		LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
		return;
	}

	for (int i = 0; i < MAX_AREA; i++)
	{
		// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
		if (m_AreaMaxJetton[i] < m_UserInfo[userid]->m_lUserJettonScore[i] + m_UserInfo[userid]->m_lPreUserJettonScore[i])
		{
			CMD_S_PlaceJettonFail placeJetFail;
			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i);
			placeJetFail.set_lplacescore(m_UserInfo[userid]->m_lPreUserJettonScore[i]);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());
			placeJetFail.set_errmsg("下注已达区域上限");
			std::string data = placeJetFail.SerializeAsString();
			//if (m_bTest==0)
			m_pITableFrame->SendTableData(Usr->GetChairId(), Hhdn::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
			LOG(WARNING) << "---------续押失败(下注已达区域上限)---------" << m_AreaMaxJetton[i] << " < " << m_UserInfo[userid]->m_lUserJettonScore[i] << "+" << m_UserInfo[userid]->m_lPreUserJettonScore[i];
			return;
		}
	}
	 LOG(ERROR) << "---------重押成功--------";
	 int64_t userId = Usr->GetUserId();
	 shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
	 // 清缓存筹码
	 userInfo->clearJetton();
	 for (int i = 0; i < MAX_AREA; i++)
	 {	 
		 int64_t areajetton = userInfo->m_lPreUserJettonScore[i];
		 while (areajetton > 0)
		 {
			 int jSize = 7;
			 for (int j = jSize - 1;j >= 0;j--)
			 {
				 if (areajetton >= m_currentchips[chairid][j])
				 {
                     int64_t tmpVar = m_currentchips[chairid][j];
					 areajetton -= tmpVar;
					 tagBetInfo betinfo = { 0 };
					 betinfo.cbJettonArea = i;
					 betinfo.lJettonScore = tmpVar;
					 userInfo->JettonValues.push_back(betinfo);					 
					 break;
				 }
			 }
		 }
	 }
	 OnUserRejetton(chairid);

	//for (int i = 0; i < MAX_AREA; i++)
	//{
	//	if (areajetton[i] <= 0)
	//	{
	//		continue;
	//	}

	//	while (areajetton[i] > 0)
	//	{
	//		int jSize = 7;
	//		for (int j = jSize - 1;j >= 0;j--)
	//		{
	//			if (areajetton[i] >= m_currentchips[chairid][j])
	//			{
	//				int32_t tmpVar = m_currentchips[chairid][j];
	//				OnUserAddScore(chairid, i, tmpVar, true);
	//				areajetton[i] -= tmpVar;
	//				break;
	//			}
	//		}
	//	}
	//}
}

void CTableFrameSink::AddPlayerJetton(int64_t chairid, int areaid, int64_t score)
{
    assert(chairid<GAME_PLAYER &&areaid<MAX_AREA);

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

    Hhdn::CMD_S_Jetton_Broadcast jetton_broadcast;

    //int iJettonCount =0;
   // double alljetton=0.0;
    int listNum=0;
    bool havejetton=false;
    for (int x=0; x<MAX_AREA; x++)
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

            for (int x=0; x<MAX_AREA; x++)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[x]);
               jetton_broadcast.add_tmpjettonscore(iarr_tmparea_jetton[x]- tmpJttonList[userInfoItem->chairId].iAreaJetton[x]);
            }
            std::string data = jetton_broadcast.SerializeAsString();
			if (m_bTestGame != 1)
				m_pITableFrame->SendTableData(userInfoItem->chairId, Hhdn::SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
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
	 if (m_bTestGame != 1)
		 m_pITableFrame->SendTableData(wChairID, Hhdn::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
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
    uint8_t cbColor = m_hhdnAlgorithm.m_GameLogic.GetCardColor(m_cbTableCardArray[0]);
    uint8_t cbValue = m_hhdnAlgorithm.m_GameLogic.GetCardValue(m_cbTableCardArray[0]);
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

void CTableFrameSink::CheckKickOutUser()
{
	vector<int> clearUserIdVec;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo) {
		userInfoItem = it.second;
		shared_ptr<CServerUserItem> pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
		if (!pUserItem)
			continue;
		if (pUserItem->IsAndroidUser())
			continue;
		if (userInfoItem->NotBetJettonCount >= 5)
		{
			pUserItem->SetUserStatus(sGetoutAtplaying);
			clearUserIdVec.push_back(userInfoItem->userId);
			m_pITableFrame->BroadcastUserStatus(pUserItem, true);
			m_pITableFrame->ClearTableUser(userInfoItem->chairId, true, true);
			LOG(WARNING) << "====连续5局未下注踢出房间====" << " chairid=" << userInfoItem->chairId << " userId=" << userInfoItem->userId;
		}
	}
	uint8_t size = clearUserIdVec.size();
	for (int i = 0; i < size; ++i)
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
	Hhdn::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	int64_t lSelfAllBet = 0;
	for (int i=0;i<MAX_AREA;i++)
	{
		lSelfAllBet += userInfo->m_lUserJettonScore[i];
	}
	details.set_allbetscore(lSelfAllBet); //总投注
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分

    LOG(ERROR)<<"---------------------------------------userInfo->m_EndWinScore"<<userInfo->m_EndWinScore;
	//系统开牌
	int32_t multi = 1;
	for (int i = 0; i < 2; ++i)
	{
		SysCards* card = details.add_syscard();
		card->set_cards(&(m_cbOxCard[i])[0], MAX_COUNT);
		card->set_ty(m_TableCardType[i]);
		card->set_multi(multi);
	}

	//special black red 0 1 2
	for (int betIdx = 0;betIdx < MAX_AREA; betIdx++)
	{
		// 结算时记录
        int64_t winScore = userInfo->dAreaWin[betIdx];
		int64_t betScore = userInfo->m_lUserJettonScore[betIdx];
		double	nMuilt = m_nMuilt[betIdx];// 倍率表m_nMuilt
		//if (betScore > 0) 
		{
			Hhdn::BetAreaRecordDetail* detail = details.add_detail();
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
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
    LOG(ERROR)<<"---------------------------------------strDetail"<<strDetail;
	m_replay.addDetail(userid, chairId, strDetail);
}

// 是否需清零今日总局
bool CTableFrameSink::isNeedClearTodayRoundCount()
{
	auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* ptm = localtime(&tt);
	char date[60] = { 0 };
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
		(int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
		(int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    if ((int)ptm->tm_hour == 0 && (int)ptm->tm_min == 0 && (int)ptm->tm_sec == 0)
	{
		openLog("===今日开奖数据清零,%s;", std::string(date).c_str());
		return true;
	}
	else
	{
		return false;
	}
}

//清理今日所有开奖统计
void CTableFrameSink::ClearTodayRoundCount()
{
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdJudgeTime);
	m_TimerIdJudgeTime = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::ClearTodayRoundCount, this));

	if (isNeedClearTodayRoundCount())
	{
		m_gameWinCount = 0;
		m_gameRoundCount = 0;
		m_todayGameRoundCount = 0;
		memset(m_areaWinCount, 0, sizeof(m_areaWinCount));
		m_openRecord.clear();
		m_openRecordType.clear();
	}
}

string CTableFrameSink::GetAreaName(int index)
{
	string strName = "";

	switch (index)
	{
	case 0:
		strName += "和";
		break;
	case 1:
		strName += "黑牛";
		break;
	case 2:
		strName += "红牛";
		break;
	case 3:
		strName += "牛一";
		break;
	case 4:
		strName += "牛二";
		break;
	case 5:
		strName += "牛三";
		break;
	case 6:
		strName += "牛四";
		break;
	case 7:
		strName += "牛五";
		break;
	case 8:
		strName += "牛六";
		break;
	case 9:
		strName += "牛七";
		break;
	case 10:
		strName += "牛八";
		break;
	case 11:
		strName += "牛九";
		break;
	case 12:
		strName += "牛牛";
		break;
	case 13:
		strName += "特殊牛";
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

		if (!m_bWriteLvLog)
		{
			char Filename[256] = { 0 };
			sprintf(Filename, "./log/Hhdn/Hhdn_%d%d%d.log", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
			FILE *fp = fopen(Filename, "a");
			if (NULL == fp) {
				return;
			}

			va_list arg;
			va_start(arg, p);
			fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			vfprintf(fp, p, arg);
			fprintf(fp, "\n");
			fclose(fp);
		} 
		else
		{
			char Filename[256] = { 0 };
			sprintf(Filename, "./log/Hhdn/Hhdn_Lv_%d%d%d.log", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
			FILE *fp = fopen(Filename, "a");
			if (NULL == fp) {
				return;
			}

			va_list arg;
			va_start(arg, p);
			fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			vfprintf(fp, p, arg);
			fprintf(fp, "\n");
			fclose(fp);
		}
        
    }
}

//测试游戏数据
void CTableFrameSink::TestGameMessage(uint32_t wChairID, int32_t type)
{
	// 游戏状态判断
	if (m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
	{
		return;
	}
    int32_t AreaProbability[MAX_AREA] = {45,400,400,15,15,15,15,15,15,15,15,15,15,5};
	auto RandArea = [&]()
	{
		uint32_t TotalWeight = 0;
		for (int i = 0; i < MAX_AREA; ++i)
			TotalWeight += AreaProbability[i];

		uint32_t randWeight = rand() % TotalWeight;
		uint32_t tempweightsize = 0;
		for (int i = 0; i < MAX_AREA; i++)
		{
			tempweightsize += AreaProbability[i];
			if (randWeight <= tempweightsize)
			{
				return i;
			}
		}
	};
	int betTimes = 0;
	// 测试1-放分; 测试2-收分; 测试3-正常; 测试4 - 保险1;  测试5 - 保险2; 测试6 - 正常;
	int betAllScore[6] = { 0,1000,100,100,900,1500 };
	int tmpbetAllScore = betAllScore[m_nTestType] * 100;
	//每次下注到一个5倍的区域 
    int32_t bJettonArea = RandArea();
	do
	{
		if (m_nTestType < 4)
		{
			/*int randn = 0;
			for (uint8_t i = 0; i < MAX_AREA; i++)
			{
				randn = RandomInt(1, 100);
				if (randn <= 30)
				{
					bJettonArea = i;
					break;
				}
			}*/
			bJettonArea = RandArea();
            if (bJettonArea == -1)
				bJettonArea = RandomInt(0, MAX_AREA - 1);
		}
		//随机可下注金额
		int64_t JettonScore = 0, canGold = 0, TotalWeightGold = 0;
		int randn = 0;
		for (int i = 5; i >= 0; --i)
		{
			if (m_pGameRoomInfo->jettons[i] > tmpbetAllScore)
				continue;
			randn = RandomInt(0, i);
			canGold = m_pGameRoomInfo->jettons[randn];
			tmpbetAllScore -= canGold;
			break;
		}
		//随机下注筹码
		JettonScore = canGold;

		LOG(INFO) << "===随机下注金额===" << JettonScore << " " << bJettonArea;

		Hhdn::CMD_C_PlaceJet mPlaceJet;
		mPlaceJet.set_cbjettonarea(bJettonArea);
		mPlaceJet.set_ljettonscore(JettonScore);

		string content = mPlaceJet.SerializeAsString();
		OnGameMessage(wChairID, Hhdn::SUB_C_PLACE_JETTON, (uint8_t *)content.data(), content.size());
	} while (tmpbetAllScore > 0);

	OnTimerJetton();
}

//更新本轮开奖结果信息
void CTableFrameSink::updateResultList(int resultId)
{
	int winIndex = -1;
	int winIndex2 = -1;
	if (m_TableCardType[BLACK - 1] > 0)
	{
		winIndex = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[BLACK - 1]);
	}
	if (m_TableCardType[RED - 1] > 0)
	{
		winIndex2 = m_hhdnAlgorithm.getAreaIndexByType(m_TableCardType[RED - 1]);
	}
	for (int i = 0;i < MAX_AREA;++i)
	{
		if (i < 3)
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
			if (i == winIndex || i == winIndex2)
			{
				m_notOpenCount[i] = 0;
			}
			else
			{
				m_notOpenCount[i]++;
			}
		}
	}

	memcpy(m_notOpenCount_last, m_notOpenCount, sizeof(m_notOpenCount_last));
}

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
//    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), Hhdn::SUB_S_QUERY_PLAYLIST, (uint8_t*)data.c_str(), data.size());
//    return true;
//}
