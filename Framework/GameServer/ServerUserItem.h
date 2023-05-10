// Copyright (c) 2019, Landy
// All rights reserved.
#ifndef SERVERUSERITEM_H
#define SERVERUSERITEM_H

#include <atomic>

#include <boost/shared_ptr.hpp>

#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/ThreadLocalSingleton.h>

#include <muduo/base/Logging.h>

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/IServerUserItem.h"



using namespace muduo::net;
struct GSKillFishBossInfo
{
    GSKillFishBossInfo()
    {
        userId = 0;
        agentId = 0;
        gameId = 0;
        roomId = 0;
        fishType = 0;
        cannonLevel = 0;
        fishAdds = 0;
        winScore = 0;
    }

    int64_t     userId;
    int32_t     agentId;
    int32_t     gameId;
    int32_t     roomId;
    int32_t     fishType;
    int32_t     cannonLevel;
    int32_t     fishAdds;
    int32_t     winScore;
    chrono::system_clock::time_point insertTime;
    int64_t     timestamp;
};
//跨线程写数据库的中间参数
struct GSUserWriteMysqlInfo
{
    GSUserWriteMysqlInfo()
    {
        userId = 0;
        strRoundId = "";
        rankId = 0;
        account = "";
        agentId = 0;
        lineCode = "";
        gameId = 0;
        roomId = 0;
        tableId = 0;
        chairId = 0;
        isBanker = 0;
        rWinScore = 0;
        cardValue = "";
        userCount = 0;
        cellScore = "";
        effectiveScore = 0;
        beforeScore = 0;
        allBet = 0;
        score = 0;
        revenue = 0;
        isAndroid = 0;
        gameInfoId = "";
        gameEvent = "";
        gameStartTime = "";
        gameEndTime = "";
        changetype = 0;
        changemoney = 0;
        fishRake = 0;
        cellScoreIntType.clear();
    }
    int64_t     userId;
    string      strRoundId;
    int32_t     rankId;
    string      account;
    int32_t     agentId;
    string      lineCode;
    int32_t     gameId;
    int32_t     roomId;
    int32_t     tableId;
    int32_t     chairId;
    int32_t     isBanker;
    int64_t     rWinScore;
    string      cardValue;
    int32_t     userCount;
    string      cellScore; //每一方下注
    int64_t     effectiveScore;//有效投注
    int64_t     beforeScore; //下注前玩家分数
    int64_t     allBet;
    int64_t     winScore;
    int64_t     score;
    int64_t     revenue;
    int32_t     isAndroid;
    string      gameInfoId;
    string      gameEvent;//游戏事件
    string      gameStartTime;
    string      gameEndTime;
    int32_t     changetype;
    int64_t     changemoney;
    int32_t     fishRake;
    int32_t     currency;
    vector<int64_t> cellScoreIntType;
    chrono::system_clock::time_point startTime;  //时间点兼容
    chrono::system_clock::time_point endTime;  //时间点兼容
};
struct GSUserBaseInfo
{
    GSUserBaseInfo()
    {
        userId = -1;
        account = "";
        headId = 0;
        nickName = "";
        userScore = 0;
        agentId = 0;
        lineCode = "";
        status = 0;
        location = "";

        takeMaxScore = 0;
        takeMinScore = 0;
		ip = 0;
        currency = 0;
    }

    int64_t    userId;                // set the user id.
    string     account;
    uint8_t    headId;                // set the header id.
    string     nickName;
    int64_t    userScore;             // set the score info.
    uint32_t   agentId;
    string     lineCode;
    uint32_t   status;
	uint32_t   ip;
    string     location;

    int64_t    takeMaxScore;
    int64_t    takeMinScore;
    int64_t    regSecond;//注册时长
    int32_t    currency;
};

//    int64_t    allChargeAmount;       // charge mount all
//    int64_t    allWinScore;           // gain score all
//    char    szHeadUrl[LEN_HEAD_URL];    // set the header url.
//    char    szIp[LEN_IP];               // set the ip.
//    uint8_t  nGender;                // set the gender.
//    uint8_t  nVipLevel;              // set the vip level.
//    uint8_t  nVipLevel2;             // set the vip level2.
//    uint8_t  nHeadBoxId;				// set the head box id.
//    double   dLogitude;              // set the logitude.
//    double   dLatitude;              // set the latitude.
//    SCORE    nDefaultScore;          // default score
//    uint8_t  nIsSuperAccount;        // is super account


//struct GSUserScoreInfo
//{
//    GSUserScoreInfo()
//    {
//        userId = 0;
//        enterScore = 0;
//        betScore = 0;
//        addScore = 0;
//    }

//    int64_t   userId;                // set the user id
//    int64_t   enterScore;
//    int64_t   betScore;              // set the bet score
//    int64_t   addScore;              // set the add score.
//};

//    int64_t   totalGain;
//    int64_t   totalLost;
//    uint32_t   nWinCount;              // set the win count.
//    uint32_t   nLostCount;             // set the lost count.
//    uint32_t   nDrawCount;             // set the draw count.
//    uint32_t   nFleeCount;             // set the flee count.
//    uint32_t   nPlayTime;              // set the play time.
//    uint32_t   nEnterTime;             // set the enter time.
//    uint32_t   nRecordID;              // record user inout.
//    int64_t   nEnterGem;
//    string    szEnterTime;        // in out in time
//    int64_t   nRevenue;               // set the revenue.
//    int64_t   nAgentrevenue;          // set the agent revenue.
//    char    szCurMonth[12];         // record curMonth
//    string    szMachine[LEN_MACHINESERIAL];
//    string    szIp[LEN_IP];

class CServerUserItem : public IServerUserItem
{
public:
    CServerUserItem();
    virtual ~CServerUserItem() = default;

public:
    virtual void ResetUserItem();

public:
    virtual bool IsAndroidUser();
    virtual shared_ptr<IAndroidUserItemSink> GetAndroidUserItemSink()
    {
        shared_ptr<IAndroidUserItemSink> pIAndroidUserItemSink;
        return pIAndroidUserItemSink;
    }

public:
    virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, const uint8_t* data, uint32_t len);
    virtual bool SendSocketData(uint8_t subId, const uint8_t* data, uint32_t len);

public:
    virtual int64_t  GetUserId()            { return m_userBaseInfo.userId;             }
    virtual const string GetNickName()      { return m_userBaseInfo.account;            }
    virtual uint8_t  GetHeaderId()          { return m_userBaseInfo.headId;             }
    virtual const string GetAccount()       { return m_userBaseInfo.account;            }
    // 注册时长
    virtual int64_t  GetUserRegSecond()     { return m_userBaseInfo.regSecond;          }


    void SetMatchId(uint32_t matchId)       { m_matchId=matchId;                         }
    uint32_t GetMatchId()                   { return m_matchId;                          }

    void SetRank(uint32_t rank)             { m_rank = rank ;                           }
    uint32_t GetRank()                      { return m_rank ;                           }

    virtual uint32_t GetTableId()           { return (m_tableId);                       }
    void SetTableId(uint32_t tableId)       { m_tableId = tableId;                      }

    virtual uint32_t GetChairId()           { return m_chairId;                         }
    void SetChairId(uint32_t chairId)       { m_chairId = chairId;                      }

    void SetAgentId(int64_t agentId)         { m_userBaseInfo.agentId = agentId;           }
    void SetUserId(int64_t userId)          { m_userBaseInfo.userId = userId;           }
    virtual int64_t GetUserScore()          { return m_userBaseInfo.userScore;          }
    void SetUserScore(int64_t userScore)    { m_userBaseInfo.userScore = userScore;     }

	virtual const uint32_t GetIp()			{ return m_userBaseInfo.ip;					}
    virtual const string GetLocation()	    { return m_userBaseInfo.location;           }

    virtual int GetUserStatus()             { return m_userStatus;                      }
    virtual void SetUserStatus(uint8_t userSatus) { m_userStatus = userSatus;           }

public:
    virtual bool isGetout()                     {  return sGetout  == m_userStatus;      }
    virtual bool isSit()                        {  return sSit     == m_userStatus;      }
    virtual bool isReady()                      {  return sReady   == m_userStatus;      }
    virtual bool isPlaying()                    {  return sPlaying == m_userStatus;      }
    virtual bool isBreakline()                  {  return sOffline == m_userStatus;      }
    virtual bool isLookon()                     {  return sLookon  == m_userStatus;      }
    virtual bool isGetoutAtplaying()            {  return sGetoutAtplaying == m_userStatus;}

    virtual void setUserReady()                 {  SetUserStatus(sReady);               }
    virtual void setUserSit()                   {  SetUserStatus(sSit);                 }
    virtual void setOffline()                   {  SetUserStatus(sOffline);             }

    virtual void setTrustee(bool bTrust)        {  SetTrustship(bTrust);                }
    virtual void setClientReady(bool bReady)    {  m_bClientReady = bReady;             }
    virtual bool isClientReady()                {  return (m_bClientReady);             }

public:
    void SetTrustship(bool bTrustship)          { m_trustShip = bTrustship;             }
    bool GetTrustship()                         { return m_trustShip;                   }

    int  GetTakeMaxScore()                      { return m_userBaseInfo.takeMaxScore;  }
    int  GetTakeMinScore()                      { return m_userBaseInfo.takeMinScore;  }

public:
    GSUserBaseInfo& GetUserBaseInfo()           { return m_userBaseInfo;                }
    void SetUserBaseInfo(const GSUserBaseInfo& info)     {      m_userBaseInfo = info;   }

    tagBlacklistInfo& GetBlacklistInfo()           { return m_blacklistInfo;                }
    tagPersonalProfit& GetPersonalPrott()           {return m_personalProfit; }
	tagBlackRoomlistInfo& GetBlackRoomlistInfo()           { return m_blackRoomlistInfo;     } //小黑屋
//    void SetBlacklistInfo(const tagBlacklistInfo& info)     {      m_blacklistInfo = info;   }

//    GSUserScoreInfo& GetUserScoreInfo()        { return m_userScoreInfo;                }
//    void SetUserScoreInfo(GSUserScoreInfo &scoreInfo)   {  m_userScoreInfo = scoreInfo; }

public:
    virtual bool isValided();
    vector<string>      m_sMthBlockList;
    bool        inQuarantine=false;
//    bool        inBlacklist=false;

protected:// userid now.
    uint32_t           m_tableId;
    uint32_t           m_chairId;
    uint32_t           m_matchId;
    uint8_t            m_userStatus;
    bool               m_trustShip;
    bool               m_bClientReady;
    uint32_t           m_rank;
protected:
    GSUserBaseInfo     m_userBaseInfo;
    tagBlacklistInfo    m_blacklistInfo;
    tagPersonalProfit  m_personalProfit;
	tagBlackRoomlistInfo    m_blackRoomlistInfo;
//    GSUserScoreInfo    m_userScoreInfo;

private:
    bool                m_bisAndroid;     // is android user.
};





#endif
