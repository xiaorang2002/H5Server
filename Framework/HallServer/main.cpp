// Copyright (c) 2019, Landy
// All rights reserved.

#include <iostream>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include "HallServer.h"
#include "public/ConsoleClr.h"

int g_bisDebug = 0;
std::string g_gsIp;

int g_nConfigId = 0;

static int kIdleSeconds = 3;


//std::atomic<bool> gRequestShutdown(false);
//Landy::HallServer *gHallServer = NULL;
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
//    if(gHallServer)
//    {
//        gHallServer->quit();
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

    if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
        return false;
    //printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
    rlmt.rlim_cur = (rlim_t)655350;
    rlmt.rlim_max  = (rlim_t)655359;
    if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
        return false;

    return true;
}


int main(int argc, char* argv[])
{
    if(!SetRLIMIT())
        return -1;

//    char * strServerId = getenv("HallConfigId");
//    // command line.
//    if(argc >= 2)
//    {
//        g_nConfigId = stoi(argv[1]);
//    }

//    // envoriment set.
//    if(strServerId)
//    {
//        g_nConfigId = atoi(strServerId);
//    }

    muduo::net::EventLoop loop;
    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/HallServer/"))
    {
        boost::filesystem::create_directories("./log/HallServer/");
    }
    muduo::AsyncLogging log(::basename("HallServer"), kRoolSize);
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

    std::string netcardName = pt.get<string>("Global.netcardName","eth0");
	LOG_INFO << __FUNCTION__ << " --- *** " << "网卡名称 = " << netcardName;

    //=====zookeeper=====
    string strZookeeperServerIP = "";
    auto childs = pt.get_child("Zookeeper");
    for(auto &child : childs)
    {
        if(child.first.substr(0, 7) == "Server.")
        {
            if(!strZookeeperServerIP.empty())
                strZookeeperServerIP += ",";
            strZookeeperServerIP += child.second.get_value<string>();
        }
    }
	LOG_INFO << __FUNCTION__ << " --- *** " << "ZookeeperIP = " << strZookeeperServerIP;

    //=====RedisCluster=====
    vector<string> addrVec;
    map<string, string> addrMap;
    string strRedisServerIP = "";
    string redisPassword = pt.get<string>("RedisCluster.Password", "");
    childs = pt.get_child("RedisCluster");
    for(auto &child : childs)
    {
        if(child.first.substr(0, 9)  == "Sentinel.")
        {
            if(!strRedisServerIP.empty())
                strRedisServerIP += ",";
            strRedisServerIP += child.second.get_value<string>();
        }else if(child.first.substr(0, 12)  == "SentinelMap.")
        {
            string temp = child.second.get_value<string>();
            addrVec.clear();
            boost::algorithm::split(addrVec, temp, boost::is_any_of( "," ));
            if(addrVec.size() == 2)
                addrMap[addrVec[0]] = addrVec[1];
        }
    }
	LOG_INFO << __FUNCTION__ << " --- *** " << "RedisClusterIP = " << strRedisServerIP;

    //=====MongoDB=====
    string strMongoDBUrl = pt.get<string>("MongoDB.Url");

    //=====HallServer IP Port=====
    string keyLoginip = "HallServer.IP";
    string keyLoginport = "HallServer.Port";
    string keyThreadCount = "HallServer.ThreadCount";
    string keyGsip    = "Global.gsip";
    if (g_nConfigId)
    {
        // try to get the special config data from special address now.
        keyLoginip     = "HallServer_" + to_string(g_nConfigId) + ".IP";
        keyLoginport   = "HallServer_" + to_string(g_nConfigId) + ".Port";
        keyThreadCount = "HallServer_" + to_string(g_nConfigId) + "ThreadCount";
        keyGsip        = "HallServer_" + to_string(g_nConfigId) + ".gsip";
    }

    // initialize the special game server ip and port content value now.
    string strHallServerIP  = pt.get<string>(keyLoginip, "127.0.0.1");
    uint16_t hallServerPort = pt.get<int>(keyLoginport, 8120);
    int16_t threadCount     = pt.get<int>(keyThreadCount, 10);

    //========RocketMQ========
    string strNameServerIp =  pt.get<string>("RocketMQ.NameServer", "192.168.2.75:9876");
//    string strGroupName =  pt.get<string>("RocketMQ.GroupName", "TianXia");
//    string strTopic =  pt.get<string>("RocketMQ.Topic", "GameInfo");
//    string strTag =  pt.get<string>("RocketMQ.Tag", "*");


    //HallServer_Report
    //=====HallServer_Report====
    map<int, int> report_money_map;
    int gameId = 0, money = 0;
    childs = pt.get_child("HallServer_Report");
    for(auto &child : childs)
    {
        gameId = stoi(child.first);
        money  = child.second.get_value<int>();
        report_money_map[gameId] = money;
    }

	LOG_INFO << __FUNCTION__ << " --- *** " << "HallServer = " << strHallServerIP << ":" << hallServerPort;

    //g_gsIp = pt.get<string>(keyGsip,"gameserver.com:26710");

    muduo::net::InetAddress serverAddr(hallServerPort);
    Landy::HallServer server(&loop, serverAddr, kIdleSeconds, netcardName);
	///////////////////////////////////////
	//网络IO线程，IO收发(recv/send) /////////
    server.setThreadNum(Landy::LOOP_THREAD_COUNT);
//    server.SetReportMoneyMap(report_money_map);


    //=====mysql=====
    string mysql_ip = pt.get<string>("mysql.IP", "192.168.2.29");
    uint16_t mysql_port = pt.get<int>("mysql.Port", 3306);
    string mysql_dbname = pt.get<string>("mysql.DBName","user_recored");
    string mysql_user = pt.get<string>("mysql.User","root");
    string mysql_password = pt.get<string>("mysql.Password","123456");
    int mysql_max_connection = pt.get<int>("mysql.Len", 0);
    int db_password_len = pt.get<int>("mysql.Len", 0);


    // start hall server.
    if(//server.loadKey() &&
       server.InitMongoDB(strMongoDBUrl) &&
       server.startDB(mysql_ip, mysql_port, mysql_dbname, mysql_user, mysql_password, db_password_len, mysql_max_connection)&&
       server.InitRedisCluster(strRedisServerIP, redisPassword) &&
       server.InitZookeeper(strZookeeperServerIP) &&
       server.startThreadPool(threadCount))// &&
    //    server.InitRocketMQ(strNameServerIp)

    {
//        gHallServer = &server;
//        registerSignalHandler(SIGTERM, Stop);
//        registerSignalHandler(SIGINT, Stop);
		///////////////////////////////////////
		//worker线程，处理游戏业务逻辑 ///////////
        server.start(threadCount);
        loop.loop();
    }

    LOG_INFO << "HallServer Exited.";
    return 0;
}
