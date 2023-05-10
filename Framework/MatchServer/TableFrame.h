#pragma once

#include <atomic>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "RedisClient/RedisClient.h"
#include "public/ISaveReplayRecord.h"
#include "IMatchRoom.h"
#include "ConnectionPool.h"

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
                      shared_ptr<EventLoopThread>& game_logic_thread, shared_ptr<EventLoopThread>& gameDBThread,ILogicThread*);

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
    virtual bool WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, string &strRound);
    virtual bool WriteSpecialUserScore(tagSpecialScoreInfo* pScoreInfo, uint32_t nCount, string &strRound);

public:// storatStorage.
    virtual int  UpdateStorageScore(int64_t changeStockScore);
    virtual bool GetStorageScore(tagStorageInfo &storageInfo);

    virtual void setkcBaseTimes(int kcBaseTimes[],double timeout,double reduction);


    virtual bool SaveKillFishBossRecord(GSKillFishBossInfo& bossinfo) ;

    bool  WriteGameChangeStorage(int64_t changeStockScore);
    bool ReadStorageScore();

    virtual bool WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord){return true;}
    virtual bool GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId){return true;}
private:
//    bool OnUserStandupAndLeft(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendState);

public: //finished.
    void DeleteUserToProxy(shared_ptr<CServerUserItem>& pIServerUserItem, int32_t nKickType = KICK_GS|KICK_CLOSEONLY);

    bool DelUserOnlineInfo(int64_t userId,bool bonlyExpired=false);
    bool SetUserOnlineInfo(int64_t userId);

    bool UpdateUserScoreToDB(int64_t userId, tagScoreInfo* pScoreInfo);
    bool UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo);

    bool AddUserGameInfoToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);
    bool AddUserGameInfoToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser = false);

    bool AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore);
    bool AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo);

    bool AddUserGameLogToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId);
    bool AddUserGameLogToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId);

public:
    bool RoomSitChair(shared_ptr<CServerUserItem>& pServerUserItem, Header *commandHeader, bool bDistribute = false);
//    bool TableUserLeft(shared_ptr<CServerUserItem> &pUserItem, bool bSendToOther);
    void SetMatchRoom(IMatchRoom* pMatchRoom){m_pMatchRoom=pMatchRoom;}

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
    virtual bool SaveReplay(tagGameReplay replay);
private:
    bool SaveReplayDetailJson(tagGameReplay& replay);
    bool SaveReplayDetailBlob(tagGameReplay& replay);
public:
    // 公共写方法
    virtual int32_t CommonFunc(eCommFuncId funcId, std::string &jsonInStr ,std::string &jsonOutStr ){return 0;}

    virtual int64_t ReadJackpot( int32_t JackpotId ){return 0;}
    virtual bool UpdateJackpot(int32_t optype,int64_t incScore,int32_t JackpotId,int64_t *newJackPot){return true;}
    bool SaveJackpotRecordMsg(string &msgStr,int32_t msgCount){return true;}



protected:  // the special protect data value item content.
    shared_ptr<ITableFrameSink>        m_pTableFrameSink;

    tagGameInfo*            m_pGameInfo;
    tagGameRoomInfo*        m_pGameRoomInfo;
    ILogicThread*           m_pLogicThread;
    TableState              m_TableState;
    IMatchRoom*              m_pMatchRoom;
    shared_ptr<EventLoopThread> m_game_logic_thread;
    shared_ptr<EventLoopThread> m_game_mysql_thread;
public:
    vector<shared_ptr<CServerUserItem>> m_UserList;

protected:
    uint8_t                 m_cbGameStatus;

private:
    static atomic_llong            m_CurStock;
    static double                  m_LowestStock;
    static double                  m_HighStock;

    static int                     m_SystemAllKillRatio;
    static int                     m_SystemReduceRatio;
    static int                     m_ChangeCardRatio;
};




