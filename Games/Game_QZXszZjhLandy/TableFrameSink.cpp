#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

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

#include "proto/QZXszZjh.Message.pb.h"

#include "CMD_Game.h"
#include "GameLogic.h"

using namespace QZXszZjh;
//using namespace muduo;

#include "TableFrameSink.h"

//打印日志
#define PRINT_GAME_INFO					1
#define PRINT_GAME_WARNING				1


//////////////////////////////////////////////////////////////////////////
////定时器
//#define ID_TIME_READY					1					//准备定时器(5S)
//#define ID_TIME_CALL_BANKER				2					//叫庄定时器(3S)
//#define ID_TIME_ADD_SCORE				3					//下注定时器(3S)
//#define ID_TIME_OPEN_CARD				4					//开牌定时器(5S)
//#define ID_TIME_GAME_END				5					//结束定时器(8S)

//定时器时间
#define TIME_BASE						1000
#define TIME_READY						1					//准备定时器(2S)
#define TIME_ENTER_PLAYING              1
#define TIME_CALL_BANKER				5					//叫庄定时器(5S)
#define TIME_ADD_SCORE					5					//下注定时器(5S) 5S->8S
#define TIME_OPEN_CARD					10					//开牌定时器(7S) 7S->3S
#define TIME_GAME_END					5+1					//结束定时器(5S)

//////////////////////////////////////////////////////////////////////////////////////

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
        string dir = "./log/QZXJinHua/";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("QZXJinHua");

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

//uint32_t g_nRoundID = 0;

CTableFrameSink::CTableFrameSink(void) :
  m_dwCellScore(1.0)
  , m_pITableFrame(NULL)
  , m_iTodayTime(time(NULL))
{
    stockWeak = 0.0;
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

    m_nLottery = 0;
    m_strRoundId = "";
    m_cbStatus = GAME_STATUS_INIT;
    m_mUserInfo.clear();
    // ReadConfigInformation();

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
    ratioScale_ = 1000;
    //5/4/3/2/单人局的概率
    ratio5_ = 40, ratio4_ = 35, ratio3_ = 20, ratio2_ = 5, ratio1_ = 0;

	//初始化桌子游戏人数概率权重
    initPlayerRatioWeight();
	// for (int i = 0; i < GAME_PLAYER; ++i) {
	// 	if (i == 5) {
	// 		//6人局的概率0
	// 		ratioGamePlayerWeight_[i] = 0;//15 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else if (i == 4) {
	// 		//5人局的概率40%
	// 		ratioGamePlayerWeight_[i] = 40 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else if (i == 3) {
	// 		//4人局的概率35%
	// 		ratioGamePlayerWeight_[i] = 35 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else if (i == 2) {
	// 		//3人局的概率20%
	// 		ratioGamePlayerWeight_[i] = 20 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else if (i == 1) {
	// 		//2人局的概率5%
	// 		ratioGamePlayerWeight_[i] = 5 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else if (i == 0) {
	// 		//单人局的概率0
	// 		ratioGamePlayerWeight_[i] = 0;//5 * ratioGamePlayerWeightScale_;
	// 	}
	// 	else {
	// 		ratioGamePlayerWeight_[i] = 0;
	// 	}
	// }
	// //计算桌子要求标准游戏人数
	// poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
	MaxGamePlayers_ = 0;

    //系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
    ratioSwap_ = 100;
    //系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
    ratioSwapLost_ = 80;
    //系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
    ratioSwapWin_ = 70;
    //发牌时尽量都发好牌的概率，玩起来更有意思
    ratioDealGood_ = 50;
    //IP访问限制
    checkIp_ = false;
    //一张桌子真实玩家数量限制
    checkrealuser_ = false;
}

CTableFrameSink::~CTableFrameSink(void)
{

}
void  CTableFrameSink::openSLog(const char *p, ...)
{
    if(!m_bWritelog) 
        return;
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//QZXJinHua//QZXJinHua_table_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
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
//按行读取文件
void CTableFrameSink::readFile(char const* filename, std::vector<string>& lines, char const* skip) {
#if 1
    {
        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
            return;
        }
        char line[512] = { 0 };
        while (!feof(fp)) {
            if (NULL != fgets(line, 512, fp)) {
                int len = strlen(line);
                if (line[len - 1] == '\r' ||
                    (line[len - 2] != '\r' && line[len - 1] == '\n')) {
                    //去掉\r或\n结尾
                    line[len - 1] = '\0';
                    --len;
                }
                else if (line[len - 2] == '\r' && line[len - 1] == '\n') {
                    //0 == strcmp(&line[len-2], "\r\n")
                    //去掉\r\n结尾
                    line[len - 2] = '\0';
                    line[len - 1] = '\0';
                    len -= 2;
                }
                //printf(line);
                if (line[0] != '\0') {
                    if (len >= strlen(skip) && 0 == strncmp(line, skip, strlen(skip))) {
                    }
                    else {
                        lines.push_back(line);
                    }
                }
            }
        }
        //printf("--- *** fgets:%d\n", lines.size());
        //for (std::vector<string>::iterator it = lines.begin();
        //  it != lines.end(); ++it) {
        //  printf("%s\n", it->c_str());
        //}
        fclose(fp);
    }
#else
    {
        lines.clear();
        std::fstream fs(filename);
        std::string line;
        while (getline(fs, line)) {
            int len = line.size();
            if (line[len - 1] == '\r' ||
                (line[len - 2] != '\r' && line[len - 1] == '\n')) {
                //去掉\r或\n结尾
                line[len - 1] = '\0';
                --len;
            }
            else if (line[len - 2] == '\r' && line[len - 1] == '\n') {
                //去掉\r\n结尾
                line[len - 2] = '\0';
                line[len - 1] = '\0';
                len -= 2;
            }
            //printf(line.c_str());
            if (line[0] != '\0') {
                if (len >= strlen(skip) &&
                    0 == strncmp(&line.front(), skip, strlen(skip))) {
                }
                else {
                    lines.push_back(line);
                }
            }
        }
        //printf("--- *** getline:%d\n", lines.size());
        //for (std::vector<string>::iterator it = lines.begin();
        //  it != lines.end(); ++it) {
        //  printf("%s\n", it->c_str());
        //}
    }
#endif
}

//初始化桌子游戏人数概率权重
void CTableFrameSink::initPlayerRatioWeight() {
    for (int i = 0; i < GAME_PLAYER; ++i) {
        if (i == 4) {
            //5人局的概率0
            ratioGamePlayerWeight_[i] = ratio5_ * ratioGamePlayerWeightScale_;
        }
        else if (i == 3) {
            //4人局的概率10%
            ratioGamePlayerWeight_[i] = ratio4_ * ratioGamePlayerWeightScale_;
        }
        else if (i == 2) {
            //3人局的概率10%
            ratioGamePlayerWeight_[i] = ratio3_ * ratioGamePlayerWeightScale_;
        }
        else if (i == 1) {
            //2人局的概率80%
            ratioGamePlayerWeight_[i] = ratio2_ * ratioGamePlayerWeightScale_;
        }
        else if (i == 0) {
            //单人局的概率0
            ratioGamePlayerWeight_[i] = ratio1_ * ratioGamePlayerWeightScale_;
        }
        else {
            ratioGamePlayerWeight_[i] = 0;
        }
    }
    //计算桌子要求标准游戏人数
    poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
}
//设置指针
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    assert(NULL != pTableFrame);
    m_pITableFrame = pTableFrame;
    if(m_pITableFrame)
    {
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        m_pGameRoomInfo = pTableFrame->GetGameRoomInfo();
        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    }
    else
    {
        return false;
    }
    //初始化游戏数据
    //更新游戏配置
    InitGameData();
    //读取配置
    ReadStorageInformation();
    //读取机器人配置
    // ReadConfig();
    //初始化桌子游戏人数概率权重
    initPlayerRatioWeight();
    //放大倍数
    int scale = 1000;
    //系统换牌概率
    int dw[MaxExchange] = {
        ratioSwap_*scale,
        (100 - ratioSwap_)*scale };
    sysSwapCardsRatio_.init(dw, MaxExchange);
    //系统通杀概率
    int lw[MaxExchange] = {
        ratioSwapLost_*scale,
        (100 - ratioSwapLost_)*scale };
    sysLostSwapCardsRatio_.init(lw, MaxExchange);
    //系统放水概率
    int ww[MaxExchange] = {
        ratioSwapWin_*scale,
        (100 - ratioSwapWin_)*scale };
    sysWinSwapCardsRatio_.init(ww, MaxExchange);

    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
    //庄家
    m_wBankerUser = INVALID_CHAIR;
    //当前筹码
//    memset(m_cbCurJetton, 0, sizeof(m_cbCurJetton));
    memset(m_sAddScore, 0, sizeof(m_sAddScore));
    memset(m_cbMaxJetton, 0, sizeof(m_cbMaxJetton));
}

// bool CTableFrameSink::ReadConfigInformation()
// {
//     //=====config=====
//     if(!boost::filesystem::exists("./conf/Sgtest.ini"))
//     {
//         return 0;
//     }

//     boost::property_tree::ptree pt;
//     boost::property_tree::ini_parser::read_ini("./conf/Sgtest.ini", pt);

//     vector<std::string> strAccount;
//     vector<std::string> strValue;


//     for(auto& section : pt)
//     {
//         if(section.first == "GAME_ACCOUNT")
//         {
//             for(auto& key : section.second)
//             {
//                 strAccount.push_back(key.second.get_value<std::string>());
//             }
//         }
//     }
//     for(auto& section : pt)
//     {
//         if(section.first == "GAME_VALUE")
//         {
//             for(auto& key : section.second)
//             {
//                 strValue.push_back(key.second.get_value<std::string>());
//             }
//         }
//     }
//     if(strAccount.size() != strValue.size()) return false;

//     for(int i = 0; i < strAccount.size(); ++i)
//     {
//         m_UserTest[strAccount[i]] = strValue[i];
//     }
//     return true;
// }

//初始化游戏数据
void CTableFrameSink::InitGameData()
{
    //随机种子
    srand((unsigned)time(NULL));
    //用户数据副本
    userinfos_.clear();
    //游戏中玩家个数
    m_wPlayCount = 0;
    //游戏中的机器的个数
    m_wAndroidCount = 0;
    //庄家
    m_wBankerUser = INVALID_CHAIR;
    //当前最大牌用户
    m_currentWinUser = INVALID_CHAIR;
    //游戏中玩家
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
    //牌数据
    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    //最大牌组合数据
    memset(m_cbMaxCard, 0, sizeof(m_cbMaxCard));
    memset(m_handCardsType, 0, sizeof(m_handCardsType));

    memset(m_RecordOutCard, 0, sizeof(m_RecordOutCard));
    m_mOutCardRecord.clear();

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_sAddScore[i] = -1;								//玩家下注(-1:未下注, 大于-1:对应筹码索引)
        m_sCardType[i] = -1;								//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
        m_iMultiple[i] = 1;									//倍数
        m_sCallBanker[i] = -1;                              //叫庄状态

        m_bPlayerIsOperation[i] = false;
    }

    //if (NULL != m_pITableFrame)
    //{//获取私人房信息
    //	m_pITableFrame->GetPrivateTableInfo(m_PrivateTableInfo);
    //}
    //时间初始化

    m_dwReadyTime = 0;										//ready时间
    m_dwEnterPlayingTime = 0;						        //EnterPlaying时间
    m_dwCallTime = 0;										//叫庄时间
    m_dwScoreTime = 0;										//下注时间
    m_dwOpenTime = 0;										//开牌时间
    m_dwEndTime = 0;										//结束时间


    m_cbLeftCardCount = MAX_CARD_TOTAL;
    memset(m_cbRepertoryCard, 0, sizeof(m_cbRepertoryCard));
    //游戏记录
    memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_bPlayerIsOperation[i]=false;
    }
    m_replay.clear();
    m_bWritelog = false;
}

void CTableFrameSink::PaoMaDeng()
{
    int64_t YouXiaoWin[GAME_PLAYER]={0};
    int64_t broadcastScore = m_pGameRoomInfo->broadcastScore;  //100

    string SpecialCard[GAME_PLAYER]={""};
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_cbPlayStatus[i] != 1)
            continue;

        // if(PaoMaDengCardType[i] == Ty30)
        // {
        //     if(PaoMaDengWinScore[i] >= broadcastScore) 
        //     {
        //         YouXiaoWin[i] = PaoMaDengWinScore[i];
        //         // SpecialCard[i] = "豹子";
        //     }
        // }
        // else if(PaoMaDengCardType[i] == Ty123sc)
        // {
        //     if(PaoMaDengWinScore[i] >= broadcastScore)
        //     {
        //         YouXiaoWin[i] = PaoMaDengWinScore[i];
        //         // SpecialCard[i] = "顺金";
        //     }
        // }
        // else if(PaoMaDengCardType[i] == Tysc)
        // {
        //     if(PaoMaDengWinScore[i] >= broadcastScore)
        //     {
        //         YouXiaoWin[i] = PaoMaDengWinScore[i];
        //         // SpecialCard[i] = "金花";
        //     }
        // }
        // if(PaoMaDengWinScore[i] >= broadcastScore) 
        // {
        //     YouXiaoWin[i] = PaoMaDengWinScore[i];
        // }
        if(PaoMaDengWinScore[i] >= m_pGameRoomInfo->broadcastScore)
        {
            int sgType = SMT_GLOBAL|SMT_SCROLL;
            m_pITableFrame->SendGameMessage(i, "", sgType, PaoMaDengWinScore[i]);
        }
    }
    // int64_t bigone = 0;
    // int index = 0;
    // for(int i = 0; i < GAME_PLAYER; ++i)
    // {
    //     if(bigone < YouXiaoWin[i])
    //     {
    //         bigone = YouXiaoWin[i];
    //         index = i;
    //     }
    // }
    // if(bigone >= broadcastScore)
    // {
    //     int sgType = SMT_GLOBAL|SMT_SCROLL;
    //     m_pITableFrame->SendGameMessage(index, SpecialCard[index], sgType, bigone);
    // }
}

int CTableFrameSink::IsMaxCard(int wChairID)
{
    int isMaxCard = 1;

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i] || i == wChairID)
            continue;
        if(!m_GameLogic.CompareHandCards(m_cbCardData[wChairID], m_cbCardData[i]))
            isMaxCard = 0;
    }
    return isMaxCard;
}

//发送场景
bool CTableFrameSink::OnUserEnter(int64_t userId, bool bisLookon)
{
    LOG(ERROR)<<"---------------------------OnUserEnter---------------------------";

    if(!m_pITableFrame) return false;

    uint32_t chairId = INVALID_CHAIR;

    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(pIServerUserItem)
    {
        chairId = pIServerUserItem->GetChairId();
        LOG(ERROR)<<"------OnUserEnter------user->GetChairID()="<<pIServerUserItem->GetChairId()<<",Account="<<pIServerUserItem->GetAccount();
    }
    else
    {
        LOG(ERROR)<<"------OnUserEnter------pIServerUserItem==NULL"<<"---------------------";
        return false;
    }

    if(chairId > 4)
    {
        LOG(ERROR)<<"------OnUserEnter------pIServerUserItem==NULL"<<"---------------------";
        return false;
    }

    if (m_pITableFrame->GetPlayerCount(true) == 1) {
		////////////////////////////////////////
		//第一个进入房间的也必须是真人
        assert(m_pITableFrame->GetPlayerCount(false) == 1);
		////////////////////////////////////////
		//权重随机乱排
		poolGamePlayerRatio_.shuffleSeq();
		////////////////////////////////////////
		//初始化桌子当前要求的游戏人数
		MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
		////////////////////////////////////////
		//重置累计时间
		totalMatchSeconds_ = 0;
		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
	}
	// if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START) {
	// 	assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
	// 	assert(MaxGamePlayers_ <= GAME_PLAYER);
	// }

    int  wUserCount = 0;
    //用户状态
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //获取用户
        shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
        if(pIServerUser)
        {
            ++wUserCount;
        }
    }
    LOG(ERROR)<<"---------------------------Player iGAME_STATUS_END---------------------------"<<m_pITableFrame->GetTableId()<<" wUserCount"<<wUserCount;

    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_INIT || m_cbStatus == XJH_GAME_STATUS_DEFAULT)
    {
        pIServerUserItem->SetUserStatus(sPlaying);
        if(wUserCount >= 2)
        {
            ClearAllTimer();
            m_dwReadyTime = (uint32_t)time(NULL);
            //m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(2.5, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
            LOG(ERROR)<<"---------------------------m_TimerIdReadyOver Start 1S---------------------------";
        }
    }
    else
    {
        //游戏已经开始了，此时是断线重连
        auto it = m_mUserInfo.find(userId);
        if(m_mUserInfo.end() != it)
        {
            m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
            m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
            OnEventGameScene(chairId, false);
        }

    }
	if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到桌子要求游戏人数，开始游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			////////////////////////////////////////
			//不超过桌子要求最大游戏人数
			assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
			printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
			GameTimerReadyOver();
		}
		else {
			if (m_pITableFrame->GetPlayerCount(true) == 1) {
				////////////////////////////////////////
				//第一个进入房间的也必须是真人
                assert(m_pITableFrame->GetPlayerCount(false) == 1);
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
    switch (m_cbStatus)
    {
    case XJH_GAME_STATUS_INIT:
    case XJH_GAME_STATUS_READY:			//空闲状态
    case XJH_GAME_STATUS_LOCK_PLAYING:
    {
        XJH_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分


        uint32_t dwStartTime = 0;
        if(m_cbStatus == XJH_GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_cbStatus == XJH_GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

        GameFree.set_dwuserid(pIServerUser->GetUserId());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
        GameFree.set_sznickname(pIServerUser->GetNickName());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        GameFree.set_wtableid(pIServerUser->GetTableId());


        //发送数据
        std::string data = GameFree.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, data.c_str(), data.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
        }
        break;
    }
    case XJH_GAME_STATUS_CALL:			//叫庄状态
    {
        XJH_MSG_GS_CALL GameCall;
        GameCall.set_roundid(m_strRoundId);
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
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未操作, 0:不叫, 1:叫庄)

            //LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
        }


        GameCall.set_dwuserid(pIServerUser->GetUserId());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
        GameCall.set_sznickname(pIServerUser->GetNickName());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, data.c_str(), data.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(dwChairID,SC_GAMESCENE_CALL, data.c_str(),data.size());
        }
        break;
    }
    // case XJH_GAME_STATUS_SCORE:		//下注状态
    // {
    //     XJH_MSG_GS_SCORE GameScore;
    //     GameScore.set_roundid(m_strRoundId);
    //     GameScore.set_dcellscore((m_dwCellScore));
    //     uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
    //     uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
    //     GameScore.set_cbtimeleave(cbTimeLeave);
    //     GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户

    //     for(int i = 0; i < GAME_PLAYER; i++)
    //     {
    //         GameScore.add_cbusermaxjetton(-1);
    //     }
    //     for (int i = 0; i < GAME_PLAYER; i++)
    //     {
    //         GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
    //         if (m_cbPlayStatus[i] == 0)
    //             continue;
    //     }

    //     GameScore.set_dwuserid(pIServerUser->GetUserId());
    //    // GameScore.set_cbgender(pIServerUser->GetGender());
    //     GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
    //    // GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
    //     GameScore.set_sznickname(pIServerUser->GetNickName());
    //    // GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
    //    // GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
    //     GameScore.set_szlocation(pIServerUser->GetLocation());
    //     GameScore.set_usstatus(pIServerUser->GetUserStatus());
    //     GameScore.set_wchairid(pIServerUser->GetChairId());

    //     //发送数据
    //     std::string data = GameScore.SerializeAsString();
    //     //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, data.c_str(), data.size());

    //     //tmp......
    //     if(m_bReplayRecordStart)
    //     {
    //         RepayRecord(dwChairID,SC_GAMESCENE_SCORE, data.c_str(),data.size());
    //     }
    //     break;
    // }
    case XJH_GAME_STATUS_OPEN:		//开牌场景消息
    {
        XJH_MSG_GS_OPEN GameOpen;
        GameOpen.set_roundid(m_strRoundId);
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwOpenTime;//m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >= TIME_OPEN_CARD ? 0 : dwScoreTime;		//剩余时间 TIME_ADD_SCORE
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);       

        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

            // GameOpen.add_cbuserjettonmultiple(m_cbCurJetton[i]);//游戏中玩家(1打牌玩家)
            // GameOpen.set_cbuserjettonmultiple(i, m_sAddScore[i]);

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1))
            {
                if (i == dwChairID)
                {
                    int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                    GameOpen.set_cbcardtype(i,cardtype);

                    LOG(INFO)<<"---------------open card"<<cardtype;
                    openSLog("开牌场景消息: open card=%d ;",cardtype);
                    //玩家最优手牌
                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                    }
                    //玩家手牌
                    int startidx2 = i * MAX_SENDCOUNT;
                    for(int j=0;j<MAX_SENDCOUNT;j++)
                    {
                        GameOpen.set_cbhandcarddata(startidx2+j,(int) m_cbHandCardData[i][j]);
                    }
                }
                else
                {   int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                    GameOpen.set_cbcardtype(i,cardtype);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                GameOpen.set_cbcardtype(i,cardtype);
                //玩家最优手牌
                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                }
                //玩家手牌
                // int startidx2 = i * MAX_SENDCOUNT;
                // for(int j=0;j<MAX_SENDCOUNT;j++)
                // {
                //     GameOpen.set_cbhandcarddata(startidx2+j,(int) m_cbHandCardData[i][j]);
                // }
            }
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
        GameOpen.set_wtableid(pIServerUser->GetTableId());
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
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(dwChairID,SC_GAMESCENE_OPEN, data.c_str(),data.size());
        }
        break;
    }
    case XJH_GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        XJH_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);


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
        //LOG(INFO)<<"GAME_STATUS1_END-------------------------------User Enter"<<(int)dwChairID;

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
        }
        break;
    }
    default:
        break;
    }

    return true;
}


bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
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

    switch (m_cbStatus)
    {
    case XJH_GAME_STATUS_INIT:
    case XJH_GAME_STATUS_READY:			//空闲状态
    case XJH_GAME_STATUS_LOCK_PLAYING:
    {
        XJH_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分

        uint32_t dwStartTime = 0;
        if(m_cbStatus == XJH_GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_cbStatus == XJH_GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

        GameFree.set_dwuserid(pIServerUser->GetUserId());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
        GameFree.set_sznickname(pIServerUser->GetNickName());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        GameFree.set_wtableid(pIServerUser->GetTableId());


        LOG(INFO)<<"GAME_STATUS1_FREE-------------------------------User Enter"<<(int)dwChairID;
        openSLog("空闲状态消息: User Enter=%d ;",(int)dwChairID);
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case XJH_GAME_STATUS_CALL:			//叫庄状态
    {
        XJH_MSG_GS_CALL GameCall;
        GameCall.set_roundid(m_strRoundId);
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
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未操作, 0:不叫, 1:叫庄)

            LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
            openSLog("叫庄状态消息: Player=%d,m_sCallBanker[i]=%d;",i,(int)m_sCallBanker[i]);
        }


        GameCall.set_dwuserid(pIServerUser->GetUserId());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
        GameCall.set_sznickname(pIServerUser->GetNickName());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());

        break;
    }
    // case XJH_GAME_STATUS_SCORE:		//下注状态
    // {
    //     XJH_MSG_GS_SCORE GameScore;
    //     GameScore.set_roundid(m_strRoundId);
    //     GameScore.set_dcellscore((m_dwCellScore));
    //     uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
    //     uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
    //     GameScore.set_cbtimeleave(cbTimeLeave);
    //     GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户

    //     for (int i = 0; i < GAME_PLAYER; i++)
    //     {
    //         GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
    //         for(int j = 0; j < GAME_JETTON_COUNT; ++j)
    //         {
    //             GameScore.add_cbusermaxjetton(m_cbMaxJetton[i][j]);//记录闲家下注
    //         }
    //     }


    //     for (int i = 0; i < GAME_PLAYER; i++)
    //         GameScore.add_cbusercurjetton(m_sAddScore[i]);//记录闲家下注倍数

    //     GameScore.set_dwuserid(pIServerUser->GetUserId());
    //     GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
    //     GameScore.set_sznickname(pIServerUser->GetNickName());
    //     GameScore.set_szlocation(pIServerUser->GetLocation());
    //     GameScore.set_usstatus(pIServerUser->GetUserStatus());
    //     GameScore.set_wchairid(pIServerUser->GetChairId());
    //     GameScore.set_wtableid(pIServerUser->GetTableId());

    //     //发送数据
    //     std::string data = GameScore.SerializeAsString();
    //     m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, (uint8_t*)data.c_str(), data.size());

    //     break;
    // }
    case XJH_GAME_STATUS_OPEN:		//开牌场景消息
    {
        XJH_MSG_GS_OPEN GameOpen;
        GameOpen.set_roundid(m_strRoundId);
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwOpenTime; //m_dwScoreTime
        uint32_t cbTimeLeave = dwScoreTime >= TIME_OPEN_CARD ? 0 : dwScoreTime;		//剩余时间  TIME_ADD_SCORE
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);

        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
            }
            for(int j=0;j<MAX_SENDCOUNT;j++)
            {
                GameOpen.add_cbhandcarddata(0);
            }
            // GameOpen.add_cbuserjettonmultiple(m_cbCurJetton[i]);
            // GameOpen.add_cbuserjettonmultiple(m_sAddScore[i]);
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1))
            {
                if (i == dwChairID)
                {
                    int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                    GameOpen.set_cbcardtype(i,cardtype);

                   LOG(INFO)<<"----------------open card"<<cardtype;
                   openSLog("开牌场景消息: open card=%d;",cardtype);
                    //玩家最优手牌
                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);                        
                    }
                    //玩家手牌
                    int startidx2 = i * MAX_SENDCOUNT;
                    for(int j=0;j<MAX_SENDCOUNT;j++)
                    {
                        GameOpen.set_cbhandcarddata(startidx2+j,(int) m_cbHandCardData[i][j]);
                    }
                }
                else
                {   int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                    GameOpen.set_cbcardtype(i,cardtype);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                GameOpen.set_cbcardtype(i,cardtype);
                //玩家最优手牌
                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                }
                //玩家手牌
                // int startidx2 = i * MAX_SENDCOUNT;
                // for(int j=0;j<MAX_SENDCOUNT;j++)
                // {
                //     GameOpen.set_cbhandcarddata(startidx2+j,(int) m_cbHandCardData[i][j]);
                // }
            }
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        GameOpen.set_wtableid(pIServerUser->GetTableId());
        LOG(INFO)<<"GAME_STATUS_OPEN-------------------------------User Enter"<<(int)dwChairID;
        openSLog("GAME_STATUS_OPEN: User Enter=%d;",(int)dwChairID);
        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
             LOG(INFO)<<"---------------------------------open card"<<res[i];
             openSLog("---------open card: %d;",(int)res[i]);
        }
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case XJH_GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        XJH_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);

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
        GameFree.set_wtableid(pIServerUser->GetTableId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        LOG(INFO)<<"GAME_STATUS_END-------------------------------User Enter"<<(int)dwChairID;
        openSLog("GAME_STATUS_END: User Enter=%d;",(int)dwChairID);
        break;
    }
    default:
        break;
    }

    if(pIServerUser->GetUserStatus() == sOffline)  //left failt
    {
        pIServerUser->SetUserStatus(sPlaying);

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecorduserinfoStatu(pIServerUser->GetChairId(), sPlaying);
        }
    }

    m_bPlayerIsOperation[chairId] = true;
    return true;
}

bool CTableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    LOG(INFO)<<"User Ready userid:"<<userid<<" islookon:"<<islookon;
    openSLog("User Ready userid=%d,islookon:%d;",(int)userid,islookon);
    return true;
}

//用户离开
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
{
    LOG(INFO)<<"User Left userid:"<<userId<<" islookon:"<<islookon;
    openSLog("User Left userid=%d,islookon:%d;",(int)userId,islookon);

    bool ret = false;
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userId);
    if(!player)
    {
        LOG(ERROR)<<"---------------------------player == NULL---------------------------";
        return false;
    }
    int chairID = player->GetChairId();
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_INIT || m_cbStatus == XJH_GAME_STATUS_READY ||
            m_pITableFrame->GetGameStatus() == GAME_STATUS_END || m_cbPlayStatus[chairID] == 0)
    {
        if(player->GetUserStatus() != sPlaying)
        {
            //tmp......
            if(m_bReplayRecordStart)
            {
                RepayRecorduserinfoStatu(chairID,0);
            }
            m_cbPlayStatus[chairID] = 0;
            player->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(chairID, true, true);
            ret = true;
        }
    }

    if(XJH_GAME_STATUS_READY == m_cbStatus)
    {
        int userNum = 0;
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i]>0*/)
            {
                userNum++;
            }
        }

        if(userNum < 2)
        {
            ClearAllTimer();

            m_mUserInfo.clear(); // Add by guansheng 20190401
            m_cbStatus = GAME_STATUS_INIT;
            m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);

            LOG(ERROR)<<"---------------------------GAME_STATUS_INIT------------------------";
        }
    }

    if(player && m_pITableFrame)
    {
        if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
        {
            player->SetUserStatus(sOffline);
        }
        else
        {
            player->SetUserStatus(sGetout);
        }

        m_pITableFrame->BroadcastUserStatus(player, true);
    }

    return ret;
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
	//初始化或空闲准备阶段，可以进入桌子
	if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到游戏人数要求禁入游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			return false;
		}
		////////////////////////////////////////
		//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
        if (pUser->GetUserId() == -1) {
			if (totalMatchSeconds_ < timeoutMatchSeconds_) {
				//printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
				return false;
			}
		}
		else {
			////////////////////////////////////////
			//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
            shared_ptr<CServerUserItem> userItem = pUser;
			if (userItem) {
				if (userItem->IsAndroidUser()) {
					if (totalMatchSeconds_ < timeoutMatchSeconds_) {
						//printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
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
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    {
        auto it = m_mUserInfo.find(pUser->GetUserId());
        if(m_mUserInfo.end() == it)
            return false;
        else
            return true;
    }

	////////////////////////////////////////
	//游戏状态为GAME_STATUS_START(100)时，不会进入该函数
	assert(false);
	return false;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
        return false;

    return true;
}

void CTableFrameSink::OnTimerGameReadyOver() {
	//assert(m_pITableFrame->GetGameStatus() < GAME_STATUS_START);
	////////////////////////////////////////
	//销毁当前旧定时器
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
	if (m_pITableFrame->GetGameStatus() >= GAME_STATUS_START) {
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
				printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
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
			//printf("--- *** --------- 匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", totalMatchSeconds_, MaxGamePlayers_);
			////////////////////////////////////////
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
	}
}

void CTableFrameSink::GameTimerReadyOver()
{
	assert(m_pITableFrame->GetGameStatus() < GAME_STATUS_START);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);

    LOG(ERROR)<<"---------------------------GameTimerReadyOver---------------------------";

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            if(user->GetUserStatus() == sOffline)
            {
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(ERROR)<<"---------------------------ClearTableUse1r---------------------------";
			}
            else if (user->GetUserScore() < m_pGameRoomInfo->enterMinScore)
            {
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(ERROR)<<"---------------------------ClearTableUser2---------------------------";
            }
        }
    }

    //获取玩家个数
    int wUserCount = 0;

    //用户状态
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        //获取用户
        // shared_ptr<CServerUserItem>pIServerUser = m_pITableFrame->GetTableUserItem(i);
        if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i] > 0*/)
        {
             ++wUserCount;
        }
       // if(m_inPlayingStatus[i] > 0)
       //     ++wUserCount;
    }
    LOG(ERROR)<<"---------------------------Game Player ------"<< wUserCount <<"---------------------";
    if(wUserCount >= 2)
    {
        //清理游戏数据
        ClearGameData();
        //初始化数据
        InitGameData();

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
            }
        }
        m_cbStatus = XJH_GAME_STATUS_LOCK_PLAYING;
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
        //m_TimerIdEnterPlaying = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::GameTimereEnterPlayingOver, this));
        LOG(INFO)<<"-------设置 开始游戏状态----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId;
        openSLog("设置 开始游戏状态:tableid=%d,roomID=%d;",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId);
        GameTimereEnterPlayingOver();
    }
    else
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
        m_mUserInfo.clear(); // Add by guansheng 20190401
        m_cbStatus = XJH_GAME_STATUS_INIT;
        m_pITableFrame->SetGameStatus(GAME_STATUS_INIT);
        LOG(ERROR)<<"---------------------------SetGameStatus GAME_STATUS_INIT---------------------------";
    }
}

void CTableFrameSink::GameTimereEnterPlayingOver()
{
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
    if(m_wPlayCount < 2)
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
        m_mUserInfo.clear(); // Add by guansheng 20190401
        m_cbStatus = XJH_GAME_STATUS_INIT;
        m_pITableFrame->SetGameStatus(GAME_STATUS_INIT);
        LOG(ERROR)<<"---------------------------SetGameStatus GAME_STATUS_INIT---------------------------";
        return;
    }

    LOG(ERROR)<<"---------------------------GameTimereEnterPlayingOver----m_pITableFrame->StartGame()-----------------------";

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            m_pITableFrame->BroadcastUserInfoToOther(user);
            m_pITableFrame->SendAllOtherUserInfoToUser(user);
            m_pITableFrame->BroadcastUserStatus(user, true);
        }
    }

    OnGameStart();
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    LOG(ERROR)<<"---------------------------OnGameStart---------------------------";

    LOG(INFO)<<"-------本局游戏开始----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId<<",m_dwCellScore="<<(int)m_dwCellScore;
    openSLog("本局游戏开始 tableid=%d ,roomID=%d m_dwCellScore=%d ",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId,(int)m_dwCellScore);

    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);
    //读取配置
    ReadStorageInformation();
    m_mUserInfo.clear();
    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;

    LOG(INFO)<<"FromGameServerm_llStorageControl"<<"="<<m_llStorageControl;
    LOG(INFO)<<"FromGameServerm_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
    LOG(INFO)<<"FromGameServerm_llStorageHighLimit"<<"="<<m_llStorageHighLimit;

    m_strRoundId = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = m_strRoundId;
    LOG(INFO)<<"-------本局游戏 m_strRoundId="<<m_strRoundId;
    openSLog("-------本局游戏 m_strRoundId=%s;",m_strRoundId.c_str());
    // PaoMaDeng();//發送跑馬燈

    assert(NULL != m_pITableFrame);

    //清除所有定时器
    ClearAllTimer();
     m_dwReadyTime = (uint32_t)time(NULL);
    //库存衰减
    if (m_llStorageControl)
    {
        //m_llStorageControl -= m_wKillRatio;
    }
    m_startTime = chrono::system_clock::now();

    //发送开始消息
    XJH_CMD_S_GameStart GameStart;
    GameStart.set_roundid(m_strRoundId);
    GameStart.set_cbcallbankertime(TIME_CALL_BANKER);

    //随机扑克发牌
    //每随机10次换一次随机种子
    // m_nLottery += 1;
    // m_nLottery = m_nLottery % 10;

    // if (0 == m_nLottery)
    // {
    //     m_GameLogic.GenerateSeed(m_pITableFrame->GetTableId());
    // }
    uint8_t cbTempArray[MAX_CARD_TOTAL] = { 0 };
    uint8_t cbCardValue[MAX_SENDCOUNT] = { 0 };
    

    string strAccount = "";
    bool bResolve = false;
    // m_GameLogic.RandCardData(cbTempArray, sizeof(cbTempArray));
    // m_GameLogic.ClearUsedCard();
    //初始化
    m_GameLogic.InitCards();
    //洗牌
    m_GameLogic.ShuffleCards();

    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    memcpy(m_cbRepertoryCard, cbTempArray,sizeof(cbTempArray));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        GameStart.add_cbplaystatus(m_cbPlayStatus[i]);
        if (0 == m_cbPlayStatus[i])
            continue;

        shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
        int64_t userId = pIServerUser->GetUserId();
        auto it = m_mUserInfo.find(userId);
        if(m_mUserInfo.end() == it)
        {
            shared_ptr<UserInfo> userInfo(new UserInfo());
            userInfo->userId = pIServerUser->GetUserId();
            userInfo->chairId = pIServerUser->GetChairId();
            userInfo->headerId = pIServerUser->GetHeaderId();
            userInfo->nickName = pIServerUser->GetNickName();
            userInfo->location = pIServerUser->GetLocation();
            m_mUserInfo[userInfo->userId] = userInfo;
            m_replay.addPlayer(userInfo->userId,pIServerUser->GetAccount(),pIServerUser->GetUserScore(),i);
        }
        // m_cbLeftCardCount -= MAX_SENDCOUNT;

        // memset(cbCardValue,0,sizeof(cbCardValue));
        // bResolve = false;
        // strAccount = "";
        // strAccount = pIServerUser->GetAccount();
        // auto itAcc = m_UserTest.find(strAccount);
        // // =============== 配置测试账号做牌 start ===================
        // bool bTestCard = false;
        // if (bTestCard)
        // {
        //     if(m_UserTest.end() != itAcc)
        //     {
        //         // find Account
        //         bResolve = m_GameLogic.GetResolvePointCard(m_UserTest[strAccount], cbCardValue, m_cbRepertoryCard);
        //     }
        //     //手牌
        //     if(!bResolve)
        //     {
        //         memcpy(&m_cbHandCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(uint8_t)*MAX_SENDCOUNT);
        //         for(int j = 0; j < MAX_SENDCOUNT; ++j)
        //             m_GameLogic.InsertUsedCard(m_cbHandCardData[i][j]);
        //     }
        //     else
        //     {
        //         memcpy(&m_cbHandCardData[i], cbCardValue, sizeof(uint8_t)*MAX_SENDCOUNT);
        //     }
        // }
        // else
        // {
        //     memcpy(&m_cbHandCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(uint8_t)*MAX_SENDCOUNT);
        //     // if (!pIServerUser->IsAndroidUser())
        //     // {
        //     //     uint8_t cbText[MAX_SENDCOUNT] = { 0x3a,0x0b,0x3c,0x1d };
        //     //     memcpy(m_cbHandCardData[i], cbText, sizeof(uint8_t)*MAX_SENDCOUNT);
        //     // }
        // }
        // // =============== 配置测试账号做牌 end ===================
        // //获取玩家最优牌型组合
        // memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
        // uint8_t maxType = m_GameLogic.GetBigTypeCard(m_cbHandCardData[i], cbMaxCardValue);
        // memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
        // LOG(INFO)<<"userMaxType:"<<"="<<(int)maxType<<";cbCardData:"<<ChangCardIndex(cbMaxCardValue[0])<<","<<ChangCardIndex(cbMaxCardValue[1])<<","<<ChangCardIndex(cbMaxCardValue[2]);
        // openSLog("wChairID=%d, userMaxType:%d,cbCardData:0x%02x,0x%02x,0x%02x;",i,(int)maxType,cbMaxCardValue[0],cbMaxCardValue[1],cbMaxCardValue[2]);
    }
    // 分配手牌给玩家
    DispenseUserHandCard();

    CMD_S_GameStartAi  GameStartAi;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        if(!m_pITableFrame->GetTableUserItem(i))
            continue;

        if(!m_pITableFrame->GetTableUserItem(i)->IsAndroidUser())
            continue;

        int cardtype =(int) m_GameLogic.GetHandCardsType(m_cbCardData[i]);
        GameStartAi.set_cbsgcarddata(cardtype);
        int isMax = IsMaxCard(i);
        GameStartAi.set_ismaxcard(isMax);
        std::string data = GameStartAi.SerializeAsString();
        m_pITableFrame->SendTableData(i, XJH_SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
    }

    std::string data = GameStart.SerializeAsString();
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        //发送数据
        m_pITableFrame->SendTableData(i, XJH_SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
        //m_mOutCardRecord[i].append(" fetch " + StringCards(m_cbCardData[i], MAX_COUNT));
    }

    //设置游戏状态
    m_cbStatus = XJH_GAME_STATUS_CALL;
    // m_pITableFrame->SetGameStatus(GAME_STATUS_CALL);
    m_dwCallTime = (uint32_t)time(NULL);
    LOG(INFO)<<"-------设置 抢庄状态----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId;
    LOG(ERROR)<<"------------------SetGameStatus GAME_STATUS_CALL---------IsTrustee---------------------------";
    openSLog("设置 抢庄状态:tableid:%d,,roomID=%d;",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId);
    //tmp......
    RepayRecordStart();
    // LOG(ERROR)<<"------------------SetGameStatus GAME_STATUS_CALL---------IsTrusteeX---------------------------";
    if(m_bReplayRecordStart)
    {
        std::string data = GameStart.SerializeAsString();

        RepayRecord(INVALID_CHAIR, XJH_SUB_S_GAME_START, data.data(),data.size());
    }
    // LOG(ERROR)<<"------------------SetGameStatus GAME_STATUS_CALL---------IsTrusteeXX---------------------------";
    //设置托管
    IsTrustee();
    // LOG(ERROR)<<"------------------SetGameStatus GAME_STATUS_CALL---------IsTrusteeXI---------------------------";
    return;
}

//分配手牌给玩家
void CTableFrameSink::DispenseUserHandCard()
{
    //配置牌型
    std::vector<std::string> lines;
    int flag = 0;
    uint8_t cbMaxCardValue[MAX_COUNT] = { 0 };
    if (!INI_CARDLIST_.empty() && boost::filesystem::exists(INI_CARDLIST_)) {
        //读取配置
        readFile(INI_CARDLIST_.c_str(), lines, ";;");
        //1->文件读取手牌 0->随机生成13张牌
        flag = atoi(lines[0].c_str());
        //系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
        ratioSwap_ = atoi(lines[1].c_str());
        //系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
        ratioSwapLost_ = atoi(lines[2].c_str());
        //系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
        ratioSwapWin_ = atoi(lines[3].c_str());
        //放大倍数
        int scale = 1000;
        //系统换牌概率
        int dw[MaxExchange] = {
            ratioSwap_*scale,
            (100 - ratioSwap_)*scale };
        sysSwapCardsRatio_.init(dw, MaxExchange);
        //系统通杀概率
        int lw[MaxExchange] = {
            ratioSwapLost_*scale,
            (100 - ratioSwapLost_)*scale };
        sysLostSwapCardsRatio_.init(lw, MaxExchange);
        //系统放水概率
        int ww[MaxExchange] = {
            ratioSwapWin_*scale,
            (100 - ratioSwapWin_)*scale };
        sysWinSwapCardsRatio_.init(ww, MaxExchange);
    }
    if (flag > 0) {
        std::map<int, bool> imap;
        for (int i = 0; i < GAME_PLAYER; ++i) {
            if (!m_pITableFrame->IsExistUser(i)) {
                continue;
            }
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i); assert(userItem);
            assert(4 <= lines.size() - 1);
            int index = RandomBetween(4, lines.size() - 1);
            do {
                std::map<int, bool>::iterator it = imap.find(index);
                if (it == imap.end()) {
                    imap[index] = true;
                    break;
                }
                index = RandomBetween(4, lines.size() - 1);
            } while (true);
            //构造一副手牌5张
            int n = m_GameLogic.MakeCardList(lines[index], &(m_cbHandCardData[i])[0], MAX_SENDCOUNT);
            assert(n == MAX_SENDCOUNT);
            //手牌排序
            //m_GameLogic.SortCards(&(m_cbHandCardData[i])[0], MAX_SENDCOUNT, false, false, false);
            // 获取最大牌型
            memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
            m_handCardsType[i] = m_GameLogic.GetBigTypeCard(m_cbHandCardData[i], cbMaxCardValue);
            memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
            //手牌牌型
            // m_handCardsType[i] = m_GameLogic.GetHandCardsType(&(m_cbCardData[i])[0]);
            //对局日志
            //m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
        }
    }
    else {
        //65%概率发好牌
        int ratioDealGood = RandomBetween(1, 100);
        //给各个玩家发牌
        {
        restart:
            assert(m_pITableFrame->GetPlayerCount(true) <= GAME_PLAYER);
            for (int i = 0; i < GAME_PLAYER; ++i) {
                if (!m_pITableFrame->IsExistUser(i)) {
                    continue;
                }
                //余牌不够则重新洗牌，然后重新分发给各个玩家
                if (m_GameLogic.Remaining() < MAX_SENDCOUNT) {
                    m_GameLogic.ShuffleCards();
                    goto restart;
                }
                //发牌
                m_GameLogic.DealCards(MAX_SENDCOUNT, &(m_cbHandCardData[i])[0]);
                // 获取最大牌型
                memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
                uint8_t maxType = m_GameLogic.GetBigTypeCard(m_cbHandCardData[i], cbMaxCardValue);
                memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
            }
        }
        //各个玩家手牌分析
        {
            for (int i = 0; i < GAME_PLAYER; ++i) {
                if (!m_pITableFrame->IsExistUser(i)) {
                    continue;
                }
                shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i); assert(userItem);
                //手牌排序
                //m_GameLogic.SortCards(&(m_cbHandCardData[i])[0], MAX_COUNT, false, false, false);
                //手牌牌型
                m_handCardsType[i] = m_GameLogic.GetHandCardsType(&(m_cbCardData[i])[0]);
                //对局日志
                //m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
            }
            //好牌数量
            int c = 0;
            for (int i = 0; i < GAME_PLAYER; ++i) {
                if (!m_pITableFrame->IsExistUser(i)) {
                    continue;
                }
                if (HandTy(m_handCardsType[i]) > Tysp) {
                    ++c;
                }
            }
            //好牌数量不够，重新洗牌发牌
            if (ratioDealGood <= ratioDealGood_ && c < (m_pITableFrame->GetPlayerCount(true) - 1)) {
                goto restart;
            }
        }
    }
    
    // uint8_t cbText[GAME_PLAYER][MAX_SENDCOUNT] = {
    //             {0x27,0x1b,0x2c,0x18,0x29},
    //             {0x2b,0x3a,0x02,0x0a,0x29},
    //             {0x1d,0x24,0x03,0x12,0x2c},
    //             {0x18,0x0d,0x26,0x27,0x25},
    //             {0x2b,0x1b,0x03,0x0b,0x29}
    //         };
    //获取玩家最优牌型组合
    // for (int i = 0; i < GAME_PLAYER; ++i){
    //     if (!m_pITableFrame->IsExistUser(i)) { continue; }
    //     //if (!m_pITableFrame->IsAndroidUser(i))
    //     {
            
    //         memcpy(&m_cbHandCardData[i], &cbText[i], sizeof(uint8_t)*MAX_SENDCOUNT);
    //     }
    //     memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
    //     uint8_t maxType = m_GameLogic.GetBigTypeCard(m_cbHandCardData[i], cbMaxCardValue);
    //     memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
    //     openSLog("换牌策略分析前:wChairID=%d, userMaxType:%d,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",i,(int)maxType,m_cbHandCardData[i][0],m_cbHandCardData[i][1],m_cbHandCardData[i][2],m_cbHandCardData[i][3],m_cbHandCardData[i][4]);
    //     openSLog("wChairID=%d, userMaxType:%d,cbCardData:0x%02x,0x%02x,0x%02x;",i,(int)maxType,cbMaxCardValue[0],cbMaxCardValue[1],cbMaxCardValue[2]);
    // }
    //换牌策略分析
    // AnalysePlayerCards();
    
    //获取玩家最优牌型组合
    for (int i = 0; i < GAME_PLAYER; ++i){
        if (!m_pITableFrame->IsExistUser(i)) { continue; }
         // if (!m_pITableFrame->IsAndroidUser(i))
         // {
         //     uint8_t cbText[MAX_SENDCOUNT] = {0x25,0x12,0x23,0x18,0x39};
         //     memcpy(m_cbHandCardData[i], cbText, sizeof(uint8_t)*MAX_SENDCOUNT);
         // }
        memset(cbMaxCardValue,0,sizeof(cbMaxCardValue));
        uint8_t maxType = m_GameLogic.GetBigTypeCard(m_cbHandCardData[i], cbMaxCardValue);
        m_handCardsType[i] = maxType;
        memcpy(&m_cbCardData[i], cbMaxCardValue, sizeof(uint8_t)*MAX_COUNT);
        openSLog("初始手牌:wChairID=%d, userMaxType:%d,cbCardData:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",i,(int)maxType,m_cbHandCardData[i][0],m_cbHandCardData[i][1],m_cbHandCardData[i][2],m_cbHandCardData[i][3],m_cbHandCardData[i][4]);
        openSLog("初始组合:wChairID=%d, userMaxType:%d,cbCardData:0x%02x,0x%02x,0x%02x;",i,(int)maxType,m_cbCardData[i][0],m_cbCardData[i][1],m_cbCardData[i][2]);
    }
    // ListPlayerCards();
}
//换牌策略分析
void CTableFrameSink::AnalysePlayerCards()
{
    //当前最大牌用户
    m_currentWinUser = INVALID_CHAIR;
    //比牌判断赢家
    for (int i = 0; i < GAME_PLAYER; ++i) {
        if (!m_pITableFrame->IsExistUser(i)) {
            continue;
        }
        //初始化
        if (m_currentWinUser == INVALID_CHAIR) {
            m_currentWinUser = i;
            continue;
        }
        //比牌判断当前最大牌用户
        if (m_GameLogic.CompareHandCards(&(m_cbCardData[i])[0], &(m_cbCardData[m_currentWinUser])[0]) > 0) {
            m_currentWinUser = i;
        }
    }
    //安全断言
    assert(m_currentWinUser != INVALID_CHAIR);
    //没有机器人
    if (m_pITableFrame->GetPlayerCount(false) == m_pITableFrame->GetPlayerCount(true)) {
        return;
    }
    //机器人最大牌
    uint32_t winAndroidUser = INVALID_CHAIR;
    //比牌判断机器人最大牌
    for (int i = 0; i < GAME_PLAYER; ++i) {
        if (!m_pITableFrame->IsExistUser(i)) {
            continue;
        }
        if (!m_pITableFrame->IsAndroidUser(i)) {
            continue;
        }
        //初始化
        if (winAndroidUser == INVALID_CHAIR) {
            winAndroidUser = i;
            continue;
        }
        //比牌判断机器人最大牌
        if (m_GameLogic.CompareHandCards(&(m_cbCardData[i])[0], &(m_cbCardData[winAndroidUser])[0]) > 0) {
            winAndroidUser = i;
        }
    }
    assert(winAndroidUser != INVALID_CHAIR);
    //系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
    if (!m_pITableFrame->IsAndroidUser(m_currentWinUser) &&
        //好牌：带A的散牌或对子以上
        (m_handCardsType[winAndroidUser] >= Ty20 ||
        (m_handCardsType[winAndroidUser] == Tysp && m_GameLogic.HasCardValue(&(m_cbCardData[winAndroidUser])[0]))
            ) &&
        Exchange == CalcExchangeOrNot2(sysSwapCardsRatio_)) {
        //交换手牌
        uint8_t t[MAX_SENDCOUNT];
        memcpy(t, &(m_cbHandCardData[winAndroidUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
        memcpy(&(m_cbHandCardData[winAndroidUser])[0], &(m_cbHandCardData[m_currentWinUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
        memcpy(&(m_cbHandCardData[m_currentWinUser])[0], t, sizeof(uint8_t)*MAX_SENDCOUNT);
        //交换牌型
        std::swap(m_handCardsType[winAndroidUser], m_handCardsType[m_currentWinUser]);
        //当前最大牌用户
        m_currentWinUser = winAndroidUser;
        LOG(WARNING) << " --- *** [" << m_strRoundId << "] ################## 系统换牌 ......";
        openSLog(" 系统换牌: winAndroidUser=%d,m_currentWinUser=%d ;",winAndroidUser,m_currentWinUser);
        return;
    }
    //系统输分则通杀
    if (m_llStorageControl < m_llStorageLowerLimit) {
        //如果玩家赢
        if (!m_pITableFrame->IsAndroidUser(m_currentWinUser)) {
            //系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
            if (Exchange == CalcExchangeOrNot2(sysLostSwapCardsRatio_)) {
                //玩家赢则换牌，让机器人赢
                uint32_t newWinUser = INVALID_CHAIR;
                do {
                    //机器人赢
                    newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
                    if (m_pITableFrame->IsExistUser(newWinUser) && m_pITableFrame->IsAndroidUser(newWinUser))
                        break;
                } while (true);
                assert(newWinUser != INVALID_CHAIR);
                //交换手牌
                uint8_t t[MAX_SENDCOUNT];
                memcpy(t, &(m_cbHandCardData[newWinUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
                memcpy(&(m_cbHandCardData[newWinUser])[0], &(m_cbHandCardData[m_currentWinUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
                memcpy(&(m_cbHandCardData[m_currentWinUser])[0], t, sizeof(uint8_t)*MAX_SENDCOUNT);
                //交换牌型
                std::swap(m_handCardsType[newWinUser], m_handCardsType[m_currentWinUser]);
                //当前最大牌用户
                m_currentWinUser = newWinUser;
                LOG(WARNING) << " --- *** [" << m_strRoundId << "] ################## 通杀换牌 ......";
                openSLog(" 通杀换牌: newWinUser=%d,m_currentWinUser=%d ;",newWinUser,m_currentWinUser);
            }
            else {
                //玩家赢则不处理
            }
        }
        else {
            //机器人赢则不处理
        }
    }
    //系统赢分则放水
    else if (m_llStorageControl > m_llStorageHighLimit) {
        //如果机器人赢
        if (m_pITableFrame->IsAndroidUser(m_currentWinUser)) {
            //系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
            if (Exchange == CalcExchangeOrNot2(sysWinSwapCardsRatio_)) {
                //机器人赢则换牌，让玩家赢
                uint32_t newWinUser = INVALID_CHAIR;
                do {
                    //玩家赢
                    newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);;
                    if (m_pITableFrame->IsExistUser(newWinUser) && !m_pITableFrame->IsAndroidUser(newWinUser)) {
                        break;
                    }
                } while (true);
                //交换手牌
                uint8_t t[MAX_SENDCOUNT];
                memcpy(t, &(m_cbHandCardData[newWinUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
                memcpy(&(m_cbHandCardData[newWinUser])[0], &(m_cbHandCardData[m_currentWinUser])[0], sizeof(uint8_t)*MAX_SENDCOUNT);
                memcpy(&(m_cbHandCardData[m_currentWinUser])[0], t, sizeof(uint8_t)*MAX_SENDCOUNT);
                //交换牌型
                std::swap(m_handCardsType[newWinUser], m_handCardsType[m_currentWinUser]);
                //当前最大牌用户
                m_currentWinUser = newWinUser;
                LOG(WARNING) << " --- *** [" << m_strRoundId << "] ################## 放水换牌 ......";
                openSLog(" 放水换牌: newWinUser=%d,m_currentWinUser=%d ;",newWinUser,m_currentWinUser);
            }
            else {
                //机器人赢则不处理
            }
        }
        else {
            //玩家赢则不处理
        }
    }
}

//输出玩家手牌/对局日志初始化
// void CTableFrameSink::ListPlayerCards() 
// {
//     for (int i = 0; i < GAME_PLAYER; ++i) {
//         if (!m_pITableFrame->IsExistUser(i)) {
//             continue;
//         }
//         //对局日志
//         // m_replay.addPlayer(UserIdBy(i), ByChairId(i)->GetAccount(), ScoreByChairId(i), i);
//         //机器人AI
//         if (m_pITableFrame->IsAndroidUser(i)) {
//             LOG(ERROR) << "\n========================================================================\n"
//                 << "--- *** [" << m_strRoundId << "] 机器人 [" << i << "] " << UserIdBy(i) << " 手牌 ["
//                 << m_GameLogic.StringCards(&(m_cbHandCardData[i])[0], MAX_COUNT)
//                 << "] 牌型 [" << m_GameLogic.StringHandTy(HandTy(m_handCardsType[i])) << "]\n";
//         }
//         else {
//             LOG(WARNING) << "\n========================================================================\n"
//                 << "--- *** [" << m_strRoundId << "] 玩家 [" << i << "] " << UserIdBy(i) << " 手牌 ["
//                 << m_GameLogic.StringCards(&(m_cbHandCardData[i])[0], MAX_COUNT)
//                 << "] 牌型 [" << m_GameLogic.StringHandTy(HandTy(m_handCardsType[i])) << "]\n";
//         }
//     }
//     assert(m_currentWinUser != INVALID_CHAIR);
//     if (m_pITableFrame->IsAndroidUser(m_currentWinUser)) {
//         LOG(WARNING) << "\n--- *** [" << m_strRoundId << "] 机器人 [" << m_currentWinUser << "] " << UserIdBy(m_currentWinUser) << " 手牌 ["
//             << m_GameLogic.StringCards(&(m_cbHandCardData[m_currentWinUser])[0], MAX_SENDCOUNT)
//             << "] 牌型 [" << m_GameLogic.StringHandTy(HandTy(m_handCardsType[m_currentWinUser])) << "] 最大牌\n\n\n";
//     }
//     else {
//         LOG(WARNING) << "\n--- *** [" << m_strRoundId << "] 玩家 [" << m_currentWinUser << "] " << UserIdBy(m_currentWinUser) << " 手牌 ["
//             << m_GameLogic.StringCards(&(m_cbHandCardData[m_currentWinUser])[0], MAX_SENDCOUNT)
//             << "] 牌型 [" << m_GameLogic.StringHandTy(HandTy(m_handCardsType[m_currentWinUser])) << "] 最大牌\n\n\n";
//     }
// }

//拼接各玩家手牌cardValue
std::string CTableFrameSink::StringCardValue() 
{
    char ch[10] = { 0 };
    std::string str = "";
    for (int i = 0; i < GAME_PLAYER; i++) {
        if (m_pITableFrame->IsExistUser(i)) {
            string cards;
            for (int j = 0; j < MAX_SENDCOUNT; ++j) {
                sprintf(ch, "%02X", m_cbHandCardData[i][j]);
                cards += ch;
            }
            str += cards;
        }
        else {
            str += "000000";
        }
    }
    if (INVALID_CHAIR != m_currentWinUser) {
        sprintf(ch, "%02d", m_currentWinUser);
        str += ch;
    }
    return str;
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

        int wCallFlag = 0;
        int32_t cbCallBanker = m_sCallBanker[i];
        if (cbCallBanker == -1)
        {
            //设置没有操作的玩家，不叫庄
            wCallFlag = 0;
        }
        else
        {
            continue; //过滤掉没有叫庄和叫庄的人
        }

        OnUserCallBanker(i, wCallFlag);
        //LOG_IF(INFO, PRINT_GAME_INFO) << "OnTimerMessage auto call chairId = " << i;
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

    //记录叫庄
    m_sCallBanker[wChairID] = wCallFlag;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wCallFlag),-1,opBet,wChairID,wChairID);
    LOG(INFO)<<"-------玩家叫庄----------wChairID="<<wChairID<<",wCallFlag="<<wCallFlag;
    openSLog("玩家叫庄:wChairID=%d,wCallFlag=%d;",wChairID,wCallFlag);
    int wTotalCount = 0; //叫庄操作人数
    int wCallBankerCount = 0; //叫庄人数
    int wCallbankerIndex[GAME_PLAYER];
    memset(wCallbankerIndex, 0, sizeof(wCallbankerIndex));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有叫庄操作玩家
        if (-1 == m_sCallBanker[i])
        {
            continue;
        }
        else if(m_sCallBanker[i] == 1)
        {
            wCallbankerIndex[wCallBankerCount++] = i; //记录叫庄的座椅号
        }
        //记录操作人数总人数
        ++wTotalCount;
    }

    //叫庄
    XJH_CMD_S_CallBanker callBanker;
    callBanker.set_wcallbankeruser( wChairID);
    callBanker.set_cbcallmultiple(wCallFlag);
    std::string data = callBanker.SerializeAsString();
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_cbPlayStatus[i])
        m_pITableFrame->SendTableData(i, XJH_SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());
    }

    //tmp......
    if(m_bReplayRecordStart)
    {
        RepayRecord(INVALID_CHAIR, XJH_SUB_S_CALL_BANKER, data.c_str(),data.size());
    }
    //记录叫庄玩家
    int wBankerUser = INVALID_CHAIR;

    XJH_CMD_S_CallBankerResult CallResult;

    if(wTotalCount == m_wPlayCount)   //所有的玩家都进行了叫庄操作(叫庄或者不叫)
    {
        if(wCallBankerCount > 0) //有人叫庄
        {
            int wSeletCallBankerIndex = rand()%wCallBankerCount;
            wBankerUser = wCallbankerIndex[wSeletCallBankerIndex];
            for(int j = 0; j < wCallBankerCount; j++)
            {
                CallResult.add_cbcallbankeruser(wCallbankerIndex[j]);
            }
        }
        else //没人叫庄
        {
            wBankerUser = INVALID_CHAIR;
            for(int j = 0; j < GAME_PLAYER; j++) //如果没人叫庄，认为每个玩家都叫庄
            {
                if(m_sCallBanker[j] == 0)
                    CallResult.add_cbcallbankeruser(m_sCallBanker[j]);
            }
        }

        //没有人叫庄
        if (wBankerUser == INVALID_CHAIR)
        {
            wBankerUser = RandBanker();
        }

        m_wBankerUser = wBankerUser;
        m_sCallBanker[m_wBankerUser] = 1;

        //设置游戏状态
        // m_cbStatus = XJH_GAME_STATUS_OPEN;   // XJH_GAME_STATUS_SCORE;
        // m_dwOpenTime = (uint32_t)time(NULL); // m_dwScoreTime
        // //设置托管
        // IsTrustee();

        //发送叫庄结果
        if (wBankerUser != INVALID_CHAIR)
        {
            m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wBankerUser),-1,opBanker,-1,-1);
            //庄已经确认下来，可以确认每个闲家的最大下注倍数
            // GetAllUserMaxAddScore();
            // for (int i = 0; i < GAME_PLAYER; ++i)
            // {
            //     //设置每个闲家的最大下注倍数
            //     for(int j = 0; j < GAME_JETTON_COUNT; ++j)
            //     CallResult.add_lmaxjettonmultiple(m_cbMaxJetton[i][j]);
            // }

            CallResult.set_dwbankeruser(wBankerUser);
            CallResult.set_cbopencardtime(TIME_OPEN_CARD);			//加注时间

            LOG(ERROR) << "XJH_SUB_S_CALL_BANKER_RESULT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" ;
            LOG(INFO)<<"-------抢庄结束----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId<<",wBankerUser="<<wBankerUser;
            openSLog("抢庄结束:tableid=%d,roomID=%d,wBankerUser=%d;",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId,wBankerUser);
            std::string data = CallResult.SerializeAsString();
            for(int i = 0; i < GAME_PLAYER; ++i)
            {                
                if (1 == m_cbPlayStatus[i])
                    m_pITableFrame->SendTableData(i, XJH_SUB_S_CALL_BANKER_RESULT, (uint8_t*)data.c_str(), data.size());
            }

            //tmp......
            if(m_bReplayRecordStart)
            {
                RepayRecord(INVALID_CHAIR, XJH_SUB_S_CALL_BANKER_RESULT, data.c_str(),data.size());
            }
            //发玩家手牌
            OnSendHandCard();
        }  
    }

    return 4;
}

//随机庄家
int CTableFrameSink::RandBanker()
{
#if 0
    int wCount = 0;
    int wTempBanker = rand() % m_wPlayCount;
    while(1 != m_cbPlayStatus[wTempBanker] && wCount < 101)
    {
        wTempBanker = rand() % m_wPlayCount;
        ++wCount;
    }
    if (wCount > 100)
    {
        if (1 != m_cbPlayStatus[wTempBanker])
        {
            for (uint8_t i = 0; i < GAME_PLAYER; ++i)
            {
                if (1 == m_cbPlayStatus[i])
                {
                    wTempBanker = i;
                    break;
                }
            }
        }
    }
    return wTempBanker;
#endif
    int wCallBankerCount = 0; //叫庄人数
    int wTotalCount = 0; //叫庄操作人数
    int wCallbankerIndex[GAME_PLAYER];
    memset(wCallbankerIndex, 0, sizeof(wCallbankerIndex));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        wCallbankerIndex[wCallBankerCount++] = i; //记录叫庄的座椅号
        //记录操作人数总人数
        ++wTotalCount;
    }
    int wSeletCallBankerIndex = rand()%wCallBankerCount;
    return wCallbankerIndex[wSeletCallBankerIndex];
}

//发手牌给玩家
int CTableFrameSink::OnSendHandCard()
{
    //是否发牌
    uint8_t cbIsSendCard = 0;
    int dwOpenTime = 0;    
    //设置游戏状态
    // m_pITableFrame->SetGameStatus(GAME_STATUS_OPEN);
    m_cbStatus = XJH_GAME_STATUS_OPEN;
    LOG(INFO)<<"SetGameStatus GAME_STATUS_OPEN";
    LOG(INFO)<<"-------设置 开牌状态----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId;
    openSLog("设置 开牌状态:tableid=%d,roomID=%d;",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId);
    m_dwOpenTime = (uint32_t)time(NULL);
    //设置发牌标志
    cbIsSendCard = 1;
    dwOpenTime = TIME_OPEN_CARD;
    //分析玩家牌
    AnalyseCardEx(); //三公库存控制
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(!user||user->IsAndroidUser()) continue;

        if(user->GetBlacklistInfo().status==1)
        {
           if(rand()%1000<user->GetBlacklistInfo().weight)
           {
               TrageBlackCard(i);
           }
        }
    }
    //在这里判断一下是否有黑名单玩家,以便做换牌控制
    //TrageBlackCard(int64_t chairid )//传入黑名单的chairid,做换牌处理
    //设置托管
    IsTrustee();

    //发牌
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i]) continue;
        if (1 == cbIsSendCard)
        {
            //有发牌
            XJH_CMD_S_SendCard sendCard;
            for(int j = 0; j < MAX_SENDCOUNT; ++j)
            {
                sendCard.add_cbsendcard(0);
            }
            for(int j = 0; j < MAX_COUNT; ++j)
            {
                sendCard.add_cbsgcard(0);
            }
            sendCard.set_cbcardtype(0);
            sendCard.set_cbopentime(dwOpenTime);

            if(m_cbPlayStatus[i] != 0 )
            {
                uint8_t cbCardType = m_GameLogic.GetHandCardsType(m_cbCardData[i]);
                sendCard.set_cbcardtype(cbCardType);

                //提牌数据
                uint8_t cbSGCard[MAX_COUNT] = { 0 };
                memcpy(cbSGCard, &m_cbCardData[i], sizeof(uint8_t)*MAX_COUNT);
                //4张手牌
                for(int j = 0; j < MAX_SENDCOUNT; ++j)
                {
                    sendCard.set_cbsendcard(j, m_cbHandCardData[i][j]);
                }
                LOG(INFO)<<"-------玩家手牌:wChairID="<<i<<",cbHandCardData="<<ChangCardIndex(m_cbHandCardData[i][0])<<","<<ChangCardIndex(m_cbHandCardData[i][1])<<","<<ChangCardIndex(m_cbHandCardData[i][2])<<","<<ChangCardIndex(m_cbHandCardData[i][3])<<","<<ChangCardIndex(m_cbHandCardData[i][4]);
                openSLog("玩家手牌:wChairID=%d:cbHandCardData=0x%02x,0x%02x,0x%02x,0x%02x,0x%02x;",i,m_cbHandCardData[i][0],m_cbHandCardData[i][1],m_cbHandCardData[i][2],m_cbHandCardData[i][3],m_cbHandCardData[i][4]);
                //最优牌型数据
                for(int j = 0; j < MAX_COUNT; ++j)
                {
                    sendCard.set_cbsgcard(j, cbSGCard[j]);
                }
                LOG(INFO)<<"-------最优组合:wChairID="<<i<<",cbSGCard="<<ChangCardIndex(cbSGCard[0])<<","<<ChangCardIndex(cbSGCard[1])<<","<<ChangCardIndex(cbSGCard[2]);
                openSLog("最优组合:wChairID=%d:cbSGCard=0x%02x,0x%02x,0x%02x;",i,cbSGCard[0],cbSGCard[1],cbSGCard[2]);
            }

            std::string data = sendCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, XJH_SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());

            //tmp......
            if(m_bReplayRecordStart && 1 == m_cbPlayStatus[i])
                //if(m_bReplayRecordStart )
            {
                RepayRecord(i, XJH_SUB_S_SEND_CARD, data.data(),data.size());
            }
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
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        bSteal = true;
    }
    else
    {
        if (m_llStorageControl > llLowerLimitVal && m_llStorageControl < llHighLimitVal)
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



    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //临时变量
    uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));
    //手牌
    uint8_t cbTmpHandCardData[GAME_PLAYER][MAX_SENDCOUNT];
    memcpy(cbTmpHandCardData, m_cbHandCardData, sizeof(cbTmpHandCardData));
    //牌型
    uint8_t cbTmpCardType[GAME_PLAYER];
    memcpy(cbTmpCardType, m_handCardsType, sizeof(cbTmpCardType));
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
            bool bCom = m_GameLogic.CompareHandCards(cbUserCardData[i], m_cbCardData[wWinUser]) > 0 ? true : false;
            if (bCom == bSteal)
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
            openSLog("AnalyseCardEx:换牌:%d<<==>>%d;",i,cbTmpValue);
            memcpy(m_cbCardData[i], cbTmpCardData[cbTmpValue], MAX_COUNT);
            memcpy(m_cbHandCardData[i], cbTmpHandCardData[cbTmpValue], MAX_SENDCOUNT);
            //交换牌型
            m_handCardsType[i] = cbTmpCardType[cbTmpValue];
        }
    }
}
//假如是黑名单，就把比他小的机器人的牌换给这个鸡巴
 void CTableFrameSink::TrageBlackCard(int64_t chairid )
 {
    shared_ptr<CServerUserItem> blackUser= m_pITableFrame->GetTableUserItem(chairid);
    if(!blackUser) return;

     //临时变量
     uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
     uint8_t cbTmpBlackeCardDatax[MAX_COUNT];
     memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));
     memcpy(cbTmpBlackeCardDatax, m_cbCardData[chairid], sizeof(cbTmpBlackeCardDatax));
     for(int i=0;i<GAME_PLAYER;i++)
     {
         shared_ptr<CServerUserItem> player= m_pITableFrame->GetTableUserItem(i);
         if(!player||!player->IsAndroidUser()||i==chairid) continue;

         if(m_GameLogic.CompareHandCards(cbTmpCardData[i], cbTmpBlackeCardDatax) < 0)
         {
             uint8_t cbTmp[MAX_COUNT];
             memcpy(cbTmp, cbTmpBlackeCardDatax, sizeof(cbTmp));
             memcpy(cbTmpBlackeCardDatax, m_cbCardData[i], sizeof(cbTmpBlackeCardDatax));
             memcpy(m_cbCardData[i], cbTmp, sizeof(cbTmp));
             memcpy(m_cbCardData[chairid], cbTmpBlackeCardDatax, sizeof(cbTmpBlackeCardDatax));
             openSLog("TrageBlackCard:换牌:%d<<==>>%d;",i,chairid);
             //换手牌
             uint8_t cbHandTmp[MAX_SENDCOUNT];
             memcpy(cbHandTmp, m_cbHandCardData[chairid], sizeof(cbHandTmp));
             memcpy(m_cbHandCardData[chairid], m_cbHandCardData[i], sizeof(m_cbHandCardData[chairid]));
             memcpy(m_cbHandCardData[i], cbHandTmp, sizeof(cbHandTmp));
             //交换牌型
            std::swap(m_handCardsType[i], m_handCardsType[chairid]);
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
        int cbOpenFlag = 0;
        OnUserOpenCard(i,cbOpenFlag,m_cbCardData[i]);
        //	LOG_IF(INFO, PRINT_GAME_INFO) << "OnTimerMessage auto open card chairId = " << i;
    }
    return 0;
}

//玩家开牌
int CTableFrameSink::OnUserOpenCard(int wChairID,int cbOpenFlag, uint8_t cbCardData[MAX_COUNT])
{
    if (-1 != m_sCardType[wChairID])
    {
        //已开牌
        return 1;
    }
    LOG(INFO)<<"-----------wChairID:"<<(int)wChairID<<"---choose card:"<<ChangCardIndex(cbCardData[0])<<","<<ChangCardIndex(cbCardData[1])<<","<<ChangCardIndex(cbCardData[2]);
    openSLog("OnUserOpenCard:wChairID=%d,chooseFlag=%d,chooseCard=0x%02x,0x%02x,0x%02x;",wChairID,cbOpenFlag,cbCardData[0],cbCardData[1],cbCardData[2]);
    int nBigType = (int)m_GameLogic.GetHandCardsType(m_cbCardData[wChairID]);
    int nChooseType = 0;
    //判断是否选择最优牌型
    if(cbOpenFlag!=0) //玩家自己选牌
    {
        bool bCheck = m_GameLogic.checkSameCard(m_cbHandCardData[wChairID], cbCardData);//检验是否服务器的牌值
        if (!bCheck)
        {
            XJH_CMD_S_OperateFail OperateFail;
            int reason = 0;
            OperateFail.set_cbreason(reason);

            std::string data = OperateFail.SerializeAsString();
            //发送数据
            m_pITableFrame->SendTableData(wChairID, XJH_SUB_S_OPERATE_FAIL, (uint8_t*)data.c_str(), data.size());

            return 3; //不是最优牌型
        }

        bool bBigType = m_GameLogic.IsSameCard(m_cbCardData[wChairID],cbCardData);
        if (!bBigType)
        {
            XJH_CMD_S_OperateFail OperateFail;
            int reason = 0;
            OperateFail.set_cbreason(reason);

            std::string data = OperateFail.SerializeAsString();
            //发送数据
            m_pITableFrame->SendTableData(wChairID, XJH_SUB_S_OPERATE_FAIL, (uint8_t*)data.c_str(), data.size());

            return 3; //不是最最优牌型
        }
        else
        {
            memcpy(&m_cbCardData[wChairID], cbCardData, sizeof(uint8_t)*MAX_COUNT);
        }
    }

    //记录牌型
    m_sCardType[wChairID] = nBigType;//(int)m_GameLogic.GetHandCardsType(m_cbCardData[wChairID]);

    //倍数
    m_iMultiple[wChairID] =(int)m_GameLogic.GetMultiple(m_cbCardData[wChairID]);

    uint8_t cbSGCard[MAX_COUNT] = { 0 };
    memcpy(cbSGCard, &m_cbCardData[wChairID], sizeof(uint8_t)*MAX_COUNT);
    memcpy(&m_cbMaxCard[wChairID], cbSGCard, sizeof(cbSGCard));

    //发送开牌结果
    XJH_CMD_S_OpenCardResult OpenCardResult;
    OpenCardResult.set_wopencarduser( wChairID);
    OpenCardResult.set_cbcardtype(m_sCardType[wChairID]);
    for(int j = 0; j < MAX_COUNT; ++j)
    {
        OpenCardResult.add_cbsgcarddata(cbSGCard[j]);
    }

    std::string data = OpenCardResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, XJH_SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

    //tmp......
    if(m_bReplayRecordStart)
    {
        RepayRecord(INVALID_CHAIR,XJH_SUB_S_OPEN_CARD_RESULT, data.c_str(),data.size());
    }
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

    LOG(INFO)<<"OnEventGameConclude Game End";
    openSLog("OnEventGameConclude:%s;","OnEventGameConclude Game End");
    m_dwEndTime = (uint32_t)time(NULL);

    //LOG_IF(INFO, PRINT_GAME_INFO) << "OnEventGameEnd";

    switch (cbReason)
    {
        case GER_NORMAL:
        {//正常退出
            NormalGameEnd(wChairID);
            break;
        }
        case GER_USER_LEFT:
        {//玩家强制退出,则把该玩家托管不出。
            break;
        }
        case GER_DISMISS:
        {//游戏解散
            break;
        }
        default:
            break;
    }
    //游戏结束
    m_pITableFrame->ConcludeGame(GAME_STATUS_END);

    //设置游戏状态-空闲状态
    m_pITableFrame->SetGameStatus(GAME_STATUS_INIT);

    //tmp......
    if(m_bReplayRecordStart)
    {
        RepayRecordEnd();
    }

    ClearUserInfo();
    m_mUserInfo.clear();
    //设置托管
   // IsTrustee();
    return true;
}

//普通场游戏结算
void CTableFrameSink::NormalGameEnd(int dwChairID)
{
    XJH_CMD_S_GameEnd GameEnd;

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
    assert(m_sCallBanker[m_wBankerUser] == 1);
    //庄家总赢的金币
    int64_t iBankerWinScore = 0;
    //庄家总输赢金币
    int64_t iBankerTotalWinScore = 0;
    //庄家牌型倍数
    int64_t cbBankerCardMultiple = m_GameLogic.GetMultiple(m_cbCardData[m_wBankerUser]);
    //庄家赢总倍数

    /****
    闲家信息
    ****/
    //闲家赢的玩家总金币
    int64_t iUserTotalWinScore = 0;
    //闲家输的玩家总金币
    int64_t iUserTotalLostScore = 0;
    //闲家本金
    int64_t llUserBaseScore[GAME_PLAYER] = { 0,0,0,0,0 };
    //闲家各自赢分
    int64_t iUserWinScore[GAME_PLAYER] = { 0,0,0,0,0 };
    memset(m_iUserWinScore,0x00,sizeof(m_iUserWinScore));
    //下注倍数
    int64_t cbUserJettonMultiple[GAME_PLAYER] = { 0,0,0,0,0 };
    //玩家牌型倍数
    int64_t cbUserCardMultiple[GAME_PLAYER] = { 0,0,0,0,0 };
    //闲家输总倍数
    int64_t wWinUserTotalMultiple = 0;
    //闲家赢总倍数
    int64_t wLostUserTotalMultiple = 0;
    int64_t calcMulti[GAME_PLAYER] = { 0 };
    int64_t wLostUserCount = 0;
    LOG(INFO)<<" 庄家牌型倍数 m_wBankerUser====="<<(int)m_wBankerUser<<","<<(int)m_GameLogic.GetHandCardsType(m_cbCardData[m_wBankerUser])<<","<<(int)cbBankerCardMultiple;
    openSLog("庄家牌型倍数:m_wBankerUser=%d,cardType=%d,beishu=%d;",m_wBankerUser,(int)m_GameLogic.GetHandCardsType(m_cbCardData[m_wBankerUser]),(int)cbBankerCardMultiple);
    //计算输赢
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(0 == m_cbPlayStatus[i])
            continue;//zhuan tai wei ling bu neng canyu jiesuan

        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
            continue;

		if (i == m_wBankerUser)
		{
			llUserBaseScore[i] = puserinfo->GetUserScore();
			continue;// zhuangjia bucanyu wanjia jiesuan
		}

        cbUserJettonMultiple[i] = 1; //m_sAddScore[i]; 没有加注倍数,默认为1倍

        cbUserCardMultiple[i] = m_GameLogic.GetMultiple(m_cbCardData[i]);

        LOG(INFO)<<" 闲家牌型倍数:cbUserCardMultiple[i]="<< i <<","<<(int)m_GameLogic.GetHandCardsType(m_cbCardData[i])<<","<< (int)cbUserCardMultiple[i];
        openSLog("闲家牌型倍数:chairID=%d,cardType=%d,beishu=%d;",i,(int)m_GameLogic.GetHandCardsType(m_cbCardData[i]),(int)cbUserCardMultiple[i]);
        //闲家原始积分

        llUserBaseScore[i] = puserinfo->GetUserScore();

        int64_t iTempScore = 0;
		calcMulti[m_wBankerUser] = cbBankerCardMultiple;
        if(m_GameLogic.CompareHandCards(m_cbCardData[m_wBankerUser], m_cbCardData[i])>0)
        {
            //庄赢  底分 * 下注倍数 * 牌型倍数
            iTempScore = m_dwCellScore * cbUserJettonMultiple[i] * cbBankerCardMultiple;
            LOG(INFO)<<" iTempScore "<<iTempScore <<"   m_dwCellScore"<<m_dwCellScore<<" cbUserJettonMultiple[i]"<<cbUserJettonMultiple[i]<<" cbBankerCardMultiple"<<cbBankerCardMultiple;
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }
			calcMulti[i] = cbBankerCardMultiple;
            iBankerWinScore += iTempScore;
            iBankerTotalWinScore += iTempScore;
            iUserTotalLostScore += iTempScore;
            //玩家总输分
            iUserWinScore[i] = -iTempScore;

            //玩家总赢倍数
           // wLostUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];
            wLostUserTotalMultiple += cbUserJettonMultiple[i]; // modify by guansheng 20190412

            //玩家输的人数
            ++wLostUserCount;
        }else
        {
            //闲赢
            iTempScore = m_dwCellScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i];
            LOG(INFO)<<" iTempScore "<<iTempScore <<"   m_dwCellScore"<<m_dwCellScore<<" cbUserJettonMultiple[i]"<<cbUserJettonMultiple[i]<<" cbBankerCardMultiple"<<cbBankerCardMultiple;
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }
			calcMulti[i] = cbUserCardMultiple[i];
            iUserTotalWinScore += iTempScore;
            iBankerTotalWinScore -= iTempScore;

            //玩家赢分
            iUserWinScore[i] = iTempScore;
            //输家总倍数
            wWinUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];
        }
    }

    //庄家计算出来的赢分
    int64_t iBankerCalWinScore = 0;

    //如果庄家赢的钱大于庄家的基数
    if (iBankerTotalWinScore > llBankerBaseScore)
    {
        //庄家应该收入金币最多为 庄家本金+闲家总赢的金币
        iBankerCalWinScore = llBankerBaseScore + iUserTotalWinScore;

        //按照这个金额平摊给输家
       // int64_t iEachPartScore = iBankerCalWinScore / wLostUserTotalMultiple;
        int64_t bankerliss=0;
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0 || i == m_wBankerUser)
                continue;

            if (iUserWinScore[i] > 0)
                continue;

            //当前玩家真正要输的金币分摊一下
           // int64_t iUserNeedLostScore = iEachPartScore * cbUserJettonMultiple[i] /** cbUserCardMultiple[i]*/; //-modify by guansheng 20190412
            // int64_t iUserNeedLostScore = (iBankerCalWinScore * cbUserJettonMultiple[i])/wLostUserTotalMultiple; //-modify by guansheng 20190412
            int64_t iUserNeedLostScore = iBankerCalWinScore * abs(iUserWinScore[i]) / iUserTotalLostScore;
            LOG(INFO)<<" iUserNeedLostScore  "<<iUserNeedLostScore;
            if (iUserNeedLostScore > llUserBaseScore[i])
            {
                iUserNeedLostScore = llUserBaseScore[i];
                bankerliss-=(iUserNeedLostScore-llUserBaseScore[i]);///////////yufang wanjia bu gou
            }

            iUserWinScore[i] = -iUserNeedLostScore;
        }

        //庄家赢的钱是庄家本金
        iUserWinScore[m_wBankerUser] = llBankerBaseScore+bankerliss;

        //闲家赢钱的，再赢多少就赢多少，不需要有变动
    }
    else if (iBankerTotalWinScore + llBankerBaseScore < 0)   //如果庄家的钱不够赔偿
    {
        //庄家能赔付的钱总额
        int64_t iBankerMaxScore = llBankerBaseScore + iBankerWinScore;

        //计算玩家每部分的钱
       // int64_t iEachPartScore = iBankerMaxScore / wWinUserTotalMultiple;

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

           // int64_t iUserRealWinScore = iEachPartScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i];
            // int64_t iUserRealWinScore = (iBankerMaxScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i])/wWinUserTotalMultiple;
            int64_t iUserRealWinScore = iBankerMaxScore * iUserWinScore[i] / iUserTotalWinScore;
            LOG(INFO)<<" iUserRealWinScore  "<<iUserRealWinScore;
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
        LOG(INFO)<<" 庄家输赢iBankerTotalWinScore  "<<iBankerTotalWinScore;
    }

    //写入积分
    int64_t iRevenue[GAME_PLAYER] = { 0,0,0,0,0 };
    int64_t CalculateAgentRevenue[GAME_PLAYER] = { 0,0,0,0,0 };
    int userCount=0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        GameEnd.add_dlwscore(0);
        GameEnd.add_dtotalscore(0);
        GameEnd.add_cboperate(0);
    }

    int64_t res = 0.0;

    int iIndex = 0;
    int64_t revenue;
    memset(m_cbSGRecord, 0, sizeof(m_cbSGRecord));
    memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if (0 == m_cbPlayStatus[i])
        {
            for(int j = 0; j < MAX_SENDCOUNT;++j)
                m_cbSGRecord[iIndex++] = 0;

            continue;
        }
        if(!puserinfo)
        {
             continue;
        }
        for(int j = 0; j < MAX_SENDCOUNT;++j)
            m_cbSGRecord[iIndex++] = m_cbHandCardData[i][j];

        if(!m_bPlayerIsOperation[i])    //没有操作
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
       }

        if (iUserWinScore[i] > 0)
        {
            iRevenue[i] = m_pITableFrame->CalculateRevenue(iUserWinScore[i]);
        }
        else
        {
            iRevenue[i] = 0;
           // CalculateAgentRevenue[i] =m_pITableFrame->CalculateAgentRevenue(i,-iUserWinScore[i]);
        }

        int64_t iLWScore = iUserWinScore[i] - iRevenue[i];
#if 0
        m_RecordScoreInfo[userCount].chairId=i;
        m_RecordScoreInfo[userCount].addScore=iLWScore;
        m_RecordScoreInfo[userCount].betScore=m_sAddScore[i];
        m_RecordScoreInfo[userCount].revenue=iRevenue[i];
        m_RecordScoreInfo[userCount].startTime=m_startTime;
#endif
        m_RecordScoreInfo[puserinfo->GetChairId()].chairId=i;

        m_RecordScoreInfo[puserinfo->GetChairId()].addScore=iLWScore;
        m_RecordScoreInfo[puserinfo->GetChairId()].betScore= m_sAddScore[i];
        m_RecordScoreInfo[puserinfo->GetChairId()].revenue=iRevenue[i];
        m_RecordScoreInfo[puserinfo->GetChairId()].startTime=m_startTime;
        int userID = (int)puserinfo->GetUserId();
        LOG(INFO)<<"userscore "<<(int)puserinfo->GetUserId()<<" "<<llUserBaseScore[i]<<" chairid="<<i;
        LOG(INFO)<<"bankerscore "<<iBankerTotalWinScore;
        LOG(INFO)<<"userid "<<(int)puserinfo->GetUserId()<<" iUserWinScore[i] "<<iUserWinScore[i];
        LOG(INFO)<<"userid "<<(int)puserinfo->GetUserId()<<" m_RecordScoreInfo[userCount].nAgentRevenueScore "<<CalculateAgentRevenue[i];
        LOG(INFO)<<"userid "<<(int)puserinfo->GetUserId()<<" m_RecordScoreInfo[userCount].nRevenuedScore "<<iLWScore;
        LOG(INFO)<<"userid "<<(int)puserinfo->GetUserId()<<" m_RecordScoreInfo[userCount].nAddScore "<<iLWScore;
        LOG(INFO)<<"userid "<<(int)puserinfo->GetUserId()<<" m_RecordScoreInfo[userCount].nRevenue "<<iRevenue[i];
        LOG(INFO)<<"-----------------------------------------";

        openSLog("userscore:userID=%d,llUserBaseScore=%d,chairid=%d;",userID,(int)llUserBaseScore[i],i);
        openSLog("bankerscore=%d;",iBankerTotalWinScore);
        openSLog("userID=%d,iUserWinScore=%d;",userID,iUserWinScore[i]);
        openSLog("userID=%d,m_RecordScoreInfo[userCount].nAgentRevenueScore=%d;",userID,CalculateAgentRevenue[i]);
        openSLog("userID=%d,m_RecordScoreInfo[userCount].nRevenuedScore=%d;",userID,iLWScore);
        openSLog("userID=%d,m_RecordScoreInfo[userCount].nAddScore=%d;",userID,iLWScore);
        openSLog("userID=%d,m_RecordScoreInfo[userCount].nRevenue=%d;",userID,iRevenue[i]);

        userCount++;
        GameEnd.set_dlwscore(i, iLWScore);
        m_iUserWinScore[i] = iLWScore;
        GameEnd.set_dtotalscore(i,iLWScore+ puserinfo->GetUserScore());
        PaoMaDengCardType[i]=m_GameLogic.GetHandCardsType(m_cbCardData[i]);
        PaoMaDengWinScore[i]=iLWScore;
        LOG(INFO)<<"m_llStorageControl"<<"="<<m_llStorageControl;
        LOG(INFO)<<"m_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
        LOG(INFO)<<"m_llStorageHighLimit"<<"="<<m_llStorageHighLimit;

        LOG(INFO)<<"palyerCardtype="<<(int)puserinfo->GetUserId()<<"    "<<(int)PaoMaDengCardType[i];

        openSLog("m_llStorageControl=%d;",m_llStorageControl);
        openSLog("m_llStorageLowerLimit=%d;",m_llStorageLowerLimit);
        openSLog("m_llStorageHighLimit=%d;",m_llStorageHighLimit);
        openSLog("palyerCardtype=%d,%d;",userID,(int)PaoMaDengCardType[i]);

        // for(int j = 0; j < 3; ++j)
        // {
        //     int inde = (int)m_cbCardData[i][j] % 16 + ((int)m_cbCardData[i][j] / 16) * 100;
        //     LOG(INFO)<<"player="<<(int)puserinfo->GetUserId()<<"      palyerCard="<<inde;
        // }
        LOG(INFO)<<"player="<<(int)puserinfo->GetUserId()<<"===>palyerCard="<<ChangCardIndex(m_cbCardData[i][0])<<","<<ChangCardIndex(m_cbCardData[i][1])<<","<<ChangCardIndex(m_cbCardData[i][2]);
        openSLog("player=%d,palyerCard=0x%02x,0x%02x,0x%02x;",userID,m_cbCardData[i][0],m_cbCardData[i][1],m_cbCardData[i][2]);
    }

    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update before-----------------"<<ceshi.lEndStorage;
    LOG(INFO)<<"update Num-----------------"<<res;
    if( res > 0)
    {
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0; //- m_pITableFrame->CalculateRevenue(res);
    }

    m_pITableFrame->UpdateStorageScore(res);

    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update after -----------------"<<ceshi.lEndStorage;

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
       // m_RecordScoreInfo[i].cardValue = GlobalFunc::converToHex((uint8_t*)m_cbSGCardRecord, GAME_PLAYER*MAX_COUNT);
        int64_t aaaa = puserinfo->GetUserScore();
        LOG(INFO)<<"userid="<<puserinfo->GetUserId()<<"    score="<<aaaa;
        openSLog("userid=%d,score=%d;",puserinfo->GetUserId(),aaaa);
    }
   // m_pITableFrame->WriteUserScore(m_RecordScoreInfo, userCount, m_strRoundId);
	//判断通杀通赔
	if (m_wPlayCount > 2)
	{
		if (wLostUserCount == 0)
		{
			GameEnd.set_cbendtype(2);
		}
		else if (wLostUserCount == (m_wPlayCount - 1))
		{
			GameEnd.set_cbendtype(1);
		}
		else
		{
			GameEnd.set_cbendtype(0);
		}
	}
	else
	{
		GameEnd.set_cbendtype(-1);
	}
	//对局记录详情
	SetGameRecordDetail(llUserBaseScore, m_iUserWinScore, calcMulti);
    WriteUserScore();

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
        openSLog("userid=%d,score=%d;",puserinfo->GetUserId(),(int)aaa);
    }

    //发送数据
    std::string data = GameEnd.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, XJH_SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());

    PaoMaDeng();//發送跑馬燈
    //tmp......
    if(m_bReplayRecordStart)
    {
        RepayRecord(INVALID_CHAIR,XJH_SUB_S_GAME_END, data.c_str(),data.size());
    }

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(!user)
        {
            continue;
        }

        user->SetUserStatus(sOffline);
    }
    LOG(INFO)<<"-------本局游戏结束----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId<<",m_dwCellScore="<<(int)m_dwCellScore;
    openSLog("本局游戏结束 tableid=%d ,roomID=%d m_dwCellScore=%d ",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId,(int)m_dwCellScore);
}

void CTableFrameSink::WriteUserScore()
{
    vector<tagScoreInfo> scoreInfoVec;

    tagScoreInfo scoreInfo;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    int64_t rBankerWinscore = 0;
    int bankerScoreIdx = 0;

    string cardValue = GlobalFunc::converToHex((uint8_t*)m_cbSGRecord, GAME_PLAYER*MAX_SENDCOUNT);
    int Banker = m_wBankerUser;
    cardValue += std::to_string(Banker);
    for(auto &it : m_mUserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(pUserItem)
        {
            scoreInfo.clear();
            scoreInfo.chairId = userInfoItem->chairId;
            uint8_t t[1];
            if(m_wBankerUser != userInfoItem->chairId) //如果不是庄家
            {
                t[0] = 0xff & m_sAddScore[userInfoItem->chairId];
                scoreInfo.rWinScore = abs(m_RecordScoreInfo[userInfoItem->chairId].addScore+m_RecordScoreInfo[userInfoItem->chairId].revenue);
                rBankerWinscore += abs(m_RecordScoreInfo[userInfoItem->chairId].addScore+m_RecordScoreInfo[userInfoItem->chairId].revenue);
                scoreInfo.betScore = scoreInfo.rWinScore;
                scoreInfo.cardValue = cardValue ;//+ GlobalFunc::converToHex(t,1);

               // scoreInfo.betScore = m_sAddScore[userInfoItem->chairId]*m_dwCellScore;
            }
            else
            {
                scoreInfo.isBanker = 1;
                bankerScoreIdx = scoreInfoVec.size();
                t[0] = 1;
                scoreInfo.cardValue = cardValue;// + GlobalFunc::converToHex(t,1);
               // scoreInfo.betScore = m_dwCellScore; //三公庄家不用下注,暂定为底注
            }

            scoreInfo.addScore = m_RecordScoreInfo[userInfoItem->chairId].addScore;
            scoreInfo.revenue = m_RecordScoreInfo[userInfoItem->chairId].revenue;
            if(m_wBankerUser != userInfoItem->chairId)
                m_replay.addResult(scoreInfo.chairId,scoreInfo.chairId,scoreInfo.betScore,scoreInfo.addScore,to_string(m_GameLogic.GetHandCardsType(m_cbCardData[scoreInfo.chairId]))+":"+GlobalFunc::converToHex( m_cbHandCardData[scoreInfo.chairId] ,5)+":"+GlobalFunc::converToHex( m_cbCardData[scoreInfo.chairId] ,3),scoreInfo.isBanker==1);

            scoreInfo.startTime = m_startTime;
            scoreInfoVec.push_back(scoreInfo);
            LOG(ERROR)<<"QZXszZjh scoreInfo.addScore="<<scoreInfo.addScore<<"---------------------";
            LOG(ERROR)<<"QZXszZjh scoreInfo.revenue="<<scoreInfo.revenue<<"---------------------";
        }
    }
    scoreInfoVec[bankerScoreIdx].betScore = rBankerWinscore;
    scoreInfoVec[bankerScoreIdx].rWinScore = rBankerWinscore;
    int32_t bankerChair = scoreInfoVec[bankerScoreIdx].chairId;
    m_replay.addResult(bankerChair,bankerChair,rBankerWinscore,scoreInfoVec[bankerScoreIdx].addScore,
                       to_string(m_GameLogic.GetHandCardsType(m_cbCardData[bankerChair]))+":"+GlobalFunc::converToHex( m_cbHandCardData[bankerChair] ,5)+":"+GlobalFunc::converToHex( m_cbCardData[bankerChair] ,3),true);

    bool bWrite = m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
    m_pITableFrame->SaveReplay(m_replay);
    LOG(ERROR)<<"QZXszZjh write score bWrite = "<<(int)bWrite<<"---------------------";
}

void CTableFrameSink::GameTimerEnd()
{
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);

    m_pITableFrame->SetGameStatus(GAME_STATUS_END);//jie su

    LOG(INFO)<<"------------------------------------GameTimerEnd------------------------------------";

    m_dwEndTime = 0;
    bool isstop = m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    if(!isstop)
    {
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {              
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
            }
        }
        return;
    }

    LOG(INFO)<<"------------------------------------GameTimerEnd1------------------------------------";

    int AndroidNum = 0;
    int realnum = 0;
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            if(user->IsAndroidUser())
            {
                AndroidNum++;
            }
            realnum++;
        }
    }
    if(AndroidNum == realnum)
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
                user->SetUserStatus(sOffline);
        }
    }
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            if(user->GetUserStatus()==sOffline)
            {
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(INFO)<<"1Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }else if(user->GetUserScore()< m_pGameRoomInfo->enterMinScore)
            {
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(INFO)<<"2Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }
        }
    }

    //设置游戏状态
   // m_pITableFrame->SetGameStatus(GAME_STATUS_READY);
    m_cbStatus = XJH_GAME_STATUS_READY;
    // LOG(INFO)<<"------------------------------------SetGameStatus GAME_STATUS_READY------------------------------------";
    LOG(INFO)<<"-------设置 准备状态----------tableid="<<m_pITableFrame->GetTableId()<<",roomID="<<m_pGameRoomInfo->roomId;
    openSLog("设置 准备状态: tableid=%d,roomID=%d ;",m_pITableFrame->GetTableId(),m_pGameRoomInfo->roomId);
}

//游戏消息
bool CTableFrameSink::OnGameMessage( uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    if(wChairID < 0 || wChairID > 4)
    {
        return true;
    }
    int wRtn = 0;
    switch (wSubCmdID)
    {
        case XJH_SUB_C_CALL_BANKER:					//叫庄消息
        {
            LOG(INFO)<<"***************************USER CALL BANK1";
            //游戏状态不对
            if (XJH_GAME_STATUS_CALL != m_cbStatus)
            {
                LOG(INFO)<<"***************************GAME_STATUS_CALL != GameStatus";
                return false;
            }

            //不是游戏玩家
            if (1 != m_cbPlayStatus[wChairID])
            {
                LOG(INFO)<<"***************************1 != m_cbPlayStatus[wChairID]";
                return false;
            }


            //变量定义
            XJH_CMD_C_CallBanker  pCallBanker;
            pCallBanker.ParseFromArray(pData,wDataSize);

            LOG(INFO)<<"***************************1 pCallBanker"<<(int)pCallBanker.cbcallflag();
            openSLog("**************1 pCallBanker=%d;",(int)pCallBanker.cbcallflag());

            //玩家叫庄
            wRtn = OnUserCallBanker(wChairID, pCallBanker.cbcallflag());
            m_bPlayerIsOperation[wChairID] = true;
            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserCallBanker wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn > 0;
        }
        // case XJH_SUB_C_ADD_SCORE:					//下注消息
        // {
        //     //游戏状态不对            
        //     if (XJH_GAME_STATUS_SCORE != m_cbStatus)
        //     {
        //         return false;
        //     }

        //     //不是游戏玩家
        //     if (1 != m_cbPlayStatus[wChairID])
        //     {
        //         return false;
        //     }

        //     //变量定义
        //     XJH_CMD_C_AddScore  pAddScore;
        //     pAddScore.ParseFromArray(pData,wDataSize);

        //     //玩家下注
        //     wRtn = OnUserAddScore(wChairID, pAddScore.cbjetton());
        //     m_bPlayerIsOperation[wChairID]=true;
        //     //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserAddScore wChairID=" << wChairID << ",wRtn=" << wRtn;
        //     return wRtn > 0;
        // }
        case XJH_SUB_C_OPEN_CARD:					//开牌消息
        {
            m_bPlayerIsOperation[wChairID]=true;
            if (XJH_GAME_STATUS_OPEN != m_cbStatus)
            {
                //游戏状态不对
                return false;
            }

            if (1 != m_cbPlayStatus[wChairID])
            {
                //不是游戏玩家
                return false;
            }

            //变量定义
            XJH_CMD_C_OpenCard  pOpenCard;
            pOpenCard.ParseFromArray(pData,wDataSize);

            int cbOpenFlag = pOpenCard.cbopenflag();

            const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartdata = pOpenCard.cbchoosecard();
            int cSize = pOpenCard.cbchoosecard_size();
            uint8_t cbChooseCard[cSize];
            for(int i = 0; i < cSize; ++i)
            {
                cbChooseCard[i]=(uint8_t)cartdata[i];
            }
            if(cbOpenFlag==0)
            {
                for(int i = 0; i < MAX_COUNT; ++i)
                {
                    cbChooseCard[i]=m_cbCardData[wChairID][i];
                }
            }
            //玩家开牌
            wRtn = OnUserOpenCard(wChairID,cbOpenFlag,cbChooseCard);

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
   // m_pITableFrame->KillGameTimer(ID_TIME_READY);
   // m_pITableFrame->KillGameTimer(ID_TIME_CALL_BANKER);
   // m_pITableFrame->KillGameTimer(ID_TIME_ADD_SCORE);
   // m_pITableFrame->KillGameTimer(ID_TIME_OPEN_CARD);
   // m_pITableFrame->KillGameTimer(ID_TIME_GAME_END);

    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    // m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
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
    int dwShowTime = 0;
    if (m_cbStatus == XJH_GAME_STATUS_CALL)
    {
        //删除超时定时器
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        dwShowTime = 0; //发牌动画  1.5 未发牌

        m_TimerIdCallBanker=m_TimerLoopThread->getLoop()->runAfter(TIME_CALL_BANKER + dwShowTime, boost::bind(&CTableFrameSink::GameTimerCallBanker, this));
    }
    // else if (m_cbStatus == XJH_GAME_STATUS_SCORE)
    // {
    //     //删除超时定时器

    //     m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    //     m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    //     dwShowTime = 3.0; //动画  3.0
    //     m_TimerIdAddScore=m_TimerLoopThread->getLoop()->runAfter(TIME_ADD_SCORE + dwShowTime, boost::bind(&CTableFrameSink::GameTimerAddScore, this));
    // }
    else if (m_cbStatus == XJH_GAME_STATUS_OPEN)
    {
        //删除超时定时器
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        dwShowTime = 2.0; //开牌动画  2.0 -> 4.0
        m_TimerIdOpenCard=m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD + dwShowTime, boost::bind(&CTableFrameSink::GameTimerOpenCard, this));
    }
    else if (m_cbStatus == XJH_GAME_STATUS_END)
    {
        //删除超时定时器
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
        dwShowTime =1000;//结束动画  5.0
        m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
    }
    return true;
}

uint8_t CTableFrameSink::GetCardTypeStorgOne()
{
    int dwRandNum = rand() % 10000;
    if (dwRandNum < 1040)
    {
        return 0;//10.4%
    }
    else if (dwRandNum >= 1040 && dwRandNum < 1936)
    {
        return 1;//8.96%
    }
    else if (dwRandNum >= 1936 && dwRandNum < 2682)
    {
        return 2;//7.46%
    }
    else if (dwRandNum >= 2682 && dwRandNum < 3578)
    {
        return 3;//8.96%
    }
    else if (dwRandNum >= 3578 && dwRandNum < 3877)
    {
        return 4;//2.99%
    }
    else if (dwRandNum >= 3877 && dwRandNum < 4328)
    {
        return 5;//4.51%
    }
    else if (dwRandNum >= 4328 && dwRandNum < 5522)
    {
        return 6;//11.94%
    }
    else if (dwRandNum >= 5522 && dwRandNum < 5970)
    {
        return 7;//4.48%
    }
    else if (dwRandNum >= 5970 && dwRandNum < 7015)
    {
        return 8;//10.45%
    }
    else if (dwRandNum >= 7015 && dwRandNum < 8060)
    {
        return 9;//10.45%
    }
    else if (dwRandNum >= 8060 && dwRandNum < 10000)
    {
        return 10;//19.4%
    }

    return 10;
}

uint8_t CTableFrameSink::GetCardTypeStorgTwo()
{
    int dwRandNum = rand() % 10000;
    if (dwRandNum < 1659)
    {
        return 0;//16.59%
    }
    else if (dwRandNum >= 1659 && dwRandNum < 2300)
    {
        return 1;//6.41%
    }
    else if (dwRandNum >= 2300 && dwRandNum < 2947)
    {
        return 2;//6.47%
    }
    else if (dwRandNum >= 2947 && dwRandNum < 3688)
    {
        return 3;//7.41%
    }
    else if (dwRandNum >= 3688 && dwRandNum < 4435)
    {
        return 4;//7.47%
    }
    else if (dwRandNum >= 4435 && dwRandNum < 5291)
    {
        return 5;//8.56%
    }
    else if (dwRandNum >= 5291 && dwRandNum < 6138)
    {
        return 6;//8.47%
    }
    else if (dwRandNum >= 6138 && dwRandNum < 7079)
    {
        return 7;//9.41%
    }
    else if (dwRandNum >= 7079 && dwRandNum < 8026)
    {
        return 8;//9.47%
    }
    else if (dwRandNum >= 8026 && dwRandNum < 8967)
    {
        return 9;//9.41%
    }
    else if (dwRandNum >= 8967 && dwRandNum < 10000)
    {
        return 10;//10.33%
    }

    return 10;
}

string CTableFrameSink::GetCardValueName(int index)
{
    string strName;

    for(int i = 0 ; i < MAX_COUNT; ++i)
    {
        //获取花色
        uint8_t cbColor = m_GameLogic.GetCardColor(m_cbCardData[index][i]);
        uint8_t cbValue = m_GameLogic.GetCardValue(m_cbCardData[index][i]);
        switch(cbColor)
        {
            case 0:
                strName += "方";
                break;
            case 1:
                strName += "梅";
                break;
            case 2:
                strName += "红";
                break;
            case 3:
                strName += "黑";
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
    }
    return strName;
}

bool CTableFrameSink::RepayRecorduserinfo(int wChairID)
{
    shared_ptr<CServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(wChairID);
    if(!puserinfo)
        return true;
   // ::GameServer::Message::MSG_SC_UserBaseInfo userinfo;

   // ::Game::Common::Header* pheader = userinfo.mutable_header();
   // pheader->set_sign(1);
   // pheader->set_crc(1);
   // pheader->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
   // pheader->set_subid(::GameServer::Message::SUB_S2C_USER_ENTER);

   // userinfo.set_userid(puserinfo->GetUserID());
   // userinfo.set_nickname(puserinfo->GetNickName(),strlen(puserinfo->GetNickName()));
   // userinfo.set_clientip("");
   // userinfo.set_dlongitude(0);
   // userinfo.set_szlocation(puserinfo->GetLocation(),strlen(puserinfo->GetLocation()));
   // userinfo.set_cbheadindex(puserinfo->GetHeaderID());
   // userinfo.set_cbgender(puserinfo->GetGender());
   // userinfo.set_cbviplevel(puserinfo->GetVipLevel());
   // userinfo.set_cbviplevel2(0);
   // userinfo.set_dwgem(0);
   // userinfo.set_llscore(puserinfo->GetUserScore());
   // userinfo.set_tableid(puserinfo->GetTableID());
   // userinfo.set_chairid(puserinfo->GetChairID());
   // userinfo.set_usstatus(puserinfo->GetUserStatus());
   // userinfo.set_cbheadboxid(puserinfo->GetHeadBoxID());

   //  回放的数据现在没有使用 -delete by guansheng 20190316
   // std::string data = userinfo.SerializeAsString();
   // ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
   // pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
   // pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_USER_ENTER);
   // pcmddate->set_msgdata(data.data(),(dword)(data.size()));
   // pcmddate->set_userid(0);
   // pcmddate->set_chairid(6);
    return true;
}

bool CTableFrameSink::RepayRecorduserinfoStatu(int wChairID,int status)
{
    shared_ptr<CServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(wChairID);
    if(!puserinfo)
        return true;
#if 0
    ::GameServer::Message::MSG_SC_GameUserStatus userstatu;

    ::Game::Common::Header* puserstatu = userstatu.mutable_header();
    puserstatu->set_sign(1);
    puserstatu->set_crc(1);
    puserstatu->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
    puserstatu->set_subid(::GameServer::Message::SUB_S2C_USER_STATUS);


    userstatu.set_userid(puserinfo->GetUserID());
    userstatu.set_tableid(puserinfo->GetTableID());
    userstatu.set_chairid(puserinfo->GetChairID());
    userstatu.set_usstatus(status);

    std::string data = userstatu.SerializeAsString();
    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
    pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_USER_STATUS);
    pcmddate->set_msgdata(data.data(),(int64_t)(data.size()));
    pcmddate->set_userid(0);
    pcmddate->set_chairid(6);
#endif
    return true;
}

bool CTableFrameSink::RepayRecordStart()
{
    m_bReplayRecordStart = true;
   // m_UserReplayRecord.Clear();
#if 0
    ::GameServer::Message::MSG_SC_EnterRoomSucc EnterRoomSucc;
    ::Game::Common::Header* pEnterRoomSucc = EnterRoomSucc.mutable_header();
    pEnterRoomSucc->set_sign(1);
    pEnterRoomSucc->set_crc(1);
    pEnterRoomSucc->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
    pEnterRoomSucc->set_subid(::GameServer::Message::SUB_S2C_ENTER_ROOM_SUCC);

    EnterRoomSucc.set_gameid(m_pITableFrame->GetGameKindInfo()->nGameId);
    EnterRoomSucc.set_kindid(m_pITableFrame->GetGameKindInfo()->nKindId);
    EnterRoomSucc.set_roomkindid(m_pITableFrame->GetGameKindInfo()->nRoomKindId);
    EnterRoomSucc.set_cellscore(m_pITableFrame->GetGameKindInfo()->nCellScore);
    EnterRoomSucc.set_configid(m_pITableFrame->GetGameKindInfo()->nConfigId);

    std::string data = EnterRoomSucc.SerializeAsString();
    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
    pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_ENTER_ROOM_SUCC);
    pcmddate->set_msgdata(data.data(),(dword)(data.size()));
    pcmddate->set_userid(0);
    pcmddate->set_chairid(6);
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        RepayRecordGameScene(i,false);
        RepayRecorduserinfo(i);
    }
#endif
    return true;
}

bool CTableFrameSink::RepayRecordEnd()
{
    if (!m_bReplayRecordStart)
        return false;

    int usercount = m_pITableFrame->GetPlayerCount(false);
    if(usercount==0)
        return true;
#if 0
    //发送数据
    std::string data = m_UserReplayRecord.SerializeAsString();
    ISaveReplayRecord* pSaveReplayRecord = nullptr;
    m_pITableFrame->QueryInterface(guid_ISaveReplay,(void **)&pSaveReplayRecord);

    {
        tagGameRecPlayback  mGameRecPlayback;

        //mGameRecPlayback.configid = m_pITableFrame->GetGameKindInfo()->nConfigId;
        mGameRecPlayback.rec_roundid = g_nRoundID;
        shared_ptr<CServerUserItem> pBankuserItem =m_pITableFrame->GetTableUserItem(m_wBankerUser);
        if(!pBankuserItem)
        {
            return true;
        }
        string mbankAccount = pBankuserItem->getAccount();
        mGameRecPlayback.banker_userid = atoi(mbankAccount.c_str());
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (0 == m_cbPlayStatusRecord[i])
                continue;

            shared_ptr<CServerUserItem> pUserItem =m_pITableFrame->GetTableUserItem(i);
            if(!pUserItem)
            {
                return true;
            }
            string mAccount = pUserItem->getAccount();
            mGameRecPlayback.player[i].account = atoi(mAccount.c_str());
            mGameRecPlayback.player[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserID();
            mGameRecPlayback.player[i].changed_score = m_iUserWinScore[i];
        }
        mGameRecPlayback.content = data;

        pSaveReplayRecord->SaveReplayRecord(mGameRecPlayback);
    }

    m_UserReplayRecord.Clear();
    m_bReplayRecordStart = false;
    //delete replyRecordData;不能删除，已托管给SaveReplayRecord函数
#endif
    return true;
}

bool CTableFrameSink::RepayRecord(int wChairID,uint8_t wSubCmdId, const char* data, int64_t wSize)
{
    //    if(wSize<=0 || data==nullptr)
    //        return true;
#if 0
    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_GAME);
    pcmddate->set_wsubcmdid(wSubCmdId);
    pcmddate->set_msgdata(data, wSize);
    if(INVALID_CHAIR != wChairID)
    {
        shared_ptr<CServerUserItem> player= m_pITableFrame->GetTableUserItem(wChairID);
        if(!player)
        {
            return true;
        }
        pcmddate->set_userid(player->GetUserID());
        pcmddate->set_chairid(wChairID);
    }
    else
    {
        pcmddate->set_userid(0);
        pcmddate->set_chairid(6);
    }
#endif
    return true;
}

bool CTableFrameSink::GetUserMaxAddScore(int64_t wChairID, int64_t lUserJettonScore[5])
{
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_INIT || m_wBankerUser == INVALID_CHAIR) return false; // 空闲状态或者庄家还没有确定下来都不能确定闲家能够下的最大倍数
    if(!m_pITableFrame->GetTableUserItem(m_wBankerUser)) return false; // 庄家的信息获取不了
    //庄家不需要下注
    if(m_wBankerUser == wChairID) return false;
    shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(wChairID);
    if(user)
    {
        int64_t dwCellScore = m_dwCellScore/100;	//基础积分
        int64_t userScore = user->GetUserScore()/100; //因为数据是精确到分，需要对100取整
        int64_t userBankerScore = m_pITableFrame->GetTableUserItem(m_wBankerUser)->GetUserScore()/100;
        if(userScore/(MAX_USER_SASIC*dwCellScore) >= MAX_JETTON_MAX && userBankerScore/((m_wPlayCount-1)*MAX_USER_SASIC*dwCellScore) >= MAX_JETTON_MAX)
        {
            for(int i = 0; i < GAME_JETTON_COUNT; ++i)
            {
                lUserJettonScore[i] = MAX_JETTON_BASE*(i+1);
            }
        }
        else
        {
            int64_t lUserMaxAddScore = (userScore/MAX_USER_SASIC >= userBankerScore/((m_wPlayCount-1)*MAX_USER_SASIC) ? userBankerScore/((m_wPlayCount-1)*MAX_USER_SASIC) : userScore/MAX_USER_SASIC);
            lUserMaxAddScore = (lUserMaxAddScore/dwCellScore);
            if(lUserMaxAddScore > 5 && lUserMaxAddScore < 35) //5~35
            {
                for(int i = 0; i < GAME_JETTON_COUNT; ++i)
                {
                    lUserJettonScore[i] = (lUserMaxAddScore*(i+1)/5);
                }
            }
            else // 1~5
            {
                for(int64_t i = 0; i < lUserMaxAddScore; ++i)
                {
                    lUserJettonScore[i] = i+1;
                }
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

void CTableFrameSink::GetAllUserMaxAddScore()
{
    //游戏人数如果少于2人，就没有必要统计
    if(m_wPlayCount < 2) return;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;
        //庄家不下注
        if(m_wBankerUser == i) continue;
        GetUserMaxAddScore(i, m_cbMaxJetton[i]);
    }
}

void CTableFrameSink::ClearUserInfo()
{
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            user->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
        }
    }
}

int CTableFrameSink::ChangCardIndex(uint8_t cbCardData)
{
    int inde = (int)cbCardData % 16 + ((int)cbCardData / 16) * 100;

    return inde;
}

//读取配置
void CTableFrameSink::ReadStorageInformation()
{
    assert(m_pITableFrame);
    //系统当前库存
    m_pITableFrame->GetStorageScore(storageInfo_);

    CINIHelp ini;
    ini.Open(INI_FILENAME);
    //获取是否判断235牌型


    stockWeak=ini.GetFloat("Global", "STOCK_WEAK",1.0);

    std::string str235(ini.GetString("Global", "nHave235"));
    int nHave235 = atof(str235.c_str());
    if (nHave235!=0)
    {
        m_GameLogic.SetTypeSp235(true);
    }
    else
    {
        m_GameLogic.SetTypeSp235(false);
    }
    //是否写日记
    std::string strlog(ini.GetString("Global", "nWritelog"));
    int nWritelog = atof(strlog.c_str());
    if (nWritelog!=0)
    {
       m_bWritelog = true;
    }
    else
    {
        m_bWritelog = false;
    }
    //匹配规则
    {
        //分片匹配时长
        std::string str1(ini.GetString("MatchRule", "sliceMatchSeconds_"));
        sliceMatchSeconds_ = atof(str1.c_str());
        //超时匹配时长
        std::string str2(ini.GetString("MatchRule", "timeoutMatchSeconds_"));
        timeoutMatchSeconds_ = atof(str2.c_str());
        //机器人候补空位超时时长
        std::string str3(ini.GetString("MatchRule", "timeoutAddAndroidSeconds_"));
        timeoutAddAndroidSeconds_ = atof(str3.c_str());
        //IP访问限制
        std::string str4(ini.GetString("MatchRule", "checkIp_"));
        checkIp_ = atoi(str4.c_str()) > 0;
        //一张桌子真实玩家数量限制
        std::string str5(ini.GetString("MatchRule", "checkrealuser_"));
        checkrealuser_ = atoi(str5.c_str()) > 0;
    }
    //匹配概率
    {
        //放大倍数
        std::string str1(ini.GetString("MatchProbability", "ratioScale_"));
        ratioScale_ = atoi(str1.c_str());
        //5人局的概率
        std::string str2(ini.GetString("MatchProbability", "ratio5_"));
        ratio5_ = atoi(str2.c_str());
        //4人局的概率
        std::string str3(ini.GetString("MatchProbability", "ratio4_"));
        ratio4_ = atoi(str3.c_str());
        //3人局的概率
        std::string str4(ini.GetString("MatchProbability", "ratio3_"));
        ratio3_ = atoi(str4.c_str());
        //2人局的概率
        std::string str5(ini.GetString("MatchProbability", "ratio2_"));
        ratio2_ = atoi(str5.c_str());
        //单人局的概率
        std::string str6(ini.GetString("MatchProbability", "ratio1_"));
        ratio1_ = atoi(str6.c_str());
    }
    //系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
    {
        std::string str1(ini.GetString("Probability", "ratio_"));
        ratioSwap_ = atoi(str1.c_str());
    }
    //系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
    {
        std::string str1(ini.GetString("Probability", "ratioLost_"));
        ratioSwapLost_ = atoi(str1.c_str());
    }
    //系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
    {
        std::string str1(ini.GetString("Probability", "ratioWin_"));
        ratioSwapWin_ = atoi(str1.c_str());
    }
    //发牌时尽量都发好牌的概率，玩起来更有意思
    {
        std::string str1(ini.GetString("Probability", "ratioDealGood_"));
        ratioDealGood_ = atoi(str1.c_str());
    }
    {
        std::string str1(ini.GetString("path", "cardList"));
        INI_CARDLIST_ = str1;
    }


}
//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userScore[], int64_t userWinScorePure[], int64_t calcMulti[])
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
		uint8_t handCards[MAX_SENDCOUNT] = { 0 };
		for (int id = 0; id < MAX_SENDCOUNT; id++)
		{
			if (id < MAX_COUNT)
			{
				handCards[count++] = m_cbCardData[i][id];
			}
			else
			{
				bool bSameChoose = false;
				for (int j = 0; j < MAX_SENDCOUNT; j++)
				{
					bSameChoose = false;
					for (int k = 0; k < MAX_SENDCOUNT; k++)
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
						if (count >= MAX_SENDCOUNT) break;
					}
				}
				if (count >= MAX_SENDCOUNT) break;
			}
		}
		pcards->set_cards(&(handCards)[0], MAX_SENDCOUNT);
		int nType = (int)m_GameLogic.GetHandCardsType(m_cbCardData[i]);
		pcards->set_ty(nType);
		//牌型倍数
		pcards->set_multi(m_iMultiple[i]);
		//携带积分
		detail->set_userscore(userScore[i]);
		//房间底注
		detail->set_cellscore(m_dwCellScore);
		//下注/抢庄倍数
		//detail->set_multi((bankerUser_ == i) ? callBankerFlag_[i] : multiJettonable_[i][addScoreIndex_[i]]);
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
