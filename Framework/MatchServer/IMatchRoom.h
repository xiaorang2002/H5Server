#ifndef IMATCHROOM_H
#define IMATCHROOM_H

#include "GameTableManager.h"
#include "GameServer/ServerUserItem.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/net/TcpServer.h"
#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/BlockingQueue.h"
#include "proto/MatchServer.Message.pb.h"
#include "RedisClient/RedisClient.h"
#include "public/TaskService.h"

class CTableFrame;
enum roomState
{
    MATCH_WAIT_START=0,
    MATCH_GAMING,
    MATCH_END
} ;
/*
    现在基本不算有积分系统，给一个初始积分就开始游戏，到最后根据积分排名，这个设计感觉不合理，等策划最后确认吧
*/
struct MatchRoomInfo
{
    uint16_t    roomCount;             //
    uint32_t    initCoins;              //初始积分
    uint32_t    maxPlayerNum;           //比赛固定人数
    uint32_t    roomAndroidMaxCount;   //比赛最多机器人数
    int64_t    joinNeedSore;          //报名费
    uint8_t     bEnableAndroid;       // is enable android.
    uint8_t     fixedAllocate;        //固定匹配匹配
    uint8_t     fixedPerGamePlayerNum; //每个游戏限定的固定人数
    vector<int64_t> awards;            //名次奖励
    vector<int64_t> upgradeGameNum;   //每轮局数
    vector<int64_t> upGradeUserNum;  //每轮晋级人数
    string     servicename;         //服务名字
    string     matchName;            //比赛名字
    uint16_t    matchRoomId;        //比赛房ID 其实就是房间roomid
    uint16_t    gameId;
    int32_t     atLeastNeedScore;   //下一局玩家至少要多少初始分数 预留字段
    int32_t     leftWaitSeconds;    //退出比赛最少需要多久才能重新报名
    int32_t     androidEnterMinGap; //机器人最小进入间隔
    int32_t     androidEnterMaxGap; //机器人最大进入间隔
    bool        bCanJoinMatch;      //比赛是否允许进入（是否在开放时间）
};

struct UserInfo
{
    uint32_t curGameCount ;  //当前游戏了的局数
    shared_ptr<CServerUserItem> ptrUserItem;
    int64_t realScore;
    bool waitStart;
};

struct ClassComp
{
  bool operator() (const shared_ptr<UserInfo> lhs, const shared_ptr<UserInfo> rhs) const
  {
      return lhs->ptrUserItem->GetUserScore() >= rhs->ptrUserItem->GetUserScore();
  }
};


/*
    比赛类的基础类（父类）
*/
class IMatchRoom
{

public:
    IMatchRoom() = default;
    ~IMatchRoom() = default;
	virtual MatchRoomInfo* GetMatchRoomInfo() { return p_MatchRoomInfo_; }
    virtual bool InitMathRoom(shared_ptr<EventLoopThread>& game_logic_thread,MatchRoomInfo* pMatchRoomInfo,ILogicThread* pLogicThread);
    virtual bool resetMatch();

    virtual bool CanJoinMatch(shared_ptr<CServerUserItem> &pIServerUserItem)=0;
    virtual bool CheckJionMatch(shared_ptr<CServerUserItem> &pIServerUserItem)=0;
    virtual bool CanLeftMatch()=0;

    virtual void TableFinish(uint32_t tableID) = 0;
    virtual void UserOutTable(uint32_t userId) = 0;
    //获取库存状态
    virtual int8_t GetStorageStatus()=0;

    virtual bool OnUserOffline(shared_ptr<CServerUserItem>& pIServerUserItem);
    virtual bool OnUserOnline(shared_ptr<CServerUserItem>& pIServerUserItem);
    virtual bool JoinMatch(shared_ptr<CServerUserItem>& pIServerUserItem,Header *commandHeader=NULL)=0;
    virtual bool LeftMatch(uint32_t userId);

    virtual void SortUser();
    virtual void GetRanks(MatchServer::MSG_S2C_UserRankResp& ranks);

    uint16_t GetMatchId(){return matchId_;}
    void SetWaitToStart(){match_status_=MATCH_WAIT_START;}
    roomState GetMatchStatus(){return match_status_;}
    uint16_t GetPlayerCount(bool isOnlyReal=false);
    uint16_t GetAllPlyerCount(){return list_MatchUser_.size();}
    string getMatchRoundId(){return strRoundId_;}                          //每个比赛的编号

protected:
    virtual void clearAllUser();
    virtual shared_ptr<CTableFrame> getSuitTable(shared_ptr<CServerUserItem>& pIServerUserItem);

protected:
    void EnterRoomFinish(shared_ptr<CServerUserItem>& pIServerUserItem);
    void SendMatchScene(shared_ptr<CServerUserItem>& pIServerUserItem);

    void initStorageScore();//初始化库存
    void updateStorageScore(int32_t score); //更新库存
    bool AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore);
    bool AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo);
    void writeUserScoreToDB(int32_t userId,int64_t score);
    void updateMatchRecord(GSUserBaseInfo& UserBase,int64_t score,int64_t beforechange,int64_t afterchange,int32_t rank,bool isAndroid);
    void updateBestRecord(shared_ptr<CServerUserItem>& pIServerUserItem,int32_t score,int32_t rank);

    void OnUserEnterAction(shared_ptr<UserInfo>& pUserInfo,Header *commandHeader);
    void updateMatchPlayer(shared_ptr<CServerUserItem>& pIServerUserItem);
    void updateRankInfo();
    void OnUserFinish(shared_ptr<UserInfo> &pUserInfo,int64_t awardScore,uint32_t rank);
    void OnUserUpgrade(shared_ptr<CServerUserItem>& pIServerUserItem,uint16_t rank);

    bool SendPackedData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                                     const uint8_t* data, int size, Header *commandHeader=NULL);

    bool sendData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
             vector<uint8_t> &data, Header *commandHeader, int enctype);

	bool SendGameMessage(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t msgType, int64_t awardScore, uint32_t rank);
protected:
//    multiset<shared_ptr<UserInfo>,ClassComp>  set_MatchUser_;
    //比赛父类变量全部用 ‘ _ ’ 结尾
    list<shared_ptr<UserInfo>> list_MatchUser_;  //比赛玩家列表
    vector<shared_ptr<UserInfo>> vec_WaitUser_;  //等待游戏列表
    ILogicThread* p_ILogicThread_;              //MatchServer父类指针
    shared_ptr<EventLoopThread> ptr_LoopThread_;  //事件线程（所有都用这个线程）
    MatchRoomInfo* p_MatchRoomInfo_;            //比赛房间配置
    uint16_t matchId_;                          //比赛id，类似每个进程的桌子编号
    roomState match_status_;                    //比赛状态
    uint8_t cur_round_count_;                   //当前比赛进行了几轮
    list<shared_ptr<CTableFrame>> list_UsedTable_; //比赛使用了的桌子
    string strRoundId_;                         //每个比赛的编号
    int32_t winlost_Score_;                     //每场比赛的系统输赢

    double  curStock_;                          //当前库存
    static double  lowestStock_;                //库存底
    static double  highStock_;                  //库存顶 
};

#endif // IMATCHROOM_H
