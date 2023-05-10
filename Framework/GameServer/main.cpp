#include <iostream>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
//#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include "GameServer.h"
#include "public/ConsoleClr.h"

using namespace std;

std::atomic<bool> gRequestShutdown(false);
Landy::GameServer *gGameServer = NULL;
muduo::net::EventLoop *gloop = NULL;

// idle scconds.
static int kIdleSeconds = 3;

// is debug value.
int g_bisDebug = 0;
int g_bisDisEncrypt = 0;

// logging global define.
int g_EastOfUtc = 60*60*8;
static int kRoolSize = 1024*1024*500;
muduo::AsyncLogging* g_asyncLog = NULL;

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define GREEN        "\033[0;32;32m"


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
    gRequestShutdown = true;
    if(gGameServer)
    {
        gGameServer->quit();
    }

    if(gloop)
    {
        gloop->quit();
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

//    if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
//        return false;
//    printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
//    rlmt.rlim_cur = (rlim_t)655350;
//    rlmt.rlim_max  = (rlim_t)655359;
//    if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
//        return false;

    return true;
}

void SetLibraryPathEnv()
{
//    char *old_library_path = getenv("LD_LIBRARY_PATH");
    string path = ".";
    setenv("LD_LIBRARY_PATH", path.c_str(), false);
}

// game server main function call.
int main(int argc, char* argv[])
{
    srand((uint32_t)time(NULL));

    SetLibraryPathEnv();

    if(!SetRLIMIT())
        return -1;

    int gameId = 0;
    int roomId = 0;

    char * strRoomId = getenv("GameConfigId");
    if(!strRoomId && argc < 3)
    {
        LOG_INFO << "argc < 2, Please Set GameId GameRoomId!";
        return 1;
    }
    gameId = strtol(argv[1], NULL, 10);
    roomId = strtol(argv[2], NULL, 10);

    muduo::net::EventLoop loop;
    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/GameServer/"))
    {
        boost::filesystem::create_directories("./log/GameServer/");
    }
    muduo::AsyncLogging log(::basename("GameServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;

    LOG_INFO << "pid = " << getpid();

    //==========config==========
    if(boost::filesystem::exists("./conf/game.conf"))
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_ini("./conf/game.conf", pt);

        // ===== loglevel ======
        int loglevel = pt.get<int>("Global.loglevel",1);
        muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
		LOG_INFO << __FUNCTION__ << " --- *** " << "日志级别 = " << loglevel;

        std::string netcardName = pt.get<string>("Global.netcardName","eth0");
		LOG_INFO << __FUNCTION__ << " --- *** " << "网卡名称 = " << netcardName;

        //=========Zookeeper==========
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

        //=====mysql=====
        string mysql_ip = pt.get<string>("mysql.IP", "192.168.2.29");
        uint16_t mysql_port = pt.get<int>("mysql.Port", 3306);
        string mysql_dbname = pt.get<string>("mysql.DBName","user_recored");
        string mysql_user = pt.get<string>("mysql.User","root");
        string mysql_password = pt.get<string>("mysql.Password","123456");
        int mysql_max_connection = pt.get<int>("mysql.Len", 0);
        int db_password_len = pt.get<int>("mysql.Len", 0);


        //=====MongoDB=====
        string strMongoDBUrl = pt.get<string>("MongoDB.Url");

        //===== GameServer IP Port =====
        string keyGsip = "GameServer.IP";
        string keyGsport = "GameServer.Port";
        string keyThreadCount = "GameServer.ThreadCount";
        if(roomId)
        {
            keyGsip   = "GameServer_" + to_string(roomId) + ".IP";
            keyGsport = "GameServer_" + to_string(roomId) + ".Port";
            keyThreadCount = "GameServer_" + to_string(roomId) + ".ThreadCount";
        }

        string strGameServerIP = pt.get<string>(keyGsip, "192.168.2.75");
        int16_t threadCount = pt.get<int>(keyThreadCount, 10);
        uint16_t gameServerPort = pt.get<int>(keyGsport, 30009);
        gameServerPort = 30000 + roomId;

        LOG_INFO << " >>> GameServer start at "<< strGameServerIP << ":" << gameServerPort;

        muduo::net::InetAddress serverAddr(gameServerPort);
        Landy::GameServer server(&loop, serverAddr, kIdleSeconds, netcardName);
		///////////////////////////////////////
		//网络IO线程，IO收发(recv/send) /////////
		server.setThreadNum(Landy::LOOP_THREAD_COUNT);

        if(server.loadKey() &&
           //server.startDB(mysql_ip, mysql_port, mysql_dbname, mysql_user, mysql_password, db_password_len, mysql_max_connection)&&
           server.InitMongoDB(strMongoDBUrl) &&
           server.InitRedisCluster(strRedisServerIP, redisPassword) &&
           server.InitZookeeper(strZookeeperServerIP, gameId, roomId))
        {

#if 0
            // init zookeeper.
            if (g_bisDebug) {
                LOG_DEBUG << " >>> debug mode, server.InitDebugInfo called.";
                // try to initialize debug info.
                server.InitDebugInfo(g_bisDebug);
            }
#endif

            // try to initialize game data now.
            if(server.InitServer(gameId, roomId))
            {
                gGameServer = &server;
        //        registerSignalHandler(SIGTERM, Stop);
                //interrupt信号捕抓
                registerSignalHandler(SIGINT, Stop);
				///////////////////////////////////////
				//worker线程，处理游戏业务逻辑 ///////////
                server.start(threadCount);
                loop.loop();
            }
        }
    }   else
    {
        LOG_ERROR<<"./conf/game.conf not exists!";
    }

//Cleanup:
    return 0;
}
