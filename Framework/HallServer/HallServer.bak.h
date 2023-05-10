#ifndef HALLSERVER_H
#define HALLSERVER_H


#include <map>
#include <list>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"

#include "RedisClient/RedisClient.h"

#include "public/gameDefine.h"
#include "public/Globals.h"

#include "proto/HallServer.Message.pb.h"

#include "IPFinder.h"
#include "public/ThreadLocalSingletonMongoDBClient.h"

#include "RocketMQ/RocketMQ.h"



using namespace std;
using namespace muduo;
using namespace muduo::net;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;


extern int g_nConfigId;



namespace Landy
{

const int LOOP_THREAD_COUNT = 4;
const int WORK_THREAD_COUNT = 10;
const int SCORE_ORDER_THREAD_COUNT = 4;




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


class HallServer : public boost::noncopyable
{
public:
    typedef function<void(muduo::net::TcpConnectionWeakPtr&, uint8_t*, int, internal_prev_header*)> AccessCommandFunctor;
    typedef shared_ptr<muduo::net::Buffer> BufferPtr;

public:
    HallServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, int idleSeconds, string networkcarname);
    ~HallServer();

    void setThreadNum(int numThreads);
    bool loadKey();
    bool loadRSAPriKey();
    string decryDBKey(string password, int db_password_real_len);

    bool InitZookeeper(string ip);
    bool InitRedisCluster(string ip, string password);
    bool InitMongoDB(string url);
    bool startThreadPool(int threadCount);
    bool InitRocketMQ(string &ip);
    uint32_t RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe);
    uint32_t ConsumerClusterScoreOrderCallback(const vector<MQMessageExt> &mqe);
//    uint32_t ConsumerClusterScoreOrderSubCallback(const vector<MQMessageExt> &mqe);

    void start(int threadCount);

    void OnRepairGameServerNode();
    void ZookeeperConnectedHandler();
    void GetProxyServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                        const string &path, void *context);
    void GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                        const string &path, void *context);


private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);
  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime);

  void processRequest(muduo::net::TcpConnectionWeakPtr& conn,
                                   BufferPtr &buf,
                                   muduo::Timestamp receiveTime);
  void ProcessScoreOrderAdd(string &tag, string body);
  int32_t AddOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId);
  void ProcessScoreOrderSub(string &tag, string body);
  int32_t SubOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId);

private:
    void init();
    void threadInit();

    void RefreshGameInfo();

    bool sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data, internal_prev_header *internal_header, Header *commandHeader);
    void cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

    //login
    string get_random_account();
    void cmd_login_servers(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    bool GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId);
//    bool DeleteUserLoginInfoFromRedis(const string &session);
//    bool ResetExpiredUserLoginInfoFromRedis(const string &session);
    bool Login3SCheck(const string &session);
    bool DBAddUserLoginLog(int64_t userId, string &strIp, string &address, uint32_t status);
    bool DBUpdateUserLoginInfo(int64_t userId, string &strIp, int64_t days);
//    bool DBWriteRecordDailyUserOnline(shared_ptr<RedisClient> &redisClient, int nOSType, int nChannelId, int nPromoterId);

    void cmd_get_game_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_playing_game_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_game_server_message(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    string getRandomGameServer(uint32_t gameId, uint32_t roomId);

    void cmd_on_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    bool DBUpdateUserOnlineStatusInfo(int64_t userId, int32_t status);
    bool DBAddUserLogoutLog(int64_t userId, chrono::system_clock::time_point &startTime);

    void cmd_set_headId(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void cmd_set_nickname(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_play_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);


    void ChangeGameOnLineStatusOffline(int64_t userId);


    void reLoginHallServer(TcpConnectionWeakPtr &weakConn, internal_prev_header *internal_header);
    void WriteDailyBindMobileRecord(int64_t userId);


private:
    void OnLoadParameter();

    void OnSendNoticeMessage();
    bool IsMasterHallServer();
    void SendNoticeMessage();
    void OnInit();



public:
    void quit();

protected:
    bool CanSubMoney(int64_t userId);
//    bool GetUserScore(dword userid, SCORE& lUserScore, SCORE& lBankScore);
//    bool AddUserGemAndScore(TcpConnectionWeakPtr &weakConn, dword userid, SCORE lAddGem, SCORE lAddScore, bool bPurchase,
//                            int32_t *vipLevel=NULL, SCORE *allCharge=NULL, bool bUpdateToClient=false, internal_prev_header *internal_header=0);

private:


private:
//    void notifyRechargeScoreMessage(string msg);
//    void notifyExchangeScoreMessage(string msg);

//    void OnRechargeRefreshTimer();
//    void OnExchangeRefreshTimer();
//    void OnAddScoreRefreshTimer();
//    void OnSubScoreRefreshTimer();
private:
    const static size_t kHeaderLen = sizeof(int16_t);

private:
//    map<string, int>   m_account_userId_map;
//    mutable boost::shared_mutex m_account_userId_map_mutex;

private:
    ::HallServer::GetGameMessageResponse m_response;
    mutable boost::shared_mutex m_game_info_mutex;


private:
    shared_ptr<ZookeeperClient> m_zookeeperclient;
    string m_szNodePath, m_szNodeValue;
    string m_netCardName;

    shared_ptr<RedisClient>  m_redisPubSubClient;
    string m_redisIPPort;
    string m_redisPassword;

    string m_mongoDBServerAddr;

//    RocketMQ m_ConsumerBroadcast;
    RocketMQ m_ConsumerClusterScoreOrder;
//    RocketMQ m_ConsumerClusterScoreOrderSub;
    RocketMQ m_Producer;

	CIpFinder m_pIpFinder;

private:
    static map<int, AccessCommandFunctor>  m_functor_map;
    list<string>  m_game_servers;
    MutexLock     m_game_servers_mutex;

    list<string> m_proxy_servers;
    MutexLock    m_proxy_servers_mutex;

private:
    string    m_pri_key;
	MutexLock m_lock;

private:
    MutexLock   m_ChangeScoreMutexLock;

public:
    muduo::net::TcpServer m_server;
    muduo::ThreadPool m_thread_pool;
    AtomicInt32  m_connection_num;

    shared_ptr<EventLoopThread>     m_timer_thread;

//    muduo::ThreadPool m_score_order_thread_pool;
};


}


//    bool IpAddressToLocation(string ip, string strCountry, string strLocation);
//    bool DBRecordLoginPlaza(shared_ptr<RedisClient> &redisClient, int64_t userId);
//	int  DBCountGuestRewards(string ip);
//    void DBSetGuestRewardsCount(int64_t userId, string ip);

//    void LoadBlackUserId(shared_ptr<sql::Connection> &conn);
//    void LoadBlackIP(shared_ptr<sql::Connection> &conn);
//    void LoadUserAccountData(shared_ptr<sql::Connection> &conn);


//bool CheckUserIdValid(int64_t userId);
//bool LoginUserBlackListCheck(int32_t userId);
//bool LoginIPBlackListCheck(string& ip);

//    void cmd_get_dialog_notice_message(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
//    void cmd_get_playback_collect(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
//    void cmd_get_playback_detail(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

//    bool hasGameServerFromConfigIdIPPort(string ip);
//    string hasGameServerFromIP(string strIp);



#endif // HALLSERVER_H
