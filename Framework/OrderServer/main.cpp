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
//#include "proto/OrderServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "OrderServer.h"

int g_bisDebug = 0;
std::string g_gsIp;

int g_nConfigId = 0;

//static int kIdleSeconds = 3;

//std::atomic<bool> gRequestShutdown(false);
//Landy::OrderServer *gOrderServer = NULL;
muduo::net::EventLoop *gloop = NULL;
using namespace std;
static int kRoolSize = 1024*1024*500;
int g_EastOfUtc = 60*60*8;
muduo::AsyncLogging* g_asyncLog = NULL;

//static void registerSignalHandler(int signal, void(*handler)(int))
//{
//    struct sigaction sa;
//    sa.sa_handler = handler;
//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = 0;
//    sigaction(signal, &sa, nullptr);
//}

//static void Stop(int signo)
//{
//    gRequestShutdown = true;
//    if(gOrderServer)
//    {
//        gOrderServer->quit();
//    }

//    if(gloop)
//    {
//        gloop->quit();
//    }
//}

bool SetRLIMIT()
{
    struct rlimit rlmt;
    if(getrlimit(RLIMIT_CORE, &rlmt) == -1)
        return false;
    //printf("Before set rlimit CORE dump current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
    rlmt.rlim_cur = 1024*1024*1024;
    rlmt.rlim_max = 1024*1024*1024;
    if(setrlimit(RLIMIT_CORE, &rlmt) == -1)
        return false;

//     if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
//         return false;
//     //printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
//     rlmt.rlim_cur = (rlim_t)655350;
//     rlmt.rlim_max  = (rlim_t)655359;
//     if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
//         return false;

    return true;
}

int main(int argc, char* argv[])
{
    if(!SetRLIMIT())
        return -1;
    muduo::net::EventLoop loop;
    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/OrderServer/"))
    {
        boost::filesystem::create_directories("./log/OrderServer/");
    }
    muduo::AsyncLogging log(::basename("OrderServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;

    //LOG_INFO << "pid = " << getpid();

    //=====config=====
    if(!boost::filesystem::exists("./conf/game.conf"))
    {
        LOG_INFO << "./conf/game.conf not exists";
        return 1;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/game.conf", pt);

    //==== global test for is debug.
    g_bisDebug = pt.get<int>("Global.isdebug",0);

    // ===== loglevel ======
    int loglevel = pt.get<int>("Global.loglevel",1);
    muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << __FUNCTION__ << " --- *** " << "日志级别 = " << loglevel;

    //获取指定网卡ipaddr
    std::string netcardName = pt.get<std::string>("Global.netcardName","eth0");
	std::string strIpAddr;
	GlobalFunc::getIP(netcardName, strIpAddr);

	LOG_INFO << __FUNCTION__ << " --- *** " << "网卡名称 = " << netcardName;

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
	LOG_INFO << __FUNCTION__ << " --- *** " << "ZookeeperIP = " << strZookeeperServerIP;

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
        }else if(child.first.substr(0, 12)  == "SentinelMap.")
        {
            std::string temp = child.second.get_value<std::string>();
            addrVec.clear();
            boost::algorithm::split(addrVec, temp, boost::is_any_of( "," ));
            if(addrVec.size() == 2)
                addrMap[addrVec[0]] = addrVec[1];
        }
    }
	LOG_INFO << __FUNCTION__ << " --- *** " << "RedisClusterIP = " << strRedisServerIP;

    //=====MongoDB=====
    std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");

    //IP地址
    //strIpAddr = "OrderServer.IP";
    //TCP监听相关
    std::string strTcpPort = "OrderServer.tcpPort";
    std::string strTcpNumThreads = "OrderServer.tcpNumThreads";
    std::string strTcpWorkerNumThreads = "OrderServer.tcpWorkerNumThreads";
    //HTTP监听相关
    std::string strHttpPort = "OrderServer.httpPort";
    std::string strHttpNumThreads = "OrderServer.httpNumThreads";
    std::string strHttpWorkerNumThreads = "OrderServer.httpWorkerNumThreads";

    std::string keyGsip    = "Global.gsip";
//     if (g_nConfigId)
//     {
//         // try to get the special config data from special address now.
//         strIpAddr = "OrderServer_" + to_string(g_nConfigId) + ".IP";
//         tcpPort = "OrderServer_" + to_string(g_nConfigId) + ".Port";
//         tcpNumThreads = "OrderServer_" + to_string(g_nConfigId) + "ThreadCount";
//         keyGsip        = "OrderServer_" + to_string(g_nConfigId) + ".gsip";
//     }

    // initialize the special game server ip and port content value now.
    //strIpAddr = pt.get<std::string>(strIpAddr, "127.0.0.1");
    //TCP监听相关
    uint16_t tcpPort = pt.get<int>(strTcpPort, 8120);
    int16_t tcpNumThreads = pt.get<int>(strTcpNumThreads, 10);
    int16_t tcpWorkerNumThreads = pt.get<int>(strTcpWorkerNumThreads, 10);
    //HTTP监听相关
	uint16_t httpPort = pt.get<int>(strHttpPort, 8120);
	int16_t httpNumThreads = pt.get<int>(strHttpNumThreads, 10);
	int16_t httpWorkerNumThreads = pt.get<int>(strHttpWorkerNumThreads, 10);
    bool isdecrypt = pt.get<int>("OrderServer.isdecrypt", 0);              //0明文HTTP请求，1加密HTTP请求
    int whiteListControl = pt.get<int>("OrderServer.whiteListControl", 0);//白名单控制
    int kMaxConnections = pt.get<int>("OrderServer.kMaxConnections", 15000); //最大连接数
    int kTimeoutSeconds = pt.get<int>("OrderServer.kTimeoutSeconds", 3); //连接超时值(s)
    int kDeltaTime = pt.get<int>("OrderServer.kDeltaTime", 20);//间隔时间(s)打印一次
    int kMaxQueueSize = pt.get<int>("OrderServer.kMaxQueueSize", 1000);//Worker单任务队列大小
    std::string strAdminList = pt.get<std::string>("OrderServer.adminList", "192.168.2.93,");//管理员挂维护/恢复服务

	int ttlUserLockSeconds = pt.get<int>("OrderServer.ttlUserLockSeconds", 1000);//上下分操作间隔时间(针对用户) ///
	int ttlAgentLockSeconds = pt.get<int>("OrderServer.ttlAgentLockSeconds", 1000);//上下分操作间隔时间(针对代理) ///
    int orderIdExpiredSeconds = pt.get<int>("OrderServer.orderIdExpiredSeconds", 30 * 60);//订单缓存过期时间
    int redLockContinue = pt.get<int>("OrderServer.redLockContinue", 0);
    //========RocketMQ========
    std::string strNameServerIp =  pt.get<std::string>("RocketMQ.NameServer", "192.168.2.75:9876");
//  std::string strGroupName =  pt.get<std::string>("RocketMQ.GroupName", "TianXia");
//  std::string strTopic =  pt.get<std::string>("RocketMQ.Topic", "GameInfo");
//  std::string strTag =  pt.get<std::string>("RocketMQ.Tag", "*");


    //OrderServer_Report
    //=====OrderServer_Report====
//     map<int, int> report_money_map;
//     int gameId = 0, money = 0;
//     childs = pt.get_child("OrderServer_Report");
//     for(auto &child : childs)
//     {
//         gameId = stoi(child.first);
//         money  = child.second.get_value<int>();
//         report_money_map[gameId] = money;
//     }

	
    //g_gsIp = pt.get<std::string>(keyGsip,"gameserver.com:26710");

    muduo::net::InetAddress listenAddr(tcpPort);
    muduo::net::InetAddress listenAddrHttp(httpPort);
    OrderServer server(&loop, listenAddr, listenAddrHttp);
    server.strIpAddr_ = strIpAddr;
    server.isdecrypt_ = isdecrypt;
    server.whiteListControl_ = (eWhiteListCtrl)whiteListControl;
    server.kMaxConnections_ = kMaxConnections;
    server.kTimeoutSeconds_ = kTimeoutSeconds;
    server.ttlUserLockSeconds_ = ttlUserLockSeconds;
    server.ttlAgentLockSeconds_ = ttlAgentLockSeconds;
    server.orderIdExpiredSeconds_ = orderIdExpiredSeconds;
    server.redLockContinue_ = (redLockContinue == 1);
	{
		std::vector<std::string> vec;
		boost::algorithm::split(vec, strAdminList, boost::is_any_of(","));
		for (std::vector<std::string>::const_iterator it = vec.begin();
			it != vec.end(); ++it) {
			std::string const& ipaddr = *it;
			if (!ipaddr.empty() &&
                boost::regex_match(ipaddr,
				boost::regex(
					"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				server.admin_list_[addr.ipNetEndian()] = eApiVisit::Enable;
				LOG_INFO << __FUNCTION__ << " --- *** " << "管理员IP[" << ipaddr << "]";
			}
		}
	}
#ifdef _STAT_ORDER_QPS_
    server.deltaTime_ = kDeltaTime;
#endif
    //redis分布式锁 ///
	std::string strRedisLock = "";
	childs = pt.get_child("RedisLock");
	for (auto& child : childs) {
		if (child.first.substr(0, 9) == "Sentinel.") {
			if (!strRedisLock.empty())
				strRedisLock += ",";
			strRedisLock += child.second.get_value<std::string>();
		}
	}
	boost::algorithm::split(server.redlockVec_, strRedisLock, boost::is_any_of(","));

    ///////////////////////////////////////
	//网络IO线程，IO收发(recv/send) /////////
    //server.setThreadNum(Landy::LOOP_THREAD_COUNT);
//    server.SetReportMoneyMap(report_money_map);

    // start hall server.
    if(//server.loadKey() &&
       server.InitMongoDB(strMongoDBUrl) &&
       server.InitRedisCluster(strRedisServerIP, redisPassword) &&
       server.InitZookeeper(strZookeeperServerIP) /*&&
       server.InitRocketMQ(strNameServerIp)*/
            )
    {
//        gOrderServer = &server;
//        registerSignalHandler(SIGTERM, Stop);
//        registerSignalHandler(SIGINT, Stop);
        server.LoadIniConfig();

		
        ///////////////////////////////////////
		//worker线程，处理游戏业务逻辑 ///////////
        server.startHttpServer(httpNumThreads, httpWorkerNumThreads, kMaxQueueSize);
        server.start(tcpNumThreads, tcpWorkerNumThreads);
        loop.loop();
    }

    LOG_INFO << "OrderServer Exited.";
    return 0;
}
