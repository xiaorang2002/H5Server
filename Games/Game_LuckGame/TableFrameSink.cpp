#include <glog/logging.h>
#include <muduo/base/Logging.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <math.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "public/GlobalFunc.h"

#include "proto/LuckGame.Message.pb.h"
#include "TableFrameSink.h"
#include "ConfigManager.h" 
#include "CMD_Game.h"

//定时器
#define ID_TIME_BET_SCORE 102 //下注定时器
#define ID_TIME_GAME_END 103  //结束定时器

//定时器时间
#define TIME_BET 15      //下注时间
#define TIME_GAME_END 10 //结束时间 (有人下注有大赢家)

#define ONCELOCK \
    for (static bool o_b_; !o_b_; o_b_ = true)

int64_t CTableFrameSink::m_llStorageControl = 0;
int64_t CTableFrameSink::m_llTodayStorage = 0;
int64_t CTableFrameSink::m_llStorageLowerLimit = 0;
int64_t CTableFrameSink::m_llStorageHighLimit = 0;

using namespace std;

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/LuckGame";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if (!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("LuckGame");
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit; // 声明全局变局,初始化库.

CTableFrameSink::CTableFrameSink()
{
    m_wGameStatus = GAME_STATUS_INIT;
    
    InitGameData();
}

CTableFrameSink::~CTableFrameSink()
{
}

void CTableFrameSink::InitGameData()
{
    //随机种子
    srand((unsigned)time(NULL));
    //游戏中玩家
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));

    //游戏记录
    //    memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));

    //玩家原始分
    memset(&m_llUserSourceScore, 0, sizeof(m_llUserSourceScore));
    memset(&m_llUserJfScore, 0, sizeof(m_llUserJfScore));

    //结算数据
    //    memset(&m_GameEnd, 0, sizeof(m_GameEnd));
    m_MinEnterScore = 0;

    m_Round = 0;
    memset(m_tLastOperate, 0, sizeof(m_tLastOperate));
    
    memset(m_lUserBetScore, 0, sizeof(m_lUserBetScore));
    memset(m_lUserWinScore, 0, sizeof(m_lUserWinScore));
   
    m_FreeRound = 0;
    m_MarryRound = 0;
    m_nPullInterval = 0;

    m_kickTime = 0;
    m_kickOutTime = 0;

    m_userId = 0;
    m_bUserIsLeft = true;
    m_lFreeWinScore = 0;
    m_lMarryWinScore = 0;
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    m_replay.clear();
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);
    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;

    
    LOG(WARNING)<<"+++++++++库存值 Storage+++++++:";
    LOG(WARNING)<<"Control:"<<m_llStorageControl;
    LOG(WARNING)<<"TodayStorage:"<<m_llTodayStorage;
    LOG(WARNING)<<"LowerLimit:"<<m_llStorageLowerLimit;
    LOG(WARNING)<<"HighLimit:"<<m_llStorageHighLimit;
    
    // LOG(WARNING) << "游戏开始 OnGameStart:" ;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t nEndTag)
{
    LOG(WARNING) << "游戏结束 OnEventGameConclude:" <<chairId;

    return true;
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    if (chairId >= GAME_PLAYER) { return false;  }
    CMD_S_GameFree StatusFree; 
    StatusFree.set_lcurrentjifen(m_llUserJfScore[chairId]);
    StatusFree.set_luserscore(m_llUserSourceScore[chairId]);

    m_vBetItemList = ConfigManager::GetInstance()->GetBetItemList();
    m_betListCount = m_vBetItemList.size();

    int idx = 0;
    //填充数据
    for(int betItem :m_vBetItemList){
        LOG(WARNING) << "vBetItemList:" << betItem;
        StatusFree.add_tbetlists(betItem);
        if(idx == 0){
            m_lUserBetScore[chairId] = betItem;
        }
    }

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t k = 0; k < AWARD_COUNT; k++)
        {
            StatusFree.add_tgoldlists(m_config.vProbability[i].Gold[k]);
            StatusFree.add_ticonlists(m_config.vProbability[i].Icon[k]);
        }
    }
   
    LOG(WARNING) << "currentjifen:" << StatusFree.lcurrentjifen() <<" " << StatusFree.luserscore();
 
    std::string data = StatusFree.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_FREE, (uint8_t *)data.c_str(), data.size());

    LOG(WARNING) << "发送场景 OnEventGameScene:" <<chairId;

    return true;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool islookon)
{
    LOG(WARNING) << "OnUserEnter:" <<userId << " "<< m_pITableFrame->GetTableId();

    m_userId = userId;

    if (islookon)  return false;

    //用户是否离开
    m_bUserIsLeft = false;

    auto dwChairID = m_pITableFrame->GetUserItemByUserId(userId)->GetChairId();
    //清数据
    ClearGameData(dwChairID);
        
    //读取玩家分数和积分 
    m_lUserScore = m_pITableFrame->GetTableUserItem(dwChairID)->GetUserScore();
    m_llUserSourceScore[dwChairID] = m_lUserScore;
    m_llUserJfScore[dwChairID] = m_pITableFrame->GetTableUserItem(dwChairID)->GetUserScore();;

    //获取玛丽记录
    // if(m_pITableFrame->GetSgjFreeGameRecord(&m_SgjFreeGameRecord,userId)) 
    memset(&m_config, 0, sizeof(m_config));
    ConfigManager::GetInstance()->GetConfig(m_config);

    LOG(WARNING) << m_config.m_testCount <<" "<<m_config.m_testWeidNum<<" "<<m_config.m_runAlg;
 
    //踢出时间
    m_kickOutTime = m_config.nKickOutTime;

    //踢出检测
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdKickOut);
    m_TimerIdKickOut=m_TimerLoopThread->getLoop()->runEvery(1, boost::bind(&CTableFrameSink::checkKickOut, this));
    
    m_TimerTestCount = 0;

    if(m_config.m_runAlg == 1){
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
        m_TimerIdTest=m_TimerLoopThread->getLoop()->runEvery(0.02, boost::bind(&CTableFrameSink::LeftReward, this,userId,dwChairID));
    }

    //用户进入发送场景消息
    OnEventGameScene(dwChairID,islookon);

    return true;
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
{
    auto dwChairID = m_pITableFrame->GetUserItemByUserId(userId)->GetChairId();
    if (dwChairID >= GAME_PLAYER)
    {
        LOG(WARNING) << "用户起立 ERROR: "<< dwChairID;
        return false;
    }
    //用户离开
    m_bUserIsLeft = true; 
    
    LOG(WARNING) << "用户离开 "<< dwChairID;
    
    //清数据
    ClearGameData(dwChairID);

    //起立
    m_pITableFrame->ClearTableUser(dwChairID,true);//INVALID_CHAIR, true);
    
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdKickOut);

    return true;
}

//用户同意
bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    return true;
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    return true;
}


// 保存对局结果
void CTableFrameSink::checkKickOut()
{
    m_kickTime++;
    if ( m_kickTime >= m_kickOutTime )
    { 
        uint32_t wChairID = 0;
	    LOG(WARNING) << "发送超时提示 "<< m_kickTime << " " << m_kickOutTime;
        //发送超时提示
        SendErrorCode(0,ERROR_OPERATE_TIMEOUT);
        // 
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(wChairID);
        if(user)
        {
            user->SetUserStatus(sGetout);
        }

        //修改桌子状态
        m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
        //踢玩家
        OnUserLeft(m_userId,false);
        //清理桌子
        m_pITableFrame->ClearTableUser(wChairID, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
        m_kickTime = 0;
    }

    //保存对局结果
	// LOG(WARNING) << "kickTime "<< m_kickTime << " " << m_kickOutTime;
}
// 
void CTableFrameSink::LeftReward(int64_t userId,uint32_t wChairID)
{
	// LOG(WARNING) << "LeftReward " << userId << " " << wChairID;

    //发送开始消息
    CMD_C_GameStart GameStart;
    GameStart.set_nbetindex(0);

    std::string data = GameStart.SerializeAsString();
    //发送数据
    OnGameMessage(wChairID,SUB_C_START,(uint8_t*)data.c_str(), data.size());

    if (m_config.m_runAlg == 1)
    {
        if (m_TimerTestCount++ > m_config.m_testCount)
        {
            int64_t affterScore = m_pITableFrame->GetTableUserItem(wChairID)->GetUserScore();
            LOG(WARNING) << "测试跑次数 " << m_TimerTestCount << " 跑前玩家分 " << m_lUserScore << "  跑后玩家分 " << affterScore << " 差 " << m_lUserScore - affterScore;
	        LOG(WARNING) << "玛丽总开次数 "<<m_MarryRound << " 得分 "<< m_lMarryWinScore << " 免费总开次数 "<<m_FreeRound <<" 得分 "<<m_lFreeWinScore;
            m_TimerTestCount = 0;

            if(m_config.m_testWeidNum==0)  LOG(WARNING) << " 跑收分 ";
            else if(m_config.m_testWeidNum==2)  LOG(WARNING) << " 跑放分 ";
            else  LOG(WARNING) << " 跑正常 ";
            m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
        }
    }
}

//游戏消息处理
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t subid, const uint8_t *data, uint32_t dwDataSize)
{
#if DEBUG_INFO
    LOG(WARNING) << "游戏消息处理 OnGameMessage:" << (int)subid ;
#endif
    //清理
    cardValueStr = "";
    //开始时间
	m_startTime = chrono::system_clock::now();//(uint32_t)time(NULL);//开始时间
	groupStartTime = (uint32_t)time(NULL);

    //清零
    m_kickTime = 0;

    m_replay.clear(); 
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;

    pUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    m_replay.addPlayer(pUserItem->GetUserId(), pUserItem->GetAccount(), pUserItem->GetUserScore(), wChairID);

    //记录原始积分
    m_llUserSourceScore[wChairID] = pUserItem->GetUserScore();
    
    if(subid == SUB_C_START) 
    {
        if(m_nPullInterval == 0){ 
            m_nPullInterval = m_config.nPullInterval;
        }

        CMD_C_GameStart pGamePulling;
        pGamePulling.ParseFromArray(data, dwDataSize);
        m_typeIdx = pGamePulling.ntypeindex();
        m_betIdx = pGamePulling.nbetindex();

        //玩家下注金额 
        if(m_betListCount > 0 ){ 
            if( m_betIdx >= 0 && m_betIdx < m_betListCount ){
                m_lUserBetScore[wChairID] = m_vBetItemList[m_betIdx];     
                m_replay.cellscore = m_lUserBetScore[wChairID];
            }
            else{
                assert( m_betIdx >= 0 && m_betIdx < m_betListCount );
                LOG(WARNING) << " betidx >= 0 && betidx < m_betListCount error "<< m_betIdx << " " << m_betListCount;
            }
        }
        else{
            assert( m_betListCount > 0);
            LOG(WARNING) << " m_betListCount > 0 "<< m_betListCount;
        }

        //测试状态下不限制 
        if (m_config.m_runAlg==1) {
             m_nPullInterval = 0;
        } 
 
        //拉动频率限制
        if ((time(NULL) - m_tLastOperate[wChairID]) < m_nPullInterval) {
            LOG(WARNING) << "拉动太频繁了 ";
            return false;
        } 
   
        //修改用户状态
        m_pITableFrame->SetUserReady(wChairID); 

        //发送开奖结果
        int bRet = GetResult(wChairID, m_lUserBetScore[wChairID],m_typeIdx);
        if(bRet > 0){
            LOG(WARNING) << "创建开奖结果失败!Get Result";
            SendErrorCode(wChairID,(enErrorCode)bRet); 
        } 

        //修改桌子状态
        m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);

        m_tLastOperate[wChairID] = time(NULL);

        m_Round++; 

        m_pITableFrame->ClearTableUser(INVALID_CHAIR, true);
        return true;
    } 

    return true;
}

//发送
bool CTableFrameSink::SendErrorCode(int32_t wChairID, enErrorCode enCodeNum)
{
    CMD_S_ErrorCode errorCode;
    //memset(&errorCode, 0, sizeof(errorCode));
    errorCode.set_nlosereason(enCodeNum);
    string data = errorCode.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SUB_S_ERROR_INFO, (uint8_t *)data.c_str(), data.size());
    return true;
}


//设置桌子
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{ 
    //更新游戏配置 

    assert(NULL != pTableFrame);
    m_pITableFrame = pTableFrame;
    if (m_pITableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        uint8_t jsize = m_pGameRoomInfo->jettons.size();
        for (uint8_t i = 0; i < jsize; i++)
        {
            m_cbJettonMultiple.push_back((uint8_t)m_pGameRoomInfo->jettons[i]);
        }

        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_TimerLoopThread = m_pITableFrame->GetLoopThread(); 
        //准入分数
        m_MinEnterScore = m_pGameRoomInfo->enterMinScore;
    }
    //初始化游戏数据
    m_wGameStatus = GAME_STATUS_INIT;


    //加载库存
    if(m_llStorageControl == 0){
        OnGameStart();
    }

    //获取游戏配置
    ConfigManager * cfgManager = ConfigManager::GetInstance();
	if (cfgManager == NULL){
		LOG(WARNING) << "ConfigManager失败 ";
		return false;
    }

	int ret = cfgManager->CheckReadConfig();
    if(ret != 0){
		LOG(WARNING) << "配置文件读取失败 " << ret;
		return false;
	}

    return true;
}


//创建旋转场景
int CTableFrameSink::GetResult(const int32_t wChairID, int64_t lbetScore, int32_t nTypeIndex)
{
    // 分数太少
    if (m_llUserSourceScore[wChairID] < lbetScore) return ERROR_SCORE_NOT_ENOUGH;
    // 分数太少
    if (2 < nTypeIndex) return ERROR_OPERATE;
   
    //拉霸开奖结果命令 
    CMD_S_RoundResult_Tag CMDGameOneEnd; 

    //填充命令参数
    WeidghtItem ItemCfg;
    memset(&ItemCfg, 0, sizeof(ItemCfg));
    ItemCfg =  m_config.vProbability[nTypeIndex];

    int count = 0;
    int retId = 0;
    int winScore = 0;
    do
    {
        if ((++count) > 100){  LOG(WARNING) << "count > 100";
            break;
        }

        // 获取结果
        retId = m_GameLogic.RandomFruitArea(AWARD_COUNT, ItemCfg.Weight);
        if(retId >= 0 && retId < AWARD_COUNT)  winScore = ItemCfg.Gold[retId];
        
        //押注库存验证
        if (winScore > 0L) {
           if(!CheckStorageVal(winScore)) {
                continue;
           }
        } 
 
        //退出
        break;

    } while (true);
 
    //记录
    cardValueStr = str(boost::format("%d,%d") % retId % winScore);
    
    LOG(WARNING) << cardValueStr;
    if(m_config.m_runAlg==1)  {
        string tempStr = str(boost::format("%5d|%s") % m_Round % cardValueStr);
        openLog(cardValueStr.c_str());
    }

    //写分
    int64_t llGameTax = 0;
    if (WriteScore(wChairID, lbetScore, winScore, llGameTax,cardValueStr) == false)
    {
        return ERROR_SIT_CHAIR_ID;
    }
   
    //保存数据
	m_lUserBetScore[wChairID] = lbetScore;
	m_lUserWinScore[wChairID] = winScore; 
   
#if 1//DEBUG_INFO
    LOG(WARNING) << "lCurrentScore: " << m_llUserSourceScore[wChairID];
    LOG(WARNING) << "winScore: " << winScore;
#endif

    //复制数据
    CMD_S_RoundResult cmdData;
    cmdData.set_lscore(winScore);
    cmdData.set_luserscore(m_llUserSourceScore[wChairID]);
    cmdData.set_lcurrentjifen(0);
    cmdData.set_cbroundid(mGameIds);

    std::string data = cmdData.SerializeAsString();
    if (!m_bUserIsLeft)
        m_pITableFrame->SendTableData(wChairID, SUB_S_NORMAL, (uint8_t *)data.c_str(), data.size());
     
    return 0;
}

bool CTableFrameSink::CheckStorageVal(int64_t winScore)
{
    int64_t temp = m_llStorageControl - m_llStorageLowerLimit;
    if (winScore > max(m_config.m_CtrlVal * 100, temp / 3)){

        LOG(WARNING) << "winScore:" << winScore << " max:"<< temp;
        return false;
    }
    return true;
} 
  
 
//拉霸写分
bool CTableFrameSink::WriteScore(const int32_t wChairID, const int32_t lBetScore, const int64_t lWinScore, int64_t &lGameTax,string cardValueStr)
{
    AssertReturn(wChairID < GAME_PLAYER, return false);
 
    int64_t lplayerWinScore = 0; 
    lplayerWinScore = lWinScore - lBetScore;
     
    m_llStorageControl -= lplayerWinScore;
    m_llTodayStorage -= lplayerWinScore;

    //算税收
    if (lWinScore > 0)  lGameTax = 0;//m_pITableFrame->CalculateRevenue(lWinScore);
       
    //写积分
    tagScoreInfo ScoreData;
    ScoreData.chairId = wChairID;
    ScoreData.addScore = lplayerWinScore - lGameTax; // 当局输赢分数 
    ScoreData.betScore = lBetScore;         // 总压注
    ScoreData.revenue = lGameTax;           // 当局税收
    ScoreData.rWinScore = ScoreData.betScore;//lBetScore;//lWinScore; //有效投注额：税前输赢
    ScoreData.cardValue = cardValueStr;
    ScoreData.startTime = m_startTime;

    // 中奖则增加玩家分
    m_llUserSourceScore[wChairID] += ScoreData.addScore;

    // 减少积分
    m_llUserJfScore[wChairID] -= lBetScore;


#if 1//!RUN_ALGORITHM

    m_pITableFrame->WriteUserScore(&ScoreData,1,mGameIds); 

    //更新系统库存
    m_pITableFrame->UpdateStorageScore((-lplayerWinScore));

    m_replay.addResult(wChairID, wChairID,ScoreData.betScore, ScoreData.addScore, "", false); 
    m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "", -1, 0, wChairID, wChairID);

    //保存对局结果
    m_pITableFrame->SaveReplay(m_replay);

#endif

#if DEBUG_INFO 
    LOG(WARNING) << "总库存: " << m_llStorageControl * TO_DOUBLE << ", 最低:" << m_llStorageLowerLimit * TO_DOUBLE << ", 今日库存: "\
        << m_llTodayStorage * TO_DOUBLE << ", 最高:" << m_llStorageHighLimit * TO_DOUBLE <<" 总次数 "<< m_Round << " 免费次数 "<< m_FreeRound << " 玛丽次数 " << m_MarryRound;
#endif
 
    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData(int dwChairID)
{ 
    m_lUserWinScore[dwChairID] = 0L;
    m_lUserBetScore[dwChairID] = 0L;   
    m_tLastOperate[dwChairID] = 0L;
}

void  CTableFrameSink::openLog(const char *p, ...)
{
    // if (m_IsEnableOpenlog)
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        // sprintf(Filename, "./log/LuckGame/Sgj_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        sprintf(Filename, "./log/LuckGame/Sgj_%d.log",m_config.m_testWeidNum);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        // fprintf(fp, "%s", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink> &pSink)
{
    pSink.reset();
}
