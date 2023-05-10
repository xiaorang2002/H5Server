#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <muduo/base/Mutex.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>

#include <muduo/net/http/HttpContext.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>
#include <sstream>
#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "crypto/crypto.h"
// #include <bsoncxx/json.hpp>
#include "json/json.h"

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "RedisClient/RedisClient.h"

using namespace muduo;
using namespace muduo::net;
using namespace Landy;
using namespace std;

#include "HttpServer.h"

HttpServer::HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &name,
                       TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
      m_hall_loop_thread(new EventLoopThread()),
      m_game_loop_thread(new EventLoopThread())
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
}

void HttpServer::start()
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "HttpServer = " << server_.ipPort();
    server_.start();
}

bool HttpServer::StartEventThread()
{
    m_hall_loop_thread->startLoop();
    m_game_loop_thread->startLoop();
}

bool HttpServer::InitRedisCluster(string ip, string password)
{
    // LOG_INFO << "InitRedisCluster...";
    m_redisPassword = password;
    m_redisIPPort = ip;
    return true;
}

bool HttpServer::startThreadPool(int threadCount)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "worker 线程数 = " << threadCount;
    m_thread_pool.setThreadInitCallback(bind(&HttpServer::threadInit, this));
    m_thread_pool.start(threadCount);
    return true;
}

void HttpServer::threadInit()
{
    if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
    {
        LOG_FATAL << "RedisClient Connection ERROR!";
        return;
    }
}

bool HttpServer::InitZookeeper(string ip)
{
    //LOG_INFO << "InitZookeeper...";
    m_zookeeperclient.reset(new ZookeeperClient(ip));
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&HttpServer::ZookeeperConnectedHandler, this));
    if (m_zookeeperclient->connectServer())
    {
        return true;
    }
    else
    {
        LOG_FATAL << "zookeeperclient->connectServer error";
        abort();
        return false;
    }
}

void HttpServer::ZookeeperConnectedHandler()
{
    LOG_INFO << __FUNCTION__;
    if (ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HttpServer"))
        m_zookeeperclient->createNode("/GAME/HttpServer", "HttpServer");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServers"))
        m_zookeeperclient->createNode("/GAME/HallServers", "HallServers");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServersInvaild"))
        m_zookeeperclient->createNode("/GAME/HallServersInvaild", "HallServersInvaild");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServers"))
        m_zookeeperclient->createNode("/GAME/GameServers", "GameServers");

    if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServersInvalid"))
        m_zookeeperclient->createNode("/GAME/GameServersInvalid", "GameServersInvalid");

    vector<string> vec;
    boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
    string ip;
    GlobalFunc::getIP(m_netCardName, ip);
    //自注册HttpServer节点到zookeeper
    m_szNodeValue = ip + ":" + vec[1];
    m_szNodePath = "/GAME/HttpServer/" + m_szNodeValue;
    LOG_ERROR << "--- *** " << m_szNodePath;
    m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
    //大厅服 ip:port 值列表 ///
    {
        vector<string> hallServers;
        GetChildrenWatcherHandler getHallServersChildrenWatcherHandler = std::bind(&HttpServer::GetHallServersChildrenWatcherHandler, this,
                                                                                   placeholders::_1, placeholders::_2,
                                                                                   placeholders::_3, placeholders::_4,
                                                                                   placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/HallServers", hallServers, getHallServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_hallServerIps_mutex);
            m_hallServerIps.assign(hallServers.begin(), hallServers.end());
        }
    }
    //invaildHallServers
    {
        vector<string> invaildHallServers;
        GetChildrenWatcherHandler getInvaildHallServersChildrenWatcherHandler = std::bind(&HttpServer::GetInvaildHallServersChildrenWatcherHandler, this,
                                                                                          placeholders::_1, placeholders::_2,
                                                                                          placeholders::_3, placeholders::_4,
                                                                                          placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/HallServersInvaild", invaildHallServers, getInvaildHallServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_hallServerIps_mutex);
            m_invaildHallServerIps.assign(invaildHallServers.begin(), invaildHallServers.end());
        }
    }
    //游戏服 roomid:ip:port 值列表 ///
    {
        vector<string> gameServers;
        GetChildrenWatcherHandler getGameServersChildrenWatcherHandler = std::bind(&HttpServer::GetGameServersChildrenWatcherHandler, this,
                                                                                   placeholders::_1, placeholders::_2,
                                                                                   placeholders::_3, placeholders::_4,
                                                                                   placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/GameServers", gameServers, getGameServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_gameServerIps_mutex);
            //m_gameServers.assign(gameServers.begin(), gameServers.end());
            m_gameServerIps.assign(gameServers.begin(), gameServers.end());
        }
        // init to dump the game servers.
        {
            MutexLockGuard loak(m_room_servers_mutex);
            m_room_servers.clear();
            for (string ip : gameServers)
            {
                //   LOG_INFO << " >>> Init GET GameServers :" << ip;
                vector<string> vec;
                boost::algorithm::split(vec, ip, boost::is_any_of(":"));
                uint32_t roomId = stoi(vec[0]);

                if (m_room_servers.find(roomId) == m_room_servers.end())
                {
                    vector<string> vecIp;
                    m_room_servers.emplace(roomId, vecIp);
                }
                m_room_servers[roomId].push_back(ip);
            }
        }
    }
    //invaildGameServers
    {
        vector<string> invaildGameServers;
        GetChildrenWatcherHandler getInvaildGameServersChildrenWatcherHandler = std::bind(&HttpServer::GetInvaildGameServersChildrenWatcherHandler, this,
                                                                                          placeholders::_1, placeholders::_2,
                                                                                          placeholders::_3, placeholders::_4,
                                                                                          placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/GameServersInvalid", invaildGameServers, getInvaildGameServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_invaildGameServerIps_mutex);
            m_invaildGameServerIps.assign(invaildGameServers.begin(), invaildGameServers.end());
        }
    }
    //比赛服 roomid:ip:port 值列表 ///
    {
        vector<string> matchServers;
        GetChildrenWatcherHandler getMatchServersChildrenWatcherHandler = std::bind(&HttpServer::GetMatchServersChildrenWatcherHandler, this,
                                                                                    placeholders::_1, placeholders::_2,
                                                                                    placeholders::_3, placeholders::_4,
                                                                                    placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/MatchServers", matchServers, getMatchServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_matchServerIps_mutex);
            //m_matchServers.assign(matchServers.begin(), matchServers.end());
            m_matchServerIps.assign(matchServers.begin(), matchServers.end());
            //for(auto ip : matchServers)
            //     m_gameServerIps.push_back(ip);
        }
        // init to dump the game servers.
        {
            MutexLockGuard loak(m_match_servers_mutex);
            m_match_servers.clear();
            for (string ip : matchServers)
            {
                //  LOG_INFO << " >>> Init GET MatchServers :" << ip;
                vector<string> vec;
                boost::algorithm::split(vec, ip, boost::is_any_of(":"));
                uint32_t roomId = stoi(vec[0]);

                if (m_match_servers.find(roomId) == m_match_servers.end())
                {
                    vector<string> vecIp;
                    m_match_servers.emplace(roomId, vecIp);
                }
                m_match_servers[roomId].push_back(ip);
            }
        }
    }
    //invaildMatchServers
    {
        vector<string> invaildMatchServers;
        GetChildrenWatcherHandler getInvaildMatchServersChildrenWatcherHandler = std::bind(&HttpServer::GetInvaildMatchServersChildrenWatcherHandler, this,
                                                                                           placeholders::_1, placeholders::_2,
                                                                                           placeholders::_3, placeholders::_4,
                                                                                           placeholders::_5);
        if (ZOK == m_zookeeperclient->getClildren("/GAME/MatchServersInvalid", invaildMatchServers, getInvaildMatchServersChildrenWatcherHandler, this))
        {
            MutexLockGuard lock(m_invaildMatchServerIps_mutex);
            m_invaildMatchServerIps.assign(invaildMatchServers.begin(), invaildMatchServers.end());
        }
    }
    startAllHallServerConnection();
    startAllGameServerConnection();
    startAllMatchServerConnection();
    //LOG_INFO << " >>> zookeeper every 3.0 second to repair node path:" << m_szNodePath << ", value:" << m_szNodeValue;
}

void HttpServer::startAllHallServerConnection()
{
    LOG_INFO << __FUNCTION__;
    MutexLockGuard lock(m_hallServerIps_mutex);
    for (string ip : m_hallServerIps)
    {
        startHallServerConnection(ip);
    }
}

void HttpServer::startHallServerConnection(string ip)
{
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
        //vec：ip:port ///
        muduo::net::InetAddress serverAddr(vec[0], stoi(vec[1]));
        LOG_INFO << __FUNCTION__ << " --- *** >>> 大厅服[" << vec[0] << ":" << vec[1] << "]";
        std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_hall_loop_thread->getLoop(), serverAddr, ip));

        tcpClient->setConnectionCallback(
            bind(&HttpServer::onHallServerConnection, this, placeholders::_1));
        tcpClient->setMessageCallback(
            bind(&HttpServer::onHallServerMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
        tcpClient->enableRetry();
        tcpClient->connect();

        MutexLockGuard lock(m_hallIPServerMapMutex);
        m_hallIPServerMap[ip] = tcpClient;
    }
}

void HttpServer::ProcessHallIPServer(vector<string> &newHallServers)
{
    LOG_INFO << __FUNCTION__;
    vector<string> addIP;
    set<string> newIPSet(newHallServers.begin(), newHallServers.end());
    set<string> oldIPSet;
    {
        MutexLockGuard lock(m_hallServerIps_mutex);
        for (string ip : m_hallServerIps)
        {
            oldIPSet.emplace(ip);
        }
    }
    //查找m_hallServerIps中有，而newHallServers中没有的 ///
    vector<string> diffIPS(oldIPSet.size());
    vector<string>::iterator it;
    it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());

    {
        MutexLockGuard lock(m_hallIPServerMapMutex);
        for (string ip : diffIPS)
        {
            auto it = m_hallIPServerMap.find(ip);
            if (m_hallIPServerMap.end() != it)
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                {
                    addIP.push_back(ip);
                    //                    tcpClient->disconnect();
                }
                else
                {
                    //LOG_WARN << "GetHallServersChildrenWatcherHandler ProxyServer::ProcessHallIPServer Delete HallServer IP:"<<ip;
                    m_hallIPServerMap.erase(it);
                }
                //                m_hallIPServerMap.erase(it);
            }
        }
    }
    //查找newHallServers中有，而m_hallServerIps中没有的 ///
    diffIPS.clear();
    diffIPS.resize(newIPSet.size());
    it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());
    for (string ip : diffIPS)
    {
        //LOG_WARN << "ProxyServer::ProcessHallIPServer ADD HallServer IP:"<<ip;
        startHallServerConnection(ip);
    }

    {
        MutexLockGuard lock(m_hallServerIps_mutex);
        m_hallServerIps.assign(newHallServers.begin(), newHallServers.end());
        for (int i = 0; i < addIP.size(); ++i)
            m_hallServerIps.push_back(addIP[i]);
    }
}

void HttpServer::GetHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                      const string &path, void *context)
{
    //LOG_INFO << " >>> zookeeper callback, type:" << type << ", state:" << state;

    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetHallServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> newHallServers;
    string hallPath = "/GAME/HallServers";
    if (ZOK == m_zookeeperclient->getClildren(hallPath, newHallServers, getChildrenWatcherHandler, this))
    {
        ProcessHallIPServer(newHallServers);
    }

    //     for(string ip : hallServers)
    //     {
    //         //LOG_INFO << " GetHallServersChildrenWatcherHandler :"<<ip;
    //     }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HttpServer::GetInvaildHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
{
    // LOG_INFO << " >>> zookeeper callback, type:" << type << ", state:" << state;

    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetInvaildHallServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> hallServers;
    string hallPath = "/GAME/HallServersInvaild";
    if (ZOK == m_zookeeperclient->getClildren(hallPath, hallServers, getChildrenWatcherHandler, this))
    {
        m_invaildHallServerIps.assign(hallServers.begin(), hallServers.end());
    }

    for (string ip : m_invaildHallServerIps)
    {
        //LOG_INFO << " GetInvaildHallServersChildrenWatcherHandler :"<<ip;
    }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

// void HttpServer::RefreshHallServers()
// {
//     vector<string> hallServers;
//     string hallPath = "/GAME/HallServers";
//     if(ZOK == m_zookeeperclient->getClildrenNoWatch(hallPath, hallServers))
//     {
//         ProcessHallIPServer(hallServers);
//     }
//
//     for(string ip : hallServers)
//     {
//       //  LOG_INFO << " GetHallServersChildrenWatcherHandler :"<<ip;
//     }
// }

void HttpServer::startAllGameServerConnection()
{
    LOG_INFO << __FUNCTION__;
    MutexLockGuard lock(m_gameServerIps_mutex);
    for (string ip : m_gameServerIps)
    {
        startGameServerConnection(ip);
    }
}

void HttpServer::startGameServerConnection(string ip)
{
    bool bFind = false;
    auto it = m_gameIPServerMap.find(ip);
    if (m_gameIPServerMap.end() == it)
    {
        bFind = false;
    }
    else
    {
        std::shared_ptr<TcpClient> tcpClient(it->second);
        if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
            bFind = true;
    }

    if (!bFind)
    {
        vector<string> vec;
        boost::algorithm::split(vec, ip, boost::is_any_of(":"));
        //vec：roomid:ip:port ///
        muduo::net::InetAddress serverAddr(vec[1], stoi(vec[2]));
        LOG_INFO << __FUNCTION__ << " --- *** >>> 游戏服[" << vec[1] << ":" << vec[2] << "] 房间号[" << vec[0] << "]";
        std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_game_loop_thread->getLoop(), serverAddr, ip));

        tcpClient->setConnectionCallback(
            bind(&HttpServer::onGameServerConnection, this, placeholders::_1));
        tcpClient->setMessageCallback(
            bind(&HttpServer::onGameServerMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
        tcpClient->enableRetry();
        tcpClient->connect();

        MutexLockGuard lock(m_gameIPServerMapMutex);
        m_gameIPServerMap[ip] = tcpClient;
    }
}

//===========================game server=================================
void HttpServer::onGameServerConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "HTTP服[" << conn->localAddress().toIpPort() << "] -> 游戏服["
             << conn->peerAddress().toIpPort() << "] "
             << (conn->connected() ? "UP" : "DOWN");

    string ip = conn->peerAddress().toIpPort();
    if (conn->connected())
    {
    }
}

void HttpServer::onGameServerMessage(const muduo::net::TcpConnectionPtr &conn,
                                     muduo::net::Buffer *buf,
                                     muduo::Timestamp receiveTime)
{

    //    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    //    {
    //        // FIXME: use Buffer::peekInt32()
    //        const uint16_t len = buf->peekInt16();
    //        if(unlikely(len > PACKET_SIZE || len < sizeof(uint16_t)))
    //        {
    //            LOG_ERROR << "onGameServerMessage invalid length to shutdown socket. length:" << len;
    //            if(conn)
    //                conn->shutdown();  // FIXME: disable reading
    //            break;
    //        }else if (likely(buf->readableBytes() >= len))
    //        {
    //            BufferPtr buffer(new muduo::net::Buffer(len));
    //            buffer->append(buf->peek(), static_cast<size_t>(len));
    //            buf->retrieve(len);

    //            internal_prev_header *pre_header = (internal_prev_header *)buffer->peek();
    //            string session(pre_header->session, sizeof(pre_header->session));

    //        }else
    //        {
    //            break;
    //        }
    //    }
}

void HttpServer::ProcessGameIPServer(vector<string> &newGameServers)
{
    LOG_INFO << __FUNCTION__;
    vector<string> addIP;
    set<string> newIPSet(newGameServers.begin(), newGameServers.end());
    set<string> oldIPSet;
    {
        MutexLockGuard lock(m_gameServerIps_mutex);
        for (string ip : m_gameServerIps)
        {
            oldIPSet.emplace(ip);
        }
    }
    //查找m_gameServerIps中有，而newGameServers中没有的 ///
    vector<string> diffIPS(oldIPSet.size());
    vector<string>::iterator it;
    it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());

    {
        MutexLockGuard lock(m_gameIPServerMapMutex);
        for (string ip : diffIPS)
        {
            auto it = m_gameIPServerMap.find(ip);
            if (m_gameIPServerMap.end() != it)
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                {
                    addIP.push_back(ip);
                }
                else
                {
                    //  LOG_WARN << "getGameServerChildrenWatcherHandler ProxyServer::ProcessGameIPServer Delete GameServer IP:"<<ip;
                    m_gameIPServerMap.erase(it);
                }
                //                m_gameIPServerMap.erase(it);
            }
        }
    }
    //查找newGameServers中有，而m_gameServerIps中没有的 ///
    diffIPS.clear();
    diffIPS.resize(newIPSet.size());
    it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());
    for (string ip : diffIPS)
    {
        startGameServerConnection(ip);
    }

    {
        MutexLockGuard lock(m_gameServerIps_mutex);
        m_gameServerIps.assign(newGameServers.begin(), newGameServers.end());
        for (int i = 0; i < addIP.size(); ++i)
            m_gameServerIps.push_back(addIP[i]);
    }
}

void HttpServer::GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                      const string &path, void *context)
{

    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetGameServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> newGameServers;
    string gamePath = "/GAME/GameServers";
    if (ZOK == m_zookeeperclient->getClildren(gamePath, newGameServers, getChildrenWatcherHandler, this))
    {
        //游戏服 roomid:ip:port 值列表
        MutexLockGuard loak(m_room_servers_mutex);
        m_room_servers.clear();
        for (string ip : newGameServers)
        {
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));
            uint32_t roomId = stoi(vec[0]);

            if (m_room_servers.find(roomId) == m_room_servers.end())
            {
                vector<string> vecIp;
                m_room_servers.emplace(roomId, vecIp);
            }
            m_room_servers[roomId].push_back(ip);
        }

        //比赛跟普通游戏用同一个map
        //         for(auto ip : matchServers)
        //             Servers.push_back(ip);

        ProcessGameIPServer(newGameServers);
    }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HttpServer::GetInvaildGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
{
    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetInvaildGameServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> InvaildgameServers;
    string gamePath = "/GAME/GameServersInvalid";
    if (ZOK == m_zookeeperclient->getClildren(gamePath, InvaildgameServers, getChildrenWatcherHandler, this))
    {
        //ProcessGameIPServer(gameServers);
        MutexLockGuard lock(m_invaildGameServerIps_mutex);
        m_invaildGameServerIps.assign(InvaildgameServers.begin(), InvaildgameServers.end());
    }

    for (string ip : InvaildgameServers)
    {
        //  LOG_INFO << " GetInvaildGameServersChildrenWatcherHandler :"<<ip;
    }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HttpServer::GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                       const string &path, void *context)
{

    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetMatchServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> newMatchServers;
    //matchServers.clear();
    string gamePath = "/GAME/MatchServers";
    if (ZOK == m_zookeeperclient->getClildren(gamePath, newMatchServers, getChildrenWatcherHandler, this))
    {
        //比赛服 roomid:ip:port 值列表
        MutexLockGuard loak(m_match_servers_mutex);
        m_match_servers.clear();
        for (string ip : newMatchServers)
        {
            vector<string> vec;
            boost::algorithm::split(vec, ip, boost::is_any_of(":"));
            uint32_t roomId = stoi(vec[0]);

            if (m_match_servers.find(roomId) == m_match_servers.end())
            {
                vector<string> vecIp;
                m_match_servers.emplace(roomId, vecIp);
            }
            m_match_servers[roomId].push_back(ip);
        }

        //比赛跟普通游戏用同一个map
        //for(auto ip : gameServers)
        //    Servers.push_back(ip);
        //连接跟普通游戏的共用一个
        //ProcessGameIPServer(Servers);
        ProcessMatchIPServer(newMatchServers);
    }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

void HttpServer::GetInvaildMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr, const string &path, void *context)
{
    GetChildrenWatcherHandler getChildrenWatcherHandler = bind(&HttpServer::GetInvaildMatchServersChildrenWatcherHandler, this,
                                                               placeholders::_1, placeholders::_2,
                                                               placeholders::_3, placeholders::_4,
                                                               placeholders::_5);

    vector<string> InvaildMatchServers;
    string gamePath = "/GAME/MatchServersInvalid";
    if (ZOK == m_zookeeperclient->getClildren(gamePath, InvaildMatchServers, getChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_invaildMatchServerIps_mutex);
        m_invaildMatchServerIps.assign(InvaildMatchServers.begin(), InvaildMatchServers.end());
    }

    for (string ip : InvaildMatchServers)
    {
        // LOG_INFO << " GetInvaildMatchServersChildrenWatcherHandler :"<<ip;
    }

    //     LOG_INFO << "type:" <<type;
    //     LOG_INFO << " ZOO_CREATED_EVENT: " <<ZOO_CREATED_EVENT;
    //     LOG_INFO << " ZOO_DELETED_EVENT: " << ZOO_DELETED_EVENT;
    //     LOG_INFO << " ZOO_CHANGED_EVENT: " << ZOO_CHANGED_EVENT;
    //     LOG_INFO << " ZOO_CHILD_EVENT: " << ZOO_CHILD_EVENT;
    //     LOG_INFO << " ZOO_SESSION_EVENT: " << ZOO_SESSION_EVENT;
    //     LOG_INFO << " ZOO_NOTWATCHING_EVENT: " << ZOO_NOTWATCHING_EVENT;
}

// void HttpServer::RefreshGameServers()
// {
//     vector<string> gameServers;
//     string gamePath = "/GAME/GameServers";
//     if(ZOK == m_zookeeperclient->getClildrenNoWatch(gamePath, gameServers))
//     {
//         ProcessGameIPServer(gameServers);
//     }
//
//     for(string ip : gameServers)
//     {
//     //    LOG_INFO << " getGameServersChildrenWatcherHandler :"<<ip;
//     }
// }

void HttpServer::startAllMatchServerConnection()
{
    LOG_INFO << __FUNCTION__;
    MutexLockGuard lock(m_matchServerIps_mutex);
    for (string ip : m_matchServerIps)
    {
        startMatchServerConnection(ip);
    }
}

void HttpServer::startMatchServerConnection(string ip)
{
    bool bFind = false;
    auto it = m_gameIPServerMap.find(ip);
    if (m_gameIPServerMap.end() == it)
    {
        bFind = false;
    }
    else
    {
        std::shared_ptr<TcpClient> tcpClient(it->second);
        if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
            bFind = true;
    }

    if (!bFind)
    {
        vector<string> vec;
        boost::algorithm::split(vec, ip, boost::is_any_of(":"));
        //vec：roomid:ip:port ///
        muduo::net::InetAddress serverAddr(vec[1], stoi(vec[2]));
        LOG_INFO << __FUNCTION__ << " --- *** >>> 比赛服[" << vec[1] << ":" << vec[2] << "] 房间号[" << vec[0] << "]";
        std::shared_ptr<TcpClient> tcpClient(new TcpClient(m_game_loop_thread->getLoop(), serverAddr, ip));

        tcpClient->setConnectionCallback(
            bind(&HttpServer::onMatchServerConnection, this, std::placeholders::_1));
        tcpClient->setMessageCallback(
            bind(&HttpServer::onMatchServerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcpClient->enableRetry();
        tcpClient->connect();

        MutexLockGuard lock(m_gameIPServerMapMutex);
        m_gameIPServerMap[ip] = tcpClient;
    }
}

void HttpServer::ProcessMatchIPServer(vector<string> &newMatchServers)
{
    LOG_INFO << __FUNCTION__;
    vector<string> addIP;
    set<string> newIPSet(newMatchServers.begin(), newMatchServers.end());
    set<string> oldIPSet;
    {
        MutexLockGuard lock(m_matchServerIps_mutex);
        for (string ip : m_matchServerIps)
        {
            oldIPSet.emplace(ip);
        }
    }
    //查找m_matchServerIps中有，而newMatchServers中没有的 ///
    vector<string> diffIPS(oldIPSet.size());
    vector<string>::iterator it;
    it = set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());

    {
        MutexLockGuard lock(m_gameIPServerMapMutex);
        for (string ip : diffIPS)
        {
            auto it = m_gameIPServerMap.find(ip);
            if (m_gameIPServerMap.end() != it)
            {
                std::shared_ptr<TcpClient> tcpClient(it->second);
                if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
                {
                    addIP.push_back(ip);
                }
                else
                {
                    m_gameIPServerMap.erase(it);
                }
            }
        }
    }
    //查找newMatchServers中有，而m_matchServerIps中没有的 ///
    diffIPS.clear();
    diffIPS.resize(newIPSet.size());
    it = set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
    diffIPS.resize(it - diffIPS.begin());
    for (string ip : diffIPS)
    {
        startMatchServerConnection(ip);
    }

    {
        MutexLockGuard lock(m_matchServerIps_mutex);
        m_matchServerIps.assign(newMatchServers.begin(), newMatchServers.end());
        for (int i = 0; i < addIP.size(); ++i)
            m_matchServerIps.push_back(addIP[i]);
    }
}

//===========================match server=================================
void HttpServer::onMatchServerConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "HTTP服[" << conn->localAddress().toIpPort() << "] -> 比赛服["
             << conn->peerAddress().toIpPort() << "] "
             << (conn->connected() ? "UP" : "DOWN");
    string ip = conn->peerAddress().toIpPort();
    if (conn->connected())
    {
    }
}

void HttpServer::onMatchServerMessage(const muduo::net::TcpConnectionPtr &conn,
                                      muduo::net::Buffer *buf,
                                      muduo::Timestamp receiveTime)
{
}

//===========================hall server=============================
void HttpServer::onHallServerConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "HTTP服[" << conn->localAddress().toIpPort() << "] -> 大厅服["
             << conn->peerAddress().toIpPort() << "] "
             << (conn->connected() ? "UP" : "DOWN");

    string ip = conn->peerAddress().toIpPort();
    if (conn->connected())
    {
    }
}

void HttpServer::onHallServerMessage(const TcpConnectionPtr &conn,
                                     muduo::net::Buffer *buf,
                                     muduo::Timestamp receiveTime)
{
    //    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    //    {
    //        // FIXME: use Buffer::peekInt32()

    //    }
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << __FUNCTION__ << " --- *** "
             << "WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
             << conn->localAddress().toIpPort() << "] "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected())
    {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime)
{
    // LOG_INFO << __FUNCTION__;
    HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());

    if (!context->parseRequest(buf, receiveTime))
    {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    if (context->gotAll())
    {
        TcpConnectionWeakPtr weakConn(conn);
        m_thread_pool.run(bind(&HttpServer::onRequest, this, weakConn, context->request()));
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionWeakPtr &weakConn, const HttpRequest &req)
{
    // LOG_INFO << __FUNCTION__;
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);

    MessageHandler(req, response);

    Buffer buf;
    response.appendToBuffer(&buf);

    TcpConnectionPtr conn = weakConn.lock();
    if (conn)
    {
        conn->send(&buf);
    }
    else
        return;

    if (response.closeConnection())
    {
        conn->forceCloseWithDelay(0.1);
    }
}

bool HttpServer::waitForResqBuffer(const TcpConnectionPtr &conn, BufferPtr &buf, string &session)
{
    while (1)
    {
        if (conn->inputBuffer()->readableBytes() >= kHeaderLen)
        {
            const uint64_t len = conn->inputBuffer()->peekInt16();
            if (unlikely(len > PACKET_SIZE || len < sizeof(int16_t)))
            {
                if (conn)
                    conn->shutdown(); // FIXME: disable reading
            }
            else if (likely(conn->inputBuffer()->readableBytes() >= len))
            {
                internal_prev_header *pre_header = (internal_prev_header *)conn->inputBuffer()->peek();
                if (0 != memcmp(session.c_str(), pre_header->session, sizeof(pre_header->session)))
                    continue;

                buf.reset(new muduo::net::Buffer(len));
                buf->append(conn->inputBuffer()->peek(), static_cast<size_t>(len));
                conn->inputBuffer()->retrieve(len);

                Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
                int headLen = sizeof(internal_prev_header) + sizeof(Header);
                uint16_t crc = GlobalFunc::GetCheckSum((uint8_t *)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);
                if (commandHeader->len == len - sizeof(internal_prev_header) && commandHeader->crc == crc && commandHeader->ver == PROTO_VER && commandHeader->sign == HEADER_SIGN)
                {
                    return true;
                }
                else
                    return false;
            }
        }
        if (conn->disconnected())
        {
            break;
        }
    }
    return false;
}

void HttpServer::parseQuery(const string &str, map<string, string> &mapQuery)
{
    try
    {
        //HTTP请求 http://ip:5847
        LOG_INFO << __FUNCTION__ << " --- *** 查询参数: " << str;
        //string str="http://123132/ddda?aa=2&bb=3";
        auto space = find(str.begin(), str.end(), ' ');
        auto query = find(str.begin(), str.end(), '?');
        string qy(query, space);
        //cout<<qy<<endl;
        map<string, string> paramters;
        while (query != space)
        {
            query++; //delete '?'
            auto equel = std::find(query, space, '=');
            auto separate = std::find(query, space, '&');
            mapQuery.emplace(string(query, equel), string(equel + 1, separate));
            query = separate;
        }
        for (auto paramter : mapQuery)
        {
            LOG_INFO << __FUNCTION__ << " --- *** " << paramter.first << "=" << paramter.second;
        }
    }
    catch (exception &exp)
    {
        cout << exp.what() << "paramers err!!" << endl;
    }
}

std::string RequestStr(const muduo::net::HttpRequest &req)
{
    std::stringstream ss;
    for (std::map<string, string>::const_iterator it = req.headers().begin();
         it != req.headers().end(); ++it)
    {
        ss << it->first << ": " << it->second;
    }
    ss << "\nmethod: " << req.methodString()
       << "\npath: " << req.path()
       << "\nquery: " << req.query() << endl;
    return ss.str();
}

//按照占位符来替换 ///
static void replace(string &json, const string &placeholder, const string &value)
{
    boost::replace_all<string>(json, "\"" + placeholder + "\"", value);
}

void HttpServer::MessageHandler(const muduo::net::HttpRequest &req, muduo::net::HttpResponse &resp)
{
    resp.setStatusCode(HttpResponse::k200Ok);
    resp.setStatusMessage("OK");
    resp.addHeader("Server", "TXQP");

    //HTTP请求 http://ip:5847
    LOG_WARN << __FUNCTION__ << " --- *** HTTP请求\n"
             << RequestStr(req);

    //解析参数
    stringstream ss;
    std::map<std::string, std::string> params;
    parseQuery(req.query(), params);

    LOG_WARN << __FUNCTION__ << " --- *** params.size()[" << params.size() << "]";

    if (req.path() == "/")
    {
        resp.setContentType("text/html;charset=utf-8");
        string now = Timestamp::now().toFormattedString();
        resp.setBody("try /help </h1>");
    }
    else if (req.path() == "/refreashConfig")
    {
        resp.setContentType("text/plain;charset=utf-8");
        int32_t GameId = 0;
        int32_t RoomId = 0;
        if (params.size() > 0)
        {
            map<std::string, std::string>::iterator paramKey = params.find("GameId");
            if (paramKey != params.end())
            { // && IsDigitStr(paramKey->second)
                GameId = atol(paramKey->second.c_str());
            }
            paramKey = params.find("RoomId");
            if (paramKey != params.end())
            { // && IsDigitStr(paramKey->second)
                RoomId = atol(paramKey->second.c_str());
            }
        }

        boost::property_tree::ptree root;
        root.put("GameId", ":GameId");
        root.put("RoomId", ":RoomId");

        stringstream s;
        boost::property_tree::json_parser::write_json(s, root, false);
        string json = s.str();
        replace(json, ":GameId", std::to_string(GameId));
        replace(json, ":RoomId", std::to_string(RoomId));
        json = boost::regex_replace(json, boost::regex("\n|\r"), "");

        LOG_WARN << __FUNCTION__ << " --- *** paramKey[" << GameId << "][" << RoomId << "],jsonMsg[" << json << "]";

        REDISCLIENT.publishRefreashConfigMessage(json);
        resp.setBody("sucess");
    }
    else if (req.path() == "/repairServer")
    {
        resp.setContentType("text/plain;charset=utf-8");
        if (repairGameServer(req.query()))
        {
            resp.setBody("sucess");
        }
        else
        {
            resp.setBody("failure");
        }
    }
    else if (req.path() == "/allGameServerInfo")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string info;
        allGameServerInfo(info);
        resp.setBody(info);
    }
    else if (req.path() == "/allGamePlayerInfo")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string info;
        allGamePlayerInfo(info);
        resp.setBody(info);
    }
    else if (req.path() == "/repairMatch")
    {
        resp.setContentType("text/plain;charset=utf-8");
        if (repairMatchServer(req.query()))
        {
            resp.setBody("sucess");
        }
        else
        {
            resp.setBody("failure");
        }
    }
    else if (req.path() == "/allMatchServerInfo")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string info;
        allMatchServerInfo(info);
        resp.setBody(info);
    }
    else if (req.path() == "/allMatchPlayerInfo")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string info;
        allMatchPlayerInfo(info);
        resp.setBody(info);
    }
    else if (req.path() == "/allHallServerInfo")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string info;
        allHallServerInfo(info);
        resp.setBody(info);
    }
    else if (req.path() == "/repairHallServer")
    {
        resp.setContentType("text/plain;charset=utf-8");
        if (repairHallServer(req.query()))
        {
            resp.setBody("sucess");
        }
        else
        {
            resp.setBody("failure");
        }
    }
    else if (req.path() == "/agentGamePlayers")
    {
        resp.setContentType("text/plain;charset=utf-8");
        string str;
        agentGamePlayers(str);
        resp.setBody(str);
    }
    else if (req.path() == "/help")
    {
        resp.setContentType("text/html;charset=utf-8");
        resp.setBody("<html>"
                     "<head><title>help</title></head>"
                     "<body>"
                     "<h4>help</h4>"
                     "/repairServer?GameServer=4001:192.168.0.1:5847&status=(0不维护/1百人类维护/2对战类不开下局维护)<br/>"
                     "/allGamePlayerInfo (所有游戏人数信息)<br/>"
                     "/allGameServerInfo (所有在线游戏服状态)<br/>"
                     "/repairMatch?MatchServer=4001:192.168.0.1:5847&status=(0不维护/1维护)<br/>"
                     "/allMatchPlayerInfo (所有比赛人数信息)<br/>"
                     "/allMatchServerInfo (所有在线比赛服状态)<br/>"
                     "/refreashConfig (刷新所有配置)<br/>"
                     "/allHallServerInfo (所有在线大厅信息)<br/>"
                     "/agentGamePlayers (所有代理下玩家分布信息)<br/>"
                     "/repairHallServer?HallServer=192.168.2.158:20001 &status=1(维护)"
                     "</body>"
                     "</html>");
    }
    else
    {
        resp.setBody("<html><head><title>httpServer</title></head>"
                     "<body><h1>error,please check paramters again</h1>"
                     "</body></html>");
    }
}

void HttpServer::allGameServerInfo(string &str)
{
    // LOG_INFO << __FILE__ << " " << __FUNCTION__;
    Json::Value root;
    try
    {
        MutexLockGuard lock(m_room_servers_mutex);

        for (auto &room_server : m_room_servers)
        {
            map<string, uint64_t> mapPlayerNum;
            REDISCLIENT.GetGameRoomplayerNum(room_server.second, mapPlayerNum);
            string infoStr;
            for (auto &ip : room_server.second)
            {
                Json::Value value;
                value["address"] = ip;
                if (mapPlayerNum[ip])
                {
                    value["num"] = mapPlayerNum[ip];
                }
                else
                {
                    value["num"] = 0;
                }
                int version;
                string nodeValue;
                list<string>::iterator findIter = std::find(m_invaildGameServerIps.begin(), m_invaildGameServerIps.end(), ip);
                if (m_invaildGameServerIps.end() != findIter)
                {
                    m_zookeeperclient->getNodeValue("/GAME/GameServersInvalid/" + ip, nodeValue, version);
                    value["status"] = nodeValue;
                }
                else
                {
                    value["status"] = "0";
                }
                root.append(value);
            }
            str += infoStr;
        }
        Json::FastWriter writer;
        str = writer.write(root);
        //  LOG_INFO<<str;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================allGameServerInfo=";
    }
}

void HttpServer::allMatchServerInfo(string &str)
{
    //   LOG_INFO << __FILE__ << " " << __FUNCTION__;
    Json::Value root;
    try
    {
        MutexLockGuard lock(m_match_servers_mutex);

        for (auto &room_server : m_match_servers)
        {
            map<string, uint64_t> mapPlayerNum;
            REDISCLIENT.GetGameRoomplayerNum(room_server.second, mapPlayerNum);
            string infoStr;
            for (auto &ip : room_server.second)
            {
                Json::Value value;
                value["address"] = ip;
                if (mapPlayerNum[ip])
                {
                    value["num"] = mapPlayerNum[ip];
                }
                else
                {
                    value["num"] = 0;
                }
                int version;
                string nodeValue;
                list<string>::iterator findIter = std::find(m_invaildMatchServerIps.begin(), m_invaildMatchServerIps.end(), ip);
                if (m_invaildMatchServerIps.end() != findIter)
                {
                    m_zookeeperclient->getNodeValue("/GAME/MatchServersInvalid/" + ip, nodeValue, version);
                    value["status"] = nodeValue;
                }
                else
                {
                    value["status"] = "0";
                }
                root.append(value);
            }
            str += infoStr;
        }
        Json::FastWriter writer;
        str = writer.write(root);
        //    LOG_INFO<<str;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================allMatchServerInfo=";
    }
}

bool HttpServer::repairGameServer(const string &str)
{
    map<string, string> queryMap;
    parseQuery(str, queryMap);
    if (queryMap.count("GameServer") && queryMap.count("status"))
    {
        auto it = m_gameIPServerMap.find(queryMap["GameServer"]);
        if (m_gameIPServerMap.end() != it)
        {
            std::shared_ptr<TcpClient> tcpClient(it->second);
            if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
            {
                ::Game::Common::HttpNotifyRepairServerResp resp;
                resp.set_status(stoi(queryMap["status"]));
                int32_t respLen = resp.ByteSizeLong();

                //为了跟其他消息保持一致
                uint16_t len = sizeof(internal_prev_header) + sizeof(Header) + respLen;
                vector<uint8_t> data(len);
                internal_prev_header *internal_header = (internal_prev_header *)&data[0];
                memset(&data[0], 0, len);
                internal_header->len = len;
                internal_header->userId = 0;

                string aesKey = Landy::Crypto::generateAESKey();
                string session = Landy::Crypto::BufferToHexString((unsigned char *)aesKey.c_str(), aesKey.size());
                memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
                memcpy(internal_header->aesKey, aesKey.c_str(), (sizeof(internal_header->aesKey)));
                GlobalFunc::SetCheckSum(internal_header);

                resp.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], respLen);

                Header *header = (Header *)&data[sizeof(internal_prev_header)];
                header->len = sizeof(Header) + respLen;
                header->ver = PROTO_VER;
                header->sign = HEADER_SIGN;
                header->mainId = ::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER;
                header->subId = ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER;
                header->encType = PUBENC__PROTOBUF_NONE;
                header->reserve = 0;
                header->reqId = 0;
                header->realSize = respLen;
                header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

                cout << "size=" << data.size() << endl;
                TcpConnectionPtr conn = tcpClient->connection();
                conn->send(data.data(), data.size());

                BufferPtr buf;
                if (waitForResqBuffer(conn, buf, session))
                {
                    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    {
                        ::Game::Common::HttpNotifyRepairServerResp msg;
                        if (msg.ParseFromArray(commandHeader + sizeof(Header), commandHeader->realSize))
                        {
                            LOG_INFO << "---repairGameServer :" << queryMap["GameServer "] << "status" << msg.status();
                            return msg.status();
                        }
                    }

                    return true;
                }
            }
        }
    }
    return false;
}

bool HttpServer::repairMatchServer(const string &str)
{
    map<string, string> queryMap;
    parseQuery(str, queryMap);
    if (queryMap.count("MatchServer") && queryMap.count("status"))
    {
        auto it = m_gameIPServerMap.find(queryMap["MatchServer"]);
        if (m_gameIPServerMap.end() != it)
        {
            std::shared_ptr<TcpClient> tcpClient(it->second);
            if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
            {
                ::Game::Common::HttpNotifyRepairServerResp resp;
                resp.set_status(stoi(queryMap["status"]));
                int32_t respLen = resp.ByteSizeLong();

                //为了跟其他消息保持一致
                uint16_t len = sizeof(internal_prev_header) + sizeof(Header) + respLen;
                vector<uint8_t> data(len);
                internal_prev_header *internal_header = (internal_prev_header *)&data[0];
                memset(&data[0], 0, len);
                internal_header->len = len;
                internal_header->userId = 0;

                string aesKey = Landy::Crypto::generateAESKey();
                string session = Landy::Crypto::BufferToHexString((unsigned char *)aesKey.c_str(), aesKey.size());
                memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
                memcpy(internal_header->aesKey, aesKey.c_str(), (sizeof(internal_header->aesKey)));
                GlobalFunc::SetCheckSum(internal_header);

                resp.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], respLen);

                Header *header = (Header *)&data[sizeof(internal_prev_header)];
                header->len = sizeof(Header) + respLen;
                header->ver = PROTO_VER;
                header->sign = HEADER_SIGN;
                header->mainId = ::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER;
                header->subId = ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER;
                header->encType = PUBENC__PROTOBUF_NONE;
                header->reserve = 0;
                header->reqId = 0;
                header->realSize = respLen;
                header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

                cout << "size=" << data.size() << endl;
                TcpConnectionPtr conn = tcpClient->connection();
                conn->send(data.data(), data.size());

                BufferPtr buf;
                if (waitForResqBuffer(conn, buf, session))
                {
                    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    {
                        ::Game::Common::HttpNotifyRepairServerResp msg;
                        if (msg.ParseFromArray(commandHeader + sizeof(Header), commandHeader->realSize))
                        {
                            LOG_INFO << "---repairMatchServer :" << queryMap["MatchServer "] << "status" << msg.status();
                            return msg.status();
                        }
                    }

                    return true;
                }
            }
        }
    }
    return false;
}

void HttpServer::allGamePlayerInfo(string &str)
{
    //  LOG_INFO << __FILE__ << " " << __FUNCTION__;
    Json::Value root;
    try
    {
        MutexLockGuard lock(m_room_servers_mutex);
        for (auto &room_server : m_room_servers)
        {
            map<string, uint64_t> mapPlayerNum;
            REDISCLIENT.GetGameRoomplayerNum(room_server.second, mapPlayerNum);
            uint64_t gamePlayerNum = 0;
            for (auto &ip : room_server.second)
            {
                gamePlayerNum += mapPlayerNum[ip];
            }
            Json::Value value;
            value["GoomID"] = (int)room_server.first / 10;
            value["RoomID"] = room_server.first;
            value["PlayersCount"] = gamePlayerNum;
            root.append(value);
            //str += (to_string(room_server.first)+":"+to_string(gamePlayerNum)+"</br>");
        }
        Json::FastWriter write;
        str = write.write(root);
        cout << str << endl;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================allGamePlayerInfo";
    }
}

void HttpServer::allMatchPlayerInfo(string &str)
{
    // LOG_INFO << __FILE__ << " " << __FUNCTION__;
    Json::Value root;
    try
    {
        MutexLockGuard lock(m_match_servers_mutex);
        for (auto &match_server : m_match_servers)
        {
            map<string, uint64_t> mapPlayerNum;
            REDISCLIENT.GetGameRoomplayerNum(match_server.second, mapPlayerNum);
            uint64_t gamePlayerNum = 0;
            for (auto &ip : match_server.second)
            {
                gamePlayerNum += mapPlayerNum[ip];
            }
            Json::Value value;
            value["GoomID"] = (int)match_server.first / 10;
            value["RoomID"] = match_server.first;
            value["PlayersCount"] = gamePlayerNum;
            root.append(value);
            //str += (to_string(room_server.first)+":"+to_string(gamePlayerNum)+"</br>");
        }
        Json::FastWriter write;
        str = write.write(root);
        cout << str << endl;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================allMatchPlayerInfo";
    }
}

//大厅维护应该也是需要的，如果不提前维护可能导致MQ正在处理的消息丢失，正在登录或者正在请求的客户端无响应
void HttpServer::allHallServerInfo(string &str)
{
    Json::Value root;
    try
    {
        MutexLockGuard lock(m_hallIPServerMapMutex);
        for (auto &hall_server : m_hallIPServerMap)
        {
            //string infoStr;
            Json::Value value;
            string ip = hall_server.first;
            value["address"] = ip;
            list<string>::iterator findIter = std::find(m_invaildHallServerIps.begin(), m_invaildHallServerIps.end(), ip);
            if (m_invaildHallServerIps.end() != findIter)
            {
                //infoStr+=ip+":status:Repairing </br>";
                value["status"] = "1";
            }
            else
            {
                //infoStr+=ip+":status:Running </br>";
                value["status"] = "0";
            }
            //str += infoStr ;
            root.append(value);
        }
        Json::FastWriter writer;
        str = writer.write(root);
        //  LOG_INFO<<str;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================allHallServerInfo=";
    }
}

bool HttpServer::repairHallServer(const string &str)
{
    map<string, string> queryMap;
    parseQuery(str, queryMap);
    if (queryMap.count("HallServer") && queryMap.count("status"))
    {
        //tcp -> hallserver
        auto it = m_hallIPServerMap.find(queryMap["HallServer"]);
        if (m_hallIPServerMap.end() != it)
        {
            std::shared_ptr<TcpClient> tcpClient(it->second);
            if (tcpClient && tcpClient->connection() && tcpClient->connection()->connected())
            {
                ::Game::Common::HttpNotifyRepairServerResp resp;
                resp.set_status(stoi(queryMap["status"]));
                int32_t respLen = resp.ByteSizeLong();

                //为了跟其他消息保持一致
                uint16_t len = sizeof(internal_prev_header) + sizeof(Header) + respLen;
                vector<uint8_t> data(len);
                internal_prev_header *internal_header = (internal_prev_header *)&data[0];
                memset(&data[0], 0, len);
                internal_header->len = len;
                internal_header->userId = 0;
                string aesKey = Landy::Crypto::generateAESKey();
                string session = Landy::Crypto::BufferToHexString((unsigned char *)aesKey.c_str(), aesKey.size());
                memcpy(internal_header->session, session.c_str(), sizeof(internal_header->session));
                memcpy(internal_header->aesKey, aesKey.c_str(), (sizeof(internal_header->aesKey)));
                GlobalFunc::SetCheckSum(internal_header);

                resp.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], respLen);

                Header *header = (Header *)&data[sizeof(internal_prev_header)];
                header->len = sizeof(Header) + respLen;
                header->ver = PROTO_VER;
                header->sign = HEADER_SIGN;
                header->mainId = ::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER;
                header->subId = ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER;
                header->encType = PUBENC__PROTOBUF_NONE;
                header->reserve = 0;
                header->reqId = 0;
                header->realSize = respLen;
                header->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], header->len - 4);

                cout << "size=" << data.size() << endl;
                TcpConnectionPtr conn = tcpClient->connection();

                conn->send(data.data(), data.size());
                BufferPtr buf;
                if (waitForResqBuffer(conn, buf, session))
                {
                    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    {
                        ::Game::Common::HttpNotifyRepairServerResp msg;
                        if (msg.ParseFromArray(commandHeader + sizeof(Header), commandHeader->realSize))
                        {
                            LOG_INFO << "---repairHallServer :" << queryMap["HallServer "] << "status " << msg.status();
                            return msg.status();
                        }
                    }

                    return true;
                }
            }
        }
    }
    return false;
}
//代理下的玩家分布包括比赛的
bool HttpServer::agentGamePlayers(string &str)
{
    // LOG_INFO << __FILE__ << " " << __FUNCTION__;
    try
    {
        MutexLockGuard lock(m_room_servers_mutex);
        Json::Reader reader;
        Json::Value value;
        uint32_t agentid = 0;
        map<uint32_t, map<uint32_t, uint32_t>> agentGamePlayersMap;
        auto packFunc = [&](uint32_t roomid, vector<string> &Ips) {
            vector<string> AgentStr;
            REDISCLIENT.GetGameAgentPlayerNum(Ips, AgentStr);
            uint64_t gamePlayerNum = 0;
            for (auto &str : AgentStr)
            {
                reader.parse(str, value);
                Json::Value::Members paths = value.getMemberNames();
                for (auto &path : paths)
                {
                    agentid = stod(path);
                    if (agentGamePlayersMap.find(agentid) != agentGamePlayersMap.end())
                    {
                        if (agentGamePlayersMap[agentid].find(roomid) == agentGamePlayersMap[agentid].end())
                        {
                            agentGamePlayersMap[agentid].emplace(roomid, value[path].asInt());
                        }
                        else
                        {
                            agentGamePlayersMap[agentid][roomid] += value[path].asInt();
                        }
                    }
                    else
                    {
                        map<uint32_t, uint32_t> agentMap{pair<uint32_t, uint32_t>(roomid, value[path].asInt())};
                        agentGamePlayersMap.emplace(agentid, agentMap);
                    }
                }
            }
        };

        for (auto &room_server : m_room_servers)
        {
            packFunc(room_server.first, room_server.second);
        }

        for (auto &match_server : m_match_servers)
        {
            packFunc(match_server.first, match_server.second);
        }

        Json::Value root;
        for (auto &agentMap : agentGamePlayersMap)
        {
            Json::Value info;
            info["agentid"] = agentMap.first;
            Json::Value roomInfo;
            for (auto &roomPlayer : agentMap.second)
            {
                Json::Value gamePlyaerInfo;
                gamePlyaerInfo["roomid"] = roomPlayer.first;
                gamePlyaerInfo["gameid"] = (int)(roomPlayer.first / 100) * 10;
                // gamePlyaerInfo["gameid"]=(int)roomPlayer.first/10;
                gamePlyaerInfo["PlayersCount"] = roomPlayer.second;
                roomInfo.append(gamePlyaerInfo);
            }
            info["roomInfo"] = roomInfo;
            root.append(info);
        }
        Json::FastWriter write;
        str = write.write(root);
        return true;
    }
    catch (exception excep)
    {
        LOG_ERROR << "================agentGamePlayers";
    }
}
