#pragma once

#include <stdint.h>

#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoopThread.h"

#include "public/gameDefine.h"
#include "ServerUserItem.h"

using namespace muduo::net;

class ITableFrameSink;


class ITableFrame : public enable_shared_from_this<ITableFrame>
{
public:
    ITableFrame()          = default;
    virtual ~ITableFrame() = default;

public:
    virtual void Init(shared_ptr<ITableFrameSink>& pSink, TableState& tableState,
                      tagGameInfo* pGameInfo, tagGameRoomInfo* pGameRoomInfo,
                      shared_ptr<EventLoopThread>& gameLogicThread, shared_ptr<EventLoopThread>& gameDBThread,ILogicThread*) = 0;

    virtual string GetNewRoundId() = 0;

public:
    virtual bool SendTableData(uint32_t chairId, uint8_t subId, const uint8_t* data = 0, int len = 0, bool isRecord = true) = 0;
    virtual bool SendUserData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t subId, const uint8_t* data, int size, bool isRecord = true) = 0;
    virtual bool SendGameMessage(uint32_t chairId, const std::string& szMesage, uint8_t msgType, int64_t score = 0) = 0;
//    virtual bool SendLookOnData(word chairid, byte_t subid, const char* data, dword size, bool isrecord = true) = 0;

public: //table info
    virtual void GetTableInfo(TableState& TableInfo) = 0;
    virtual uint32_t GetTableId() = 0;

    virtual shared_ptr<EventLoopThread> GetLoopThread() = 0;

    virtual bool DismissGame() = 0;
    virtual bool ConcludeGame(uint8_t gameStatus) = 0;
    virtual int64_t CalculateRevenue(int64_t score) = 0;
//    virtual int64_t CalculateAgentRevenue(uint32_t chairId, int64_t revenueScore) = 0;

public:  //user
    virtual shared_ptr<CServerUserItem> GetTableUserItem(uint32_t chairId) = 0;
    virtual shared_ptr<CServerUserItem> GetUserItemByUserId(int64_t userId) = 0;

    virtual bool IsExistUser(uint32_t chairId) = 0;
    virtual bool CloseUserConnect(uint32_t chairId) = 0;

public:// game status.
    virtual void SetGameStatus(uint8_t status = GAME_STATUS_FREE) = 0;
    virtual uint8_t GetGameStatus() = 0;
//    virtual void GameRoundStartDeploy() = 0;

public:  //trustee
    virtual void SetUserTrustee(uint32_t chairId, bool bTrustee) = 0;
    virtual bool GetUserTrustee(uint32_t chairId) = 0;

    virtual void SetUserReady(uint32_t chairId) = 0;
    virtual bool OnUserLeft(shared_ptr<CServerUserItem>& pUser, bool bSendToSelf=true, bool bForceLeave=false) = 0;
    virtual bool OnUserOffline(shared_ptr<CServerUserItem>& pUser, bool bLeavedGS=false) = 0;

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser) = 0;
    virtual bool CanLeftTable(int64_t userId) = 0;

public:
    virtual uint32_t GetPlayerCount(bool bIncludeAndroid) = 0;
    virtual uint32_t GetAndroidPlayerCount() = 0;
    virtual void GetPlayerCount(uint32_t &nRealPlayerCount, uint32_t &nAndroidPlayerCount)=0;
    virtual uint32_t GetMaxPlayerCount() = 0;

public:  //user status
    virtual tagGameRoomInfo* GetGameRoomInfo() = 0;

public:
//    virtual int64_t GetUserEnterMinScore() = 0;
//    virtual int64_t GetGameCellScore() = 0;
//    virtual bool AddUserScore(uint32_t chairId, int64_t addScore, bool bBroadcast = true) = 0;

public:
    virtual void ClearTableUser(uint32_t chairId = INVALID_CHAIR, bool bSendState = true, bool bSendToSelf = true, uint8_t sendErrorCode = 0) = 0;
    virtual bool IsAndroidUser(uint32_t chairId) = 0;

public: //game event
    virtual bool OnGameEvent(uint32_t chairId, uint8_t subId, const uint8_t* pData, int dataSize) = 0;
    virtual void onStartGame() = 0;

    virtual bool IsGameStarted() =  0;
//    virtual bool CheckGameStart() = 0;
public:
    virtual bool OnUserEnterAction(shared_ptr<CServerUserItem>& pUserItem, Header *commandHeader, bool bDistribute = false) = 0;
    virtual bool OnUserStandup(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendState = true, bool bSendToSelf = false) = 0;

public:
    virtual void BroadcastUserInfoToOther(shared_ptr<CServerUserItem>& pServerUserItem) = 0;
    virtual void SendAllOtherUserInfoToUser(shared_ptr<CServerUserItem>& pServerUserItem) = 0;
    virtual void SendOtherUserInfoToUser(shared_ptr<CServerUserItem>& pServerUserItem, tagUserInfo &userInfo) = 0;
    virtual void BroadcastUserStatus(shared_ptr<CServerUserItem>& pUser, bool bSendToSelf = false) = 0;


public:
    virtual bool WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, string &strRound) = 0;
    virtual bool WriteSpecialUserScore(tagSpecialScoreInfo* pScoreInfo, uint32_t nCount, string &strRound) = 0;
    bool UpdateUserScoreToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo);

    //记录水果机免费游戏剩余次数 add by caiqing
    virtual bool WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord) = 0;
    virtual bool GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId) = 0;

public: // storage.
    virtual int  UpdateStorageScore(int64_t changeStockScore) = 0;
    virtual bool GetStorageScore(tagStorageInfo &storageInfo) = 0;
    // 彩金消息
    virtual bool UpdateJackpot(int32_t optype,int64_t incScore,int32_t JackpotId,int64_t *newJackPot) = 0;
    virtual int64_t ReadJackpot( int32_t JackpotId ) = 0;

public: // 公共的接口.
    virtual int32_t CommonFunc(eCommFuncId funcId, std::string &jsonInStr ,std::string &jsonOutStr) = 0;

public:
    //save on gameend
    virtual bool SaveReplay(tagGameReplay replay) = 0;

    virtual bool SaveKillFishBossRecord(GSKillFishBossInfo& bossinfo) = 0;

public: // try to query the external interface.
    virtual int QueryInterface(const char* guid, void** ptr, int ver=1) { return -2;}
    //设置小黑屋个人库存控制等级
    virtual void setkcBaseTimes(int kcBaseTimes[],double timeout,double reduction) = 0;
//public:
//    mutable boost::shared_mutex    m_list_mutex;
//    mutable muduo::RecursiveMutexLock      m_RecursiveMutextLockEnterLeave;

};

// declare the special function type to create sink.
typedef shared_ptr<ITableFrameSink> (*PFNCreateTableSink)(void);



