#pragma once



#include <map>
#include <list>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>


#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/net/TcpServer.h"
#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/BlockingQueue.h"



#include "public/Globals.h"
#include "public/EntryPtr.h"
#include "public/UserProxyInfo.h" 
#include "public/TaskService.h"



#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "ConnectionPool.h"

#include "ServerUserItem.h"
#include "GameTableManager.h"

#include "TableFrame.h"



using namespace std;
using namespace muduo;
using namespace muduo::net;

class ITableFrameSink;
class CServerUserItem;
struct RedisClient;

namespace Landy
{





const int LOOP_THREAD_COUNT = 2;
const int WORK_THREAD_COUNT = 10;



#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const shared_ptr<T>& x)
{
  return boost::hash_value(x.get());
}
}
#endif


//#pragma pack(1)

//struct MsgPacker
//{
////    TcpConnectionWeakPtr weakConn;
//    int32_t userId;
//    int msgdataLen;
//    vector<unsigned char> msgdata;
////    shared_ptr<internal_prev_header> internal_header;
//};

//#pragma pack()


struct UserProxyConnectionInfo
{
    string strProxyIP;
    shared_ptr<internal_prev_header> internal_header;
};

class GameServer : public boost::noncopyable,
                   public ILogicThread
{
public:
    typedef function<void(muduo::net::TcpConnectionWeakPtr&, uint8_t*, int, internal_prev_header *internal_header)> AccessCommandFunctor;
    typedef shared_ptr<muduo::net::Buffer> BufferPtr;

public:
    GameServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, int idleSeconds, string netcardName);
    ~GameServer();

    void init();
    void quit();

    void setThreadNum(int numThreads);
    bool loadRSAPriKey();
    string decryDBKey(string password, int db_password_real_len);
    bool loadKey();

    bool startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize);
//    bool startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize);
    bool InitRedisCluster(string ip, string password);
    bool InitZookeeper(string ip, uint32_t gameId, uint32_t roomId);
    bool InitMongoDB(string url);

    void singleThreadInit(EventLoop* ep);
    void singleThreadInit1(EventLoop* ep);
    void threadInit();

    void start(int threadCount);

    void OnRepairGameServerNode();
    void ZookeeperConnectedHandler();
//    void getChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
//                        const string &path, void *context);

private: // load game and room kind data.
    bool LoadGameRoomKindInfo(uint32_t gameId, uint32_t roomId);


public:
    bool LoadLibraryEx(PFNCreateTableSink& pfnCreateSink);
    bool InitServer(uint32_t gameId, uint32_t roomId);
    bool LoadAndroidParam();


    bool IsServerStoped() { return m_bStopServer; }

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime);

    void processRequest(muduo::net::TcpConnectionWeakPtr& conn,
                                   BufferPtr &buf,
                                   muduo::Timestamp receiveTime);

public:


private:
    boost::tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>>  GetProxyConnectionWeakPtrFromUserId(int64_t userId);
    bool sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data, internal_prev_header *internal_header, Header *commandHeader);

    bool GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId);

private:
    void cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);

    void cmd_enter_room(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    bool SendGameErrorCode(TcpConnectionWeakPtr& weakConn, uint8_t mainId, uint8_t subId,
                           uint32_t nRetCode, string errMsg,
                           internal_prev_header *internal_header, Header *commandHeader);
    bool SwitchUserItem(shared_ptr<CServerUserItem>& pIServerUserItem, TcpConnectionWeakPtr& conn, internal_prev_header *internal_header, Header *commandHeader);


    bool CheckUserCanEnterRoom(int64_t userId, TcpConnectionWeakPtr& weakPtr, internal_prev_header *internal_header, Header *commandHeader, uint32_t tableId = INVALID_TABLE);
    bool GetUserBaseInfo(int64_t userId, GSUserBaseInfo &baseinfo);


    void cmd_user_change_table(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);

    void cmd_user_ready(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void cmd_user_left(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void cmd_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void clearUserIdProxyConnectionInfo(int64_t userId);

    void cmd_game_message(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);


    void cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void cmd_user_recharge_ok(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);

    void cmd_notifyRepairServerResp(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);

//    void WriteDailyEnterRoomRecord(int32_t userId, int nOSType, int channelId, int promoterId);
    void DBWriteRecordDailyUserOnline(shared_ptr<RedisClient> &redisClient, int nOSType, int channelId, int promoterId);
    void ChangeGameOnLineStatus(shared_ptr<RedisClient> &redisClient, int64_t userId, int configId);

//    void cmd_user_change_table_sit_chair(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &msgdata, int msgdataLen, internal_prev_header *internal_header);

    virtual void KickUserIdToProxyOffLine(int64_t userId, int32_t kickType = KICK_GS|KICK_CLOSEONLY);
    void updateAgentPlayers();

    void PersonalProfit( shared_ptr<CServerUserItem> & pIServerUserItem );
public:
    void SavePlayGameTime(int64_t userId);

    bool SetBlackeListInfo(string insert,map<string,int16_t> &usermap);

public:
//    virtual uint32_t GetWaitListCount();
//    virtual bool OnAddUserWaitList(muduo::net::TcpConnectionPtr pSocket, int64_t userId, uint64_t value1);
//    bool AddWaitList(int64_t userId, uint64_t clientaddr, muduo::net::TcpConnectionWeakPtr pSocket);
//    void UpdateServerIDWaitListCount(uint32_t nwaitlist);

    void DistributeTable();
    bool DelGameUserToProxy(int64_t userId, muduo::net::TcpConnectionWeakPtr pproxySock);


public:
    void notifyRechargeScoreToGameServerMessage(string msg);
    void notifyExchangeScoreToGameServerMessage(string msg);

    void notifyStopGameServerMessage(string msg);
    void notifyRefreashConfig(string msg);

private:
    shared_ptr<ZookeeperClient>         m_zookeeperclient;
    string m_redisIPPort;
    string m_redisPassword;

    string m_mongoDBServerAddr;
    string m_szNodePath, m_szNodeValue;
    string m_szInvalidNodePath;
    string m_netCardName;

    uint32_t m_gameId;
    uint32_t m_roomId;

    shared_ptr<RedisClient>  m_redisPubSubClient;

private:
    static map<int, AccessCommandFunctor>  m_functor_map;


private:
    string m_pri_key;

private:
    string m_myIP;
    uint16_t m_myPort;
    int32_t m_proxyId;
    int32_t m_currency;

private:
    map<string, TcpConnectionWeakPtr>       m_IPProxyMap;
    mutable boost::shared_mutex             m_IPProxyMapMutex;

    map<int64_t, shared_ptr<UserProxyConnectionInfo>>   m_UserIdProxyConnectionInfoMap;
    mutable boost::shared_mutex             m_UserIdProxyConnectionInfoMapMutex;

public:

    muduo::net::TcpServer m_server;
    AtomicInt32  m_connection_num;

private:
    const static size_t kHeaderLen = sizeof(int16_t);

//    shared_ptr<EventLoopThread>     m_timer_thread;
    shared_ptr<EventLoopThread>     m_game_logic_thread;
    atomic_bool m_bStopServer;
    atomic_bool m_bInvaildServer;
    shared_ptr<EventLoopThread>     m_game_sqldb_thread;
private:


private:
    tagGameInfo                 m_GameInfo;         // game kind.
    tagGameRoomInfo             m_GameRoomInfo;     // game play kind.
    tagGameReplay               m_GameReplay;// game replay log
//    PFNCreateTableSink  m_CreateTableSink;  // table sink.

//    list<tagWaitListInfo>       m_listWaitList;     // wait player list.

};


//    shared_ptr<RedisClient>      m_redisClient;
//    muduo::ThreadPool m_thread_pool;
//    muduo::ThreadPool m_game_msg_pool;
//    shared_ptr<muduo::Thread>       m_game_thread;
//private:
//    muduo::MutexLock    m_lockEnter;
//    muduo::RecursiveMutexLock m_RecursiveMutexLockEnter;

//      MAPUSERBASEINFO m_mapBaseinfo;          // declare the map.


#if 0
    void getInvalidGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                        const string &path, void *context);
    void ProcessInvalidGameIPServer(vector<string> &gameServers);

    void KickGameServerUser(vector<string> &invalidGameServersVec);
#endif

//    void LoadRoomServerAndroidCount();
//    bool InsertGameServerRoomInfo();

//    int GetRandomNickNameForAndroid(vector<string> &nickNameVec, int num);
//    int GetRandomLocationForAndroid(vector<string> &locationVec, int num);
//    string GetRandomLocationForAndroid();
//    bool LoadBenefitConfig();
//    int GetRandomHeadBoxIndex(int vip);

//private:// reload parameter per 5 minute.
//    void OnReLoadParameter();
//    void OnReloadAndroidParam();

//    bool GetConfigId(int serverId, int& configId);
//    void OnTimerTick();
//    void OnAndroidTick();
//    void OnAndroidCheckIn();
//    void RefreshTableUserCountTimer();

//    string createVerifyCode();
//    private:
//        void process_game_message();

//    void cmd_get_aes_key(muduo::net::TcpConnectionWeakPtr &weakConn, vector<unsigned char> &msgdata, int msgdataLen, internal_prev_header *internal_header);
//    void cmd_get_verify_code(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &msgdata, int msgdataLen, internal_prev_header *internal_header);
//    void cmd_login_servers(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &msgdata, int msgdataLen, internal_prev_header *internal_header);

//#if 0
//    //==========game server list==========
//    list<string>  m_invalidGameServerIps;
//    MutexLock m_invalidGameServerIps_mutex;
//#endif

//#if 0
// int InitDebugInfo(int bisDebug);
//#endif
// bool GetAllChargeWinScore(int64_t userId, int64_t &nAllChargeAmount, int64_t &nAllWinScore);

//    protected:
//        bool DBRecordGuestRewards(string ip, int64_t userId);
//        int  DBCountGuestRewards(string ip);

//    bool DelUserOnlineInfo(int64_t userId);

//    void RunWriteDataToMongoDB(string db, string collection, MongoDBOptType type, bsoncxx::document::view view1, bsoncxx::document::view view2);
//    void WriteDataToMongoDB(string db, string collection, MongoDBOptType type, bsoncxx::document::view view1, bsoncxx::document::view view2);

}



