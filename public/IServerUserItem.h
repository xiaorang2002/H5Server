#pragma once

#include <stdint.h>

//#include <bsoncxx/json.hpp>
//#include <bsoncxx/types.hpp>
//#include <bsoncxx/oid.hpp>
//#include <bsoncxx/stdx/optional.hpp>
//#include <mongocxx/client.hpp>
//#include <mongocxx/stdx.hpp>
//#include <mongocxx/uri.hpp>


#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include <muduo/net/Callbacks.h>

#include "Globals.h"
#include "gameDefine.h"


class IAndroidUserItemSink;



class ILogicThread
{
public:
//    virtual dword GetWaitListCount() = 0;
//    virtual bool OnAddUserWaitList(muduo::net::TcpConnectionPtr pSocket, dword userid, uint64_t value1) = 0;
    virtual void KickUserIdToProxyOffLine(int64_t userId, int32_t nKickType) = 0;
    virtual void SavePlayGameTime(int64_t userId) = 0;
    virtual boost::tuple<muduo::net::TcpConnectionWeakPtr, shared_ptr<internal_prev_header>> GetProxyConnectionWeakPtrFromUserId(int64_t userId) = 0;
    virtual bool IsServerStoped() = 0;

    virtual void clearUserIdProxyConnectionInfo(int64_t userId) = 0;

//    virtual void RunWriteDataToMongoDB(string db, string collection, MongoDBOptType type, bsoncxx::document::view view1, bsoncxx::document::view view2) = 0;
};


class IServerUserItem
{
public:
    IServerUserItem() = default;
    virtual ~IServerUserItem() = default;

public:
    virtual void ResetUserItem() = 0;

public:
    virtual bool IsAndroidUser() = 0;
    virtual shared_ptr<IAndroidUserItemSink> GetAndroidUserItemSink() = 0;

public:
    virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, const uint8_t* data, uint32_t len) = 0;
    virtual bool SendSocketData(uint8_t subId, const uint8_t* data, uint32_t len) = 0;

    virtual int64_t    GetUserId()         = 0;
    virtual const string GetAccount()      = 0;
    virtual const string GetNickName()     = 0;
    virtual uint8_t    GetHeaderId()       = 0;

    virtual uint32_t   GetTableId()        = 0;
    virtual uint32_t   GetChairId()        = 0;

    virtual int64_t    GetUserScore()      = 0;

    virtual const string GetLocation()     = 0;

    virtual int        GetUserStatus()     = 0;
    virtual void       SetUserStatus(uint8_t) = 0;

    virtual int64_t    GetUserRegSecond()  = 0;
public:
    virtual bool isGetout()             = 0;
    virtual bool isSit()                = 0;
    virtual bool isReady()              = 0;
    virtual bool isPlaying()            = 0;
    virtual bool isBreakline()          = 0;
    virtual bool isLookon()             = 0;
    virtual bool isGetoutAtplaying()    = 0;

    virtual void setUserReady()         = 0;
    virtual void setUserSit()           = 0;
    virtual void setOffline()           = 0;

    virtual void setTrustee(bool bTrust)= 0;
    virtual void setClientReady(bool bReady) = 0;
    virtual bool isClientReady()        = 0;
};




