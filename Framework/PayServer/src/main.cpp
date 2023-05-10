#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/InetAddress.h>

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
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <sys/wait.h>
#include <sys/resource.h>

#include "public/GlobalFunc.h"
#include "public/ConsoleClr.h"

#include "proto/Game.Common.pb.h"
//#include "proto/PayServer.Message.pb.h"
//#include "proto/GameServer.Message.pb.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "PayServer.h"

int g_bisDebug = 0;
std::string g_gsIp;

int g_nConfigId = 0;

//static int kIdleSeconds = 3;

// std::atomic<bool> gRequestShutdown(false);
PayServer *gTransferServer = NULL;
muduo::net::EventLoop *gloop = NULL;

using namespace std;
static int kRoolSize = 1024*1024*500;
int g_EastOfUtc = 60*60*8;
muduo::AsyncLogging* g_asyncLog = NULL;

static void registerSignalHandler(int signal, void(*handler)(int))
{
   struct sigaction sa;
   sa.sa_handler = handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigaction(signal, &sa, nullptr);
}

static void Stop(int signo)
{
    //gRequestShutdown = true;
    printf("Stop signo is:%d, getpid is:%d\n", signo, (int)getpid());

    if (gloop)
    {
        printf("Stop gloop quit 1\n");
        gloop->quit();
    }

    if (gTransferServer)
    {
        printf("Stop signo quit 1\n");//is:%d, getpid is:%d\n", signo, (int)getpid());
        gTransferServer->quit();
    }
}

bool SetRLIMIT()
{
    struct rlimit rlmt;
    if(getrlimit(RLIMIT_CORE, &rlmt) == -1)
        return false;
    printf("Before set rlimit CORE dump current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
    rlmt.rlim_cur = 1024*1024*1024;
    rlmt.rlim_max = 1024*1024*1024;
    if(setrlimit(RLIMIT_CORE, &rlmt) == -1)
        return false; 
    return true;
}


int main(int argc, char* argv[])
{
    if(!SetRLIMIT())
        return -1;
    // 支持
    int32_t http_port = 0;
    if( argc == 2 ) {
        http_port = strtol(argv[1], NULL, 10);
        LOG_INFO << "---argv[1],http_port["<< argv[1]<<"]";
    }

    muduo::net::EventLoop loop;
    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/PayServer/"))
    {
        boost::filesystem::create_directories("./log/PayServer/");
    }
    muduo::AsyncLogging log(::basename("PayServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;

    LOG_INFO << "---PayServer pid = " << getpid();

    //=====config=====
    if(!boost::filesystem::exists("./conf/pay.conf"))
    {
        LOG_INFO << "./conf/pay.conf not exists";
        return 1;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/pay.conf", pt);

    //==== global test for is debug.
    g_bisDebug = pt.get<int>("Global.isdebug",0);

    // ===== loglevel ======
    int loglevel = pt.get<int>("Global.loglevel",1);
    muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << " --- " << "日志级别 = " << loglevel;

    //获取指定网卡ipaddr
    std::string netcardName = pt.get<std::string>("Global.netcardName","eth0");
	std::string strIpAddr;
	GlobalFunc::getIP(netcardName, strIpAddr);

	LOG_INFO << " --- " << "网卡名称 = " << netcardName <<" strIpAddr["<<strIpAddr<<"]";

    //=====zookeeper=====
    std::string strZookeeperServerIP = "";
    auto childs = pt.get_child("Zookeeper");
    for(auto &child : childs)
    {
        if(child.first.substr(0, 7) == "Server.")
        {
            if(!strZookeeperServerIP.empty())
                strZookeeperServerIP += ",";
            strZookeeperServerIP += child.second.get_value<std::string>();
        }
    }
	LOG_INFO << "--- " << "ZookeeperIP = " << strZookeeperServerIP;

    //=====RedisCluster=====
    std::vector<std::string> addrVec;
    map<std::string, std::string> addrMap;
    std::string strRedisServerIP = "";
    std::string redisPassword = pt.get<std::string>("RedisCluster.Password", "");
    childs = pt.get_child("RedisCluster");
    for(auto &child : childs)
    {
        if(child.first.substr(0, 9)  == "Sentinel.")
        {
            if(!strRedisServerIP.empty())
                strRedisServerIP += ",";
            strRedisServerIP += child.second.get_value<std::string>();
        }
        else if(child.first.substr(0, 12)  == "SentinelMap.")
        {
            std::string temp = child.second.get_value<std::string>();
            addrVec.clear();
            boost::algorithm::split(addrVec, temp, boost::is_any_of( "," ));
            if(addrVec.size() == 2)
                addrMap[addrVec[0]] = addrVec[1];
        }
    }
	LOG_INFO << " --- " << "RedisClusterIP = " << strRedisServerIP;

    //=====MongoDB=====
    std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");

    //IP地址
    //strIpAddr = "PayServer.IP";
    //TCP监听相关
    std::string strTcpPort = "PayServer.tcpPort";
    std::string strTcpNumThreads = "PayServer.tcpNumThreads";
    std::string strTcpWorkerNumThreads = "PayServer.tcpWorkerNumThreads";
    //HTTP监听相关
    std::string strHttpPort = "PayServer.httpPort";
    std::string strHttpNumThreads = "PayServer.httpNumThreads";
    std::string strHttpWorkerNumThreads = "PayServer.httpWorkerNumThreads";
    
    std::string keyGsip    = "Global.gsip";

    //TCP监听相关
    uint16_t tcpPort = pt.get<int>(strTcpPort, 9010);
    int16_t tcpNumThreads = pt.get<int>(strTcpNumThreads, 4);
    int16_t tcpWorkerNumThreads = pt.get<int>(strTcpWorkerNumThreads, 4);
    //HTTP监听相关
	uint16_t httpPort = pt.get<int>(strHttpPort, 9070);
	int16_t httpNumThreads = pt.get<int>(strHttpNumThreads, 2);
	int16_t httpWorkerNumThreads = pt.get<int>(strHttpWorkerNumThreads, 2);
    if ( http_port > 0 ){
        httpPort = http_port;
    }
    
    LOG_WARN << "---HttpPort["<< httpPort << "],httpNumThreads[" << httpNumThreads << "]";
 
    InetAddress listenAddr(tcpPort);
    InetAddress listenAddrHttp(httpPort);
    PayServer _pay_server(&loop, listenAddr, listenAddrHttp, strIpAddr,netcardName);
 
    // start  _pay_server.
    if(_pay_server.LoadIniConfig() &&
       _pay_server.InitMongoDB(strMongoDBUrl) &&
       _pay_server.InitRedisCluster(strRedisServerIP, redisPassword) &&
       _pay_server.InitZookeeper(strZookeeperServerIP))
    {
        gTransferServer = &_pay_server;
        _pay_server.startThreadPool(tcpWorkerNumThreads);
        _pay_server.startHttpServer(httpNumThreads, httpWorkerNumThreads);
        _pay_server.start(tcpNumThreads, tcpWorkerNumThreads);
        loop.loop();
    }
   
    // CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
    // curl_global_cleanup();

    LOG_INFO << "---PayServer Exited.";
    return 0;
}
