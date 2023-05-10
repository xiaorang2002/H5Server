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
#include <muduo/base/AsyncLogging.h>

#include <muduo/base/TimeZone.h>
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
#include <ctype.h> 
#include <sys/resource.h>

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
#include "public/RedisClient/RedisClient.h"

#include "public/ConsoleClr.h"

using namespace muduo;
using namespace muduo::net;
using namespace Landy;
using namespace  std;

#include "HttpServer.h"

static int kRoolSize = 1024*1024*1024;
int g_EastOfUtc = 60*60*8;
muduo::AsyncLogging* g_asyncLog = NULL;

void SetLibraryPathEnv()
{
//    char *old_library_path = getenv("LD_LIBRARY_PATH");
    string path = ".";
    setenv("LD_LIBRARY_PATH", path.c_str(), false);
    path = "/usr/local/lib64";
    setenv("LD_LIBRARY_PATH", path.c_str(), false);
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


int main(int args,char* argv[])
{

    if (!SetRLIMIT())
        return -1;

    int numThreads = 1;
    if (args > 1)
    {
      Logger::setLogLevel(Logger::WARN);
      numThreads = atoi(argv[1]);
    }
    SetLibraryPathEnv();

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./conf/game.conf"))
    {
        LOG_INFO << "./conf/game.conf not exists";
        return 1;
    }

    if(!boost::filesystem::exists("./log/HttpServer/"))
    {
        boost::filesystem::create_directories("./log/HttpServer/");
    }

    muduo::AsyncLogging log(::basename("HttpServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/game.conf", pt);

    // ===== loglevel ======
    int loglevel = pt.get<int>("Global.loglevel",1);
    muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << __FUNCTION__ << " --- *** " << "��־���� = " << loglevel;

    std::string netcardName = pt.get<string>("Global.netcardName","eth0");
    LOG_INFO << __FUNCTION__ << " --- *** " << "�������� = " << netcardName;

    //=====zookeeper=====
    string strZookeeperServerIP = "";
    auto childs = pt.get_child("Zookeeper");
    for(auto &child : childs)
    {
        //LOG_INFO<<"Zookeeper child:"<<child.first;
        
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

    EventLoop loop;
    HttpServer server(&loop, InetAddress(5847), "HttpServer");
    server.setThreadNum(numThreads);
    if(server.StartEventThread()&&
        server.InitRedisCluster(strRedisServerIP, redisPassword) &&
        server.startThreadPool(1)&&
        server.InitZookeeper(strZookeeperServerIP)){

            // registerSignalHandler(SIGINT, Stop);
            server.start();
            loop.loop();
        }
}
