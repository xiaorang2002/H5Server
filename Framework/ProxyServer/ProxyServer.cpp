#include "ProxyServer.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <functional>

#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/progress.hpp>

#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/Logging.h"
#include "muduo/base/TimeZone.h"
#include "muduo/net/TcpClient.h"

#include "crypto/crypto.h"

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "public/GlobalFunc.h"
#include "public/gameDefine.h"
#include "public/SubNetIP.h"
#include "public/CryptoPublic.h"



namespace Landy
{

    using namespace std;
    using namespace placeholders;
    using namespace muduo;
    using namespace muduo::net;

    std::map<int, ProxyServer::AccessCommandFunctor> ProxyServer::m_functor_map;

    ProxyServer::ProxyServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, int idleSeconds, string networkCardname, int32_t hallIpIsFixed, string strHallIP) : 
    m_hall_loop_thread_pool(new MyEventLoopThreadPool("ProxyHallMessageEventLoopThread")), 
    m_game_loop_thread_pool(new MyEventLoopThreadPool("ProxyGameMessageEventLoopThread")), 
    m_pay_loop_thread_pool(new MyEventLoopThreadPool("ProxyPayMessageEventLoopThread")), 
    m_timer_thread(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "ProxyTimerEventLoopThread")), 
    m_loop(loop), m_WebSocketServer(loop, listenAddr, "ProxyServerWebSocketServer"),
    m_connection_buckets(idleSeconds), m_netCardName(networkCardname), m_hallIpIsFixed(hallIpIsFixed), m_strHallIP(strHallIP)
    {
        init();
        m_WebSocketServer.setTcpConnectionCallback(bind(&ProxyServer::onTcpConnection, this, std::placeholders::_1));
        m_WebSocketServer.setWebSocketConnectedCallback(bind(&ProxyServer::onWebSocketConnected, this, std::placeholders::_1, std::placeholders::_2));
        m_WebSocketServer.setWebSocketCloseCallback(bind(&ProxyServer::onWebSocketClosed, this, std::placeholders::_1));
        m_WebSocketServer.setWebSocketMessageCallback(
            bind(&ProxyServer::onWebSocketMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        m_connection_buckets.resize(idleSeconds);
        //    dumpConnectionBuckets();
    }

    ProxyServer::~ProxyServer()
    {
        m_hallServerIps.clear();
        m_gameServerIps.clear();

        m_functor_map.clear();

        m_redisPubSubClient->unsubscribe();
    }

    void ProxyServer::init()
    {
        m_hallServerIps.clear();
        m_gameServerIps.clear();
        m_functor_map.clear();
        
        int cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY << 8) | ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_REQ;
        m_functor_map[cmdId] = CALLBACK_2(ProxyServer::cmd_get_aes_key, this);
    }

    void ProxyServer::quit()
    {
        m_zookeeperclient->closeServer();

#if MULTI_THREAD
        m_thread_pool.stop();
#elif MUDUO_THREAD_POOL
        for (int i = 0; i < m_thread_pool_vec.size(); ++i)
            m_thread_pool_vec[i]->stop();
#endif

        // m_rocketmq_thread_pool.stop();

        if (m_zookeeperclient)
            m_zookeeperclient->closeServer();
        m_redisPubSubClient->unsubscribe();
    }

    //idle connections
    void ProxyServer::onTimer()
    {
        MutexLockGuard lock(m_connection_buckets_mutex);
        m_connection_buckets.push_back(Bucket());
        //    dumpConnectionBuckets();
    }

    void ProxyServer::dumpConnectionBuckets() const
    {
        LOG_INFO << "size = " << m_connection_buckets.size();
        //    int idx = 0;
        //    for (WeakConnectionList::const_iterator bucketI = m_connection_buckets.begin();
        //      bucketI != m_connection_buckets.end();
        //      ++bucketI, ++idx)
        //    {
        //        const Bucket& bucket = *bucketI;
        //        printf("[%d] len = %zd : ", idx, bucket.size());
        //        for (Bucket::const_iterator it = bucket.begin();
        //             it != bucket.end();
        //             ++it)
        //        {
        //            bool connectionDead = (*it)->m_weakConn.expired();
        //            printf("(%ld)%s, ",  it->use_count(),
        //                   connectionDead ? " DEAD" : "");
        //        }
        //        puts("");
        //    }
    }

    void ProxyServer::setThreadNum(int numThreads)
    {
        m_WebSocketServer.setThreadNum(numThreads);
    }

    bool ProxyServer::loadKey()
    {
        // string pubKey,priKey;
        // Landy::Crypto::generateRSAKey(pubKey,priKey);
        LOG_ERROR << "---RSA PRI Key,pubKey[" << TX_PUB_KEY <<"],priKey[" << TX_PRI_KEY <<"]" ;

        LOG_INFO << "Load RSA PRI Key...";
        //    boost::progress_timer timer("Load RSA PRI Key...");
        return loadRSAPriKey();
    }

    bool ProxyServer::loadRSAPriKey()
    {
        int len = 0;
        vector<unsigned char> vector_AESKey;
        vector_AESKey.resize(KEY.size() / 2);
        Landy::Crypto::HexStringToBuffer(KEY, (unsigned char *)&vector_AESKey[0], len);
        string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

        vector<unsigned char> vector_decrypt_pri_key_data;

        char buf[1024 * 3];
        memset(buf, 0, sizeof(buf));
        if (boost::filesystem::exists("prikey.bin"))
        {
            ifstream in("prikey.bin", ios::in | ios::binary);
            if (in.is_open())
            {
                in.seekg(0, ios::end);
                int size = in.tellg();
                in.seekg(0, ios::beg);
                in.read(buf, size);
                Landy::Crypto::aesDecrypt(strAESKey, (unsigned char *)buf, KEY_SIZE, vector_decrypt_pri_key_data);
                m_pri_key.assign(vector_decrypt_pri_key_data.begin(), vector_decrypt_pri_key_data.begin() + KEY_SIZE);
                if (m_pri_key == ORG_PRI_KEY)
                    LOG_INFO << "+++++++++++++++++++++OKOKOK+++++++++++++++++++++";
                else
                    LOG_INFO << "+++++++++++++++++++++Error+++++++++++++++++++++";
                in.close();
                return true;
            }
        }

        LOG_ERROR << "PRI KEY FILE not exists!";
        return false;
    }

    string ProxyServer::decryDBKey(string password, int db_password_real_len)
    {
        LOG_INFO << "Load DB Key...";
        //    boost::progress_timer timer("Load DB Key...");

        int len = 0;
        vector<unsigned char> vector_AESKey;
        vector_AESKey.resize(KEY.size() / 2);
        Landy::Crypto::HexStringToBuffer(KEY, (unsigned char *)&vector_AESKey[0], len);
        string strAESKey(vector_AESKey.begin(), vector_AESKey.end());

        len = 0;
        vector<unsigned char> dbKey;
        dbKey.resize(password.size() / 2);
        Landy::Crypto::HexStringToBuffer(password, (unsigned char *)&dbKey[0], len);

        vector<unsigned char> vector_decrypt_db_key_data;
        Landy::Crypto::aesDecrypt(strAESKey, (unsigned char *)&dbKey[0], dbKey.size(), vector_decrypt_db_key_data);
        string strDBKey;
        strDBKey.assign(vector_decrypt_db_key_data.begin(), vector_decrypt_db_key_data.begin() + db_password_real_len);
        return strDBKey;
    }

    void ProxyServer::start(int threadCount)
    {
        LOG_INFO << "starting " << threadCount << " work threads.";
        LOG_INFO << "start ProxyServer...";

#if MULTI_THREAD
        m_thread_pool.start(threadCount);
#elif MUDUO_THREAD_POOL
        string strName = "ThreadPool:";
        for (int i = 0; i < threadCount; ++i)
        {
            shared_ptr<muduo::ThreadPool> threadPool = make_shared<muduo::ThreadPool>(strName + to_string(i));
            threadPool->setThreadInitCallback(bind(&ProxyServer::threadInit, this));
            threadPool->start(1);
            m_thread_pool_vec.push_back(threadPool);
        }

#elif MY_THREAD_POOL

        for (int i = 0; i < threadCount; ++i)
        {
            m_proxy_thread_pool_vec.push_back(thread(&ProxyServer::processProxyRequest, this));
            m_hall_thread_pool_vec.push_back(thread(&ProxyServer::processHallRequest, this));
            m_game_thread_pool_vec.push_back(thread(&ProxyServer::processGameRequest, this));
        }

#endif

        m_timer_thread->getLoop()->runEvery(60 * 1.0, bind(&ProxyServer::onTimer, this));
        m_timer_thread->getLoop()->runEvery(5.0f, bind(&ProxyServer::OnRepairProxyServerNode, this));
        m_timer_thread->getLoop()->runEvery(1.0f * (5 + (2 + rand()% 3)), bind(&ProxyServer::checkHeartBeat, this));

        //    m_timer_thread->getLoop()->runEvery(60*5.0, bind(&ProxyServer::RefreshHallServers, this));
        //    m_timer_thread->getLoop()->runEvery(60*3.0, bind(&ProxyServer::RefreshGameServers, this));
        // 启动 Web Socket Server
        m_WebSocketServer.start();
    }

    void ProxyServer::checkHeartBeat()
    {
        MutexLockGuard lock(m_gameIPServerMapMutex);
        for (auto it = m_gameIPServerMap.begin();it != m_gameIPServerMap.end(); ++it)
        {
            // LOG_INFO << "===Keep Ping,ipport[" << it->first << "]==";
            weak_ptr<TcpClient> gameTcpClient(it->second);
            TcpConnectionPtr connptr = it->second->connection();
            if (connptr && connptr->connected()) //在线状态判断
            {
                check_keep_alive_ping(gameTcpClient);
            }
            else
            {
                if(connptr)
                    LOG_INFO << "====在线状态判断[" << connptr->connected() << "]====,ipport[" << it->first << "]==";
                else
                    LOG_INFO << "====在线状态判断connptr Null===,ipport[" << it->first << "]==";
            }
        }
        
    }

    bool ProxyServer::startThreadPool(int threadCount)
    {
        LOG_INFO << "starting ThreadPool" << threadCount << " work threads.";

        m_hall_loop_thread_pool->setThreadNum(2);
        m_hall_loop_thread_pool->start();

        m_game_loop_thread_pool->setThreadNum(8);
        m_game_loop_thread_pool->start();

        m_pay_loop_thread_pool->setThreadNum(2);
        m_pay_loop_thread_pool->start();

        // m_rocketmq_thread_pool.setThreadInitCallback(bind(&ProxyServer::threadInit, this));
        // m_rocketmq_thread_pool.start(threadCount);

        m_timer_thread->startLoop();

        return true;
    }

    void ProxyServer::threadInit()
    {
        if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
        {
            LOG_ERROR << "RedisClient Connection ERROR!";
            abort();
            return;
        }
    }

    bool ProxyServer::InitZookeeper(string ip)
    {
        LOG_INFO << "Init Zoo keeper...";
        //    progress_timer timer("Init Zoo keeper...");
        m_zookeeperclient.reset(new ZookeeperClient(ip));
        //    shared_ptr<ZookeeperClient> zookeeperclient(new ZookeeperClient("192.168.100.160:2181"));
        //    m_zookeeperclient = zookeeperclient;
        m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&ProxyServer::ZookeeperConnectedHandler, this));
        if (m_zookeeperclient->connectServer())
        {
            return true;
        }
        else
        {
            LOG_ERROR << "zookeeperclient->connectServer error";
            abort();
            return false;
        }
    }

    void ProxyServer::ZookeeperConnectedHandler()
    {
        LOG_INFO << "************ProxyServer::Zookeeper Connected Handler**************";
        if (ZNONODE == m_zookeeperclient->existsNode("/GAME"))
            m_zookeeperclient->createNode("/GAME", "Landy");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/ProxyServers"))
            m_zookeeperclient->createNode("/GAME/ProxyServers", "ProxyServers");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServers"))
            m_zookeeperclient->createNode("/GAME/HallServers", "HallServers");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServersInvaild"))
            m_zookeeperclient->createNode("/GAME/HallServersInvaild", "HallServersInvaild");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServers"))
            m_zookeeperclient->createNode("/GAME/GameServers", "GameServers");

        vector<string> vec;
        boost::algorithm::split(vec, m_WebSocketServer.m_TcpServer.ipPort(), boost::is_any_of(":"));
        string ip;
        GlobalFunc::getIP(m_netCardName, ip);

        m_szNodeValue = ip + ":" + vec[1];
        m_szNodePath = "/GAME/ProxyServers/" + m_szNodeValue;
        m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);

        // 1，大厅服节点变化回调函数
        GetChildrenWatcherHandler getHallServersChildrenWatcherHandler = CALLBACK_5(ProxyServer::GetHallServersChildrenWatcherHandler, this);

        vector<string> hallServers;
        if (ZOK == m_zookeeperclient->getClildren("/GAME/HallServers", hallServers, getHallServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_hallServerIps_mutex);
            m_hallServerIps.assign(hallServers.begin(), hallServers.end());
        }

        for (string ip : hallServers)
        {
            LOG_INFO << " >>> Init GET HallServers :" << ip;
        }

        // 2，无效大厅服节点变化回调函数
        GetChildrenWatcherHandler getInvaildHallServersChildrenWatcherHandler = CALLBACK_5(ProxyServer::GetInvaildHallServersChildrenWatcherHandler, this);
        hallServers.clear();
        if (ZOK == m_zookeeperclient->getClildren("/GAME/HallServersInvaild", hallServers, getInvaildHallServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_hallServerIps_mutex);
            m_invaildHallServerIps.assign(hallServers.begin(), hallServers.end());
        }

        // 3，游戏服节点变化回调函数
        GetChildrenWatcherHandler getGameServersChildrenWatcherHandler = CALLBACK_5(ProxyServer::GetGameServersChildrenWatcherHandler, this);

        vector<string> gameServers;
        if (ZOK == m_zookeeperclient->getClildren("/GAME/GameServers", gameServers, getGameServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_gameServerIps_mutex);
            m_gameServers.assign(gameServers.begin(), gameServers.end());
            m_gameServerIps.assign(gameServers.begin(), gameServers.end());
        }

        // init to dump the game servers.
        for (string ip : gameServers)
        {
            LOG_INFO << " >>> Init GET GameServers :" << ip;
        }

        // 4，比赛服节点变化回调函数
        GetChildrenWatcherHandler getMatchServersChildrenWatcherHandler = CALLBACK_5(ProxyServer::GetMatchServersChildrenWatcherHandler, this);

        vector<string> matchServers;
        if (ZOK == m_zookeeperclient->getClildren("/GAME/MatchServers", matchServers, getMatchServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_gameServerIps_mutex);
            m_matchServers.assign(matchServers.begin(), matchServers.end());
            for (auto ip : matchServers)
                m_gameServerIps.push_back(ip);
        }

        // init to dump the game servers.
        for (string ip : matchServers)
            LOG_INFO << " >>> Init GET matchServers :" << ip;

        LOG_DEBUG << " >>> 开始处理支付服 ==========================>>>>";
         // 5，支付服节点变化回调函数
        vector<string> payServers;
        if (ZOK == m_zookeeperclient->getClildren(ZK_ROOT_NODE + PAY_SERVER, payServers, CALLBACK_5(ProxyServer::GetPayServersChildrenWatcherHandler, this), this))
        {
            MutexLockGuard lock(m_payServerIps_mutex);
            m_payServerIps.assign(payServers.begin(), payServers.end());
        }

        for (string ip : payServers)
            LOG_INFO << " >>> Init GET PayServers :" << ip;

        // 2，无效服节点变化回调函数
        payServers.clear();
        if (ZOK == m_zookeeperclient->getClildren(ZK_ROOT_NODE + PAY_SERVER_INVAILD, payServers, CALLBACK_5(ProxyServer::GetInvaildPayServersChildrenWatcherHandler, this), this))
        {
            MutexLockGuard lock(m_payServerIps_mutex);
            m_invaildPayServerIps.assign(payServers.begin(), payServers.end());
        }

        for (string ip : m_invaildPayServerIps)
            LOG_INFO << " >>> Init GET PayServers :" << ip;

        LOG_DEBUG << " >>> 结束处理支付服 ==========================>>>>";

        // 连接所有大厅
        startAllHallServerConnection();
        // 连接所有游戏服
        startAllGameServerConnection(); 
        // 连接所有支付服
        startAllPayServerConnection();

        LOG_INFO << " >>> zookeeper every 3.0 second to repair node path:" << m_szNodePath << ", value:" << m_szNodeValue;
    }

    void ProxyServer::startAllPayServerConnection()
    {
        MutexLockGuard lock(m_payServerIps_mutex);
        for (string ip : m_payServerIps)
        {
            startPayServerConnection(ip);
        }
    }
    
    void ProxyServer::startPayServerConnection(string ip)
    {
        LOG_INFO << " >>> "<< __FUNCTION__ <<" to ip:" << ip;

        bool bFind = false;
        {
            MutexLockGuard lock(m_payIPServerMapMutex);
            auto it = m_payIPServerMap.find(ip);
            if (m_payIPServerMap.end() == it)
            {
                bFind = false;
            }
            else
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                    bFind = true;
            }
        }

        if (!bFind)
        {
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));

            // 2，建立网关与支付服连接
            muduo::net::InetAddress serverAddr(vec[0], stoi(vec[1]));
            std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_pay_loop_thread_pool->getNextLoop(), serverAddr, ip));

            tcpClient->setConnectionCallback(CALLBACK_1(ProxyServer::onPayServerConnection, this));
            tcpClient->setMessageCallback(CALLBACK_3(ProxyServer::onPayServerMessage, this));
            tcpClient->enableRetry();
            tcpClient->connect();

            // 3，缓存支付服连接IP
            MutexLockGuard lock(m_payIPServerMapMutex);
            m_payIPServerMap[ip] = tcpClient;
        }
    }

    void ProxyServer::ProcessPayIPServer(vector<string> &payServers)
    {
        vector<string> addIP;  //新增加IP
        set<string> oldIPSet; //旧IP集合
        set<string> newIPSet; //新IP集合
        {
            MutexLockGuard lock(m_payServerIps_mutex);
            for (string ip : m_payServerIps)
            {
                oldIPSet.emplace(ip);
            }
        }

        for (string ip : payServers){
            newIPSet.emplace(ip);
        }
       
        // 求出老旧过时的IP
        vector<string> diffIPS(oldIPSet.size());
        vector<string>::iterator it;
        it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());

        {
            MutexLockGuard lock(m_payIPServerMapMutex);
            for (string ip : diffIPS)
            {
                LOG_INFO << "---different old PayServer IP:" << ip;
                
                auto it = m_payIPServerMap.find(ip);
                if (m_payIPServerMap.end() != it)
                {
                    std::shared_ptr<TcpClient> tcpClient(it->second);
                    if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                    {
                        addIP.push_back(ip);
                        LOG_ERROR << "---addIP PayServer IP:" << ip;
                    }
                    else
                    {
                        LOG_WARN << "---ProcessPayIPServer delete PayServer IP:" << ip;
                        m_payIPServerMap.erase(it);
                    }
                }
            }
        }

        // 求出新增加的IP
        diffIPS.clear();
        diffIPS.resize(newIPSet.size());
        it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());
        for (string ip : diffIPS)
        {
            LOG_INFO << "---different new PayServer IP:" << ip;
            startPayServerConnection(ip);
        }

         //这是正序输出：
        /*
        int32_t idx = 0;
        for (list<string>::iterator it = m_payServerIps.begin(); it != m_payServerIps.end(); ++it){
            LOG_INFO << "---前 idx[" << ++idx <<"]m_pay Server Ips[" << *it << "]";
        }
        */

        // 复制元素
        MutexLockGuard lock(m_payServerIps_mutex);
        m_payServerIps.assign(payServers.begin(), payServers.end());

        // 额外添加
        for (int i = 0; i < addIP.size(); ++i)
            m_payServerIps.push_back(addIP[i]);

        /*
        idx = 0;
        for (list<string>::iterator it = m_payServerIps.begin(); it != m_payServerIps.end(); ++it){
            LOG_INFO << "---后 idx[" << ++idx <<"]m_pay Server Ips[" << *it << "]";
        }
        */
    }

    
    void ProxyServer::GetPayServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                           const string &path, void *context)
    {
        LOG_INFO << " >>> zookeeper pay callback, type:" << type << ", state:" << state;

        vector<string> payServers;
        string payPath = ZK_ROOT_NODE + PAY_SERVER;//"/GAME/PayServers";
        if (ZOK == m_zookeeperclient->getClildren(payPath, payServers, CALLBACK_5(ProxyServer::GetPayServersChildrenWatcherHandler, this), this))
        {
            ProcessPayIPServer(payServers);
        }

        for (string ip : payServers)
        {
            LOG_INFO << " pay ip " << ip;
        }

        LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
    }

    void ProxyServer::GetInvaildPayServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
    {
        LOG_ERROR << " >>> zookeeper 无效支付服 callback, type:" << type << ", state:" << state;

        vector<string> payServers;
        string payPath = ZK_ROOT_NODE + PAY_SERVER_INVAILD;// "/GAME/PayServersInvaild";
        if (ZOK == m_zookeeperclient->getClildren(payPath, payServers, CALLBACK_5(ProxyServer::GetInvaildPayServersChildrenWatcherHandler, this), this))
        {
            m_invaildPayServerIps.assign(payServers.begin(), payServers.end());
        }

        for (string ip : m_invaildPayServerIps)
            LOG_INFO << " 支付服无效服IP :" << ip;

        LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
    }


    void ProxyServer::startAllHallServerConnection()
    {
        MutexLockGuard lock(m_hallServerIps_mutex);
        for (string ip : m_hallServerIps)
        {
            startHallServerConnection(ip);
        }
    }

    void ProxyServer::startHallServerConnection(string ip)
    {
        LOG_INFO << " >>> startHallServerConnection to ip:" << ip;

        bool bFind = false;

        {
            MutexLockGuard lock(m_hallIPServerMapMutex);
            auto it = m_hallIPServerMap.find(ip);
            if (m_hallIPServerMap.end() == it)
            {
                bFind = false;
            }
            else
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                    bFind = true;
            }
        }

        if (!bFind)
        {
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));

            // 2，建立网关与大厅连接
            muduo::net::InetAddress serverAddr(vec[0], stoi(vec[1]));
            std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_hall_loop_thread_pool->getNextLoop(), serverAddr, ip));

            tcpClient->setConnectionCallback(bind(&ProxyServer::onHallServerConnection, this, std::placeholders::_1));
            tcpClient->setMessageCallback(bind(&ProxyServer::onHallServerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            tcpClient->enableRetry();
            tcpClient->connect();

            // 3，缓存大厅连接IP
            MutexLockGuard lock(m_hallIPServerMapMutex);
            m_hallIPServerMap[ip] = tcpClient;
        }
    }

    void ProxyServer::ProcessHallIPServer(vector<string> &hallServers)
    {
        vector<string> addIP;  //新增加IP
        set<string> oldIPSet; //旧IP集合
        set<string> newIPSet; //新IP集合
        {
            MutexLockGuard lock(m_hallServerIps_mutex);
            for (string ip : m_hallServerIps)
            {
                oldIPSet.emplace(ip);
            }
        }

        for (string ip : hallServers){
            newIPSet.emplace(ip);
        }
       
        // 求出老旧过时的IP
        vector<string> diffIPS(oldIPSet.size());
        vector<string>::iterator it;
        it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());

        {
            MutexLockGuard lock(m_hallIPServerMapMutex);
            for (string ip : diffIPS)
            {
                LOG_INFO << "---different old HallServer IP:" << ip;
                
                auto it = m_hallIPServerMap.find(ip);
                if (m_hallIPServerMap.end() != it)
                {
                    std::shared_ptr<TcpClient> tcpClient(it->second);
                    if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                    {
                        addIP.push_back(ip);
                        //                    tcpClient->disconnect();
                        LOG_ERROR << "---addIP HallServer IP:" << ip;
                    }
                    else
                    {
                        LOG_WARN << "---ProcessHallIPServer delete HallServer IP:" << ip;
                        m_hallIPServerMap.erase(it);
                    }
                }
            }
        }

        // 求出新增加的IP
        diffIPS.clear();
        diffIPS.resize(newIPSet.size());
        it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());
        for (string ip : diffIPS)
        {
            LOG_INFO << "---different new HallServer IP:" << ip;
            startHallServerConnection(ip);
        }

         //这是正序输出：
        /*
        int32_t idx = 0;
        for (list<string>::iterator it = m_hallServerIps.begin(); it != m_hallServerIps.end(); ++it){
            LOG_INFO << "---前 idx[" << ++idx <<"]m_hall Server Ips[" << *it << "]";
        }
        */

        // 复制元素
        MutexLockGuard lock(m_hallServerIps_mutex);
        m_hallServerIps.assign(hallServers.begin(), hallServers.end());

        // 额外添加
        for (int i = 0; i < addIP.size(); ++i)
            m_hallServerIps.push_back(addIP[i]);

        /*
        idx = 0;
        for (list<string>::iterator it = m_hallServerIps.begin(); it != m_hallServerIps.end(); ++it){
            LOG_INFO << "---后 idx[" << ++idx <<"]m_hall Server Ips[" << *it << "]";
        }
        */
    }

    void ProxyServer::GetHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                           const string &path, void *context)
    {
        LOG_INFO << " >>> zookeeper callback, type:" << type << ", state:" << state;

        GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&ProxyServer::GetHallServersChildrenWatcherHandler, this,
                                                                   placeholders::_1, placeholders::_2,
                                                                   placeholders::_3, placeholders::_4,
                                                                   placeholders::_5);

        vector<string> hallServers;
        string hallPath = "/GAME/HallServers";
        if (ZOK == m_zookeeperclient->getClildren(hallPath, hallServers, getChildrenWatcherHandler, this))
        {
            ProcessHallIPServer(hallServers);
        }

        // for (string ip : hallServers)
        // {
        //     LOG_INFO << " GetHallServersChildrenWatcherHandler :" << ip;
        // }

        // LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        // if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        // else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        // else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        // else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        // else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        // else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
    }

    void ProxyServer::GetInvaildHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
    {
        LOG_INFO << " >>> zookeeper callback, type:" << type << ", state:" << state;

        GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&ProxyServer::GetInvaildHallServersChildrenWatcherHandler, this,
                                                                   placeholders::_1, placeholders::_2,
                                                                   placeholders::_3, placeholders::_4,
                                                                   placeholders::_5);

        vector<string> hallServers;
        string hallPath = "/GAME/HallServersInvaild";
        if (ZOK == m_zookeeperclient->getClildren(hallPath, hallServers, getChildrenWatcherHandler, this))
        {
            m_invaildHallServerIps.assign(hallServers.begin(), hallServers.end());
        }

        // for (string ip : m_invaildHallServerIps)
        // {
        //     LOG_INFO << " GetInvaildHallServersChildrenWatcherHandler :" << ip;
        // }

        // LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        // if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        // else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        // else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        // else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        // else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        // else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
    }

    void ProxyServer::RefreshHallServers()
    {
        LOG_INFO << " " << __FUNCTION__;

        vector<string> hallServers;
        string hallPath = "/GAME/HallServers";
        if (ZOK == m_zookeeperclient->getClildrenNoWatch(hallPath, hallServers))
        {
            ProcessHallIPServer(hallServers);
        }

        // for (string ip : hallServers)
        // {
        //     LOG_INFO << " GetHallServersChildrenWatcherHandler :" << ip;
        // }
    }

    void ProxyServer::startAllGameServerConnection()
    {
        LOG_INFO << "---连接所有游戏服---";
        MutexLockGuard lock(m_gameServerIps_mutex);
        for (string ip : m_gameServerIps)
        {
            startGameServerConnection(ip);
        }
    }

    // 建立网关与游戏服连接
    void ProxyServer::startGameServerConnection(string ip)
    {
        LOG_INFO << "---连接游戏服IP[" << ip << "]---";
        bool bFind = false;
        {
            MutexLockGuard lock(m_gameIPServerMapMutex);
            auto it = m_gameIPServerMap.find(ip);
            if (m_gameIPServerMap.end() != it)
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                    bFind = true;
                else
                    LOG_INFO << "---gameIpMap已经缓存连接,但缓存连接已经失效,connected is " << (tcpClient->connection()->connected() ? "UP" : "DOWN");
            }
        }

        if (!bFind)
        {
           LOG_INFO << "---gameIpMap没缓存["<< ip <<"],须新建连接---";

            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));

            muduo::net::InetAddress serverAddr(vec[1], stoi(vec[2]));
            std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_game_loop_thread_pool->getNextLoop(), serverAddr, ip));

            tcpClient->setConnectionCallback(bind(&ProxyServer::onGameServerConnection, this, std::placeholders::_1));
            tcpClient->setMessageCallback(bind(&ProxyServer::onGameServerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            tcpClient->enableRetry();
            tcpClient->connect();

            {
                // 缓存大厅到游戏连接
                MutexLockGuard lock(m_gameIPServerMapMutex);
                m_gameIPServerMap[ip] = tcpClient;
            }
            
        }
    }

    // 网关连接游戏服
    void ProxyServer::ProcessGameIPServer(vector<string> &gameServers)
    {
        vector<string> addIP;       //新增加IP
        set<string> oldIPSet;       //旧IP集合
        set<string> newIPSet;       //新IP集合
        {
            MutexLockGuard lock(m_gameServerIps_mutex);
            for (string ip : m_gameServerIps){
                oldIPSet.emplace(ip);
            }
        }

        for (string ip : gameServers){
            newIPSet.emplace(ip);
        }

        // 求出老旧过时的IP
        vector<string> diffIPS(oldIPSet.size());
        vector<string>::iterator it;
        it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());
        {
            MutexLockGuard lock(m_gameIPServerMapMutex);
            for (string ip : diffIPS)
            {
                LOG_INFO << "---different old GameServer IP:" << ip;

                auto it = m_gameIPServerMap.find(ip);
                if (m_gameIPServerMap.end() != it)
                {
                    std::shared_ptr<TcpClient> tcpClient(it->second);
                    if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected()){
                        addIP.push_back(ip);

                        LOG_ERROR << "---addIP GameServer IP:" << ip;
                    }
                    else{
                        LOG_WARN << "---ProcessGameIPServer delete GameServer IP:" << ip;
                        m_gameIPServerMap.erase(it);
                    }
                }
            }
        }

        // 求出新增加的IP
        diffIPS.clear();
        diffIPS.resize(newIPSet.size());
        it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
        diffIPS.resize(it - diffIPS.begin());
        for (string ip : diffIPS)
        {
            LOG_INFO << "---different new GameServer IP:" << ip;
            startGameServerConnection(ip);
        }

        //这是正序输出：
        /*
        int32_t idx = 0;
        for (list<string>::iterator it = m_gameServerIps.begin(); it != m_gameServerIps.end(); ++it){
            LOG_INFO << "---前 idx[" << ++idx <<"]m_game Server Ips[" << *it << "]";
        }
        */

        // 复制元素
        MutexLockGuard lock(m_gameServerIps_mutex);
        m_gameServerIps.assign(gameServers.begin(), gameServers.end());
        // 额外添加
        for (int i = 0; i < addIP.size(); ++i)
            m_gameServerIps.push_back(addIP[i]);

        /*
        idx = 0;
        for (list<string>::iterator it = m_gameServerIps.begin(); it != m_gameServerIps.end(); ++it){
            LOG_INFO << "---后 idx[" << ++idx <<"]m_game Server Ips[" << *it << "]";
        }
        */

    }

    void ProxyServer::GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                           const string &path, void *context)
    {
        LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        
        if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;

        // ZK回调
        GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&ProxyServer::GetGameServersChildrenWatcherHandler, this,
                                                                   placeholders::_1, placeholders::_2,
                                                                   placeholders::_3, placeholders::_4,
                                                                   placeholders::_5);

        vector<string> Servers;
        m_gameServers.clear();
        string gamePath = "/GAME/GameServers";
        if (ZOK == m_zookeeperclient->getClildren(gamePath, Servers, getChildrenWatcherHandler, this))
        {
            // 存放最新游戏服信息
            m_gameServers.assign(Servers.begin(), Servers.end());
            // 增加比赛服信息
            for (auto ip : m_matchServers)
                Servers.push_back(ip);
            // 连接新游戏服
            ProcessGameIPServer(Servers);
        }

        LOG_INFO << "---m_gameServers size[" << m_gameServers.size() <<"],all size[" << Servers.size() <<"]";

        // for (string ip : m_gameServers)
        //     LOG_INFO << " getGameServersChildrenWatcherHandler :" << ip;
    }

    void ProxyServer::GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                            const string &path, void *context)
    {
        LOG_INFO << __FUNCTION__ << " ---ZK回调,type:" << type;
        if (type == (int)ZOO_CREATED_EVENT) LOG_INFO << " ZOO_CREATED_EVENT: " << ZOO_CREATED_EVENT;
        else if (type == (int)ZOO_DELETED_EVENT) LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT; 
        else if (type == (int)ZOO_CHANGED_EVENT) LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT; 
        else if (type == (int)ZOO_CHILD_EVENT)  LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
        else if (type == (int)ZOO_SESSION_EVENT)  LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
        else if (type == (int)ZOO_NOTWATCHING_EVENT)  LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;

        GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&ProxyServer::GetMatchServersChildrenWatcherHandler, this,
                                                                   placeholders::_1, placeholders::_2,
                                                                   placeholders::_3, placeholders::_4,
                                                                   placeholders::_5);

        vector<string> Servers;
        m_matchServers.clear();
        string gamePath = "/GAME/MatchServers";
        if (ZOK == m_zookeeperclient->getClildren(gamePath, Servers, getChildrenWatcherHandler, this))
        {
            // 存放最新比赛服信息
            m_matchServers.assign(Servers.begin(), Servers.end());

            // 增加游戏服信息
            for (auto ip : m_gameServers)
                Servers.push_back(ip);

            // 连接新游戏服
            ProcessGameIPServer(Servers);
        }

        LOG_INFO << "---m_Match Servers size[" << m_matchServers.size() <<"],all size[" << Servers.size() <<"]";

        // for (string ip : m_matchServers)
        //     LOG_INFO << " GetMatchServersChildrenWatcherHandler :" << ip;
    }

    void ProxyServer::RefreshGameServers()
    {
        LOG_INFO << " " << __FUNCTION__;

        vector<string> gameServers;
        string gamePath = "/GAME/GameServers";
        if (ZOK == m_zookeeperclient->getClildrenNoWatch(gamePath, gameServers))
        {
            ProcessGameIPServer(gameServers);
        }

        for (string ip : gameServers)
        {
            LOG_INFO << " getGameServersChildrenWatcherHandler :" << ip;
        }
    }

    void ProxyServer::OnRepairProxyServerNode()
    {
        if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath))
        {
            LOG_DEBUG << " >>> OnRepairProxyServerNode repair zookeeper node path:" << m_szNodePath << ", value:" << m_szNodeValue;
            m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
        }
    }

    bool ProxyServer::InitRedisCluster(string ip, string password)
    {
        LOG_INFO << "InitRedisCluster...";
        //    boost::progress_timer timer("InitRedisCluster...");

        //==============redis subscribe client
        m_redisPubSubClient.reset(new RedisClient());
        if (!m_redisPubSubClient->initRedisCluster(ip, password))
        {
            LOG_ERROR << "m_redisPubSubClient connection error";
            return false;
        }
        m_redisPassword = password;

        //    m_redisPubSubClient->subscribeRechargeScoreToProxyMessage(bind(&ProxyServer::notifyRechargeScoreToProxyMessage, this, placeholders::_1));
        //    m_redisPubSubClient->subscribeExchangeScoreToProxyMessage(bind(&ProxyServer::notifyExchangeScoreToProxyMessage, this, placeholders::_1));
        m_redisPubSubClient->subscribeUserLoginMessage(bind(&ProxyServer::notifyUserLoginMessage, this, std::placeholders::_1));
        m_redisPubSubClient->subsreibeOrderScoreMessage(bind(&ProxyServer::notifyUserOrderScoreMessage, this, std::placeholders::_1));
        // m_redisPubSubClient->subscribeUserKillBossMessage(bind(&ProxyServer::notifyUserKillBossMessage, this, placeholders::_1));
        //    m_redisPubSubClient->subscribeNewChatMessage(bind(&ProxyServer::notifyNewChatMessage, this, _1));
        //    m_redisPubSubClient->subscribeNewMailMessage(bind(&ProxyServer::notifyNewMailMessage, this, _1));
        //    m_redisPubSubClient->subscribeNoticeMessage(bind(&ProxyServer::notifyNoticeMessage, this, placeholders::_1));

        // 公告消息
        m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_public_notice, [&](string msg)
        {
            LOG_INFO << "公告消息 "<< msg;
            Json::Reader reader;
            Json::Value root;

            try
            {
                if (reader.parse(msg, root))
                {
                    int size = root.size();
                    for (int i = 0; i < size; ++i)
                    {
                        Json::Value value = root[i];
                        notifyPublicNoticeMessage(value);
                    }
                }
                else
                {
                    LOG_ERROR << "Json数据解析有误";
                }
            }
            catch (exception excep)
            {
                LOG_ERROR << __FUNCTION__ << " " << excep.what();
            }
        });
        // 踢人消息
        m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_kick_out_notice, [&](string msg) {
            // 踢人消息
            LOG_WARN << "踢人消息 " << msg;
            string tagStr;
            ProcessKickUser(tagStr, msg);
        });
        // 跑马灯通告消息
        m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_marquee, CALLBACK_1(ProxyServer::notifyUserKillBossMessage, this));
        //订单幸运转盘消息
        m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_luckyGame, [&](string msg) {
            m_timer_thread->getLoop()->runAfter(10, bind(&ProxyServer::luckyGamePushMsg, this, msg));
        });
        // 启动redis线程
        m_redisPubSubClient->startSubThread();
 

        //===============redis client=============
        m_redisIPPort = ip;
        return true;
    }

    // 大厅公共消息
    void ProxyServer::sendHallMessage(string title, string msg, int32_t agentid, eProxyPublicMsgType msgtype)
    {
        // LOG_WARN << title <<" msg "<< msg;

        if (!msg.empty())
        {
            ::ProxyServer::Message::NotifyNoticeMessageFromProxyServerMessage response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->set_sign(PROTO_BUF_SIGN);
            //
            response.set_title(title.c_str());
            response.set_message(msg);
            response.add_agentid(agentid);
            response.set_msgtype((int32_t)msgtype); //(int32_t)eProxyPublicMsgType::marqueeMsg);

            size_t len = response.ByteSize();
            vector<uint8_t> data(len + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(Header)], len))
            {
                Header *commandSendHeader = (Header *)(&data[0]);
                commandSendHeader->ver = PROTO_VER;
                commandSendHeader->sign = HEADER_SIGN;
                commandSendHeader->reserve = 0;
                commandSendHeader->reqId = 0;
                commandSendHeader->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
                commandSendHeader->subId = ::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY;
                commandSendHeader->encType = PUBENC__PROTOBUF_NONE;

                commandSendHeader->len = len + sizeof(Header);
                commandSendHeader->realSize = len;
                commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[4], commandSendHeader->len - 4);

                {
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    for (auto &it : m_clientHallUserIdWeaConnMap)
                    {
                        TcpConnectionPtr conn = it.second.lock();
                        if (conn)
                            m_WebSocketServer.SendData(conn, data);
                    }
                }
            }
        }
    }

    // 跑马灯消息
    void ProxyServer::notifyUserKillBossMessage(string msg)
    {
        // LOG_WARN << "跑马灯消息:" << msg;
        sendHallMessage("跑马灯消息", msg, 0, eProxyPublicMsgType::marqueeMsg);
    }

    // 免费送金广播消息
    void ProxyServer::luckyGamePushMsg(string msg)
    {
        // LOG_WARN << "广播免费送金消息:"<< msg;
        sendHallMessage("免费送金消息", msg, 0, eProxyPublicMsgType::lkGameMsg);
    }

    bool ProxyServer::InitRocketMQ(string &ip)
    {
        LOG_INFO << " " << __FILE__ << " " << __FUNCTION__;

        // m_ConsumerBroadcast.SetMesageCallback(std::bind(&ProxyServer::RocketMQBroadcastCallback, this, std::placeholders::_1));
        bool bInitOK = false;

#if 1
        try
        {
            //m_ConsumerBroadcast.Start(ip, "TianXia", "PublicNotify", "*", MessageModel::BROADCASTING);
            // m_ConsumerBroadcast.Start(ip, "TianXiaNotify", "PublicNotify", "*", MessageModel::BROADCASTING);

            //        for(int i = 10; i < 100; ++i)
            //        {
            //            string data = to_string(i);
            //            m_Producer.SendData("Cluster1", "tag1", "userId", data);
            //        }

            bInitOK = true;
        }
        catch (exception &e)
        {
            LOG_ERROR << "Init RocketMQ Error:" << e.what();
        }
        //=======================================
#else
        string strGroupName = "GroupName";
        string strTopic = "Topic";
        string strTag = "Tag1";
        string strKey = "KEY";
        string strData = "123456";
        try
        {
            m_Consumer.Start(ip, strGroupName, strTopic, strTag, MessageModel::BROADCASTING);
            m_Producer.Start(ip, strGroupName);

            m_Producer.SendData(strTopic, strTag, strKey, strData);

            bInitOK = true;
        }
        catch (exception &e)
        {
            LOG_ERROR << "Init RocketMQ Error:" << e.what();
        }

#endif

        return bInitOK;
    }

    // uint32_t ProxyServer::RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe)
    // {
    //     for (MQMessageExt mqItem : mqe)
    //     {
    //         LOG_ERROR << "RocketMQBroadcastCallback:" << mqItem.getBody();
    //         string tag = mqItem.getTags();
    //         string key = mqItem.getKeys();
    //         string body = mqItem.getBody();
    //         if (tag == "PublicNotice")
    //             m_rocketmq_thread_pool.run(bind(&ProxyServer::ProcessPublicNotice, this, tag, body));
    //         else if (tag == "KickUserOffline")
    //         {
    //             m_rocketmq_thread_pool.run(bind(&ProxyServer::ProcessKickUser, this, tag, body));
    //         }
    //         else if (tag == "")
    //         {
    //         }
    //     }
    //     return 0;
    // }

    void ProxyServer::ProcessPublicNotice(string &tag, string body)
    {
        string title = "", publicNotice = "";

        Json::Reader reader;
        Json::Value root;
        if (reader.parse(body, root))
        {
            int size = root.size();
            for (int i = 0; i < size; ++i)
            {
                Json::Value value = root[i];
                notifyPublicNoticeMessage(value);
            }
        }
    }

    void ProxyServer::ProcessKickUser(string &tag, string body)
    {
        Json::Reader reader;
        Json::Value root;
        int64_t userid = 0;

        try
        {
            if (reader.parse(body, root))
            {
                userid = root["userid"].asInt64();
                if (userid == 0)
                    return;

                LOG_WARN << "Kick User userid " << userid;
            }
        }
        catch (exception excep)
        {
            LOG_ERROR << __FUNCTION__ << " " << excep.what();
        }

        if (m_clientHallUserIdWeaConnMap.count(userid))
        {
            TcpConnectionWeakPtr weakPtr;
            {
                MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                auto it = m_clientHallUserIdWeaConnMap.find(userid);
                if (it != m_clientHallUserIdWeaConnMap.end())
                {
                    weakPtr = it->second;
                    LOG_WARN << "find Kick User userid " << userid;
                }
            }
            TcpConnectionPtr clientPtr(weakPtr.lock());
            if (clientPtr)
            {
                LOG_WARN << "User clientPtr OK " << userid;

                vector<uint8_t> data;
                GetShutDownUserClientMessage(userid, data, 1);
                m_WebSocketServer.SendData(clientPtr, data);
                EntryPtr entry(boost::any_cast<EntryPtr>(clientPtr->getContext()));
                if (likely(entry))
                {
                    entry->setUserLeftCloseOnly(1);
                }

                clientPtr->forceCloseWithDelay(0.1); //如果是0.0可能导致先释放链接
            }
        }
    }

    void ProxyServer::notifyPublicNoticeMessage(const Json::Value &value)
    {

        // LOG_DEBUG << " >>> notify Public Notice Message title=" << value["title"].asString() << " <<<";
        try
        {
            ::ProxyServer::Message::NotifyNoticeMessageFromProxyServerMessage response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->set_sign(PROTO_BUF_SIGN);

            string tmype = value["language"].asString();

            response.set_lantype(tmype);//
            response.set_title(value["title"].asString());
            response.set_message(value["publicNotice"].asString());
            vector<int32_t>  agentIdVec;
            agentIdVec.clear();
            for (int j = 0; j < value["agentid"].size(); ++j)
            {
                int32_t agentid = value["agentid"][j].asInt();
                response.add_agentid(agentid);
                agentIdVec.push_back(agentid);
            }
            size_t len = response.ByteSize();
            vector<uint8_t> data(len + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(Header)], len))
            {
                Header *commandSendHeader = (Header *)(&data[0]);
                commandSendHeader->ver = PROTO_VER;
                commandSendHeader->sign = HEADER_SIGN;
                commandSendHeader->reserve = 0;
                commandSendHeader->reqId = 0;
                commandSendHeader->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
                commandSendHeader->subId = ::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY;
                commandSendHeader->encType = PUBENC__PROTOBUF_NONE;

                commandSendHeader->len = len + sizeof(Header);
                commandSendHeader->realSize = len;
                commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[4], commandSendHeader->len - 4);
                {
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    for (auto &it : m_clientHallUserIdWeaConnMap)
                    {
                        TcpConnectionPtr conn = it.second.lock();
                        if (conn)
                        {
                            m_WebSocketServer.SendData(conn, data);
                        }
                    }
                }
            }
        }
        catch (exception excep)
        {
            LOG_ERROR << __FUNCTION__ << " " << excep.what();
        }
    }
    // websocket 建立连接回调
    void ProxyServer::onTcpConnection(const TcpConnectionPtr &conn)
    {
        string remoteIp = conn->peerAddress().toIpPort();
        string localIp = conn->localAddress().toIpPort();

        // check if connected.
        if (conn->connected())
        {
            int32_t num = m_connection_num.incrementAndGet();

            LOG_INFO << " >>>>>> H5 connected, from ip:" << remoteIp << " -> " << localIp << " is UP."
                     << ", conn:" << conn.get();
            LOG_INFO << "======================H5 connectionsNum=" << num << "======================";

            CountTimeOutPtr countTimeOut(new CountTimeOut(conn));
            WeakCountTimeOutPtr weakCountTimeOutPtr(countTimeOut);

            EntryPtr entry(new Entry(weakCountTimeOutPtr));
            string aesKey = Landy::Crypto::generateAESKey();
            string session = Landy::Crypto::BufferToHexString((unsigned char *)aesKey.c_str(), aesKey.size());
            entry->setSession(session);
            entry->setIP(conn->peerAddress().ipNetEndian());
            //        entry->setBWebSocketHandShake(false);

            {
                TcpConnectionWeakPtr weakPtr(conn);
                MutexLockGuard lock(m_clientSessionWeakConnMaMutex);
                m_clientSessionWeakConnMap[session] = weakPtr;
            }

            {
                MutexLockGuard lock(m_connection_buckets_mutex);
                m_connection_buckets.back().insert(countTimeOut);
                //            dumpConnectionBuckets();
            }

            //        WeakEntryPtr weakEntry(entry);
            conn->setContext(entry);
        }
        else
        {
            int32_t num = m_connection_num.decrementAndGet();
            LOG_INFO << " >>> client closed, from ip:" << remoteIp << " -> " << localIp << " is DOWN."
                     << ", conn:" << conn.get();
            LOG_INFO << "======================H5 connectionsNum=" << num << "======================";
            assert(!conn->getContext().empty());
            EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
            if (likely(entry))
            {
                int64_t userId = entry->getUserId();
                string session = entry->getSession();

                OnUserOfflineHall(entry);
                OnUserOfflineGS(entry);

                TcpConnectionWeakPtr weakPtr;
                map<int64_t, TcpConnectionWeakPtr>::iterator it;
                {
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    it = m_clientHallUserIdWeaConnMap.find(userId);
                    if (m_clientHallUserIdWeaConnMap.end() != it)
                        weakPtr = it->second;
                    // 
                    TcpConnectionPtr connPtr(weakPtr.lock());
                    if (connPtr && connPtr == conn)
                    {
                        m_clientHallUserIdWeaConnMap.erase(it);
                    }
                }

                {
                    MutexLockGuard lockSession(m_clientSessionWeakConnMaMutex);
                    m_clientSessionWeakConnMap.erase(session);
                }

                LOG_INFO << " >>> Delete m_clientSessionWeaConnMap.erase(session) <<<.";
            }
        }
    }
    // websocket 连接成功回调
    void ProxyServer::onWebSocketConnected(const muduo::net::TcpConnectionPtr &conn, string &ip)
    {
        LOG_INFO << __FUNCTION__ << " IP:" << ip;
        assert(!conn->getContext().empty());
        EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
        if (likely(entry))
        {
            InetAddress address(ip, 0);
            entry->setIP(address.ipNetEndian());
        }
    }

    void ProxyServer::onWebSocketClosed(const muduo::net::TcpConnectionPtr &conn)
    {
        LOG_INFO << __FUNCTION__;
        EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
        if (likely(entry))
        {
            LOG_INFO << ">>> on WebSocket Closed IP:" << Inet2Ipstr(entry->getIP());
        }
    }

    void ProxyServer::onWebSocketMessage(const muduo::net::TcpConnectionPtr &conn,
                                         vector<uint8_t> &buf, muduo::Timestamp receiveTime)
    {
        EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
        if (likely(entry))
        {
#if MULTI_THREAD

            TcpConnectionWeakPtr weakPtr(conn);
            m_thread_pool.run(bind(&ProxyServer::processRequest, this, weakPtr, buffer, receiveTime));

#elif MUDUO_THREAD_POOL

            int index = 0;
            EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
            if (likely(entry))
            {
                string session = entry->getSession();
                index = m_hashUserSession(session) % m_thread_pool_vec.size();
            }

            m_thread_pool_vec[index]->run(bind(&ProxyServer::processWebSocketRequest, this, conn, buf, receiveTime));

#elif MY_THREAD_POOL

            TcpConnectionWeakPtr weakPtr(conn);
            shared_ptr<MsgPacker> packet(new MsgPacker);
            packet->weakConn = weakPtr;
            packet->userId = -1;
            packet->bufPtr = buffer;
            m_proxy_packet_queue.put(packet);

#endif
        }

        if (likely(entry))
        {
            CountTimeOutPtr countTimeOutPtr(entry->getCountTimeOutPtr());
            MutexLockGuard lock(m_connection_buckets_mutex);
            m_connection_buckets.back().insert(countTimeOutPtr);
        }
    }

    void ProxyServer::processWebSocketRequest(const TcpConnectionPtr &conn,
                                              vector<uint8_t> &buf,
                                              muduo::Timestamp receiveTime)
    {

        int32_t size = buf.size();
        if (size < sizeof(Header))
        {
            LOG_ERROR << " " << __FUNCTION__ << " BUFFER IS NULL !!!!!!!";
            return;
        }

        //=============TEST============
        //    int64_t num = *(int64_t*)(&buf[0]);
        //    LOG_DEBUG << " num = "<<num;
        //    //string tmp = string(buf.begin(), buf.end());
        //    LOG_DEBUG << "0->"<< buf[0] << " 1->" << buf[1] << " 2->" << buf[2] << " 3->" << buf[3];

        //    m_WebSocketServer.SendData(conn, buf);

        Header *commandHeader = (Header *)(&buf[0]);
        uint16_t crc = GlobalFunc::GetCheckSum(&buf[4], buf.size() - 4);
        if (commandHeader->len == buf.size() && commandHeader->crc == crc && commandHeader->ver == PROTO_VER && commandHeader->sign == HEADER_SIGN)
        {
            EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
            if (likely(entry))
            {
                switch (commandHeader->mainId)
                {
                //网关消息
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
                {
                    switch (commandHeader->encType)
                    {
                    case PUBENC__PROTOBUF_NONE:
                    {
                        int mainid = commandHeader->mainId;
                        int subid = commandHeader->subId;
                        LOG_DEBUG << "---PUBENC__PROTOBUF_NONE: mainId=" << commandHeader->mainId << " subId=" << commandHeader->subId;
                        int id = (mainid << 8) | subid;
                        if (likely(m_functor_map.count(id)))
                        {
                            AccessCommandFunctor functor = m_functor_map[id];
                            functor(conn, buf);
                        }
                        else
                        {
                            LOG_WARN << "Not Define Command: mainId=" << mainid << " subId=" << subid;
                        }
                        break;
                    }
                    case PUBENC__PROTOBUF_RSA: //RSA
                    {
                        LOG_DEBUG << "---PUBENC__PROTOBUF_RSA: mainId=" << commandHeader->mainId << " subId=" << commandHeader->subId;
                        break;
                    }
                    case PUBENC__PROTOBUF_AES: //AES
                    {
                        LOG_DEBUG << "---PUBENC__PROTOBUF_AES: mainId=" << commandHeader->mainId << " subId=" << commandHeader->subId;
                       
                        string aesKey = "0000000000000000";
                        int len = 0;
                        vector<unsigned char> data(commandHeader->len + 16);
                        int ret = Landy::Crypto::aesDecrypt(aesKey, (unsigned char *)(&buf[0]) + sizeof(Header), buf.size() - sizeof(Header), &data[0], len);
                        if (ret > 0)
                        {
                            int mainid = commandHeader->mainId;
                            int subid = commandHeader->subId;
                            int id = (mainid << 8) | subid;
                            if (likely(m_functor_map.count(id)))
                            {
                                AccessCommandFunctor functor = m_functor_map[id];
                                memcpy((unsigned char *)(&buf[0] + sizeof(Header)), &data[0], len);
                                functor(conn, buf);
                            }
                            else
                            {
                                LOG_WARN << "Not Define Command: mainId=" << mainid << " subId=" << subid;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    return;
                }
                break;
                // 到大厅，游戏服或者比赛服消息
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER:
                case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PAY_SERVER:
                {
                    string session = entry->getSession();
                    int64_t userId = entry->getUserId();
                    string aesKey = entry->getAESKey();
                    uint32_t ip = entry->getIP();

                    weak_ptr<TcpClient> hallTcpClient = entry->getHallTcpClient();
                    weak_ptr<TcpClient> gameTcpClient = entry->getGameTcpClient();
                    weak_ptr<TcpClient> payTcpClient = entry->getPayTcpClient();

                    uint16_t len = sizeof(internal_prev_header) + commandHeader->len;
                    internal_prev_header internal_head;
                    memset(&internal_head, 0, sizeof(internal_prev_header));
                    internal_head.len = len;
                    internal_head.userId = userId;

                    internal_head.ip = ip;
                    memcpy(internal_head.session, session.c_str(), sizeof(internal_head.session));
                    memcpy(internal_head.aesKey, aesKey.c_str(), min(sizeof(internal_head.aesKey), aesKey.size()));
                    GlobalFunc::SetCheckSum(&internal_head);

                    vector<uint8_t> data(len);
                    memcpy(&data[0], &internal_head, sizeof(internal_head));
                    memcpy(&data[0 + sizeof(internal_head)], &buf[0], commandHeader->len);

                    // 支付服
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PAY_SERVER)
                    {
                        // LOG_ERROR << " >>> To Pay,len: " << len << ",userid:" << userId <<",subId:"<<commandHeader->subId <<",aesKey:" << aesKey ;
                        sendDataToPayServer(conn, payTcpClient, data, userId);
                    }
                    else if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL)
                    {
                        string host = "";
                        if (userId <= 0)
                        {
                            shared_ptr<TcpClient> tcp = hallTcpClient.lock();
                            if (tcp)
                                host = tcp->name();
                        }

                        // LOG_WARN << " >>> To Hall,len: " << len << ",userid:" << userId <<",subId:"<<commandHeader->subId <<",aesKey:" << aesKey;
                        sendDataToHallServer(conn, hallTcpClient, data, userId);
                    }
                    else
                    {
                        // LOG_INFO << " >>> send To Game,len: " << len << ", userid: " << userId <<" subId "<<commandHeader->subId;
                        // if(commandHeader->subId != 1) 
                            // LOG_INFO << " >>> To Game,len:" << len << ",userid:" << userId <<",subId:"<<commandHeader->subId;
                        sendDataToGameServer(conn, gameTcpClient, data, userId);
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER LEN ERROR OR CRC ERROR! header->len=" << commandHeader->len
                      << " bufferLen=" << buf.size() << " header->crc=" << commandHeader->crc << " buffer crc=" << crc;
        }
    }

    //=============================work thread=========================
    bool ProxyServer::sendData(const TcpConnectionPtr &conn, vector<uint8_t> &data)
    {
        string aesKey = "";
        if (likely(conn))
        {
            //        WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
            //        EntryPtr entry = weakEntry.lock();
            EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
            if (likely(entry))
                aesKey = entry->getAESKey();
            else
            {
                LOG_ERROR << "AES Key Is NULL.";
                return false;
            }
            vector<uint8_t> encryptedData;
            int ret = Landy::Crypto::aesEncrypt(aesKey, data, encryptedData);
            if (likely(ret > 0 && !encryptedData.empty()))
            {
                conn->send(&encryptedData[0], ret);
            }
        }
        else
        {
            LOG_DEBUG << "TcpConnectionPtr closed.";
            return false;
        }
        return true;
    }
 

    void ProxyServer::cmd_get_aes_key(const TcpConnectionPtr &conn, vector<uint8_t> &msgdata)
    {
        Header *commandHeader = (Header *)(&msgdata[0]);
        commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_RES;
        ::ProxyServer::Message::GetAESKeyMessage msg;
        if (likely(msg.ParseFromArray(&msgdata[sizeof(Header)], commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();

            try
            {
                string pubKey = msg.pubkey();
                string aeskeymsg = msg.keymsg();
                int32_t agentid = msg.agentid();
                int32_t reqType = msg.type();

                ::ProxyServer::Message::GetAESKeyMessageResponse response;
                ::Game::Common::Header *resp_header = response.mutable_header();
                resp_header->CopyFrom(header);

                int32_t retcode = 1;
                string errormsg = "error.";
                string aesKey;
                string pri_rsa_key;

                EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));

                LOG_INFO << " >>> 接收加密信息,reqType[" << reqType << "],agentid[" << agentid << "],pubKey[" << pubKey << "],aeskeymsg[" << aeskeymsg << "]";
                //第一次请求
                if ( reqType == 0 )
                {
                    pri_rsa_key = RAS_KEY;//Landy::Crypto::generateAESKeyEx();

                    string prikeyStr, pubkeyStr,pri_rsa_Str;
                    Landy::Crypto::generateRSAKey(pubkeyStr, prikeyStr);
                    if (entry)
                    {
                        entry->setPubKey(pubkeyStr);
                        entry->setPriKey(prikeyStr);
                        entry->setAESKeyCopy(pri_rsa_key);
                    }

                    retcode = 0;
                    errormsg = "OK";
                    aesKey.assign(pubkeyStr.begin(), pubkeyStr.end());
                    response.set_keytext(pri_rsa_key);
                    // LOG_INFO << ">>>第一次请求 priKay[" << prikeyStr << "],pri_rsa_key[" << pri_rsa_key << "],pubKey[" << pubkeyStr << "],aesKey[" << aesKey << "]";
                }
                else if ( reqType == 1 ) //第二次请求
                {
                    string newkeystr;
                    if (entry && (!aeskeymsg.empty()))
                    {
                        // 使用私钥加密 
                        string privateKey = entry->getPriKey();

                        string base64_crypt_str = base64_decode(aeskeymsg);
                        // Landy::Crypto::base64Decode(aeskeymsg,base64_crypt_str); //此方法有问题
                        if (!base64_crypt_str.empty())
                        {
                            // LOG_INFO << " >>> base64Decode[" << base64_crypt_str << "]<<<=======";
                            vector<uint8_t> decryptedData;
                            Landy::Crypto::rsaPrivateDecrypt(privateKey, (unsigned char *)base64_crypt_str.c_str(), base64_crypt_str.length(), decryptedData);
                            newkeystr.assign(decryptedData.begin(), decryptedData.end());
                            if(!newkeystr.empty()){
                                retcode = 0;
                                errormsg = "OK";
                                entry->setAESKey(newkeystr);
                                LOG_INFO << " >>> 第二次请求,新密钥保存成功,新密钥[" << newkeystr << "]<<<=======";
                            }
                            else
                            {
                                errormsg = "error,newkey str empty.";
                                LOG_ERROR << " >>> 第二次请求,新密钥保存失败,新密钥[" << newkeystr << "]<<<=======";
                            }
                        }
                    }
                }

                response.set_type(reqType);
                response.set_retcode(retcode);
                response.set_aeskey(aesKey);
                response.set_errormsg(errormsg);

                size_t len = response.ByteSize();
                vector<uint8_t> data(len + sizeof(Header));
                memcpy(&data[0], &msgdata[0], sizeof(Header));
                if (response.SerializeToArray(&data[sizeof(Header)], len))
                {
                    Header *commandSendHeader = (Header *)(&data[0]);
                    commandSendHeader->len = len + sizeof(Header);
                    commandSendHeader->realSize = len;
                    commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[4], commandSendHeader->len - 4);
                    m_WebSocketServer.SendData(conn, data);
                }
            }
            catch (exception excep)
            {
                LOG_ERROR << __FUNCTION__ << " " << excep.what();
            }
        }
        else
        {
            LOG_ERROR << __FUNCTION__ << " parse data error!";
        }
    }

    string ProxyServer::getRandomHallServer()
    {
        // 有效的大厅IP
        vector<string> ips;
        {
            MutexLockGuard lock(m_hallServerIps_mutex); 
            for (string &ip : m_hallServerIps)
            {
                auto it = find(m_invaildHallServerIps.begin(), m_invaildHallServerIps.end(), ip);
                if (it == m_invaildHallServerIps.end())
                {
                    if ((m_hallIpIsFixed == 1))
                    {
                        LOG_INFO << " >>> HallServerIP[" << ip << "]IpIsFixed[" << m_hallIpIsFixed << "][" << m_strHallIP << "]";
                        if ((ip.find(m_strHallIP) != std::string::npos))
                        {
                            ips.push_back(ip);
                            LOG_ERROR << " >>> getRandomHallServer[" << ip << "]";
                        }
                    }
                    else
                    {
                        ips.push_back(ip);
                    }
                }
            }
        }
        string ip = "";
        if (ips.size() > 0)
        {
            int index = GlobalFunc::RandomInt64(1, ips.size());
            ip = ips[index - 1];
        }
        return ip;
    }

    // 随机获取支付服
    string ProxyServer::getRandomPayServer()
    {
        for (string ip : m_gameServerIps)
        {
            LOG_INFO << " >>> " << __FUNCTION__ << ",无效支付服 ip[" << ip << "]";
        }

        // 有效的大厅IP
        vector<string> ips;
        {
            MutexLockGuard lock(m_payServerIps_mutex); 
            for (string &ip : m_payServerIps)
            {
                auto it = find(m_invaildPayServerIps.begin(), m_invaildPayServerIps.end(), ip);
                if (it == m_invaildPayServerIps.end())
                {
                    ips.push_back(ip); 
                }
                else
                {
                    LOG_INFO << " >>> " << __FUNCTION__ << ", 123 无效支付服 ip[" << ip << "]";
                }
                
            }
        }
        string ip = "";
        if (ips.size() > 0)
        {
            int index = GlobalFunc::RandomInt64(1, ips.size());
            ip = ips[index - 1];
        }
        LOG_ERROR << " >>> "<< __FUNCTION__ <<",ip[" << ip << "]";
        return ip;
    }
    // 发送数据到支付服
    void ProxyServer::sendDataToPayServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &payTcpClient, vector<uint8_t> &msgdata, int64_t userId)
    {
        //1，如果存在连接，则提升conn级别
        shared_ptr<TcpClient> payTcpClientPtr(payTcpClient.lock());
        if (likely(payTcpClientPtr))
        {
            //1.1 如果当前发送的目标大厅服务器当前不是无效的，则直接发送数据，否则往后跑
            auto it = find(m_invaildPayServerIps.begin(), m_invaildPayServerIps.end(), payTcpClientPtr->name());
            if (it == m_invaildPayServerIps.end())
            {
                TcpConnectionPtr payConn = payTcpClientPtr->connection();
                if (payConn && payConn->connected())
                {
                    payConn->send(msgdata.data(), msgdata.size());
                    return;
                }
            }
        }

        // 2，如果第一次发送数据或连接已经无效，则随机选择一个大厅服跟客户端绑定
        string strPayServer = getRandomPayServer();
        LOG_WARN << "userid:[" << userId << "] Random Pay IP=" << strPayServer;
        do
        {
            if (likely(strPayServer.empty()))
            {
                LOG_WARN << "+++++++++随机选择一个支付服跟客户端绑定 get Random Pay Server IS NULL +++++userId:" << userId << " <<<";
                break;
            }

            //2.1 校检是否存在这个PayServer
            MutexLockGuard lock(m_payIPServerMapMutex);
            if (!m_payIPServerMap.count(strPayServer))
            {
                LOG_ERROR << "+++++++++校检是否存在这个PayServer " << strPayServer << " +++++userId:" << userId << " <<<";
                break;
            }

            // 2.2 取出连接
            shared_ptr<TcpClient> payTcpClientPtr = m_payIPServerMap[strPayServer];
            if (!payTcpClientPtr)
            {
                LOG_ERROR << "+++++++++取出连接 IP+++++" << strPayServer << " +++++userId:" << userId << " <<<";
                break;
            }

            // 2.3 保存连接弱指针
            weak_ptr<TcpClient> payTcpClient(payTcpClientPtr);
            if (!conn)
            {
                LOG_ERROR << "+++++++++保存连接弱指针 client to proxy tcp connection Lost+++++++++";
                break;
            }

            // 2.4 取出客户端连接的实体数据对象
            EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
            if (!entry)
            {
                LOG_ERROR << "+++++++++取出客户端连接的实体数据对象 client to proxy tcp connection ENTRY Lost+++++++++";
                break;
            }

            // 2.5，绑定到大厅的连接到客户端连接中
            entry->setPayTcpClient(payTcpClient); 

            // 连接有效性判断
            TcpConnectionPtr payConn = payTcpClientPtr->connection();
            if (!payConn || !payConn->connected())
            {
                LOG_ERROR << "+++++++++连接有效性判断 conn to pay Lost+++++++++";
                break;
            }

            // 3 发送数据
            payConn->send(msgdata.data(), msgdata.size());

            LOG_ERROR << "+++++++++建立客户端连接的实体数据对象 client to proxy tcp connection payserver+++++++++";

        } while (0);
    }
 
    // 发送数据到大厅
    void ProxyServer::sendDataToHallServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &hallTcpClient, vector<uint8_t> &msgdata, int64_t userId)
    {
        //1，如果存在大厅连接，则提升conn级别
        shared_ptr<TcpClient> hallTcpClientPtr(hallTcpClient.lock());
        if (likely(hallTcpClientPtr))
        {
            //1.1 如果当前发送的目标大厅服务器当前不是无效的，则直接发送数据，否则往后跑
            auto it = find(m_invaildHallServerIps.begin(), m_invaildHallServerIps.end(), hallTcpClientPtr->name());
            if (it == m_invaildHallServerIps.end())
            {
                TcpConnectionPtr hallConn = hallTcpClientPtr->connection();
                if (hallConn && hallConn->connected())
                {
                    hallConn->send(msgdata.data(), msgdata.size());
                    return;
                }
            }
        }

        // 2，如果第一次发送数据或连接已经无效，则随机选择一个大厅服跟客户端绑定
        string strHallServer = getRandomHallServer();
        LOG_WARN << "userid:[" << userId << "] Random Hall IP=" << strHallServer;

        if (likely(!strHallServer.empty()))
        {
            //2.1 校检是否存在这个HallServer
            MutexLockGuard lock(m_hallIPServerMapMutex);
            if (likely(m_hallIPServerMap.count(strHallServer)))
            {
                // 2.2 取出大厅连接
                shared_ptr<TcpClient> hallTcpClientPtr = m_hallIPServerMap[strHallServer];
                if (likely(hallTcpClientPtr))
                {
                    // 2.3 保存连接弱指针
                    weak_ptr<TcpClient> hallTcpClient(hallTcpClientPtr);
                    if (likely(conn))
                    {
                        // 2.4 取出客户端连接的实体数据对象
                        EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
                        if (likely(entry))
                        {
                            // 2.5，绑定到大厅的连接到客户端连接中
                            entry->setHallTcpClient(hallTcpClient);
                        }
                        else
                        {
                            LOG_ERROR << "++++++++++++++++++client to proxy tcp connection ENTRY Lost++++++++++++++++++ISNULL";
                        }
                    }
                    else
                    {
                        LOG_ERROR << "++++++++++++++++++client to proxy tcp connection Lost++++++++++++++++++";
                    }

                    // 3 发送数据
                    TcpConnectionPtr hallConn = hallTcpClientPtr->connection();
                    if (hallConn && hallConn->connected())
                    {
                        hallConn->send(msgdata.data(), msgdata.size());
                    }
                    else
                    {
                        LOG_ERROR << "++++++++++++++++++ conn to hall Lost++++++++++++++++++";
                    }
                }
                else
                {
                    LOG_ERROR << "+++++++++++++++++++++++++++++getRandomHallServer++++IP+++++" << strHallServer << "+++shared_ptr<TcpClient> hallTcpClientPtr++Error+++++++++++++++++++++++++++++++++++++userId:" << userId << " <<<";
                }
            }
            else
            {
                LOG_ERROR << "+++++++++++++++++++++++++++++getRandomHallServer++++IP+++++m_hallIPServerMap++cout+++" << strHallServer << " 0 ++Error+++++++++++++++++++++++++++++++++++++userId:" << userId << " <<<";
            }
        }
        else
        {
            LOG_WARN << "+++++sendData To HallServer+++getRandomHallServer IS NULL!!! NO HALL SERVER+++++++++++++++++userId:" << userId << " <<<";
        }
    }

    //发送数据到游戏服
    void ProxyServer::sendDataToGameServer(const TcpConnectionPtr &conn, weak_ptr<TcpClient> &gameTcpClient, vector<uint8_t> &msgdata, int64_t userId)
    {
        shared_ptr<TcpClient> gameTcpClientPtr(gameTcpClient.lock());
        if (gameTcpClientPtr)
        {
            TcpConnectionPtr conn_ = gameTcpClientPtr->connection();
            if (conn_ && conn_->connected()) //在线状态判断
            {
                conn_->send(msgdata.data(), msgdata.size());
                return;
            }
            else
            {  
                if(conn_) 
                    LOG_ERROR << " >>> connected:" << conn_->peerAddress().toIpPort() << " is " << (conn_->connected() ? "UP" : "DOWN") << " " << userId;
                else
                {
                    LOG_ERROR << " >>> conn:null, userId:" << userId;
                }
                    
                LOG_ERROR << " >>> sendData To GameServer failure, userId:" << userId;
            }
        }
        else
            LOG_WARN << " >>> sendData To GameServer,connection IS NULL! userId:" << userId ;
    }


    //===========================hall server=============================
    void ProxyServer::onHallServerConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        string ip = conn->peerAddress().toIpPort();
        if (conn->connected())
        {
            int32_t num = m_connection_num.incrementAndGet();
            LOG_INFO << "===========connected OK Hall Server Connections Num=" << num << "======================"; 
        }
        else
        {
            int32_t num = m_connection_num.decrementAndGet();
            LOG_INFO << "=========== connected failure Hall Server Connections Num=" << num << "======================"; 
        }
    }

    void ProxyServer::onPayServerMessage(const TcpConnectionPtr &conn,muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) 
        {
            const uint16_t len = buf->peekInt16();
            if (unlikely(len > PACKET_SIZE || len < sizeof(int16_t)))
            {
                LOG_ERROR << "onHallServerMessage invalid length to shutdown socket. length:" << len;
                if (conn)
                    conn->shutdown(); 
                break;
            }
            else if (likely(buf->readableBytes() >= len))
            {
                // 包长度
                BufferPtr buffer(new muduo::net::Buffer(len));
                buffer->append(buf->peek(), static_cast<size_t>(len));
                buf->retrieve(len);
 
                internal_prev_header *pre_header = (internal_prev_header *)buffer->peek();
            
                string session(pre_header->session, sizeof(pre_header->session));
                int index = m_hashUserSession(session) % m_thread_pool_vec.size();
                m_thread_pool_vec[index]->run(bind(&ProxyServer::processPayRequest, this, conn, buffer, receiveTime));
            }
            else
            {
                break;
            }
        }
    }

    //===========================pay server=============================
    void ProxyServer::onPayServerConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> " << conn->peerAddress().toIpPort() << " is " << (conn->connected() ? "UP" : "DOWN");

        string ip = conn->peerAddress().toIpPort();
        if (conn->connected())
        {
            int32_t num = m_connection_num.incrementAndGet();
            LOG_INFO << "===========connected OK Pay Server Connections Num=" << num << "======================";
        }
        else
        {
            int32_t num = m_connection_num.decrementAndGet();
            LOG_INFO << "=========== connected failure Pay Server Connections Num=" << num << "======================";
        }
    }
    
    void ProxyServer::processPayRequest(const TcpConnectionPtr &conn, BufferPtr &buf, muduo::Timestamp receiveTime)
    {
        if (!buf)
        {
            LOG_ERROR  << __FUNCTION__ << " BUFFER IS NULL!!!!!!!";
            return;
        }

        int32_t size = buf->readableBytes();
        if (size <= 0)
        {
            LOG_ERROR << __FUNCTION__ << " buf->readableBytes IS <=0 !!!!!!!";
            return;
        }

        internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

        string session(pre_header->session, sizeof(pre_header->session));
        int64_t userId = pre_header->userId;

        TcpConnectionWeakPtr weakPtr;
        {
            MutexLockGuard lock(m_clientSessionWeakConnMaMutex);
            auto it = m_clientSessionWeakConnMap.find(session);
            if (m_clientSessionWeakConnMap.end() != it)
                weakPtr = it->second;
        }
        Header *header = (Header *)(buf->peek() + sizeof(internal_prev_header));

        LOG_WARN <<  __FUNCTION__ << " userId[" << userId << "],session[" << session <<"],mainId[" << header->mainId <<"],subId[" << header->subId << "]";

        TcpConnectionPtr sendConn(weakPtr.lock());
        if (!sendConn)
        {
            LOG_ERROR << __FUNCTION__ << " TcpConnectionPtr conn(weakPtr.lock()) IS NULL! userId[" << userId << "],session[" << session <<"]";
            return;
        }

        // 返回登录信息
        // if (header->mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PAY_SERVER &&  pre_header->nOK == 1)

        // 支付服返回消息发送到客户端
        m_WebSocketServer.SendData(sendConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), header->len);

        LOG_INFO << " >>> To H5 Client, userId:" << userId << ",len:" << header->len << ",mainid:" << header->mainId << ",subid:" << header->subId;
    }

    void ProxyServer::onHallServerMessage(const TcpConnectionPtr &conn,
                                          muduo::net::Buffer *buf,
                                          muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
        {
            // FIXME: use Buffer::peekInt32()
            const uint16_t len = buf->peekInt16();
            if (unlikely(len > PACKET_SIZE || len < sizeof(int16_t)))
            {
                LOG_ERROR << "onHallServerMessage invalid length to shutdown socket. length:" << len;
                if (conn)
                    conn->shutdown(); // FIXME: disable reading
                break;
            }
            else if (likely(buf->readableBytes() >= len))
            {
                // 包长度
                BufferPtr buffer(new muduo::net::Buffer(len));
                buffer->append(buf->peek(), static_cast<size_t>(len));
                buf->retrieve(len);

                internal_prev_header *pre_header = (internal_prev_header *)buffer->peek();
              
                string session(pre_header->session, sizeof(pre_header->session));
                int index = m_hashUserSession(session) % m_thread_pool_vec.size();
                m_thread_pool_vec[index]->run(bind(&ProxyServer::processHallRequest, this, conn, buffer, receiveTime));
            }
            else
            {
                break;
            }
        }
    }

    void ProxyServer::processHallRequest(const TcpConnectionPtr &conn,
                                         BufferPtr &buf,
                                         muduo::Timestamp receiveTime)
    {
        if (!buf)
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER IS NULL!!!!!!!";
            return;
        }

        int32_t size = buf->readableBytes();
        if (size <= 0)
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " buf->readableBytes IS <=0 !!!!!!!";
            return;
        }

        internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

        string session(pre_header->session, sizeof(pre_header->session));
        int64_t userId = pre_header->userId;

        TcpConnectionWeakPtr weakPtr;
        {
            MutexLockGuard lock(m_clientSessionWeakConnMaMutex);
            auto it = m_clientSessionWeakConnMap.find(session);
            if (m_clientSessionWeakConnMap.end() != it)
                weakPtr = it->second;
        }
        Header *header = (Header *)(buf->peek() + sizeof(internal_prev_header));
        TcpConnectionPtr sendConn(weakPtr.lock());
        if (likely(sendConn))
        {
            // 返回登录信息
            if (header->mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
                header->subId == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES &&
                pre_header->nOK == 1)
            {
                // 给客户端连接绑定用户ID
                EntryPtr entry(boost::any_cast<EntryPtr>(sendConn->getContext()));
                if (likely(entry))
                {
                    entry->setUserId(pre_header->userId);
                    //ntry->setProxyId();
                }
                // 关闭旧连接
                TcpConnectionWeakPtr oldWeakPtr;
                {
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    auto it = m_clientHallUserIdWeaConnMap.find(userId);
                    if (m_clientHallUserIdWeaConnMap.end() != it)
                        oldWeakPtr = it->second;
                }
                TcpConnectionPtr oldConn(oldWeakPtr.lock());
                if (oldConn)
                {
                    vector<uint8_t> data;
                    GetShutDownUserClientMessage(userId, data);
                    m_WebSocketServer.SendData(oldConn, data);
                    oldConn->forceCloseWithDelay(0.1);
                }
                {
                    //玩家登录成功后唯一维护更新对客户端的连接weakPtr
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    m_clientHallUserIdWeaConnMap[userId] = weakPtr;
                }
            }
            else if (header->mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
                     header->subId == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES &&
                     pre_header->nOK == 1)
            {
                EntryPtr entry(boost::any_cast<EntryPtr>(sendConn->getContext()));
                if (likely(entry))
                { 
                    // 从大厅绑定游戏服信息成功后从中获取游戏服IP
                    string ip;
					LOG_INFO << " >>> GetUserOnlineInfoIP, userId:" << userId << ", ip:" << ip;
                    if (REDISCLIENT.GetUserOnlineInfoIP(userId, ip))
                    {
                        MutexLockGuard lock(m_gameIPServerMapMutex);

                        //游戏服的IP地址信息
                        if (m_gameIPServerMap.count(ip))
                        {
                            // 使用连接
                            shared_ptr<TcpClient> gameTcpClientPtr = m_gameIPServerMap[ip];
                            weak_ptr<TcpClient> gameTcpClient(gameTcpClientPtr);

                            // 更新玩家绑定的连接
                            if (gameTcpClientPtr && gameTcpClientPtr->connection() && gameTcpClientPtr->connection()->connected()){
                                entry->setGameTcpClient(gameTcpClient);
                                LOG_INFO << " >>> bind game server succeeded, userId:" << userId << ", ip:" << ip;
                            }
                            // 检查连接状态
                            else{
                                LOG_ERROR << " >>> bind game serever ip connected is down, userId:" << userId << ", ip:" << ip;
                            }
                        }
						else
						{
							LOG_INFO << " >>> m_gameIPServerMap.count(ip), userId:" << userId << ", ip:" << ip;
						}
                    }
                }
            }

            // LOG_INFO << " >>> To H5 Client, userId:" << userId << ",len:" << header->len << ",mainid:" << header->mainId << ",subid:" << header->subId ;
            // 大厅返回消息发送到客户端
            m_WebSocketServer.SendData(sendConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), header->len);
        }
        else
        {
            LOG_ERROR << " " << __FUNCTION__ << "processHallRequest TcpConnectionPtr conn(weakPtr.lock()) IS NULL!";
            LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
            LOG_ERROR << " >>> HallServer -> client, userid:" << userId << ", mainid:" << header->mainId << ", subid:" << header->subId << ", datasize:" << header->len;
        }
    }

    // 检查连接
    bool ProxyServer::checkTcpConnected(int64_t userId,EntryPtr &entry)
    {
        bool ret = false;
        // 从大厅绑定游戏服信息成功后从中获取游戏服IP
        string ip;
        if (REDISCLIENT.GetUserOnlineInfoIP(userId, ip))
        {
            MutexLockGuard lock(m_gameIPServerMapMutex);

            //游戏服的IP地址信息
            if (m_gameIPServerMap.count(ip))
            {
                // 在给玩家分配连接的时候先尝试连接一次，做一次检查
                // 检查一下连接是否有效，如已经失效则重连接
                startGameServerConnection(ip);

                // 使用连接
                shared_ptr<TcpClient> gameTcpClientPtr = m_gameIPServerMap[ip];
                weak_ptr<TcpClient> gameTcpClient(gameTcpClientPtr);

                // 更新玩家绑定的连接
                if (gameTcpClientPtr && gameTcpClientPtr->connection() && gameTcpClientPtr->connection()->connected()){
                    entry->setGameTcpClient(gameTcpClient);
                    ret = true;
                    LOG_INFO << " >>> bind game server succeeded, userId:" << userId << ", ip:" << ip;
                }
                // 检查连接状态
                else{ //(!gameTcpClientPtr->connection()->connected()){
                    LOG_ERROR << " >>> bind game serever ip connected is down, userId:" << userId << ", ip:" << ip;
                }
            }
        }

        return ret;
    }

    //===========================game server=================================
    void ProxyServer::onGameServerConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO << __FUNCTION__ << " " << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        string ip = conn->peerAddress().toIpPort();
        if (conn->connected())
        {
            int32_t num = m_connection_num.incrementAndGet();

            // 172.16.0.92:43776 -> 172.16.0.87:38903 
            vector<string> addrVec;
            addrVec.clear();
            boost::algorithm::split(addrVec, ip, boost::is_any_of(":"));
            if (addrVec.size() == 2)
            {
                string port = to_string(atoll(addrVec[1].c_str()) % 30000);
                string ipport = port + ":" + ip;
                {  
                    // 连接上则发送一次心跳信息
                    MutexLockGuard lock(m_gameIPServerMapMutex);
                    if (m_gameIPServerMap.count(ipport))
                    {
                        shared_ptr<TcpClient> gameTcpClientPtr = m_gameIPServerMap[ipport];
                        weak_ptr<TcpClient> gameTcpClient(gameTcpClientPtr);
                        TcpConnectionPtr connptr = gameTcpClientPtr->connection();
                        if (connptr && connptr->connected()) //在线状态判断
                        {
                            LOG_INFO << "==keep alive ping,ipport[" << ipport << "]==";
                            check_keep_alive_ping(gameTcpClient);
                        }
                    }
                    else
                    {
                        LOG_INFO << "====找不到[" << ipport << "]====";
                    }
                }
                LOG_INFO << "========= 1 connected ok GameServer Connections Num=" << num << " " << addrVec[0] << ",v1" << addrVec[1] << ",port=" << port << ",ipport=" << ipport;
            }
        }
        else
        {
            int32_t num = m_connection_num.decrementAndGet();
            LOG_INFO << "===========connected failure GameServer Connections Num=" << num << "======================";
            //        MutexLockGuard lock(m_hallIPServerMapMutex);
            //        m_hallIPServerMap.erase(ip);
        }
    }

    void ProxyServer::onGameServerMessage(const muduo::net::TcpConnectionPtr &conn,
                                          muduo::net::Buffer *buf,
                                          muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
        {
            // FIXME: use Buffer::peekInt32()
            const uint16_t len = buf->peekInt16();
            if (unlikely(len > PACKET_SIZE || len < sizeof(uint16_t)))
            {
                LOG_ERROR << "onGameServerMessage invalid length to shutdown socket. length:" << len;
                if (conn)
                    conn->shutdown(); // FIXME: disable reading
                break;
            }
            else if (likely(buf->readableBytes() >= len))
            {
                BufferPtr buffer(new muduo::net::Buffer(len));
                buffer->append(buf->peek(), static_cast<size_t>(len));
                buf->retrieve(len);
#if MULTI_THREAD
                TcpConnectionWeakPtr weakPtr(conn);
                m_thread_pool.run(bind(&ProxyServer::processGameRequest, this, weakPtr, buffer, receiveTime));
#elif MUDUO_THREAD_POOL

                internal_prev_header *pre_header = (internal_prev_header *)buffer->peek();
                //            bool ok = GlobalFunc::CheckCheckSum(pre_header);
                //            if(!ok)
                //            {
                //                LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
                //                LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
                //                LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
                //                conn->shutdown();
                //                return;
                //            }
                string session(pre_header->session, sizeof(pre_header->session));
                int index = m_hashUserSession(session) % m_thread_pool_vec.size();
                //            LOG_ERROR << "+++++++++++++++++++++++++++++INDEX:"<<index<<"++++++++++++++++++++++++++++";
                //            TcpConnectionWeakPtr weakPtr(conn);
                m_thread_pool_vec[index]->run(bind(&ProxyServer::processGameRequest, this, conn, buffer, receiveTime));

#elif MY_THREAD_POOL

                TcpConnectionWeakPtr weakPtr(conn);
                shared_ptr<MsgPacker> packet(new MsgPacker);
                packet->weakConn = weakPtr;
                packet->userId = -1;
                packet->bufPtr = buffer;
                m_game_packet_queue.put(packet);

#endif
            }
            else
            {
                break;
            }
        }
    }

    void ProxyServer::processGameRequest(const TcpConnectionPtr &conn,
                                         BufferPtr &buf,
                                         muduo::Timestamp receiveTime)
    {
        // LOG_INFO << " " <<  __FILE__ << " " << __FUNCTION__;
        if (!buf)
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER IS NULL!!!!!!!";
            return;
        }

        int32_t size = buf->readableBytes();
        if (size <= 0)
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " buf->readableBytes IS <=0 !!!!!!!";
            return;
        }

        internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

        //    bool ok = GlobalFunc::CheckCheckSum(pre_header);
        //    if(!ok)
        //    {
        //        LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
        //        LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
        //        LOG_ERROR << " EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERROREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
        ////        TcpConnectionPtr conn(weakConn.lock());
        //        if(conn)
        //            conn->shutdown();
        //        return;
        //    }

        string session(pre_header->session, sizeof(pre_header->session));

        // LOG_ERROR << "Reveive session :" << session;

        int64_t userId = pre_header->userId;

        // get special kick type from game server.
        int16_t kickType = pre_header->bKicking;
        //    if(kickType)
        //    {
        //        LOG_ERROR << " " <<  __FILE__ << " " << __FUNCTION__ << "kickType KickUserId";
        //        KickUserId(session, userId, kickType);
        //    }else
        {
            TcpConnectionWeakPtr weakPtr;
            {
                MutexLockGuard lock(m_clientSessionWeakConnMaMutex);
                auto it = m_clientSessionWeakConnMap.find(session);
                if (m_clientSessionWeakConnMap.end() != it)
                    weakPtr = it->second;
            }

            //    if(userId > 0)
            //    {
            //        MutexLockGuard lockUserId(m_clientGameUserIdWeaConnMapMutex);
            //        m_clientGameUserIdWeaConnMap[userId] = weakPtr;
            //    }

            Header *header = (Header *)(buf->peek() + sizeof(internal_prev_header));
            TcpConnectionPtr sendConn(weakPtr.lock());
            if (likely(sendConn))
            {
                // check the session and userid.
                {
                    //                WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
                    //                LOG_DEBUG << "Entry use_count = " << weakEntry.use_count();
                    //                EntryPtr entry(weakEntry.lock());
                    EntryPtr entry(boost::any_cast<EntryPtr>(sendConn->getContext()));
                    if (likely(entry))
                    {
                        int64_t userId2 = entry->getUserId();
                        string session2 = entry->getSession();

                        if ((userId2 != userId) ||
                            (session2 != session))
                        {
                            string hexsession1 = Landy::Crypto::BufferToHexString((unsigned char *)session.data(), session.size());
                            string hexsession2 = Landy::Crypto::BufferToHexString((unsigned char *)session2.data(), session2.size());
                            LOG_ERROR << " >>>>>>>>>>>>>> Proxy session error, header.id:" << userId << ", conn.id:" << userId2 << ", session1:" << hexsession1 << ", session2:" << hexsession2;
                        }
                    }
                    else
                    {
                        LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << "EntryPtr entry IS NULL!";
                    }
                }

                //            LOG_DEBUG << " <<< GameServer -> client, userid:" << userId << ", mainid:" << mainid << ", subid:" << subid << ", datasize:" << header->len;
                m_WebSocketServer.SendData(sendConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), header->len);
            }
            else
            {
                if(userId > 0)
                    LOG_ERROR << " " << __FUNCTION__ << " conn(weakPtr.lock()) IS NULL! userId[" << userId <<"],session[" << session <<"],kickType[" << kickType <<"]";
            }
        }
    }

    void ProxyServer::notifyUserLoginMessage(string msg)
    {
        //    long tick = Utility::gettimetick();

        try
        {

            Json::Reader reader;
            Json::Value root;
            if (reader.parse(msg, root))
            {
                int64_t userId = root["userId"].asInt();
                string session = root["session"].asString();
                // PrintSockerMap();
                bool bFind = true;
                {
                    MutexLockGuard lock(m_clientSessionWeakConnMaMutex);
                    if (m_clientSessionWeakConnMap.find(session) == m_clientSessionWeakConnMap.end())
                    {
                        bFind = false;
                    }
                }

                // 本网关有此用户
                if(bFind){
                    LOG_INFO << "================= Hall UserId (" << m_clientHallUserIdWeaConnMap.size() << ")=================";
                    LOG_INFO << " >>> notifyUserLoginMessage msg=" << msg ;
                }

                // 本服务器没有找到缓存连接，则关闭大厅连接
                if (!bFind) //not myself
                {
                    ////            SendShutDownMessageToClient(userId, session);
                    //            KickUserId(session, userId, KICK_GS);
                    //            DeleteUserIdMap(userId);
                    TcpConnectionWeakPtr oldWeakPtr;
                    {
                        MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                        auto it = m_clientHallUserIdWeaConnMap.find(userId);
                        if (m_clientHallUserIdWeaConnMap.end() != it)
                            oldWeakPtr = it->second;
                    }
                    TcpConnectionPtr oldConn(oldWeakPtr.lock());
                    if (oldConn)
                    {
                        vector<uint8_t> data;
                        GetShutDownUserClientMessage(userId, data);
                        m_WebSocketServer.SendData(oldConn, data);
                        oldConn->forceCloseWithDelay(0.1);

                        LOG_ERROR << "++++++++++++++++++++++++++++++++++++++++++";
                        LOG_ERROR << "==================关闭玩家=================";
                        LOG_ERROR << "++++++++++++++++++++++++++++++++++++++++++";
                    }
                }
            }
        }
        catch (exception excep)
        {
            LOG_ERROR << __FUNCTION__ << " " << excep.what();
        }

    }

    void ProxyServer::notifyUserOrderScoreMessage(string msg)
    {
        LOG_INFO << " >>> notifyUserOrderScoreMessage msg=" << msg ;

        try
        {
            Json::Value value;
            Json::Reader reader;
            if (reader.parse(msg, value))
            {
                int64_t userId = value["userId"].asInt64();
                int64_t Score = value["score"].asInt64();
                int64_t safeScore = value["safeScore"].asInt64();
                LOG_INFO << "userId=" << userId << "  Score="<<Score<<"    safeScore"<<safeScore;
                TcpConnectionWeakPtr weakPtr;
                {
                    
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    auto it = m_clientHallUserIdWeaConnMap.find(userId);
                    if (m_clientHallUserIdWeaConnMap.end() != it)
                        weakPtr = it->second;
                }

                LOG_INFO << ">>> notifyUserOrderScoreMessage userId=" << userId << " <<<";
                ::Game::Common::ProxyNotifyOrderScoreMessage response;
                ::Game::Common::Header *resp_header = response.mutable_header();
                resp_header->set_sign(PROTO_BUF_SIGN);
                //    resp_header->set_mainid(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY);
                //    resp_header->set_subid(::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY);
                response.set_score(Score);
                response.set_userid(userId);
                response.set_safescore(safeScore);
                vector<uint8_t> data;
                size_t len = response.ByteSize();
                data.resize(len + sizeof(Header));
                response.SerializeToArray(&data[sizeof(Header)], len);

                Header *header = (Header *)&data[0];
                header->len = len + sizeof(Header);
                header->ver = PROTO_VER;
                header->sign = HEADER_SIGN;
                header->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
                header->subId = ::Game::Common::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE;
                header->encType = PUBENC__PROTOBUF_NONE;
                header->reserve = 0;
                header->reqId = 0;
                header->realSize = len;
                header->crc = GlobalFunc::GetCheckSum(&data[4], header->len - 4);

                TcpConnectionPtr conn = weakPtr.lock();
                if (conn)
                {
                    m_WebSocketServer.SendData(conn, data);
                }
            }
            else
            {
                 LOG_INFO << "cuo  cuo  cuo  cuo  cuo ";
            }
        }
        catch (exception excep)
        {
            LOG_ERROR << __FUNCTION__ << " " << excep.what();
        }
    }

    void ProxyServer::KickUserId(string &session, int64_t userId, int32_t kickType)
    {
        SendKickMessageToClient(session, userId, kickType);
        //    DeleteUserIdMap(userId);
    }

    void ProxyServer::SendKickMessageToClient(string &session, int64_t userId, int32_t kickType)
    {
        LOG_INFO << ">>> SendKickMessageToClient userId:" << userId << ", kickType:" << kickType;

        vector<uint8_t> data;

        int kickGS = (kickType & KICK_GS);
        int kickHALL = (kickType & KICK_HALL);
        int kickClose = (kickType & KICK_CLOSEONLY);
        if (kickGS)
        {
            TcpConnectionWeakPtr weakPtr;
            {
                MutexLockGuard lockUserId(m_clientSessionWeakConnMaMutex);
                auto it = m_clientSessionWeakConnMap.find(session);
                if (it != m_clientSessionWeakConnMap.end())
                    weakPtr = it->second;
            }
            if (data.empty())
            {
                GetKickUserClientMessage(userId, data);
            }
            SendKickDataAndClose(weakPtr, data, kickClose);
        }

        if (kickHALL)
        {
            if (m_clientHallUserIdWeaConnMap.count(userId))
            {
                TcpConnectionWeakPtr weakPtr;
                {
                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
                    auto it = m_clientHallUserIdWeaConnMap.find(userId);
                    if (it != m_clientHallUserIdWeaConnMap.end())
                        weakPtr = it->second;
                }
                if (data.empty())
                {
                    GetKickUserClientMessage(userId, data);
                }
                SendKickDataAndClose(weakPtr, data, kickClose);
            }
            else
            {
                LOG_DEBUG << " >>> SendKickMessageToClient skipped, because client hall connection has been lost.";
            }
        }
    }

    bool ProxyServer::GetKickUserClientMessage(int64_t userId, vector<uint8_t> &data)
    {
        //    ::Game::Common::ProxyNotifyShutDownUserClientMessage response;
        //    ::Game::Common::Header *resp_header = response.mutable_header();
        //    resp_header->set_crc(0);
        //    resp_header->set_sign(PROTO_BUF_SIGN);
        //    resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
        //    resp_header->set_subid(::Game::Common::PROXY_NOTIFY_KICKING_USER_CLIENT_MESSAGE_REQ);

        //    response.set_userid(userId);

        //    size_t len = response.ByteSize();
        //    data.resize(len);
        //    return (response.SerializeToArray(&data[0], len));
    }

    void ProxyServer::SendKickDataAndClose(TcpConnectionWeakPtr weakPtr, vector<uint8_t> &data, int32_t bCloseOnly)
    {
        TcpConnectionPtr clientPtr(weakPtr.lock());
        if (clientPtr)
        {
            EntryPtr entry(boost::any_cast<EntryPtr>(clientPtr->getContext()));
            if (likely(entry))
            {
                entry->setUserLeftCloseOnly(1);
            }

            //        if(!bCloseOnly)
            sendData(clientPtr, data);

            clientPtr->forceCloseWithDelay(0.1); //如果是0.0可能导致先释放链接
            LOG_INFO << ">>> SendKickDataAndClose Sending  <<<, bCloseOnly:" << bCloseOnly;
            return;
        }
        //Cleanup:
        LOG_INFO << ">>> SendKickDataAndClose NOT SEND !!!<<<";
    }

    // on user offline from the special hall connect now.
    void ProxyServer::OnUserOfflineHall(EntryPtr &entry)
    {
        string session = entry->getSession();
        int64_t userId = entry->getUserId();
        string aesKey = entry->getAESKey();

        weak_ptr<TcpClient> hallTcpClient = entry->getHallTcpClient();

        LOG_DEBUG << " >>> OnUserOffline Arrived, userId:" << userId;

        //    ::HallServer::HallOnUserOfflineMessage hallOffline;
        //    ::Game::Common::Header *resp_header = hallOffline.mutable_header();

        //    resp_header->set_crc(0);
        //    resp_header->set_sign(PROTO_BUF_SIGN);
        //    resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
        //    resp_header->set_subid(::Game::Common::SUBID::HALL_ON_USER_OFFLINE);
        //    hallOffline.set_userid(userId);

        uint16_t len = sizeof(internal_prev_header) + sizeof(Header);
        vector<uint8_t> data(len);
        internal_prev_header *internal_header = (internal_prev_header *)&data[0];
        memset(&data[0], 0, len);
        internal_header->len = len;
        internal_header->userId = userId;
        memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
        memcpy(internal_header->aesKey, aesKey.c_str(), min(sizeof(internal_header->aesKey), aesKey.size()));
        GlobalFunc::SetCheckSum(internal_header);

        Header *header = (Header *)&data[sizeof(internal_prev_header)];
        header->len = sizeof(Header);
        header->ver = PROTO_VER;
        header->sign = HEADER_SIGN;
        header->mainId = ::Game::Common::MAIN_MESSAGE_PROXY_TO_HALL;
        header->subId = ::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE;
        header->encType = PUBENC__PROTOBUF_NONE;
        header->reserve = 0;
        header->reqId = 0;
        header->realSize = 0;
        header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

        TcpConnectionPtr conn;
        sendDataToHallServer(conn, hallTcpClient, data, userId);
    }

    void ProxyServer::OnUserOfflineGS(EntryPtr &entry, bool bLeaveGs)
    {
        string session = entry->getSession();
        int64_t userId = entry->getUserId();
        string aesKey = entry->getAESKey();

        //  weak_ptr<TcpClient> hallTcpClient = entry->getHallTcpClient();
        weak_ptr<TcpClient> gameTcpClient = entry->getGameTcpClient();

        LOG_DEBUG << " >>> Client side socket closed, send OnUserOffline to GS, userId:" << userId;

        uint16_t len = sizeof(internal_prev_header) + sizeof(Header);
        vector<uint8_t> data(len);
        internal_prev_header *internal_header = (internal_prev_header *)&data[0];
        memset(&data[0], 0, len);
        internal_header->len = len;
        internal_header->userId = userId;
        if (bLeaveGs)
            internal_header->bKicking = KICK_LEAVEGS;
        memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
        memcpy(internal_header->aesKey, aesKey.c_str(), min(sizeof(internal_header->aesKey), aesKey.size()));
        GlobalFunc::SetCheckSum(internal_header);

        Header *header = (Header *)&data[sizeof(internal_prev_header)];
        header->len = sizeof(Header);
        header->ver = PROTO_VER;
        header->sign = HEADER_SIGN;
        header->mainId = ::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER;
        header->subId = ::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE;
        header->encType = PUBENC__PROTOBUF_NONE;
        header->reserve = 0;
        header->reqId = 0;
        header->realSize = 0;
        header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

        TcpConnectionPtr conn;
        sendDataToGameServer(conn, gameTcpClient, data, userId);
    }

    // 检查连接状态
    void ProxyServer::check_keep_alive_ping( weak_ptr<TcpClient> gameTcpClient)
    {
        string session = "";// entry->getSession();
        int64_t userId = 0;//entry->getUserId();
        string aesKey = "";//entry->getAESKey();

        // LOG_INFO << " >>> 1 keep_alive_ping, userId:" << userId;

        uint16_t len = sizeof(internal_prev_header) + sizeof(Header);
        vector<uint8_t> data(len);
        internal_prev_header *internal_header = (internal_prev_header *)&data[0];
        memset(&data[0], 0, len);
        internal_header->len = len;
        internal_header->userId = userId;
        // if (bLeaveGs)
        //     internal_header->bKicking = KICK_LEAVEGS;
        memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
        memcpy(internal_header->aesKey, aesKey.c_str(), min(sizeof(internal_header->aesKey), aesKey.size()));
        GlobalFunc::SetCheckSum(internal_header);

        Header *header = (Header *)&data[sizeof(internal_prev_header)];
        header->len = sizeof(Header);
        header->ver = PROTO_VER;
        header->sign = HEADER_SIGN;
        header->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
        header->subId = ::Game::Common::KEEP_ALIVE_REQ;//::Game::Common::KEEP_ALIVE_REQ
        header->encType = PUBENC__PROTOBUF_NONE;
        header->reserve = 0;
        header->reqId = 0;
        header->realSize = 0;
        header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

        TcpConnectionPtr conn;
        sendDataToGameServer(conn, gameTcpClient, data, userId);
        // LOG_INFO << " >>> 2 keep_alive_ping, userId:" << userId;
    }

    bool ProxyServer::GetShutDownUserClientMessage(int64_t userId, vector<uint8_t> &data, int32_t status)
    {
        LOG_ERROR << ">>> GetShutDownUserClientMessage userId=" << userId << " <<<";
        ::Game::Common::ProxyNotifyShutDownUserClientMessage response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->set_sign(PROTO_BUF_SIGN);
        //    resp_header->set_mainid(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY);
        //    resp_header->set_subid(::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY);

        response.set_userid(userId);
        response.set_status(status);
        size_t len = response.ByteSize();
        data.resize(len + sizeof(Header));
        response.SerializeToArray(&data[sizeof(Header)], len);

        Header *header = (Header *)&data[0];
        header->len = len + sizeof(Header);
        header->ver = PROTO_VER;
        header->sign = HEADER_SIGN;
        header->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
        header->subId = ::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY;
        header->encType = PUBENC__PROTOBUF_NONE;
        header->reserve = 0;
        header->reqId = 0;
        header->realSize = len;
        header->crc = GlobalFunc::GetCheckSum(&data[4], header->len - 4);

        return true;
    }

    // delete the special user hall and gs connection.
    void ProxyServer::DeleteUserIdMap(int64_t userId)
    {
        LOG_INFO << ">>> CloseUserConnectionToClient userId=" << userId << " <<<";

        TcpConnectionWeakPtr weakPtr;
        {
            MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
            auto it = m_clientHallUserIdWeaConnMap.find(userId);
            if (m_clientHallUserIdWeaConnMap.end() != it)
            {
                weakPtr = it->second;
            }
        }
        TcpConnectionPtr connPtr(weakPtr.lock());
        if (connPtr)
        {
            EntryPtr gameEntry(boost::any_cast<EntryPtr>(connPtr->getContext()));
            if (gameEntry)
            {
                OnUserOfflineGS(gameEntry, true);
            }
        }
        m_clientHallUserIdWeaConnMap.erase(userId);

        //    // kick game server connection.
        //    {
        //        TcpConnectionWeakPtr weakPtr;
        //        {
        //            MutexLockGuard lockUserId(m_clientGameUserIdWeaConnMapMutex);
        //            auto it = m_clientGameUserIdWeaConnMap.find(userId);
        //            if(m_clientGameUserIdWeaConnMap.end() != it)
        //            {
        //                weakPtr = it->second;
        //            }

        //            // try to get the special connection item.
        //            TcpConnectionPtr connPtr(weakPtr.lock());
        //            if(connPtr)
        //            {
        ////                WeakEntryPtr gameWeakEntry(boost::any_cast<WeakEntryPtr>(connPtr->getContext()));
        ////                EntryPtr gameEntry(gameWeakEntry.lock());
        //                EntryPtr gameEntry(boost::any_cast<EntryPtr>(connPtr->getContext()));
        //                if (gameEntry)
        //                {
        ////                  string gameSession = gameEntry->getSession();
        ////                  if(session.size() > 0 && gameSession.size() > 0 && gameSession == session)
        //                    {
        //                        // send the special user offline.
        //                        OnUserOfflineGS(gameEntry, true);

        //                        m_clientGameUserIdWeaConnMap.erase(it);
        //                        LOG_INFO <<" >>> Delete m_clientGameUserIdWeaConnMap.erase(userId) userid" << userId;
        //                    }
        //                }
        //            }
        //        }

        //        /* kick has been do before, this block code no use yet.
        //        // kick connect.
        //        {
        //            // try to delete the special game connection for client item.
        //            MutexLockGuard lockUserId(m_clientGameUserIdWeaConnMapMutex);
        //            m_clientGameUserIdWeaConnMap.erase(userId);
        //        }
        //        */
        //    }
    }

} // namespace Landy

    //========================notify message=================================
    //void ProxyServer::notifyRechargeScoreToProxyMessage(string msg)
    //{
    //    LOG_DEBUG << " >>> notifyRechargeScoreToProxyMessage msg=" << msg << " <<<";

    //    Json::Reader reader;
    //    Json::Value root;
    //    if (reader.parse(msg, root))
    //    {
    //        int64_t userId = root["userId"].asUInt();
    //        int32_t Id = root["id"].asUInt();
    //        string orderId = root["orderId"].asString();
    //        int32_t status = root["status"].asUInt();
    //        double realPay = 0;
    //        if(root["realPay"].isInt())
    //            realPay = root["realPay"].asInt();
    //        else if(root["realPay"].isDouble())
    //            realPay = root["realPay"].asDouble();
    //        else if(root["realPay"].isString())
    //            realPay = stod(root["realPay"].asString());

    //        ::Game::Common::ProxyNotifyRechargeScoreMessage response;
    //        ::Game::Common::Header *resp_header = response.mutable_header();
    //        resp_header->set_sign(HEADER_SIGN);
    //        resp_header->set_crc(0);
    //        resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
    //        resp_header->set_subid(::Game::Common::PROXY_NOTIFY_RECHARGE_SCORE_MESSAGE_REQ);

    //        response.set_userid(userId);
    //        response.set_id(Id);
    //        response.set_orderid(orderId);
    //        response.set_realpay(realPay);
    //        response.set_status(status);

    //        size_t len = response.ByteSize();
    //        vector<uint8_t> data(len);
    //        if(response.SerializeToArray(&data[0], len))
    //        {
    //            if(m_clientHallUserIdWeaConnMap.count(userId))
    //            {
    //                TcpConnectionWeakPtr weakPtr;
    //                {
    //                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
    //                    auto it = m_clientHallUserIdWeaConnMap.find(userId);
    //                    if(it != m_clientHallUserIdWeaConnMap.end())
    //                        weakPtr = it->second;
    //                }
    //                sendData(weakPtr, data);
    //            }

    ////            if(m_clientGameUserIdWeaConnMap.count(userId))
    ////            {
    ////                TcpConnectionWeakPtr weakPtr;
    ////                {
    ////                    MutexLockGuard lockUserId(m_clientGameUserIdWeaConnMapMutex);
    ////                    auto it = m_clientGameUserIdWeaConnMap.find(userId);
    ////                    if(m_clientGameUserIdWeaConnMap.end() != it)
    ////                        weakPtr = it->second;
    ////                }
    ////                sendData(weakPtr, data);
    ////            }
    //        }
    //    }
    //}

    //void ProxyServer::notifyExchangeScoreToProxyMessage(string msg)
    //{
    //    LOG_INFO << " >>> notifyExchangeScoreToProxyMessage msg=" << msg << " <<<";

    //    Json::Reader reader;
    //    Json::Value root;
    //    if (reader.parse(msg, root))
    //    {
    //        int64_t userId = root["userId"].asUInt();
    //        int32_t Id = root["id"].asUInt();
    //        string orderId = root["orderId"].asString();
    //        int32_t status = root["status"].asUInt();
    //        double realPay = 0;
    //        if(root["realPay"].isInt())
    //            realPay = root["realPay"].asInt();
    //        else if(root["realPay"].isDouble())
    //            realPay = root["realPay"].asDouble();
    //        else if(root["realPay"].isString())
    //            realPay = stod(root["realPay"].asString());

    //        ::HallServer::NotifyExchangeMessage response;
    //        ::Game::Common::Header *resp_header = response.mutable_header();
    //        resp_header->set_sign(HEADER_SIGN);
    //        resp_header->set_crc(0);
    //        resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
    //        resp_header->set_subid(::Game::Common::PROXY_NOTIFY_EXCHANGE_MESSAGE_REQ);

    //        response.set_userid(userId);
    //        response.set_id(Id);
    //        response.set_orderid(orderId);
    //        response.set_realpay(realPay);
    //        response.set_status(status);

    //        size_t len = response.ByteSize();
    //        vector<unsigned char> data(len);
    //        if(response.SerializeToArray(&data[0], len))
    //        {
    //            if(m_clientHallUserIdWeaConnMap.count(userId))
    //            {
    //                TcpConnectionWeakPtr weakPtr;
    //                {
    //                    MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
    //                    auto it  = m_clientHallUserIdWeaConnMap.find(userId);
    //                    if  (it != m_clientHallUserIdWeaConnMap.end())
    //                        weakPtr = it->second;
    //                }
    //                sendData(weakPtr, data);
    //            }
    //        }
    //    }
    //}

    //// kick preview user when new user login value.
    //void ProxyServer::notifyUserLoginMessage(string msg)
    //{
    ////    long tick = Utility::gettimetick();
    //    LOG_INFO << " >>> notifyUserLoginMessage msg=" << msg << " <<<";

    //#if 0
    //    vector<string> vec;
    //    boost::algorithm::split(vec, msg, boost::is_any_of( ":" ));
    //    if(vec.size() < 2)
    //    {
    //        return;
    //    }
    //#else
    //    Json::Reader reader;
    //    Json::Value root;
    //    if (reader.parse(msg, root))
    //    {
    //        int64_t userId = root["userId"].asInt();
    //        string session = root["session"].asString();
    ////        PrintSockerMap();
    //        SendShutDownMessageToClient(userId, session);
    //        DeleteUserIdMap(userId);
    //    }
    //#endif

    ////Cleanup:
    ////    long sub = Utility::gettimetick() - tick;
    //    LOG_INFO << " >>> refreshUserLogin end msg=" << msg;// << ", time used:" << sub;
    //}

    //void ProxyServer::notifyNoticeMessage(string msg)
    //{
    //    LOG_DEBUG << " >>> notifyNoticeMessage msg=" << msg << " <<<";

    //    ::HallServer::HallNotifyNoticeMessageFromServerMessage response;
    //    ::Game::Common::Header *resp_header = response.mutable_header();
    //    resp_header->set_sign(HEADER_SIGN);
    //    resp_header->set_crc(0);
    //    resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
    //    resp_header->set_subid(::Game::Common::HALL_NOTIFY_NEW_NOTICE_MESSAGE_REQ);

    //    response.set_message(msg);

    //    Json::Reader reader;
    //    Json::Value root;
    //    if (reader.parse(msg, root))
    //    {
    ////        int64_t userId = root["userId"].asInt();
    ////        int32_t channelId = root["channelId"].asInt();

    ////        int32_t channelId = 0;

    //        size_t len = response.ByteSize();
    //        vector<unsigned char> data(len);
    //        if(response.SerializeToArray(&data[0], len))
    //        {
    ////            if(channelId > 0)
    ////            {
    ////                MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
    ////                for(auto &it : m_clientHallUserIdWeaConnMap)
    ////                {
    ////                    EntryPtr entry(boost::any_cast<EntryPtr>(it.second.lock()->getContext()));
    ////                    if(likely(entry))
    ////                    {
    ////                        if(channelId == entry->getChannelId())
    ////                            sendData(it.second, data);
    ////                    }
    ////                }
    ////            }else
    //            {
    //                MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
    //                for(auto &it : m_clientHallUserIdWeaConnMap)
    //                {
    //                    sendData(it.second, data);
    //                }
    //            }
    //        }
    //    }
    //}

    // void ProxyServer::PrintSockerMap()
    // {
        // {
        //     MutexLockGuard lockUser(m_clientHallUserIdWeaConnMapMutex);

        //     for (auto it = m_clientHallUserIdWeaConnMap.begin(); it != m_clientHallUserIdWeaConnMap.end(); ++it)
        //     {
        //         TcpConnectionWeakPtr weakPtr = it->second;
        //         TcpConnectionPtr clientPtr(weakPtr.lock());
        //         string strEntrySession;
        //         if (clientPtr)
        //         {
        //             //                WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(clientPtr->getContext()));
        //             //                EntryPtr entry(weakEntry.lock());
        //             EntryPtr entry(boost::any_cast<EntryPtr>(clientPtr->getContext()));
        //             strEntrySession = entry->getSession();
        //             // LOG_INFO << "Hall UserId :" << it->first << " WeakConn:" << clientPtr.get() << " Session:" << strEntrySession;
        //         }
        //     }
        // }

        //    LOG_INFO << " GS: ==============================================================================";
        //    {
        //        MutexLockGuard lockUserId(m_clientGameUserIdWeaConnMapMutex);

        //        for(auto it = m_clientGameUserIdWeaConnMap.begin(); it != m_clientGameUserIdWeaConnMap.end(); ++it)
        //        {
        //            TcpConnectionWeakPtr weakPtr = it->second;
        //            TcpConnectionPtr clientPtr(weakPtr.lock());
        //             string strEntrySession;
        //            if(clientPtr)
        //            {
        ////                WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(clientPtr->getContext()));
        ////                EntryPtr entry(weakEntry.lock());
        //                EntryPtr entry(boost::any_cast<EntryPtr>(clientPtr->getContext()));
        //                strEntrySession = entry->getSession();
        //            }

        //            LOG_INFO<<"GameUserId :"<<it->first<<" WeakConn:"<<clientPtr.get()<<" Session:"<<strEntrySession;
        //        }
        //    }

        // LOG_INFO << "====================================== END ====================================== ";
    // }

// } // namespace Landy

//string ProxyServer::getRandomGameServer()
//{
//    vector<string> ips;
//    {
//        MutexLockGuard lock(m_gameServerIps_mutex);
//        ips.assign(m_game ServerIps.begin(), m_game ServerIps.end());
//    }

//    int index = GlobalFunc::RandomInt64(1, ips.size());
//    string ip = ips[index-1];
//    return ip;
//}

//void ProxyServer::reEnterRoom(const TcpConnectionPtr& conn)
//{
//    ::Game::Common::ProxyNotifyReenterRoomMessage response;
//    ::Game::Common::Header *resp_header = response.mutable_header();
//    resp_header->set_sign(HEADER_SIGN);
////    resp_header->set_crc(0);
////    resp_header->set_mainid(::Game::Common::SYSTEM_CMD);
////    resp_header->set_subid(::Game::Common::PROXY_NOTIFY_REENTER_ROOM_MESSAGE_REQ);

//    size_t len = response.ByteSize();
//    vector<unsigned char> data(len);
//    if(response.SerializeToArray(&data[0], len))
//    {
//        sendData(conn, data);
//    }
//}

//void ProxyServer::SendShutDownDataAndClose(TcpConnectionkPtr& conn, vector<uint8_t> &data)
//{
////    LOG_INFO << ">>> SendShutDownDataAndClose session="<<session<<" <<<";
////    TcpConnectionPtr clientPtr(weakPtr.lock());
//    if(conn)
//    {
////        WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(clientPtr->getContext()));
////        EntryPtr entry(weakEntry.lock());
////        EntryPtr entry(boost::any_cast<EntryPtr>(conn->getContext()));
////        string strEntrySession = entry->getSession();
////        if(strEntrySession.size() > 0 && session.size() > 0 && strEntrySession != session)
////        {
//            sendData(conn, data);

//            conn->forceCloseWithDelay(0.0);

//            LOG_INFO << ">>>>>>>>>>>>>>>>>>>>>>>> SendShutDownDataAndClose Sending  <<<<<<<<<<<<<<<<<<<<< strEntrySession:"<<strEntrySession<<" session:"<<session;
//            return;
////        }
//    }
//    LOG_INFO << ">>> SendShutDownDataAndClose NOT SEND !!!<<<";
//}

//void ProxyServer::SendShutDownMessageToClient(int64_t userId, string &session)
//{
//    LOG_INFO << ">>> SendShutDownMessageToClient userId:"<<userId<<" <<<";

//    vector<uint8_t> data;

//    if(m_clientHallUserIdWeaConnMap.count(userId))
//    {
//        TcpConnectionWeakPtr weakPtr;
//        {
//            MutexLockGuard lockUserId(m_clientHallUserIdWeaConnMapMutex);
//            auto it  = m_clientHallUserIdWeaConnMap.find(userId);
//            if  (it != m_clientHallUserIdWeaConnMap.end())
//            {
//                weakPtr = it->second;
//            }
//        }

//        GetShutDownUserClientMessage(userId, data);

//        SendShutDownDataAndClose(weakPtr, data, session);
//    }
//}

//vector<int> ProxyServer::getAllConfigId()
//{
//    vector<int> configIds;

////    auto fuc = bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
////    shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(), fuc);
////    shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { statmt->close(); delete statmt; });
////    shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

////    pstmt.reset(conn->prepareStatement("SELECT configid FROM game_room_kind"));
////    res.reset(pstmt->executeQuery());
////    while(res->next())
////    {
////        configIds.push_back(res->getInt(1));
////    }
//    return configIds;
//}
