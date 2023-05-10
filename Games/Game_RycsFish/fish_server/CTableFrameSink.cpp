#include "StdAfx.h"
//#include "util/NickNameUtility.h"
//#include <TextParserUtility.h>
//#include <pthread.h>
//#include <kernel/CriticalSection.h>
#ifdef _WIN32
#else
#include <iunknown.h>
#include "../../../public/Globals.h"
#include "../../../public/ITableFrameSink.h"
#include "../../../public/ITableFrame.h"
#include <glog/logging.h>
#endif//_WIN32

using namespace std;

#include "../inc/aics/IAicsEngine.h"
#include "SceneManager.h"
#include "FishProduceManager.h"
#include "ISendTableData.h"
#include "gameplay/collect/CollectUtility.h"
#include "ITimerEvent.h"
#include "UserPrefs.h"
#include "../FishProduceUtils/FishProduceDataManager.h"
#include "../FishProduceUtils/FishArrayManager.h"

#ifdef __USE_LIFE2__
#include "ArithmLog.h"
#endif

#include "../FishProduceUtils/NetworkDataUtility.h"
#include "../FishProduceUtils/FishProduceCommon.h"

#define CHECK_BOSS(fishLife) (fishLife >= 50)

#ifdef __DFW__ // 大富翁
#define ROOM_MESSAGE_KILL_BOSS _T("恭喜“%s”在“%s”捕获“%s”， 获得 ,%u, 金币！")
#define ROOM_MESSAGE_MSYQ _T("恭喜“%s”在“%s”获得“马上有钱”彩金奖励 ,%u, 金币！")
#define ROOM_MESSAGE_COLLECT _T("恭喜“%s”在“%s”完成“收集”任务， 获得额外 ,%u, 金币！")
#define ROOM_MESSAGE_PPL _T("恭喜“%s”在“%s”通过“开心30秒”拍得彩金 ,%u, 金币！")
#else
#define ROOM_MESSAGE_KILL_BOSS _T("恭喜“%s”在“%s”捕获“%s”， 获得 ,%u, 金币！")
#define ROOM_MESSAGE_MSYQ _T("恭喜“%s”在“%s”获得“马上有钱”彩金奖励 ,%u, 金币！")
#define ROOM_MESSAGE_COLLECT _T("恭喜“%s”在“%s”完成“收集”任务， 获得额外 ,%u, 金币！")
#define ROOM_MESSAGE_PPL _T("恭喜“%s”在“%s”通过“开心30秒”拍得彩金 ,%u", 金币！)
#endif
#include "CTableFrameSink.h"
// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 3;

#ifdef __HJSDS__
        dir += "/sdsfish";
    google::InitGoogleLogging("sdsfish");
#else
        dir += "/rycsfish";
    google::InitGoogleLogging("rycsfish");
#endif//__HJSDS__
        FLAGS_log_dir = dir;
        // try to make the special directory value item now.
        mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
        // set std output special log content value now.
        google::SetStderrLogging(google::GLOG_WARNING);
    }
    virtual ~CGLogInit() {
        // google::ShutdownGoogleLogging();
    }
};
CGLogInit glog_init;
extern void log_fishes(const char* fmt,...);
/*
void log_bullets(const char* fmt,...)
{
    va_list va;
    char buf[1024]={0};
    char sztime[256]={0};
    // build the buffer.
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

    timeval tv;
    time_t tt;
    time(&tt);
    struct tm* st = localtime(&tt);
    gettimeofday(&tv,NULL);
    int milliSecond = int((tv.tv_sec*1000)+(tv.tv_usec/1000));
    snprintf(sztime,sizeof(sztime),"%02d/%02d %02d:%02d:%02d-%d : ",
             st->tm_mon+1,st->tm_mday,st->tm_hour,
             st->tm_min,  st->tm_sec,milliSecond,
             (int)pthread_self());
    // try to build the full buffer.
    std::string strValue = sztime;
    strValue += buf;
    strValue += "\n";
    // try to open the special fish item now.
    FILE* fp = fopen("bullets.log","a");
    fputs(strValue.c_str(),fp);
    fclose(fp);
} */
// 应当注意：计时器ID的范围是1-20 ( >=1 && < 20 )
// 这里有超过20的 所以要把GameServiceHead.h的TIME_TABLE_SINK_RANGE改大点才行，或者改进计时器，比如共用
const DWORD kGlobalTimer             = 1;       //
const DWORD kPauseBoomTimer          = 2;       //
const DWORD kWriteBgDataTimer        = 10;      //
const DWORD kSynchorScoreTimer       = 11;      // synchor user score on switch scene.

const DWORD kGlobalTimerElasped      = 1000;     // 50; modify by James min time interval is 0.1 second.
const DWORD kSwitchSceneTimerElasped = 10000;
const DWORD kPauseBoomTimerElasped   = 10000;
const DWORD kWriteBgDataTimerElasped = 30000;
const DWORD kGlobalTimerSynchorScore = 5000;    // delay 5 second to synchor user score.
const DWORD kGlobalTimerClearuserElasped = 5000;    // delay 5 second to synchor user score.

const DWORD kGlobalTimerClearNotOperateUserElasped = 1000;    // delay 5 second to synchor user score.
// #define BROADCAST_MESSAGE_MIN_VALUE 1
#define MESSAGE_WINDOW_CLASS_NAME TEXT("TableFrameSinkProcMsgWnd")
#define IS_GLOBAL_MESSAGE(fishScore) ((!game_config.isExperienceRoom) && (game_config.broadcast_minval) && (getExchangeUserScore(fishScore) >= game_config.broadcast_minval))

enum eGameTimerType {
    GAME_TIMER_FISH_PRODUCE = 1,    // produce fish.
    GAME_TIMER_SCENE_SWITCH = 2,    // switch scene.
    GAME_TIMER_FISH_ARRAY   = 3,    // fish array value.
};

enum eGameTimerNewType {
    GAME_POINT_FISH_PRODUCE = 100,    // produce fish.
    GAME_POINT_SCENE_SWITCH = 101,    // switch scene.
    GAME_POINT_FISH_ARRAY   = 102,    // fish array value.
    GAME_POINT_kPauseBoom   = 103,    // fish pause bomb.
    GAME_POINT_kWriteBgData   = 104,    // fish writedate.
    GAME_POINT_kSynchorScore   = 105,    // fish SynchorScore.
};
const DWORD kRepeatTimer = -1;
// UserPrefs* CTableFrameSink::m_userPrefs = NULL;
WORD CTableFrameSink::instanceCount = 0;
vector<std::shared_ptr<ITableFrame>> CTableFrameSink::tableFrameList;

#ifdef __USE_LIFE2__
ArithmLog* CTableFrameSink::m_traceArithm = NULL;
#endif

CTableFrameSink::CTableFrameSink() : m_pITableFrame(NULL)
                    //,m_pGameRoomInfo(NULL)                    //,game_service_option_(NULL)
                    ,exchange_count_(1)
                    ,m_is_switching_scene(false)
                    ,m_is_nestScene(false)
                    ,wSwitchSceneTimerElasped(kSwitchSceneTimerElasped)
                    ,m_beginUserID(0)
                    ,game_started(false)
                    /*,m_i_log_db_engine(NULL), m_debugTickProduce(0)*/ 
                    ,m_currentFishArray(NULL)
                    ,m_fishProduceDataManager(NULL)
                    ,m_coinRatioCoin(1)
                    ,m_coinRatioScore(1)
                    ,fpalyerReventPercent(0.07f)
                    ,exchangeAmountForExperienceRoom(100)
#ifdef __GAMEPLAY_COLLECT__
                    ,m_collectGiftingMultipleMin(0)
                    ,m_collectGiftingMultipleMax(0)
#endif//__GAMEPLAY_COLLECT__
#ifdef __GAMEPLAY_MSYQ__
                    ,m_msyqCopies(1)
#endif//__GAMEPLAY_MSYQ__
{
#ifdef __GAMEPLAY_COLLECT__
	memset(m_collectTotalFishScore, 0, sizeof(m_collectTotalFishScore));
	memset(m_collectGiftingMultiple, 0, sizeof(m_collectGiftingMultiple));
#endif

#ifdef __GAMEPLAY_MSYQ__
	memset(m_msyqAward, 0, sizeof(m_msyqAward));
	memset(m_msyqSumAward, 0, sizeof(m_msyqSumAward));
	memset(m_msyqAverageAward, 0, sizeof(m_msyqAverageAward));
#endif
	memset(m_rumtimeWashData, 0, sizeof(m_rumtimeWashData));
	memset(m_washPercent, 0, sizeof(m_washPercent));
	memset(&m_washXmlConfig, 0, sizeof(m_washXmlConfig));
	memset(m_exp_user_score, 0, sizeof(m_exp_user_score));
	memset(m_currentSN, 0, sizeof(m_currentSN));
	memset(m_pServerUserItemForGamescene, 0, sizeof(m_pServerUserItemForGamescene));
	memset(m_bulletCount, 0, sizeof(m_bulletCount));
	memset(m_bullets, 0, sizeof(m_bullets));
	memset(&game_config, 0, sizeof(game_config));
	setExchangeScoreFormat(0);
    memset(&bulletlevel, 1, sizeof(bulletlevel));
	//memset(m_playerScoreInfo, 0, sizeof(m_playerScoreInfo));

#ifdef __GAMEPLAY_MSYQ__
	SET_GAMEPLAY(game_config.supportedGameplays, gptMsyq);      // 玩法:马上有钱
#endif

#ifdef __GAMEPLAY_COLLECT__
	SET_GAMEPLAY(game_config.supportedGameplays, gptCollect);   // 玩法:收集玩法
#endif

#ifdef __GAMEPLAY_PAIPAILE__
	SET_GAMEPLAY(game_config.supportedGameplays, gptPpl);       // 玩法:拍拍乐玩法
#endif

#ifdef __DYNAMIC_CHAIR__
	resetDynamicChair();
#endif

    // initialize the game version now.
	game_config.version = GAME_VERSION;
	game_config.ticketRatio = 1;
    game_config.exchangeRatioUserScore = game_config.exchangeRatioFishScore = 1;
	//game_config.exchangeRatio = game_config.ticketRatio;

#ifdef __GAMEPLAY_PAIPAILE__
	memset(m_pplRuntimeData, 0, sizeof(m_pplRuntimeData));
	game_config.pplXmlConfig.fishType = -1;
#endif
// 	memset(m_debugTickQuit, 0, sizeof(m_debugTickQuit));
#ifdef __MULTI_THREAD__
	CCriticalSectionEx::Create(&m_cs);
	CCriticalSectionEx::Create(&m_csSend);
#endif
 //	if (0 == instanceCount++) {
 //		FishProduceCommon::initializeInstance();
 //		CreateMessageWnd();
 //#ifdef ENABLE_NEW_USER_CHECK
 //		m_userPrefs = new UserPrefs;
 //		TRACE_LOG_BEGIN;
 //#endif
 //#ifdef __USE_LIFE2__
 //		m_traceArithm = new ArithmLog();
 //#endif
 //		TRACE_INFO_BEGIN;
 //	}
	m_fishProduceDataManager = new FishProduceDataManager();
    aics_kindid = 0;
    memset(m_fishIDs,0,sizeof(m_fishIDs));
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        lostScore[i]=0;
        isNotOutRoom[i]=false;
        sumScore[i]=0;
        playerScore[i]=0;
        playerAddScore[i]=0;
        notOperate[i]=0;
    }
    sceneTimeLeft=0.0f;
    isGroupScene=false;

    Resurrection = 0;
	m_startChangSceneTime = 0;
	memset(m_bHaveLaser, 0, sizeof(m_bHaveLaser));
	m_nBaseLaserOdds = 0;
    m_nLessLaserOdds = 0;
	memset(m_bHaveBorebit, 0, sizeof(m_bHaveBorebit));
	m_startFireCountTime = 0;
	m_startFireTime = muduo::Timestamp::now();
	m_startFireAngle = 0;
	memset(m_fireScore, 0, sizeof(m_fireScore));
	memset(	m_fireAllWinScore, 0, sizeof(m_fireAllWinScore));
	m_nfireMaxOdds = 0;
	m_bBorebitFireBoom = false;
	m_isNeedExtendSwitchScene = false;
	m_SpecialKillFishProbili_JG[0] = 5;
	m_SpecialKillFishProbili_JG[1] = 10;
	m_SpecialKillFishProbili_JG[2] = 25;
	m_SpecialKillFishProbili_JG[3] = 35;
	m_SpecialKillFishProbili_ZT[0] = 2;
	m_SpecialKillFishProbili_ZT[1] = 8;
	m_SpecialKillFishProbili_ZT[2] = 15;
	m_SpecialKillFishProbili_ZT[3] = 30;
	memset(m_bCatchSpecialFireInGlobalBomb, 0, sizeof(m_bCatchSpecialFireInGlobalBomb));
}

CTableFrameSink::~CTableFrameSink()
{
 	//LOG(INFO) << "CTableFrameSink::~CTableFrameSink. table_id=%d"), m_pITableFrame->GetTableID());
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireLaser);
	m_TimerLoopThread->getLoop()->cancel(m_TimeridBorebitStartRotate);
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebit);
	m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebitEnd);
	GetAicsEngine()->Term();
	if (m_fishProduceDataManager) {
		delete m_fishProduceDataManager;
		m_fishProduceDataManager = NULL;
	}
	if (--instanceCount == 0)
    {
		FishProduceCommon::destroyInstance();
        /*
		if (m_userPrefs)
		{
			delete m_userPrefs;
			m_userPrefs = NULL;
			TRACE_LOG_END;
        }   */
#ifdef __USE_LIFE2__
		if (m_traceArithm)
		{
			delete m_traceArithm;
			m_traceArithm = NULL;
		}
#endif//__USE_LIFE2__
		tableFrameList.clear();
		TRACE_INFO_END;
	}
#ifdef __MULTI_THREAD__
	CCriticalSectionEx::Destroy(&m_cs);
	CCriticalSectionEx::Destroy(&m_csSend);
#endif
}

// initialize special table sink item and store table pointer.
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    // store table frame pointer.
    m_pITableFrame = pTableFrame;
    if (!m_pITableFrame) return false;
    // initialize the special attribute content value now.
    // m_pGameRoomInfo = m_pITableFrame->GetGameKindInfo();
 //#ifdef __GAMEPLAY_PAIPAILE__
 	//fish_mgr.initialize(&game_config.pplXmlConfig);
 //#endif
    // set the special fish produce data manager content value.
    m_TimerLoopThread=m_pITableFrame->GetLoopThread();
	fish_mgr.setFishProduceDataManager(m_fishProduceDataManager);
    m_pGameRoomInfo=m_pITableFrame->GetGameRoomInfo();
	if (!LoadConfig())
	{
        // initialize the special config data from specail xml and set the config id.
        LOG(ERROR) << "配置资源解析失败，请检查, 配置ID:" << m_pITableFrame->GetGameRoomInfo()->roomId;
		return false;
	}
    ReadConfigInformation();
    // try to push the special list content.LOG_ERROR
	tableFrameList.push_back(m_pITableFrame);
	if (0 == instanceCount++) {
		FishProduceCommon::initializeInstance();
        UserPrefs::Instance().setConfigID(m_pITableFrame->GetGameRoomInfo()->roomId);
        // try to initialize the speical user prefs content value now.
        // m_userPrefs = new UserPrefs(m_pGameRoomInfo->nConfigId);
		TRACE_LOG_BEGIN;
#ifdef __USE_LIFE2__
		m_traceArithm = new ArithmLog();
#endif
        // initialize file name.
		TCHAR filename[MAX_PATH];
        wsprintf(filename, _T("fish_server_%d"), m_pITableFrame->GetGameRoomInfo()->roomId);
		TRACE_INFO_BEGIN(filename);
	}
#ifdef AICS_ENGINE
 	//m_i_log_db_engine = GetLogdbEngine();
	// try to initialize the special aics engine item now.
	// add by James. 初始化.
	LONG nStatus=-1;
    static BOOL bInitialized = FALSE;
	if (!bInitialized)
	{
		bInitialized = TRUE;
		GameSettings set={0};
		set.nCurrToubiBiLi = game_config.ticketRatio;
        set.nMaxPlayer = MAX_PLAYERS;
        set.nMaxTable  = m_pITableFrame->GetGameRoomInfo()->tableCount;
		/// try to trace the special game setting info for later user now the special game setting info for later content user. 
 		//LOG(INFO) << "toubiBiLi:[%d] tablecount:[%d] maxplayer:[%d]"),set.nCurrToubiBiLi,set.nMaxTable,set.nMaxPlayer);
		// try to initialize special engine.
		nStatus = GetAicsEngine()->Init(&set);
		if (S_OK !=nStatus) {
            LOG(INFO) << "GetAicsEngine()->Init failed!:[%x]"<<nStatus;
			return false;
		}
        /*
        // try to select special room id now value content data value item.
        nStatus=GetAicsEngine()->SetRoomID(game_service_option_->wServerID);
        */
        // try to update the kind id content item value.
        int nKindID = m_pITableFrame->GetGameRoomInfo()->roomId;
        if (aics_kindid) {
            nKindID = aics_kindid;
        }
		// try to select special room id now value.
		nStatus=GetAicsEngine()->SetRoomID(nKindID);
		if (S_OK != nStatus) {
            LOG(INFO) << "GetAicsEngine()->SetRoomID failed!:[%x]"<<nStatus;
            return false;
		}
 		//if (m_i_log_db_engine && S_OK != m_i_log_db_engine->Init())
 		//{
 		//	m_i_log_db_engine = NULL;
 		//}
	}
#endif
    //set the timer thread
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        bulletlevel[i]=game_config.cannonPowerMin;
    }
    m_replay.clear();
    m_replay.cellscore=game_config.cannonPowerMin;
    m_replay.roomname=m_pITableFrame->GetGameRoomInfo()->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	return true;
}

void CTableFrameSink::OnTimerSwitchFish()
{
	isGroupScene = true;
	sceneTimeLeft = GetTickCount();
	OnTimerSwitchScene(0, m_send_buffer_for_timer_thread, CMD_BUFFER_SIZE_MAX);
	m_isNeedExtendSwitchScene = false;
}
void CTableFrameSink::OnTimerProduceFish()
{
#ifdef __MULTI_THREAD__
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__
    m_fishProduceDataManager->updateWithEndTick(GetTickCount());
    scene_mgr.Update();
    m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
    if (scene_mgr.IsDone())
    {
        // modify by James 190104-switch  only do once.
        m_is_nestScene=true;
        OnTimerSceneEnd(0, m_send_buffer_for_timer_thread, CMD_BUFFER_SIZE_MAX);
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdSCENE_SWITCH);
        m_TimerIdSCENE_SWITCH = m_TimerLoopThread->getLoop()->runAfter(wSwitchSceneTimerElasped/1000,boost::bind(&CTableFrameSink::OnTimerSwitchFish, this));
		m_startSwitchSceneTime = muduo::Timestamp::now(); //切换场景的开始时间		
    }
    else
	{
		//m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
        m_TimeridFISH_PRODUCE = m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerElasped/1000,boost::bind(&CTableFrameSink::OnTimerProduceFish, this));
        // try to call the special fish produce fish content item data for later user item content value.
        OnTimerFishProduce(GAME_TIMER_FISH_PRODUCE, m_send_buffer_for_timer_thread, CMD_BUFFER_SIZE_MAX);		
    }
}
void CTableFrameSink::OnTimerFishArray()
{
    isGroupScene=false;// jie su changjing
    deathfishs.clear();
    //sceneTimeLeft=GetTickCount();
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdFISH_ARRAY);
    m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
    m_TimeridFISH_PRODUCE = m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerElasped/1000.0,boost::bind(&CTableFrameSink::OnTimerProduceFish, this));
}
void CTableFrameSink::OnTimerPauseBomb()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdPauseBoom);
    OnTimerPauseBoom(0, m_send_buffer_for_timer_thread, CMD_BUFFER_SIZE_MAX);
    LOG(INFO) << "CTableFrameSink::OnTimerMessage OnTimerPauseBoom";
}
void CTableFrameSink::OnTimerBgData()
{
#ifdef  AICS_ENGINE
    GetAicsEngine()->OnSyncData(FALSE);
    LOG(INFO) << "CTableFrameSink::OnTimerMessage GetAicsEngine()->OnSyncData";
#endif//AICS_ENGINE
}

void CTableFrameSink::OnTimerClearNotOperateUser()
{
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        shared_ptr<CServerUserItem>  pUserItem = m_pITableFrame->GetTableUserItem(i);
        if(!pUserItem)
        {
            continue;
        }
        notOperate[i]+=1; //统计玩家是不是超时不操作
        if(notOperate[i] >= game_config.timeOutQuitCheck + 30)
        {
            notOperate[i]=0;
            pUserItem->SetUserStatus(sGetoutAtplaying);
            KickOutUser(pUserItem->GetChairId());
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);
            m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_LONGTIME_NOOP);
        }
    }
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdClearNotOperateUser);
    m_TimerIdClearNotOperateUser= m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerClearNotOperateUserElasped/1000,boost::bind(&CTableFrameSink::OnTimerClearNotOperateUser, this));
}
void CTableFrameSink::OnTimerClearUser()
{
    shared_ptr<IServerUserItem> pUserItem;
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        pUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pUserItem)
        {
            if(m_pITableFrame->GetGameRoomInfo()->serverStatus !=SERVER_RUNNING|| pUserItem->GetUserStatus() == sOffline)
            {
                LOG(ERROR)<<"ting fu ti wanjia *************************************id="<<pUserItem->GetUserId()<<"    GetUserStatus() ="<<pUserItem->GetUserStatus();
                pUserItem->SetUserStatus(sOffline);
                KickOutUser(pUserItem->GetChairId());
                m_pITableFrame->ClearTableUser(i,true,true);
            }
        }
    }
    ReadConfigInformation();
}
void CTableFrameSink::OnTimerSynchor()
{
    //m_pITableFrame->KillGameTimer(kSynchorScoreTimer);----
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdSynchorScore);	
    LOG(INFO) << " >>> OnTimerMessage GAME_TIMER_SYNCHOR_SCORE done, Produce fish again!";	
	bool bHaveSpecialFire = false;
	for (WORD i = 0; i < MAX_PLAYERS; ++i)
	{
		shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(i);
		if (user_item == NULL)
			continue;
		if (m_bHaveLaser[i] || m_bHaveFireLaser[i] || m_bHaveBorebit[i] || m_bHaveFireBorebit[i]) //获得了激光炮、钻头炮
		{
			LOG(INFO) << ">>> 获得了特殊炮,暂不写分，发射结束再写  >>> UserId : " << user_item->GetUserId() << " :ChairId: " << user_item->GetChairId();
			bHaveSpecialFire = true;
			break;
		}
	}
	if (!bHaveSpecialFire) //没获得激光炮、钻头炮
	{
        OnTimerSynchorScore();
        m_TimerIdSynchorScore = m_TimerLoopThread->getLoop()->runEvery(30, boost::bind(&CTableFrameSink::OnTimerSynchor, this));
	}
}
void CTableFrameSink::OnTimerFireLaser(WORD chair_id)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireLaser);
	LOG(INFO) << " >>> OnTimerMessage OnTimerFireLaser end !";
	if (m_bHaveFireLaser[chair_id])
	{
		LOG(WARNING) << " ****** CTableFrameSink::OnTimerFireLaser. AddHitRecord 53 " << " fireScore:" << m_fireScore[chair_id] << " AllWinScore:" << m_fireAllWinScore[chair_id] << " Recordodds:" << m_fireAllWinScore[chair_id] / m_fireScore[chair_id];
		playerLog[chair_id].AddHitRecord(m_fireScore[chair_id], 53, m_fireAllWinScore[chair_id] / m_fireScore[chair_id], m_fireAllWinScore[chair_id], m_bCatchSpecialFireInGlobalBomb[chair_id]);
		WORD tableId = m_pITableFrame->GetTableId();
		onFishCatched(tableId, chair_id, m_fireAllWinScore[chair_id]);
		//m_fireScore[chair_id] = 0;
		m_fireAllWinScore[chair_id] = 0;
		m_bCatchSpecialFireInGlobalBomb[chair_id] = false;
	}
	m_bHaveFireLaser[chair_id] = false;

	CMD_S_SpecialFireEnd cmdData = { 0 };
	cmdData.hitType = 1;
	cmdData.playerID = chair_id;

	unsigned short dataSize = NetworkUtility::encodeSpecialFireEnd(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_SPECIALFIREEND, m_send_buffer, dataSize, true);
	m_fishProduceDataManager->setbProduceSpecialFire(true);

	m_TimerLoopThread->getLoop()->cancel(m_TimerIdSynchorScore);
	shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(chair_id);
	if (user_item != NULL)
	{
		LOG(INFO) << ">>> 获得了特殊炮,发射结束,同步分数  >>> UserId : " << user_item->GetUserId() << " :ChairId: " << user_item->GetChairId();		
	}
    //OnTimerSynchorScore();
    m_TimerIdSynchorScore = m_TimerLoopThread->getLoop()->runEvery(30, boost::bind(&CTableFrameSink::OnTimerSynchor, this));
}

void CTableFrameSink::OnTimerBorebitStartRotate(WORD chair_id)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimeridBorebitStartRotate);
	LOG(INFO) << " >>> OnTimerMessage OnTimerBorebitStartRotate end!";
	if (m_bBorebitStartRotate || !m_bHaveFireBorebit[chair_id])
	{
		return;
	}
	m_bBorebitStartRotate = true;
	double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime); //经过了多少时间
	m_nBorebitFireAllTime = cbtimeleave + 2.0f; //计算飞行的总时间

	LOG(WARNING) << " >>> OnTimerMessage OnTimerBorebitStartRotate allTime = " << m_nBorebitFireAllTime;

	CMD_S_StartRotate cmdData = { 0 };
	cmdData.bStartRotate = m_bBorebitStartRotate;
	cmdData.chairid = chair_id;

	unsigned short dataSize = NetworkUtility::encodeBorebitStartRotate(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_ZTPSTARTROTATE, m_send_buffer, dataSize, true);

	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebit);
	m_TimeridFireBorebit = m_TimerLoopThread->getLoop()->runAfter(2.0, boost::bind(&CTableFrameSink::OnTimerFireBorebit, this, chair_id));

	if (m_isNeedExtendSwitchScene)
	{
		double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startExtendSwitchSceneTime); //经过了多少时间
		DWORD beLeftSwitchSceneTime = m_dwSceneLessTime - cbtimeleave*1000;
		LOG(WARNING) << " >>> OnTimerMessage OnTimerBorebitStartRotate 离切换场景 cbtimebeleft = " << beLeftSwitchSceneTime/1000;
		int proTime = 5;
		if (beLeftSwitchSceneTime > proTime * 1000 ) //离切换场景大于
		{
			DWORD addSwitchSceneTime = 0;
			if (beLeftSwitchSceneTime <= m_dwNeedAddTick)
			{
				addSwitchSceneTime = m_dwNeedAddTick - beLeftSwitchSceneTime;
				addSwitchSceneTime += 5 * 1000;
				scene_mgr.SetAddTick(addSwitchSceneTime);
				LOG(WARNING) << " >>> OnTimerMessage OnTimerBorebitStartRotate 延长切换场景 A m_dwSceneLessTime = " << m_dwSceneLessTime << " cbtimebeleft = " << addSwitchSceneTime;
			}
			else
			{
				addSwitchSceneTime += 5 * 1000;
				scene_mgr.SetAddTick(addSwitchSceneTime);
				LOG(WARNING) << " >>> OnTimerMessage OnTimerBorebitStartRotate 延长切换场景 B m_dwSceneLessTime = " << m_dwSceneLessTime << " cbtimebeleft = " << addSwitchSceneTime;
			}
		}
	}

}

void CTableFrameSink::OnTimerFireBorebit(WORD chair_id)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebit);
	LOG(INFO) << " >>> OnTimerMessage OnTimerFireBorebit end!";

	CMD_S_SpecialFireEnd cmdData = { 0 };
	cmdData.hitType = 2;
	cmdData.playerID = chair_id;

	unsigned short dataSize = NetworkUtility::encodeSpecialFireEnd(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_SPECIALFIREEND, m_send_buffer, dataSize, true);

	m_bBorebitFireBoom = true;
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebitEnd); //爆炸结束再清标志
	m_TimeridFireBorebitEnd = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::OnTimerFireBorebitEnd, this, chair_id));
	
}
void CTableFrameSink::OnTimerCountDownFire(WORD chair_id)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
	LOG(INFO) << " >>> OnTimerMessage OnTimerCountDownFire end!";
	if (!m_bHaveLaser[chair_id] && !m_bHaveBorebit[chair_id])
		return;

	CMD_C_UserFire user_fire = { 0 };
	if (m_bHaveLaser[chair_id])
	{
		user_fire.fireType = 1;
	} 
	else if (m_bHaveBorebit[chair_id])
	{
		user_fire.fireType = 2;
	}
	
	CMD_S_UserFire cmdData = { 0 };
	cmdData.c = user_fire;
	cmdData.playerID = chair_id;
	
	unsigned short dataSize = NetworkUtility::encodeUserFire(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	if (!dataSize) {
		LOG(ERROR) << "encodeUserFire failed!";
	}
	send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_USER_FIRE, m_send_buffer, dataSize, true);
	//AddFishScoreForPlayer(chair_id, sutBulletScore, -cmdData.c.cannonPower);
	if (user_fire.fireType == 1 && !m_bHaveFireLaser[chair_id]) //激光炮
	{
		LOG(WARNING) << "CTableFrameSink::OnTimerCountDownFire. fireType:" << user_fire.fireType;
		m_startFireTime = muduo::Timestamp::now(); //发射钻头炮的开始时间
		m_startFireAngle = user_fire.angle;
		m_bHaveLaser[chair_id] = false;
		m_bHaveFireLaser[chair_id] = true;		
		m_TimerLoopThread->getLoop()->cancel(m_TimeridFireLaser);
		m_TimeridFireLaser = m_TimerLoopThread->getLoop()->runAfter(5.0, boost::bind(&CTableFrameSink::OnTimerFireLaser, this, chair_id));
	}
	else if (user_fire.fireType == 2 && !m_bHaveFireBorebit[chair_id]) //钻头炮
	{
		LOG(WARNING) << "CTableFrameSink::OnTimerCountDownFire. fireType:" << user_fire.fireType;
		m_startFireTime = muduo::Timestamp::now(); //发射钻头炮的开始时间
		m_startFireAngle = user_fire.angle;
		m_bHaveBorebit[chair_id] = false;
		m_bHaveFireBorebit[chair_id] = true;
		m_TimerLoopThread->getLoop()->cancel(m_TimeridBorebitStartRotate);
		m_TimeridBorebitStartRotate = m_TimerLoopThread->getLoop()->runAfter(m_nBorebitMaxFireTime*1.0f - 2.0, boost::bind(&CTableFrameSink::OnTimerBorebitStartRotate, this, chair_id));
	}

}

void CTableFrameSink::OnTimerFireBorebitEnd(WORD chair_id)
{
	m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebitEnd);
	LOG(INFO) << " >>> OnTimerMessage m_TimeridFireBorebitEnd end!";
	if (m_bHaveFireBorebit[chair_id])
	{
		LOG(WARNING) << " ****** CTableFrameSink::OnTimerFireBorebitEnd. AddHitRecord 54 " << " fireScore:" << m_fireScore[chair_id] << " AllWinScore:" << m_fireAllWinScore[chair_id] << " Recordodds:" << m_fireAllWinScore[chair_id] / m_fireScore[chair_id];
		playerLog[chair_id].AddHitRecord(m_fireScore[chair_id], 54, m_fireAllWinScore[chair_id] / m_fireScore[chair_id], m_fireAllWinScore[chair_id], m_bCatchSpecialFireInGlobalBomb[chair_id]);
		WORD tableId = m_pITableFrame->GetTableId();
		onFishCatched(tableId, chair_id, m_fireAllWinScore[chair_id]);
		m_fireAllWinScore[chair_id] = 0;
		m_bCatchSpecialFireInGlobalBomb[chair_id] = false;
	}
	m_bHaveFireBorebit[chair_id] = false;
	m_bBorebitStartRotate = false;
	m_fishProduceDataManager->setbProduceSpecialFire(true);

	m_TimerLoopThread->getLoop()->cancel(m_TimerIdSynchorScore);
	shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(chair_id);
	if (user_item != NULL)
	{
		LOG(INFO) << ">>> 获得了特殊炮,发射结束,同步分数  >>> UserId : " << user_item->GetUserId() << " :ChairId: " << user_item->GetChairId();
	}
    //OnTimerSynchorScore();
    m_TimerIdSynchorScore = m_TimerLoopThread->getLoop()->runEvery(30, boost::bind(&CTableFrameSink::OnTimerSynchor, this));
}

bool CTableFrameSink::OnEventGameStart()
{
 	//debug("CTableFrameSink::OnEventGameStart.");
	return true;
}

// 解散游戏,reason=原因.
// bool CTableFrameSink::OnEventGameConclude(WORD chair_id, IServerUserItem* pIServerUserItem, BYTE reason)
bool CTableFrameSink::OnEventGameConclude( uint32_t chair_id, uint8_t nEndTag)
{
    word table_id = m_pITableFrame->GetTableId();
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude table_id:" << table_id  << ", chair_id:" << chair_id << ", reason:" << nEndTag;
    if (nEndTag == GER_DISMISS)
	{
        for (WORD i = 0; i < MAX_PLAYERS; ++i)
        {
            m_pServerUserItemForGamescene[i] = NULL;

        }
        shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(chair_id);
        if (user_item == NULL) return false;
        CalcScore(user_item);
		m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
		killAllGameTimer();

#ifdef __MULTI_THREAD__
		CCriticalSectionEx lock;
		lock.Lock(&m_cs);
#endif//__MULTI_THREAD__
		OnGameFree();
#ifdef __DYNAMIC_CHAIR__
		resetDynamicChair();
#endif//__DYNAMIC_CHAIR__
        LOG(INFO) << "CTableFrameSink::OnEventGameConclude End[GER_DISMISS].";
	}
    else if (chair_id < MAX_PLAYERS)
	{
        shared_ptr<CServerUserItem>  pIServerUserItem = m_pITableFrame->GetTableUserItem(chair_id);
        if (!pIServerUserItem) {
            LOG(ERROR) << "OnEventGameConclude GetTableUserItem is null!";
            return false;
        }
		CalcScore(pIServerUserItem);
 		//m_debugTickQuit[chair_id] = 0;
		m_pServerUserItemForGamescene[chair_id] = NULL;
 		//m_bulletCount[chair_id] = 0;
 		//memset(m_bullets[chair_id], 0, sizeof(m_bullets[chair_id]));
        int chair_count = MAX_PLAYERS;
		int player_count = 0;
		for (int i = 0; i < chair_count; ++i)
		{
			if ((m_pITableFrame->GetTableUserItem(i)) && i != chair_id)
			{
				++player_count;
				break;
			}
		}
        // is first user enter.
		if (player_count == 0)
		{
			m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
			killAllGameTimer();

#ifdef __MULTI_THREAD__
			CCriticalSectionEx lock;
			lock.Lock(&m_cs);
#endif
            // game free.
			OnGameFree();

#ifdef __DYNAMIC_CHAIR__
			resetDynamicChair();
#endif
            LOG(INFO) << "CTableFrameSink::OnEventGameConclude End[GAME_STATUS_FREE].";
		}
		else
		{
#ifdef __GAMEPLAY_PAIPAILE__
			if (game_config.pplData[chair_id].isPlaying)
            {
				resetPplData(chair_id);
				DWORD score = 0;
				sendGameplayData(chair_id, INVALID_CHAIR, gptPpl, gctEnd, chair_id, &score, sizeof(score));
			}
#endif
		}
	}
 	//debug("CTableFrameSink::OnEventGameConclude end.[table_id=%d chair_id=%d]", m_pITableFrame->GetTableID(), chair_id);
    return true;
}

// 发送场景消息.
//bool CTableFrameSink::(WOnEventSendGameSceneORD chair_id, IServerUserItem* pIServerUserItem, BYTE game_status, bool send_secret)
bool CTableFrameSink::OnEventGameScene(DWORD chair_id, bool bisLookon)
{
    BYTE game_status = GAME_STATUS_START; // modify by James 180722.
    word table_id = m_pITableFrame->GetTableId();
    LOG(INFO) << "CTableFrameSink::OnEventSendGameScene start. table_id:" << table_id << ", chair_id:" << chair_id << ", game_status:" << game_status;

	bool ret = false;
    std::shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chair_id);
    if (!pIServerUserItem) {
        LOG(ERROR) << "OnEventGameScene GetTableUserItem failed,chair id:" << chair_id;
        return false;
    }
    switch (game_status) // modify by James.
	{
    case GAME_STATUS_FREE:
    case GAME_STATUS_START:
 		//if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
 		//{
 		//	LOG(INFO) << "GameCheatUser.[chair_id=%d]"), pIServerUserItem->GetChairID());
 		//	return false;
 		//}
        // 因为加了锁，不能写在一起
        if (!game_started) {
            killAllGameTimer();
        }

#ifdef __MULTI_THREAD__
		CCriticalSectionEx lock;
		lock.Lock(&m_cs);
#endif
        // 因为加了锁，不能写在一起
        if (!game_started) {
            OnGameStart();
        }
        // check if switch scene.
		if (m_is_switching_scene)
		{
            LOG(INFO) << "CTableFrameSink::OnEventSendGameScene. scene time's up.";
			m_pServerUserItemForGamescene[chair_id] = pIServerUserItem;
            //m_pITableFrame->SendGameMessage(chair_id, _T("正在同步游戏场景，请稍候..."), SMT_CUSTOM_WAITING_START);
			ret = true;
		}
		else
		{
            // 发送场景消息给客户端.
            ret =  SendGameScene(pIServerUserItem);
			playerScore[chair_id] = game_config.fishScore[chair_id];
            //OnSendInfoToOther(pIServerUserItem);
		}
		break;
	}
//Cleanup:
    if (!ret) {
        LOG(WARNING) << "CTableFrameSink::OnEventSendGameScene end. table_id=:" << table_id << ", chair_id: " << chair_id << ", game_status:" << game_status << ", ret:" << ret;
    }
    return true;
}

// 定时器消息.
bool CTableFrameSink::OnTimerMessage(DWORD dwTimerID, WPARAM dwBindParameter)
{
 	return true;
}

bool CTableFrameSink::OnTimerPauseBoom(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize)
{
    LOG(INFO) << "CTableFrameSink::OnTimerPauseBoom[end egttTimerPause]";
    CMD_S_PauseFish cmdData = {0};
	unsigned short dataSize = NetworkUtility::encodePauseFish(buffer, bufferSize, cmdData);
	send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_FISH_PAUSE, buffer, dataSize, true);
	m_fishProduceDataManager->resume(GetTickCount());
 	//debug("CTableFrameSink::OnTimerPauseBoom. table_id=%d", m_pITableFrame->GetTableID());
	return true;
}

bool CTableFrameSink::OnGameMessage( uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize)
{
#ifdef __MULTI_THREAD__
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__

    // LOG(INFO) << " >>> CTableFrameSink::OnGameMesasge subid:" << (int)subid << ", chairid:" << (int)chairid;
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairid);
    if ((!pIServerUserItem) || (subid == 0)) {
        LOG(ERROR) << " >>> CTableFrameSink::OnGameMesasge error, pIServerUserItem: " << pIServerUserItem << ", subid:" << (int)subid << ", chairid:" << (int)chairid;
    }
	//if (pIServerUserItem->GetUserStatus() == US_LOOKON) return true; // modify by James.
 	//LOG(INFO) << "CTableFrameSink::OnGameMessage start. table_id=%d chair_id=%d sub_cmdid=%d data_size=%d"), m_pITableFrame->GetTableID(), pIServerUserItem->GetChairID(), sub_cmdid, data_size);
 	//debug("CTableFrameSink::OnGameMessage start.[table_id=%d chair_id=%d[%d]]", m_pITableFrame->GetTableID(), pIServerUserItem->GetChairID(), pIServerUserItem);
	bool ret = false;
    unsigned char* _data = (unsigned char*)data;
    switch (subid)
	{
     //case SUB_C_SCORE_UP:
     //   {
     //       CMD_C_ScoreUp cmdData = {0};
     //       if (ret = NetworkUtility::decodeScoreUp(cmdData, _data, data_size)) {
     //           ret = OnSubExchangeFishScore(pIServerUserItem, cmdData);
     //       }
     //    assert(ret == true);
     //   }
     //   break;
#ifdef __DYNAMIC_CHAIR__
    case SUB_C_DYNAMIC_CHAIR:
		{
			CMD_C_DynamicChair cmdData = {0};
            if (ret = NetworkUtility::decodeDynamicChair(cmdData, _data, datasize)) {
				ret = OnSubDynamicChair(pIServerUserItem, cmdData);
			}
		}
		break;
#endif
    case SUB_C_EXCHANGE_SCORE:
        {
            CMD_C_Exchange_Score exchangeScore = {0};
            if (ret = NetworkUtility::decodeExchangeScore(exchangeScore, _data, datasize)) {
                ret = OnSubExchangeFishScore(pIServerUserItem, true);
            }else
            {
                LOG(WARNING) << "SUB_C_EXCHANGE_SCORE decode failed!";
            }
        }
        break;
	case SUB_C_USER_FIRE:
		{
			CMD_C_UserFire cmdData = {0};
            if (ret = NetworkUtility::decodeUserFire(cmdData, _data, datasize))
            {
                notOperate[pIServerUserItem->GetChairId()]=0;//玩家不操作踢出控制
				ret = OnSubUserFire(pIServerUserItem, cmdData);
            }
            else
            {
                LOG(WARNING) << "SUB_C_USER_FIRE decodeUserFire failed!";
            }
		}
		break;
	case SUB_C_FISH_HIT:
		{
			CMD_C_HitFish cmdData = {0};
			unsigned short fishCount = 0;
            if (ret = NetworkUtility::decodeHitFish(cmdData, m_fishIDs, fishCount, MAX_FISH_ID, _data, datasize))
            {
                notOperate[pIServerUserItem->GetChairId()]=0;//玩家不操作踢出控制
                // LOG(INFO) << " >>> decodeHitFish fish id:" << (int)m_fishIDs[0];
                ret = OnSubHitFish(pIServerUserItem, cmdData, fishCount);
			}
            else
            {
                LOG(WARNING) << "SUB_C_FISH_HIT decodeHitFish failed!";
            }
		}
		break;
	case SUB_C_FISH_LOCK:
		if (!game_config.fishLockable)
        {
            // 当前鱼不能被锁定.
			ret = true;
		}
        else
        {
			CMD_C_LockFish cmdData = {0};
            if (ret = NetworkUtility::decodeLockFish(cmdData, _data, datasize)) {
				ret = OnSubLockFish(pIServerUserItem, cmdData);
			}
		}
		break;
	case SUB_C_FISH_UNLOCK: //解锁.
        {
		    ret = OnSubUnlockFish(pIServerUserItem);
        }
		break;
	case SUB_C_FROZEN:      //冰冻.
        {
		    ret = OnSubFrozenFish(pIServerUserItem);
        }
		break;
	case SUB_C_GAMEPLAY:    //玩法.
		{
            WORD size = 0;
			CMD_C_Gameplay cmdData = {0};
            if (ret = NetworkUtility::decodeGameplay(cmdData, _data, datasize, &size))
            {
                ret = OnSubGameplay(pIServerUserItem, cmdData, _data + size, datasize - size);
 				//LOG(INFO) << "CTableFrameSink::OnGameMessage End[SUB_C_GAMEPLAY%d]."), ret);
			}
            else
            {
                LOG(WARNING) << "SUB_C_GAMEPLAY decodeGameplay failed!";
            }
		}
		break;
    case SUB_C_ADDLEVEL:    //玩法.
        {
            WORD size = 0;
            CMD_C_AddLevel cmdData = {0};
            if (ret = NetworkUtility::decodeAddLevel(cmdData, _data, datasize, &size))
            {
                ret = OnSubAddLevel(pIServerUserItem, cmdData);
 				//LOG(INFO) << "CTableFrameSink::OnGameMessage End[SUB_C_GAMEPLAY%d]."), ret);
            }
            else
            {
                LOG(WARNING) << "SUB_C_GAMEPLAY decodeGameplay failed!";
            }
        }
        break;
	case SUB_C_SETSPECIALROTATE:
		{
			CMD_C_SpecialRotate cmdData = { 0 };
			if (ret = NetworkUtility::decodeSpecialRotate(cmdData, _data, datasize))
			{
				ret = OnSubSpecialRotate(pIServerUserItem, cmdData);
			}
			else
			{
				LOG(WARNING) << "SUB_C_SETSPECIALROTATE decodeUserFire failed!";
			}
		}
		break;
	}
    if (!ret) {
        LOG(ERROR) << "OnGameMessage ret failed,cmdid:" << (int)subid;
    }
	//debug("CTableFrameSink::OnGameMessage end.[table_id=%d chair_id=%d]", m_pITableFrame->GetTableID(), pIServerUserItem->GetChairID());
 	//LOG(INFO) << "CTableFrameSink::OnGameMessage end[%d].[table_id=%d chair_id=%d]"), ret, m_pITableFrame->GetTableID(), pIServerUserItem->GetChairID());
	return ret;
}

//bool CTableFrameSink::OnActionUserSitDown(WORD chair_id, IServerUserItem* pIServerUserItem, bool lookon_user)
bool CTableFrameSink::OnUserEnter(int64_t userid, bool islookon)
{
    std::shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userid);
    if (!pIServerUserItem) {
        LOG(ERROR) << " >>> OnUserEnter get pIServerUserItem Error! userid:" << userid;
        return false;
    }    
    // try to get the special user chair id content.
    WORD table_id = m_pITableFrame->GetTableId();
    WORD chair_id = pIServerUserItem->GetChairId();
	notOperate[chair_id] = 0;
    if(!isNotOutRoom[chair_id])
    {
        strRoundID[chair_id]="";
        strRoundID[chair_id]=m_pITableFrame->GetNewRoundId();
        do
        {
            bool exis=false;
			for(int i=0;i<4;i++)
			{
				if(i==chair_id) continue;
				if(strRoundID[chair_id].compare(strRoundID[i])==0)
				{
					exis=true;

				}
			}
			if(!exis) {
				OldstrRoundID[chair_id]=strRoundID[chair_id];
				break;
			}
			strRoundID[chair_id]=m_pITableFrame->GetNewRoundId();
        }while(true);

        LOG(ERROR) << "CTableFrameSink::OnActionUserSitDown start. table_id:" << table_id << ", chair_id:" << chair_id << ", user_id:" << userid;
        m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
        if (!islookon)
        {
     		/*game_config.exchange_fish_score_[chair_id] = 0;
     		game_config.fish_score_[chair_id] = 0;*/
            m_bulletCount[chair_id] = 0;
            memset(m_bullets[chair_id], 0, sizeof(m_bullets[chair_id]));
            if (game_config.isExperienceRoom && !m_exp_user_score[chair_id])
            {
                // 体验场自动上分.
                m_exp_user_score[chair_id] = exchangeAmountForExperienceRoom;
            }
			/*if (game_config.is_experience_room) {
				writeLog("Player[%u] was entered in the experience room[%d] table[%d]. The balance is %I64d.",
					pIServerUserItem->GetUserID(), game_service_option_->wServerID, m_pITableFrame->GetTableID(), (long long)pIServerUserItem->GetUserScore());
			}
			else {
				writeLog("Player[%u] was entered in the room[%d] table[%d]. The balance is %I64d.",
					pIServerUserItem->GetUserID(), game_service_option_->wServerID, m_pITableFrame->GetTableID(), (long long)pIServerUserItem->GetUserScore());
			}*/

    #ifdef __DYNAMIC_CHAIR__
            CMD_S_DynamicChair cmdData = {0};
            cmdData.newChairID = genDynamicChairID(chair_id);
            cmdData.playerID = chair_id;
            unsigned short dataSize = NetworkUtility::encodeDynamicChair(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
            send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_DYNAMIC_CHAIR, m_send_buffer, dataSize, true, chair_id);
    #endif
            if ( !game_config.isExperienceRoom)
            {
                UserPrefs::Instance().onUserSitdown(pIServerUserItem,
                                           m_pITableFrame->GetGameRoomInfo()->roomId,
                                           game_config.ticketRatio,
                                           (double)game_config.exchangeRatioUserScore / game_config.exchangeRatioFishScore,
                                           getXimaliangForNewOldUser(),
                                           getWinCoinForNewOldUser(),
                                           m_beginUserID);
            }

    #ifdef AICS_ENGINE
    #ifdef __DYNAMIC_CHAIR__

    #else
    //        LOG(INFO) << "玩家昵称=%s,玩家ID=%d,房间号=%d)玩家坐下!!!"),
    //                       pIServerUserItem->GetNickName(),
    //                       pIServerUserItem->GetUserID(),
    //                       game_service_option_->wServerID);

            // call the AicsEngine for user sit down action for later.
            GetAicsEngine()->OnUserSitdown(pIServerUserItem->GetUserId(),
                                           m_pITableFrame->GetGameRoomInfo()->roomId,
                                           chair_id, FALSE, FALSE);
    #endif
    #endif
    #ifdef __GAMEPLAY_PAIPAILE__
            // 重置拍拍乐玩法数据.
            resetPplData(chair_id);
    #endif//__GAMEPLAY_PAIPAILE__
        }
        LOG(INFO) << "CTableFrameSink::OnActionUserSitDown end.";
        //debug("CTableFrameSink::OnActionUserSitDown end.[table_id=%d chair_id=%d]", m_pITableFrame->GetTableID(), chair_id);
        // start game.
        if (m_pITableFrame->GetGameStatus() == GAME_STATUS_FREE)
        {
            m_pITableFrame->SetGameStatus(GAME_STATUS_START);
     		//debug("CTableFrameSink::OnActionUserSitDown m_pITableFrame->StartGame.");
        }
        bulletlevel[chair_id]=game_config.cannonPowerMin;
        OnEventGameScene(chair_id,  islookon);
        for(int i=0;i<MAX_PLAYERS;i++)
        {
            if(m_pITableFrame->GetTableUserItem(i))
            {
                CMD_C_AddLevel cmdData = {0};
                cmdData.userid=i;
                cmdData.powerlevel=bulletlevel[i];
                if(cmdData.powerlevel>=game_config.cannonPowerMin&&cmdData.powerlevel<=game_config.cannonPowerMax)
                {
                    int bytesize = 0;
                    char buffer[1024]={0};
                    ::rycsfish::CMD_S_AddLevel addcanno;
                    addcanno.set_cannonpower(cmdData.powerlevel);
                    addcanno.set_playerid(i);
                    bytesize = addcanno.ByteSize();
                    addcanno.SerializeToArray(buffer,bytesize);
                    send_table_data(chair_id, chair_id, SUB_S_ADDLEVEL, buffer, bytesize, true);
                }
            }
        }
    }
    else// player not out
    {
        if (!m_is_switching_scene)
        {
			//send scene
			// 子弹积分返还.
			doReturnBulletScore(pIServerUserItem->GetChairId());
			// 构建场景消息.
			CMD_S_GameScene cmdData = {0};
			cmdData.pause      = m_fishProduceDataManager->getPause();      // 当前是否暂停.
			cmdData.gameConfig = game_config;
			if(m_is_nestScene)
			{
				cmdData.sceneID    =scene_mgr.GetNextSceneId();
			}
			else
			{
				cmdData.sceneID    = scene_mgr.GetCurSceneId();
			}
			if(isGroupScene)
			{
				for(int i=0;i<deathfishs.size();i++)
				{
					cmdData.fishLiveArray.push_back(deathfishs[i]);
				}
				cmdData.sceneTime=(GetTickCount()-sceneTimeLeft)/1000.0f;
			}
			else
			{
				cmdData.sceneTime=0;
			}
			WORD dHaveSpecailFireChairID = 0;
			for (int i = 0;i < MAX_PLAYERS;i++)
			{
				if (m_pITableFrame->GetTableUserItem(i))
				{
					if (m_bHaveLaser[i] || m_bHaveFireLaser[i] || m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
					{
						dHaveSpecailFireChairID = i;
						break;
					}
				}
			}
			//set player score
			// 玩法参数配置.
			for(int i=0;i<MAX_PLAYERS;i++)
			{
				if(m_pITableFrame->GetTableUserItem(i))
				{
					cmdData.players[i].nikename=m_pITableFrame->GetTableUserItem(i)->GetNickName();
					cmdData.players[i].score=game_config.fishScore[i];
					cmdData.players[i].userid=m_pITableFrame->GetTableUserItem(i)->GetUserId();
					cmdData.players[i].chairid=i;
					cmdData.players[i].isExist=true;
					cmdData.players[i].status=m_pITableFrame->GetTableUserItem(i)->GetUserStatus();

					if (m_bHaveLaser[i] || m_bHaveFireLaser[i])
					{
						cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
						cmdData.specialFire[i].chairid = i;
						cmdData.specialFire[i].FireType = 1;
						cmdData.specialFire[i].bHaveSpecialFire = true;
						cmdData.specialFire[i].bHaveFire = m_bHaveFireLaser[i];
						cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
						if (m_bHaveFireLaser[i])
						{
							cmdData.specialFire[i].FireAngle = m_startFireAngle;
							cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
						}
						else
						{
							int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
							cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
						}
					}
					else if (m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
					{
						cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
						cmdData.specialFire[i].chairid = i;
						cmdData.specialFire[i].FireType = 2;
						cmdData.specialFire[i].bHaveSpecialFire = true;
						cmdData.specialFire[i].bHaveFire = m_bHaveFireBorebit[i];
						cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
						if (m_bHaveFireBorebit[i])
						{
							cmdData.specialFire[i].FireAngle = m_startFireAngle;
							cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
							if (m_bBorebitStartRotate && m_nBorebitFireAllTime > cmdData.specialFire[i].FirePassTime)
							{
								cmdData.specialFire[i].FireEndCountTime = m_nBorebitFireAllTime - cmdData.specialFire[i].FirePassTime;
							}
						}
						else
						{
							int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
							cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
						}
					}
				}
				else
				{
					cmdData.players[i].isExist=false;
					cmdData.specialFire[i].bHaveSpecialFire = false;
				}
			}
			// 当前鱼群中是否包含鱼阵.
			if (m_currentFishArray&&isGroupScene)
			{
				if (m_fishProduceDataManager->contains(stSchool)) {
					cmdData.fishArray.fishArrayID = m_currentFishArray->getID();
					cmdData.fishArray.randomSeed = m_currentFishArray->getRandomSeed();
					cmdData.fishArray.firstFishID = m_currentFishArray->getFirstFishID();
				}
			}
			unsigned short dataSize = NetworkUtility::encodeGameScene(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData, m_fishProduceDataManager->getAliveFishes(),isGroupScene, m_bBorebitStartRotate);
			m_pITableFrame->SendUserData(pIServerUserItem, SUB_S_GAME_SCENE, (const uint8_t*)m_send_buffer, dataSize);
			for(int i=0;i<MAX_PLAYERS;i++)
			{
				if(m_pITableFrame->GetTableUserItem(i))
				{
					CMD_C_AddLevel cmdData = {0};
					cmdData.userid=i;
					cmdData.powerlevel=bulletlevel[i];
					if(cmdData.powerlevel>=game_config.cannonPowerMin&&cmdData.powerlevel<=game_config.cannonPowerMax)
					{
						int bytesize = 0;
						char buffer[1024]={0};
						::rycsfish::CMD_S_AddLevel addcanno;
						addcanno.set_cannonpower(cmdData.powerlevel);
						addcanno.set_playerid(i);
						bytesize = addcanno.ByteSize();
						addcanno.SerializeToArray(buffer,bytesize);
						send_table_data(chair_id, chair_id, SUB_S_ADDLEVEL, buffer, bytesize, true);
					}
				}
			}
        }
        else
        {
            m_pServerUserItemForGamescene[chair_id] = pIServerUserItem;
        }
    }
    return true;
}

void CTableFrameSink::KickOutUser(DWORD chairid)
{
    std::shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairid);
    if (pIServerUserItem)
    {
        isNotOutRoom[pIServerUserItem->GetChairId()]=false;
        // try to get the special user chair id content.
        word table_id = pIServerUserItem->GetTableId();
        word chair_id = pIServerUserItem->GetChairId();
        LOG(INFO) << "CTableFrameSink::KickOutUser table_id: " << table_id  << ", chair_id:" << chair_id ;
        // try to call special event to calc score.
        //writeUserInfo(chair_id);
        this->OnEventGameConclude(chair_id, GER_NORMAL);
        // clear the special table user if exist item now.
        //tableframe_->CanLeftTable(int64_t userId)
        DWORD dwChairID = pIServerUserItem->GetChairId();
        lostScore[pIServerUserItem->GetChairId()]=0;
        sumScore[pIServerUserItem->GetChairId()]=0;
        playerScore[pIServerUserItem->GetChairId()]=0;
        playerAddScore[pIServerUserItem->GetChairId()]=0;
    }
    else
    {
        LOG(ERROR) << "CTableFrameSink::KickOutUser Error! user not exist, userid: " ;
    }
}
// 玩家起立.
// bool CTableFrameSink::OnActionUserStandUp(WORD chair_id, IServerUserItem* pIServerUserItem, bool lookon_user)
bool CTableFrameSink::OnUserLeft(int64_t userid, bool islookon)
{
    LOG(INFO) << "CTableFrameSink::OnUserLeft user_id: " << userid << ",lookon_user:" << islookon;
    std::shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userid);
    if (pIServerUserItem)
    {
        isNotOutRoom[pIServerUserItem->GetChairId()]=false;		
        // try to get the special user chair id content.
        word table_id = pIServerUserItem->GetTableId();
        word chair_id = pIServerUserItem->GetChairId();
        LOG(INFO) << "CTableFrameSink::OnUserLeft table_id: " << table_id  << ", chair_id:" << chair_id << ",lookon_user:" << islookon;
		if (m_bHaveLaser[chair_id] || m_bHaveFireLaser[chair_id])
		{
			OnTimerCountDownFire(chair_id);
			OnTimerFireLaser(chair_id);
		}
		if (m_bHaveBorebit[chair_id] || m_bHaveFireBorebit[chair_id])
		{
			OnTimerCountDownFire(chair_id);
			OnTimerFireBorebit(chair_id);
			OnTimerFireBorebitEnd(chair_id);
		}
        // try to call special event to calc score.
        this->OnEventGameConclude(chair_id, GER_NORMAL);
        // clear the special table user if exist item now.
        //tableframe_->CanLeftTable(int64_t userId)
        DWORD dwChairID = pIServerUserItem->GetChairId();
        lostScore[pIServerUserItem->GetChairId()]=0;
        sumScore[pIServerUserItem->GetChairId()]=0;
        playerScore[pIServerUserItem->GetChairId()]=0;
        playerAddScore[pIServerUserItem->GetChairId()]=0;
        //玩家离开房间
        pIServerUserItem->SetUserStatus(sGetout);
        //玩家离开广播
        m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
        //清除玩家信息
        m_pITableFrame->ClearTableUser(pIServerUserItem->GetChairId(), true, true);
    }
    else
    {
        LOG(ERROR) << "CTableFrameSink::OnUserLeft Error! user not exist, userid: " << userid << ", lookon_user:" << islookon;
    }
    return true;
}

void CTableFrameSink::OnGameFree()
{
    LOG(INFO) << "CTableFrameSink::OnGameFree()";
#ifdef AICS_ENGINE
	GetAicsEngine()->OnSyncData(TRUE);
#endif//AICS_ENGINE
    game_started = false;
	m_is_switching_scene = false;
	memset(m_bulletCount, 0, sizeof(m_bulletCount));
	memset(m_bullets, 0, sizeof(m_bullets));
	memset(m_currentSN, 0, sizeof(m_currentSN));
 	//memset(m_debugTickQuit, 0, sizeof(m_debugTickQuit));
	fish_mgr.reset();
	m_fishProduceDataManager->reset();
	scene_mgr.Reset();
#ifdef __GAMEPLAY_PAIPAILE__
	memset(m_pplRuntimeData, 0, sizeof(m_pplRuntimeData));
	memset(game_config.pplData, 0, sizeof(game_config.pplData));
#endif
}

void CTableFrameSink::OnGameStart()
{
    LOG(INFO) << "CTableFrameSink::OnGameStart()";
	OnGameFree();
    m_startTime=chrono::system_clock::now();
    game_started = true;
	scene_mgr.Start(scene_mgr.GetNextSceneId());
	StartGameTimer();
	m_startFireTime = muduo::Timestamp::now(); //发射钻头炮的开始时间
	m_startSwitchSceneTime = muduo::Timestamp::now(); //切换场景的开始时间
}

void CTableFrameSink::StartGameTimer()
{
    //sceneTimeLeft=GetTickCount();
    LOG(INFO) << " >>> StartGameTimer(). set timer GAME_TIMER_FISH_PRODUCE.";
    m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
    m_TimeridFISH_PRODUCE = m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerElasped/1000,boost::bind(&CTableFrameSink::OnTimerProduceFish, this));
    m_TimerIdWriteBgData  = m_TimerLoopThread->getLoop()->runEvery(kWriteBgDataTimer/1000,boost::bind(&CTableFrameSink::OnTimerBgData, this));
    m_TimerIdClearUser  = m_TimerLoopThread->getLoop()->runEvery(kGlobalTimerClearuserElasped/1000,boost::bind(&CTableFrameSink::OnTimerClearUser, this));
    m_TimerIdClearNotOperateUser= m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerClearNotOperateUserElasped/1000,boost::bind(&CTableFrameSink::OnTimerClearNotOperateUser, this));
    m_TimerIdSynchorScore = m_TimerLoopThread->getLoop()->runEvery(30,boost::bind(&CTableFrameSink::OnTimerSynchor, this));
    //setGameTimer(kGlobalTimer, kGlobalTimerElasped, kRepeatTimer, GAME_TIMER_FISH_PRODUCE);
    //setGameTimer(kWriteBgDataTimer, kWriteBgDataTimerElasped, kRepeatTimer, 0);
}

void CTableFrameSink::killAllGameTimer()
{
    LOG(INFO) << "killAllGameTimer()";
    m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdSCENE_SWITCH);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdFISH_ARRAY);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdPauseBoom);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdWriteBgData);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdSynchorScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdClearUser);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdClearNotOperateUser);
	//m_TimerLoopThread->getLoop()->cancel(m_TimeridFireLaser);
	//m_TimerLoopThread->getLoop()->cancel(m_TimeridFireBorebit);
	//killGameTimer(kGlobalTimer);
	//killGameTimer(kPauseBoomTimer);
	//killGameTimer(kWriteBgDataTimer);
    //KillAllGameTimer(true);
}

// 启动定时器.
void CTableFrameSink::setGameTimer(DWORD dwTimerID, 
                                  DWORD dwElapse, DWORD dwRepeat, 
                                  WPARAM dwBindParameter, 
                                  bool noSync/* = false*/)
{
    LOG(INFO) << "CTableFrameSink::setGameTimer timerid:" << dwTimerID << ", dwElapse:" << dwElapse << ", dwRepeat:" << dwRepeat << ", dwBindParameter:" << dwBindParameter;
    if (m_pITableFrame) {
        //m_pITableFrame->SetGameTimer(dwTimerID, dwElapse, dwRepeat, dwBindParameter/*, noSync*/);----
    }
}

// 杀掉定时器.
void CTableFrameSink::killGameTimer(DWORD dwTimerID, bool noSync/* = false*/)
{
    LOG(INFO) << "killGameTimer() dwTimerID:" << dwTimerID;
    //m_pITableFrame->KillGameTimer(dwTimerID/*, noSync*/);---
}

// load the special config value.
bool CTableFrameSink::LoadConfig()
{
    CXmlParser xml;
	char file_name[MAX_PATH] = { 0 };
    sprintf(file_name, "conf/rycsfish_config_%u.xml", m_pITableFrame->GetGameRoomInfo()->roomId);
	if (!xml.Load(file_name)) {
        WORD configId = m_pITableFrame->GetTableId();
        LOG(ERROR) << "CTableFrameSink::LoadConfig failed, configId:" << configId;
		return false;
	}
    int i;
    CTextParserUtility txtParser;
	readGameplayDataFromXml(xml);
	if (xml.Sel("/Global"))
	{
        // modify by James 180607-增加.
        aics_kindid = xml.GetLong("aics_kindid");
        // modify by James end.
        fpalyerReventPercent=xml.GetFloat("Taxrevenue");
 		game_config.fishLockable = xml.GetLong("fish_lockable");
		game_config.isExperienceRoom = xml.GetLong("experience_room");
        game_config.broadcast_minval = xml.GetLong("broadcast_minval");
        exchangeAmountForExperienceRoom = xml.GetLong("experience_ExchangeAmount");
		game_config.isAntiCheatRoom = xml.GetLong("anti_cheat_room");
		game_config.timeOutQuitCheck = xml.GetLong("time_out_quit_check");
		game_config.timeOutQuitMsg = xml.GetLong("time_out_quit_msg");
		game_config.ticketRatio = xml.GetLong("ToubiBili");
		game_config.cannonPowerMin = xml.GetLong("ZuixiaoPaoshu");
		game_config.cannonPowerMax = xml.GetLong("ZuidaPaoshu");
		game_config.decimalPlaces = xml.GetLong("DecimalPlaces");
        game_config.revenue_ratio = xml.GetFloat("revenue")/100.0f;
		setExchangeScoreFormat(game_config.decimalPlaces);
		//game_config.bulletStrengtheningScale = xml.GetLong("JiapaoFudu");
		game_config.bulletStrengtheningScale = game_config.cannonPowerMin;
		game_config.bulletSpeed = xml.GetLong("ZidanSudu");
		game_config.consoleType = xml.GetLong("JitaiLeixing");
		exchange_count_ = xml.GetLong("ExchangeAmount");
		game_config.prizeScore = xml.GetLong("BaojiFenshu");
		txtParser.Parse(xml.GetString("ExchangeRatio"), ":");
		m_coinRatioCoin = m_coinRatioScore = atoi(txtParser[0]);
		if (txtParser.Count() > 1) m_coinRatioScore = atoi(txtParser[1]);
        if (m_coinRatioCoin   < 1) m_coinRatioCoin  = 1;    // set default.
        if (m_coinRatioScore  < 1) m_coinRatioScore = 1;    // set default.
		exchange_count_ = convertToFishScore(exchange_count_);
		game_config.prizeScore = convertToFishScore(game_config.prizeScore);
		txtParser.Parse(xml.GetString("RechargeRatio"), ":");
		game_config.exchangeRatioUserScore = game_config.exchangeRatioFishScore = atoi(txtParser[0]);
		if (txtParser.Count() > 1) game_config.exchangeRatioFishScore = atoi(txtParser[1]);
		if (game_config.exchangeRatioUserScore < 1) {
			game_config.exchangeRatioUserScore = 1;
		}
		if (game_config.exchangeRatioFishScore < 1) {
			game_config.exchangeRatioFishScore = 1;
		}
        /*LOG(ERROR) << " >>> LoadConfig m_conRatioCoin:" << m_coinRatioCoin << ",m_coinRatioScore:" << m_coinRatioScore
                   << ", exchangeRatioFishScore:" << game_config.exchangeRatioFishScore
                   << ", broadcast Minval:" << game_config.broadcast_minval;*/

 		//game_config.exchangeRatioUserScore *= m_coinRatioCoin;
 		//game_config.exchangeRatioFishScore *= m_coinRatioScore;

#ifdef __USE_LIFE2__
		txtParser.Parse(xml.GetString("ExchangeRatioIngot"), ":");
		game_config.exchangeRatioScore2 = game_config.exchangeRatioIngot = atoi(txtParser[0]);
		if (txtParser.Count() > 1) game_config.exchangeRatioScore2 = atoi(txtParser[1]);
		if (game_config.exchangeRatioScore2 < 1) {
			game_config.exchangeRatioScore2 = 1;
		}
		if (game_config.exchangeRatioIngot < 1) {
			game_config.exchangeRatioIngot = 1;
		}
#endif
		txtParser.Parse(xml.GetString("Ximaliang"), ",");
		for (i = 0; i < txtParser.Count(); ++i) {
			m_gainForNewOldUser.push_back(atoi(txtParser[i]));
		}
		txtParser.Parse(xml.GetString("WinCoin"), ",");
		for (i = 0; i < txtParser.Count(); ++i) {
			m_winCoinForNewOldUser.push_back(atoi(txtParser[i]));
		}

		game_config.nBaseLaserOdds = xml.GetLong("BaseLaserOdds");		//激光炮的基础倍数
		game_config.nBaseBorebitOdds = xml.GetLong("BaseBorebitOdds");	//钻头炮的基础倍数
		m_nBaseLaserOdds = game_config.nBaseLaserOdds;
		m_nBaseBorebitOdds = game_config.nBaseBorebitOdds;

		m_nHitFishBoomOdds = xml.GetLong("BaseBorebitBoomOdds");		//钻头炮预留爆炸的倍数;

		txtParser.Parse(xml.GetString("time_fire_end"), ":");
		m_nBorebitMinFireTime = m_nBorebitMaxFireTime = atoi(txtParser[0])*1.0;
		if (txtParser.Count() > 1) m_nBorebitMaxFireTime = atoi(txtParser[1])*1.0;
        m_nBorebitFireAllTime = m_nBorebitMaxFireTime;
		game_config.nFlyMaxFireTime = m_nBorebitMaxFireTime;
			
		txtParser.Parse(xml.GetString("Special_KillFishProbili_JG"), ":");
		for (int i = 0; i < txtParser.Count(); ++i)
		{
			if (i < 4)
			{
				m_SpecialKillFishProbili_JG[i] = atoi(txtParser[i]);
			}
		}
		txtParser.Parse(xml.GetString("Special_KillFishProbili_ZT"), ":");
		for (int i = 0; i < txtParser.Count(); ++i)
		{
			if (i < 4)
			{
				m_SpecialKillFishProbili_ZT[i] = atoi(txtParser[i]);
			}
		}
	}
	if (xml.Sel("/Scene"))
	{
		int timer = xml.GetLong("SwitchSceneTime");
		if (timer > 0)
		{
			wSwitchSceneTimerElasped = timer;
		}
		if (xml.CountSubet("Item") > 0)
		{
			while(xml.MoveNext())
			{
				scene_mgr.Push(xml.GetLong("Duration"));
			}
		}
	}
	m_fishArrayManager.setIFishProduce(m_fishProduceDataManager);
	m_fishArrayManager.InitWithXmlFile(xml, game_config.fishArrayParam);
	return fish_mgr.initFromXml(xml);
}

// 读取玩法配置.
void CTableFrameSink::readGameplayDataFromXml(CXmlParser& xml)
{
	if (xml.Sel("/GamePlay"))
    {
#ifdef __GAMEPLAY_COLLECT__
		if (xml.Sel("Collect")) {
			CTextParserUtility txtParser;
			int i;
			txtParser.Parse(xml.GetString("gifting_multiple"), "~");
			m_collectGiftingMultipleMin = m_collectGiftingMultipleMax = static_cast<BYTE>(atoi(txtParser[0]));
			if (txtParser.Count() > 1) m_collectGiftingMultipleMax = static_cast<BYTE>(atoi(txtParser[1]));
			txtParser.Parse(xml.GetString("fish_id"), ",");
			for (i = 0; i < txtParser.Count() && i < COLLECT_COUNT; ++i) {
				game_config.collectData.collectFishId[i] = static_cast<BYTE>(atoi(txtParser[i]));
			}
			game_config.collectData.collectCount = xml.GetLong("collect_count");
			if (game_config.collectData.collectCount > COLLECT_COUNT) {
				game_config.collectData.collectCount = COLLECT_COUNT;
			}
            for (i = 0; i < m_pGameRoomInfo->nStartMaxPlayer; ++i) {
				resetPlayerCollectStats(i);
			}
			xml.Sel("..");
		}
		m_collectUtility.initialize(game_config.collectData);
#endif
#ifdef __GAMEPLAY_MSYQ__
		if (xml.Sel("Msyq")) {
			readXmlWashData(xml);
			m_msyqCopies = xml.GetLong("copies");
			xml.Sel("..");
		}
#endif
#ifdef __GAMEPLAY_PAIPAILE__
		if (xml.Sel("Paipaile")) {
			game_config.pplXmlConfig.fishType = xml.GetLongEx("fish_id");
			game_config.pplXmlConfig.fishLife = xml.GetLong("fish_life");
			xml.Sel("..");
		}
#endif
	}
}

// bool CTableFrameSink::SendGameConfig(IServerUserItem* pIServerUserItem)
// {
// 	return m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_GAME_CONFIG, &game_config, sizeof(game_config));
// }

bool CTableFrameSink::OnTimerSwitchScene(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize)
{
    LOG(INFO) << "OnTimerSwitchScene()";
    bool ret = true;
    bool flag = false;
    killGameTimer(kPauseBoomTimer, true);
    m_is_switching_scene = false;
    fish_mgr.reset();
    m_fishProduceDataManager->reset();
    scene_mgr.Start(scene_mgr.GetNextSceneId());
    m_is_nestScene=false;
    WORD scene_id = scene_mgr.GetCurSceneId();
    WORD table_id = m_pITableFrame->GetTableId();
    LOG(WARNING) << " >>> OnTimerSwitchScene. table_id:" << table_id << " next_scene_id:" << scene_id;
    // loop to send the game scene data to all chair player value now.
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (m_pServerUserItemForGamescene[i])
        {
            if(!isNotOutRoom[i])
            {
                SendGameScene(m_pServerUserItemForGamescene[i]);
            }
            else
            {
                SendGameScene1(m_pServerUserItemForGamescene[i]);
            }
            m_pServerUserItemForGamescene[i] = NULL;
        }
    }
    // try to initialize the special fish array content value.
    m_currentFishArray = m_fishArrayManager.getNextFishArray();
    if (m_currentFishArray && m_currentFishArray->createFishArray())
    {
        WORD table_id  = m_pITableFrame->GetTableId();
        word array_id  = m_currentFishArray->getID();
        DWORD duration = m_currentFishArray->getDuration();
        LOG(INFO) << " >>>> >>>> >>>> FishArray id:" << array_id << ", duration:" << duration;
        CMD_S_ProduceFishArray cmdData = {0};
        cmdData.fishArrayID     = m_currentFishArray->getID();
        cmdData.randomSeed      = m_currentFishArray->getRandomSeed();
        cmdData.firstFishID     = m_currentFishArray->getFirstFishID();
        unsigned short dataSize = NetworkUtility::encodeProduceFishArray(buffer, bufferSize, cmdData);
        m_fishProduceDataManager->popFishesForSending(NULL, 0, GetTickCount());
        send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_FISH_ARRAY_PRODUCE, buffer, dataSize, true);
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdFISH_ARRAY);
        m_TimerIdFISH_ARRAY = m_TimerLoopThread->getLoop()->runAfter(m_currentFishArray->getDuration()/1000,boost::bind(&CTableFrameSink::OnTimerFishArray, this));
        //setGameTimer(kGlobalTimer, m_currentFishArray->getDuration(), 1, GAME_TIMER_FISH_ARRAY, true);
        flag = true;
    }
    if (!flag)
    {
        isGroupScene=false;
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdFISH_ARRAY);
		m_TimerLoopThread->getLoop()->cancel(m_TimeridFISH_PRODUCE);
        LOG(INFO) << " >>> OnTimerSwitchScene. SetGameTimer To produce fish, timerid:" << kGlobalTimer << ", kRepeatTime:" << kRepeatTimer;
        m_TimeridFISH_PRODUCE = m_TimerLoopThread->getLoop()->runAfter(kGlobalTimerElasped/1000,boost::bind(&CTableFrameSink::OnTimerProduceFish, this));
        //setGameTimer(kGlobalTimer, kGlobalTimerElasped, kRepeatTimer, GAME_TIMER_FISH_PRODUCE, true);
	}
	return ret;
}

bool CTableFrameSink::OnTimerSceneEnd(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize)
{
    LOG(INFO) << "OnTimerSceneEnd()";
	m_is_switching_scene = true;
	CMD_S_SwitchScene cmdData = { 0 };
	cmdData.sceneID = scene_mgr.GetNextSceneId();
	cmdData.switchTimeLength = wSwitchSceneTimerElasped * 0.001f;
    LOG(WARNING) << "CTableFrameSink::OnTimerSceneEnd. scene_id:" << cmdData.sceneID;
    // try to set the special timer to synchor the special user score value.
    //setGameTimer(kSynchorScoreTimer, kGlobalTimerSynchorScore, 0, 0, true);
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdSynchorScore);
    m_TimerIdSynchorScore = m_TimerLoopThread->getLoop()->runEvery(kGlobalTimerSynchorScore/1000,boost::bind(&CTableFrameSink::OnTimerSynchor, this));
	unsigned short dataSize = NetworkUtility::encodeSwitchScene(buffer, bufferSize, cmdData);
	send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_SCENE_SWITCH, buffer, dataSize, true);
	return true;
}

bool CTableFrameSink::OnTimerFishProduce(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize)
{
    if (!m_fishProduceDataManager->getPause())
    {
        fish_mgr.produceFish(kGlobalTimerElasped);     
        unsigned short dataSize = m_fishProduceDataManager->popFishesForSending(buffer, bufferSize, GetTickCount());
        if (dataSize)
        {
            WORD table_id = m_pITableFrame->GetTableId();
            WORD fish_id  = m_fishProduceDataManager->getCurrentFishID();
            //LOG(WARNING)<<"cunrrent_fish_id"<<fish_id;
            for (int i = 0; i < MAX_PLAYERS; ++i)
            {
                std::shared_ptr<CServerUserItem> pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
                if  (pIServerUserItem)
                {
                    LOG(WARNING)<<"playerid"<<i<<"------------"<<dataSize;
                    m_pITableFrame->SendTableData(i, SUB_S_FISH_PRODUCE, (const uint8_t*)buffer, dataSize);
                }
            }
            //send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_FISH_PRODUCE, buffer, dataSize, true);
        }
		
		//double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startSwitchSceneTime); //经过了多少时间
		//LOG(WARNING) << "===== CTableFrameSink::OnTimerFishProduce cbtimeleave : " << cbtimeleave << " 秒;";
    }
//Cleanup:
	return true;
}

// 发送桌台数据.
bool CTableFrameSink::send_table_data(WORD me_chair_id, WORD chair_id, WORD sub_cmdid, 
                                     void* data, WORD data_size, bool lookon, 
                                     WORD exclude_chair_id/* = INVALID_CHAIR*/)
{
#ifdef __TEST_NETWORK_PACKET__
	static DECLARE_CMD_BUFFER(buffer) = {0};
	static DWORD sn = 0;
#endif

#ifdef __MULTI_THREAD__
	CCriticalSectionEx lock;
	lock.Lock(&m_csSend);
#endif

#ifdef __TEST_NETWORK_PACKET__
	memcpy(buffer, &sn, sizeof(sn));
	memcpy(buffer + sizeof(sn), data, data_size);
	data = buffer;
	data_size += sizeof(sn);
	++sn;
#endif

    int playernum=m_pITableFrame->GetPlayerCount(false);
	bool ret = false;
	if (INVALID_CHAIR == exclude_chair_id)
    {
        ret = m_pITableFrame->SendTableData(chair_id, sub_cmdid, (const uint8_t*)data, data_size);
	}
    else
    {
        if (INVALID_CHAIR == chair_id)
        {
            for (int i = 0; i < MAX_PLAYERS; ++i)
            {
                if  (i != exclude_chair_id && m_pITableFrame->GetTableUserItem(i))
                {
                    ret = m_pITableFrame->SendTableData(i, sub_cmdid, (const uint8_t*)data, data_size);

                    if(sub_cmdid==SUB_S_USER_FIRE)
                    {
                        //LOG(WARNING)<<"my id===="<<me_chair_id<<"my bullet count "<<m_bulletCount[me_chair_id]<<"        otherid===="<<i<<"     zidan shu ==="<<m_bulletCount[i];

                    }

				}
			}
        } else if (chair_id != exclude_chair_id)
        {
            ret = m_pITableFrame->SendTableData(chair_id, sub_cmdid, (const uint8_t*)data, data_size);
        }
    }
 	//LOG(INFO) << "m_pITableFrame->SendTableData. type[%d] table_id[%d] chair_id[%d] data_size[%d]"), sub_cmdid, m_pITableFrame->GetTableID(), me_chair_id, data_size);
#ifdef __TEST_NETWORK_PACKET__
    LOG(INFO) << "m_pITableFrame->SendTableData. sn[%u] type[%d] table_id[%d] chair_id[%d] data_size[%d]"), sn, sub_cmdid, m_pITableFrame->GetTableID(), me_chair_id, data_size);
	TRACE_INFO_BIN((LPBYTE)data, data_size);
#endif
 	//if (ret && lookon) {
 	//	ret = m_pITableFrame->SendLookonData(INVALID_CHAIR, sub_cmdid, data, data_size);
 	//	LOG(INFO) << "m_pITableFrame->SendLookonData. chair_id[%d] sub_cmdid[%d] data_size[%d]"), chair_id, sub_cmdid, data_size);
 	//}
	return ret;
}
//tong bu bie ren de fenshu
bool CTableFrameSink::OnSendInfoToOther(shared_ptr<CServerUserItem> pIServerUserItem)
{
    word chair_id=pIServerUserItem->GetChairId();
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        if(i==chair_id)
        {
            game_config.exchangeFishScore[i]=m_pITableFrame->GetTableUserItem(i)->GetUserScore()*1000;
            game_config.fishScore[i]=m_pITableFrame->GetTableUserItem(i)->GetUserScore()*1000;
            CMD_S_ExchangeScore cmdData = {0};
            cmdData.playerID = chair_id;
            cmdData.inc = true;
            cmdData.score = pIServerUserItem->GetUserScore()*1000;
            unsigned short dataSize = NetworkUtility::encodeExchangeScore(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
            send_table_data(chair_id, chair_id, SUB_S_EXCHANGE_SCORE, m_send_buffer, dataSize, true);
        }
        if(m_pITableFrame->GetTableUserItem(i)&&i!=chair_id)
        {
            CMD_S_ExchangeScore cmdData = {0};
            cmdData.playerID = i;
            cmdData.inc = true;
            cmdData.score = game_config.exchangeFishScore[i];
            unsigned short dataSize = NetworkUtility::encodeExchangeScore(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
            send_table_data(chair_id, chair_id, SUB_S_EXCHANGE_SCORE, m_send_buffer, dataSize, true);
        }
    }
}
// 上下分操作.
bool CTableFrameSink::OnSubExchangeFishScore(shared_ptr<CServerUserItem> pIServerUserItem, BOOL inc)
{
    LOG(ERROR) << "OnSubExchangeFishScore inc:" << inc << ", isExperienceRoom:" << (int)game_config.isExperienceRoom;
    WORD chair_id = pIServerUserItem->GetChairId();
    DWORD user_id = pIServerUserItem->GetUserId();
    double user_score;
    if (game_config.isExperienceRoom)
    {
 		//user_score = m_exp_user_score[chair_id];
        user_score = m_exp_user_score[chair_id] *PALYER_SCORE_SCALE;
    }
    else
    {
        user_score = pIServerUserItem->GetUserScore();
    }
    LOG(INFO) << "OnSubExchangeFishScore user_score, userid:" <<  user_id <<  ", score:" << user_score;
    long long exchange = 0;//exchange_count_;
#ifdef __USE_LIFE2__
    long long exchange_score2 = 0;
#endif
    double user_leave_score = 0;
 	//SCORE need_user_score = getExchangeUserScore(exchange);
 	//if (game_config.isExperienceRoom) {
        user_leave_score = user_score - getExchangeUserScore(game_config.exchangeFishScore[chair_id]);
 	//}

    CMD_S_ExchangeScore cmdData = {0};
    cmdData.playerID = chair_id;
    cmdData.inc = inc;
    if (inc)
    {

        exchange = getExchangeFishScore(user_leave_score);
        BulletScoreT divNum = getDivNumber();
        exchange = ((exchange / divNum) * divNum);
        if (exchange <= 0) {
            return true;
        }
        // try to get the special exchange score item value.
        game_config.exchangeFishScore[chair_id] = exchange;
        game_config.fishScore[chair_id] = exchange;



        if (user_leave_score <= 0)
        {
            LOG(INFO) << "user_leave_score < 0, user_leave_score:" << user_leave_score;
        }
        else
        {



#ifdef __USE_LIFE2__
            exchange_score2 = pIServerUserItem->GetUserIngot() - game_config.exchangeFishScore2[chair_id];
            if (exchange_score2 > 0) {
                game_config.exchangeFishScore2[chair_id] += exchange_score2;
                game_config.fishScore2[chair_id] += exchange_score2;
            } else {
                exchange_score2 = 0;
            }
            writeArithmLog2(chair_id, -exchange);
#endif
        }

        LOG(WARNING) << "OnSubExchangeFishScore userid:" << user_id
                     << ", user_score:" << user_score << ", user_leave_score:" << user_leave_score
                     << ", exchange:" << exchange << ", exchange score:" << getExchangeFishScore((user_leave_score))
                     << ", game_config.exchangeFishScore:" << game_config.exchangeFishScore[chair_id]
                     << ", game_config.fishScore:" << game_config.fishScore[chair_id]
                     << ", chair_id" << chair_id;

#ifdef AICS_ENGINE

#endif
    }
    else
    {
        // try to get the special fish score value.
        if (game_config.fishScore[chair_id] <= 0)
        {
            LOG(INFO) << "game_config.fishScore <= 0, chair_id:" << chair_id;
            return true;
        }
        else
        {
            exchange = convertToFishScore(convertToFishCoin(game_config.fishScore[chair_id]));
            if (exchange <= 0) {
                return true;
            }
            // try to get the special exchange score value item.
            game_config.exchangeFishScore[chair_id] -= exchange;
            game_config.fishScore[chair_id] -= exchange;

#ifdef AICS_ENGINE

#endif
        }
    }

    cmdData.score = exchange;
#ifdef __USE_LIFE2__
    cmdData.ingot = exchange_score2;
#endif

    // send the special message to the special user to update the special user score content value for later now.
    unsigned short dataSize = NetworkUtility::encodeExchangeScore(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
    send_table_data(chair_id, INVALID_CHAIR, SUB_S_EXCHANGE_SCORE, m_send_buffer, dataSize, true);

    LOG(ERROR) << "CTableFrameSink::OnSubExchangeFishScore End. table_id:" << m_pITableFrame->GetTableId()
             << ", chair_id:" << chair_id << ", player_score:" << game_config.fishScore[chair_id]
             << ", exchange_score:" << game_config.exchangeFishScore[chair_id];

    return true;
}


bool CTableFrameSink::OnSubUserFire(shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_UserFire& user_fire)
{
    // LOG(ERROR) << " >>> OnSubUserFire bullet_id:" << (int)user_fire.bulletID << ", lockFishID:" << user_fire.lockedFishID << ", cannonPower:" << user_fire.cannonPower;
    // check the cannon power and the bullet id is validate check the cannon power and the bullet validate.
	if (user_fire.fireType == 0)
	{
		if (!CheckCannonPower(user_fire.cannonPower, game_config.cannonPowerMin, game_config.cannonPowerMax) || !CheckBulletID(user_fire.bulletID))
		{
			LOG(INFO) << "CTableFrameSink::OnSubUserFire. invalid firescore:" << user_fire.cannonPower;
			return false;
		}
	}
	else if (user_fire.fireType != 1 && user_fire.fireType != 2)
	{
		LOG(INFO) << "CTableFrameSink::OnSubUserFire. invalid firescore:" << user_fire.cannonPower;
		return false;
	}

    WORD chair_id = pIServerUserItem->GetChairId();
 	//LOG(INFO) << "CTableFrameSink::OnSubUserFire Start. chair_id=%d bullet_count=%d"), chair_id, m_bullet_count[chair_id]);
	if (user_fire.fireType == 1 && !m_bHaveLaser[chair_id])
	{
		LOG(INFO) << "CTableFrameSink::OnSubUserFire. invalid fireType:" << user_fire.fireType << (m_bHaveLaser[chair_id] ? " true" : " false");
		return false;
	}
	else if (user_fire.fireType == 2 && !m_bHaveBorebit[chair_id])
	{
		LOG(INFO) << "CTableFrameSink::OnSubUserFire. invalid fireType:" << user_fire.fireType << (m_bHaveBorebit[chair_id] ? " true" : " false");
		return false;
	}

	if (m_bulletCount[chair_id] >= MAX_BULLETS)
	{
        LOG(WARNING) << "m_bulletCount[chair_id] >= MAX_BULLETS!";
		return true;
	} 
    else if (m_bullets[chair_id][user_fire.bulletID])
    {
        LOG(WARNING) << "m_bullets[chair_id][user_fire.bulletID] in use!";
		return true;
	}
	CMD_S_UserFire cmdData = {0};
	cmdData.c = user_fire;
	cmdData.playerID = chair_id;
	long long fishScore = game_config.fishScore[chair_id];
	if (user_fire.fireType == 0 && fishScore < user_fire.cannonPower)
	{
		if (fishScore >= game_config.cannonPowerMin)
		{
			cmdData.c.cannonPower = fishScore;
		}
		else
		{
			++m_bulletCount[chair_id];
			m_bullets[chair_id][cmdData.c.bulletID] = 0;
            LOG(INFO) << " >>> OnSubUserFire, m_bulletCount:" << m_bulletCount;

 			//LOG(INFO) << "CTableFrameSink::OnSubUserFire End. No more fish score.[chair_id=%d player_score=%I64d fire_score=%d]"), chair_id, game_config.fish_score_[chair_id], user_fire.multiple);
			return true;
		}
	}
    unsigned short dataSize = NetworkUtility::encodeUserFire(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
    if (!dataSize) {
        LOG(ERROR) << "encodeUserFire failed!";
    }
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_USER_FIRE, m_send_buffer, dataSize, true, chair_id);
	if (user_fire.fireType == 0)
	{
		AddFishScoreForPlayer(chair_id, sutBulletScore, -cmdData.c.cannonPower);
		++m_bulletCount[chair_id];
		m_bullets[chair_id][cmdData.c.bulletID] = cmdData.c.cannonPower;
	}
	else if (user_fire.fireType == 1 && !m_bHaveFireLaser[chair_id]) //激光炮
	{
		LOG(WARNING) << "CTableFrameSink::OnSubUserFire. fireType:" << user_fire.fireType;
		m_startFireTime = muduo::Timestamp::now(); //发射钻头炮的开始时间
		double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime); //经过了多少时间
		m_startFireAngle = user_fire.angle;
		m_bHaveLaser[chair_id] = false;
		m_bHaveFireLaser[chair_id] = true;
		m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
		m_TimerLoopThread->getLoop()->cancel(m_TimeridFireLaser);
		m_TimeridFireLaser = m_TimerLoopThread->getLoop()->runAfter(5.0, boost::bind(&CTableFrameSink::OnTimerFireLaser, this, chair_id));
	}
	else if (user_fire.fireType == 2 && !m_bHaveFireBorebit[chair_id]) //钻头炮
	{
		LOG(WARNING) << "CTableFrameSink::OnSubUserFire. fireType:" << user_fire.fireType;
		m_startFireTime = muduo::Timestamp::now(); //发射钻头炮的开始时间
		double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime); //经过了多少时间
		m_startFireAngle = user_fire.angle;
		m_bHaveBorebit[chair_id] = false;
		m_bHaveFireBorebit[chair_id] = true;
		m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
		m_TimerLoopThread->getLoop()->cancel(m_TimeridBorebitStartRotate);
		m_TimeridBorebitStartRotate = m_TimerLoopThread->getLoop()->runAfter(m_nBorebitMaxFireTime*1.0f - 2.0, boost::bind(&CTableFrameSink::OnTimerBorebitStartRotate, this, chair_id));
	}
    // LOG(INFO) << " >>> OnSubUserFire bullet_id:" << (int)user_fire.bulletID << ", lockFishID:" << user_fire.lockedFishID << "cannonPower:" << cmdData.c.cannonPower << ", chairid:" << (int)chair_id;
    //log_bullets("  +  OnSubUserFire, chairid:[%d], bulletid:%d, bulletcount:%d",chair_id, user_fire.bulletID, m_bulletCount[chair_id]);
  	//LOG(INFO) << "CTableFrameSink::OnSubUserFire End.";
	return true;
}

bool CTableFrameSink::OnSubSpecialRotate(shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_SpecialRotate& specialRotate)
{
	WORD chair_id = pIServerUserItem->GetChairId();
	if ((!m_bHaveLaser[chair_id] && !m_bHaveBorebit[chair_id]) ||
		(m_bHaveFireLaser[chair_id] || m_bHaveFireBorebit[chair_id]))
	{
		LOG(WARNING) << "==========OnSubSpecialRotate failed!";
		return false;
	}
	CMD_S_SpecialRotate cmdData = { 0 };
	cmdData.chairid = chair_id;
	cmdData.hitType = specialRotate.hitType;
	cmdData.angle = specialRotate.angle;
	unsigned short dataSize = NetworkUtility::decodeSpecialRotate(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	if (!dataSize) {
		LOG(ERROR) << "decodeSpecialRotate failed!";
	}
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_SETSPECIALROTATE, m_send_buffer, dataSize, true, chair_id);
	
	return true;
}

bool CTableFrameSink::SendGameScene1(std::shared_ptr<CServerUserItem> pIServerUserItem)
{
      //send scene
      // 子弹积分返还.
      int chair_id=pIServerUserItem->GetChairId();
      doReturnBulletScore(pIServerUserItem->GetChairId());
      // 构建场景消息.
      CMD_S_GameScene cmdData = {0};
      cmdData.pause      = m_fishProduceDataManager->getPause();      // 当前是否暂停.
      cmdData.gameConfig = game_config;
      if(m_is_nestScene)
      {
          cmdData.sceneID    =scene_mgr.GetNextSceneId();
      }
      else
      {
          cmdData.sceneID    = scene_mgr.GetCurSceneId();
      }
      if(isGroupScene)
      {
          for(int i=0;i<deathfishs.size();i++)
          {
              cmdData.fishLiveArray.push_back(deathfishs[i]);
          }
          cmdData.sceneTime=(GetTickCount()-sceneTimeLeft)/1000.0f;
      }
      else
      {
           cmdData.sceneTime=0;
      }
	  WORD dHaveSpecailFireChairID = 0;
	  for (int i = 0;i < MAX_PLAYERS;i++)
	  {
		  if (m_pITableFrame->GetTableUserItem(i))
		  {
			  if (m_bHaveLaser[i] || m_bHaveFireLaser[i] || m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
			  {
				  dHaveSpecailFireChairID = i;
				  break;
			  }
		  }
	  }
      //set player score
      // 玩法参数配置.
      for(int i=0;i<MAX_PLAYERS;i++)
      {
          if(m_pITableFrame->GetTableUserItem(i))
          {
              cmdData.players[i].nikename=m_pITableFrame->GetTableUserItem(i)->GetNickName();
              cmdData.players[i].score=game_config.fishScore[i];
              cmdData.players[i].userid=m_pITableFrame->GetTableUserItem(i)->GetUserId();
              cmdData.players[i].chairid=i;
              cmdData.players[i].isExist=true;
              cmdData.players[i].status=m_pITableFrame->GetTableUserItem(i)->GetUserStatus();

			  if (m_bHaveLaser[i] || m_bHaveFireLaser[i])
			  {
				  cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
				  cmdData.specialFire[i].chairid = i;
				  cmdData.specialFire[i].FireType = 1;
				  cmdData.specialFire[i].bHaveSpecialFire = true;
				  cmdData.specialFire[i].bHaveFire = m_bHaveFireLaser[i];
				  cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
				  if (m_bHaveFireLaser[i])
				  {
					  cmdData.specialFire[i].FireAngle = m_startFireAngle;
					  cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
				  }
				  else 
				  {
					  int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
					  cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
				  }
			  }
			  else if (m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
			  {
				  cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
				  cmdData.specialFire[i].chairid = i;
				  cmdData.specialFire[i].FireType = 2;
				  cmdData.specialFire[i].bHaveSpecialFire = true;
				  cmdData.specialFire[i].bHaveFire = m_bHaveFireBorebit[i];
				  cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
				  if (m_bHaveFireBorebit[i])
				  {
					  cmdData.specialFire[i].FireAngle = m_startFireAngle;
					  cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
					  if (m_bBorebitStartRotate && m_nBorebitFireAllTime > cmdData.specialFire[i].FirePassTime)
					  {
						  cmdData.specialFire[i].FireEndCountTime = m_nBorebitFireAllTime - cmdData.specialFire[i].FirePassTime;
					  }
				  }
				  else
				  {
					  int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
					  cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
				  }
			  }
          }
          else
          {
              cmdData.players[i].isExist=false;
			  cmdData.specialFire[i].bHaveSpecialFire = false;
          }
      }
      // 当前鱼群中是否包含鱼阵.
      if (m_currentFishArray)
      {
          if (m_fishProduceDataManager->contains(stSchool)) {
              cmdData.fishArray.fishArrayID = m_currentFishArray->getID();
              cmdData.fishArray.randomSeed = m_currentFishArray->getRandomSeed();
              cmdData.fishArray.firstFishID = m_currentFishArray->getFirstFishID();
          }
      }
      unsigned short dataSize = NetworkUtility::encodeGameScene(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData, m_fishProduceDataManager->getAliveFishes(),isGroupScene, m_bBorebitStartRotate);
      m_pITableFrame->SendUserData(pIServerUserItem, SUB_S_GAME_SCENE, (const uint8_t*)m_send_buffer, dataSize);
      for(int i=0;i<MAX_PLAYERS;i++)
      {
          if(m_pITableFrame->GetTableUserItem(i))
          {
              CMD_C_AddLevel cmdData = {0};
              cmdData.userid=i;
              cmdData.powerlevel=bulletlevel[i];
              if(cmdData.powerlevel>=game_config.cannonPowerMin&&cmdData.powerlevel<=game_config.cannonPowerMax)
              {
                  int bytesize = 0;
                  char buffer[1024]={0};
                  ::rycsfish::CMD_S_AddLevel addcanno;
                  addcanno.set_cannonpower(cmdData.powerlevel);
                  addcanno.set_playerid(i);
                  bytesize = addcanno.ByteSize();
                  addcanno.SerializeToArray(buffer,bytesize);
                  send_table_data(chair_id, chair_id, SUB_S_ADDLEVEL, buffer, bytesize, true);
              }
          }
      }
}
// 发送场景消息,这个消息约1780字节,是游戏中最大的一个包.
bool CTableFrameSink::SendGameScene(std::shared_ptr<CServerUserItem> pIServerUserItem)
{
    DWORD chair_id = pIServerUserItem->GetChairId();
    DWORD user_id  = pIServerUserItem->GetUserId();
	bool ret = true;
    // 子弹积分返还.
	doReturnBulletScore(chair_id);
#ifdef __GAMEPLAY_MSYQ__
	memcpy(game_config.msyqData.msyqPercent, m_washPercent, sizeof(game_config.msyqData.msyqPercent));
#endif
	m_fishProduceDataManager->updateWithEndTick(GetTickCount());
    isNotOutRoom[pIServerUserItem->GetChairId()]=true;
    // 构建场景消息.
	CMD_S_GameScene cmdData = {0};
	cmdData.sceneID    = scene_mgr.GetCurSceneId();                 // 当前场景编号(控制背景与背景音乐).
    cmdData.pause      = m_fishProduceDataManager->getPause();      // 当前是否暂停.
    cmdData.gameConfig = game_config;
    if(isGroupScene)
    {
        for(int i=0;i<deathfishs.size();i++)
        {
            cmdData.fishLiveArray.push_back(deathfishs[i]);
        }
        cmdData.sceneTime=(GetTickCount()-sceneTimeLeft)/1000.0f;
    }
    else
		cmdData.sceneTime=0;
    //set player score

    CalcScore(pIServerUserItem, true);
    OnSubExchangeFishScore(pIServerUserItem, TRUE);
    playerScore[chair_id]=game_config.fishScore[chair_id];
	WORD dHaveSpecailFireChairID = 0;
	for (int i = 0;i < MAX_PLAYERS;i++)
	{
		if (m_pITableFrame->GetTableUserItem(i))
		{
			if (m_bHaveLaser[i] || m_bHaveFireLaser[i] || m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
			{
				dHaveSpecailFireChairID = i;
				break;
			}
		}
	}
    // 玩法参数配置.
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        if(m_pITableFrame->GetTableUserItem(i))
        {
            cmdData.players[i].nikename=m_pITableFrame->GetTableUserItem(i)->GetNickName();
            cmdData.players[i].score=game_config.fishScore[i];
            cmdData.players[i].userid=m_pITableFrame->GetTableUserItem(i)->GetUserId();
            cmdData.players[i].chairid=i;
            cmdData.players[i].isExist=true;
            cmdData.players[i].status=m_pITableFrame->GetTableUserItem(i)->GetUserStatus();

			if (m_bHaveLaser[i] || m_bHaveFireLaser[i])
			{
				cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
				cmdData.specialFire[i].chairid = i;
				cmdData.specialFire[i].FireType = 1;
				cmdData.specialFire[i].bHaveSpecialFire = true;
				cmdData.specialFire[i].bHaveFire = m_bHaveFireLaser[i];
				cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
				if (m_bHaveFireLaser[i])
				{
					cmdData.specialFire[i].FireAngle = m_startFireAngle;
					cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
				}
				else
				{
					int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
					cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
				}
			}
			else if (m_bHaveBorebit[i] || m_bHaveFireBorebit[i])
			{
				cmdData.specialFire[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserId();
				cmdData.specialFire[i].chairid = i;
				cmdData.specialFire[i].FireType = 2;
				cmdData.specialFire[i].bHaveSpecialFire = true;
				cmdData.specialFire[i].bHaveFire = m_bHaveFireBorebit[i];
				cmdData.specialFire[i].specialScore = m_fireAllWinScore[dHaveSpecailFireChairID];
				if (m_bHaveFireBorebit[i])
				{
					cmdData.specialFire[i].FireAngle = m_startFireAngle;
					cmdData.specialFire[i].FirePassTime = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime);
					if (m_bBorebitStartRotate && m_nBorebitFireAllTime > cmdData.specialFire[i].FirePassTime)
					{
						cmdData.specialFire[i].FireEndCountTime = m_nBorebitFireAllTime - cmdData.specialFire[i].FirePassTime;
					}
				}
				else
				{
					int32_t passtime = ((int32_t)time(nullptr) - m_startFireCountTime);
					cmdData.specialFire[i].FireCountTime = FIRECOUNTTIME > passtime ? FIRECOUNTTIME - passtime : 0;
				}
			}
        }
        else
        {
            cmdData.players[i].isExist=false;
			cmdData.specialFire[i].bHaveSpecialFire = false;
        }
    }
    // 当前鱼群中是否包含鱼阵.
    if (m_currentFishArray)
    {
		if (m_fishProduceDataManager->contains(stSchool)) {
			cmdData.fishArray.fishArrayID = m_currentFishArray->getID();
			cmdData.fishArray.randomSeed = m_currentFishArray->getRandomSeed();
			cmdData.fishArray.firstFishID = m_currentFishArray->getFirstFishID();
		}
	}
    unsigned short dataSize = NetworkUtility::encodeGameScene(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData, m_fishProduceDataManager->getAliveFishes(),isGroupScene, m_bBorebitStartRotate);
    LOG(ERROR) << " >>> SendGameScene() size:" << dataSize << ", userid: " << user_id;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        LOG(INFO) << "chair_id:" << i << ", player_score:" << game_config.fishScore[i];
    }
    // try to send the special scene message to the special client content item for later user content item now.
    ret = m_pITableFrame->SendUserData(pIServerUserItem, SUB_S_GAME_SCENE, (const uint8_t*)m_send_buffer, dataSize);
	if (ret)
    {
		TCHAR msg[1024]=TEXT("");
		wsprintf(msg, _T("欢迎进入“%s”游戏，祝您游戏愉快！"), _T(GAME_NAME));
	}
    else
    {
        LOG(INFO) << "SendUserData SUB_S_GAME_SCENE failed, userid:" << user_id;
    }

//Cleanup:
	return ret;
}

bool CTableFrameSink::OnSubLockFish(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_LockFish& lock_fish)
{
    LOG(INFO) << "OnSubLockFish() chair_id:" << pIServerUserItem->GetChairId();
	CMD_S_LockFish cmdData = { 0 };
	cmdData.c = lock_fish;
    cmdData.playerID = pIServerUserItem->GetChairId();
	unsigned short dataSize = NetworkUtility::encodeLockFish(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
//Cleanup:
	send_table_data(cmdData.playerID, INVALID_CHAIR, SUB_S_FISH_LOCK, m_send_buffer, dataSize, true, cmdData.playerID);
	return true;
}

bool CTableFrameSink::OnSubUnlockFish(std::shared_ptr<CServerUserItem> pIServerUserItem)
{
    LOG(INFO) << "OnSubUnlockFish() chair_id:" << pIServerUserItem->GetChairId();
	CMD_S_UnlockFish cmdData = { 0 };
    cmdData.playerID = pIServerUserItem->GetChairId();
	unsigned short dataSize = NetworkUtility::encodeUnlockFish(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(cmdData.playerID, INVALID_CHAIR, SUB_S_FISH_UNLOCK, m_send_buffer, dataSize, true, cmdData.playerID);
	return true;
}

bool CTableFrameSink::OnSubFrozenFish(std::shared_ptr<CServerUserItem> pIServerUserItem)
{
    char buf[1024]={0};
    WORD table_id = pIServerUserItem->GetTableId();
    WORD chair_id = pIServerUserItem->GetChairId();
    LOG(INFO) << "CTableFrameSink::OnSubFrozenFish. table_id:" << table_id <<", chair_id:" << chair_id;
    // initialize the data value.
    ::rycsfish::CMD_S_Frozen frozen;
    frozen.set_bfrozen(true);
	int bytesize = frozen.ByteSize();
    frozen.SerializeToArray(buf,bytesize);
    // try to send the special frozen data to the special client value content now.
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_FROZEN, buf, bytesize, true);
	setGameTimer(kPauseBoomTimer, kPauseBoomTimerElasped, 1, 0);

#ifdef __MULTI_THREAD__
	CCriticalSectionEx lock;
	lock.Lock(&m_cs);
#endif
	frozenAllFish(chair_id);
	return true;
}

bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/fish_ry.ini"))
    {
        LOG_INFO << "./conf/fish_lk.fish_ry not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/fish_ry.ini", pt);

    controlledProxy.clear();
    controlledProxy = to_array<int32_t>(pt.get<string>("GAME_CONFIG.AGENTID","0,0,0,0"));

    controllPoint=pt.get<double>("GAME_CONFIG.CONTROLLPOINT", 1.0);
	vector<int64_t> personvec;

    personalProfitLevel.clear();
    string Alritham="PERSONALLEVEL"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    personalProfitLevel=to_array<int64_t>(pt.get<string>(Alritham.c_str(),"10000,10000,10000,10000"));

    Alritham="PERSONALPROBIRITY"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    personalPribirity=to_array<float_t>(pt.get<string>(Alritham,"1.0,1.0,1.0,1.0"));


    smallOrBigLevelbet = pt.get<int64_t>("GAME_CONFIG.BIGALLSMALLBET", 2000000);
    smallPersonalLevel.clear();
    Alritham="SMALLEVEL"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    smallPersonalLevel=to_array<int64_t>(pt.get<string>(Alritham.c_str(),"10000,10000,10000,10000"));

    Alritham="SMALLPROBIRITY"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    smallPersonalPribirity=to_array<float_t>(pt.get<string>(Alritham,"1.0,1.0,1.0,1.0"));


    Alritham="OVERALLPROFIT"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    overallProfit = to_array<int64_t>(pt.get<string>(Alritham,"10000,10000,10000,10000"));

    Alritham="OVERALLPROBIRITY"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".""temp";
    overallProbirity = to_array<float_t>(pt.get<string>(Alritham,"1.0,1.0,1.0,1.0"));

    fairProbability=pt.get<double>("GAME_CONFIG.Fair_Probability", 1.0);
    fishRake=pt.get<int32_t>("GAME_CONFIG.FishRake", 5);

    Resurrection=pt.get<int32_t>("GAME_CONFIG.Resurrection", 5);
	
	
    fishCal=pt.get<int32_t>("GAME_CONFIG.FishCal", 5);
    fishWinScore=pt.get<int32_t>("GAME_CONFIG.FishUpScore", 500000);
}

bool CTableFrameSink::OnSubHitFish(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_HitFish& hit_fish, unsigned short fish_count)
{
    WORD chair_id = pIServerUserItem->GetChairId();
    int userid = pIServerUserItem->GetUserId();
	if (!CheckBulletID(hit_fish.bulletID))
    {
        LOG(ERROR) << "OnSubHitFish error, invalid bullet. chair_id:" << chair_id <<", bullet_id:" << hit_fish.bulletID;
		return false;
	}
	WORD realChairID = getRealChairID(chair_id);
	BulletScoreT fire_score = m_bullets[chair_id][hit_fish.bulletID];
	CMD_S_CatchFish cmdData = { 0 };
	cmdData.bulletID = hit_fish.bulletID;
	cmdData.cannonPower = hit_fish.hitType == 0 ? fire_score : m_fireScore[chair_id];
	cmdData.playerID = chair_id;
	//cmdData.hitType = hit_fish.hitType;
    // check validate item now.
    if (hit_fish.hitType == 0 && ((fish_count <= 0) || (fire_score <= 0) || (m_bulletCount[chair_id] <= 0)))
    {
        LOG(ERROR) << "OnSubHitFish invalid bullet error!. chair_id:" << chair_id
                   << ", fish_count: "  << (int)fish_count
                   << ", fire_score:"   << (int)fire_score
                   << ", bullet_count:" << (int)m_bulletCount[chair_id]
                   << ", bullet id:"    << (int)hit_fish.bulletID;
		return false;
	}
    // sub the special bullet.
	if (hit_fish.hitType == 0)
	{
		--m_bulletCount[chair_id];
		m_bullets[chair_id][hit_fish.bulletID] = 0;
	}
    //log_bullets("  -  OnSubHitFish dec bullet, chairid:[%d], bulletid:%d, bulletcount:%d",chair_id, hit_fish.bulletID, m_bulletCount[chair_id]);
	unsigned short dataSize;
	if (hit_fish.hitType == 0 && !CheckCannonPower(fire_score, game_config.cannonPowerMin, game_config.cannonPowerMax))
    {
        LOG(INFO) << "CTableFrameSink::OnSubHitFish[invalid bullet. chair_id:" << chair_id << ", firescore: " << fire_score << ", fish_count:" << fish_count;

        dataSize = NetworkUtility::encodeCatchFish(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData, NULL, 0,NULL,0);
        if (!dataSize)
        {
            LOG(WARNING) << "CTableFrameSink::OnSubHitFish[invalid bullet. chair_id:" << chair_id <<", firescore: " << fire_score <<", fish_count:" << fish_count;
        }
		send_table_data(chair_id, INVALID_CHAIR, SUB_S_FISH_CATCH, m_send_buffer, dataSize, true);
		return true;
	}
	
    WORD tableId = m_pITableFrame->GetTableId();
	const FishKind* fish_kind = NULL;
	bool is_pause_boom = false;
	short catch_fish_count = 0;
	bool bCatchSpecialFire = false;
	if ((hit_fish.hitType == 1 && m_bHaveFireLaser[chair_id]) || (hit_fish.hitType == 2 && m_bHaveFireBorebit[chair_id])) //激光炮、钻头炮
	{
		fire_score = m_fireScore[chair_id];
	}
#ifdef __MULTI_THREAD__
	CCriticalSectionEx lock;
	lock.Lock(&m_cs);
#endif
	if (fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[0]))
	{
		if ((hit_fish.hitType == 1 && !m_bHaveFireLaser[chair_id] && !m_bHaveLaser[chair_id]) || (hit_fish.hitType == 2 && !m_bHaveFireBorebit[chair_id] && !m_bHaveBorebit[chair_id])) //激光炮、钻头炮
		{
			LOG(WARNING) << "CTableFrameSink::OnSubHitFish[invalid Specialbullet. 无效的特殊炮 chair_id:" << chair_id << ", firescore: " << fire_score << ", fish_count:" << fish_count;
			return false;
		}
		BOOL bKilled = FALSE;
		LONG score = 0;
		int i;
		FishKind fishKind = *fish_kind;
		int kill_count = 0;
		bool isBoss = false;
		bool bBossOrGlobalBomb = false; //boss或全屏炸弹
		memset(m_usedFishIDs, 0, sizeof(m_usedFishIDs));
        tagStorageInfo tagstorage;
        m_pITableFrame->GetStorageScore(tagstorage);
        tagPersonalProfit& personalCtr = pIServerUserItem->GetPersonalPrott();
		if (fish_kind->fishType.king > 0) // 特殊鱼，非扩散攻击处理
		{
			isBoss = true;
			int life_sum = 0;
			if ((hit_fish.hitType == 1 && m_bHaveFireLaser[chair_id]) || (hit_fish.hitType == 2 && m_bHaveFireBorebit[chair_id])) //激光炮、钻头炮
			{
				fire_score = m_fireScore[chair_id];
				bKilled = OnSubHitFish_Special(pIServerUserItem, hit_fish, fish_count, fire_score, fish_kind, catch_fish_count, isBoss, bBossOrGlobalBomb, is_pause_boom, score);
			}
			else
			{
				if (fish_kind->fishType.king == fktPauseBomb)
				{
					is_pause_boom = true;
				}
				if (fish_kind->fishType.king == fktGlobalBomb || fish_kind->fishType.king == fktAreaBomb)
				{
					bBossOrGlobalBomb = true;
				}
				for (i = 0; i < fish_count; ++i)
				{
					fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[i]);
					if (!m_usedFishIDs[m_fishIDs[i]] && fish_kind)
					{
						m_fishIDs[catch_fish_count++] = m_fishIDs[i];
						m_catchFishKindList[m_fishIDs[i]] = *fish_kind;
						m_usedFishIDs[m_fishIDs[i]] = 1;
						life_sum += fish_kind->life;
					}
					else {
						LOG(INFO) << "CTableFrameSink::OnSubHitFish. skip fish_id:" << m_fishIDs[i] << ", chair_id:" << chair_id;
					}
				}
				m_kill_info[0].nId = m_fishIDs[0];
				m_kill_info[0].nOdds = life_sum;
				m_kill_info[0].nResult = 0;
				kill_count = 1;
				onPlayerFired(tableId, chair_id, fire_score);
#ifdef AICS_ENGINE
				int  isHaveBlackPlayer[MAX_PLAYERS] = { 0 };
                int  isBlackAgent[MAX_PLAYERS]={0};
				for (int i = 0;i < MAX_PLAYERS;i++)
				{
					shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
					if (!user || !user->GetBlacklistInfo().status)
					{
						continue;
					}
					if (user->IsAndroidUser()) continue;
					if (user->GetBlacklistInfo().listRoom.size() > 0)
					{
						auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
						if (it != user->GetBlacklistInfo().listRoom.end())
						{
							if (it->second > 0)
							{
								isHaveBlackPlayer[i] = it->second;
							}
							//openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
							//openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
						}
					}
				}
                for(int i=0;i<MAX_PLAYERS;i++)
                {
                     shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                    if(!user)
                    {
                        continue;
                    }
                    if(user->IsAndroidUser()) continue;
                    int32_t agentId = user->GetUserBaseInfo().agentId;
                    for(int i=0;i<controlledProxy.size();i++)
                    {
                        if(agentId==controlledProxy[i])
                        {
                            isBlackAgent[user->GetChairId()]=1;
                        }
                    }
                }
				int ret = 0;
				if (isHaveBlackPlayer[pIServerUserItem->GetChairId()])
				{
                    ret=GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
					//ret = GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
					if (ret == true)
					{
						if (isHaveBlackPlayer[pIServerUserItem->GetChairId()] > 1000)
						{
							isHaveBlackPlayer[pIServerUserItem->GetChairId()] = 1000;
						}
						double probili = ((double)isHaveBlackPlayer[pIServerUserItem->GetChairId()]) / 1000.0;
						float fr = m_random.betweenFloat(0, 1.0).randFloat_mt(true);
						if (fr < probili)
						{
							ret = 0;
							int scorex = life_sum * fire_score;
							onFishCatched(tableId, realChairID, scorex);
						}
					}
				}
                else if(isBlackAgent[pIServerUserItem->GetChairId()])
                {
                    ret=GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
                    //ret=GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
                    if(ret == true)
                    {
                        float fr = m_random.betweenFloat(0,100.0f).randFloat_mt(true);
                        if(fr<controllPoint)
                        {
                            ret=0;
                            int scorex = life_sum * fire_score;
                            onFishCatched(tableId, realChairID, scorex);
                        }
                    }
                }
				else
                    ret=GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
					//ret = GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
				bKilled = FALSE;


				//******** Test Start ********
//                int index = 0;
//				STD::Random r2;
//				index = r2.betweenInt(0, 100).randInt_mt();
//				if (index < 100)
//				{
//					ret = 1;
//                }
				//******** Test End ********

				if (ret == 1) {
					bKilled = TRUE;
					m_kill_info[0].nResult = AICSERR_KILLED;
					if (bBossOrGlobalBomb) //打死的是全屏炸弹，判断是否有特殊炮
					{
						DWORD dwSceneLessTime = scene_mgr.GetLessRunTime();
						const FishKind* fish_kind2 = NULL;
						for (i = 0; i < catch_fish_count; ++i)
						{
							//是否打死的是激光蟹或者是钻头蟹
							if (fish_kind2 = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[i]))
							{
								FishKind fishKind2 = *fish_kind2;
								if (fishKind2.fishType.type == 54) //获得激光炮
								{
									bCatchSpecialFire = true;
									m_bCatchSpecialFireInGlobalBomb[chair_id] = true;
									m_fireScore[chair_id] = 0;
									life_sum -= fish_kind2->life;
									m_nAddHitFishOdds = 0;
									m_nLessLaserOdds = fish_kind2->life - m_nBaseLaserOdds;
									m_bHaveLaser[chair_id] = true;
									m_fireScore[chair_id] = fire_score;
									m_fireAllWinScore[chair_id] = m_nBaseLaserOdds * fire_score;
									m_nAddHitFishOdds = m_nBaseLaserOdds;
									m_nfireMaxOdds = fish_kind2->life;
									STD::Random rd;
									int32_t addOdds = rd.betweenInt(-10, 10).randInt_mt();
									m_nfireMaxOdds += addOdds;
									LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish_Normal. m_nfireMaxOdds:" << m_nfireMaxOdds << ", addOdds:" << addOdds << ", dwSceneLessTime:" << dwSceneLessTime;
									m_startFireCountTime = (int32_t)time(nullptr);
									m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
									m_TimeridCountDownFire = m_TimerLoopThread->getLoop()->runAfter(FIRECOUNTTIME, boost::bind(&CTableFrameSink::OnTimerCountDownFire, this, chair_id));
									m_fishProduceDataManager->setbProduceSpecialFire(false);

									dwSceneLessTime = scene_mgr.GetLessRunTime();
									if (dwSceneLessTime < 20 * 1000) //离切换场景还剩少于20s,需延长切换的时间
									{
										m_isNeedExtendSwitchScene = true;
										DWORD dwNeedAddTick = 20 * 1000 - dwSceneLessTime;
										scene_mgr.SetAddTick(dwNeedAddTick);
										LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish. dwSceneLessTime:" << dwSceneLessTime << ", dwNeedAddTick:" << dwNeedAddTick;
									}

									break;
								}
								else if (fishKind2.fishType.type == 55) //获得钻头炮
								{
									bCatchSpecialFire = true;
									m_bCatchSpecialFireInGlobalBomb[chair_id] = true;
									m_fireScore[chair_id] = 0;
									life_sum -= fish_kind2->life;
									m_nAddHitFishOdds = 0;
									m_nLessBorebitOdds = fish_kind2->life - m_nBaseBorebitOdds;
									m_bHaveBorebit[chair_id] = true;
									m_fireScore[chair_id] = fire_score;
									m_fireAllWinScore[chair_id] = m_nBaseBorebitOdds * fire_score;
									m_nAddHitFishOdds = m_nBaseBorebitOdds;
									m_nfireMaxOdds = fish_kind2->life;
									STD::Random rd;
									int32_t addOdds = rd.betweenInt(-10, 10).randInt_mt();
									m_nfireMaxOdds += addOdds;
									LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish_Normal. m_nfireMaxOdds:" << m_nfireMaxOdds << ", addOdds:" << addOdds << ", dwSceneLessTime:" << dwSceneLessTime;
									m_startFireCountTime = (int32_t)time(nullptr);
									m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
									m_TimeridCountDownFire = m_TimerLoopThread->getLoop()->runAfter(FIRECOUNTTIME, boost::bind(&CTableFrameSink::OnTimerCountDownFire, this, chair_id));
									m_fishProduceDataManager->setbProduceSpecialFire(false);

									dwSceneLessTime = scene_mgr.GetLessRunTime();
									if (dwSceneLessTime < (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000) //离切换场景还剩少于20s,需延长切换的时间
									{
										m_startExtendSwitchSceneTime = muduo::Timestamp::now();
										DWORD dwNeedAddTick = (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000 - dwSceneLessTime;
										scene_mgr.SetAddTick(dwNeedAddTick);
										LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish. dwSceneLessTime:" << dwSceneLessTime << ", dwNeedAddTick:" << dwNeedAddTick;

										m_isNeedExtendSwitchScene = true;
										m_dwSceneLessTime = (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000;
										m_dwNeedAddTick = dwNeedAddTick;
										LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish.获得钻头炮最大需延长切换场景 dwSceneLessTime:" << dwSceneLessTime << ", m_dwNeedAddTick:" << m_dwNeedAddTick << " ,m_dwSceneLessTime: " << m_dwSceneLessTime;
									}

									break;
								}
							}
						}
					}
				}
				//if (m_kill_info[0].nResult == AICSERR_KILLED) {
				//	bKilled = TRUE;
				//}
#else
				bKilled = TRUE;
#endif
				score = life_sum * fire_score;
				fishKind.life = life_sum;
			}
		}
		else // 普通鱼，扩散攻击处理
		{
            if ((hit_fish.hitType == 1 && m_bHaveFireLaser[chair_id]) || (hit_fish.hitType == 2 && m_bHaveFireBorebit[chair_id])) //激光炮、钻头炮
			{
				fire_score = m_fireScore[chair_id];
				bKilled = OnSubHitFish_Special(pIServerUserItem, hit_fish, fish_count, fire_score, fish_kind, catch_fish_count, isBoss, bBossOrGlobalBomb, is_pause_boom, score);
			}
			else
			{
				bKilled = OnSubHitFish_Normal(pIServerUserItem, hit_fish, fish_count, fire_score, fish_kind, catch_fish_count, isBoss, bBossOrGlobalBomb, score);
			}
		}
		if (bKilled)
        {
  			for (i = 0; i < catch_fish_count; ++i)
            {
               if(isGroupScene)
               {
                   deathfishs.push_back(m_fishIDs[i]);
               }
                m_fishProduceDataManager->remove(m_fishIDs[i]);
			}
            dataSize = NetworkUtility::encodeCatchFish(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData, m_fishIDs, catch_fish_count,m_fishIDs, catch_fish_count);
			send_table_data(chair_id, INVALID_CHAIR, SUB_S_FISH_CATCH, m_send_buffer, dataSize, true);
            if (score > 0)
            {
				if (isBoss)
				{
					if (bBossOrGlobalBomb)
					{
						if (bCatchSpecialFire) //获得特殊炮，先加基础倍数给玩家
						{
							AddFishScoreForPlayer(chair_id, sutHandselBoss, score);
							if (m_bHaveLaser[chair_id])
							{
								AddFishScoreForPlayer(chair_id, sutFishScore, m_nBaseLaserOdds * fire_score);
							} 
							else if (m_bHaveBorebit[chair_id])
							{
								AddFishScoreForPlayer(chair_id, sutFishScore, m_nBaseBorebitOdds * fire_score);
							}
						} 
						else
						{
							AddFishScoreForPlayer(chair_id, sutHandselBoss, score);
						}
					}
					else
					{
						AddFishScoreForPlayer(chair_id, sutHandsel, score);
					}
				}
				else
				{
					AddFishScoreForPlayer(chair_id, sutFishScore, score);
				}
				if (hit_fish.hitType == 0)
				{					
					double exchangedScore = getExchangeUserScore(score);
					std::string fishname = fish_mgr.getFishName(fishKind.fishType);
					int fishtype = fishKind.fishType.type;
					int bking = fishKind.fishType.king;
					int onlyType = fishKind.fishType.Onlytype;
                    int fishkindpaihang = 0;
					if (bking == fktSameBomb)//是同类炸弹的时候传的type==21
					{
						playerLog[chair_id].AddHitRecord(fire_score, 21, score / fire_score, score);
                        fishkindpaihang = 21;
					}
					else if (bking == fktCombination)  //组合鱼的时候小于三是大三元，大于等三是大四喜
					{
						if (fishtype < 3)
						{
							playerLog[chair_id].AddHitRecord(fire_score, 22, score / fire_score, score);
                            fishkindpaihang = 22;
						}
						else
						{
							playerLog[chair_id].AddHitRecord(fire_score, 23, score / fire_score, score);
                            fishkindpaihang = 23;
						}

					}
					else if (bking == fktGlobalBomb)//全屏炸弹的时候传24
					{
						LOG(WARNING) << " ****** CTableFrameSink::OnSubHitFish. AddHitRecord 24 " << " fireScore:" << fire_score << " score:" << score << " Recordodds:" << score / fire_score;
						playerLog[chair_id].AddHitRecord(fire_score, 24, score / fire_score, score);
                        fishkindpaihang = 24;
					}
					else if (bking == fktYWDJ)//一网打尽的时候传25
					{
						playerLog[chair_id].AddHitRecord(fire_score, 25, score / fire_score, score);
                        fishkindpaihang = 25;
					}
					else if (bking == fktAreaBomb)//局部炸弹的时候传26
					{
						playerLog[chair_id].AddHitRecord(fire_score, 26, score / fire_score, score);
                        fishkindpaihang = 26;
					}
					else//剩余的就是普通鱼了
					{
						if (fishtype <= 18)
						{
							playerLog[chair_id].AddHitRecord(fire_score, fishtype, score / fire_score, score);
                            fishkindpaihang = fishtype;
                        }
						else if (fishtype != 54 && fishtype != 55) //激光炮和钻头炮发射后再记录
						{
							playerLog[chair_id].AddHitRecord(fire_score, fishtype - 1, score / fire_score, score);
                            fishkindpaihang = fishtype-1;
                        }
					}
					if ((score / fire_score >= 50 || score >= m_pGameRoomInfo->broadcastScore) && fishname.compare(""))
					{
						SendBossMessage(chair_id, fishname, score / 10);//跑马灯信息
					}
                    //排行榜存储
                    if ((score / fire_score >= fishCal || score/ 10 >= fishWinScore) )
                    {

                        GSKillFishBossInfo fishboss;
                        fishboss.agentId = pIServerUserItem->GetUserBaseInfo().agentId;
                        fishboss.userId = userid;
                        fishboss.cannonLevel = fire_score;
                        fishboss.fishAdds = score / fire_score;
                        fishboss.fishType = fishkindpaihang;
                        fishboss.winScore = score/ 10;
                        m_pITableFrame->SaveKillFishBossRecord(fishboss);
                    }



					if (CHECK_BOSS(fishKind.life))
					{
						double t = getExchangeUserScore(score);
						LONG tempScore = score;
					}
					if (fishtype != 54 && fishtype != 55)
						onFishCatched(tableId, chair_id, score);
				} 				
			}
			if (is_pause_boom)
            {
				frozenAllFish(chair_id);
#ifdef __MULTI_THREAD__
				lock.Unlock();
#endif
                m_TimerIdPauseBoom=m_TimerLoopThread->getLoop()->runAfter(kPauseBoomTimerElasped,boost::bind(&CTableFrameSink::OnTimerPauseBomb, this));
			}
        }
        else if(hit_fish.hitType == 0)
        {
            playerLog[chair_id].AddHitRecord(fire_score,0,0,0);
        }
    }
	else if (hit_fish.hitType == 0)
	{
        AddFishScoreForPlayer(chair_id, sutReturnedBulletScore, fire_score);
        LOG(WARNING) << "CTableFrameSink::OnSubHitFish chair_id:" << chair_id << ", 鱼无效！ fish_id:" << m_fishIDs[0];
    }

	return true;
}
bool CTableFrameSink::GetResult(PONEBKINFO pList ,tagPersonalProfit currentper,int64_t currentstock,int64_t minstock,int64_t maxstock,int32_t ncount)
{
    float fishProbirity = 0.0f;

    int64_t mutical = 0;
    for(int i=0;i<ncount;i++)
    {
        mutical+=pList[i].nOdds;
    }
    if(mutical!=0)
    {
        fishProbirity = 1.0f/mutical;
    }

    LOG(ERROR) << "个人总戏码量playerALlbet:" << currentper.playerALlbet<<"     判定高低级别戏码量smallOrBigLevelbet: "<<smallOrBigLevelbet;
    if(currentper.playerALlbet<smallOrBigLevelbet)
    {
        fishProbirity=fishProbirity*GetSmallPersonalProbability(currentper.playerAllProfit);
    }
    else
    {
        fishProbirity=fishProbirity*GetPersonalProbability(currentper.playerAllProfit);
    }

    fishProbirity=fishProbirity*GetOverallProbability(currentstock,minstock,maxstock);

    float death = m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
    if(currentper.playerAllProfit>personalProfitLevel[1]||currentstock-minstock<-overallProfit[1])
    {
        //超出第二级的时候适当复活
        if(death<fishProbirity&&mutical>=30)
        {
            death = m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
            int killed = m_random.betweenInt(0,100).randInt_mt(true);
            if(killed<Resurrection)
            {
                death = m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
            }
        }

    }
    if(death<fishProbirity)
    {
        //难度干涉值
        //m_difficulty
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,命中概率系统必须赚钱
        if(randomRet<m_difficulty)
        {
            for(int i=0;i<ncount;i++)
            {
                pList[i].nResult = AICSERR_NOKILL;
            }
            return false;
        }
        else
        {
            for(int i=0;i<ncount;i++)
            {
                pList[i].nResult = AICSERR_KILLED;
            }
            return true;
        }
    }
    for(int i=0;i<ncount;i++)
    {
        pList[i].nResult = AICSERR_NOKILL;
    }
    return false;
}
float CTableFrameSink::GetSmallPersonalProbability(int64_t currentpersomstock)
{

    LOG(ERROR) << "个人库存当前值currentpersomstock:" << currentpersomstock;

    //个人概率控制
    float personalPri = 0.0f;
    int64_t LevelOne  = smallPersonalLevel[0];
    int64_t LevelTwo  = smallPersonalLevel[1];
    int64_t LevelThre = smallPersonalLevel[2];
    int64_t LevelFour = smallPersonalLevel[3];
    LOG(ERROR) << "smallPersonalLevel[0]概率:" << smallPersonalLevel[0];
    LOG(ERROR) << "smallPersonalLevel[1]概率:" << smallPersonalLevel[1];
    LOG(ERROR) << "smallPersonalLevel[2]概率:" << smallPersonalLevel[2];
    LOG(ERROR) << "smallPersonalLevel[3]概率:" << smallPersonalLevel[3];

    LOG(ERROR) << "smallPersonalPribirity[0]概率:" << smallPersonalPribirity[0];
    LOG(ERROR) << "smallPersonalPribirity[1]概率:" << smallPersonalPribirity[1];
    LOG(ERROR) << "smallPersonalPribirity[2]概率:" << smallPersonalPribirity[2];
    LOG(ERROR) << "smallPersonalPribirity[3]概率:" << smallPersonalPribirity[3];
    LOG(ERROR) << "smallPersonalPribirity[4]概率:" << smallPersonalPribirity[4];
    LOG(ERROR) << "smallPersonalPribirity[5]概率:" << smallPersonalPribirity[5];
    LOG(ERROR) << "smallPersonalPribirity[6]概率:" << smallPersonalPribirity[6];
    LOG(ERROR) << "smallPersonalPribirity[7]概率:" << smallPersonalPribirity[7];
    if(currentpersomstock<=-LevelFour)
    {
        personalPri = smallPersonalPribirity[0];
    }
    else if(currentpersomstock<=-LevelThre&&currentpersomstock>-LevelFour)
    {
        personalPri = smallPersonalPribirity[1];

    }
    else if(currentpersomstock<=-LevelTwo&&currentpersomstock>-LevelThre)
    {
        personalPri = smallPersonalPribirity[2];
    }
    else if(currentpersomstock<=-LevelOne&&currentpersomstock>-LevelTwo)
    {
        personalPri = smallPersonalPribirity[3];
    }
    else if(currentpersomstock>-LevelOne&&currentpersomstock<=LevelOne)
    {
        personalPri = fairProbability;
    }
    else if(currentpersomstock>LevelOne&&currentpersomstock<=LevelTwo)
    {
        personalPri = smallPersonalPribirity[4];
    }
    else if(currentpersomstock>LevelTwo&&currentpersomstock<=LevelThre)
    {
        personalPri = smallPersonalPribirity[5];
    }
    else if(currentpersomstock>LevelThre&&currentpersomstock<=LevelFour)
    {
        personalPri = smallPersonalPribirity[6];
    }
    else if(currentpersomstock>LevelFour)
    {
        personalPri = smallPersonalPribirity[7];
    }
    LOG(ERROR) << "当前个人控制系数概率:" << personalPri;
    return personalPri;
}
float CTableFrameSink::GetPersonalProbability(int64_t currentpersomstock)
{

    LOG(ERROR) << "个人库存当前值currentpersomstock:" << currentpersomstock;

    //个人概率控制
    float personalPri = 0.0f;
    int64_t LevelOne  = personalProfitLevel[0];
    int64_t LevelTwo  = personalProfitLevel[1];
    int64_t LevelThre = personalProfitLevel[2];
    int64_t LevelFour = personalProfitLevel[3];
    LOG(ERROR) << "personalProfitLevel[0]概率:" << personalProfitLevel[0];
    LOG(ERROR) << "personalProfitLevel[1]概率:" << personalProfitLevel[1];
    LOG(ERROR) << "personalProfitLevel[2]概率:" << personalProfitLevel[2];
    LOG(ERROR) << "personalProfitLevel[3]概率:" << personalProfitLevel[3];

    LOG(ERROR) << "personalPribirity[0]概率:" << personalPribirity[0];
    LOG(ERROR) << "personalPribirity[1]概率:" << personalPribirity[1];
    LOG(ERROR) << "personalPribirity[2]概率:" << personalPribirity[2];
    LOG(ERROR) << "personalPribirity[3]概率:" << personalPribirity[3];
    LOG(ERROR) << "personalPribirity[4]概率:" << personalPribirity[4];
    LOG(ERROR) << "personalPribirity[5]概率:" << personalPribirity[5];
    LOG(ERROR) << "personalPribirity[6]概率:" << personalPribirity[6];
    LOG(ERROR) << "personalPribirity[7]概率:" << personalPribirity[7];
    if(currentpersomstock<=-LevelFour)
    {
        personalPri = personalPribirity[0];
    }
    else if(currentpersomstock<=-LevelThre&&currentpersomstock>-LevelFour)
    {
        personalPri = personalPribirity[1];

    }
    else if(currentpersomstock<=-LevelTwo&&currentpersomstock>-LevelThre)
    {
        personalPri = personalPribirity[2];
    }
    else if(currentpersomstock<=-LevelOne&&currentpersomstock>-LevelTwo)
    {
        personalPri = personalPribirity[3];
    }
    else if(currentpersomstock>-LevelOne&&currentpersomstock<=LevelOne)
    {
        personalPri = 1.0f;
    }
    else if(currentpersomstock>LevelOne&&currentpersomstock<=LevelTwo)
    {
        personalPri = personalPribirity[4];
    }
    else if(currentpersomstock>LevelTwo&&currentpersomstock<=LevelThre)
    {
        personalPri = personalPribirity[5];
    }
    else if(currentpersomstock>LevelThre&&currentpersomstock<=LevelFour)
    {
        personalPri = personalPribirity[6];
    }
    else if(currentpersomstock>LevelFour)
    {
        personalPri = personalPribirity[7];
    }
    LOG(ERROR) << "当前个人控制系数概率:" << personalPri;
    return personalPri;
}
float  CTableFrameSink::GetOverallProbability(int64_t currentstock,int64_t minstock,int64_t maxstock)
{
    LOG(ERROR) << "历史总库存:" << currentstock
               << ", minstock: "  << (int)minstock
               << ", maxstock:"   << (int)maxstock;

    int64_t LevelOne  = overallProfit[0];
    int64_t LevelTwo  = overallProfit[1];
    int64_t LevelThre = overallProfit[2];
    int64_t LevelFour = overallProfit[3];
    //整体上的控制概率
    float  overallProbability = 0.0f;
    //大于库存上限
    if(currentstock>maxstock)
    {      
        if(currentstock-maxstock>=LevelFour)
        {
            overallProbability = overallProbirity[0];
        }
        else if(currentstock-maxstock>LevelThre&&currentstock-maxstock<LevelFour)
        {
            overallProbability = overallProbirity[0];
        }
        else if(currentstock-maxstock>LevelTwo&&currentstock-maxstock<LevelThre)
        {
            overallProbability = overallProbirity[1];
        }
        else if(currentstock-maxstock>LevelOne&&currentstock-maxstock<LevelTwo)
        {
            overallProbability = overallProbirity[2];
        }
        else
        {
            overallProbability = overallProbirity[3];
        }
    }
    //小于库存下限
    else if(currentstock<minstock)
    {

        if(currentstock-minstock>=-LevelOne)
        {
            overallProbability = overallProbirity[4];
        }
        else if(currentstock-minstock<-LevelOne&&currentstock-minstock>=-LevelTwo)
        {
            overallProbability = overallProbirity[5];
        }
        else if(currentstock-minstock<-LevelTwo&&currentstock-minstock>=-LevelThre)
        {
            overallProbability = overallProbirity[6];
        }
        else if(currentstock-minstock<-LevelThre&&currentstock-minstock>=-LevelFour)
        {
            overallProbability = overallProbirity[7];
        }
        else
        {
            overallProbability = 0.1;
        }
    }
    //在库存以内波动
    else
    {
        overallProbability = fairProbability;
    }
    LOG(ERROR) << "整体控制系数:" << overallProbability;
    return overallProbability ;
}
bool CTableFrameSink::OnSubHitFish_Normal(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_HitFish& hit_fish, unsigned short fish_count, BulletScoreT fire_score, const FishKind* fish_kind, short &catch_fish_count, bool &isBoss, bool &bBossOrGlobalBomb, LONG &score)
{
	BOOL bKilled = FALSE;
	FishKind fishKind = *fish_kind;
	WORD chair_id = pIServerUserItem->GetChairId();
	WORD tableId = m_pITableFrame->GetTableId();
	WORD realChairID = getRealChairID(chair_id);
	int kill_count = 0;
	int i = 0;
    tagStorageInfo tagstorage;
    m_pITableFrame->GetStorageScore(tagstorage);

    tagPersonalProfit& personalCtr = pIServerUserItem->GetPersonalPrott();
	for (i = 0; i < fish_count; ++i)
	{
		fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[i]);
		if (!m_usedFishIDs[m_fishIDs[i]] && fish_kind)
		{
			m_catchFishKindList[m_fishIDs[i]] = *fish_kind;
			m_usedFishIDs[m_fishIDs[i]] = 1;
			m_kill_info[kill_count].nId = m_fishIDs[i];
			m_kill_info[kill_count].nOdds = fish_kind->life;
			m_kill_info[kill_count].nResult = 0;
			if (fishKind.fishType.type != 54 && fishKind.fishType.type != 55)
			{
				if (CHECK_BOSS(fish_kind->life))
				{
					isBoss = true;
					fishKind = *fish_kind;
					if (fishKind.fishType.type == 20)
					{
						bBossOrGlobalBomb = true;
					}
				}
				else if (fish_kind->life >= 25)
				{
					isBoss = true;
				}
			}
			++kill_count;
		}
		else
		{
			LOG(INFO) << "CTableFrameSink::OnSubHitFish. skip fish_id:" << m_fishIDs[i] << ", chair_id:" << chair_id;
		}
	}
	onPlayerFired(tableId, chair_id, fire_score);
	DWORD dwSceneLessTime = scene_mgr.GetLessRunTime();
#ifdef AICS_ENGINE
	if (kill_count > 0)
	{
		int  isHaveBlackPlayer[MAX_PLAYERS] = { 0 };
        int  isBlackAgent[MAX_PLAYERS]={0};
		for (int i = 0;i < MAX_PLAYERS;i++)
		{
			shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
			if (!user || !user->GetBlacklistInfo().status)
			{
				continue;
			}
			if (user->IsAndroidUser()) continue;
			if (user->GetBlacklistInfo().listRoom.size() > 0)
			{
				auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
				if (it != user->GetBlacklistInfo().listRoom.end())
				{
					isHaveBlackPlayer[i] = it->second;
					//openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
					//openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
				}
			}
		}
        for(int i=0;i<MAX_PLAYERS;i++)
        {
             shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(!user)
            {
                continue;
            }
            if(user->IsAndroidUser()) continue;
            int32_t agentId = user->GetUserBaseInfo().agentId;
            for(int i=0;i<controlledProxy.size();i++)
            {
                if(agentId==controlledProxy[i])
                {
                    isBlackAgent[user->GetChairId()]=1;
                }
            }
        }
		if (isHaveBlackPlayer[pIServerUserItem->GetChairId()])
		{
            GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
			//GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
			int scorex = 0;
			int fishcount = 0;
			for (i = 0; i < kill_count; ++i)
			{
				if (AICSERR_KILLED == m_kill_info[i].nResult)
				{
					scorex += m_kill_info[i].nOdds * fire_score;
					fishcount++;
				}
			}

			if (m_kill_info[0].nResult == AICSERR_KILLED)
			{
				if (isHaveBlackPlayer[pIServerUserItem->GetChairId()] > 1000)
				{
					isHaveBlackPlayer[pIServerUserItem->GetChairId()] = 1000;
				}
				double probili = ((double)isHaveBlackPlayer[pIServerUserItem->GetChairId()]) / 1000.0;
				float fr = m_random.betweenFloat(0, 1.0).randFloat_mt(true);
				if (fr < probili)
				{
					for (i = 0; i < kill_count; ++i)
					{
						if (AICSERR_KILLED == m_kill_info[i].nResult)
						{
							m_kill_info[i].nResult = AICSERR_NOKILL;
						}
					}
					kill_count = 0;
					//黑名单玩家打中了即使后面改变结果也照样给他加钱到库存
					if (fishcount)
					{
						onFishCatched(tableId, realChairID, scorex);
					}
				}
			}
		}
        else if(isBlackAgent[pIServerUserItem->GetChairId()])
        {
            GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
            //GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
            int scorex = 0;
            int fishcount = 0;
            for (i = 0; i < kill_count; ++i)
            {
                if (AICSERR_KILLED == m_kill_info[i].nResult)
                {
                    scorex += m_kill_info[i].nOdds * fire_score;
                    fishcount++;
                }
            }

            if (m_kill_info[0].nResult == AICSERR_KILLED)
            {

                float fr = m_random.betweenFloat(0, 100.0).randFloat_mt(true);
                if (fr < controllPoint)
                {
                    for (i = 0; i < kill_count; ++i)
                    {
                        if (AICSERR_KILLED == m_kill_info[i].nResult)
                        {
                            m_kill_info[i].nResult = AICSERR_NOKILL;
                        }
                    }
                    kill_count = 0;
                    //黑名单玩家打中了即使后面改变结果也照样给他加钱到库存
                    if (fishcount)
                    {
                        onFishCatched(tableId, realChairID, scorex);
                    }
                }
            }
        }
		else
            GetResult(m_kill_info,personalCtr,tagstorage.lEndStorage,tagstorage.lLowlimit,tagstorage.lUplimit,kill_count);
			//GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);

		//场景剩下的时间少于3s,打不死
		if (m_isNeedExtendSwitchScene || m_is_nestScene || scene_mgr.IsInSceneTime() == false) //离切换场景还剩少于3s,打不死特殊鱼
		{
			for (i = 0; i < kill_count; ++i)
			{
				if (AICSERR_KILLED == m_kill_info[i].nResult)
				{
					fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_kill_info[i].nId);
					fishKind = *fish_kind;
					if (fishKind.fishType.type == 54 || fishKind.fishType.type == 55)
					{
						m_kill_info[i].nResult = AICSERR_NOKILL;
						LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish. not HitKill:" << dwSceneLessTime << ", fishKind.fishType.type:" << (int32_t)fishKind.fishType.type;
					}
				}
			}
		}
	}
#else
	m_kill_info[0].nResult = AICSERR_KILLED;
#endif
	for (i = 0; i < kill_count; ++i)
	{
		//******** Test Start ********
//        int index = 0;
//		STD::Random r2;
//		index = r2.betweenInt(0, 100).randInt_mt();
//		if (index < 100 && scene_mgr.IsInSceneTime())
//		{
//			m_kill_info[i].nResult = AICSERR_KILLED;
//        }
		//******** Test End ********

		if (AICSERR_KILLED == m_kill_info[i].nResult)
		{
			m_fishIDs[catch_fish_count++] = m_kill_info[i].nId;							
			//是否打死的是激光蟹或者是钻头蟹
			fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_kill_info[i].nId);
			fishKind = *fish_kind;
			if (fishKind.fishType.type == 54) //获得激光炮
			{
				m_fireScore[chair_id] = 0;
				score += m_nBaseLaserOdds * fire_score;
				m_nAddHitFishOdds = 0;
				m_nLessLaserOdds = m_kill_info[i].nOdds - m_nBaseLaserOdds;
				m_bHaveLaser[chair_id] = true;
				m_fireScore[chair_id] = fire_score;
				m_fireAllWinScore[chair_id] = m_nBaseLaserOdds * fire_score;
				m_nAddHitFishOdds = m_nBaseLaserOdds;
				m_nfireMaxOdds = m_kill_info[i].nOdds;
				STD::Random rd;
                int32_t addOdds = rd.betweenInt(-10, 10).randInt_mt();
				m_nfireMaxOdds += addOdds;
				LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish_Normal. m_nfireMaxOdds:" << m_nfireMaxOdds << ", addOdds:" << addOdds << ", dwSceneLessTime:" << dwSceneLessTime;
				m_startFireCountTime = (int32_t)time(nullptr);
				m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
				m_TimeridCountDownFire = m_TimerLoopThread->getLoop()->runAfter(FIRECOUNTTIME, boost::bind(&CTableFrameSink::OnTimerCountDownFire, this, chair_id));
				m_fishProduceDataManager->setbProduceSpecialFire(false);
			}
			else if (fishKind.fishType.type == 55) //获得钻头炮
			{
				m_fireScore[chair_id] = 0;
				score += m_nBaseBorebitOdds * fire_score;
				m_nAddHitFishOdds = 0;
				m_nLessBorebitOdds = m_kill_info[i].nOdds - m_nBaseBorebitOdds;
				m_bHaveBorebit[chair_id] = true;
				m_fireScore[chair_id] = fire_score;
				m_fireAllWinScore[chair_id] = m_nBaseBorebitOdds * fire_score;
				m_nAddHitFishOdds = m_nBaseBorebitOdds;
				m_nfireMaxOdds = m_kill_info[i].nOdds;
				STD::Random rd;
                int32_t addOdds = rd.betweenInt(-10, 10).randInt_mt();
				m_nfireMaxOdds += addOdds;
				LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish_Normal. m_nfireMaxOdds:" << m_nfireMaxOdds << ", addOdds:" << addOdds << ", dwSceneLessTime:" << dwSceneLessTime;
				m_startFireCountTime = (int32_t)time(nullptr);
				m_TimerLoopThread->getLoop()->cancel(m_TimeridCountDownFire);
				m_TimeridCountDownFire = m_TimerLoopThread->getLoop()->runAfter(FIRECOUNTTIME, boost::bind(&CTableFrameSink::OnTimerCountDownFire, this, chair_id));
				m_fishProduceDataManager->setbProduceSpecialFire(false);
				m_bBorebitFireBoom = false;
				m_bBorebitStartRotate = false;
			}
			else
			{
				score += m_kill_info[i].nOdds * fire_score;
			}
			//场景剩下的时间
			if (fishKind.fishType.type == 54)
			{
				dwSceneLessTime = scene_mgr.GetLessRunTime();
				if (dwSceneLessTime < 20 * 1000) //离切换场景还剩少于20s,需延长切换的时间
				{
					m_isNeedExtendSwitchScene = true;
					DWORD dwNeedAddTick = 20 * 1000 - dwSceneLessTime;
					scene_mgr.SetAddTick(dwNeedAddTick);
					LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish. dwSceneLessTime:" << dwSceneLessTime << ", dwNeedAddTick:" << dwNeedAddTick;
				}
			} 
			else if (fishKind.fishType.type == 55)
			{
				dwSceneLessTime = scene_mgr.GetLessRunTime();
				if (dwSceneLessTime < (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000) //离切换场景还剩少于20s,需延长切换的时间
				{					
					m_startExtendSwitchSceneTime = muduo::Timestamp::now();
					DWORD dwNeedAddTick = (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000 - dwSceneLessTime;
					scene_mgr.SetAddTick(dwNeedAddTick);
					LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish. dwSceneLessTime:" << dwSceneLessTime << ", dwNeedAddTick:" << dwNeedAddTick;

					m_isNeedExtendSwitchScene = true;
					m_dwSceneLessTime = (FIRECOUNTTIME + m_nBorebitMaxFireTime) * 1000;
					m_dwNeedAddTick = dwNeedAddTick;
					LOG(WARNING) << " ==== CTableFrameSink::OnSubHitFish.获得钻头炮最大需延长切换场景 dwSceneLessTime:" << dwSceneLessTime << ", m_dwNeedAddTick:" << m_dwNeedAddTick << " ,m_dwSceneLessTime: " << m_dwSceneLessTime;
				}
			}			
		}
	}
	if (catch_fish_count)
	{
		bKilled = TRUE;
	}
	return bKilled;
}

bool CTableFrameSink::OnSubHitFish_Special(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_HitFish& hit_fish, unsigned short fish_count, BulletScoreT fire_score, const FishKind* fish_kind, short &catch_fish_count, bool &isBoss, bool &bBossOrGlobalBomb, bool &is_pause_boom, LONG &score)
{
	BOOL bKilled = FALSE;
	FishKind fishKind = *fish_kind;
	WORD chair_id = pIServerUserItem->GetChairId();
	WORD tableId = m_pITableFrame->GetTableId();
	WORD realChairID = getRealChairID(chair_id);
	int kill_count = 0;
	int i = 0;
	int32_t probili = 45;
	int32_t SpecialKillFishProbili[4] = { 0 };
	if (hit_fish.hitType == 1) //激光炮
	{
		memcpy(SpecialKillFishProbili, m_SpecialKillFishProbili_JG, sizeof(SpecialKillFishProbili));
	} 
	else if (hit_fish.hitType == 2) //钻头炮
	{
		memcpy(SpecialKillFishProbili, m_SpecialKillFishProbili_ZT, sizeof(SpecialKillFishProbili));
	}
	if (m_bBorebitFireBoom)
	{
		SpecialKillFishProbili[0] = 40;
		SpecialKillFishProbili[1] = 60;
		SpecialKillFishProbili[2] = 80;
		SpecialKillFishProbili[3] = 100;
	}
	if (fish_kind->fishType.king > 0) // 特殊鱼，非扩散攻击处理
	{
		if (fish_kind->fishType.king == fktPauseBomb)
		{
			is_pause_boom = true;
		}
		/*if (fish_kind->fishType.king == fktGlobalBomb)
		{
			bBossOrGlobalBomb = true;
		}*/
		int life_sum = 0;
		for (i = 0; i < fish_count; ++i)
		{
			fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[i]);
			if (!m_usedFishIDs[m_fishIDs[i]] && fish_kind)
			{
				m_fishIDs[catch_fish_count++] = m_fishIDs[i];
				m_catchFishKindList[m_fishIDs[i]] = *fish_kind;
				m_usedFishIDs[m_fishIDs[i]] = 1;
				life_sum += fish_kind->life;
			}
			else {
				LOG(INFO) << "CTableFrameSink::OnSubHitFish. skip fish_id:" << m_fishIDs[i] << ", chair_id:" << chair_id;
			}
		}
		m_kill_info[0].nId = m_fishIDs[0];
		m_kill_info[0].nOdds = life_sum;
		m_kill_info[0].nResult = 0;
		kill_count = 1;
		int32_t nBoomOdds = m_bBorebitFireBoom ? 0 : m_nHitFishBoomOdds;
		//onPlayerFired(tableId, chair_id, fire_score);
#ifdef AICS_ENGINE
		int ret = 0;
		//int  isHaveBlackPlayer[MAX_PLAYERS] = { 0 };
		//for (int i = 0;i < MAX_PLAYERS;i++)
		//{
		//	shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
		//	if (!user || !user->GetBlacklistInfo().status)
		//	{
		//		continue;
		//	}
		//	if (user->IsAndroidUser()) continue;
		//	if (user->GetBlacklistInfo().listRoom.size() > 0)
		//	{
		//		auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
		//		if (it != user->GetBlacklistInfo().listRoom.end())
		//		{
		//			if (it->second > 0)
		//			{
		//				isHaveBlackPlayer[i] = it->second;
		//			}
		//			//openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
		//			//openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
		//		}
		//	}
		//}
		//int ret = 0;
		//if (isHaveBlackPlayer[pIServerUserItem->GetChairId()])
		//{
		//	//ret = GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
		//	for (i = 0; i < kill_count; ++i)
		//	{
		//		LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. king > 0 fishID i=" << i << " m_nfireMaxOdds:" << m_nfireMaxOdds << " <<====>> m_nAddHitFishOdds:" << m_nAddHitFishOdds << " + nOdds:" << life_sum;
  //              STD::Random r2;
  //              int randnum = r2.betweenInt(0, 100).randInt_mt();
		//		if (life_sum>=100)
		//		{
		//			probili = SpecialKillFishProbili[0];
		//		} 
		//		else if (life_sum >= 25)
		//		{
		//			probili = SpecialKillFishProbili[1];
		//		}
		//		else if (life_sum >= 10)
		//		{
		//			probili = SpecialKillFishProbili[2];
		//		}
		//		else
		//		{
		//			probili = SpecialKillFishProbili[3];
		//		}
		//		if ((m_nAddHitFishOdds + life_sum) <= (m_nfireMaxOdds - nBoomOdds) && randnum < probili)
		//		{
		//			ret = 1;
		//			m_nAddHitFishOdds += life_sum;
		//			m_kill_info[i].nResult = AICSERR_KILLED;
		//			LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. king > 0 AICSERR_KILLED i=" << i << " m_nAddHitFishOdds:" << m_nAddHitFishOdds;
		//		}
		//	}
		//	if (ret == true)
		//	{
		//		if (isHaveBlackPlayer[pIServerUserItem->GetChairId()] > 1000)
		//		{
		//			isHaveBlackPlayer[pIServerUserItem->GetChairId()] = 1000;
		//		}
		//		double probili = ((double)isHaveBlackPlayer[pIServerUserItem->GetChairId()]) / 1000.0;
		//		float fr = m_random.betweenFloat(0, 1.0).randFloat_mt(true);
		//		if (fr < probili)
		//		{
		//			ret = 0;
		//			int scorex = life_sum * fire_score;
		//			//onFishCatched(tableId, realChairID, scorex);
		//			for (i = 0; i < kill_count; ++i)
		//			{
		//				if (AICSERR_KILLED == m_kill_info[i].nResult)
		//				{
		//					m_nAddHitFishOdds -= life_sum;
		//					m_kill_info[i].nResult = AICSERR_NOKILL;
		//				}
		//			}
		//		}
		//	}
		//}
		//else
		{
			//ret = GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
			for (i = 0; i < kill_count; ++i)
			{
				LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. king > 0 fishID i=" << i << " m_nfireMaxOdds:" << m_nfireMaxOdds << " <<====>> m_nAddHitFishOdds:" << m_nAddHitFishOdds << " + nOdds:" << m_kill_info[i].nOdds;
                STD::Random r2;
                int randnum = r2.betweenInt(0, 100).randInt_mt();
				if (life_sum >= 100)
				{
					probili = SpecialKillFishProbili[0];
				}
				else if (life_sum >= 25)
				{
					probili = SpecialKillFishProbili[1];
				}
				else if (life_sum >= 10)
				{
					probili = SpecialKillFishProbili[2];
				}
				else
				{
					probili = SpecialKillFishProbili[3];
				}
				if ((m_nAddHitFishOdds + life_sum) <= (m_nfireMaxOdds - nBoomOdds) && randnum < probili)
				{
					ret = 1;
					m_nAddHitFishOdds += life_sum;
					LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. king > 0 AICSERR_KILLED i=" << i << " m_nAddHitFishOdds:" << m_nAddHitFishOdds << " life_sum:" << life_sum;
				}
			}
		}
		bKilled = FALSE;

		if (ret == 1) {
			bKilled = TRUE;
			m_kill_info[0].nResult = AICSERR_KILLED;
		}
		//if (m_kill_info[0].nResult == AICSERR_KILLED) {
		//	bKilled = TRUE;
		//}
#else
		bKilled = TRUE;
#endif
		score = life_sum * fire_score;
		fishKind.life = life_sum;
		for (i = 0; i < kill_count; ++i)
		{
			if (AICSERR_KILLED == m_kill_info[i].nResult)
			{
				//m_fishIDs[catch_fish_count++] = m_kill_info[i].nId;
				//score += m_kill_info[i].nOdds * m_fireScore[chair_id];
				if (m_bHaveFireLaser[chair_id])
				{
					m_fireAllWinScore[chair_id] += life_sum * m_fireScore[chair_id];
				}
				else if (m_bHaveFireBorebit[chair_id])
				{
					m_fireAllWinScore[chair_id] += life_sum * m_fireScore[chair_id];
				}
				LOG(WARNING) << " ****** CTableFrameSink::OnSubHitFish_Special. bKilledFishID king > 0 i=" << i << " m_fireAllWinScore[chair_id]:" << m_fireAllWinScore[chair_id] << " m_fireScore:" << m_fireScore[chair_id];
			}
		}
	}
	else // 普通鱼，扩散攻击处理
	{
		for (i = 0; i < fish_count; ++i)
		{
			fish_kind = m_fishProduceDataManager->getFishKindWithFishID(m_fishIDs[i]);
			if (!m_usedFishIDs[m_fishIDs[i]] && fish_kind)
			{
				m_catchFishKindList[m_fishIDs[i]] = *fish_kind;
				m_usedFishIDs[m_fishIDs[i]] = 1;
				m_kill_info[kill_count].nId = m_fishIDs[i];
				m_kill_info[kill_count].nOdds = fish_kind->life;
				m_kill_info[kill_count].nResult = 0;
				if (CHECK_BOSS(fish_kind->life))
				{
					isBoss = true;
					fishKind = *fish_kind;
					/*if (fishKind.fishType.type == 20)
					{
						bBossOrGlobalBomb = true;
					}*/
				}
				else if (fish_kind->life >= 25)
				{
					isBoss = true;
				}
				++kill_count;
			}
			else
			{
				LOG(INFO) << "CTableFrameSink::OnSubHitFish. skip fish_id:" << m_fishIDs[i] << ", chair_id:" << chair_id;
			}
		}
		//onPlayerFired(tableId, chair_id, fire_score);
#ifdef AICS_ENGINE
		if (kill_count > 0)
		{
			STD::Random r2;
			int randnum = 0;
			int32_t nBoomOdds = m_bBorebitFireBoom ? 0 : m_nHitFishBoomOdds;
			//int  isHaveBlackPlayer[MAX_PLAYERS] = { 0 };
			//for (int i = 0;i < MAX_PLAYERS;i++)
			//{
			//	shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
			//	if (!user || !user->GetBlacklistInfo().status)
			//	{
			//		continue;
			//	}
			//	if (user->IsAndroidUser()) continue;
			//	if (user->GetBlacklistInfo().listRoom.size() > 0)
			//	{
			//		auto it = user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
			//		if (it != user->GetBlacklistInfo().listRoom.end())
			//		{
			//			isHaveBlackPlayer[i] = it->second;
			//			//openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
			//			//openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
			//		}
			//	}
			//}
			//if (isHaveBlackPlayer[pIServerUserItem->GetChairId()])
			//{
			//	for (i = 0; i < kill_count; ++i)
			//	{
			//		randnum = r2.betweenInt(0, 100).randInt_mt();
			//		if (m_kill_info[i].nOdds >= 100)
			//		{
			//			probili = SpecialKillFishProbili[0];
			//		}
			//		else if (m_kill_info[i].nOdds >= 25)
			//		{
			//			probili = SpecialKillFishProbili[1];
			//		}
			//		else if (m_kill_info[i].nOdds >= 10)
			//		{
			//			probili = SpecialKillFishProbili[2];
			//		}
			//		else
			//		{
			//			probili = SpecialKillFishProbili[3];
			//		}
			//		if ((m_nAddHitFishOdds + m_kill_info[i].nOdds) <= (m_nfireMaxOdds - nBoomOdds) && randnum < probili)
			//		{
			//			m_nAddHitFishOdds += m_kill_info[i].nOdds;
			//		}
			//	}
			//
			//	if (m_kill_info[0].nResult == AICSERR_KILLED)
			//	{
			//		if (isHaveBlackPlayer[pIServerUserItem->GetChairId()] > 1000)
			//		{
			//			isHaveBlackPlayer[pIServerUserItem->GetChairId()] = 1000;
			//		}
			//		double probili = ((double)isHaveBlackPlayer[pIServerUserItem->GetChairId()]) / 1000.0;
			//		float fr = m_random.betweenFloat(0, 1.0).randFloat_mt(true);
			//		if (fr < probili)
			//		{
			//			for (i = 0; i < kill_count; ++i)
			//			{
			//				if (AICSERR_KILLED == m_kill_info[i].nResult)
			//				{
			//					m_nAddHitFishOdds -= m_kill_info[i].nOdds;
			//					m_kill_info[i].nResult = AICSERR_NOKILL;
			//				}
			//			}
			//		}
			//	}
			//}
			//else
			{
				//GetAicsEngine()->OneBulletKill(tableId, realChairID, pIServerUserItem->GetUserId(), convertToAicsScore(fire_score), m_kill_info, kill_count);
				for (i = 0; i < kill_count; ++i)
				{
					//LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. fishID i=" << i << " m_nfireMaxOdds:" << m_nfireMaxOdds << " <<====>> m_nAddHitFishOdds:" << m_nAddHitFishOdds << " + nOdds:" << m_kill_info[i].nOdds;
					randnum = r2.betweenInt(0, 100).randInt_mt();
					if (m_kill_info[i].nOdds >= 100)
					{
						probili = SpecialKillFishProbili[0];
					}
					else if (m_kill_info[i].nOdds >= 25)
					{
						probili = SpecialKillFishProbili[1];
					}
					else if (m_kill_info[i].nOdds >= 10)
					{
						probili = SpecialKillFishProbili[2];
					}
					else
					{
						probili = SpecialKillFishProbili[3];
					}
					if ((m_nAddHitFishOdds + m_kill_info[i].nOdds) <= (m_nfireMaxOdds - nBoomOdds) && randnum < probili)
					{
						m_nAddHitFishOdds += m_kill_info[i].nOdds;
						m_kill_info[i].nResult = AICSERR_KILLED;
						//LOG(WARNING) << "CTableFrameSink::OnSubHitFish_Special. AICSERR_KILLED i=" << i << " m_nAddHitFishOdds:" << m_nAddHitFishOdds << " ,nBoomOdds:" << nBoomOdds;
					}
				}
			}
		}
#else
		m_kill_info[0].nResult = AICSERR_KILLED;
#endif
		for (i = 0; i < kill_count; ++i)
		{

			//******** Test Start ********
			/*int index = 0;
			STD::Random r2;
			index = r2.betweenInt(0, 100).randInt_mt();
			if (index < 100)
			{
				m_kill_info[i].nResult = AICSERR_KILLED;
			}*/
			//******** Test End ********

			if (AICSERR_KILLED == m_kill_info[i].nResult)
			{
				m_fishIDs[catch_fish_count++] = m_kill_info[i].nId;
				score += m_kill_info[i].nOdds * m_fireScore[chair_id];
				if (m_bHaveFireLaser[chair_id])
				{
					m_fireAllWinScore[chair_id] += m_kill_info[i].nOdds * m_fireScore[chair_id];
				}
				else if (m_bHaveFireBorebit[chair_id])
				{
					m_fireAllWinScore[chair_id] += m_kill_info[i].nOdds * m_fireScore[chair_id];
				}
				LOG(WARNING) << " ****** CTableFrameSink::OnSubHitFish_Special. bKilledFishID i=" << i << " m_fireAllWinScore[chair_id]:" << m_fireAllWinScore[chair_id] << " m_fireScore:" << m_fireScore[chair_id];
			}
		}
		if (catch_fish_count)
		{
			bKilled = TRUE;
		}
	}

	if (hit_fish.hitType == 2 && m_bHaveFireBorebit[chair_id] && !m_bBorebitStartRotate) //激光炮、钻头炮
	{
		double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), m_startFireTime); //经过了多少时间
		int32_t needLeijiOdds = (m_nfireMaxOdds - m_nHitFishBoomOdds - 5);
		//LOG(WARNING) << " ****** CTableFrameSink::OnSubHitFish_Special. m_nAddHitFishOdds " << m_nAddHitFishOdds << " needLeijiOdds:" << needLeijiOdds << " cbtimeleave:" << cbtimeleave << " m_nBorebitMinFireTime:" << m_nBorebitMinFireTime;
		if (m_nAddHitFishOdds >= needLeijiOdds && cbtimeleave >= m_nBorebitMinFireTime) //倍数和时间都已达到
		{
			LOG(WARNING) << " ****** CTableFrameSink::OnSubHitFish_Special. 累计倍数已达到 m_nAddHitFishOdds " << m_nAddHitFishOdds << " cbtimeleave:" << cbtimeleave << " m_nBorebitMinFireTime:" << m_nBorebitMinFireTime;
			OnTimerBorebitStartRotate(chair_id);	//发送开始旋转标志
		}
	}

	return bKilled;
}

bool CTableFrameSink::OnSubAddLevel(shared_ptr<CServerUserItem> pIServerUserItem,const CMD_C_AddLevel& addlevle,bool Sendother)
{
    WORD chairId = pIServerUserItem->GetChairId();
    LOG(INFO) << "OnSubGameplay() chair_id:" << chairId;
    if(addlevle.powerlevel>=game_config.cannonPowerMin&&addlevle.powerlevel<=game_config.cannonPowerMax)
    {
        int bytesize = 0;
        char buffer[1024]={0};
        ::rycsfish::CMD_S_AddLevel addcanno;
        bulletlevel[chairId]=addlevle.powerlevel;
        addcanno.set_cannonpower(addlevle.powerlevel);
        addcanno.set_playerid(pIServerUserItem->GetChairId());
        bytesize = addcanno.ByteSize();
        addcanno.SerializeToArray(buffer,bytesize);
        if(Sendother)
        {
            send_table_data(chairId, INVALID_CHAIR, SUB_S_ADDLEVEL, buffer, bytesize, true,pIServerUserItem->GetChairId());
        }
        else
        {
            send_table_data(chairId, INVALID_CHAIR, SUB_S_ADDLEVEL, buffer, bytesize, true);
        }
    }
    return true;
}
bool CTableFrameSink::OnSubGameplay(shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_Gameplay& gamePlay, const unsigned char* data, WORD dataSize)
{
    WORD chairId = pIServerUserItem->GetChairId();
    LOG(INFO) << "OnSubGameplay() chair_id:" << chairId;
#ifdef __GAMEPLAY_PAIPAILE__
	if (gptPpl == gamePlay.gameplayID) {
		PplData& pplData = game_config.pplData[chairId];
		if (pplData.isPlaying) {
			switch(gamePlay.commandType)
			{
			case gctStep: // 玩家的连击数
				{
					WORD needSize = sizeof(pplData.hits);
					if (dataSize != needSize) {
						ASSERT(false);
						return false;
					}
					memcpy(&pplData.hits, data, needSize);
					sendGameplayData(chairId, INVALID_CHAIR, gptPpl, gctStep, chairId, &pplData.hits, needSize);
				}
				break;
			case gctEnd: // 拍拍乐结束
				{
					if (dataSize != 0) {
						ASSERT(false);
						return false;
					}
					DWORD score;
					if (pplData.hits > m_pplRuntimeData[chairId].life) {
						score = m_pplRuntimeData[chairId].life * m_pplRuntimeData[chairId].bulletScore;
					} else {
						score = pplData.hits * m_pplRuntimeData[chairId].bulletScore;
					}
					sendGameplayData(chairId, INVALID_CHAIR, gptPpl, gctEnd, INVALID_CHAIR, &score, sizeof(score));
					AddFishScoreForPlayer(chairId, sutGameplayPplScore, score, true);
					onFishCatched(m_pITableFrame->GetTableID(), chairId, score);
					if (score > 0) {
						if (IS_GLOBAL_MESSAGE(score)) {
							SendAllRoomMessage(ROOM_MESSAGE_PPL, getNickName(pIServerUserItem), getServerName(), score);
						} else {
							SendRoomMessage(SMT_CUSTOM_CONGRATULATION, ROOM_MESSAGE_PPL, getNickName(pIServerUserItem), getServerName(), score);
						}
					}
                    LOG(INFO) << "*** 玩家:[%d] 拍拍乐结束: 共拍打次数[%d] 炮大小[%d] 应得分数[%d] 实得分数[%d]"),
						chairId, pplData.hits, m_pplRuntimeData[chairId].bulletScore,
						m_pplRuntimeData[chairId].life * m_pplRuntimeData[chairId].bulletScore, score);
					resetPplData(chairId);
				}
				break;
			}
		}
	}
#endif
	return true;
}

void CTableFrameSink::AddFishScoreForPlayer(WORD chairId, ScoreUpType type, LONG score, WORD exclude_chair_id)
{
	game_config.fishScore[chairId] += score;
	CMD_S_ScoreUp cmdData = {0};
	cmdData.playerID = chairId;
    cmdData.score = score;
	cmdData.playerScore = game_config.fishScore[chairId];
	cmdData.type = type;
	cmdData.specialScore = m_fireAllWinScore[chairId];
	unsigned short dataSize = NetworkUtility::encodeScoreUp(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chairId, INVALID_CHAIR, SUB_S_SCORE_UP, m_send_buffer, dataSize, true, exclude_chair_id);
    /*LOG(ERROR) << "AddFishScoreForPlayer() chair_id:" << chairId << ",type:" << type
			   << ", exchangeFishScore:" << game_config.exchangeFishScore[chairId]  << ",Addscore:" << score << ", fishScore:" << game_config.fishScore[chairId];*/
}


#ifdef __USE_LIFE2__
void CTableFrameSink::AddFishScoreForPlayer2(WORD chairId, ScoreUpType type, LONG score, LONG score2)
{
    LOG(INFO) << "AddFishScoreForPlayer2() chair_id:[%d],type:[%d],score:[%d]"),chairId, type, score);
	if (score2 == 0)
	{
		AddFishScoreForPlayer(chairId, type, score);
		return;
	}
	CMD_S_ScoreUp2 cmdData = {0};
	cmdData.playerID = chairId;
	cmdData.score = score - score2;
	cmdData.score2 = score2;
	cmdData.type = type;
	game_config.fishScore[chairId] += cmdData.score;
	game_config.fishScore2[chairId] += cmdData.score2;
	unsigned short dataSize = NetworkUtility::encodeScoreUp2(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chairId, INVALID_CHAIR, SUB_S_SCORE_UP2, m_send_buffer, dataSize, true);
	writeArithmLog2(chairId, cmdData.score2);
}
#endif


// 玩家正在飞没有打中鱼的子弹,需要把积分返回给玩家.
void CTableFrameSink::doReturnBulletScore(WORD chair_id)
{
    LOG(INFO) << "doReturnBulletScore() chair_id:" << chair_id;
	// 子弹返还
	if (m_bulletCount[chair_id] > 0) {
		BulletScoreT totalBulletScore = 0;
		for (int i = 0; i < MAX_BULLETS; ++i) {
			if (m_bullets[chair_id][i] > 0) {
				totalBulletScore += m_bullets[chair_id][i];
			}
		}
		AddFishScoreForPlayer(chair_id, sutReturnedBulletScore, totalBulletScore, chair_id);
		//game_config.fishScore[chair_id] += totalBulletScore;
        WORD table_id = m_pITableFrame->GetTableId();
        LOG(INFO) << "CTableFrameSink::SendGameScene. 子弹返还. table_id:" << table_id << ", chair_id:" << chair_id <<
                    ", bulletCount:" << m_bulletCount[chair_id] << ", bulletScore:" << totalBulletScore;
	}
	m_bulletCount[chair_id] = 0;
	memset(m_bullets[chair_id], 0, sizeof(m_bullets[chair_id]));
    //log_bullets("     doReturnBulletScore, chair_id:[%d], reset bulletcount to 0.",chair_id);
}

// OnTimer to synchor user score value now.
bool CTableFrameSink::OnTimerSynchorScore()
{
    bool bSuccess = false;
    do
    {
        //for (WORD i = 0; i < MAX_PLAYERS; ++i)
        //{
        //    // loop to synchor the special user item data value content now.
        //    shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(i);
        //    if (user_item == NULL)
        //    continue;
        //    writeUserInfo(i);
        //}
        // loop to synchor all the player score value item for current now.
        for (WORD i = 0; i < MAX_PLAYERS; ++i)
        {
            // loop to synchor the special user item data value content now.
            shared_ptr<CServerUserItem> user_item = m_pITableFrame->GetTableUserItem(i);
            if (user_item == NULL)
            continue;			  
			CalcScore(user_item, true);
        }

    } while (0);
//Cleanup:
    return (bSuccess);
}

void CTableFrameSink::CalcScore(std::shared_ptr<CServerUserItem> pIServerUserItem, bool bSynchorOnly)
{
    double score = 0;
    WORD chair_id = pIServerUserItem->GetChairId();
    LOG(INFO) << "CalcScore() chair_id:" << chair_id;
	if (pIServerUserItem == NULL) return;
	doReturnBulletScore(chair_id);
	if (game_config.isExperienceRoom)
    {
		m_exp_user_score[chair_id] = exchangeAmountForExperienceRoom;
#ifdef __USE_LIFE2__
		writeArithmLog1(chair_id);
		writeArithmLog2(chair_id, 0);
#endif
        //writeUserScore(chair_id, 0);
	}
    else
    {
#ifdef __USE_LIFE2__
		SCORE score = getExchangeUserScore(game_config.fishScore[chair_id] - game_config.exchangeFishScore[chair_id]);
		SCORE ingot = getExchangeUserIngot(game_config.fishScore2[chair_id] - game_config.exchangeFishScore2[chair_id]);
		writeArithmLog1(chair_id);
		writeArithmLog2(chair_id, 0);
		writeUserScore(chair_id, score, ingot);
#else
        score = getExchangeUserScore((game_config.fishScore[chair_id] - game_config.exchangeFishScore[chair_id])*PALYER_SCORE_SCALE);
        writeUserInfo(chair_id);
        writeUserScore(chair_id, score);
        playerScore[chair_id]=game_config.fishScore[chair_id];
        game_config.exchangeFishScore[chair_id]=game_config.fishScore[chair_id];
#endif
	}
#ifdef AICS_ENGINE

#endif
    if (bSynchorOnly)
    {
        // add by James 181220-.
        long long loldFishScore = game_config.fishScore[chair_id];
        //game_config.fishScore[chair_id] = game_config.exchangeFishScore[chair_id];
        LOG(WARNING) << "CTableFrameSink::CalcScore SynchorOnly chairid:" << chair_id
                     << ", updated score:" << score << ", loldFishScore:" << loldFishScore
                     << ", reseted exchangeFishScore:" << game_config.exchangeFishScore[chair_id];
    }
    else
    {
        // try to call the special user prefs standup now.
        if (/*m_userPrefs &&*/ !game_config.isExperienceRoom)
        {
            UserPrefs::Instance().onUserStandup(pIServerUserItem, m_pITableFrame->GetGameRoomInfo()->roomId);
        }
#ifdef AICS_ENGINE
#ifdef __DYNAMIC_CHAIR__
        char info[1024]={0};
        //snprintf(info,sizeof(info),"玩家昵称=%s,玩家ID=%d,房间号=%d,椅子号=%d)玩家起立!!!",
        //         pIServerUserItem->GetNickName(), pIServerUserItem->GetUserId(),
        //         m_pITableFrame->GetTableId(), game_config.dynamicChair[chair_id]);
        //GetAicsEngine()->OnUserStandup(pIServerUserItem->GetUserId(), pIServerUserItem->GetTableId(), game_config.dynamicChair[chair_id], FALSE, FALSE);
        //resetDynamicChair(chair_id);
        //LOG(INFO) << info;
#else
        //LOG(INFO) << "玩家昵称=%s,玩家ID=%d,房间号=%d)玩家起立!!!"),
        //    pIServerUserItem->GetNickName(), pIServerUserItem->GetUserID(), game_service_option_->wServerID);
        GetAicsEngine()->OnUserStandup(pIServerUserItem->GetUserId(), m_pITableFrame->GetGameRoomInfo()->roomId, pIServerUserItem->GetChairId(), FALSE, FALSE);
#endif
#endif
        // update the special fish score now.
        game_config.fishScore[chair_id] = 0;
#ifdef __USE_LIFE2__
        game_config.fishScore2[chair_id] = 0;
        game_config.exchangeFishScore2[chair_id] = 0;
#endif
        game_config.exchangeFishScore[chair_id] = 0;
    	//memset(&m_playerScoreInfo[chair_id], 0, sizeof(m_playerScoreInfo[chair_id]));
    }
}

void CTableFrameSink::frozenAllFish(WORD chair_id)
{
    LOG(INFO) << "frozenAllFish() chair_id:" << chair_id;
	m_fishProduceDataManager->pause(GetTickCount());
	CMD_S_PauseFish fish_pause = {0};
	fish_pause.pause = 1;
	WORD dataSize = NetworkUtility::encodePauseFish(m_send_buffer, CMD_BUFFER_SIZE_MAX, fish_pause);
	send_table_data(chair_id, INVALID_CHAIR, SUB_S_FISH_PAUSE, m_send_buffer, dataSize, true);
}

#ifdef __USE_LIFE2__
void CTableFrameSink::writeUserScore(WORD chair_id, SCORE score, SCORE ingot)
{
    LOG(INFO) << "writeUserScore() chair_id:[%d],score:[%d],ingot:[%d]"),chair_id,(int)score,(int)ingot);
	if (game_config.isExperienceRoom) {
		return;
	}
	if (score || ingot)	{
		tagScoreInfo score_info = {0};
		score_info.cbType = SCORE_TYPE_WIN;
		score_info.lScore = score;
        LOG(INFO) << "***CTableFrameSink::writeUserScore[%d,%.03f,%.03f]***"), chair_id, score, ingot);
		m_pITableFrame->WriteUserScore(chair_id, score_info, ingot);
	}
}
#endif

void CTableFrameSink::writeUserScore(WORD chair_id, double score)
{
    LOG(ERROR) << "writeUserScore() chair_id:" << chair_id << ",score:" << score << ", isExperienceRoom:" << (int)game_config.isExperienceRoom;
	if (game_config.isExperienceRoom)
	{
        score = 0;
	}
    tagUserPrefs* prefs = NULL;
    // get the special config id content item data.
    int configId = m_pITableFrame->GetGameRoomInfo()->roomId;
    shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(chair_id);
    if (pServerUserItem) {
        DWORD userid = pServerUserItem->GetUserId();
        prefs = UserPrefs::Instance().getUserPrefs(userid,configId);
    }
    long long userscore=pServerUserItem->GetUserScore();
    // gain score now.
    double nPlayerGain = 0;
    double nPlayerLost = 0; // swith gain lost.
    if (prefs) nPlayerGain = prefs->sumLost;
    if (prefs) nPlayerLost = prefs->sumGain;
    //lostScore[chair_id]=prefs->sumLost-lostScore[chair_id];
    //sumScore[chair_id]=prefs->sumGain-sumScore[chair_id];
    double gainsScore=nPlayerLost-lostScore[chair_id];
    lostScore[chair_id]=nPlayerLost;
    double   fscore=getExchangeUserScore((game_config.fishScore[chair_id]-playerScore[chair_id])*PALYER_SCORE_SCALE);

    tagScoreInfo score_info ;
    score_info.chairId = chair_id;
    if(fscore<0&&fscore*PALYER_SCORE_SCALE<-userscore)
    {
        fscore=-userscore;
    }
    else
    {
        fscore*=PALYER_SCORE_SCALE;
    }
    score_info.addScore = fscore;
    //score_info. = true; // James - broadcast player changed.
    // score_info.nRevenue = m_pITableFrame->CalculateRevenue(chair_id, score);
    score_info.revenue = 0; // m_pITableFrame->CalculateRevenue(chair_id, nPlayerLost);
    //score_info. = (nPlayerLost * game_config.revenue_ratio); // set same - modify by James 181112.
    //score_info.nAgentRevenueScore = m_pITableFrame->CalculateAgentRevenue(chair_id, nGain);
    //score_info.nRevenuedScore = score - score_info.nRevenue;
    score_info.betScore = gainsScore*PALYER_SCORE_SCALE;  // set system lost;
    score_info.rWinScore = gainsScore*PALYER_SCORE_SCALE;
    score_info.startTime = m_startTime;
    LOG(ERROR) << "***CTableFrameSink::writeUserScore chairid:" << chair_id << "score:" << score << "revenue:"
               << score_info.revenue ;
    string rav="";
    printf("------------%s------------",strRoundID[chair_id].c_str());
     score_info.fishRake = (fishRake*score_info.betScore)/1000;
     tagPersonalProfit& personalBefore = pServerUserItem->GetPersonalPrott();
     LOG(ERROR) << "写库存前玩家库存 :" <<personalBefore.playerAllProfit;
    if(score_info.betScore>0)
    {
        m_pITableFrame->WriteUserScore(&score_info,1,strRoundID[chair_id]);
        int64_t res =0;
        int32_t StockWeak = 7;
        if(score_info.addScore<0)
        {
           res= score_info.addScore*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
        }
        else
        {
           res=score_info.addScore;
        }

        m_pITableFrame->UpdateStorageScore(-res);
    }
    
    strRoundID[chair_id]=m_pITableFrame->GetNewRoundId();
    do
    {
		if(OldstrRoundID[chair_id].compare(strRoundID[chair_id])==0)
		{
			strRoundID[chair_id]=m_pITableFrame->GetNewRoundId();
		}
		else
		{
			OldstrRoundID[chair_id]=strRoundID[chair_id];
			break;
		}
		strRoundID[chair_id]=m_pITableFrame->GetNewRoundId();
    }while(true);
    do
    {
        bool exis=false;
		for (int i = 0;i < 4;i++)
		{
			if (i == chair_id) continue;
			if (strRoundID[chair_id].compare(strRoundID[i]) == 0)
			{
				exis = true;
			}
		}
		if (!exis) {
			OldstrRoundID[chair_id] = strRoundID[chair_id];
			break;
		}
		strRoundID[chair_id] = m_pITableFrame->GetNewRoundId();
    }while(true);
    m_startTime=chrono::system_clock::now();

	LOG(ERROR) << "下注金额 :" << score_info.betScore;
	LOG(ERROR) << "个人库存衰减值 :" << score_info.fishRake;
	LOG(ERROR) << "个人库存衰减系数 :" << fishRake;
	LOG(ERROR) << "本次玩家赢分 :" << score_info.addScore;
	LOG(ERROR) << "局号 :" << m_pITableFrame->GetNewRoundId();

	tagPersonalProfit& personalCtr = pServerUserItem->GetPersonalPrott();

	LOG(ERROR) << "写库存后玩家个人库存 :" << personalCtr.playerAllProfit;

}
void CTableFrameSink::writeUserInfo1(int chairid)
{
    if(game_config.isExperienceRoom)
    {
        return;
    }
    for(int i=0;i<MAX_PLAYERS;i++)
    {
        shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if(!pServerUserItem)
        {
            continue;
        }
        tagUserPrefs* prefs = NULL;
        // get the special config id content item data.
        int configId = m_pITableFrame->GetGameRoomInfo()->roomId;
        if (pServerUserItem) {
            DWORD userid = pServerUserItem->GetUserId();
            prefs = UserPrefs::Instance().getUserPrefs(userid,configId);
        }
        // gain score now.
        double nPlayerGain = 0;
        double nPlayerLost = 0; // swith gain lost.
        if (prefs) nPlayerGain = prefs->sumLost;
        if (prefs) nPlayerLost = prefs->sumGain;
        //lostScore[chair_id]=prefs->sumLost-lostScore[chair_id];
        //sumScore[chair_id]=prefs->sumGain-sumScore[chair_id];
        double gainsScore=nPlayerLost-lostScore[i];
        double score = getExchangeUserScore((game_config.fishScore[i] - game_config.exchangeFishScore[i])*PALYER_SCORE_SCALE);
        double  fscore=getExchangeUserScore((game_config.fishScore[i]-playerScore[i])*PALYER_SCORE_SCALE);
        m_replay.addResult(i, 0, gainsScore*PALYER_SCORE_SCALE, fscore*PALYER_SCORE_SCALE, "", false);
        if(pServerUserItem)
        {
            m_replay.addPlayer(pServerUserItem->GetUserId(), pServerUserItem->GetAccount(), pServerUserItem->GetUserScore(), i);
        }
        m_replay.gameinfoid=strRoundID[chairid];
    }
    m_pITableFrame->SaveReplay(m_replay);
    m_replay.clear();
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
}
void CTableFrameSink::writeUserInfo(int chairid)
{
    if(game_config.isExperienceRoom)
    {
        return;
    }
    tagUserPrefs* prefs = NULL;
    // get the special config id content item data.
    int configId = m_pITableFrame->GetGameRoomInfo()->roomId;
    shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(chairid);
    if (pServerUserItem) {
        DWORD userid = pServerUserItem->GetUserId();
        prefs = UserPrefs::Instance().getUserPrefs(userid,configId);
    }
    // gain score now.
    double nPlayerGain = 0;
    double nPlayerLost = 0; // swith gain lost.
    if (prefs) nPlayerGain = prefs->sumLost;
    if (prefs) nPlayerLost = prefs->sumGain;
    //lostScore[chair_id]=prefs->sumLost-lostScore[chair_id];
    //sumScore[chair_id]=prefs->sumGain-sumScore[chair_id];
    double gainsScore=nPlayerLost-lostScore[chairid];
    double score = getExchangeUserScore((game_config.fishScore[chairid] - game_config.exchangeFishScore[chairid])*PALYER_SCORE_SCALE);
    double  fscore=getExchangeUserScore((game_config.fishScore[chairid]-playerScore[chairid])*PALYER_SCORE_SCALE);
	//设置当局游戏详情
	if (pServerUserItem && playerLog[chairid].m_mPaoInfo.size()>0)
		SetGameRecordDetail(pServerUserItem->GetUserId(), chairid, pServerUserItem->GetUserScore(), fscore*PALYER_SCORE_SCALE, playerLog[chairid]);
    string fishlog="";
    playerLog[chairid].GetRecordStr(fishlog);
    m_replay.addResult(chairid, 0, gainsScore*PALYER_SCORE_SCALE, fscore*PALYER_SCORE_SCALE, fishlog, false);
    if(pServerUserItem)
    {
        m_replay.addPlayer(pServerUserItem->GetUserId(), pServerUserItem->GetAccount(), pServerUserItem->GetUserScore(), chairid);		
    }
    m_replay.gameinfoid=strRoundID[chairid];
    m_pITableFrame->SaveReplay(m_replay);
    m_replay.clear();
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
}
void CTableFrameSink::OnAddScore(const char* user_id, WORD table_id, ULONG64 score)
{
    LOG(INFO) << "OnAddScore() user_id:" << user_id  << ", table_id:" << table_id << ", score:" << score;
 	//if (m_i_log_db_engine)
 	//{
 	//	USES_CONVERSION;
 	//	m_i_log_db_engine->WriteAddScore(VENDOR_ID, PRODUCT_ID, game_service_option_->wServerID, table_id, A2T(user_id), TRUE, score, score / game_config.exchange_ratio);
 	//}
}

void CTableFrameSink::OnSubScore(const char* user_id, WORD table_id, ULONG64 score)
{
    LOG(INFO) << "OnSubScore() user_id:" << user_id << ",table_id:" << table_id << ",score:" << score;
 	//if (m_i_log_db_engine)
 	//{
 	//	USES_CONVERSION;
 	//	m_i_log_db_engine->WriteAddScore(VENDOR_ID, PRODUCT_ID, game_service_option_->wServerID, table_id, A2T(user_id), FALSE, score, score / game_config.exchange_ratio);
 	//}
}

void CTableFrameSink::sendGameplayData(WORD me_chair_id, WORD chairId, BYTE gameplayId, BYTE commandType,
									  WORD excludeChairId/* = INVALID_CHAIR*/, void* addiData/* = NULL*/, WORD addiDataSize/* = 0*/)
{
    LOG(INFO) << "sendGameplayData() me_chair_id:" << me_chair_id << ", chair_id:" << chairId << ", gameplayId:" << gameplayId << ", commandType:" << commandType;
	CMD_S_Gameplay cmdData = {0};
	cmdData.playerID = me_chair_id;
	cmdData.c.gameplayID = gameplayId;
	cmdData.c.commandType = commandType;
	unsigned short dataSize;
    if (!addiData || !addiDataSize)
    {
		dataSize = NetworkUtility::encodeGameplay(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
		send_table_data(me_chair_id, chairId, SUB_S_GAMEPLAY, m_send_buffer, dataSize, true, excludeChairId);
    }
	else
    {
		dataSize = NetworkUtility::encodeGameplay(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
		memcpy(m_send_buffer + dataSize, addiData, addiDataSize);
		send_table_data(me_chair_id, chairId, SUB_S_GAMEPLAY, m_send_buffer, dataSize + addiDataSize, true, excludeChairId);
	}
}

void CTableFrameSink::resetRumtimeWashData(WORD chairId)
{
    LOG(INFO) << "resetRumtimeWashData() chair_id:" << chairId;
	WORD realChairID = getRealChairID(chairId);
    if (m_washXmlConfig.washDataCount > 0)
    {
		int index = Random::getInstance()->RandomInt(0, m_washXmlConfig.washDataCount - 1);
		m_rumtimeWashData[realChairID].arrivalAmount = Random::getInstance()->RandomInt((LONG)m_washXmlConfig.washData[index].amountMin, (LONG)m_washXmlConfig.washData[index].amountMax);
        //LOG(WARNING)<<"resetRumtimeWashData()  --->randomint()";
        m_rumtimeWashData[realChairID].currentAmount = 0;
	} 
	else {
		memset(&m_rumtimeWashData[realChairID], 0, sizeof(m_rumtimeWashData[realChairID]));
	}
	onWashUpdated(chairId, 0);
	m_washPercent[realChairID] = 0;
}

LONG CTableFrameSink::getAwardFromArithPool()
{
    LOG(INFO) << "getAwardFromArithPool()";
	LONG award = 0;
    GetAicsEngine()->GetGainLostEx(m_pITableFrame->GetGameRoomInfo()->roomId, NULL, NULL, NULL, &award);
    LOG(INFO) << "*** 从彩金池获取彩金：可用数:" << award;
	award *= m_washXmlConfig.percent * 0.01f;
	award = award / m_coinRatioScore * m_coinRatioCoin;
    if (award < m_washXmlConfig.minimumAwardMin)
    {
		award = Random::getInstance()->RandomInt(m_washXmlConfig.minimumAwardMin, m_washXmlConfig.minimumAwardMax);
        LOG(INFO) << "*** *** 从彩金池获取彩金：可用数不足[" << m_washXmlConfig.minimumAwardMin
                 << "]，重新计算后的彩金大小[" << m_washXmlConfig.minimumAwardMin << "~"
                 << m_washXmlConfig.minimumAwardMax << "]:[" << award<< "]";
	}
	return award;
}

void CTableFrameSink::onPlayerFired(WORD tableID, WORD chairId, int fireScore)
{
    //LOG(INFO) << "onPlayerFired() table_id:" << tableID << ",chair_id:" << chairId << ",fireScore:" << fireScore;
	WORD realChairID = getRealChairID(chairId);
	int score = convertToAicsScore(fireScore);
#ifdef AICS_ENGINE
	//GetAicsEngine()->OnPlayerFired(tableID, realChairID, score);
#endif
    // try to update the special player pefs content.
    if (/*m_userPrefs &&*/ !game_config.isExperienceRoom)
	{
        UserPrefs::Instance().onUserFired(m_pITableFrame->GetTableUserItem(chairId)->GetUserId(), m_pITableFrame->GetGameRoomInfo()->roomId, fireScore);
	}
	if (m_rumtimeWashData[realChairID].currentAmount < m_rumtimeWashData[realChairID].arrivalAmount) {
		m_rumtimeWashData[realChairID].currentAmount += fireScore / game_config.ticketRatio;
		if (m_rumtimeWashData[realChairID].currentAmount >= m_rumtimeWashData[realChairID].arrivalAmount) {
			onWashUpdated(chairId, 100);
			onWashArrived(chairId);
		} else {
			float percent = (float)m_rumtimeWashData[realChairID].currentAmount / m_rumtimeWashData[realChairID].arrivalAmount * 100;
			onWashUpdated(chairId, static_cast<BYTE>(percent));
		}
	}
}

void CTableFrameSink::onFishCatched(WORD tableID, WORD chairId, LONG score)
{
    LOG(WARNING) << "onFishCatched() table_id:" << tableID << ",chair_id:" << chairId << ",fireScore:" << score;
	WORD realChairID = getRealChairID(chairId);
    double aicsScore  = convertToAicsScore(score);
#ifdef AICS_ENGINE
    //GetAicsEngine()->OnFishCatched(tableID, realChairID, aicsScore);
#endif
    // try to call the special prefs on fish catched.
    if (/*m_userPrefs &&*/ !game_config.isExperienceRoom)
	{
        UserPrefs::Instance().onFishCatched(m_pITableFrame->GetTableUserItem(chairId)->GetUserId(), m_pITableFrame->GetGameRoomInfo()->roomId, score);
	}
}

void CTableFrameSink::sendWashPercentData(WORD chairId, BYTE percent)
{
    LOG(INFO) << "sendWashPercentData() chair_id:" << chairId << ",percent:" << percent;
	CMD_S_WashPercent cmdData = {0};
	cmdData.percent = percent;
	cmdData.playerID = chairId;
	unsigned short dataSize = NetworkUtility::encodeWashPercent(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
	send_table_data(chairId, INVALID_CHAIR, SUB_S_WASH_PERCENT, m_send_buffer, dataSize, true);
}

void CTableFrameSink::onWashUpdated(WORD chairId, BYTE percent)
{
    LOG(INFO) << "onWashUpdated() chair_id:" << chairId << ",percent:" << percent;
	WORD realChairID = getRealChairID(chairId);
	if (m_washPercent[realChairID] != percent) {
		m_washPercent[realChairID] = percent;
		sendWashPercentData(chairId, percent);
	}
}

void CTableFrameSink::onWashArrived(WORD chairId)
{
    LOG(INFO) << "onWashArrived() chair_id:" << chairId;
	WORD realChairID = getRealChairID(chairId);
#ifdef __GAMEPLAY_MSYQ__
	sendGameplayData(chairId, INVALID_CHAIR, gptMsyq, gctStart);
	m_msyqSumAward[realChairID] = 0;
	m_msyqAward[realChairID] = getAwardFromArithPool();
    LOG(INFO) << "*** *** 玩家[%d]马上有钱任务开始，从彩金池获取彩金[%d]币."), realChairID, m_msyqAward[realChairID]);
	m_msyqAverageAward[realChairID] = m_msyqAward[realChairID] / m_msyqCopies;
	if (m_msyqAverageAward[realChairID] < 1) {
		m_msyqAverageAward[realChairID] = 1;
	}
#endif
}

void CTableFrameSink::readXmlWashData(CXmlParser& xml)
{
    LOG(INFO) << "readXmlWashData()";
	if (xml.Sel("wash")) {
		int i;
		m_washXmlConfig.percent = xml.GetLong("percent");
		m_washXmlConfig.washDataCount = 0;
		const char* str = xml.GetString("amount_string");
		CTextParserUtility txtParser0;
		if (str && strlen(str)) {
			CTextParserUtility txtParser1;
			txtParser0.Parse(str, ",");
			for (i = 0; i < txtParser0.Count() && m_washXmlConfig.washDataCount < MAX_WASH_COUNT; ++i) {
				if (strlen(txtParser0[i])) {
					txtParser1.Parse(txtParser0[i], "~");
					m_washXmlConfig.washData[m_washXmlConfig.washDataCount].amountMin = atoi(txtParser1[0]);
					m_washXmlConfig.washData[m_washXmlConfig.washDataCount].amountMax = atoi(txtParser1[1]);
					++m_washXmlConfig.washDataCount;
				}
			}
		}
		str = xml.GetString("minimum_award_string");
		if (str && strlen(str)) {
			txtParser0.Parse(str, "~");
			m_washXmlConfig.minimumAwardMin = m_washXmlConfig.minimumAwardMax = atoi(txtParser0[0]);
			if (txtParser0.Count() > 1) {
				m_washXmlConfig.minimumAwardMax = atoi(txtParser0[1]);
			}
		}
		for (i = 0; i < MAX_PLAYERS; ++i) {
			resetRumtimeWashData(i);
		}
		xml.Sel("..");
	}
}

void CTableFrameSink::SendRoomMessage(WORD type, LPCTSTR format, ...)
{
    LOG(INFO) << "SendRoomMessage() type:" << type << ", format:" << format;
    // experience room no send message.
    if (game_config.isExperienceRoom){
        return;
    }
    va_list va;
    shared_ptr<ITableFrame> frame;
    TCHAR szFormat[1024];
	// build the list data.
	va_start(va, format);
	_vstprintf(szFormat, format, va);
	va_end(va);
	TRACE_INFO(szFormat);
	for (WORD i = 0; i < tableFrameList.size(); ++i) {
		if (frame = tableFrameList.at(i)) {
            //frame->SendGameMessage(INVALID_CHAIR, szFormat, type);----
		}
	}
}

void CTableFrameSink::SendGlobalRoomMessage(WORD type, LPCTSTR format, ...)
{
    LOG(ERROR) << "SendGlobalRoomMessage() type:" << type << ", format:" << format;
	va_list va;
	TCHAR szFormat[1024];
	// build the list data.
	va_start(va, format);
    vsnprintf(szFormat, sizeof(szFormat), format, va);
	va_end(va);
    // send the special game message to the content item.
    //m_pITableFrame->SendGameMessage(INVALID_CHAIR, szFormat, type); //type=SMT_GLOBAL|SMT_SCROLL-----
    /*
	TRACE_INFO(szFormat);
	for (WORD i = 0; i < tableFrameList.size(); ++i) {
		if (frame = tableFrameList.at(i)) {
            frame->SendGameMessage(INVALID_CHAIR, szFormat, type);
		}
	}
    */
}
void CTableFrameSink::SendBossMessage(int chairid, string fishname,int64_t score)//跑马灯信息
{
    m_pITableFrame->SendGameMessage(chairid, fishname,SMT_GLOBAL|SMT_SCROLL, score);
}
void CTableFrameSink::SendGlobalRoomMessage(shared_ptr<CServerUserItem> pIServerUserItem, WORD type, LPCTSTR format, ...)
{
    LOG(INFO) << "SendGlobalRoomMessage() type:" << type <<", format:" << format;
	va_list va;
	TCHAR szFormat[1024];
	// build the list data.
	va_start(va, format);
    vsnprintf(szFormat, sizeof(szFormat), format, va);
	va_end(va);
//Cleanup:
	TRACE_INFO(szFormat);
   // m_pITableFrame->SendGameMessage(pIServerUserItem->GetChairId(), szFormat, type);----
}

LPCTSTR CTableFrameSink::getNickName(shared_ptr<CServerUserItem> pIServerUserItem)
{
    static char nickName[32]={0};
    LOG(INFO) << "getNickName() nickName:" << pIServerUserItem->GetNickName();
    /*if (pIServerUserItem) {
        return NickNameUtility::convertNickName(pIServerUserItem->GetNickName(),nickName, sizeof(nickName));
    } else */{
		return _T("");
	}
}

void CTableFrameSink::writeLog(const char* lpszFormat, ...)
{
    if (false == CLogflags::Singleton().isOn()) return;
    va_list va;
    time_t now;
    timeval tv;
    struct tm *st = 0;
    TCHAR szFormat[1024]=TEXT("");
    TCHAR szResult[4096]=TEXT("");
    // build the list data.
    va_start(va,lpszFormat);
    vsnprintf(szFormat,sizeof(szFormat),lpszFormat,va);
    va_end(va);
    time(&now);
    gettimeofday(&tv,NULL);
    st = localtime(&now);
    int millisecond = (((tv.tv_sec*1000)+(tv.tv_usec/1000))/100000000);
    // build the full time string value time string value content item value content data now.
    snprintf(szResult,sizeof(szResult),_T("%02d/%02d %02d:%02d:%02d.%03d,pid=%d,tid=%d\t%s\n"),
        st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
        st->tm_sec,millisecond,(int)getpid(),
        (int)pthread_self(),
        szFormat);
    // try to write the special content value.
    FILE* fp = fopen("FishServer.log", "a");
	if (fp)
	{
		fputs(szResult, fp);
		fclose(fp);
	}
}

void CTableFrameSink::SendAllRoomMessage(LPCTSTR format, ...)
{
	va_list va;
    static TCHAR szContent[1024];
	// build the list data.
	va_start(va, format);
    _vstprintf(szContent, format, va);
	va_end(va);
    this->SendGlobalRoomMessage(SMT_GLOBAL|SMT_SCROLL, szContent);
}

//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, int64_t winScore, PlayLog playlog)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
	int32 fireCount = 0;
	int32 hitfishCount = 0;
	PaoInfo _paoInfo = { 0 };
	int32_t paoValues = 0;
	map<int32_t, FishRecord> _fishRecords;
	for (auto iter = playlog.m_mPaoInfo.begin(); iter != playlog.m_mPaoInfo.end(); iter++)
	{
		paoValues = iter->first;
		_paoInfo = iter->second;
		fireCount += _paoInfo.fireCount;
		// 组装鱼
		HitFishInfo fsInfo = { 0 };		
		for (size_t i = 0; i < _paoInfo.hitFsInfo.size(); i++)
		{
			fsInfo = _paoInfo.hitFsInfo[i];
			// 如果不存在这条鱼则下一条
			if (fsInfo.odds == 0) continue;
			hitfishCount += fsInfo.count;
            auto it = _fishRecords.find(fsInfo.fishId);
            if (it !=_fishRecords.end())
			{
                it->second.count += fsInfo.count;
			}
			else
			{
				FishRecord fishrecord = { 0 };
				fishrecord.fishId = fsInfo.fishId;
				fishrecord.odds = fsInfo.odds;
				fishrecord.count = fsInfo.count;
				_fishRecords.insert(make_pair(fsInfo.fishId, fishrecord));
			}
		}
	}
	::rycsfish::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_firecount(fireCount);			//开炮数
	details.set_hitfishcount(hitfishCount);		//命中鱼数
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(winScore);		    //总输赢积分

	FishRecord fishdata = { 0 };
	for (auto &iter : _fishRecords)
	{
		fishdata = iter.second;
		// 结算时记录
		::rycsfish::FishRecordDetail* detail = details.add_detail();
		//鱼类型
		detail->set_fishid(fishdata.fishId);
		//鱼倍数
		detail->set_odds(fishdata.odds);
		//打死数量
		detail->set_count(fishdata.count);
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
	_fishRecords.clear();
}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink()
{
    shared_ptr<CTableFrameSink> pGameProcess(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFrameSink = dynamic_pointer_cast<ITableFrameSink>(pGameProcess);
    return pITableFrameSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pITableFrameSink)
{
    pITableFrameSink.reset();
}

extern "C" bool ValidTableFrameConfig()
{
    LOG(INFO) << "ValidTableFrameConfig called.";
    return TRUE;
}
