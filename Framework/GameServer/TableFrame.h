#pragma once

#include <atomic>

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/ThreadLocalSingletonRedisClient.h"
#include "ConnectionPool.h"
#include "public/ISaveReplayRecord.h"


using namespace muduo;
using namespace muduo::net;
using namespace boost::posix_time;
using namespace boost::gregorian;


class ITableFrameSink;
class CServerUserItem;


class CTableFrame : public ITableFrame, public ISaveReplayRecord
{
public:
    CTableFrame();
    virtual ~CTableFrame();

//    void log_userlist(const char* fmt,...);
//    void log_writescore(const char* fmt,...);

public:
    virtual void Init(shared_ptr<ITableFrameSink>& pSink, TableState& tableState,
                      tagGameInfo* pGameInfo, tagGameRoomInfo* pGameRoomInfo,
                      shared_ptr<EventLoopThread>& game_logic_thread, shared_ptr<EventLoopThread>& game_dbmysql_thread,ILogicThread*);

    virtual string GetNewRoundId();
protected:
    bool sendData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId, vector<uint8_t> &data, Header *commandHeader, int enctype = PUBENC__PROTOBUF_NONE);

public:
    virtual bool SendTableData(uint32_t chairId, uint8_t subId, const uint8_t* data, int size, bool bRecord = true);
    virtual bool SendUserData(shared_ptr<CServerUserItem>& pServerUserItem, uint8_t subId, const uint8_t* data, int size, bool bRecord = true);
    virtual bool SendGameMessage(uint32_t chairId, const std::string& szMessage, uint8_t msgType, int64_t score = 0);
//    virtual bool SendLookOnData(uint32_t chairId, uint8_t subId, const uint8_t* data, int size, bool bRecord = true);

public: //table info
    virtual void GetTableInfo(TableState& TableInfo);
    virtual uint32_t GetTableId();

    virtual shared_ptr<EventLoopThread> GetLoopThread();

    virtual bool DismissGame();
    virtual bool ConcludeGame(uint8_t nGameStatus);
    virtual int64_t CalculateRevenue(int64_t nScore);

public://user.
    virtual shared_ptr<CServerUserItem> GetTableUserItem(uint32_t chairId);
    virtual shared_ptr<CServerUserItem> GetUserItemByUserId(int64_t userId);

    virtual bool IsExistUser(uint32_t chairId);
    virtual bool CloseUserConnect(uint32_t chairId);

public:// game status.
    virtual void SetGameStatus(uint8_t cbStatus = GAME_STATUS_FREE);
    virtual uint8_t GetGameStatus();

public://Trustee
    virtual void SetUserTrustee(uint32_t chairId, bool bTrustee);
    virtual bool GetUserTrustee(uint32_t chairId);

    virtual void SetUserReady(uint32_t chairId);
    virtual bool OnUserLeft(shared_ptr<CServerUserItem>& pUser, bool bSendStateMyself=true, bool bForceLeave=false);
    virtual bool OnUserOffline(shared_ptr<CServerUserItem>& pUser,bool bLeaveGs=false);

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);

public:
    virtual uint32_t GetPlayerCount(bool bIncludeAndroid=true);
    virtual uint32_t GetAndroidPlayerCount();
    virtual uint32_t GetMaxPlayerCount();
    virtual void GetPlayerCount(uint32_t &nRealPlayerCount, uint32_t &nAndroidPlayerCount);

public:
    virtual tagGameRoomInfo* GetGameRoomInfo();

public:

public://table.
    virtual void ClearTableUser(uint32_t chairId = INVALID_CHAIR, bool bSendState = true, bool bSendToSelf = true, uint8_t cbSendErrorCode = 0);
    virtual bool IsAndroidUser(uint32_t chairId);

public://event.
    virtual bool OnGameEvent(uint32_t chairId, uint8_t subId, const uint8_t *data, int size);
    virtual void onStartGame();

    virtual bool IsGameStarted()                { return m_cbGameStatus >= GAME_STATUS_START && m_cbGameStatus < GAME_STATUS_END; }
//    virtual bool CheckGameStart();

public:
    virtual bool OnUserEnterAction(shared_ptr<CServerUserItem>& pServerUserItem, Header *commandHeader, bool bDistribute = false);
    virtual bool OnUserStandup(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendState = true, bool bSendToSelf = false);

public:// broadcast user status.
    void SendUserSitdownFinish(shared_ptr<CServerUserItem>& pIServerUserItem, Header *commandHeader, bool bDistribute=false);
    void BroadcastUserScore(shared_ptr<CServerUserItem>& pServerUserItem);

    virtual void BroadcastUserInfoToOther(shared_ptr<CServerUserItem>& pServerUserItem);
    virtual void SendAllOtherUserInfoToUser(shared_ptr<CServerUserItem>& pServerUserItem);
    virtual void SendOtherUserInfoToUser(shared_ptr<CServerUserItem>& pServerUserItem, tagUserInfo &userInfo);
    virtual void BroadcastUserStatus(shared_ptr<CServerUserItem>& pUserItem, bool bSendTySelf = true);

public:
    bool WriteBlacklistLog(shared_ptr<CServerUserItem>& pIServerUserItem, int status);
    virtual bool WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, string &strRound);
    virtual bool WriteSpecialUserScore(tagSpecialScoreInfo* pScoreInfo, uint32_t nCount, string &strRound);

    //记录水果机免费游戏剩余次数
    virtual bool WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord);
    virtual bool GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId);

public:// storatStorage.
    virtual int  UpdateStorageScore(int64_t changeStockScore);
    virtual bool GetStorageScore(tagStorageInfo &storageInfo);
    bool  WriteGameChangeStorage(int64_t changeStockScore);
    bool ReadStorageScore();
    void BlackListControl(shared_ptr<CServerUserItem> & pIServerUserItem , GSUserBaseInfo & userBaseInfo , tagScoreInfo *scoreInfo);
    void PersonalProfit( shared_ptr<CServerUserItem> & pIServerUserItem );

    void WriteDBThread(GSUserWriteMysqlInfo MysqlInfo);

    void InsertUserWeekBetting(GSUserWriteMysqlInfo MysqlInfo);
    //void InsertUserWeekBetting(int64_t betting,int64_t userid,int32_t agentid,int64_t profit);
private:
//    bool OnUserStandupAndLeft(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendState);

public: //finished.
    void DeleteUserToProxy(shared_ptr<CServerUserItem>& pIServerUserItem, int32_t nKickType = KICK_GS|KICK_CLOSEONLY);

    bool DelUserOnlineInfo(int64_t userId,bool bonlyExpired=false);
    bool SetUserOnlineInfo(int64_t userId,int32_t tableId);

    bool UpdateUserScoreToDB(int64_t userId, tagScoreInfo* pScoreInfo);
    bool UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo);

    bool AddUserGameInfoToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);
    bool AddUserGameInfoToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);

    bool AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore);
    bool AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo);

    bool AddUserGameLogToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId);
    bool AddUserGameLogToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId);

    inline string InitialConversion(int64_t timeSeconds)
    {
        time_t rawtime = (time_t)timeSeconds;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);

        int Year = timeinfo->tm_year + 1900;
        int Mon = timeinfo->tm_mon + 1;
        int Day = timeinfo->tm_mday;
        int Hour = timeinfo->tm_hour;
        int Minuts = timeinfo->tm_min;
        int Seconds = timeinfo->tm_sec;
        string strYear;
        string strMon;
        string strDay;
        string strHour;
        string strMinuts;
        string strSeconds;

        if (Seconds < 10)
            strSeconds = "0" + to_string(Seconds);
        else
            strSeconds = to_string(Seconds);
        if (Minuts < 10)
            strMinuts = "0" + to_string(Minuts);
        else
            strMinuts = to_string(Minuts);
        if (Hour < 10)
            strHour = "0" + to_string(Hour);
        else
            strHour = to_string(Hour);
        if (Day < 10)
            strDay = "0" + to_string(Day);
        else
            strDay = to_string(Day);
        if (Mon < 10)
            strMon = "0" + to_string(Mon);
        else
            strMon = to_string(Mon);

        return to_string(Year) + "-" + strMon + "-" + strDay + " " + strHour + ":" + strMinuts + ":" + strSeconds;
    }
    /////////////////////////write sqlserver////////////////////////
    /// \brief WtiteUserInfoToMysql
    /// \param GSUserWriteMysqlInfo
    /// \return
    //////不要传引用，也不要传指针，直接传值，这样跨线程保证数据拷贝是安全的,指针和引用万一在原线程被销毁会出问题
    void WtiteUserPlayRecordMysql(GSUserWriteMysqlInfo MysqlInfo);
    void WtiteUserPlayRecordMysqlx(string mysqlStr);
    void WtiteUserGameLogMysql(GSUserWriteMysqlInfo MysqlInfo);
    void WtiteUserScoreTrangeMysql(GSUserWriteMysqlInfo MysqlInfo);
    void WtiteUserScoreTrangeMysqlx(string mysqlStr);
    void WtiteUserPersonalRedis(GSUserWriteMysqlInfo MysqlInfo);
    void WtiteUserBasicInfoRedis(GSUserWriteMysqlInfo MysqlInfo);
    void WtiteUserUserscoreRedisMysql(GSUserWriteMysqlInfo MysqlInfo);
    void ConversionFormat(GSUserWriteMysqlInfo &userMysqlInfo,GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);
    void ConversionFormat(GSUserWriteMysqlInfo &userMysqlInfo,tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);
    void WriteRecordMongodb(GSUserWriteMysqlInfo MysqlInfo);
    void WriteChangeToMongodb(GSUserWriteMysqlInfo MysqlInfo);
    ////////////////////////////////////////////////////////////////

    bool WritePersonalRedis(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo);
    bool WritePersonalRedis(GSUserBaseInfo &userBaseInfo, tagSpecialScoreInfo *scoreInfo);


    void UpdateUserRedisBasicInfo(int64_t userId, tagScoreInfo* pScoreInfo);
    void UpdateUserRedisBasicInfo(int64_t userId, tagSpecialScoreInfo* pScoreInfo);
public:
    bool RoomSitChair(shared_ptr<CServerUserItem>& pServerUserItem, Header *commandHeader, bool bDistribute = false);
//    bool TableUserLeft(shared_ptr<CServerUserItem> &pUserItem, bool bSendToOther);


public://function.
    bool IsLetAndroidOffline(shared_ptr<CServerUserItem>& pIServerUserItem);
    //when androidCount>MaxAndroidCount remove one android a tick
    void LeftExcessAndroid();
protected: // inner send packed unpackdata.
    // send raw data : the package has been include the header data value now content item data value for later value.
    bool SendUnpackData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId, const uint8_t* data, int size);
    bool SendPackedData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId, const uint8_t* data, int size, Header *commandHeader);

protected: // bisOut = false is in.
    bool DBRecordUserInout(shared_ptr<CServerUserItem>& pIServerUserItem, bool bisOut = false, int nReason = 0);

protected:  // query the special interface item value content item now.
    virtual int QueryInterface(const char* guid, void** ptr, int ver);

protected: // try to save the special replay record.
    bool SaveReplayRecord(tagGameRecPlayback& rec);
    // dump user list.
    void dumpUserList();

public:
    //save on gameend
    bool SaveReplay(tagGameReplay replay);
private:
    bool SaveReplayDetailJson(tagGameReplay replay);
    bool SaveReplayDetailBlob(tagGameReplay replay);
public:
    // 公共的
    int32_t CommonFunc(eCommFuncId funcId, std::string &jsonInStr ,std::string &jsonOutStr );

    int64_t ReadJackpot( int32_t JackpotId );
    bool UpdateJackpot(int32_t optype,int64_t incScore,int32_t JackpotId,int64_t *newJackPot);
    bool SaveJackpotRecordMsg(string &msgStr,int32_t msgCount);

    bool SetBlackeListInfo(string insert,map<string,int16_t> &usermap);
	//刷新个人累计输赢库存控制参数
    void RefreshKcControlParameter(shared_ptr<CServerUserItem> pIServerUserItem,int64_t addScore);
    
    bool SaveKillFishBossRecord(GSKillFishBossInfo& bossinfo);
    
    //获取当前的控制等级
    int GetCurrentControlLv(int curTime);
    //设置个人库存控制等级和 玩家不玩的话控制解除时间
    virtual void setkcBaseTimes(int kcBaseTimes[],double timeout,double reduction);
protected:  // the special protect data value item content.
    shared_ptr<ITableFrameSink>        m_pTableFrameSink;

    tagGameInfo*            m_pGameInfo;
    tagGameRoomInfo*        m_pGameRoomInfo;
    ILogicThread*           m_pLogicThread;
    TableState              m_TableState;

    shared_ptr<EventLoopThread> m_game_logic_thread;

    shared_ptr<EventLoopThread> m_game_dbsql_thread;


public:
    vector<shared_ptr<CServerUserItem>> m_UserList;

    static  vector<string>  ListDBMysql;

protected:
    uint8_t                 m_cbGameStatus; 

private:
    static atomic_llong            m_CurStock;
    static double                  m_LowestStock;
    static double                  m_HighStock;

    static int64_t                 m_TotalJackPot[5];

    static int                     m_SystemAllKillRatio;
    static int                     m_SystemReduceRatio;
    static int                     m_ChangeCardRatio;
	//个人库存控制的等级倍数
    int                            m_kcBaseTimes[6];
    //控制过期时间
    double                         m_kcTimeoutHour;
    //玩家累计为负时的衰减率(百分比)
    double                         m_kRatiocReduction;
};



//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include "muduo/base/Timestamp.h"

//#include <kernel/CriticalSection.h>

//#define GAME_STATUS_FREE    (0)
//#define GAME_STATUS_PLAY    (100)


//    virtual int64_t GetUserEnterMinScore();
//    virtual int64_t GetUserEnterMaxScore();
//    virtual int64_t GetGameCellScore();
//    virtual bool AddUserScore(uint32_t chairId, int64_t addScore, bool bBroadcast = true);
//    virtual bool AddUserScore(shared_ptr<CServerUserItem>& pIServerUserItem, int64_t addScore, int64_t revenue);


//    bool WriteScoreChangeRecordLogToDB(string &curMonth, dword userId, dword nRound, tagScoreInfo& scoreInfo, shared_ptr<CServerUserItem>& pServerUserItem, SCORE &sourceScore, SCORE &newScore);
//    bool WriteRoundScoreChangeRecordLogToDB(string &curMonth, dword nRound, SCORE &waste_system, SCORE &waste_all, SCORE &waste_android, SCORE &revenue, SCORE &agent_revenue);

//    bool RedisLoginInfoScoreBankScoreChange(int32_t userId, SCORE nAddscore, SCORE nAddBankScore, SCORE &newScore, SCORE &newBankScore);
//    bool InsertScoreChangeRecord(dword userid, SCORE lUserScore, SCORE lBankScore,
//                                             SCORE lAddScore, SCORE lAddBankScore,
//                                             SCORE lTargetUserScore, SCORE lTargetBankScore,
//                                             int type);



