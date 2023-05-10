#include <glog/logging.h>
#include "Log.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <deque>
#include <vector>
#include <list>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrameSink.h"
#include "public/ITableFrame.h"

#include "CMD_Game.h"
#include "proto/BBQznn.Message.pb.h"
#include "proto/Game.Common.pb.h"

#include "public/GlobalFunc.h"
#include "proto/GameServer.Message.pb.h"
#include "public/ISaveReplayRecord.h"

#include "public/Random.h"
#include "public/weights.h"
//#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"

#include "GameLogic.h"
#include "TestCase.h"
#include "public/StdRandom.h"

#include "TableFrameSink.h"
//打印日志
#define PRINT_GAME_INFO					1
#define PRINT_GAME_WARNING				1


//////////////////////////////////////////////////////////////////////////
//定时器时间
#define TIME_READY						1					//准备定时器(2S)
#define TIME_ENTER_PLAYING              1
#define TIME_CALL_BANKER				5					//叫庄定时器(5S)
#define TIME_ADD_SCORE					5					//下注定时器(5S)
#define TIME_OPEN_CARD					5					//开牌定时器(5S)
#define TIME_OPEN_CARD_SIX              3                   //开牌定时器(5S)
#define TIME_GAME_END					1					//结束定时器(5S)
#define TIME_MATCH_GAME_END             2				//结束定时器(5S)
//frontend got a animation less then 5s,so after animation they can click the restart button

//////////////////////////////////////////////////////////////////////////////////////
uint8_t CTableFrameSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,3 };		//叫庄倍数
int64_t CTableFrameSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 1,5,8,11,15 };	//下注倍数

int64_t CTableFrameSink::m_llStorageControl = 0;
int64_t CTableFrameSink::m_llTodayStorage = 0;
int64_t CTableFrameSink::m_llStorageLowerLimit = 0;
int64_t CTableFrameSink::m_llStorageHighLimit = 0;
bool  CTableFrameSink::m_isReadstorage = false;
bool  CTableFrameSink::m_isReadstorage1 = false;
bool  CTableFrameSink::m_isReadstorage2 = false;

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/bbqznn";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("bbqznn");
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit;	// 声明全局变局,初始化库.


CTableFrameSink::CTableFrameSink(void) :  m_dwCellScore(1.0), m_pITableFrame(NULL), m_iTodayTime(time(NULL))
  ,m_isMatch(false)
{
    m_wGameStatus = GAME_STATUS_INIT;
    //清理游戏数据
    ClearGameData();
    //初始化数据
    InitGameData();

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        PaoMaDengCardType[i]=-1;
        PaoMaDengWinScore[i]=-1;
        m_bPlayerIsOperation[i] = false;
    }

    m_wGameStatus = GAME_STATUS_INIT;
    m_nLottery = 0;
    m_bContinueServer = true;
    if(ENABLE_TEST)
    {
        testCase = make_shared<CTestCase>();
        testCase->setCardCount(m_bPlaySixChooseFive ? 6 : 5);
    }
   // memset(m_SitTableFrameStatus, 0, sizeof(m_SitTableFrameStatus));
   // m_TimerLoopThread.startLoop();
	////////////////////////////////////////
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 3.6f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 1.4f;
	//放大倍数
	ratioGamePlayerWeightScale_ = 1000;

	MaxGamePlayers_ = 0;

    m_playGameType = PlayGameType_Normal;
    m_bPlaySixChooseFive = false;
    m_bHaveKing = false;
    uint8_t Multiple[CT_MAXCOUNT] = {1,1,1,1,1,1,1,2,2,2,3,5,5,5,5,5,5,5};
    for (int i = 0; i < CT_MAXCOUNT; ++i)
    {
        m_iTypeMultiple[i] = Multiple[i];
    }
    m_testMakeType = 0;
    m_testSameTypeCount = 0;
    m_bTestCount = 0;
    stockWeak=0.0f;
	m_bTestMakeTwoking = false;
}

CTableFrameSink::~CTableFrameSink(void)
{

}
void CTableFrameSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//bbqznn//bbqznn_table_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
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
//设置指针
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    assert(NULL != pTableFrame);
    m_pITableFrame = pTableFrame;
    if(m_pITableFrame){
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();        
        uint8_t jsize = m_pGameRoomInfo->jettons.size();
        for(uint8_t i=0; i < jsize; i++)
        {
            m_cbJettonMultiple[i] = m_pGameRoomInfo->jettons[i];
        }
       // memcpy(m_cbJettonMultiple, (vector<uint8_t>)m_pGameRoomInfo->jettons, sizeof(m_cbJettonMultiple));
        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        // if(m_pGameRoomInfo->roomId - m_pGameRoomInfo->gameId*10 > 20)
        //     m_isMatch=true;

    }
    //初始化游戏数据
    m_wGameStatus = GAME_STATUS_INIT;

    memset(ratioGamePlayerWeight_,0,sizeof(ratioGamePlayerWeight_));
    //初始化桌子游戏人数概率权重
    // for (int i = m_pGameRoomInfo->minPlayerNum-1; i < m_pGameRoomInfo->maxPlayerNum; ++i) {
    //     if (i == 4) {
    //         //5人局的概率0
    //         ratioGamePlayerWeight_[i] = 0;//25 * ratioGamePlayerWeightScale_;
    //     }
    //     else if (i == 3) {
    //         //4人局的概率85%
    //         ratioGamePlayerWeight_[i] = 85 * ratioGamePlayerWeightScale_;
    //     }
    //     else if (i == 2) {
    //         //3人局的概率10%
    //         ratioGamePlayerWeight_[i] = 10 * ratioGamePlayerWeightScale_;
    //     }
    //     else if (i == 1) {
    //         //2人局的概率5%
    //         ratioGamePlayerWeight_[i] = 5 * ratioGamePlayerWeightScale_;
    //     }
    //     else if (i == 0) {
    //         //单人局的概率5%
    //         ratioGamePlayerWeight_[i] = 0;//5 * ratioGamePlayerWeightScale_;
    //     }
    //     else {
    //         ratioGamePlayerWeight_[i] = 0;
    //     }
    // }
    // //计算桌子要求标准游戏人数
    // poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);

    InitGameData();
    //更新游戏配置
    InitLoadini();
    ReadConfigInformation();

    //计算桌子要求标准游戏人数
    poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
    //庄家
    m_wBankerUser = INVALID_CHAIR;
    //当前筹码
   // memset(m_cbCurJetton, 0, sizeof(m_cbCurJetton));
}

//初始化游戏数据
void CTableFrameSink::InitGameData()
{
    //随机种子
    srand((unsigned)time(NULL));

    // //游戏状态
    //   m_wGameStatus = GAME_STATUS_INIT;
    g_nRoundID = 0;
    //游戏中玩家个数
    m_wPlayCount = 0;
    //游戏中的机器的个数
    m_wAndroidCount = 0;
    m_bContinueServer = true;
    //游戏中玩家
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
    //牌数据
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    //牛牛牌数据
    memset(m_cbOXCard, 0, sizeof(m_cbOXCard));

    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_sMaxCardType,0,sizeof(m_sMaxCardType));
    memset(m_cbKingChangCard,0,sizeof(m_cbKingChangCard));
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_sCallBanker[i] = -1;								//玩家叫庄(-1:未叫, 0:不叫, 124:叫庄)
        m_sAddScore[i] = -1;								//玩家下注(-1:未下注, 大于-1:对应筹码索引)
        m_sCardType[i] = -1;								//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
        m_iMultiple[i] = 1;									//倍数

        m_bPlayerIsOperation[i] = false;
    }

    //时间初始化
    m_dwReadyTime = 0;										//ready时间
    m_dwEnterPlayingTime = 0;						        //EnterPlaying时间
    m_dwCallTime = 0;										//叫庄时间
    m_dwScoreTime = 0;										//下注时间
    m_dwOpenTime = 0;										//开牌时间
    m_dwEndTime = 0;										//结束时间
   // m_dwStartTime = 0;										//开始时间

    //基础积分
    if(m_pITableFrame)
    {
        m_dwCellScore = m_pGameRoomInfo->floorScore;//m_pITableFrame->GetGameCellScore();
    }

   // memcpy(m_cbCurJetton, m_cbJettonMultiple, sizeof(m_cbJettonMultiple));
    m_cbLeftCardCount = m_bHaveKing ? MAX_CARD_TOTAL_KING : MAX_CARD_TOTAL;
    memset(m_cbRepertoryCard, 0, sizeof(m_cbRepertoryCard));
    //游戏记录
   // memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_bPlayerIsOperation[i]=false;
    }

    m_replay.clear();
    m_iBlacklistNum = 0;
}
void CTableFrameSink::InitLoadini()
{
    if(!boost::filesystem::exists("./conf/bbqznn_config.ini"))
    {
        LOG_ERROR<<"No bbqznn_config";
        return;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/bbqznn_config.ini",pt);

    stockWeak=pt.get<double>("GAME_CONF.STOCK_WEAK",1.0);
    ratioGamePlayerWeight_[0] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_0",10);
    ratioGamePlayerWeight_[1] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_1",20);
    ratioGamePlayerWeight_[2] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_2",30);
    ratioGamePlayerWeight_[3] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_3",40);

	int64_t nTestMakeTwoking = pt.get<int64_t>("GAME_CONF.TestMakeTwoking", 0);
	if (nTestMakeTwoking == 1)
	{
		m_bTestMakeTwoking = true;     //测试带两个王的情况
	}
	else
	{
		m_bTestMakeTwoking = false;
	}
}
//读取配置
void CTableFrameSink::ReadConfigInformation()
{
    m_isMatch = false;//m_pGameRoomKind->roomId-m_pGameRoomKind->gameId*10 > 20;
    //设置文件名
    const TCHAR * p = nullptr;
    TCHAR szPath[MAX_PATH] = TEXT("");
    TCHAR szConfigFileName[MAX_PATH] = TEXT("");
    // szConfigFileName[MAX_PATH] = TEXT("");
    TCHAR OutBuf[255] = TEXT("");
    CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
    myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/bbqznn_config_%d.ini"), szPath,m_pGameRoomInfo->roomId/10);

    int readSize = GetPrivateProfileString(TEXT("GAME_CONFIG"), TEXT("TypeMultiple"), _T"1,1,1,1,1,1,1,2,2,2,3,5,5,5,5,5,5,5", OutBuf, sizeof(OutBuf), szConfigFileName);
    ASSERT(readSize > 0);
    p = OutBuf;
    for (int i = 0; i < CT_MAXCOUNT; ++i)
    {
        float temp = 0;        
        if(p==nullptr)
            return;
        temp = atoi(p);
        m_iTypeMultiple[i] = temp;
        p = mystchr(p, ',') + 1;
    }

    int test = 0;
    test =1;

    //获取当前玩法类型
    int type = m_pGameRoomInfo->roomId%100/10 + 1;
    m_bHaveKing = false;
    m_bPlaySixChooseFive = false;
    m_playGameType = GetPrivateProfileInt(TEXT("GAME_CONFIG"), TEXT("PlayGameType"),1,szConfigFileName);
    int nChooseFive = GetPrivateProfileInt(TEXT("GAME_CONFIG"), TEXT("SixChooseFive"),0,szConfigFileName);
    if (nChooseFive==1)
    {
        m_bPlaySixChooseFive = true;     //6选5
    }
    int nHaveKing = GetPrivateProfileInt(TEXT("GAME_CONFIG"), TEXT("HaveKing"),0,szConfigFileName);
    if (nHaveKing==1)
    {
        m_bHaveKing = true;     //百变牛
    }
    m_testMakeType = GetPrivateProfileInt(TEXT("GAME_CONFIG"), TEXT("testMakeType"),10,szConfigFileName);
    m_testSameTypeCount = GetPrivateProfileInt(TEXT("GAME_CONFIG"), TEXT("testSameTypeCount"),10,szConfigFileName);
    m_bTestCount = m_testSameTypeCount;
    m_GameLogic.SetIsHaveKing(m_bHaveKing);
    m_GameLogic.SetSixChooseFive(m_bPlaySixChooseFive);
	
}

void CTableFrameSink::PaoMaDeng()
{
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_cbPlayStatus[i] != 1)
            continue;
        if(PaoMaDengWinScore[i] >= m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
            int sgType = SMT_GLOBAL|SMT_SCROLL;
            m_pITableFrame->SendGameMessage(i, "", sgType, PaoMaDengWinScore[i]);
        }

    }
}

int CTableFrameSink::IsMaxCard(int wChairID)
{
    int isMaxCard = 1;

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i] || i == wChairID)
            continue;
        if(!m_GameLogic.CompareCard(m_cbCardData[wChairID], m_cbCardData[i],m_cbKingChangCard[wChairID],m_cbKingChangCard[i],false))
            isMaxCard = 0;
    }
    return isMaxCard;
}

void CTableFrameSink::OnTimerGameReadyOver()
{
	//assert(m_wGameStatus < GAME_STATUS_START);
    ////////////////////////////////////////
	//销毁当前旧定时器
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
	if (m_wGameStatus >= GAME_STATUS_START) {
		return;
	}
	////////////////////////////////////////
	//计算累计匹配时间，当totalMatchSeconds_ >= timeoutMatchSeconds_时，
	//如果桌子游戏人数不足会自动开启 CanJoinTable 放行机器人进入桌子补齐人数
	totalMatchSeconds_ += sliceMatchSeconds_;
	////////////////////////////////////////
	//达到游戏人数要求开始游戏
	if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
		assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
	}
	else {
		////////////////////////////////////////
		//桌子游戏人数不足，且没有匹配超时，再次启动定时器
		if (totalMatchSeconds_ < timeoutMatchSeconds_) {
			m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
			////////////////////////////////////////
			//桌子游戏人数不足，且机器人候补空位超时
			if (m_pITableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
				////////////////////////////////////////
				//达到最小游戏人数，开始游戏
				openLog("--- 桌子游戏人数不足，且机器人候补空位超时 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
				GameTimerReadyOver();
			}
			else {
				////////////////////////////////////////
				//仍然没有达到最小游戏人数，继续等待
				m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			////////////////////////////////////////
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
			// printf("--- *** --------- 匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", totalMatchSeconds_, MaxGamePlayers_);
			////////////////////////////////////////
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
	}
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    //////////////////测试机器人玩注释//////////////////////
    // if (m_pITableFrame->GetPlayerCount(true) == 0) {
    //     ////////////////////////////////////////
    //     //第一个进入房间的也必须是真人
    //     //assert(m_pITableFrame->GetPlayerCount(false) == 1);
    //     ////////////////////////////////////////
    //     //权重随机乱排
    //     poolGamePlayerRatio_.shuffleSeq();
    //     ////////////////////////////////////////
    //     //初始化桌子当前要求的游戏人数
    //     MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
    //     ////////////////////////////////////////
    //     //重置累计时间
    //     totalMatchSeconds_ = 4;
    //     // printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 测试机器人玩 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
    // }
    ////////////////////////////////////////
    //初始化或空闲准备阶段，可以进入桌子
    if(m_wGameStatus == GAME_STATUS_END){
        return false;
    }
    if (m_wGameStatus < GAME_STATUS_LOCK_PLAYING)
    {
        ////////////////////////////////////////
        //达到游戏人数要求禁入游戏
        if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
            return false;
        }
        ////////////////////////////////////////
        //timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
        if (pUser->GetUserId() == -1) {
            if (totalMatchSeconds_ < timeoutMatchSeconds_) {
               // printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
                return false;
            }
        }
        else {
            ////////////////////////////////////////
            //timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
            if (userItem) {
                if (userItem->IsAndroidUser()) {
                    if (totalMatchSeconds_ < timeoutMatchSeconds_) {
                       // printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
                        return false;
                    }
                }
                else {
                    //真实玩家底分不足，不能进入房间
                }
            }
        }
        return true;
    }
    if(m_wGameStatus > GAME_STATUS_LOCK_PLAYING )
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
        return pIServerUserItem != NULL;
    }
    ////////////////////////////////////////
    //游戏状态为GAME_STATUS_LOCK_PLAYING(100)时，不会进入该函数
    assert(false);
    return false;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG(ERROR) << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return true;
    }
   // uint32_t chairId = pIServerUserItem->GetChairId();
    if( m_wGameStatus < GAME_STATUS_LOCK_PLAYING || m_wGameStatus == GAME_STATUS_END)// ||  !m_bJoinedGame[chairId]
        return true;
    else
        return false;
}


//发送场景
bool CTableFrameSink::OnUserEnter(int64_t userId, bool bisLookon)
{
	//muduo::MutexLockGuard lock(m_mutexLock);

	LOG(ERROR) << "---------------------------OnUserEnter---------------------------";

	uint32_t chairId = INVALID_CHAIR;

	shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);

	if (!pIServerUserItem)
	{
		LOG(ERROR) << "-----------------OnUserEnter--Fail----pIServerUserItem==NULL----------------";
		return false;
	}

	chairId = pIServerUserItem->GetChairId();
	LOG(ERROR) << "-------------OnUserEnter------user->GetChairID()=" << pIServerUserItem->GetChairId() << "---------------------";
	if (chairId >= GAME_PLAYER)
	{
		LOG(ERROR) << "---------------------------OnUserEnter--Fail----chairId=" << chairId << "---------------------";
		return false;
	}

	//if (m_wGameStatus == GAME_STATUS_INIT) {
	if (m_pITableFrame->GetPlayerCount(true) == 1) {
		////////////////////////////////////////
		//第一个进入房间的也必须是真人
        //////////////////测试机器人玩注释//////////////////////
        assert(m_isMatch||m_pITableFrame->GetPlayerCount(false) == 1);
		////////////////////////////////////////
		//权重随机乱排
		// poolGamePlayerRatio_.shuffleSeq();
		// ////////////////////////////////////////
		// //初始化桌子当前要求的游戏人数
		// MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
		////////////////////////////////////////
        int sum = 0;
        int index=0;
        for (int i = 0; i < GAME_PLAYER; ++i) {
            sum += ratioGamePlayerWeight_[i];
        }
        if (sum <= 1) {
            index = 0;
        }
        int r=0;
        if(sum>0)
        r= 1+rand()%(sum); //m_random.betweenInt(1,sum).randInt_mt(true) ;
        int c = r;
        for (int i = 0; i < GAME_PLAYER; ++i) {
            c -= ratioGamePlayerWeight_[i];
            if (c <= 0) {
                index = i+1;
                break;
            }
        }
        if(index<2)
        {
            index=2;
        }
        MaxGamePlayers_=index;
		//重置累计时间
		totalMatchSeconds_ = 0;
		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
	}
	if (m_wGameStatus < GAME_STATUS_START) {
		assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
		assert(MaxGamePlayers_ <= GAME_PLAYER);
	}
    m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
    m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
    m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);

    if(m_wGameStatus != GAME_STATUS_INIT )
    {
       OnEventGameScene(chairId,false);
    }
   // m_SitTableFrameStatus[dwChairID] = 2;

    if(m_wGameStatus < GAME_STATUS_READY)
    {
        int  wUserCount = 0;
        //用户状态
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //获取用户
            shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
            if(pIServerUser/* && m_SitTableFrameStatus[i] > 0*/)
            {
                ++wUserCount;
            }
        }
        LOG(ERROR)<<"---------Player iGAME_STATUS_INIT---------------------"<<m_pITableFrame->GetTableId()<<" wUserCount"<<wUserCount;
        if( wUserCount >= m_pGameRoomInfo->minPlayerNum)
        {
            ClearAllTimer();

            m_wGameStatus = GAME_STATUS_READY;
            LOG(ERROR)<<"-----------------m_wGameStatus == GAME_STATUS_READY && wUserCount >= "<< m_pGameRoomInfo->minPlayerNum << ",tableid:"<< m_pITableFrame->GetTableId();
            m_dwStartTime =  chrono::system_clock::now();//(uint32_t)time(NULL);
            m_dwReadyTime = (uint32_t)time(NULL);

            //m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
            LOG(ERROR)<<"---------------------------m_TimerIdReadyOver Start 1S---------------------------";
        }
    }

    if(m_wGameStatus == GAME_STATUS_LOCK_PLAYING)
    {
        pIServerUserItem->SetUserStatus(sReady);
    }

    LOG(ERROR)<<"---------------------------OnUserEnter-----m_wGameStatus----------------------"<<m_wGameStatus;
	if (m_wGameStatus < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到桌子要求游戏人数，开始游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			////////////////////////////////////////
			//不超过桌子要求最大游戏人数
			assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
			// printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
			//GameTimereEnterPlayingOver();
			GameTimerReadyOver();
		}
		else {
			if (m_pITableFrame->GetPlayerCount(true) == 1) {
				////////////////////////////////////////
				//第一个进入房间的也必须是真人
                //////////////////测试机器人玩注释//////////////////////
                assert(m_isMatch||m_pITableFrame->GetPlayerCount(false) == 1);
				ClearAllTimer();
				////////////////////////////////////////
				//修改游戏准备状态
				//m_pITableFrame->SetGameStatus(GAME_STATUS_READY);
				//m_cbGameStatus = GAME_STATUS_READY;
				m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
			}
		}
	}

    return true;
}
/*
bool CTableFrameSink::RepayRecordGameScene(int64_t chairid, bool bisLookon)
{
    uint32_t dwChairID = chairid;
    shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(chairid);
    if(!pIServerUser)
    {
        return true;
    }

    dwChairID = pIServerUser->GetChairId();
    if(dwChairID >= GAME_PLAYER || dwChairID != chairid)
    {
        return true;
    }
    switch (m_wGameStatus)
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_LOCK_PLAYING:
    {
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分


        uint32_t dwStartTime = 0;
        if(m_wGameStatus == GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_wGameStatus == GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());


        //发送数据
        std::string data = GameFree.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        NN_MSG_GS_CALL GameCall;
        GameCall.set_dcellscore(m_dwCellScore);
        uint32_t dwCallTime = (uint32_t)time(NULL) - m_dwCallTime;
        uint32_t cbTimeLeave = dwCallTime >= TIME_CALL_BANKER ? 0 : dwCallTime;		//剩余时间
        GameCall.set_cbtimeleave(cbTimeLeave);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_scallbanker(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未叫, 0:不叫, 124:叫庄)

            //LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
            GameCall.add_cbjettonmultiple(m_cbJettonMultiple[i]);

        for (int i = 0; i < MAX_BANKER_MUL; ++i)
            GameCall.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameCall.set_dwuserid(pIServerUser->GetUserId());
//        GameCall.set_cbgender(pIServerUser->GetGender());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameCall.set_cbviplevel(pIServerUser->GetVipLevel());
        GameCall.set_sznickname(pIServerUser->GetNickName());
//        GameCall.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameCall.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_CALL, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
        NN_MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
        GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户
        GameScore.set_cbbankermultiple(m_sCallBanker[m_wBankerUser]);

        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbuserjettonmultiple(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            if (m_cbPlayStatus[i] == 0)
                continue;

            int cbUserJettonMultiple =((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameScore.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameScore.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }

        GameScore.set_dwuserid(pIServerUser->GetUserId());
//        GameScore.set_cbgender(pIServerUser->GetGender());
        GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
        GameScore.set_sznickname(pIServerUser->GetNickName());
//        GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameScore.set_szlocation(pIServerUser->GetLocation());
        GameScore.set_usstatus(pIServerUser->GetUserStatus());
        GameScore.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameScore.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_SCORE, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_OPEN:		//开牌场景消息
    {
        NN_MSG_GS_OPEN GameOpen;
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >= TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);

        //LOG(INFO)<<"GameOpen.set_wbankeruser(m_wBankerUser)=="<<(int)m_wBankerUser;
        //庄家用户
        GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser]);		//庄家倍数

        //LOG(INFO)<<"GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser])=="<<(int)m_sCallBanker[m_wBankerUser];

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameOpen.add_cbjettonmultiple(0);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.add_cbcallbankermultiple(0);
        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
                GameOpen.add_cbhintcard(0);
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

            GameOpen.add_cbuserjettonmultiple(0);//游戏中玩家(1打牌玩家)

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1))
            {
                if (i == dwChairID)
                {
                    int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);

                    LOG(INFO)<<"---------------------------------open card"<<cardtype;

                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                        GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
                else
                {   int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                GameOpen.set_cbcardtype(i,cardtype);

                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                    GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                }
            }

            int cbUserJettonMultiple = m_sAddScore[i] >= 0 ? m_cbJettonMultiple[m_sAddScore[i]] : 0;
            GameOpen.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameOpen.set_cbjettonmultiple(i,m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.set_cbcallbankermultiple(i,m_cbBankerMultiple[i]);
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
//        GameOpen.set_cbgender(pIServerUser->GetGender());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameOpen.set_cbviplevel(pIServerUser->GetVipLevel());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
//        GameOpen.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameOpen.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        //LOG(INFO)<<"GAME_STATUS_OPEN-------------------------------User Enter"<<(int)dwChairID;

        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
            //LOG(INFO)<<"---------------------------------open card"<<res[i];
        }
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_OPEN, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);
        //memcpy(GameFree.cbAddJettonMultiple, m_cbJettonMultiple, sizeof(GameFree.cbAddJettonMultiple));
        //memcpy(GameFree.cbCallBankerMultiple, m_cbBankerMultiple, sizeof(GameFree.cbAddJettonMultiple));

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        //LOG(INFO)<<"GAME_STATUS1_END-------------------------------User Enter"<<(int)dwChairID;

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
//        }
        break;
    }
    default:
        break;
    }

    return true;
}

*/
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    //muduo::MutexLockGuard lock(m_mutexLock);

    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";
    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";
    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";

    uint32_t dwChairID = chairId;
    shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUser)
    {
        return false;
    }

    dwChairID = pIServerUser->GetChairId();
    if(dwChairID >= GAME_PLAYER || dwChairID != chairId)
    {
        return false;
    }
    switch (m_wGameStatus)
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_LOCK_PLAYING:
    {
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分

       // int wUserCount = 0;
       // for (int i = 0; i < GAME_PLAYER; ++i)
       // {
       //     if (m_pITableFrame->IsExistUser(i) && m_inPlayingStatus[i]>0)
       //     {
       //         ++wUserCount;
       //     }
       // }

        uint32_t dwStartTime = 0;
        if(m_wGameStatus == GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_wGameStatus == GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

       // for (int i = 0; i < MAX_JETTON_MUL; ++i)
       //     GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
       // for (int i = 0; i < MAX_BANKER_MUL; ++i)
       //     GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameFree.set_dwuserid(pIServerUser->GetUserId());
       // GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
       // GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
       // GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
       // GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        GameFree.set_bplaysixchoosefive(m_bPlaySixChooseFive);
        GameFree.set_bhaveking(m_bHaveKing);

        LOG(INFO)<<"GAME_STATUS1_FREE-------------------------------User Enter"<<(int)dwChairID;

        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        NN_MSG_GS_CALL GameCall;
        GameCall.set_dcellscore(m_dwCellScore);
        uint32_t dwCallTime = (uint32_t)time(NULL) - m_dwCallTime;
        uint32_t cbTimeLeave = dwCallTime >= TIME_CALL_BANKER ? 0 : dwCallTime;		//剩余时间
        GameCall.set_cbtimeleave(cbTimeLeave);
        GameCall.set_roundid(mGameIds);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
             GameCall.add_scallbanker(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未叫, 0:不叫, 124:叫庄)

            LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
        }

       // for (int i = 0; i < MAX_JETTON_MUL; ++i)
       //     GameCall.add_cbjettonmultiple(m_cbJettonMultiple[i]);

        for (int i = 0; i < MAX_BANKER_MUL; ++i){
             GameCall.add_cbcallbankermultiple(m_cbCallable[chairId][i]);
            // GameCall.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }



        GameCall.set_dwuserid(pIServerUser->GetUserId());
       // GameCall.set_cbgender(pIServerUser->GetGender());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
       // GameCall.set_cbviplevel(pIServerUser->GetVipLevel());
        GameCall.set_sznickname(pIServerUser->GetNickName());
       // GameCall.set_cbviplevel2(pIServerUser->GetVipLevel());
       // GameCall.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());
        GameCall.set_bplaysixchoosefive(m_bPlaySixChooseFive);
        GameCall.set_bhaveking(m_bHaveKing);

        //发送数据
        std::string data = GameCall.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
        NN_MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
        GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户
        GameScore.set_cbbankermultiple(m_sCallBanker[m_wBankerUser]);
        GameScore.set_roundid(mGameIds);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbuserjettonmultiple(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            if (m_cbPlayStatus[i] == 0)
                continue;

            int cbUserJettonMultiple = ((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonable[i][m_sAddScore[i]] : 0) ;
                    //((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameScore.add_cbjettonmultiple(m_cbJettonable[chairId][i]);//m_cbJettonMultiple[i]);
        }
       // for (int i = 0; i < MAX_BANKER_MUL; ++i)
       // {
       //     GameScore.add_cbcallbankermultiple(m_cbCallable[chairId][i]);//m_cbBankerMultiple[i]);
       // }

        GameScore.set_dwuserid(pIServerUser->GetUserId());
       // GameScore.set_cbgender(pIServerUser->GetGender());
        GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
       // GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
        GameScore.set_sznickname(pIServerUser->GetNickName());
       // GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
       // GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameScore.set_szlocation(pIServerUser->GetLocation());
        GameScore.set_usstatus(pIServerUser->GetUserStatus());
        GameScore.set_wchairid(pIServerUser->GetChairId());
        GameScore.set_bplaysixchoosefive(m_bPlaySixChooseFive);
        GameScore.set_bhaveking(m_bHaveKing);
        //发送数据
        std::string data = GameScore.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_OPEN:		//开牌场景消息
    {
        NN_MSG_GS_OPEN GameOpen;
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >= TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        int mycardtype = 0;  //自己是手牌牌型
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);
        GameOpen.set_roundid(mGameIds);
        LOG(INFO)<<"GameOpen.set_wbankeruser(m_wBankerUser)=="<<(int)m_wBankerUser;
        //庄家用户
        GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser]);		//庄家倍数

        LOG(INFO)<<"GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser])=="<<(int)m_cbCallable[chairId][m_sCallBanker[m_wBankerUser]];

       // for (int i = 0; i < MAX_JETTON_MUL; ++i)
       // {
       //     GameOpen.add_cbjettonmultiple(0);
       // }
       // for (int i = 0; i < MAX_BANKER_MUL; ++i)
       // {
       //     GameOpen.add_cbcallbankermultiple(0);
       // }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_bPlaySixChooseFive)
            {
                for(int j=0;j<MAX_COUNT_SIX;j++)
                {                     
                    GameOpen.add_cbcarddata(0);
                    if (j<MAX_COUNT)
                        GameOpen.add_cbhintcard(0);
                }
            }
            else
            {
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.add_cbcarddata(0);
                    GameOpen.add_cbhintcard(0);
                }
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

            GameOpen.add_cbuserjettonmultiple(0);//游戏中玩家(1打牌玩家)
			GameOpen.add_cbtypemultiple(0);//游戏牌型默认最小
        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1)) //未开牌
            {
                int cardtype = 0;                
                if (i == dwChairID)
                {
                    int cardtype =(int)m_sMaxCardType[i];//m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    uint8_t cbOXCard[MAX_COUNT] = { 0 };
                    uint8_t kingChangCard[2] = {0};
                    uint8_t cbMaxCardValue[MAX_COUNT] = { 0 };
                    if (m_bPlaySixChooseFive)
                    {
                        //获得前5张手牌的牛牛牌
                        for (int j = 0; j < MAX_COUNT; ++j)
                        {
                            cbOXCard[j] = m_cbHandCardData[i][j];
                        }
                        if (m_bHaveKing)
                        {
                            cardtype = m_GameLogic.GetHaveKingBigCardType(cbOXCard,cbMaxCardValue,5,kingChangCard);
                            m_GameLogic.GetOxCard(cbOXCard,kingChangCard);
                        }
                        else
                        {
                            m_GameLogic.GetOxCard(cbOXCard,kingChangCard);
                            cardtype = (int)m_GameLogic.GetCardType(cbOXCard, MAX_COUNT);
                        }                        
                        memcpy(&m_cbOXCard[i], cbOXCard, sizeof(cbOXCard));
                    }

                    GameOpen.set_cbcardtype(i,cardtype);
                    mycardtype = cardtype;
                    LOG(INFO)<<"---------------------------------open card"<<cardtype;

                    int startidx = i * MAX_COUNT;
                    int startidx_six = i * MAX_COUNT_SIX;
                    if (m_bPlaySixChooseFive)
                    {
                        for(int j=0;j<MAX_COUNT_SIX;j++)
                        {                     
                            GameOpen.set_cbcarddata(startidx_six+j,(int) m_cbHandCardData[i][j]);
                            if (j<MAX_COUNT)
                                GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                        }
                    }
                    else
                    {
                        for(int j=0;j<MAX_COUNT;j++)
                        {                        
                            GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                            GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                        }
                    }                    
                }
                //else
                //{   
                //    cardtype =(int)m_sMaxCardType[i];//m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                //    GameOpen.set_cbcardtype(i,cardtype);
                //}
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
				int cardtype = (int)m_sMaxCardType[i];//m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                GameOpen.set_cbcardtype(i,cardtype);
                if (i==dwChairID)
                    mycardtype = cardtype;
                int startidx = i * MAX_COUNT;
                int startidx_six = i * MAX_COUNT_SIX;
                if (m_bPlaySixChooseFive)
                {
                    for(int j=0;j<MAX_COUNT_SIX;j++)
                    {                     
                        GameOpen.set_cbcarddata(startidx_six+j,(int) m_cbHandCardData[i][j]);
                        if (j<MAX_COUNT)
                            GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
                else
                {
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                        GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
				GameOpen.set_cbtypemultiple(i, m_iTypeMultiple[cardtype]);//已开牌玩家牌型倍数
            }

            int cbUserJettonMultiple = ((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonable[i][m_sAddScore[i]] : 0) ;
                    //((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameOpen.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
           // GameOpen.set_cbjettonmultiple(i,m_cbJettonMultiple[i]);
            GameOpen.add_cbjettonmultiple(m_cbJettonable[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.add_cbcallbankermultiple(m_cbCallable[chairId][i]);//GameOpen.set_cbcallbankermultiple(i,m_cbBankerMultiple[i]);
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
       // GameOpen.set_cbgender(pIServerUser->GetGender());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
       // GameOpen.set_cbviplevel(pIServerUser->GetVipLevel());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
       // GameOpen.set_cbviplevel2(pIServerUser->GetVipLevel());
       // GameOpen.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        LOG(INFO)<<"GAME_STATUS_OPEN-------------------------------User Enter"<<(int)dwChairID;
        GameOpen.set_bplaysixchoosefive(m_bPlaySixChooseFive);
        GameOpen.set_bhaveking(m_bHaveKing);        
        GameOpen.set_typemultiple(m_iTypeMultiple[mycardtype]); //牌型对应的倍数

        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
             LOG(INFO)<<"---------------------------------open card"<<res[i];
        }
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());

        break;
    }
#if 0
    case GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);
        //memcpy(GameFree.cbAddJettonMultiple, m_cbJettonMultiple, sizeof(GameFree.cbAddJettonMultiple));
        //memcpy(GameFree.cbCallBankerMultiple, m_cbBankerMultiple, sizeof(GameFree.cbAddJettonMultiple));

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
           // GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
            GameFree.add_cbjettonmultiple(m_cbJettonable[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
           // GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
            GameFree.add_cbcallbankermultiple(m_cbCallable[chairId][i]);
        }


        GameFree.set_dwuserid(pIServerUser->GetUserId());
       // GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
       // GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
       // GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
       // GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        LOG(INFO)<<"GAME_STATUS_END-------------------------------User Enter"<<(int)dwChairID;

        break;
    }
#endif
    default:
        break;
    }

    if(pIServerUser->GetUserStatus() == sOffline)  //left failt
    {
        pIServerUser->SetUserStatus(sPlaying);

        //tmp......
       // if(m_bReplayRecordStart)
       // {
       //     RepayRecorduserinfoStatu(pIServerUser->GetChairId(), sPlaying);
       // }
    }

   // if(pIServerUser->GetUserStatus()==sOffline)
   // {
   //     pIServerUser->SetUserStatus(sReady);
   // }

    m_bPlayerIsOperation[chairId] = true;
    return true;
}

bool CTableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    return true;
}

//用户离开
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
{
    LOG(ERROR)<<"---------------------------OnUserLeft-----------m_wGameStatus----------------"<<m_wGameStatus;
    if(!CanLeftTable(userId))
    {
        return false;
    }

    bool ret = false;
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userId);
    if(!player)
    {
        LOG(ERROR)<<"---------------------------player == NULL---------------------------";
        return false;
    }
    int chairID = player->GetChairId();
    //if(m_wGameStatus==GAME_STATUS1_FREE||m_wGameStatus==GAME_STATUS_END||m_cbPlayStatus[chairID]==0)
    if(m_wGameStatus == GAME_STATUS_INIT || m_wGameStatus == GAME_STATUS_READY ||
            m_wGameStatus == GAME_STATUS_END || m_cbPlayStatus[chairID] == 0)
    {
        // if(player->GetUserStatus() != sPlaying)
        // {
            //tmp......
           // if(m_bReplayRecordStart)
           // {
           //     RepayRecorduserinfoStatu(chairID,0);
           // }
            //            m_inPlayingStatus[chairID] = 0;
            LOG(ERROR)<<"---------------------------player->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << chairID;
           // openLog("7OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",player->GetUserId(),player->GetChairId());
            m_cbPlayStatus[chairID] = 0;
            player->SetUserStatus(sGetout);
            m_pITableFrame->BroadcastUserStatus(player, true);
            m_pITableFrame->ClearTableUser(chairID, true, true);
           // player->SetUserStatus(sGetout);

            ret = true;
        // }
    }

    if(GAME_STATUS_READY == m_wGameStatus)
    {
        int userNum = 0;
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i]>0*/)
            {
                userNum++;
            }
           // if(m_inPlayingStatus[i]>0)
           //     usernum++;
        }

        if(userNum < m_pGameRoomInfo->minPlayerNum)
        {
            //m_pITableFrame->KillGameTimer(ID_TIME_READY);
           // m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
            ClearAllTimer();

            m_wGameStatus = GAME_STATUS_INIT;

            LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT;------------------------";
        }
    }

    // if(player && m_pITableFrame)
    // {
    //     if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    //     {
    //         player->SetUserStatus(sOffline);
    //     }
    //     else
    //     {
    //         player->SetUserStatus(sGetout);
    //     }

    //     m_pITableFrame->BroadcastUserStatus(player, true);
    // }
    LOG(ERROR)<<"---------------------------OnUserLeft1-----------m_wGameStatus----------------"<<m_wGameStatus;

   // openLog("  ------------------------------user left ");
    return ret;
}

void CTableFrameSink::GameTimerReadyOver()
{
    //muduo::MutexLockGuard lock(m_mutexLock);

	assert(m_wGameStatus < GAME_STATUS_START);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);

    LOG(ERROR)<<"---------------------------GameTimerReadyOver---------------------------";
    //获取玩家个数
    int wUserCount = 0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
           // openLog("  wanjia zhuang tai  renshu=%d",user->GetUserStatus());
            if(user->GetUserStatus() == sOffline)
            {
               // openLog(" 1ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(ERROR)<<"---------------------------ClearTableUse1r---------------------------";
            }else if(user->GetUserScore()< m_pGameRoomInfo->enterMinScore )//m_pITableFrame->GetUserEnterMinScore())
            {
               // openLog(" 2ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(ERROR)<<"---------------------------ClearTableUser2---------------------------";
            }else{
                ++wUserCount;
            }
        }
    }



    //用户状态
   // for(int i = 0; i < GAME_PLAYER; ++i)
   // {
   //      获取用户
   //     shared_ptr<CServerUserItem>pIServerUser = m_pITableFrame->GetTableUserItem(i);
   //    if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i] > 0*/)
   //     {
   //          ++wUserCount;
   //     }
   //     if(m_inPlayingStatus[i] > 0)
   //         ++wUserCount;
   // }
   // openLog("  yijing diaoyong m_pITableFrame->StartGame()  renshu=%d",wUserCount);
    if(wUserCount >= m_pGameRoomInfo->minPlayerNum)
    {
        //清理游戏数据
        ClearGameData();
        //初始化数据
        InitGameData();
        InitLoadini();
        ReadConfigInformation();

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
            }
        }
        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_PLAYING GameTimereEnterPlayingOver ---------------------------";
        m_wGameStatus = GAME_STATUS_LOCK_PLAYING;
        //m_TimerIdEnterPlaying = m_TimerLoopThread->getLoop()->runAfter(2.0, boost::bind(&CTableFrameSink::GameTimereEnterPlayingOver, this));
        GameTimereEnterPlayingOver();
    }else
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                user->SetUserStatus(sReady);
            }
            m_cbPlayStatus[i] = 0;
        }
        ClearAllTimer();
        m_wGameStatus = GAME_STATUS_INIT;
        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT---------------------------";
    }
}

void CTableFrameSink::GameTimereEnterPlayingOver()
{
    {
        //muduo::MutexLockGuard lock(m_mutexLock);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);


        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
                m_cbPlayStatus[i] = 1;
                m_cbPlayStatusRecord[i] = 1;
                ++m_wPlayCount;
            }
            else
            {
                m_cbPlayStatus[i] = 0;
                m_cbPlayStatusRecord[i]=0;
            }

        }
        if(m_wPlayCount < m_pGameRoomInfo->minPlayerNum)
        {
            for(int i = 0; i < GAME_PLAYER; ++i)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(user)
                {
                    LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                    user->SetUserStatus(sReady);
                }
                m_cbPlayStatus[i] = 0;
                m_cbPlayStatusRecord[i] = 0;
            }
            ClearAllTimer();
            m_wGameStatus = GAME_STATUS_INIT;
            LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT---------------------------";
            return;
        }
    }

    {
        LOG(ERROR)<<"---------------------------GameTimereEnterPlayingOver----m_pITableFrame->StartGame()-----------------------";

       // m_pITableFrame->StartGame();
        OnGameStart();
       // openLog("  yijing diaoyong m_pITableFrame->StartGame() ");
    }
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    LOG(ERROR)<<"---------------------------OnGameStart---------------------------";
    mGameIds = m_pITableFrame->GetNewRoundId();
      
    m_replay.gameinfoid = mGameIds;
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;

    LOG(INFO)<<"FromGameServerm_llStorageControl"<<"="<<m_llStorageControl;
    LOG(INFO)<<"FromGameServerm_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
    LOG(INFO)<<"FromGameServerm_llStorageHighLimit"<<"="<<m_llStorageHighLimit;
    if (m_testMakeType>0 && m_testSameTypeCount>0)
    {
        openLog("=== 游戏开始,%d人, 测试牌型:%d,局数:%d ------",MaxGamePlayers_,m_testMakeType,m_testSameTypeCount);
    }
    else
    {
        openLog("=== 游戏开始 %d,mGameRoundId:[%s],lEndStorage:[%ld],lLowlimit:[%ld],lUplimit:[%ld];",MaxGamePlayers_,mGameIds.c_str(),m_llStorageControl,m_llStorageLowerLimit,m_llStorageHighLimit);
    }  
    if(ENABLE_TEST)
    {
        testCase->clearHaveCard();
        testCase->setIsHaveking(m_bHaveKing);
        testCase->setCardCount(m_bPlaySixChooseFive ? 6 : 5);
    }

    assert(NULL != m_pITableFrame);

    //清除所有定时器
    ClearAllTimer();
    m_dwReadyTime = (uint32_t)time(NULL);

    //发送开始消息
    NN_CMD_S_GameStart GameStart;
    GameStart.set_cbcallbankertime(TIME_CALL_BANKER);
    GameStart.set_roundid(mGameIds);

    //随机扑克发牌
    //每随机10次换一次随机种子
   // m_nLottery += 1;
   // m_nLottery = m_nLottery % 10;
    //同一房间多个同一秒开始游戏牌是一样的
   // if (0 == m_nLottery)
   // {
   //     m_GameLogic.GenerateSeed(m_pITableFrame->GetTableId());
   // }
    int playersnum = 0;
    uint8_t cbTempArray[MAX_CARD_TOTAL] = { 0 };
    m_GameLogic.RandCardData(cbTempArray, sizeof(cbTempArray));
    memset(m_cbCardData, 0, sizeof(m_cbCardData));

    //带大小王
    uint8_t cbTempArrayKing[MAX_CARD_TOTAL_KING] = { 0 };
    m_GameLogic.RandCardData(cbTempArrayKing, sizeof(cbTempArrayKing));

    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    uint8_t cbMaxCardValue[MAX_COUNT] = { 0 };
    bool bChooseFiveSmallNiu = false;
    
    uint8_t cbHandCardData[MAX_COUNT_SIX] = {0};
    uint8_t cbCardData[MAX_COUNT] = {0};
    //bool bTestMakeTwoking = true;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        GameStart.add_cbplaystatus(m_cbPlayStatus[i]);

        if (0 == m_cbPlayStatus[i])
            continue;
        m_cbLeftCardCount -= (m_bPlaySixChooseFive ? MAX_COUNT_SIX : MAX_COUNT);
        ++playersnum;
        //手牌
        if (m_bPlaySixChooseFive) //6选5
        {
            if (m_bHaveKing) //带大小王
            {
                memcpy(&m_cbHandCardData[i], &cbTempArrayKing[m_cbLeftCardCount], sizeof(m_cbHandCardData[i]));
            }
            else
            {
                memcpy(&m_cbHandCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(m_cbHandCardData[i]));
            }            
        }
        else //5张
        {
            if (m_bHaveKing) //带大小王
            {
                memcpy(&m_cbCardData[i], &cbTempArrayKing[m_cbLeftCardCount], sizeof(m_cbCardData[i]));
            }
            else 
            {
                memcpy(&m_cbCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(m_cbCardData[i]));
            }            
        }
        
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        m_replay.addPlayer(user->GetUserId(),user->GetAccount(),user->GetUserScore(),i);
		if (!user->IsAndroidUser())
		{
			openLog("===黑名单信息:userid=%d,status=%d,weight=%d;", user->GetUserId(), user->GetBlacklistInfo().status, user->GetBlacklistInfo().weight);
		}
		if(user->GetBlacklistInfo().status == 1 && user->GetBlacklistInfo().weight >= rand()%1000 )
        {
            m_iBlacklistNum++;
            m_bBlacklist[i]=true;
        }else
        {
            m_bBlacklist[i]=false;
        }

        if(ENABLE_TEST && m_testMakeType>=11)
        {
            MakeCardTest(user,i);
        }
        TestCard(user,i);
        //获取玩家最优牌型组合
        GetMaxHandCard(cbMaxCardValue,i);        
        //bTestMakeTwoking = false;     
    }
    if (m_bTestMakeTwoking)
    {
        TestTwoKing();
    }
    memcpy(m_cbRepertoryCard, cbTempArray,sizeof(cbTempArray));

    CMD_S_GameStartAi  GameStartAi;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        if(!m_pITableFrame->GetTableUserItem(i))
            continue;

        if(!m_pITableFrame->GetTableUserItem(i)->IsAndroidUser())
            continue;

        int cardtype =m_sMaxCardType[i];//(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
        GameStartAi.set_cboxcarddata(cardtype);
        int isMax = IsMaxCard(i);
        GameStartAi.set_ismaxcard(isMax);
        std::string data = GameStartAi.SerializeAsString();
        m_pITableFrame->SendTableData(i, NN_SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
        if (isMax==1)
            openLog("=====玩家%d手牌最大,userMaxType:%d,%d倍",i,(int)m_sMaxCardType[i],m_iTypeMultiple[m_sMaxCardType[i]]);
    }

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i]){
            for (int j = 0; j < MAX_BANKER_MUL; ++j)
            {
                m_cbCallable[i][j] = 0;
                GameStart.add_cbcallbankermultiple(m_cbCallable[i][j]);
            }
            continue;
        }


        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i);
        int64_t score = userItem->GetUserScore();
        int64_t maxCallTimes = score/(m_pGameRoomInfo->floorScore * 8 * (playersnum-1));

        m_cbCallable[i][0] = m_cbBankerMultiple[0];
        GameStart.add_cbcallbankermultiple(m_cbBankerMultiple[0]);

        for (int j = 1; j < MAX_BANKER_MUL; ++j)
        {
            if(maxCallTimes >= m_cbBankerMultiple[j]){
                m_cbCallable[i][j] = m_cbBankerMultiple[j];
                GameStart.add_cbcallbankermultiple(m_cbBankerMultiple[j]);
            }else{
                m_cbCallable[i][j] = -m_cbBankerMultiple[j];
                GameStart.add_cbcallbankermultiple(-m_cbBankerMultiple[j]);
            }
        }
    }
    std::string data = GameStart.SerializeAsString();
    //发送数据
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
    //设置游戏状态
    m_wGameStatus = GAME_STATUS_CALL;//zhuan tai gai bian
    m_dwCallTime = (uint32_t)time(NULL);

    LOG(ERROR)<<"------------------m_wGameStatus = GAME_STATUS_CALL---------IsTrustee---------------------------";
    //设置托管
    IsTrustee();
}
void CTableFrameSink::TestCard(shared_ptr<CServerUserItem> user,int userIndex)
{
    if (0)
    {
        if (!user->IsAndroidUser() && m_bHaveKing) //百变牛玩法,待测没有王、1个王、2个王,对应5张手牌、6张手牌的判断情况,以及百变牛时的大小比较；
        {
            uint8_t testcard5[MAX_COUNT] = {0x15,0x4f,0x3a,0x35,0x05};
            uint8_t testcard6[MAX_COUNT_SIX] = { 0x15,0x4f,0x3a,0x35,0x05,0x28 };//{0x0d,0x4e,0x2c,0x1c,0x38,0x12};//{0x4e,0x0d,0x2c,0x1c,0x38,0x12};//{0x16,0x38,0x02,0x0b,0x4e,0x4f};
            if (m_bPlaySixChooseFive)
            {
                memcpy(m_cbHandCardData[userIndex],testcard6,sizeof(testcard6));
            }
            else
            {
                memcpy(m_cbCardData[userIndex],testcard5,sizeof(testcard5));
            }
        }
        else
        {
            uint8_t testcard5[GAME_PLAYER][MAX_COUNT] = {{0x27,0x07,0x1b,0x4e,0x26},
                                                         {0x2c,0x0c,0x1a,0x0b,0x0a},
                                                         {0x12,0x4e,0x38,0x32,0x08},
                                                         {0x33,0x23,0x03,0x13,0x2d}};
            uint8_t testcard6[GAME_PLAYER][MAX_COUNT_SIX] = {
                                                             // {0x16,0x28,0x22,0x2b,0x14,0x0d},
                                                             // {0x06,0x08,0x12,0x3a,0x19,0x1d},
                                                             // {0x25,0x39,0x34,0x2c,0x15,0x2d},
                                                             // {0x16,0x38,0x02,0x0b,0x4e,0x4f},
                                                             // {0x3b,0x2b,0x1b,0x0b,0x4e,0x4f},
                                                             // {0x3a,0x2b,0x1c,0x0d,0x4e,0x4f},
                                                             // {0x0b,0x07,0x4f,0x26,0x34,0x04},
                                                             // {0x27,0x18,0x02,0x1b,0x3d,0x3b}
                                                             // {0x4f,0x37,0x4e,0x27,0x1a,0x0b}
                                                             {0x27,0x07,0x16,0x4e,0x26,0x3d},//{0x01,0x33,0x2d,0x17,0x32,0x04}
                                                             {0x2c,0x0c,0x1a,0x0b,0x0a,0x31},
                                                             {0x12,0x4f,0x32,0x32,0x02,0x13},
                                                             {0x33,0x23,0x03,0x13,0x2d,0x38} // 0x22,0x19,0x2d,0x2c,0x0c,0x28
                                                            };
            if (m_bPlaySixChooseFive)
            {
                memcpy(m_cbHandCardData[userIndex],testcard6[userIndex],sizeof(testcard6[userIndex]));
            }
            else
            {
                memcpy(m_cbCardData[userIndex],testcard5[userIndex],sizeof(testcard5[userIndex]));
            }
        }
    }    
}
//获取玩家最优牌型组合
void CTableFrameSink::GetMaxHandCard(uint8_t cbMaxCardValue[MAX_COUNT],int chairid)
{
    memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
    uint8_t maxType = 0;
    uint8_t kingChangCard[2] = {0};
    if (m_bHaveKing) //百变牛玩法
    {
        if (m_bPlaySixChooseFive)
        { 
            maxType = m_GameLogic.GetHaveKingBigCardType(m_cbHandCardData[chairid],cbMaxCardValue,6,kingChangCard);
            openLog("玩家%d手牌,userMaxType:%d,%d倍,cbHandCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",chairid,(int)maxType,m_iTypeMultiple[maxType],m_cbHandCardData[chairid][0],m_cbHandCardData[chairid][1],m_cbHandCardData[chairid][2],m_cbHandCardData[chairid][3],m_cbHandCardData[chairid][4],m_cbHandCardData[chairid][5]);
            memcpy(&m_cbCardData[chairid], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
        }
        else
        {
            maxType = m_GameLogic.GetHaveKingBigCardType(m_cbCardData[chairid],cbMaxCardValue,5,kingChangCard);
        }
        memcpy(m_cbKingChangCard[chairid],kingChangCard,sizeof(kingChangCard));
        openLog("玩家%d选牌,userMaxType:%d,%d倍,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,王变牌值:0x%02x,0x%02x;",chairid,(int)maxType,m_iTypeMultiple[maxType],m_cbCardData[chairid][0],m_cbCardData[chairid][1],m_cbCardData[chairid][2],m_cbCardData[chairid][3],m_cbCardData[chairid][4],m_cbKingChangCard[chairid][0],m_cbKingChangCard[chairid][1]);
    }
    else
    {
        if (m_bPlaySixChooseFive)
        {            
            maxType = m_GameLogic.GetBigTypeCard(m_cbHandCardData[chairid], cbMaxCardValue);
            memcpy(&m_cbCardData[chairid], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
            openLog("玩家%d手牌,userMaxType:%d,%d倍,cbHandCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",chairid,(int)maxType,m_iTypeMultiple[maxType],m_cbHandCardData[chairid][0],m_cbHandCardData[chairid][1],m_cbHandCardData[chairid][2],m_cbHandCardData[chairid][3],m_cbHandCardData[chairid][4],m_cbHandCardData[chairid][5]);
        } 
        else
        {
            maxType = m_GameLogic.GetCardType(m_cbCardData[chairid], MAX_COUNT);
        } 
        openLog("玩家%d选牌,userMaxType:%d,%d倍,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",chairid,(int)maxType,m_iTypeMultiple[maxType],m_cbCardData[chairid][0],m_cbCardData[chairid][1],m_cbCardData[chairid][2],m_cbCardData[chairid][3],m_cbCardData[chairid][4]);
    }  
    // // 记录牌型
    m_sMaxCardType[chairid] = maxType; 
}
void CTableFrameSink::MakeCardTest(shared_ptr<CServerUserItem> user,int userIndex)
{
    srand((int)time(NULL));
    uint8_t chooseFiveSmallNiu = 0;
    uint8_t chooseWuHuaNiu = 0;
    uint8_t cbHandCardData[MAX_COUNT_SIX] = {0};
    uint8_t cbCardData[MAX_COUNT] = {0};
    uint8_t TestType = m_testMakeType; //连续测试相同的牌型
    if (m_testSameTypeCount==0) //随机测试特殊的牌型
    {
        TestType = STD::Random(11,m_testMakeType).randInt_mt();
    }
    memset(cbHandCardData,0,sizeof(cbHandCardData));
    memset(cbCardData,0,sizeof(cbCardData));           
    if (chooseFiveSmallNiu>=1)
    {
        TestType = rand()%5+10;
    }
    if (chooseWuHuaNiu>=2)
    {
        do
        {
            TestType = rand()%5+10;
        }while(TestType==12);
    }
    
    openLog("==玩家%d制造牌型%d;",userIndex,TestType);
    bool bAndroid = user->IsAndroidUser();
    if (m_bPlaySixChooseFive)
    {
		if (testCase->enabled)
		{
			if (testCase->Check(user->GetAccount(), cbHandCardData, 6, TestType))
			{
				if (m_GameLogic.CheckHanCardIsOK(cbHandCardData, 6))
				{
					if (!bAndroid)
						memcpy(m_cbHandCardData[userIndex], cbHandCardData, sizeof(cbHandCardData));
					else if (testCase->android)
						memcpy(m_cbHandCardData[userIndex], cbHandCardData, sizeof(cbHandCardData));
				}
				else
				{
					openLog("==玩家%d制造牌型%d,66错误！！！！;", userIndex, TestType);
				}
			}
		}
        if (testCase->android && bAndroid)
		{
			testCase->CheckAdr("android_" + to_string(userIndex), m_cbHandCardData[userIndex], 6);
		}
    }
    else
    {   
		if (testCase->enabled)
		{
			if (testCase->Check(user->GetAccount(), cbCardData, 5, TestType))
			{				
				if (m_GameLogic.CheckHanCardIsOK(cbCardData, 5))
				{
					if (!bAndroid)
						memcpy(m_cbCardData[userIndex], cbCardData, sizeof(cbCardData));
					else if (testCase->android)
						memcpy(m_cbCardData[userIndex], cbCardData, sizeof(cbCardData));
				}
				else
				{
					openLog("==玩家%d制造牌型%d,55错误！！！！;", userIndex, TestType);
				}
			}
		}
        if (testCase->android && bAndroid)
		{
			testCase->CheckAdr("android_"+to_string(userIndex), m_cbCardData[userIndex], 5);
		}
    }
    if (TestType==CT_FIVESMALL_NIU)
    {
        chooseFiveSmallNiu++;
    }
    if (TestType==CT_WUHUA_NIU)
    {
        chooseWuHuaNiu++;
    }
}
void CTableFrameSink::TestTwoKing()
{
    if (m_bHaveKing) //百变牛玩法
    {
        uint8_t cbMaxCardValue[MAX_COUNT] = {0};
        memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
        uint8_t maxType = 0;
        uint8_t kingChangCard[2] = {0};
        uint8_t bChooseKing[2] = {0};
        uint8_t userIndex = 0;
        if (m_bPlaySixChooseFive)
        {
            for (int i = 0; i < GAME_PLAYER; ++i)
            {
                for (int j = 0; j < MAX_COUNT_SIX; ++j)
                {
                    if (m_cbHandCardData[i][j]==0x4e)
                    {
                        bChooseKing[0] = 1;
                    }
                    if (m_cbHandCardData[i][j]==0x4f)
                    {
                        bChooseKing[1] = 1;
                    }
                }
                if (bChooseKing[0]!=0 && bChooseKing[1]!=0) //大小王已选
                {
                    break;
                }
            }
            if (bChooseKing[0]==0) //没选到小王
            {
                do
                {
                    userIndex = rand()%GAME_PLAYER;
                }while(0 == m_cbPlayStatus[userIndex]);

                if (bChooseKing[1]==0) // 没选到大王
                {
                    m_cbHandCardData[userIndex][rand()%6] = 0x4e;
                    bChooseKing[0]=1;
                }
                else
                {
                    bool bHaveBigking = false;
                    for (int i = 0; i < MAX_COUNT_SIX; ++i)
                    {
                        if (m_cbHandCardData[userIndex][i] == 0x4f)
                        {
                            bHaveBigking = true;
                            break;
                        }
                    }
                    if (bHaveBigking)
                    {
                        for (int i = 0; i < MAX_COUNT_SIX; ++i)
                        {
                            if (m_cbHandCardData[userIndex][i] != 0x4f)
                            {
                                m_cbHandCardData[userIndex][i] = 0x4e;
                                bChooseKing[0]=1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        m_cbHandCardData[userIndex][rand()%6] = 0x4e;
                        bChooseKing[0]=1;
                    }
                }
            }
            if (bChooseKing[1]==0) //没选到大王
            {
                do
                {
                    userIndex = rand()%GAME_PLAYER;
                }while(0 == m_cbPlayStatus[userIndex]);

                bool bHaveSmallking = false;
                for (int i = 0; i < MAX_COUNT_SIX; ++i)
                {
                    if (m_cbHandCardData[userIndex][i] == 0x4e)
                    {
                        bHaveSmallking = true;
                        break;
                    }
                }
                if (bHaveSmallking)
                {
                    for (int i = 0; i < MAX_COUNT_SIX; ++i)
                    {
                        if (m_cbHandCardData[userIndex][i] != 0x4e)
                        {
                            m_cbHandCardData[userIndex][i] = 0x4f;
                            bChooseKing[1]=1;
                            break;
                        }
                    }
                }
                else
                {
                    m_cbHandCardData[userIndex][rand()%6] = 0x4f;
                    bChooseKing[1]=1;
                }
            }            
            for (int i = 0; i < GAME_PLAYER; ++i)
            {
                if (0 == m_cbPlayStatus[i])
                    continue;
                //获取玩家最优牌型组合
                memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
                memset(kingChangCard,0,sizeof(kingChangCard));

                maxType = m_GameLogic.GetHaveKingBigCardType(m_cbHandCardData[i],cbMaxCardValue,6,kingChangCard);
                openLog("玩家%d手牌,userMaxType:%d,%d倍,cbHandCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",i,(int)maxType,m_iTypeMultiple[maxType],m_cbHandCardData[i][0],m_cbHandCardData[i][1],m_cbHandCardData[i][2],m_cbHandCardData[i][3],m_cbHandCardData[i][4],m_cbHandCardData[i][5]);
                memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
                memcpy(m_cbKingChangCard[i],kingChangCard,sizeof(kingChangCard));
                openLog("玩家%d选牌,userMaxType:%d,%d倍,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,王变牌值:0x%02x,0x%02x;",i,(int)maxType,m_iTypeMultiple[maxType],m_cbCardData[i][0],m_cbCardData[i][1],m_cbCardData[i][2],m_cbCardData[i][3],m_cbCardData[i][4],m_cbKingChangCard[i][0],m_cbKingChangCard[i][1]);
                //记录牌型
                m_sMaxCardType[i] = maxType; 
            }
        }
        else
        {
            for (int i = 0; i < GAME_PLAYER; ++i)
            {
                for (int j = 0; j < MAX_COUNT; ++j)
                {
                    if (m_cbCardData[i][j]==0x4e)
                    {
                        bChooseKing[0] = 1;
                    }
                    if (m_cbCardData[i][j]==0x4f)
                    {
                        bChooseKing[1] = 1;
                    }
                }
                if (bChooseKing[0]!=0 && bChooseKing[1]!=0) //大小王已选
                {
                    break;
                }
            }
            if (bChooseKing[0]==0) //没选到小王
            {
                do
                {
                    userIndex = rand()%GAME_PLAYER;
                }while(0 == m_cbPlayStatus[userIndex]);

                if (bChooseKing[1]==0) // 没选到大王
                {
                    m_cbCardData[userIndex][rand()%5] = 0x4e;
                    bChooseKing[0]=1;
                }
                else
                {
                    bool bHaveBigking = false;
                    for (int i = 0; i < MAX_COUNT; ++i)
                    {
                        if (m_cbCardData[userIndex][i] == 0x4f)
                        {
                            bHaveBigking = true;
                            break;
                        }
                    }
                    if (bHaveBigking)
                    {
                        for (int i = 0; i < MAX_COUNT; ++i)
                        {
                            if (m_cbCardData[userIndex][i] != 0x4f)
                            {
                                m_cbCardData[userIndex][i] = 0x4e;
                                bChooseKing[0]=1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        m_cbCardData[userIndex][rand()%5] = 0x4e;
                        bChooseKing[0]=1;
                    }
                }
            }
            if (bChooseKing[1]==0) //没选到大王
            {
                do
                {
                    userIndex = rand()%GAME_PLAYER;
                }while(0 == m_cbPlayStatus[userIndex]);

                bool bHaveSmallking = false;
                for (int i = 0; i < MAX_COUNT; ++i)
                {
                    if (m_cbCardData[userIndex][i] == 0x4e)
                    {
                        bHaveSmallking = true;
                        break;
                    }
                }
                if (bHaveSmallking)
                {
                    for (int i = 0; i < MAX_COUNT; ++i)
                    {
                        if (m_cbCardData[userIndex][i] != 0x4e)
                        {
                            m_cbCardData[userIndex][i] = 0x4f;
                            bChooseKing[1]=1;
                            break;
                        }
                    }
                }
                else
                {
                    m_cbCardData[userIndex][rand()%5] = 0x4f;
                    bChooseKing[1]=1;
                }
            }
            for (int i = 0; i < GAME_PLAYER; ++i)
            {
                if (0 == m_cbPlayStatus[i])
                    continue;
                //获取玩家最优牌型组合
                memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
                memset(kingChangCard,0,sizeof(kingChangCard));
                maxType = m_GameLogic.GetHaveKingBigCardType(m_cbCardData[i],cbMaxCardValue,5,kingChangCard);
            
                memcpy(m_cbKingChangCard[i],kingChangCard,sizeof(kingChangCard));
                openLog("玩家%d选牌,userMaxType:%d,%d倍,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,王变牌值:0x%02x,0x%02x;",i,(int)maxType,m_iTypeMultiple[maxType],m_cbCardData[i][0],m_cbCardData[i][1],m_cbCardData[i][2],m_cbCardData[i][3],m_cbCardData[i][4],m_cbKingChangCard[i][0],m_cbKingChangCard[i][1]);
                //记录牌型
                m_sMaxCardType[i] = maxType; 
            }
        }        
    }
}
void CTableFrameSink::GameTimerCallBanker()
{
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    m_dwCallTime = 0;

    //代替叫庄
    AutoUserCallBanker();
}

int CTableFrameSink::AutoUserCallBanker()
{
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if (m_sCallBanker[i] != -1)
        {
            continue;
        }

        OnUserCallBanker(i, 0);
    }
    return 0;
}

//玩家叫庄
int CTableFrameSink::OnUserCallBanker(int wChairID, int wCallFlag)
{
    //已叫过庄
    if (-1 != m_sCallBanker[wChairID])
    {
        return 1;
    }

    //叫庄标志错误
    if (wCallFlag < 0)
    {
        return 2;
    }

    //检测是否正确的倍数
    bool bCorrectMultiple = false;
    if (wCallFlag != 0)
    {
        for (int cbIndex = 0; cbIndex != MAX_BANKER_MUL; ++cbIndex)
        {
            if (m_cbCallable[wChairID][cbIndex] == wCallFlag)//(m_cbBankerMultiple[cbIndex] == wCallFlag)
            {
                bCorrectMultiple = true;
                break;
            }
        }

        if (bCorrectMultiple == false)
        {
            return 3;
        }
    }

    //记录叫庄
    m_sCallBanker[wChairID] = wCallFlag;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wCallFlag),-1,opBet,wChairID,wChairID);
    //处理叫庄
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有叫庄玩家
        if (-1 == m_sCallBanker[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //叫庄
    NN_CMD_S_CallBanker callBanker;
    callBanker.set_wcallbankeruser( wChairID);
    callBanker.set_cbcallmultiple(wCallFlag);
    std::string data = callBanker.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());

    //记录叫庄玩家
    int wBankerUser = INVALID_CHAIR;

     NN_CMD_S_CallBankerResult CallResult;
     for (int j = 0; j < GAME_PLAYER; ++j)
     {
         CallResult.add_cbcallbankeruser(0);
     }

    CallResult.set_cbrandbanker(0);   //是否随机抢庄
    if(wTotalCount == m_wPlayCount)  //all callBanker
    {
        //计算叫庄玩家
        int wCallBankerUser[MAX_BANKER_CALL][GAME_PLAYER];
        memset(wCallBankerUser, 0, sizeof(wCallBankerUser));
        int wCallBankerCount[MAX_BANKER_CALL];
        memset(wCallBankerCount, 0, sizeof(wCallBankerCount));  //0 1 2 4 count

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            CallResult.set_cbcallbankeruser(i, m_sCallBanker[i]);
            if(m_cbPlayStatus[i] != 1)
                continue;

            if(m_sCallBanker[i] != 0)
            {
                uint8_t cbCallScore = (uint8_t)m_sCallBanker[i];  //0,1,2,4
                uint8_t cbCallCount = (uint8_t)wCallBankerCount[cbCallScore];  //0,1,2,4 count

                wCallBankerUser[cbCallScore][cbCallCount] = i;
                ++wCallBankerCount[cbCallScore];
            }
        }

        //确定庄家
        for(int i = (MAX_BANKER_CALL - 1); i >= 0; --i)
        {
            if(wCallBankerCount[i] > 0)
            {
                int wCount = wCallBankerCount[i];
                if (wCount == 1)
                {
                    wBankerUser = wCallBankerUser[i][0];

                }else
                {
                    CallResult.set_cbrandbanker(1);
                    //todo need to be optimus
                   // wBankerUser = wCallBankerUser[i][rand() % wCount];
                    int64_t curMaxScore = 0;
                    for(int j = 0; j < wCount; j++)
                    {
                        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(wCallBankerUser[i][j]);
                        if(userItem->GetUserScore() > curMaxScore)
                        {
                            curMaxScore = userItem->GetUserScore();
                            wBankerUser = wCallBankerUser[i][j];
                        }
                    }
                }

                break;
            }
        }


        //没有人叫地主
        if (wBankerUser == INVALID_CHAIR)
        {
            wBankerUser = RandBanker();
           // for (int i = 0; i < GAME_PLAYER; ++i)
           // {
           //     if (m_cbPlayStatus[i] == 0)
           //         continue;

           //     CallResult.set_cbcallbankeruser(i, 1);
           // }
            CallResult.set_cbrandbanker(1);   //是否随机抢庄
            //如果人叫庄则随机到的庄家默认叫1倍
            m_sCallBanker[wBankerUser] = m_cbBankerMultiple[0];
        }

        m_wBankerUser = wBankerUser;
        //设置游戏状态
        m_wGameStatus = GAME_STATUS_SCORE;
        m_dwScoreTime = (uint32_t)time(NULL);
        //设置托管
        IsTrustee();
    }

    //发送叫庄结果
    if (wBankerUser != INVALID_CHAIR)
    {
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wBankerUser),-1,opBanker,-1,-1);
        CallResult.set_dwbankeruser(wBankerUser);
        shared_ptr<CServerUserItem> bankerItem = m_pITableFrame->GetTableUserItem(wBankerUser);
        int64_t maxBankerCallTimes = bankerItem->GetUserScore()/(m_pGameRoomInfo->floorScore * (wTotalCount - 1) * m_sCallBanker[wBankerUser] );
        for(int j = 0; j < GAME_PLAYER; j++ )
        {
            if( m_cbPlayStatus[j] == 1 && j != wBankerUser)
            {
                shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(j);
                int64_t maxPlayerCallTimes = userItem->GetUserScore()/(m_pGameRoomInfo->floorScore*m_sCallBanker[wBankerUser]);
                int64_t callMaxTimes = maxPlayerCallTimes > maxBankerCallTimes ? maxBankerCallTimes : maxPlayerCallTimes;
                // 产品确认需要修改为默认下注倍数,不能选择的就置灰
                uint8_t jsize = m_pGameRoomInfo->jettons.size();
                if (jsize>MAX_JETTON_MUL)
                {
                    jsize = MAX_JETTON_MUL;
                }
                if( callMaxTimes >= m_cbJettonMultiple[jsize-1]){
                    for (int i = 0; i < MAX_JETTON_MUL; ++i)
                    {
                        m_cbJettonable[j][i]=m_cbJettonMultiple[i];
                        CallResult.add_cbjettonmultiple(m_cbJettonMultiple[i]);
                    }
                }
                else
                {
                    for (int i = 0; i < MAX_JETTON_MUL; ++i)
                    {
                        if(callMaxTimes >= m_cbJettonMultiple[i]){
                            m_cbJettonable[j][i]=m_cbJettonMultiple[i];
                            CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                        }else{
                            m_cbJettonable[j][i]=0;//-m_cbJettonMultiple[i]; //不能选择的就置灰
                            CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                        }
                    }
                }
                // else if(callMaxTimes >= 5){
                //     for (int i = 0; i < MAX_JETTON_MUL; ++i)
                //     {
                //         m_cbJettonable[j][i]=callMaxTimes*(i+1)/5;
                //         CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                //     }
                // }else{
                //     for (int i = 1; i <= MAX_JETTON_MUL; ++i)
                //     {
                //         if(i <= callMaxTimes)
                //         {
                //             m_cbJettonable[j][i-1]=i;
                //         }else{
                //             m_cbJettonable[j][i-1]=0;
                //         }
                //         CallResult.add_cbjettonmultiple(m_cbJettonable[j][i-1]);
                //     }
                // }
            }else{
                for (int i = 0; i < MAX_JETTON_MUL; ++i)
                {
                    m_cbJettonable[j][i] = 0;
                    CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                }
            }
            CallResult.set_cbaddjettontime(TIME_ADD_SCORE);			//加注时间
            //LOG(WARNING) << "random banker ?" << (int)CallResult.cbRandBanker;
        }
        std::string data = CallResult.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_CALL_BANKER_RESULT, (uint8_t*)data.c_str(), data.size());
    }
    return 4;
}

//随机庄家
int CTableFrameSink::RandBanker()
{
    int wTempBanker = 0;//rand() % m_wPlayCount;
    int64_t maxUserScore = 0;
    for (uint8_t i = 0; i < GAME_PLAYER; ++i)
    {
        if (1 == m_cbPlayStatus[i])
        {
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i);
            if(userItem->GetUserScore() > maxUserScore){
                maxUserScore = userItem->GetUserScore();
                wTempBanker = i;
            }
        }
    }

    return wTempBanker;
}


void CTableFrameSink::GameTimerAddScore()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_dwScoreTime = 0;
    //代替下注
    AutoUserAddScore();
}

int CTableFrameSink::AutoUserAddScore()
{
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if (m_sAddScore[i] != -1)
        {
            continue;
        }

        OnUserAddScore(i, 0);
    }
    return 0;
}

//玩家下注
int CTableFrameSink::OnUserAddScore(int wChairID, int wJettonIndex)
{
    //庄家不能下注
    if (m_wBankerUser == wChairID)
    {
        return 1;
    }

    //已下过注
    if (-1 != m_sAddScore[wChairID])
    {
        return 2;
    }

    //下注筹码错误
    if (wJettonIndex < 0 || wJettonIndex >= MAX_JETTON_MUL)  //1 5 8 11 15
    {
        return 3;
    }
    if(m_cbJettonable[wChairID][wJettonIndex] <= 0){
        return 4;
    }

    //记录下注
    m_sAddScore[wChairID] = wJettonIndex;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(m_cbJettonable[wChairID][wJettonIndex]),-1,opAddBet,wChairID,wChairID);
    NN_CMD_S_AddScoreResult CallResult;
    CallResult.set_waddjettonuser(wChairID);
    CallResult.set_cbjettonmultiple(m_cbJettonable[wChairID][wJettonIndex]);//m_cbJettonMultiple[wJettonIndex]);

    //处理下注
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤庄家
        if (i == m_wBankerUser)
            continue;

        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有下注玩家
        if (-1 == m_sAddScore[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //是否发牌
    uint8_t cbIsSendCard = 0;
    int dwOpenTime = 0;
    if ((wTotalCount + 1) == m_wPlayCount)
    {
        //所有人都下注了
        //设置游戏状态
        m_wGameStatus = GAME_STATUS_OPEN;
        LOG(INFO)<<"m_wGameStatus = GAME_STATUS_OPEN";

        m_dwOpenTime = (uint32_t)time(NULL);
        //设置发牌标志
        cbIsSendCard = 1;
        dwOpenTime = TIME_OPEN_CARD;
        if (m_bPlaySixChooseFive)
        {
            dwOpenTime = TIME_OPEN_CARD_SIX;
        }
        // if(!ENABLE_TEST)
        {
            if(m_iBlacklistNum == 0 )//|| m_pGameRoomInfo->matchforbids[MTH_BLACKLIST])
            {
                //分析玩家牌
                AnalyseCardEx();
            }else
            {
                AnalyseForBlacklist();
            }
        }

        //设置托管
        IsTrustee();
    }


    //发送下注结果
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //if (0 == m_cbPlayStatus[i]) continue;
        std::string data = CallResult.SerializeAsString();
        m_pITableFrame->SendTableData(i, NN_SUB_S_ADD_SCORE_RESULT, (uint8_t*)data.c_str(), data.size());

        if (1 == cbIsSendCard)
        {
            //有发牌
            NN_CMD_S_SendCard sendCard;

            for(int j = 0; j < MAX_COUNT; ++j)
            {
                sendCard.add_cbsendcard(0);
                sendCard.add_cboxcard(0);
            }
            if (m_bPlaySixChooseFive) //第6张牌
                sendCard.add_cbsendcard(0);
            sendCard.set_cbcardtype(0);
            sendCard.set_cbopentime(dwOpenTime);
            int cbCardType = 0;
            if(m_cbPlayStatus[i] != 0 )
            {
                uint8_t cbOXCard[MAX_COUNT] = { 0 };
                uint8_t cbMaxCardValue[MAX_COUNT] = {0};
                uint8_t kingChangCard[2] = {0};
                if (m_bPlaySixChooseFive)
                {
                    //获得牛牛牌
                    for (int j = 0; j < MAX_COUNT; ++j)
                    {
                        cbOXCard[j] = m_cbHandCardData[i][j];
                    }
                    if (m_bHaveKing)
                    {
                        cbCardType = m_GameLogic.GetHaveKingBigCardType(cbOXCard,cbMaxCardValue,5,kingChangCard);
                        m_GameLogic.GetOxCard(cbOXCard,kingChangCard);
                    }
                    else
                    {
                        m_GameLogic.GetOxCard(cbOXCard,kingChangCard);
                        cbCardType = (int)m_GameLogic.GetCardType(cbOXCard, MAX_COUNT);
                    }
                    
                    for(int j = 0; j < MAX_COUNT_SIX; ++j)
                    {
                        sendCard.set_cbsendcard(j, m_cbHandCardData[i][j]);
                        if (j<MAX_COUNT)
                            sendCard.set_cboxcard(j, cbOXCard[j]);
                    }
                }
                else
                {
                    cbCardType = m_sMaxCardType[i]; //(int)m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    //提牌数据                
                    memcpy(cbOXCard, &m_cbCardData[i], sizeof(uint8_t)*MAX_COUNT);
                    m_GameLogic.GetOxCard(cbOXCard,m_cbKingChangCard[i]);
                    for(int j = 0; j < MAX_COUNT; ++j)
                    {
                        sendCard.set_cbsendcard(j, m_cbCardData[i][j]);
                        sendCard.set_cboxcard(j, cbOXCard[j]);
                    }
                }
                sendCard.set_cbcardtype(cbCardType);
            }
            sendCard.set_typemultiple(m_iTypeMultiple[cbCardType]); //牌型对应的倍数

            std::string data = sendCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, NN_SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());
        }
    }
    if (1 == cbIsSendCard)
    {
        openLog("=====玩家准备开牌=====");
    }
    return 4;
}

void CTableFrameSink::AnalyseForBlacklist()
{
    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //临时变量
    uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));

    //王临时变量
    uint8_t cbTmpkingData[GAME_PLAYER][2];
    memcpy(cbTmpkingData, m_cbKingChangCard, sizeof(cbTmpkingData));

    //6张手牌
    uint8_t cbTmpHandCardData[GAME_PLAYER][MAX_COUNT_SIX];
    memcpy(cbTmpHandCardData, m_cbHandCardData, sizeof(cbTmpHandCardData));

    //玩家牌型
    int cbtempCardtype[GAME_PLAYER] = {0};
    memcpy(cbtempCardtype, m_sMaxCardType, sizeof(m_sMaxCardType));

    int androidPlayerNum = m_pITableFrame->GetAndroidPlayerCount();
   // int realPlayerNum = m_wPlayCount - androidPlayerNum;
    int strategy = 1;
    int winnerSeat = INVALID_CHAIR;
    int winnerWeight = 10000;
    if(androidPlayerNum == 0)//没有机器人找一个通杀率较低的玩家当赢家
    {
        strategy = 1;
    }else//机器人和玩家混用的时候找个机器人玩家赢
    {
        strategy = 2;
    }
    shared_ptr<CServerUserItem> tempUser;
    std::vector<int> vecCardVal;
    for (int j = 0; j < m_wPlayCount; j++)
    {
        //变量定义
        int wWinUser = INVALID_CHAIR;

        //查找数据
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //用户过滤
            if (m_cbPlayStatus[i] == 0)
                continue;

            bool bExit = false;
            std::vector<int>::iterator  it = vecCardVal.begin();
            for (; it != vecCardVal.end(); ++it)
            {
                if (*it == i)
                {
                    bExit = true;
                    break;
                }
            }
            if(bExit)
            {
                continue;
            }

            //设置用户
            if (wWinUser == INVALID_CHAIR)
            {
                wWinUser = i;
                continue;
            }

            //对比扑克
            if (m_GameLogic.CompareCard(cbUserCardData[i], m_cbCardData[wWinUser],m_cbKingChangCard[i],m_cbKingChangCard[wWinUser],false,i==m_wBankerUser))
            {
                wWinUser = i;
            }
        }
        vecCardVal.push_back(wWinUser);
        if (m_cbPlayStatus[j] == 0)
            continue;
        if(strategy == 1)
        {
            if(!m_pITableFrame->IsAndroidUser(j))
            {
                tempUser = m_pITableFrame->GetTableUserItem(j);
                if(!m_bBlacklist[j])//& tempUser->GetBlacklistInfo().status == 0)
                {
                    if(winnerWeight == 0)
                    {
                        if(rand()%100 > 50)
                        {
                            winnerSeat = j;
                        }
                    }else
                    {
                        winnerWeight = 0;
                        winnerSeat = j;
                    }
                }else if( tempUser->GetBlacklistInfo().weight < winnerWeight )
                {
                    winnerSeat = j;
                    winnerWeight = tempUser->GetBlacklistInfo().weight;
                }
            }
        }else{
            if(m_pITableFrame->IsAndroidUser(j))
            {
                if(winnerSeat == INVALID_CHAIR || rand()%100 > 50)
                    winnerSeat = j;
            }
        }
    }    
    int cbTmpValue = 0;

   // int i = rand()%GAME_PLAYER;// i need to be random?
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_pITableFrame->IsAndroidUser(i) || winnerSeat == i)
        {
            cbTmpValue = vecCardVal.front();
            vecCardVal.erase(vecCardVal.begin());
        }else
        {
            cbTmpValue = vecCardVal.back();
            vecCardVal.pop_back();
        }

        if (cbTmpValue != i)
        {
            openLog("AnalyseForBlacklist:换牌:%d<<==>>%d;",i,cbTmpValue);
            memcpy(m_cbCardData[i], cbTmpCardData[cbTmpValue], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbKingChangCard[i], cbTmpkingData[cbTmpValue], sizeof(cbTmpkingData[cbTmpValue]));
            memcpy(m_cbHandCardData[i], cbTmpHandCardData[cbTmpValue], sizeof(uint8_t)*MAX_COUNT_SIX);
            m_sMaxCardType[i] = cbtempCardtype[cbTmpValue];
        }
    }
}

void CTableFrameSink::AnalyseCardEx()
{
    if (m_pITableFrame->GetAndroidPlayerCount() == 0)
    {
        return;
    }

    int64_t llLowerLimitVal = m_llStorageLowerLimit;		//吸分值 下限值
    int64_t llHighLimitVal = m_llStorageHighLimit;		//吐分值 上限值

    //////////////////////////////////////////////////////////////////////////
    bool bSteal = true;
    LOG(INFO) << "-------------------m_llStorageControl = " << (int)m_llStorageControl;
    LOG(INFO) << "-------------------m_llStorageminis = " << (int)llLowerLimitVal;
    LOG(INFO) << "-------------------m_llStoragemax = " << (int)llHighLimitVal;
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度
    if(randomRet<m_difficulty)
    {
        bSteal = true;
    }
    else
    {
        if (m_llStorageControl >= llLowerLimitVal && m_llStorageControl <= llHighLimitVal)
        {
            return;
        }
        else if (m_llStorageControl > llHighLimitVal )
        {
            bSteal = false;
        }
        else if (m_llStorageControl < llLowerLimitVal )
        {
            bSteal = true;
        }

        if (m_llStorageControl > llHighLimitVal && rand() % 100 <= 30)
            return ;
        else if (m_llStorageControl < llLowerLimitVal && rand() % 100 <= 20)
            return ;
    }
    if(bSteal)
    {
        LOG(ERROR) << "suan fa 吸分";
    }else
    {
        LOG(ERROR) << "suan fa 吐分";
    }
    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //临时变量
    uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));

    //王临时变量
    uint8_t cbTmpkingData[GAME_PLAYER][2];
    memcpy(cbTmpkingData, m_cbKingChangCard, sizeof(cbTmpkingData));

    //6张手牌
    uint8_t cbTmpHandCardData[GAME_PLAYER][MAX_COUNT_SIX];
    memcpy(cbTmpHandCardData, m_cbHandCardData, sizeof(cbTmpHandCardData));

    //玩家牌型
    int cbtempCardtype[GAME_PLAYER] = {0};
    memcpy(cbtempCardtype, m_sMaxCardType, sizeof(m_sMaxCardType));

    std::vector<int> vecCardVal;
    for (int j = 0; j < m_wPlayCount; j++)
    {
        //变量定义
        int wWinUser = INVALID_CHAIR;

        //查找数据
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //用户过滤
            if (m_cbPlayStatus[i] == 0)
                continue;

            bool bExit = false;
            std::vector<int>::iterator  it = vecCardVal.begin();
            for (; it != vecCardVal.end(); ++it)
            {
                if (*it == i)
                {
                    bExit = true;
                    break;
                }
            }
            if(bExit)
            {
                continue;
            }

            //设置用户
            if (wWinUser == INVALID_CHAIR)
            {
                wWinUser = i;
                continue;
            }

            //对比扑克
            if (m_GameLogic.CompareCard(cbUserCardData[i], m_cbCardData[wWinUser],m_cbKingChangCard[i],m_cbKingChangCard[wWinUser],false,i==m_wBankerUser) == bSteal)
            {
                wWinUser = i;
            }
        }
        vecCardVal.push_back(wWinUser);
    }

    int cbTmpValue = 0;

    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_pITableFrame->IsAndroidUser(i))
        {
            cbTmpValue = vecCardVal.front();
            vecCardVal.erase(vecCardVal.begin());
        }else
        {
            cbTmpValue = vecCardVal.back();
            vecCardVal.pop_back();
        }

        if (cbTmpValue != i)
        {
            // LOG(WARNING) << "换牌 " << m_pITableFrame->GetEscortID(cbTmpValue) << " -> " << m_pITableFrame->GetEscortID(i);
            openLog("AnalyseCardEx:换牌:%d<<==>>%d;",i,cbTmpValue);
            memcpy(m_cbCardData[i], cbTmpCardData[cbTmpValue], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbKingChangCard[i], cbTmpkingData[cbTmpValue], sizeof(cbTmpkingData[cbTmpValue]));
            memcpy(m_cbHandCardData[i], cbTmpHandCardData[cbTmpValue], sizeof(uint8_t)*MAX_COUNT_SIX);
            m_sMaxCardType[i] = cbtempCardtype[cbTmpValue];
        }
    }
}


void CTableFrameSink::GameTimerOpenCard()
{
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
    m_dwOpenTime = 0;
    //代替开牌
    AutoUserOpenCard();
}

int CTableFrameSink::AutoUserOpenCard()
{
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        if(m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_sCardType[i] != -1)
        {
            continue;
        }

        OnUserOpenCard(i);
        //	LOG_IF(INFO, PRINT_GAME_INFO) << "OnTimerMessage auto open card chairId = " << i;
    }
    return 0;
}

//玩家开牌
int CTableFrameSink::OnUserOpenCard(int wChairID)
{
    if (-1 != m_sCardType[wChairID] && !m_bPlaySixChooseFive)
    {
        //已开牌
        return 1;
    }
    //记录牌型
    m_sCardType[wChairID] = m_sMaxCardType[wChairID];    //(int)m_GameLogic.GetCardType(m_cbCardData[wChairID], MAX_COUNT);
    //倍数
    m_iMultiple[wChairID] =(int)m_iTypeMultiple[m_sCardType[wChairID]];//m_GameLogic.GetMultiple(m_cbCardData[wChairID],m_playGameType);

    //获得牛牛牌
    uint8_t cbOXCard[MAX_COUNT] = { 0 };
    memcpy(cbOXCard, &m_cbCardData[wChairID], sizeof(uint8_t)*MAX_COUNT);
    if (m_sCardType[wChairID]==CT_SHUNZI_NIU || m_sCardType[wChairID]==CT_TONGHUASHUN_NIU)
    {
        m_GameLogic.SortCardList(cbOXCard, MAX_COUNT, true);
    }
    else if (m_sCardType[wChairID]==CT_HULU_NIU || m_sCardType[wChairID]==CT_BOMB_NIU)
    {
        m_GameLogic.SortCardListBH(cbOXCard, m_sCardType[wChairID]);
    }
    else
    {
        m_GameLogic.GetOxCard(cbOXCard,m_cbKingChangCard[wChairID]);
    }
    memcpy(&m_cbOXCard[wChairID], cbOXCard, sizeof(cbOXCard));

    //发送开牌结果
    NN_CMD_S_OpenCardResult OpenCardResult;
    OpenCardResult.set_wopencarduser( wChairID);
    OpenCardResult.set_cbcardtype(m_sCardType[wChairID]);
    if (m_bPlaySixChooseFive)
    {
        for(int j = 0; j < MAX_COUNT_SIX; ++j)
        {
            OpenCardResult.add_cbcarddata(m_cbHandCardData[wChairID][j]);
            if (j<MAX_COUNT)
                OpenCardResult.add_cboxcarddata(cbOXCard[j]);
        }        
    }
    else
    {
        for(int j = 0; j < MAX_COUNT; ++j)
        {
            OpenCardResult.add_cbcarddata(m_cbCardData[wChairID][j]);
            OpenCardResult.add_cboxcarddata(cbOXCard[j]);
        }
    }
    openLog("玩家%d开牌,userMaxType:%d,%d倍,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",wChairID,(int)m_sCardType[wChairID],m_iMultiple[wChairID],m_cbOXCard[wChairID][0],m_cbOXCard[wChairID][1],m_cbOXCard[wChairID][2],m_cbOXCard[wChairID][3],m_cbOXCard[wChairID][4]);
    OpenCardResult.set_typemultiple(m_iTypeMultiple[m_sCardType[wChairID]]); //牌型对应的倍数

    std::string data = OpenCardResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

    //处理下注
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有开牌玩家
        if (-1 == m_sCardType[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //所有人都开牌了
    if ((wTotalCount) == m_wPlayCount)
    {
        //游戏结束
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
        return 10;
    }

    return 12;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t wChairID, uint8_t cbReason)
{
    //清除所有定时器
    ClearAllTimer();

    LOG(INFO)<<"m_wGameStatus = GAME_STATUS_END";

    m_dwEndTime = (uint32_t)time(NULL);

    switch (cbReason)
    {
        case GER_NORMAL:
        {//正常退出
            NormalGameEnd(wChairID);
            break;
        }
        // case GER_USER_LEFT:
        // {//玩家强制退出,则把该玩家托管不出。
        //     break;
        // }
        case GER_DISMISS:
        {//游戏解散
            break;
        }
        default:
            break;
    }
    //游戏结束
    m_bContinueServer = m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    m_pITableFrame->SetGameStatus(GAME_STATUS_END);
    //设置游戏状态-空闲状态
    m_wGameStatus = GAME_STATUS_END;

    //设置托管
    IsTrustee();
}

//普通场游戏结算
void CTableFrameSink::NormalGameEnd(int dwChairID)
{
    NN_CMD_S_GameEnd GameEnd;
    /****
    庄家信息
    ****/
    //庄家本金
    shared_ptr<IServerUserItem> pBankerinfo = m_pITableFrame->GetTableUserItem(m_wBankerUser);
    int64_t llBankerBaseScore;
    if(pBankerinfo)
    {
        llBankerBaseScore= pBankerinfo->GetUserScore();
    }
    else
    {
        llBankerBaseScore = 0;
    }
    //庄家总赢的金币
    int64_t iBankerWinScore = 0;
    //庄家总输赢金币
    int64_t iBankerTotalWinScore = 0;
    //叫庄倍数
    int cbBankerMultiple = m_sCallBanker[m_wBankerUser];
    //庄家牌型倍数
    int cbBankerCardMultiple = m_iTypeMultiple[m_sCardType[m_wBankerUser]];// m_GameLogic.GetMultiple(m_cbCardData[m_wBankerUser],m_playGameType);
    //庄家赢总倍数
    openLog("庄家:%d,抢庄倍数:%d,牌型:%d,倍数:%d,结算前分数:%d;",m_wBankerUser,cbBankerMultiple,m_sCardType[m_wBankerUser],cbBankerCardMultiple,llBankerBaseScore);
    /****
    闲家信息
    ****/
    //闲家总赢的金币
    int64_t iUserTotalWinScore = 0;
    //player total lose score
    int64_t iUserTotalLoseScore = 0;
    //闲家本金
    int64_t llUserBaseScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //闲家各自赢分
    int64_t iUserWinScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //banker rwinscores
    int64_t rBankerWinScore=0;
    //牌型大于牛牛的玩家,额外获得系统赠送的喜钱(底注的3倍),喜钱不算抽水
    int64_t iUserWinXiScore[GAME_PLAYER]={0};

    memset(m_iUserWinScore,0x00,sizeof(m_iUserWinScore));

    //下注倍数
    int64_t cbUserJettonMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //玩家牌型倍数
    int32_t cbUserCardMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //闲家输总倍数
    int64_t wWinUserTotalMultiple = 0;
    //闲家赢总倍数
    int64_t wLostUserTotalMultiple = 0;

    int32_t wLostUserCount = 0;
	int32_t calcMulti[GAME_PLAYER] = { 0 };
    stringstream iStr;
    //计算输赢
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_bPlaySixChooseFive)
        {
            iStr << GlobalFunc::converToHex( m_cbHandCardData[i] ,MAX_COUNT_SIX);
        }
        else
        {
            iStr << GlobalFunc::converToHex( m_cbCardData[i] ,MAX_COUNT);
        }
        
        // 获得的喜钱(底注的3倍)
        if (m_sCardType[i]>10)
        {
			iUserWinXiScore[i] = 0;//m_dwCellScore * 3; //暂不需要
        }

        if(0 == m_cbPlayStatus[i])
            continue;//没有参与游戏不参与结算

        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
            continue;

		if (i == m_wBankerUser)
		{
			llUserBaseScore[i] = puserinfo->GetUserScore();
			continue;	// 庄家不参与闲家结算
		}

        cbUserJettonMultiple[i] = m_cbJettonable[i][m_sAddScore[i]];//m_cbJettonMultiple[m_sAddScore[i]];
        cbUserCardMultiple[i] = m_iTypeMultiple[m_sCardType[i]];// m_GameLogic.GetMultiple(m_cbCardData[i],m_playGameType);

        LOG(INFO)<<"m_cbJettonMultiple[m_sAddScore[i]]"<<(int)cbUserJettonMultiple[i]<<"m_sAddScore[i]"<<(int)m_sAddScore[i];//m_cbJettonMultiple[m_sAddScore[i]]
        LOG(INFO)<<" cbUserCardMultiple[i]"<< (int)cbUserCardMultiple[i];
        LOG(INFO)<<" cbBankerMultiple======"<< (int)cbBankerMultiple<<"m_wBankerUser====="<<(int)m_wBankerUser;
        //闲家原始积分

        llUserBaseScore[i] = puserinfo->GetUserScore();
        int64_t iTempScore = 0;
        if(m_GameLogic.CompareCard(m_cbCardData[m_wBankerUser], m_cbCardData[i],m_cbKingChangCard[m_wBankerUser],m_cbKingChangCard[i],false))
        {
            //庄赢  底分 * 抢庄倍数 * 下注倍数 * 牌型倍数
            iTempScore = m_dwCellScore * cbBankerMultiple * cbUserJettonMultiple[i] * cbBankerCardMultiple;
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }
            openLog("闲家:%d输,牌型:%d,下注倍数:%d,输庄家倍数:%d,理论输分:%d;",i,m_sCardType[i],cbUserJettonMultiple[i],cbBankerCardMultiple,-iTempScore);
            iBankerWinScore += iTempScore;
            iBankerTotalWinScore += iTempScore;
            rBankerWinScore += iTempScore;

            //玩家总输分
            iUserWinScore[i] = -iTempScore;
            iUserTotalLoseScore += -iTempScore;
            //玩家总赢倍数
            wLostUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];

            //玩家输的人数
            ++wLostUserCount;
			//牌局倍数
			calcMulti[i] = cbBankerMultiple * cbUserJettonMultiple[i] * cbBankerCardMultiple;
			calcMulti[m_wBankerUser] = cbBankerMultiple * cbUserJettonMultiple[i] * cbBankerCardMultiple;
        }else
        {
            //闲赢
            iTempScore = m_dwCellScore * cbBankerMultiple * cbUserJettonMultiple[i] * cbUserCardMultiple[i];
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }
            openLog("闲家:%d赢,牌型:%d,下注倍数:%d,赢庄家倍数:%d,理论赢分:%d;",i,m_sCardType[i],cbUserJettonMultiple[i],cbUserCardMultiple[i],iTempScore);
            iUserTotalWinScore += iTempScore;
            iBankerTotalWinScore -= iTempScore;
            rBankerWinScore += iTempScore;

            //玩家赢分
            iUserWinScore[i] = iTempScore;
            //输家总倍数
            wWinUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];
			//牌局倍数
			calcMulti[i] = cbBankerMultiple * cbUserJettonMultiple[i] * cbUserCardMultiple[i];
			calcMulti[m_wBankerUser] = cbBankerMultiple * cbUserJettonMultiple[i] * cbBankerCardMultiple;
        }
    }

    iStr << m_wBankerUser;
    //庄家计算出来的赢分
    int64_t iBankerCalWinScore = 0;

    //如果庄家赢的钱大于庄家的基数
    if (iBankerTotalWinScore > llBankerBaseScore)
    {
        rBankerWinScore = 0;
        //庄家应该收入金币最多为 庄家本金+闲家总赢的金币
        iBankerCalWinScore = llBankerBaseScore + iUserTotalWinScore;

        //按照这个金额平摊给输家
       // int64_t iEachPartScore = iBankerCalWinScore * 100 / wLostUserTotalMultiple;
        int64_t bankerliss=0;
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0 || i == m_wBankerUser)
                continue;

            if (iUserWinScore[i] > 0){
                rBankerWinScore += iUserWinScore[i] ;
                continue;
            }

            //当前玩家真正要输的金币分摊一下
            double iUserNeedLostScore = iBankerCalWinScore*iUserWinScore[i]/iUserTotalLoseScore;//iEachPartScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i]/100;
            if (iUserNeedLostScore > llUserBaseScore[i])
            {
                iUserNeedLostScore = llUserBaseScore[i];
                bankerliss-=(iUserNeedLostScore-llUserBaseScore[i]);///////////yufang wanjia bu gou
            }
            rBankerWinScore += iUserNeedLostScore;
            iUserWinScore[i] = -iUserNeedLostScore;
        }

        //庄家赢的钱是庄家本金
        //iBankerRealWinScore = (int)llBankerBaseScore;
        iUserWinScore[m_wBankerUser] = llBankerBaseScore+bankerliss;

        //闲家赢钱的，再赢多少就赢多少，不需要有变动
    }
    else if (iBankerTotalWinScore + llBankerBaseScore < 0)   //如果庄家的钱不够赔偿
    {
        //庄家能赔付的钱总额
        int64_t iBankerMaxScore = llBankerBaseScore + iBankerWinScore;
        rBankerWinScore = iBankerMaxScore;
        //计算玩家每部分的钱
       // int64_t iEachPartScore = iBankerMaxScore * 100 / wWinUserTotalMultiple;

        //闲赢 (庄家本金 + 庄赢的总金币)*当前闲家赢的金币 /（闲家总赢）
        int64_t bankerscore=0.0;
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (0 == m_cbPlayStatus[i])
                continue;
            if (i == m_wBankerUser)
                continue;

            //输钱不算
            if (iUserWinScore[i] < 0)
                continue;

            double iUserRealWinScore = iBankerMaxScore*iUserWinScore[i]/iUserTotalWinScore;//iEachPartScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i]/100;
            if (iUserRealWinScore > llUserBaseScore[i])
            {
                iUserRealWinScore = llUserBaseScore[i];
                bankerscore+=(iUserRealWinScore-llUserBaseScore[i]);
            }

            iUserWinScore[i] = iUserRealWinScore;
        }

        //庄家赢的钱
        //iBankerRealWinScore = -llBankerBaseScore;
        iUserWinScore[m_wBankerUser] -= llBankerBaseScore;
        iUserWinScore[m_wBankerUser] += bankerscore;
    }
    else
    {
        //其他情况，庄家输赢
        iUserWinScore[m_wBankerUser] = iBankerTotalWinScore;
    }
	if (iUserWinScore[m_wBankerUser]<0)
	{
		calcMulti[m_wBankerUser] = cbBankerMultiple;
	}
	else
	{
		calcMulti[m_wBankerUser] = cbBankerMultiple * cbBankerCardMultiple;
	}
    //写入积分
    int64_t iRevenue[GAME_PLAYER] = {0};
   // int64_t CalculateAgentRevenue[GAME_PLAYER] = {0};
    int userCount=0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        GameEnd.add_dlwscore(0);
        GameEnd.add_dtotalscore(0);
        GameEnd.add_cboperate(0);
        GameEnd.add_cbxiscore(iUserWinXiScore[i]);
    }

	int64_t res = 0.0;
   // int64_t revenue;
    vector<tagScoreInfo> scoreInfoVec;
    tagScoreInfo scoreInfo;
    // memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
        if(!puserinfo)
        {
             continue;
        }

        if(!m_bPlayerIsOperation[i])//meiyou ca0 zuo
        {
            // GameEnd.set_cboperate(i, 0);
             m_cbPlayStatus[i] = 0;
        }
        GameEnd.set_cboperate(i, 0);

        //计算税收
        bool bAndroid = m_pITableFrame->IsAndroidUser(i);

        //统计库存
        if (bAndroid)
        {
            m_llStorageControl += iUserWinScore[i];
            res += iUserWinScore[i];
            //ResetTodayStorage();
        }else
        {

        }

        if (iUserWinScore[i] > 0)
        {
            iRevenue[i] = m_pITableFrame->CalculateRevenue(iUserWinScore[i]);
           // CalculateAgentRevenue[i] = m_pITableFrame->CalculateAgentRevenue(i,iUserWinScore[i]);
        }
       // else
       // {
       //     iRevenue[i] = 0;
       //     CalculateAgentRevenue[i] =m_pITableFrame->CalculateAgentRevenue(i,-iUserWinScore[i]);
       // }

        //LOG(WARNING) << "user  " << i << " win score: " << iUserWinScore[i];
        if (m_wBankerUser==i)
        {
            openLog("庄家:%d,最终得分:%d;",i,iUserWinScore[i]-iRevenue[i]+iUserWinXiScore[i]);
        }
        else
        {
            openLog("玩家:%d,最终得分:%d;",i,iUserWinScore[i]-iRevenue[i]+iUserWinXiScore[i]);
        }
        
        double iLWScore = iUserWinScore[i] - iRevenue[i] + iUserWinXiScore[i];
        scoreInfo.clear();
        scoreInfo.chairId=i;

        scoreInfo.addScore=iLWScore;
        scoreInfo.startTime=m_dwStartTime;

        scoreInfo.revenue=iRevenue[i];
        string tmp96 = iUserWinXiScore[i] > 0 ? (","+ to_string(iUserWinXiScore[i])) : "";
        uint8_t t[1];
        if( i != m_wBankerUser){
            t[0] = 0xff & cbUserJettonMultiple[i];
            scoreInfo.cardValue = iStr.str() + GlobalFunc::converToHex(t,1) + tmp96.c_str();
            scoreInfo.rWinScore = abs(iUserWinScore[i]);
        }else{
            t[0] = 0xff & cbBankerMultiple;
            scoreInfo.cardValue = iStr.str() + GlobalFunc::converToHex(t,1) + tmp96.c_str();
            scoreInfo.isBanker = 1;
            scoreInfo.rWinScore = rBankerWinScore;
        }
        LOG(INFO)<<"playerUserID:"<< (int)puserinfo->GetUserId() << " scoreInfo.cardValue:"<< scoreInfo.cardValue;
        scoreInfo.betScore = scoreInfo.rWinScore;
        if (m_bPlaySixChooseFive)
        {
            m_replay.addResult(i,i,scoreInfo.betScore,scoreInfo.addScore,
                           to_string(m_sCardType[i])+":"+GlobalFunc::converToHex( m_cbHandCardData[i] ,MAX_COUNT_SIX)+":"+GlobalFunc::converToHex( m_cbCardData[i] ,5)+"|"+to_string(iUserWinXiScore[i]),i == m_wBankerUser);
        }
        else
        {
            m_replay.addResult(i,i,scoreInfo.betScore,scoreInfo.addScore,
                           to_string(m_sCardType[i])+":"+GlobalFunc::converToHex( m_cbCardData[i] ,5)+"|"+to_string(iUserWinXiScore[i]),i == m_wBankerUser);
        }
        scoreInfoVec.push_back(scoreInfo);

        userCount++;
        GameEnd.set_dlwscore(i, iLWScore);
        m_iUserWinScore[i] = iLWScore;
        GameEnd.set_dtotalscore(i,iLWScore+ puserinfo->GetUserScore());
        PaoMaDengCardType[i]=m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
        PaoMaDengWinScore[i]=iLWScore;
        LOG(INFO)<<"m_llStorageControl"<<"="<<m_llStorageControl;
        LOG(INFO)<<"m_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
        LOG(INFO)<<"m_llStorageHighLimit"<<"="<<m_llStorageHighLimit;

        LOG(INFO)<<"palyerCardtype="<<(int)puserinfo->GetUserId()<<"    "<<(int)PaoMaDengCardType[i];

        for(int j = 0; j < 5; ++j)
        {
            int inde = (int)m_cbCardData[i][j] % 16 + ((int)m_cbCardData[i][j] / 16) * 100;
            LOG(INFO)<<"player="<<(int)puserinfo->GetUserId()<<"      palyerCard="<<inde;
        }
    }

    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
   // ZeroMemory(&ceshi,sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update before----------------- "<<ceshi.lEndStorage;
    LOG(INFO)<<"update Num----------------- "<<res;
    if( res > 0)
    {
        res = res*((1000.0f-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0f); // - m_pITableFrame->CalculateRevenue(res);
    }
	openLog("=== 本局库存变化:%ld;", res);
    m_pITableFrame->UpdateStorageScore(res);
	//对局记录详情
	SetGameRecordDetail(llUserBaseScore, m_iUserWinScore, calcMulti);

    m_pITableFrame->SaveReplay(m_replay);
    memset(&ceshi, 0, sizeof(ceshi));
   // ZeroMemory(&ceshi,sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update after ----------------- "<<ceshi.lEndStorage;

	m_llStorageControl = ceshi.lEndStorage;
	m_llStorageLowerLimit = ceshi.lLowlimit;
	m_llStorageHighLimit = ceshi.lUplimit;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
        {
            continue;
        }
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
		int64_t aaaa = puserinfo->GetUserScore();
        LOG(INFO)<<"userid="<<puserinfo->GetUserId()<<"    score="<<aaaa;
    }
    g_nRoundID++;
    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), mGameIds);

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
        {
            continue;
        }
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
		int64_t aaa=puserinfo->GetUserScore();
        LOG(INFO)<<"userid="<<puserinfo->GetUserId()<<"    score="<<aaa;
    }
    //判断通杀通赔
    if (m_wPlayCount > 2)
    {
        if (wLostUserCount == 0)
        {
            GameEnd.set_cbendtype( 2);
        }
        else if (wLostUserCount == (m_wPlayCount - 1))
        {
            GameEnd.set_cbendtype( 1);
        }
        else
        {
            GameEnd.set_cbendtype( 0);
        }
    }
    else
    {
        GameEnd.set_cbendtype( -1);
    }

    //发送数据
    std::string data = GameEnd.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
    // if (m_testSameTypeCount>0) m_testSameTypeCount--;
    // openLog("====测试牌型%d,还剩余:%d局;",m_testMakeType,m_testSameTypeCount);
    LOG(INFO)<<"===SendTableData NN_SUB_S_GAME_END===";
    // if (m_testSameTypeCount==0 && m_testMakeType>0)
    // {
    //     m_testMakeType--;
    //     m_testSameTypeCount=m_bTestCount;
    // }

    PaoMaDeng();//發送跑馬燈
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(!user)
        {
            continue;
        }
        if(user->GetUserStatus() != sOffline)
        {
            user->SetUserStatus(sReady);
        }
        if((!m_bPlayerIsOperation[i]))
        {
             user->SetUserStatus(sOffline);
            // m_inPlayingStatus[i] = 0;

             LOG(INFO)<<"NO NO NO!!!!!!!!!!!!!!!!Operration and Setoffline";
        }
        LOG(INFO)<<"SET ALL PALYER  READY-------------------";
    }
}

void CTableFrameSink::GameTimerEnd()
{
    //muduo::MutexLockGuard lock(m_mutexLock);
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);

    // m_pITableFrame->SetGameStatus(201);//jie su

    LOG(INFO)<<"------------------------------------GameTimerEnd------------------------------------";

    m_dwEndTime = 0;

    if(!m_bContinueServer)
    {
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                openLog("3ConcludeGame=false  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
            }
        }
        return;
    }

    LOG(INFO)<<"------------------------------------GameTimerEnd1------------------------------------";
    openLog("=== 游戏结束 mGameRoundId:[%s],lEndStorage:[%ld],lLowlimit:[%ld],lUplimit:[%ld];",mGameIds.c_str(), m_llStorageControl, m_llStorageLowerLimit, m_llStorageHighLimit);
    // int AndroidNum = 0;
    // int realnum = 0;
    // for(int i = 0; i < GAME_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
    //     if(user)
    //     {
    //         if(user->IsAndroidUser())
    //         {
    //             AndroidNum++;
    //         }
    //         realnum++;
    //     }
    // }
    // if(AndroidNum == realnum)
    // {
    //     for(int i = 0; i < GAME_PLAYER; ++i)
    //     {
    //         shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
    //         if(user)
    //             user->SetUserStatus(sOffline);
    //     }
    // }

    for(uint8_t i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            LOG(ERROR) << "Clean User, chairId" << (int)i;
            user->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_LONGTIME_NOOP);
            /*
            //openLog("  wanjia zhuang tai  renshu=%d",user->GetUserStatus());
            if(user->GetUserStatus()==sOffline)
            {
                // m_inPlayingStatus[i] = 0;
                openLog("4OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(INFO)<<"1Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }else if(user->GetUserScore()<m_pGameRoomInfo->enterMinScore)
            {
                // m_inPlayingStatus[i] = 0;
                openLog("5OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(INFO)<<"2Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }
            */
        }
    }

    //设置游戏状态
   // m_wGameStatus = GAME_STATUS_READY;
    m_wGameStatus = GAME_STATUS_INIT;
    // LOG(INFO)<<"------------------------------------m_wGameStatus = GAME_STATUS_READY------------------------------------";

    //游戏结束
    //当前时间
    //std::string strTime = Utility::GetTimeNowString();
    //m_pITableFrame->ConcludeGame(GAME_STATUS1_FREE, strTime.c_str());
    m_pITableFrame->ClearTableUser(INVALID_CHAIR, true);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    // m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(TIME_READY, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
    // m_dwReadyTime = (uint32_t)time(NULL);
    //m_pITableFrame->SetGameTimer(ID_TIME_READY,TIME_READY*TIME_BASE);
    InitGameData();
}

//游戏消息
bool CTableFrameSink::OnGameMessage( uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //muduo::MutexLockGuard lock(m_mutexLock);
    if( wChairID < 0 || wChairID >= GAME_PLAYER)
    {
        LOG(ERROR) << " mGameRoundId[" << mGameIds << "] OnGameMessage INVALID_CHAIR wChairID " << wChairID;
        return true;
    }
    int wRtn = 0;
    switch (wSubCmdID)
    {
        case NN_SUB_C_CALL_BANKER:					//叫庄消息
        {
            LOG(INFO)<<"***************************USER CALL BANK1";
            //游戏状态不对
            if (GAME_STATUS_CALL != m_wGameStatus)
            {
                LOG(INFO)<<"***************************GAME_STATUS_CALL != m_wGameStatus";
                return false;
            }

            //不是游戏玩家
            if (1 != m_cbPlayStatus[wChairID])
            {
                LOG(INFO)<<"***************************1 != m_cbPlayStatus[wChairID]";
                return false;
            }


            //变量定义
            NN_CMD_C_CallBanker  pCallBanker;
            pCallBanker.ParseFromArray(pData,wDataSize);

            LOG(INFO)<<"***************************1 pCallBanker"<<(int)pCallBanker.cbcallflag();

            //玩家叫庄
            wRtn = OnUserCallBanker(wChairID, pCallBanker.cbcallflag());
            m_bPlayerIsOperation[wChairID] = true;
            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserCallBanker wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn > 0;
        }
        case NN_SUB_C_ADD_SCORE:					//下注消息
        {
            //游戏状态不对
            if (GAME_STATUS_SCORE != m_wGameStatus)
            {
                return false;
            }

            //不是游戏玩家
            if (1 != m_cbPlayStatus[wChairID])
            {
                return false;
            }

            //变量定义
            NN_CMD_C_AddScore  pAddScore;
            pAddScore.ParseFromArray(pData,wDataSize);

            //玩家下注
            wRtn = OnUserAddScore(wChairID, pAddScore.cbjettonindex());
            m_bPlayerIsOperation[wChairID]=true;
            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserAddScore wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn > 0;
        }
        case NN_SUB_C_OPEN_CARD:					//开牌消息
        {
            //如果是6张手牌,不处理开牌
            if (m_bPlaySixChooseFive)
                return true;
            m_bPlayerIsOperation[wChairID]=true;
            if (GAME_STATUS_OPEN != m_wGameStatus)
            {
                //游戏状态不对
                return false;
            }

            if (1 != m_cbPlayStatus[wChairID])
            {
                //不是游戏玩家
                return false;
            }

            //玩家下注
            wRtn = OnUserOpenCard(wChairID);

            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserOpenCard wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn   > 0;
        }
    }
    return true;
}


////更新游戏配置
//void CTableFrameSink::UpdateGameConfig()
//{
//    if (NULL == m_pITableFrame)
//    {
//        return;
//    }
//}

//清除所有定时器
void CTableFrameSink::ClearAllTimer()
{
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);

}

void CTableFrameSink::ResetTodayStorage()
{
    uint32_t nTimeNow = time(NULL);
    // && isAcrossTheDay(m_iTodayTime, nTimeNow)
    if (nTimeNow - m_iTodayTime > 86400)
    {
        LOG(WARNING) << "AcrossTheDay 重置今日库存";
        m_llTodayStorage = 0;
        m_iTodayTime = nTimeNow;
    }
}

bool CTableFrameSink::IsTrustee(void)
{
    float dwShowTime = 0.0;
    if (m_wGameStatus == GAME_STATUS_CALL)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        dwShowTime = 1.5; //发牌动画  1.5
        m_TimerIdCallBanker=m_TimerLoopThread->getLoop()->runAfter(TIME_CALL_BANKER + dwShowTime, boost::bind(&CTableFrameSink::GameTimerCallBanker, this));
    }
    else if (m_wGameStatus == GAME_STATUS_SCORE)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        dwShowTime = 1.5; //抢庄动画  2.0
        m_TimerIdAddScore=m_TimerLoopThread->getLoop()->runAfter(TIME_ADD_SCORE + dwShowTime, boost::bind(&CTableFrameSink::GameTimerAddScore, this));
    }
    else if (m_wGameStatus == GAME_STATUS_OPEN)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        dwShowTime = 1.0; //开牌动画  2.0
        if (m_bPlaySixChooseFive)
        {
            m_TimerIdOpenCard=m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD_SIX + dwShowTime, boost::bind(&CTableFrameSink::GameTimerOpenCard, this));
        }
        else
        {
            m_TimerIdOpenCard=m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD + dwShowTime, boost::bind(&CTableFrameSink::GameTimerOpenCard, this));
        }        
    }
    else if (m_wGameStatus == GAME_STATUS_END)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
        if(m_isMatch)
            m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_MATCH_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
        else
            m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
        //LOG_IF(INFO, PRINT_GAME_INFO) << "OnEventGameEnd game end------";
    }
    return true;
}


void CTableFrameSink::RepositionSink()
{

}

//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userScore[], int64_t userWinScorePure[], int32_t calcMulti[])
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pITableFrame->IsExistUser(i)) {
			continue;
		}
		shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);

		PlayerRecordDetail* detail = details.add_detail();
		//boost::property_tree::ptree item, itemscores, itemscorePure;
		//账号/昵称
		detail->set_account(std::to_string(pIServerUser->GetUserId()));
		//座椅ID
		detail->set_chairid(i);
		//是否庄家
		detail->set_isbanker(m_wBankerUser == i ? true : false);
		//手牌/牌型
		HandCards* pcards = detail->mutable_handcards();
		int count = 0;
		if (m_bPlaySixChooseFive)
		{
			uint8_t handCards[MAX_COUNT_SIX] = { 0 };
			for (int id = 0; id < MAX_COUNT_SIX; id++)
			{
				if (id < MAX_COUNT)
				{
					handCards[count++] = m_cbCardData[i][id];
				}
				else
				{
					bool bSameChoose = false;
					for (int j = 0; j < MAX_COUNT_SIX; j++)
					{
						bSameChoose = false;
						for (int k = 0; k < MAX_COUNT_SIX; k++)
						{
							if (k >= count) continue;
							if (m_cbHandCardData[i][j] == handCards[k])
							{
								bSameChoose = true;
								break;
							}
						}
						if (!bSameChoose)
						{
							handCards[count++] = m_cbHandCardData[i][j];
							if (count >= MAX_COUNT_SIX) break;
						}
					}
					if (count >= MAX_COUNT_SIX) break;
				}
			}
			pcards->set_cards(&(handCards)[0], MAX_COUNT_SIX);
		} 
		else
		{
			pcards->set_cards(&(m_cbCardData[i])[0], MAX_COUNT);
		}
		int nType = m_sCardType[i];//(int)m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
		pcards->set_ty(nType);
		//牌型倍数
		pcards->set_multi(m_iTypeMultiple[m_sCardType[i]]);
		//携带积分
		detail->set_userscore(userScore[i]);
		//房间底注
		detail->set_cellscore(m_dwCellScore);
		//下注/抢庄倍数
		detail->set_multi((m_wBankerUser == i) ? m_sCallBanker[i] : m_cbJettonable[i][m_sAddScore[i]]);
		//牌局倍数
		detail->set_calcmulti(calcMulti[i]);
		//输赢积分
		detail->set_winlostscore(userWinScorePure[i]);
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		m_replay.detailsData = details.SerializeAsString();
	}
}

//////////////////////////////////////////////////////////////////////////
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
