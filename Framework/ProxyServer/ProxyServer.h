//#ifndef PROXYSERVER_H
//#define PROXYSERVER_H

#pragma once


#include <map>
#include <list>


#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>


#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/net/WebSocket/WebSocketServer.h>

#include "public/MyEventLoopThreadPool.h"

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"

#include "public/RedisClient/RedisClient.h"

#include "public/ThreadLocalSingletonMongoDBClient.h"

// #include "RocketMQ/RocketMQ.h"
#include "json/json.h"

#include "public/base64.h"



#define MULTI_THREAD      (0)
#define MUDUO_THREAD_POOL (1)
#define MY_THREAD_POOL    (0)


using namespace std;
using namespace muduo;
using namespace muduo::net;



namespace  Landy
{

// 公共消息(针对前端使用)
enum class eProxyPublicMsgType
{
    bulletinMsg             = 0,//电子公告消息
    lkGameMsg               = 1,//幸运转盘的消息
    marqueeMsg              = 2,//游戏获奖跑马灯消息
    lkJackpot               = 3,//奖池消息
};



#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const boost::shared_ptr<T>& x)
{
  return boost::hash_value(x.get());
}
}
#endif



class ProxyServer : public noncopyable
{

private:
    struct CountTimeOut : public muduo::copyable
    {
        explicit CountTimeOut(const TcpConnectionWeakPtr& weakCountTimeOut) : m_weakConn(weakCountTimeOut)
        {

        }

        ~CountTimeOut()
        {
            TcpConnectionPtr conn = m_weakConn.lock();
            if(conn)
            {
//                conn->shutdown();
                conn->forceCloseWithDelay(0.2);
            }
        }

        TcpConnectionWeakPtr m_weakConn;
    };
    typedef shared_ptr<CountTimeOut> CountTimeOutPtr;
    typedef weak_ptr<CountTimeOut>   WeakCountTimeOutPtr;


private:   //idle connections
    struct Entry : public muduo::copyable
    {
        explicit Entry(WeakCountTimeOutPtr& weakCountTimeOutPtr)
        : m_weakweakCountTimeOutPtr(weakCountTimeOutPtr)
        {
            m_session = "";
            m_userId = -1;
            m_aesKey = "";
            m_proxyId = 0;
//            m_channelId = -1;
            m_UserLeftCloseOnly = 0;
        }

        ~Entry()
        {

        }

        void setSession(string session) { m_session = session; }
        string getSession() { return m_session; }

        void setUserId(int64_t userId) { m_userId = userId; }
        int64_t getUserId() { return m_userId; }

        void setProxyId(int64_t proxyId) { m_proxyId = proxyId; }
        int64_t getProxyId() { return m_proxyId; }

        void setAESKey(string key) { m_aesKey = key; }
        string getAESKey() { return m_aesKey; }

        void setAESKeyCopy(string key) { m_radom_aesKey = key; }
        string getAESKeyCopy() { return m_radom_aesKey; }

        void setPubKey(string key) { m_rsaPubKey = key; }
        string getPubKey() { return m_rsaPubKey; }

        void setPriKey(string key) { m_rsaPriKey = key; }
        string getPriKey() { return m_rsaPriKey; }

        void setIP(uint32_t ip) { m_ip = ip; }
        uint32_t getIP() { return m_ip; }

        void setHallTcpClient(weak_ptr<TcpClient> &client) { m_hallTcpClient = client; }
        weak_ptr<TcpClient> getHallTcpClient() { return m_hallTcpClient; }

        void setGameTcpClient(weak_ptr<TcpClient> &client) { m_gameTcpClient = client; }
        weak_ptr<TcpClient> getGameTcpClient() { return m_gameTcpClient; }

        void setPayTcpClient(weak_ptr<TcpClient> &client) { m_payTcpClient = client; }
        weak_ptr<TcpClient> getPayTcpClient() { return m_payTcpClient; }

//        void setMatchTcpClient(weak_ptr<TcpClient> &client) { m_matchTcpClient = client; }
//        weak_ptr<TcpClient> getMatchTcpClient() { return m_matchTcpClient; }

        void setUserLeftCloseOnly(int32_t userLeftCloseOnly)   {  m_UserLeftCloseOnly = userLeftCloseOnly;  }
        int32_t getUserLeftCloseOnly()  { return m_UserLeftCloseOnly;  }


//        void setChannelId(int32_t channelId) { m_channelId = channelId; }
//        int32_t getChannelId() { return m_channelId; }

//        void setBLogin(bool blogin) { m_bLogin = blogin; }
//        bool getBLogin() { return m_bLogin; }

        CountTimeOutPtr getCountTimeOutPtr()
        {
            CountTimeOutPtr countTimeOutPtr(m_weakweakCountTimeOutPtr.lock());
            return countTimeOutPtr;
        }

    private:
        WeakCountTimeOutPtr m_weakweakCountTimeOutPtr;

        string               m_session;
        int64_t              m_userId;
        int32_t              m_proxyId;
        string               m_aesKey;
        string               m_rsaPubKey;
        string               m_rsaPriKey;
        string               m_radom_aesKey;
        uint32_t             m_ip;
        weak_ptr<TcpClient>  m_hallTcpClient;
        weak_ptr<TcpClient>  m_gameTcpClient;
        weak_ptr<TcpClient>  m_payTcpClient;
        int32_t              m_UserLeftCloseOnly;

//        int32_t              m_channelId;
//        bool                 m_bLogin;
    };

    typedef shared_ptr<Entry> EntryPtr;
    typedef weak_ptr<Entry> WeakEntryPtr;
    typedef boost::unordered_set<CountTimeOutPtr> Bucket;
//    typedef std::set<EntryPtr> Bucket;
    typedef boost::circular_buffer<Bucket> WeakConnectionList;

    WeakConnectionList m_connection_buckets;
    MutexLock m_connection_buckets_mutex;
private:


private:
    typedef function<void(const TcpConnectionPtr&, vector<uint8_t>&)> AccessCommandFunctor;
    typedef shared_ptr<Buffer> BufferPtr;

public:
    ProxyServer(EventLoop* loop, const InetAddress& listenAddr, int idleSeconds, string networkCardname,int32_t hallIpIsFixed = 0,string strHallIP = "");
    ~ProxyServer();
    void init();
    void quit();

    void setThreadNum(int numThreads);
    bool loadKey();

    bool InitZookeeper(string ip);
    bool InitRedisCluster(string ip, string password);

    bool InitRocketMQ(string &ip);
    // uint32_t RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe);

    bool startThreadPool(int threadCount);
    void ProcessPublicNotice(string &tag, string body);
    void ProcessKickUser(string &tag, string body);

//    void notifyNoticeMessage(string &title, string &publicNotice);

    void start(int threadCount);

private:
    void onTimer();
    void dumpConnectionBuckets() const;

    bool loadRSAPriKey();
    string decryDBKey(string password, int db_password_real_len);

    void threadInit();

    void ZookeeperConnectedHandler();
    void startAllHallServerConnection();
    void startHallServerConnection(string ip);
    void ProcessHallIPServer(vector<string> &servers);
    void RefreshHallServers();
    void GetHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

    void GetInvaildHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

    void startAllGameServerConnection();
    void startGameServerConnection(string ip);



    void ProcessGameIPServer(vector<string> &gameServers);
    void RefreshGameServers();
    void GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

    //担心可能的扩展 ，先分开处理
    void startAllMatchServerConnection();
    void startMatchServerConnection(string ip);
    void ProcessMatchIPServer(vector<string> &matchServers);
    void GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

//    vector<int> getAllConfigId();
    void OnRepairProxyServerNode();

    // 支付服
    void startAllPayServerConnection();
    void startPayServerConnection(string ip);
    void ProcessPayIPServer(vector<string> &payServers);
    void GetPayServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

    void GetInvaildPayServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                           const string &path, void *context);

private:
    void onTcpConnection(const TcpConnectionPtr& conn);
    void onWebSocketConnected(const TcpConnectionPtr& conn, string &ip);
    void onWebSocketClosed(const TcpConnectionPtr& conn);
    void onWebSocketMessage(const TcpConnectionPtr& conn, vector<uint8_t> &buf, muduo::Timestamp receiveTime);
    void processWebSocketRequest(const TcpConnectionPtr& conn, vector<uint8_t> &buf, muduo::Timestamp receiveTime);


private:
    bool sendData(const TcpConnectionPtr &conn, vector<uint8_t> &data);

    void cmd_get_aes_key(const TcpConnectionPtr &conn, vector<uint8_t> &msgdata);

    string getRandomHallServer();
    string getRandomPayServer();
    void sendDataToHallServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &hallTcpClient, vector<uint8_t> &msgdata, int64_t userId);
    void sendDataToGameServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &gameTcpClient, vector<uint8_t> &msgdata, int64_t userId);
    void sendDataToPayServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &hallTcpClient, vector<uint8_t> &msgdata, int64_t userId);

private:
    void onHallServerConnection(const TcpConnectionPtr& conn);
    void onHallServerMessage(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp receiveTime);
    void processHallRequest(const TcpConnectionPtr& conn, BufferPtr &buf, muduo::Timestamp receiveTime);

    void onGameServerConnection(const TcpConnectionPtr& conn);
    void onGameServerMessage(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp receiveTime);
    void processGameRequest(const TcpConnectionPtr& conn, BufferPtr &buf, muduo::Timestamp receiveTime);

    void onPayServerConnection(const TcpConnectionPtr &conn);
    void onPayServerMessage(const TcpConnectionPtr &conn,muduo::net::Buffer *buf,muduo::Timestamp receiveTime);
    void processPayRequest(const TcpConnectionPtr &conn, BufferPtr &buf, muduo::Timestamp receiveTime);
private:
    void KickUserId(string &session, int64_t userId, int32_t kickType);
    bool GetKickUserClientMessage(int64_t userId, vector<uint8_t> &data);
    void SendKickDataAndClose(const TcpConnectionWeakPtr weakPtr, vector<uint8_t> &data,int32_t bCloseOnly);
    void SendKickMessageToClient(string &session, int64_t userId, int32_t kickType);

    void OnUserOfflineHall(EntryPtr &entry);
    void OnUserOfflineGS(EntryPtr &entry,bool bLeaveGs=false);
    void DeleteUserIdMap(int64_t userId);
    bool GetShutDownUserClientMessage(int64_t userId, vector<uint8_t> &data,int32_t status=0);


private:
    void notifyPublicNoticeMessage(const Json::Value& value);
    void notifyUserLoginMessage(string msg);
    void notifyUserOrderScoreMessage(string msg);
//    void notifyRechargeScoreToProxyMessage(string msg);
//    void notifyExchangeScoreToProxyMessage(string msg);
   void notifyUserKillBossMessage(string msg);
//    void notifyNewChatMessage(string msg);
//    void notifyNewMailMessage(string msg);

    void luckyGamePushMsg(string msg); 
    void sendHallMessage(string title,string msg,int32_t agentid,eProxyPublicMsgType msgtype);

private:
    void PrintSockerMap();
    // 检查连接
    bool checkTcpConnected(int64_t userId,EntryPtr &entry);
    void check_keep_alive_ping( weak_ptr<TcpClient> gameTcpClient);
    void checkHeartBeat();

//    string getRandomGameServer();
//    void reEnterRoom(const TcpConnectionPtr &conn);


private:
    shared_ptr<ZookeeperClient> m_zookeeperclient;
    string m_szNodePath, m_szNodeValue;
    string m_netCardName;

    shared_ptr<RedisClient>  m_redisPubSubClient;
    string m_redisIPPort;
    string m_redisPassword;

    int32_t m_hallIpIsFixed;
    string m_strHallIP;

private:
    static map<int, AccessCommandFunctor>  m_functor_map;

    //==========pay server list==========
    list<string>  m_payServerIps;
    MutexLock m_payServerIps_mutex;
    
    //==========hall server list==========
    list<string>  m_hallServerIps;
    MutexLock m_hallServerIps_mutex;
    
    //==========game server list==========
    list<string>  m_gameServerIps;
    MutexLock m_gameServerIps_mutex;
    vector<string> m_gameServers;

     //==========pay ip to TcpClient========
    map<string, std::shared_ptr<TcpClient> > m_payIPServerMap;
    list<string>  m_invaildPayServerIps;
    MutexLock m_payIPServerMapMutex;

    //==========hall ip to TcpClient========
    map<string, std::shared_ptr<TcpClient> > m_hallIPServerMap;
    list<string>  m_invaildHallServerIps;
    MutexLock m_hallIPServerMapMutex;
    //==========game ip to TcpClient========
    map<string, std::shared_ptr<TcpClient> > m_gameIPServerMap;
    MutexLock m_gameIPServerMapMutex;

    //==========match ip to TcpClient========
    list<string>  m_matchServerIps;
    map<string, std::shared_ptr<TcpClient> > m_matchIPServerMap;
    MutexLock m_matchIPServerMapMutex;
    vector<string> m_matchServers;


    //与H5客户端的连接
    map<string, TcpConnectionWeakPtr> m_clientSessionWeakConnMap;
    MutexLock m_clientSessionWeakConnMaMutex;

    // hall connection --->client   userId->client  hall connection  for broadcast
    //与H5客户端的连接
    map<int64_t, TcpConnectionWeakPtr> m_clientHallUserIdWeaConnMap;
    MutexLock m_clientHallUserIdWeaConnMapMutex;
 


//    // game connection --->client   userId->client  game connection  for broadcast
//    map<int32_t, TcpConnectionWeakPtr> m_clientGameUserIdWeaConnMap;   //for broadcast
//    MutexLock m_clientGameUserIdWeaConnMapMutex;

private:
    EventLoop *m_loop;

private:
//    mt19937 m_rng;
    string m_pri_key;


public:
    WebSocketServer   m_WebSocketServer;

    // RocketMQ m_ConsumerBroadcast;
    // muduo::ThreadPool m_rocketmq_thread_pool;

    vector<shared_ptr<muduo::ThreadPool>> m_thread_pool_vec;

    shared_ptr<MyEventLoopThreadPool> m_hall_loop_thread_pool;
    shared_ptr<MyEventLoopThreadPool> m_game_loop_thread_pool;
    shared_ptr<MyEventLoopThreadPool> m_pay_loop_thread_pool;
    shared_ptr<EventLoopThread>       m_timer_thread;

    hash<string>            m_hashUserSession;

    AtomicInt32             m_connection_num;

    const static size_t kHeaderLen = sizeof(int16_t);


};


}

//#endif // PROXYSERVER_H


//    void SendShutDownMessageToClient(int64_t userId, string &session);
//    void SendShutDownDataAndClose(TcpConnectionkPtr& conn, vector<uint8_t> &data);
