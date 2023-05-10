#ifndef ANDROIDUSERMANAGER_H
#define ANDROIDUSERMANAGER_H

#include <list>
#include <set>
#include <map>
#include <deque>

#include <boost/serialization/singleton.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/net/TcpClient.h>

#include "public/IAndroidUserItemSink.h"

#include "GameServer/AndroidUserItem.h"
#include "MatchRoom.h"

using namespace std;


using boost::serialization::singleton;


class CAndroidUserManager : public singleton<CAndroidUserManager>
{
public:
    CAndroidUserManager();
    virtual ~CAndroidUserManager();

public:
    void Init(MatchRoomInfo* pMatchInfo, tagGameRoomInfo * pGameRoomInfo, shared_ptr<EventLoopThread> pILogicThread);
    bool LoadAndroidParam();

    uint32_t GetMaxAndroidCountPerTable();

    shared_ptr<CAndroidUserItem> GetFreeAndroidUserItem();
//    void CreateAllAndroid();

//    void OnTimeTick();
    void OnTimerCheckUserIn();
    void Hourtimer(shared_ptr<EventLoopThread>& eventLoopThread);
    void AddAndroid(shared_ptr<tagAndroidUserParameter>& pAndroidParam);
    void ClearAndroidUserParam();

//    shared_ptr<CAndroidUserItem> CreateAndroidUserItem();
    bool DeleteAndroidUserItem(int64_t userId);

//    shared_ptr<CServerUserItem> FindAndroidUserItem(uint32_t userid);

public:
//    uint32_t GetOnlineAndroidCount()    {   return (uint32_t)m_AndroidUserIdToUserItemMap.size();   }

//    void  UpdateOnlineCount();

//    void SetAndroidUserScore(int64_t userId, int64_t userScore);
    void SetAndroidUserCount(uint32_t activeAndroidCount, uint32_t androidMaxUserCount);

//    uint32_t GetAndroidMaxUserCount(bool bMaxonly=false);

    // update the special android user status content value.
//    void UpdateAndroidStatus(int64_t userId, uint8_t cbStatus);


protected:
    MatchRoomInfo*                              m_pMatchInfo;
    tagGameRoomInfo*                            m_pGameRoomInfo;
    tagAndroidStrategyParam                     m_pAndroidStrategy;
    shared_ptr<EventLoopThread>                 m_game_logic_thread;

    vector<shared_ptr<CAndroidUserItem>>         m_AllAndroidUserVec;
    list<shared_ptr<CAndroidUserItem>>           m_AndroidUserItemUsedList;
    list<shared_ptr<CAndroidUserItem>>           m_AndroidUserItemFreeList;

    uint32_t                                    m_maxAndroidCountPerTable;
    uint32_t                                    m_maxAndroidMaxUserCountPerTable;
    double_t                                    m_androidEnterPercentage;
    PFNCreateAndroid                            m_pfnCreateAndroid;
    muduo::net::TimerId m_timerIdCheckUserIn;
private:
//#if 0
//    Landy::GameTimer                             m_TimerCheckIn;
//#else
//    muduo::net::EventLoopThread                 m_TimerLoopThread;
//#endif

private:
//    mutable boost::shared_mutex m_mutex;

private:
//    muduo::MutexLock   m_AddAndroidUsermutex;


//using namespace muduo;
//    map<uint32_t, shared_ptr<CServerUserItem>>  m_AndroidUserIdToUserItemMap;
//    shared_ptr<CQipaiAIThread>                  m_pQipaiAIThread;
//typedef set<shared_ptr<tagAndroidUserParameter>>           setAndroidParam;
//typedef list<shared_ptr<tagAndroidUserParameter>>          listAndroidParam;
//typedef map<uint32_t, shared_ptr<tagAndroidUserParameter>>  mapAndroidParam;
//typedef std::map<uint32_t, GS_UserBaseInfo>                           mapAndroidUserBaseInfo;

//    int GetRandomHeadBoxIndex(int vip);
//    int GetRandomNickNameForAndroid(vector<string> &nickNameVec, int num);
//    int GetRandomLocationForAndroid(vector<string> &locationVec, int num);
//    string GetRandomLocationForAndroid();
//protected://android config.
//    set<shared_ptr<tagAndroidUserParameter>>                    m_AllAndroidParamList;
//    list<shared_ptr<tagAndroidUserParameter>>                   m_AndroidParamFreeList;
//    map<uint32_t, shared_ptr<tagAndroidUserParameter>>          m_AndroidParamUsedUserIdToParamMap;

//public:
//    void SetQipaiAiThread(shared_ptr<CQipaiAIThread>& pThread)  { m_pQipaiAIThread = pThread;           }
//    shared_ptr<CQipaiAIThread> GetQipaiThread()                 { return m_pQipaiAIThread;              }

//    GS_UserBaseInfo* GetAndroidUserBaseInfo(uint32_t userid);
//    void SetAndroidTimeTickInterval(uint32_t dwTickInterval)
//    {
//        m_dwAndroidTickInterval = dwTickInterval;
//    }
//    uint32_t GetAndroidTickInterval()               { return m_dwAndroidTickInterval;       }



};


#endif // ANDROIDUSERMANAGER_H
