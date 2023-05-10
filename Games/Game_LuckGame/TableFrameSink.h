#ifndef SGJ_GAMEPROCESS_H
#define SGJ_GAMEPROCESS_H

#include <chrono>

#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>

#include "public/ITableFrameSink.h"
#include "public/ITableFrame.h"
#include "GameLogic.h"
#include "proto/LuckGame.Message.pb.h"
#include "CMD_Game.h"
#include "ConfigManager.h"

using namespace LuckGame;

#define USER_HISTORY_COUNT 20 //玩家游戏记录
#define HISTORY_COUNT 70      //历史记录

class CTableFrameSink : public ITableFrameSink
{
public:
    struct tagAreaWinScore
    {
        uint8_t cbAreaIndex;
        int64_t llWinScore;
    };

    struct tagUserHistory
    {
        int32_t wChairID;                          //椅子ID
        int32_t wPlayCount;                        //玩的局数
        uint8_t cbWinCount[USER_HISTORY_COUNT];    //赢的局数
        int64_t llJettonScore[USER_HISTORY_COUNT]; //加注总额

        uint8_t GetWinCount()
        {
            uint8_t cb = 0;
            for (uint8_t i = 0; i < USER_HISTORY_COUNT; ++i)
            {
                cb += cbWinCount[i];
            }
            return cb;
        }

        int64_t GetJettonScore()
        {
            int64_t ll = 0;
            for (uint8_t i = 0; i < USER_HISTORY_COUNT; ++i)
            {
                ll += llJettonScore[i];
            }
            return ll;
        }

        string ToString()
        {
            stringstream s;
            for (int i = 0; i < USER_HISTORY_COUNT; i++)
            {
                s << (int)cbWinCount[i] << " " << (int)llJettonScore[i] << ",";
            }
            return s.str();
        }
    };

    struct tagUserHistoryEx
    {
        int32_t wChairID;      //椅子ID
        uint8_t cbWinCount;    //赢的局数
        int64_t dwJettonScore; //加注总额
    };

    struct tagUserFollowBet
    {
        uint8_t cbFollowSitID;     //跟投的座椅ID
        uint8_t cbJettonIndex;     //跟注索引
        uint8_t cbTotalCount;      //跟投总次数
        uint8_t cbCurrCount;       //当前跟投次数
        uint8_t cbFollowThisRound; //这局已经跟投
    };

public:
    CTableFrameSink();
    virtual ~CTableFrameSink();

public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon);
    //用户进入
    virtual bool OnUserEnter(int64_t userId, bool islookon);
    //用户起立
    virtual bool OnUserLeft(int64_t userId, bool islookon);
    //用户同意
    virtual bool OnUserReady(int64_t userId, bool islookon);

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);

    //游戏消息处理
    virtual bool OnGameMessage(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize);

    virtual bool SetTableFrame(shared_ptr<ITableFrame> &pTableFrame);
    virtual bool SendErrorCode(int32_t wChairID, enErrorCode enCodeNum);

    int GetResult(const int32_t wChairID, int64_t lbetScore, int32_t nTypeIndex); 
    bool WriteScore(const int32_t wChairID, const int32_t lBetScore, const int64_t lWinScore, int64_t &llGameTax,string cardValuesStr);

    //复位桌子
    virtual void RepositionSink();
 
    void LeftReward(int64_t userId,uint32_t wChairID);
    void checkKickOut(); 
private:
    //初始化游戏数据
    void InitGameData();
    void ClearGameData(int chairId );
    bool CheckStorageVal(int64_t winScore);

    void openLog(const char *p, ...);
private:
    //游戏信息
    uint8_t m_wGameStatus;               //游戏状态
    uint8_t m_cbPlayStatus[GAME_PLAYER]; //游戏中玩家,下注了的玩家
    //历史记录
    uint8_t m_cbHistory[HISTORY_COUNT]; //历史记录(1 表示黑赢 2 表示红赢)
    uint8_t m_cbHistoryCount;           //历史记录局数
    vector<int> m_cbJettonMultiple;     //下注倍数
    int32_t m_dwCellScore;              //基础积分

private:
    //玩家信息
    int64_t m_llUserSourceScore[GAME_PLAYER];   //用户分数
    int64_t m_llUserJfScore[GAME_PLAYER];       //用户积分

    //玩家下注记录
    tagUserHistory m_UserHistory[GAME_PLAYER];      //玩家历史记录
    std::vector<tagUserHistoryEx> m_vecUserHistory; //历史下注排行榜

    int32_t m_FreeRound;
    int32_t m_MarryRound;
    int32_t m_Round;

    time_t m_tLastOperate[GAME_PLAYER];    //上次操作时间
    int64_t m_lUserBetScore[GAME_PLAYER];      //玩家下注金额
    int64_t m_lUserWinScore[GAME_PLAYER];      //玩家旋转场景赢分

    int64_t m_lFreeWinScore; //免费旋转中的赢分
    int64_t m_lMarryWinScore; //免费旋转中的赢分
    //牌型
    string cardValueStr;
    string iconResStr;
    string tmpWinLinesInfo ; 
    //理牌开始时间
	uint32_t groupStartTime;
    uint32_t m_nPullInterval;

    int32_t m_TimerTestCount;

    int64_t m_lUserScore;      //玩家旋转场景赢分

    int32_t m_typeIdx;
    int32_t m_betIdx;
    int32_t m_betListCount;

    tagAllConfig m_config;
protected:
    string mGameIds;
	//本局开始时间/本局结束时间
    chrono::system_clock::time_point m_startTime; //
    chrono::system_clock::time_point m_endTime; 
    
    //用户指针
    shared_ptr<CServerUserItem> pUserItem;

private:
    tagGameRoomInfo *m_pGameRoomInfo;
    tagGameReplay m_replay; //game replay
    uint32_t m_dwReadyTime;
    //记录
    tagSgjFreeGameRecord m_SgjFreeGameRecord;
private:
    GameLogic m_GameLogic;                  //游戏逻辑
    shared_ptr<ITableFrame> m_pITableFrame; //框架接口
    shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;

    muduo::net::TimerId                         m_TimerIdTest;
    muduo::net::TimerId                         m_TimerIdKickOut;
private:                                  //Storage Related
    static int64_t m_llStorageControl;    //库存控制
    static int64_t m_llStorageLowerLimit; //库存控制下限值
    static int64_t m_llStorageHighLimit;  //库存控制上限值
    static int64_t m_llTodayStorage;      //今日库存
    static int64_t m_llStorageAverLine;   //average line
    static int64_t m_llGap;               //refer gap:for couting rate
    int32_t m_wSystemAllKillRatio;        //系统必杀概率
    int32_t m_wChangeCardRatio;           //系统改牌概率
    int32_t m_dwTodayTime;                //今日时间
    //    CMD_S_GameEnd				m_GameEnd;
    int32_t m_MinEnterScore;                     //税收
    int32_t m_kickTime;                     //踢出计时
    int32_t m_kickOutTime;                  //踢出时间
    int64_t m_userId;                   //用户Id
    
    bool m_bUserIsLeft;                   //用户是否离开

    vector<int> m_vBetItemList;
};

#endif // GAMEPROCESS_H
