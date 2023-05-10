#include <iostream>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/algorithm.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>


#include "ProxyServer.h"


static int kIdleSeconds = 3;

const int LOOP_THREAD_COUNT = 8;

//std::atomic<bool> gRequestShutdown(false);
//Landy::ProxyServer *gProxyServer = NULL;
//muduo::net::EventLoop *gloop = NULL;


using namespace std;
//using namespace  Landy;


static int kRoolSize = 1024*1024*1024;
int g_EastOfUtc = 60*60*8;
muduo::AsyncLogging* g_asyncLog = NULL;

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define GREEN        "\033[0;32;32m"

void asyncOutput(const char *msg, int len)
{
    g_asyncLog->append(msg, len);
    string out = msg;
    int pos = out.find("ERROR");
    if (pos >= 0) {
        out = RED + out + NONE;
    }

    // dump the warning now.
    pos = out.find("WARN");
    if (pos >= 0) {
        out = GREEN + out + NONE;
    }

    // dump the special content for write the output window now.
    size_t n = std::fwrite(out.c_str(), 1, out.length(), stdout);
    (void)n;
}

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
//    if(gProxyServer)
//    {
//        gProxyServer->quit();
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
    printf("Before set rlimit CORE dump current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
    rlmt.rlim_cur = 1024*1024*1024;
    rlmt.rlim_max = 1024*1024*1024;
    if(setrlimit(RLIMIT_CORE, &rlmt) == -1)
        return false;

    if(getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
        return false;
    printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
    rlmt.rlim_cur = (rlim_t)655350;
    rlmt.rlim_max  = (rlim_t)655356;
    if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
        return false;

    return true;
}

//void ATEXIT(void)
//{
//    cout<<"++++++++++++++++++cout++++++++++++++++++++"<<endl;
//    cout<<"EXIT"<<endl;
//}

void SetLibraryPathEnv()
{
//    char *old_library_path = getenv("LD_LIBRARY_PATH");
    string path = ".";
    setenv("LD_LIBRARY_PATH", path.c_str(), false);
    path = "/usr/local/lib64";
    setenv("LD_LIBRARY_PATH", path.c_str(), false);
}

int main(int argc, char* argv[])
{
    if(!SetRLIMIT())
        return -1;

    SetLibraryPathEnv();

    muduo::net::EventLoop loop;
//    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/ProxyServer/"))
    {
        boost::filesystem::create_directories("./log/ProxyServer/");
    }
    muduo::AsyncLogging log(::basename("ProxyServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;

    LOG_INFO << "pid = " << getpid();

    uint16_t proxyServerPort = 0;
    uint16_t proxyVip   = 0;
    uint16_t proxyHttpPort = 8999;

    int configId = 0;
    // command line.
    if(argc == 2)
    {
        configId = stoi(argv[1]);
    }else if(argc == 3)
    {
        proxyServerPort = stoi(argv[1]);
        proxyVip   = stoi(argv[2]);
    }

    //=====config=====
    if(!boost::filesystem::exists("./conf/game.conf"))
    {
        LOG_INFO << "./conf/game.conf not exists";
        return 1;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/game.conf", pt);

    // ===== loglevel ======
    int loglevel = pt.get<int>("Global.loglevel",1);
    muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
    LOG_WARN << " >>> LogLevel:" << loglevel;

    std::string netcardName = pt.get<string>("Global.netcardName","eth0");
    LOG_WARN << " >>> network name:" << netcardName;

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
    LOG_INFO<<"ZookeeperIP:"<<strZookeeperServerIP;

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
    LOG_INFO << "RedisClusterIP:" << strRedisServerIP; 


    //=====MongoDB=====
    string strMongoDBUrl = pt.get<string>("MongoDB.Url");
//    string mysql_user = pt.get<string>("mysql.User");
//    string mysql_password = pt.get<string>("mysql.Password");
//    int db_password_len = pt.get<int>("mysql.Len", 0);

    //========RocketMQ========
    string strNameServerIp =  pt.get<string>("RocketMQ.NameServer", "192.168.2.75:9876");

    //=====ProxyServer IP Port=====
    string strProxyServerIP = pt.get<string>("ProxyServer.IP", "127.0.0.1");
    if(!proxyServerPort) proxyServerPort = pt.get<int>("ProxyServer.Port", 8010);
    if(!proxyHttpPort)   proxyHttpPort  = pt.get<int>("ProxyServer.HttpPort", 8999);
    if(!proxyVip)        proxyVip = pt.get<int>("ProxyServer.VIP", 0);
    int16_t threadCount = pt.get<int>("ProxyServer.ThreadCount", 10);
    if (configId)
    {
        string tmpProxyIP;
        uint16_t tmpProxyHttpPort;
        uint16_t tmpProxyPort;//,tmpProxyHttpPort;
        string strKeyIP = "ProxyServer_" + to_string(configId) + ".IP";
        string strKeyPort = "ProxyServer_" + to_string(configId)+ ".Port";
        string strKeyHttpPort = "ProxyServer_" + to_string(configId)+ ".HttpPort";
        string strKeyVIP = "ProxyServer_" + to_string(configId)+ ".VIP";
        tmpProxyIP       = pt.get<string>(strKeyIP,"");
        tmpProxyPort     = pt.get<int>(strKeyPort,0);
        tmpProxyHttpPort = pt.get<int>(strKeyHttpPort, 8999);
        if (tmpProxyIP.length()) strProxyServerIP = tmpProxyIP;
        if (tmpProxyPort)        proxyServerPort = tmpProxyPort;
        if (tmpProxyHttpPort)    proxyHttpPort = tmpProxyHttpPort;
    }

    LOG_INFO << "ProxyServer:" << strProxyServerIP << ":" << proxyServerPort << ", httpPort:" << proxyHttpPort;
    
    //===== test HallIP =====
    int32_t hallIpIsFixed = pt.get<int>("ProxyServer.HallIpIsFixed",0);
    string strHallIP = pt.get<string>("ProxyServer.HallIP", "192.168.2.95");

    muduo::net::InetAddress serverAddr(proxyServerPort);
    Landy::ProxyServer server(&loop, serverAddr, kIdleSeconds, netcardName,hallIpIsFixed,strHallIP);
    server.setThreadNum(LOOP_THREAD_COUNT);

    if ( server.loadKey() &&
//      server.startDB(mysql_ip, mysql_port, mysql_dbname, mysql_user, mysql_password, db_password_len, mysql_max_connection) &&
        server.InitRedisCluster(strRedisServerIP, redisPassword) &&
        server.InitZookeeper(strZookeeperServerIP) &&
        server.startThreadPool(2))//  &&
        // server.InitRocketMQ(strNameServerIp)
    {
//        gProxyServer = &server;
//        registerSignalHandler(SIGTERM, Stop);
//        registerSignalHandler(SIGINT, Stop);
        server.start(threadCount);
        loop.loop();
    }

    return 0;
}
