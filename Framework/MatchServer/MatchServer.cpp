#include "MatchServer.h"

#include <sys/types.h>
#include <sys/time.h>
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
#include <dlfcn.h>

#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include "ConnectionPool.h"
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

#include "RedisClient/RedisClient.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/GlobalFunc.h"
// #include "public/GameGlobalFunc.h"
#include "public/ITableFrameSink.h"
#include "AndroidUserManager.h"
#include "GameServer/ServerUserManager.h"
#include "TableFrame.h"
#include "ThreadLocalSingletonMongoDBClient.h"
#include "ThreadLocalSingletonRedisClient.h"
#include "proto/Game.Common.pb.h"
#include "proto/GameServer.Message.pb.h"
#include "proto/MatchServer.Message.pb.h"

#include "TraceMsg/FormatCmdId.h"

// is debug server now.
extern int g_bisDebug;

namespace Landy
{

    extern int g_EastOfUtc;

    using namespace muduo;
    using namespace muduo::net;

    using bsoncxx::builder::stream::close_array;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_array;
    using bsoncxx::builder::stream::open_document;

    map<int, CMatchServer::AccessCommandFunctor> CMatchServer::m_functor_map;

    //// timer clock internal value.
    //double g_fTickSysClock = 1.0f;
    //timeval g_lastTickTime = { 0 };

    // dump the data content value now.
    void dumpbin(void *datain, int size)
    {
        if (!g_bisDebug)
            return;

        string strValue;
        char buf[32] = {0};
        unsigned char *data = (unsigned char *)datain;
        for (int i = 0; i < size; ++i)
        {
            snprintf(buf, sizeof(buf), "%02X ", data[i]);
            strValue += buf;
        }
        //Cleanup:
        //LOG_DEBUG << ">>> bin:" << strValue;
    }

    CMatchServer::CMatchServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, int idleSeconds, string netcardName) : m_server(loop, listenAddr, "MatchServer-MatchServer"), m_game_logic_thread(new EventLoopThread(bind(&CMatchServer::singleThreadInit, this, placeholders::_1), "GameLogicEventLoopThread")), m_netCardName(netcardName)
    {
        init();
        m_server.setConnectionCallback(bind(&CMatchServer::onConnection, this, placeholders::_1));
        m_server.setMessageCallback(bind(&CMatchServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));

        //    m_timer_thread->startLoop();
    }

    CMatchServer::~CMatchServer()
    {

#if 0
    m_invalidGameServerIps.clear();
#endif

        m_functor_map.clear();
    }

    void CMatchServer::init()
    {
#if 0
    m_invalidGameServerIps.clear();
#endif

        m_functor_map.clear();

        m_myIP = "";
        m_myPort = 0;

        m_bStopServer = false;
        m_bInvaildServer = false;
        m_UserIdProxyConnectionInfoMap.clear();
        m_IPProxyMap.clear();

        TaskService::get_mutable_instance();

        m_bCanJoinMatch = false;
        m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER << 8) | ::Game::Common::KEEP_ALIVE_REQ] = bind(&CMatchServer::cmd_keep_alive_ping, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
        m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER << 8) | ::MatchServer::SUB_C2S_ENTER_MATCH_REQ] = bind(&CMatchServer::cmd_enter_match, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
        m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER << 8) | ::MatchServer::SUB_C2S_LEFT_MATCH_REQ] = bind(&CMatchServer::cmd_left_match, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

        m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER << 8) | ::MatchServer::SUB_C2S_USER_RANK_REQ] = bind(&CMatchServer::cmd_get_match_user_rank, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
        m_functor_map[(::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER << 8) | ::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE] = bind(&CMatchServer::cmd_user_offline, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
        m_functor_map[(::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER << 8) | ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER] = bind(&CMatchServer::cmd_notifyRepairServerResp, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);

        m_functor_map[(::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC << 8)] = bind(&CMatchServer::cmd_game_message, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
    }

    void CMatchServer::quit()
    {
        m_zookeeperclient->closeServer();
        //    m_redisClient->
        //    m_thread_pool.stop();
    }

    void CMatchServer::setThreadNum(int numThreads)
    {
        LOG_INFO << __FUNCTION__ << " --- *** "
                 << "网络IO线程数 = " << numThreads;
        m_server.setThreadNum(numThreads);
    }

    bool CMatchServer::loadRSAPriKey()
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
                //             if(m_pri_key == ORG_PRI_KEY)
                //                 LOG_INFO <<"+++++++++++++++++++++OKOKOK+++++++++++++++++++++";
                //             else
                //                 LOG_INFO<<"+++++++++++++++++++++Error+++++++++++++++++++++";
                in.close();
                return true;
            }
        }

        //LOG_ERROR << "PRI KEY FILE not exists!";
        return false;
    }

    string CMatchServer::decryDBKey(string password, int db_password_real_len)
    {
        //LOG_INFO << "Load DB Key...";

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

    bool CMatchServer::loadKey()
    {
        //LOG_INFO << "Load RSA PRI Key...";
        return loadRSAPriKey();
    }

    bool CMatchServer::InitRedisCluster(string ip, string password)
    {
        //LOG_INFO << "InitRedisCluster m_redisPubSubClient... ";
        m_redisPubSubClient.reset(new RedisClient());
        if (!m_redisPubSubClient->initRedisCluster(ip, password))
        {
            LOG_FATAL << "redisclient connection error";
            return false;
        }
        m_redisPassword = password;

        m_redisPubSubClient->subscribeStopGameServerMessage(bind(&CMatchServer::notifyStopGameServerMessage, this, placeholders::_1));
        m_redisPubSubClient->subscribeRechargeScoreToGameServerMessage(bind(&CMatchServer::notifyRechargeScoreToGameServerMessage, this, placeholders::_1));
        m_redisPubSubClient->subscribeExchangeScoreMessage(bind(&CMatchServer::notifyExchangeScoreToGameServerMessage, this, placeholders::_1));
        m_redisPubSubClient->subsreibeRefreashConfigMessage(bind(&CMatchServer::notifyRefreashConfig, this, placeholders::_1));

        // 更新任务通知
        m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_updateTask, [&](string msg) {
            LOG_ERROR << "-------------updateTask------------" << __FUNCTION__ << " " << msg;
            TaskService::get_mutable_instance().setUpdateTask(true);
            TaskService::get_mutable_instance().loadTaskStatus();
        });

        LOG_ERROR << "-------------初始化------------" << __FUNCTION__;

        m_redisPubSubClient->startSubThread();

        m_redisIPPort = ip;

        return true;
    }

    bool CMatchServer::InitZookeeper(string ip, uint32_t gameId, uint32_t roomId)
    {
        m_roomId = roomId;
        m_gameId = gameId;
        m_zookeeperclient.reset(new ZookeeperClient(ip));
        //    shared_ptr<ZookeeperClient> zookeeperclient(new ZookeeperClient("192.168.100.160:2181"));
        //    m_zookeeperclient = zookeeperclient;
        m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&CMatchServer::ZookeeperConnectedHandler, this));
        if (m_zookeeperclient->connectServer())
        {
            return true;
        }
        else
        {
            LOG_FATAL << "zookeeperclient->connectServer error";
            return false;
        }
    }
    bool CMatchServer::startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize)
    {
        string url = "tcp://"+host+":"+std::to_string(port);
        string dbPassword = /*decryDBKey(password, db_password_len);*/password;
        bool bSucces = ConnectionPool::GetInstance()->initPool(url, name, dbPassword, dbname, maxSize);
        return bSucces;
    }
    bool CMatchServer::InitMongoDB(string url)
    {
        // LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;

        mongocxx::instance instance{}; // This should be done only once.
        m_mongoDBServerAddr = url;
        ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
        return true;
    }

    void CMatchServer::singleThreadInit(EventLoop *ep)
    {
        if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
        {
            LOG_FATAL << "RedisClient Connection ERROR!";
            return;
        }
        REDISCLIENT.hset(REDIS_CUR_STOCKS, to_string(m_GameRoomInfo.roomId), to_string(m_GameRoomInfo.totalStock));
        mongocxx::database db = MONGODBCLIENT["gamemain"];
        MONGODBCLIENT;
    }

    void CMatchServer::threadInit()
    {
        //===============RedisClient=============
        if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
        {
            LOG_FATAL << "RedisClient Connection ERROR!";
            return;
        }
        REDISCLIENT.hset(REDIS_CUR_STOCKS, to_string(m_GameRoomInfo.roomId), to_string(m_GameRoomInfo.totalStock));
        mongocxx::database db = MONGODBCLIENT["gamemain"];
        MONGODBCLIENT;
    }

    void CMatchServer::start(int threadCount)
    {
        LOG_INFO << __FUNCTION__ << " --- *** "
                 << "worker 线程数 = " << 1;
        //    m_thread_pool.setThreadInitCallback(bind(&GameServer::threadInit, this));
        //    m_thread_pool.start(1);
        m_game_logic_thread->startLoop();

        if (m_GameRoomInfo.bEnableAndroid)
        {
            //if(m_GameInfo.gameType == GameType_Confrontation)
            m_game_logic_thread->getLoop()->runAfter(3.0f, bind(&CAndroidUserManager::OnTimerCheckUserIn, &(CAndroidUserManager::get_mutable_instance())));
            shared_ptr<CServerUserItem> pUser = make_shared<CServerUserItem>();
            MatchRoomManager::get_mutable_instance().FindNormalSuitMatch(pUser);
        }

        //    m_timer_thread->getLoop()->runEvery(5.0f, bind(&GameServer::OnRepairGameServerNode, this));
        if (m_GameRoomInfo.updatePlayerNumTimes)
        {
            m_game_logic_thread->getLoop()->runEvery(m_GameRoomInfo.updatePlayerNumTimes,
                                                     bind(&MatchRoomManager::UpdataPlayerCount2Redis, &(MatchRoomManager::get_mutable_instance()), m_szNodeValue));

            m_game_logic_thread->getLoop()->runEvery(m_GameRoomInfo.updatePlayerNumTimes * 2,
                                                     bind(&CMatchServer::updateAgentPlayers, this));
        }
        m_openMatchTimer = m_game_logic_thread->getLoop()->runAfter(1, bind(&CMatchServer::openMatchTimer, this));

        m_server.start();
        m_server.getLoop()->runEvery(5.0f, bind(&CMatchServer::OnRepairGameServerNode, this));
    }

    void CMatchServer::OnRepairGameServerNode()
    {
        if (!m_bStopServer) // && ZNONODE == m_zookeeperclient->existsNode(m_szNodePath))
        {
            if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath) && ZNONODE == m_zookeeperclient->existsNode(m_szInvalidNodePath))
            {
                LOG_DEBUG << m_szNodePath;
                m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
            }
        }
    }

    void CMatchServer::ZookeeperConnectedHandler()
    {
        LOG_INFO << __FUNCTION__;
        if (ZNONODE == m_zookeeperclient->existsNode("/GAME"))
            m_zookeeperclient->createNode("/GAME", "Landy");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/MatchServers"))
            m_zookeeperclient->createNode("/GAME/MatchServers", "MatchServers");

        if (ZNONODE == m_zookeeperclient->existsNode("/GAME/MatchServersInvalid"))
            m_zookeeperclient->createNode("/GAME/MatchServersInvalid", "MatchServersInvalid");

        vector<string> vec;
        boost::algorithm::split(vec, m_server.ipPort(), boost::is_any_of(":"));
        string ip;
        GlobalFunc::getIP(m_netCardName, ip);
        m_myIP = ip;
        m_myPort = stoi(vec[1]);
        m_szNodeValue = to_string(m_roomId) + ":" + ip + ":" + vec[1];
        m_szNodePath = "/GAME/MatchServers/" + m_szNodeValue;
        m_szInvalidNodePath = "/GAME/MatchServersInvalid/" + m_szNodeValue;
        m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);

#if 0
    GetChildrenWatcherHandler getInvalidGameServersChildrenWatcherHandler = std::bind(&GameServer::getInvalidGameServersChildrenWatcherHandler, this,
                                                       placeholders::_1, placeholders::_2,
                                                       placeholders::_3,placeholders::_4,
                                                       placeholders::_5);

    vector<string> invalidGameServers;
    if(ZOK == m_zookeeperclient->getClildren("/GAME/GameServersInvalid", invalidGameServers, getInvalidGameServersChildrenWatcherHandler, this))
    {
        MutexLockGuard lock(m_invalidGameServerIps_mutex);
        m_invalidGameServerIps.assign(invalidGameServers.begin(), invalidGameServers.end());
    }

    // init to dump the game servers.
    for(string ip : invalidGameServers)
    {
        LOG_INFO << " >>> Init GET GameServers :" << ip;
    }
#endif
    }

    bool CMatchServer::LoadGameRoomKindInfo(uint32_t gameId, uint32_t roomId)
    {
        bool bFind = false;
        try
        {
            mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["match_kind"];
            //        auto query_value = document{} << "gameid" << (int32_t)gameId << "roomId" << (int32_t)roomId << finalize;
            auto query_value = document{} << "matchid" << (int32_t)roomId << finalize;
            LOG_DEBUG << "Query:" << bsoncxx::to_json(query_value);
            bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
            if (maybe_result)
            {
                LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(maybe_result->view());

                bsoncxx::document::view view = maybe_result->view();
                m_GameInfo.gameId = view["gameid"].get_int32();
                m_GameInfo.gameName = view["gamename"].get_utf8().value.to_string();
                m_GameInfo.sortId = view["sort"].get_int32();
                m_GameInfo.gameServiceName = view["servicename"].get_utf8().value.to_string();
                m_GameInfo.revenueRatio = view["revenueRatio"].get_int32();
                m_GameInfo.gameType = 2; //view["type"].get_int32();

                m_GameRoomInfo.updatePlayerNumTimes = view["updatePlayerNum"].get_int32();

                m_GameRoomInfo.gameId = gameId;
                m_GameRoomInfo.roomId = roomId;

                m_GameRoomInfo.roomName = view["matchname"].get_utf8().value.to_string();
                m_GameRoomInfo.tableCount = view["tablecount"].get_int32();
                m_GameRoomInfo.floorScore = view["floorscore"].get_int64();
                m_GameRoomInfo.ceilScore = -1;

                m_GameRoomInfo.enterMinScore = 0;
                m_GameRoomInfo.enterMaxScore = 0;

                //m_GameRoomInfo.minPlayerNum = view["gameminplayernum"].get_int32();

                m_GameRoomInfo.maxPlayerNum = view["gamemaxplayernum"].get_int32();
                m_GameRoomInfo.minPlayerNum = m_GameRoomInfo.maxPlayerNum;

                m_GameRoomInfo.broadcastScore = view["broadcastscore"].get_int64();
                m_GameRoomInfo.bEnableAndroid = view["enableandroid"].get_int32();
                m_GameRoomInfo.androidCount = view["androidcount"].get_int32();
                m_GameRoomInfo.androidMaxUserCount = view["androidmaxusercount"].get_int32();

                m_GameRoomInfo.maxJettonScore = view["maxjettonscore"].get_int64();

                m_GameRoomInfo.totalStock = view["totalstock"].get_int64();
                m_GameRoomInfo.totalStockLowerLimit = view["totalstocklowerlimit"].get_int64();
                m_GameRoomInfo.totalStockHighLimit = view["totalstockhighlimit"].get_int64();

                m_GameRoomInfo.systemKillAllRatio = view["systemkillallratio"].get_int32();
                m_GameRoomInfo.systemReduceRatio = view["systemreduceratio"].get_int32();
                m_GameRoomInfo.changeCardRatio = view["changecardratio"].get_int32();

                m_GameRoomInfo.serverStatus = view["status"].get_int32();

                vector<int64_t> jettonsVec;
                vector<int64_t> vec_upgradeGameNum;
                vector<int64_t> vec_upgradeUserNum;
                vector<int64_t> vec_award;
                auto loadData = [&](vector<int64_t> &vec, string key) {
                    bsoncxx::types::b_array array;
                    bsoncxx::document::element array_obj = view[key];
                    if (!array_obj)
                        return;
                    if (array_obj.type() == bsoncxx::type::k_array)
                        array = array_obj.get_array();
                    else
                        array = array_obj["_v"].get_array();
                    for (auto &val : array.value)
                    {
                        vec.push_back(val.get_int64());
                    }
                };

                loadData(vec_upgradeGameNum, "upgradeGameNum");
                loadData(vec_upgradeUserNum, "upgradeUserNum");
                loadData(jettonsVec, "jettons");
                loadData(vec_award, "award");
                m_vecMatchOpenTime.clear();
                loadData(m_vecMatchOpenTime, "openTime");
                m_MatchRoomInfo.bCanJoinMatch = true;
                m_GameRoomInfo.jettons = jettonsVec;
                m_MatchRoomInfo.awards.clear();
                m_MatchRoomInfo.upgradeGameNum.clear();
                m_MatchRoomInfo.upGradeUserNum.clear();

                m_MatchRoomInfo.awards = vec_award;
                m_MatchRoomInfo.upgradeGameNum = vec_upgradeGameNum;
                m_MatchRoomInfo.upGradeUserNum = vec_upgradeUserNum;
                m_MatchRoomInfo.maxPlayerNum = view["matchplayernum"].get_int32();
                m_MatchRoomInfo.roomCount = view["roomCount"].get_int32();
                m_MatchRoomInfo.initCoins = view["initCoins"].get_int32();
                m_MatchRoomInfo.roomAndroidMaxCount = view["androidmaxusercount"].get_int32();
                m_MatchRoomInfo.joinNeedSore = view["joinNeedScore"].get_int32();
                m_MatchRoomInfo.fixedAllocate = view["fixedAllocate"].get_int32();
                m_MatchRoomInfo.fixedPerGamePlayerNum = m_GameRoomInfo.maxPlayerNum;
                m_MatchRoomInfo.matchRoomId = roomId;
                m_MatchRoomInfo.gameId = gameId;
                m_MatchRoomInfo.matchName = m_GameRoomInfo.roomName;
                m_MatchRoomInfo.servicename = m_GameInfo.gameServiceName;
                m_MatchRoomInfo.atLeastNeedScore = view["atLeastNeedCoins"].get_int32();
                m_MatchRoomInfo.leftWaitSeconds = view["leftWaitSeconds"].get_int32();
                m_MatchRoomInfo.androidEnterMinGap = view["androidEnterMinGap"].get_int32();
                m_MatchRoomInfo.androidEnterMaxGap = view["androidEnterMaxGap"].get_int32();

                CAndroidUserManager::get_mutable_instance().SetAndroidUserCount(
                    m_GameRoomInfo.androidCount, m_GameRoomInfo.androidMaxUserCount);
                bFind = true;
            }
        }
        catch (exception &e)
        {
            LOG_ERROR << "MongoDB exception: " << e.what();
        }
        return bFind;
    }

    bool CMatchServer::LoadLibraryEx(PFNCreateTableSink &CreateTableFrameSink)
    {
        //开始加载机器人模块
        char path[1024] = {0};
        if (!getcwd(path, sizeof(path)))
        {
            strcpy(path, ".");
        }
        // build the full android path.
        string strDllPath = path;
        strDllPath.append("/");

        // std::string strDllName(pGameKind->szServerDLL);
        string strDllName = GlobalFunc::clearDllPrefix(m_GameInfo.gameServiceName);
        // try to build the full android dll file name.
        strDllName.insert(0, "./lib");
        strDllName.append(".so");
        strDllName.insert(0, strDllPath);

        LOG_DEBUG << "Load Game Logic so:" << strDllName;
        // try to load the special library from the database.
        void *lib_dl = dlopen(strDllName.c_str(), RTLD_LAZY);
        if (!lib_dl)
        {
            LOG_ERROR << "==dlopen error=====" << dlerror();
            return false;
        }

        // try to get the special function pointer for create the special table frame sink item data now.
        CreateTableFrameSink = (shared_ptr<ITableFrameSink>(*)(void))dlsym(lib_dl, PFNNAME_CREATE_SINK);
        if (!CreateTableFrameSink)
        {
            dlclose(lib_dl);
            LOG_ERROR << "==dlsym function: " << PFNNAME_CREATE_SINK << " error=====" << dlerror();
            return false;
        }

        return true;
    }

    bool CMatchServer::InitServer(uint32_t gameId, uint32_t roomId)
    {
        PFNCreateTableSink pfnCreateSink = nullptr;
        if ((LoadGameRoomKindInfo(gameId, roomId)) &&
            LoadLibraryEx(pfnCreateSink))
        {
            //本次请求开始时间戳(微秒)
            muduo::Timestamp timestart = muduo::Timestamp::now();

            // initialize the special room kind value content data item now.
            CServerUserManager::get_mutable_instance().Init(&m_GameRoomInfo);
            CGameTableManager::get_mutable_instance().InitAllTable(m_GameRoomInfo.tableCount, &m_GameInfo, &m_GameRoomInfo, pfnCreateSink, m_game_logic_thread,m_game_mysql_thread, this);
            MatchRoomManager::get_mutable_instance().InitAllMatchRoom(m_game_logic_thread, &m_MatchRoomInfo, this);

            //LOG_WARN << " >>> InitAllTable called, nTableCount:" << m_GameRoomInfo.tableCount;
            // check if current is enable android.
            if (m_GameRoomInfo.bEnableAndroid)
            {
                CAndroidUserManager::get_mutable_instance().Init(&m_MatchRoomInfo, &m_GameRoomInfo, m_game_logic_thread);
                CAndroidUserManager::get_mutable_instance().LoadAndroidParam();
            }
            else
            {
                LOG_WARN << "LoadLibrary android disabled!, bisEnableAndroid:" << m_GameRoomInfo.bEnableAndroid;
            }

            muduo::Timestamp timenow = muduo::Timestamp::now();
            double timdiff = muduo::timeDifference(timenow, timestart);
            LOG_ERROR << "---InitServer耗时(s)=======>[" << timdiff << "]<=======";

            return true;
        }
        else
        {
            return false;
        }
    }

    void CMatchServer::onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            TcpConnectionWeakPtr weakPtr(conn);
            {
                WRITE_LOCK(m_IPProxyMapMutex);
                m_IPProxyMap[conn->peerAddress().toIpPort()] = weakPtr;
            }
            uint32_t num = m_connection_num.incrementAndGet();
            LOG_INFO << __FUNCTION__ << " --- *** "
                     << "网关服[" << conn->peerAddress().toIpPort() << "] -> 比赛服["
                     << conn->localAddress().toIpPort() << "] "
                     << (conn->connected() ? "UP" : "DOWN") << " " << num;
        }
        else
        {
            {
                WRITE_LOCK(m_IPProxyMapMutex);
                m_IPProxyMap.erase(conn->peerAddress().toIpPort());
            }

            uint32_t num = m_connection_num.decrementAndGet();
            LOG_INFO << __FUNCTION__ << " --- *** "
                     << "网关服[" << conn->peerAddress().toIpPort() << "] -> 比赛服["
                     << conn->localAddress().toIpPort() << "] "
                     << (conn->connected() ? "UP" : "DOWN") << " " << num;
        }
    }

    void CMatchServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                                 muduo::net::Buffer *buf,
                                 muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
        {
            // FIXME: use Buffer::peekInt32()
            const uint16_t len = buf->peekInt16();
            if (unlikely(len > PACKET_SIZE) || (len < sizeof(int16_t)))
            {
                LOG_ERROR << "Invalid length " << len;
                if (conn)
                    conn->shutdown(); // FIXME: disable reading
                break;
            }
            else if (likely(buf->readableBytes() >= len))
            {
                BufferPtr buffer(new muduo::net::Buffer(len));
                buffer->append(buf->peek(), static_cast<size_t>(len));
                buf->retrieve(len);
                TcpConnectionWeakPtr weakPtr(conn);
                //            m_thread_pool.run(bind(&GameServer::processRequest, this, weakPtr, buffer, receiveTime));
                m_game_logic_thread->getLoop()->runInLoop(bind(&CMatchServer::processRequest, this, weakPtr, buffer, receiveTime));
            }
            else
            {
                break;
            }
        }
    }

    void CMatchServer::processRequest(TcpConnectionWeakPtr &weakConn,
                                      BufferPtr &buf,
                                      muduo::Timestamp receiveTime)
    {
        //LOG_INFO <<" " << __FILE__ << " " << __FUNCTION__ ;
        int32_t size = buf->readableBytes();
        if (size < sizeof(internal_prev_header) + sizeof(Header))
        {
            LOG_ERROR << " " << __FILE__ << " " << __FUNCTION__ << " BUFFER IS NULL !!!!!!!";
            return;
        }

        internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

        string session(pre_header->session, sizeof(pre_header->session));
        int64_t userId = pre_header->userId;
        string aesKey(pre_header->aesKey, sizeof(pre_header->aesKey));

        Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
        int headLen = sizeof(internal_prev_header) + sizeof(Header);
        uint16_t crc = GlobalFunc::GetCheckSum((uint8_t *)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);
        if (commandHeader->len == size - sizeof(internal_prev_header) && commandHeader->crc == crc && commandHeader->ver == PROTO_VER && commandHeader->sign == HEADER_SIGN)
        {
            TRACE_COMMANDID(commandHeader->mainId, commandHeader->subId);
            switch (commandHeader->mainId)
            {
            case Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
            {
                switch (commandHeader->encType)
                {
                case PUBENC__PROTOBUF_NONE:
                {
                    int mainId = commandHeader->mainId;
                    int subId = commandHeader->subId;
                    int id = mainId << 8;
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER || commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER || commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER)
                        id = id | subId;

                    if (likely(m_functor_map.count(id)))
                    {
                        AccessCommandFunctor functor = m_functor_map[id];
                        functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                    }
                    else
                    {
                        LOG_WARN << "Not Define Command: mainId=" << mainId << " subId=" << subId;
                    }
                    break;
                }
                case PUBENC__PROTOBUF_RSA: //RSA
                {
                    //        int ret = Landy::Crypto::rsaPrivateDecrypt(m_pri_key, (unsigned char*)buf->peek()+headLen, buf->readableBytes()-headLen, data);
                    //        if(likely(ret > 0 && !data.empty()))
                    //        {
                    //            ::Game::Common::CommonHeader commonHeader;
                    //            if (likely(commonHeader.ParsePartialFromArray(&data[0], header->realsize)))
                    //            {
                    //                if(likely(commonHeader.header().sign() == SIGN))
                    //                {
                    //                    int mainid = commonHeader.header().mainid();
                    //                    int subid = commonHeader.header().subid();

                    //                    LOG_INFO << ">>> processRequest mainid:" << mainid << ", subid:" << subid;

                    //	                int id = (mainid << 8) | subid;
                    //					int msgid_gs  =  (id & 0xff00);
                    //	                if (likely(m_functor_map.count(id)) ||
                    //					    likely(m_functor_map.count(msgid_gs))) // if mainid == MSGGS_MAIN_GAME.
                    //	                {
                    //	                    try
                    //	                    {
                    //                            // filter to function all the game logic message.
                    //                            if (mainid == Game::Common::MSGGS_MAIN_GAME) {
                    //                                id = msgid_gs;
                    //                            }

                    //	                        AccessCommandFunctor functor = m_functor_map[id];
                    //                            functor(weakConn, data, header->realsize, pre_header);
                    //	                    }   catch (std::exception &ex)
                    //	                    {
                    //	                        LOG_ERROR << "exception caught in ThreadPool";
                    //	                        LOG_ERROR << "reason: " << ex.what();
                    //	                    } catch (google::protobuf::FatalException& ex)
                    //	                    {
                    //	                        LOG_ERROR << ex.what();
                    //	                    }
                    //	                    catch (...) {
                    //	                        LOG_ERROR << "exception unknown!";
                    //	                    }
                    //	                }
                    //					else
                    //                    {
                    //                        LOG_WARN <<"Not Define Command: mainId=1 subId="<<subid;
                    //                    }
                    //                }else
                    //                {
                    //                    LOG_WARN <<"SIGN NOT CORRECT:"<<commonHeader.header().sign();
                    //                }
                    //            }else
                    //            {
                    //                LOG_WARN <<"Failed to parse address book.";
                    //            }
                    //        }
                    //        LOG_ERROR <<"++++++++++++++GameServer::processRequest header->enctype & 0x00ff == 1 NOT Implement!++++++++++++++";
                    break;
                }
                case PUBENC__PROTOBUF_AES: //AES
                {
                    //                int len = 0;
                    //                vector<unsigned char> data(commandHeader->len);
                    //                int ret = Landy::Crypto::cryptoppAesDecrypt(aesKey, (unsigned char*)(buf->peek()+sizeof(internal_prev_header)+sizeof(Header)),
                    //                                                    buf->readableBytes()-sizeof(internal_prev_header)-sizeof(Header), &data[sizeof(Header)], len);
                    //                if(ret > 0)
                    //                {
                    //                    int mainId = commandHeader->mainId;
                    //                    int subId = commandHeader->subId;
                    //                    int id = mainId << 8;
                    //                    if(commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER
                    //                            ||commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                    //                        id = id | subId;

                    //                    if(likely(m_functor_map.count(id)))
                    //                    {
                    //                        AccessCommandFunctor functor = m_functor_map[id];
                    //                        functor(weakConn, (uint8_t*)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                    //                    }else
                    //                    {
                    //                        LOG_WARN <<"Not Define Command: mainId="<<mainId <<" subId="<<subId;
                    //                    }
                    //                }
                    break;
                }
                default:
                    break;
                }
            }
            break;
            case Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER:
            {
                switch (commandHeader->encType)
                {
                case PUBENC__PROTOBUF_NONE:
                {
                    int mainId = commandHeader->mainId;
                    int subId = commandHeader->subId;
                    int id = mainId << 8;
                    if (commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER || commandHeader->mainId == Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER)
                        id = id | subId;

                    if (likely(m_functor_map.count(id)))
                    {
                        AccessCommandFunctor functor = m_functor_map[id];
                        functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
                    }
                    else
                    {
                        LOG_WARN << "Not Define Command: mainId=" << mainId << " subId=" << subId;
                    }
                    break;
                }
                case PUBENC__PROTOBUF_RSA: //RSA
                {
                    //        int ret = Landy::Crypto::rsaPrivateDecrypt(m_pri_key, (unsigned char*)buf->peek()+headLen, buf->readableBytes()-headLen, data);
                    //        if(likely(ret > 0 && !data.empty()))
                    //        {
                    //            ::Game::Common::CommonHeader commonHeader;
                    //            if (likely(commonHeader.ParsePartialFromArray(&data[0], header->realsize)))
                    //            {
                    //                if(likely(commonHeader.header().sign() == SIGN))
                    //                {
                    //                    int mainid = commonHeader.header().mainid();
                    //                    int subid = commonHeader.header().subid();

                    //                    LOG_INFO << ">>> processRequest mainid:" << mainid << ", subid:" << subid;

                    //	                int id = (mainid << 8) | subid;
                    //					int msgid_gs  =  (id & 0xff00);
                    //	                if (likely(m_functor_map.count(id)) ||
                    //					    likely(m_functor_map.count(msgid_gs))) // if mainid == MSGGS_MAIN_GAME.
                    //	                {
                    //	                    try
                    //	                    {
                    //                            // filter to function all the game logic message.
                    //                            if (mainid == Game::Common::MSGGS_MAIN_GAME) {
                    //                                id = msgid_gs;
                    //                            }

                    //	                        AccessCommandFunctor functor = m_functor_map[id];
                    //                            functor(weakConn, data, header->realsize, pre_header);
                    //	                    }   catch (std::exception &ex)
                    //	                    {
                    //	                        LOG_ERROR << "exception caught in ThreadPool";
                    //	                        LOG_ERROR << "reason: " << ex.what();
                    //	                    } catch (google::protobuf::FatalException& ex)
                    //	                    {
                    //	                        LOG_ERROR << ex.what();
                    //	                    }
                    //	                    catch (...) {
                    //	                        LOG_ERROR << "exception unknown!";
                    //	                    }
                    //	                }
                    //					else
                    //                    {
                    //                        LOG_WARN <<"Not Define Command: mainId=1 subId="<<subid;
                    //                    }
                    //                }else
                    //                {
                    //                    LOG_WARN <<"SIGN NOT CORRECT:"<<commonHeader.header().sign();
                    //                }
                    //            }else
                    //            {
                    //                LOG_WARN <<"Failed to parse address book.";
                    //            }
                    //        }
                    //        LOG_ERROR <<"++++++++++++++GameServer::processRequest header->enctype & 0x00ff == 1 NOT Implement!++++++++++++++";
                    break;
                }
                case PUBENC__PROTOBUF_AES: //AES
                {
                    //                    if(!aesKey.empty())
                    //                    {
                    //                        int ret = Landy::Crypto::aesDecrypt(aesKey, (unsigned char*)buf->peek()+headLen, buf->readableBytes()-headLen, data);
                    //                        if(ret > 0 && !data.empty())
                    //                        {
                    //                            ::Game::Common::CommonHeader commonHeader;
                    //                            if(commonHeader.ParsePartialFromArray(&data[0], header->realsize))
                    //                            {
                    //                                if(commonHeader.header().sign() == SIGN)
                    //                                {
                    //                                    int mainid = commonHeader.header().mainid();
                    //                                    int subid = commonHeader.header().subid();

                    //                                    int id = (mainid << 8) | subid;
                    //                                    int msgid_gs  =  (id & 0xff00);
                    //                                    if (likely(m_functor_map.count(id)) ||
                    //                                            likely(m_functor_map.count(msgid_gs))) // if mainid == MSGGS_MAIN_GAME.
                    //                                    {
                    //                                        try
                    //                                        {
                    //                                            // filter to function all the game logic message.
                    //                                            if (mainid == Game::Common::MSGGS_MAIN_GAME)
                    //                                            {
                    //                                                id = msgid_gs;
                    //                                            }

                    //                                            long tick = Utility::gettimetick();
                    //                                            // call the special function to get the content.
                    //                                            AccessCommandFunctor functor = m_functor_map[id];
                    //                                            functor(weakConn, data, header->realsize, pre_header);
                    //                                            long sub = Utility::gettimetick() - tick;
                    //                                            if (sub >= TIMEOUT_PERFRAME) {
                    //                                                LOG_ERROR << " >>> processRequest timeout functor, mainid:" << mainid << ", subid:" << subid << ", time used:" << sub;
                    //                                            }
                    //                                        }
                    //                                        catch (std::exception &ex)
                    //                                        {
                    //                                            LOG_ERROR << "exception caught in ThreadPool mainId:"<<mainid<<" subid"<<subid<<" id:"<<id<<" msgid_gs:"<<msgid_gs;
                    //                                            LOG_ERROR << "reason: " << ex.what();
                    //                                        }catch(google::protobuf::FatalException& ex)
                    //                                        {
                    //                                            LOG_ERROR << "exception caught in ThreadPool mainId:"<<mainid<<" subid"<<subid<<" id:"<<id<<" msgid_gs:"<<msgid_gs;
                    //                                            LOG_ERROR << ex.what();
                    //                                        }
                    //                                        catch (...)
                    //                                        {
                    //                                            LOG_ERROR << "exception caught in ThreadPool mainId:"<<mainid<<" subid"<<subid<<" id:"<<id<<" msgid_gs:"<<msgid_gs;
                    //                                            LOG_ERROR << "exception unknown!";
                    //                                        }
                    //                                    }else
                    //                                    {
                    //                                        LOG_WARN <<"Not Define Command: mainId=1 subId="<<subid;
                    //                                    }
                    //                                }
                    //                            }
                    //                        }
                    //                    }
                    break;
                }
                default:
                    break;
                }
            }
            break;
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
            case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
                //LOG_ERROR << " " <<  __FILE__ << " " << __FUNCTION__ << " BUFFER MAINID ERROR commandHeader->mainId = " << commandHeader->mainId;
                break;
            }
        }
        else
        {
            // LOG_ERROR << " " <<  __FILE__ << " " << __FUNCTION__ << " BUFFER LEN ERROR OR CRC ERROR! header->len="<<commandHeader->len
            //           <<" bufferLen="<<size - sizeof(internal_prev_header)<<" header->crc="<<commandHeader->crc<<" buffer crc="<<crc;
        }
    }

    boost::tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>> CMatchServer::GetProxyConnectionWeakPtrFromUserId(int64_t userId)
    {
        string strProxy;
        shared_ptr<UserProxyConnectionInfo> userProxyConnInfo;
        shared_ptr<internal_prev_header> header;
        TcpConnectionWeakPtr weakConn;

        if (userId > 0)
        {
            {
                READ_LOCK(m_UserIdProxyConnectionInfoMapMutex);
                auto it = m_UserIdProxyConnectionInfoMap.find(userId);
                if (it != m_UserIdProxyConnectionInfoMap.end())
                {
                    userProxyConnInfo = it->second;
                    strProxy = userProxyConnInfo->strProxyIP;
                }
            }
            if (!strProxy.empty())
            {
                READ_LOCK(m_IPProxyMapMutex);
                auto it = m_IPProxyMap.find(strProxy);
                if (it != m_IPProxyMap.end())
                {
                    weakConn = it->second;
                }
            }
            if (userProxyConnInfo && userProxyConnInfo->internal_header)
                header = userProxyConnInfo->internal_header;
        }

        return boost::make_tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>>(weakConn, header);
    }

    bool CMatchServer::sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data,
                                internal_prev_header *internal_header, Header *commandHeader)
    {
        // LOG_INFO << __FILE__ << __FUNCTION__;
        bool bSuccess = true;
        TcpConnectionPtr conn(weakConn.lock());
        if (likely(conn))
        {
#if 0
        string aesKey(internal_header->aesKey, sizeof(internal_header->aesKey));
        // check header value now.
        if(internal_header)
        {
            internal_header->ip = cmdid;
        }

        vector<unsigned char> encryptedData;
        int ret = Landy::Crypto::aesEncrypt(aesKey, data, encryptedData, internal_header);
        if(likely(ret > 0 && !encryptedData.empty()))
        {
            conn->send(&encryptedData[0], ret);
            LOG_INFO << __FILE__ << __FUNCTION__ << "conn->send OK";
        }
        else
        {
            LOG_WARN <<"Landy::Crypto::aesEncrypt Error.";
            bSuccess = false;
        }
#else
            int size = data.size();
            internal_prev_header *send_internal_header = (internal_prev_header *)(&data[0]);
            memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
            send_internal_header->len = size;

            Header *commandSendHeader = (Header *)(&data[sizeof(internal_prev_header)]);
            memcpy(commandSendHeader, commandHeader, sizeof(Header));
            commandSendHeader->encType = PUBENC__PROTOBUF_NONE;
            commandSendHeader->len = size - sizeof(internal_prev_header);
            commandSendHeader->realSize = size - sizeof(internal_prev_header) - sizeof(Header);
            commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);
#if 0
        vector<unsigned char> encryptedData;
        int ret = Landy::Crypto::aesEncrypt(aesKey, data, encryptedData, internal_header);
        if(likely(ret > 0 && !encryptedData.empty()))
        {
            conn->send(&encryptedData[0], ret);
        }
        else
        {
            LOG_WARN <<"TcpConnectionPtr has been closed.";
            bSuccess = false;
        }
#else
            TRACE_COMMANDID(commandSendHeader->mainId, commandSendHeader->subId);
            conn->send(&data[0], data.size());
#endif

#endif
        }
        else
        {
            // check userid.
            int64_t userId = 0;
            if (internal_header)
            {
                userId = internal_header->userId;
            }

            //LOG_WARN << "GameServer::sendData conn(weakConn.lock()) failed, userId:" << userId;
        }
        return (bSuccess);
    }

    bool CMatchServer::GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId)
    {
        string value;
        bool ret = REDISCLIENT.get(session, value);
        if (ret)
        {
            Json::Reader reader;
            Json::Value root;
            if (reader.parse(value, root))
            {
                userId = root["userid"].asInt64();
                account = root["account"].asString();
                agentId = root["agentid"].asUInt();
                return true;
            }
        }
        return false;
    }

    //=====================================work thread=================================
    void CMatchServer::cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {
        // LOG_DEBUG << "" << __FILE__ << " "  << __FUNCTION__ ;

        Header *commandHeader = (Header *)msgdata;
        commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES;
        ::Game::Common::KeepAliveMessage msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            string session = msg.session();

            ::Game::Common::KeepAliveMessageResponse response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            // 如果是玩家则处理
            if (!session.empty())
            {
                response.set_retcode(1);
                response.set_errormsg("User LoginInfo TimeOut, Restart Login.");

                int64_t userId = 0;
                string account;
                uint32_t agentId = 0;

                bool ret = GetUserLoginInfoFromRedis(session, userId, account, agentId);
                if (ret)
                {
                    //比赛持续时间比较久过期时长加长
                    if (REDISCLIENT.ResetExpiredUserOnlineInfo(userId, MAX_USER_ONLINE_INFO_IDLE_TIME * 10))
                    {
                        LOG_WARN << "m_redisClient existsUserOnlineInfo  resetExpiredUserOnlineInfo OK. userId=" << userId;
                        response.set_retcode(0);
                        response.set_errormsg("KEEP ALIVE PING OK.");
                    }
                    else
                    {
                        response.set_retcode(2);
                        response.set_errormsg("KEEP ALIVE PING Error UserId Not Find!");
                    }
                }
                else
                {
                    response.set_retcode(1);
                    response.set_errormsg("KEEP ALIVE PING Error Session Not Find!");
                }

                size_t len = response.ByteSizeLong();
                vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
                if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
                {
                    sendData(weakConn, data, internal_header, commandHeader);
                }
            }
        }
    }

    void CMatchServer::cmd_enter_match(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {
        // LOG_DEBUG << __FILE__ << __FUNCTION__;
        //    muduo::RecursiveLockGuard lock(m_RecursiveMutexLockEnter);
        Header *commandHeader = (Header *)msgdata;
        commandHeader->subId = ::MatchServer::SUB_S2C_ENTER_MATCH_RESP;
        ::MatchServer::MSG_C2S_UserEnterMathReq msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            int64_t userId = internal_header->userId;
            uint32_t userIp = internal_header->ip;
            do
            {
                ::Game::Common::Header header = msg.header();

                ::MatchServer::MSG_S2C_UserEnterMatchResp response;
                ::Game::Common::Header *resp_header = response.mutable_header();
                resp_header->CopyFrom(header);

                uint32_t gameId = msg.gameid();
                uint32_t roomId = msg.roomid();
                string password = msg.dynamicpassword();

                //LOG_ERROR << "gameId:"<<gameId<<" roomId:"<<to_string(roomId);
                response.set_retcode(0);
                response.set_errormsg("InitError!");

                string zkPath = "/GAME/UserOrderScore/" + to_string(userId);
                if (ZOK == m_zookeeperclient->existsNode(zkPath))
                {
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_USER_ORDER_SCORE, "Order Score Now!", internal_header, commandHeader);
                    break;
                }
                zkLockGuard zkLock(m_zookeeperclient, zkPath, to_string(userId));
                //online
                if (REDISCLIENT.ExistsUserOnlineInfo(userId))
                {
                    string dynamicPassword;
                    if (REDISCLIENT.GetUserLoginInfo(userId, "dynamicPassword", dynamicPassword))
                    {
                        string strUserPassword = Landy::Crypto::BufferToHexString((unsigned char *)password.c_str(), password.size());
                        if (dynamicPassword == strUserPassword)
                        {
                            // LOG_WARN << " >>> cmd_enter_match dynamicPassword == strUserPassword ok, userid:" << userId;
                            {
                                TcpConnectionPtr conn(weakConn.lock());
                                internal_prev_header *head = new internal_prev_header();
                                memcpy(head, internal_header, sizeof(internal_prev_header));
                                shared_ptr<internal_prev_header> shared_iternal(head);
                                shared_ptr<UserProxyConnectionInfo> userProxyConnInfo(new UserProxyConnectionInfo());
                                // userProxyConnInfo->strProxyIP = conn->peerAddress().toIp();
                                userProxyConnInfo->strProxyIP = conn->peerAddress().toIpPort();
                                userProxyConnInfo->internal_header = shared_iternal;
                                {
                                    WRITE_LOCK(m_UserIdProxyConnectionInfoMapMutex);
                                    m_UserIdProxyConnectionInfoMap[userId] = userProxyConnInfo;
                                }
                            }
                        }
                        else
                        {
                            // LOG_INFO << "user password Error, LOGIN_PASSWORD_ERCOR. uid : " << userId << ", client:" << strUserPassword << ", stored:" << dynamicPassword;
                            SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_PASSWORD_ERROR, "Password Error!", internal_header, commandHeader);
                            break;
                        }
                    }
                    else
                    {
                        LOG_ERROR << "m_redisClient get account Error, LOGIN_ACCOUNTS_NOT_EXIST.";
                        SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_NOSESSION, "User Login Info Not Exists!", internal_header, commandHeader);
                        break;
                    }
                }
                else
                {
                    LOG_ERROR << "m_redisClient existsUserIdInfo Error on login, LOGIN_RESTART. userid:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_GAME_IS_END, "Room Info Not Exists!", internal_header, commandHeader);
                    break;
                }

                // find if user already exist.
                {
                    shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);

                    //MatchId 从0开始
                    if (pIServerUserItem && pIServerUserItem->GetMatchId() >= 0)
                    {
                        //LOG_DEBUG << ">>> cmd_enter_match, find exist old pIServerUserItem, do SwitchUserItem. userId:" << userId
                        //          <<"GetMatchId()" << pIServerUserItem->GetMatchId();
                        //玩家掉线不知道是不是直接退出比赛
                        shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().GetMatch(pIServerUserItem->GetMatchId());
                        if (pMatchRoom)
                        {
                            pMatchRoom->OnUserOnline(pIServerUserItem);
                        }
                        //                    if(pMatchRoom&&pMatchRoom->CheckJionMatch(pIServerUserItem->GetUserBaseInfo()))
                        shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());
                        if (pITableFrame) //&& pITableFrame->GetGameStatus() != GAME_STATUS_END)
                        {

                            if (!SwitchUserItem(pIServerUserItem, weakConn, internal_header, commandHeader))
                            {
                                SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_USERNOTEXIST, "Enter Room Error!", internal_header, commandHeader);
                                return;
                            }
                        }
                        else
                        {
                            if (pITableFrame)
                                LOG_WARN << "pITableFrame->GetGameStatus():" << pITableFrame->GetGameStatus();
                        }
                        // LOG_DEBUG << "after SwitchUserItem(pIServerUserItem, weakConn, internal_header)";
                        break;
                    }
                }

                //卡线检测.
                bool bCanEnter = CheckUserCanEnterRoom(userId, weakConn, internal_header, commandHeader);
                if (!bCanEnter)
                {
                    LOG_WARN << "user can't enter room, because in other room, userId:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_USERINGAME, "User In Other Room!", internal_header, commandHeader);
                    break;
                }

                //不再任何房间或者比赛里面

                GSUserBaseInfo userInfo;
                // get user base info value.
                if (weakConn.lock())
                {
                    bool bExist = GetUserBaseInfo(userId, userInfo);
                    if (!bExist)
                    {
                        REDISCLIENT.DelUserOnlineInfo(userId);
                        LOG_DEBUG << "can't find user base info, userId:" << userId;
                        SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_USERNOTEXIST, "In DB,UserId Not Exists!", internal_header, commandHeader);
                        break;
                    }
                }
                else
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    LOG_DEBUG << "weakConn.lock() == NULL, user id:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_NOSESSION, "Android Cannot Login!", internal_header, commandHeader);
                    break;
                }

                if (!m_bCanJoinMatch)
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_MATCH_WAIT_OPEN, "wait time to open match", internal_header, commandHeader);
                    break;
                }

                //是否满足最小进入条件.
                if (m_MatchRoomInfo.joinNeedSore > 0 && userInfo.userScore < m_MatchRoomInfo.joinNeedSore)
                {
                    REDISCLIENT.DelUserOnlineInfo(userId);
                    LOG_DEBUG << ">>> enter room failed, reason: nUserScore < GetUserEnterMinScore(). userId:" << userId;
                    SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_SCORENOENOUGH, "nUserScore < GetUserEnterMinScore()", internal_header, commandHeader);
                    break;
                }

                // lock the special table item now.
                {
                    shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().GetUserItem(userId);
                    if (!pIServerUserItem)
                    {
                        REDISCLIENT.DelUserOnlineInfo(userId);
                        LOG_ERROR << "insert or create user item failed!, userId:" << userId;
                        break;
                    }

                    userInfo.ip = internal_header->ip;
                    pIServerUserItem->SetUserBaseInfo(userInfo);
                    shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().FindNormalSuitMatch(pIServerUserItem);
                    //=============================================
                    if (!pMatchRoom)
                    {
                        REDISCLIENT.DelUserOnlineInfo(userId);
                        LOG_INFO << " >>> user can't enter room, because room all full, userId:" << userId;
                        SendGameErrorCode(weakConn, Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, ::MatchServer::SUB_S2C_ENTER_MATCH_RESP, ERROR_ENTERROOM_TABLE_FULL, "table all full", internal_header, commandHeader);
                        CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);
                        break;
                    }

                    bool bDistribute = false;
                    if (!pMatchRoom->JoinMatch(pIServerUserItem))
                    {
                        LOG_INFO << " >>> user can't JoinMatch, userId:" << userId;
                        REDISCLIENT.DelUserOnlineInfo(userId);
                        CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);
                    }
                }
            } while (0);
        }
        else
        {
            //LOG_DEBUG << "command: (100, 2) cmd_enter_match parse data failed!";
        }
    }

    void CMatchServer::cmd_left_match(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header)
    {
        // LOG_DEBUG << __FILE__ << __FUNCTION__;

        Header *commandHeader = (Header *)data;
        commandHeader->subId = ::MatchServer::SUB_S2C_LEFT_MATCH_RESP;
        ::MatchServer::MSG_C2S_UserLeftMathReq msg;
        if (likely(msg.ParseFromArray(data + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            int64_t userId = internal_header->userId;

            ::MatchServer::MSG_S2C_UserLeftMatchResp response;
            ::Game::Common::Header *resp_header = response.mutable_header();
            resp_header->CopyFrom(header);

            shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
            if (pIServerUserItem)
            {
                //LOG_WARN << " >>> OnUserLeft userId:" << userId << ", score:" << pIServerUserItem->GetUserScore();
                //shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());
                shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().GetMatch(pIServerUserItem->GetMatchId());
                if (pMatchRoom)
                {
                    //                KickUserIdToProxyOffLine(userId, KICK_GS|KICK_CLOSEONLY);

                    bool bRet = false;
                    //用户比赛进行中(MATCH_GAMING)，不能退赛
                    if (pMatchRoom->CanLeftMatch())
                    {
                        //用户退赛 ///
                        bRet = pMatchRoom->LeftMatch(userId);
                        if (bRet)
                        {
                            response.set_retcode(0);
                            response.set_errormsg("OK");
                        }
                        else
                        {
                            response.set_retcode(1);
                            response.set_errormsg("Error");
                        }
                    }
                    else
                    {
                        response.set_retcode(2);
                        response.set_errormsg("正在游戏中，不能离开");
                    }
                }
                else
                {
                    response.set_retcode(0);
                    response.set_errormsg(" >>> OnUserLeft find pITableFrame failed! <<< ");
                }
            }
            else
            {
                response.set_retcode(0);
                response.set_errormsg(" >>> OnUserLeft find user failed! <<< ");
                LOG_ERROR << " >>> OnUserLeft find user failed!, userid:" << userId;
            }

            size_t len = response.ByteSizeLong();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
            }
        }
    }

    bool CMatchServer::SendGameErrorCode(TcpConnectionWeakPtr &weakConn, uint8_t mainId, uint8_t subId, uint32_t nRetCode, string errMsg,
                                         internal_prev_header *internal_header, Header *commandHeader)
    {
        bool bSuccess = false;
        do
        {
            ::MatchServer::MSG_S2C_UserEnterMatchResp response;
            Game::Common::Header *header = response.mutable_header();
            header->set_sign(HEADER_SIGN);
            response.set_retcode(nRetCode);
            response.set_errormsg(errMsg);

            commandHeader->mainId = mainId;
            commandHeader->subId = subId;
            TRACE_COMMANDID(commandHeader->mainId, commandHeader->subId);
            size_t len = response.ByteSizeLong();
            vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
            if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, data, internal_header, commandHeader);
                bSuccess = false;
            }
            else
            {
                // LOG_ERROR << " >>> SendGameErrorCode serial to array failed.";
            }

        } while (0);
        //Cleanup:
        return (bSuccess);
    }

    bool CMatchServer::SwitchUserItem(shared_ptr<CServerUserItem> &pIServerUserItem,
                                      TcpConnectionWeakPtr &conn,
                                      internal_prev_header *internal_header, Header *commandHeader)
    {
        // LOG_DEBUG << __FILE__ <<__FUNCTION__;
        do
        {
            if (!pIServerUserItem)
            {
                // LOG_DEBUG << __FILE__ << __FUNCTION__ <<"pIServerUserItem == NULL";
                break;
            }

            // LOG_DEBUG << __FILE__ << __FUNCTION__ << "TABLEID="<<pIServerUserItem->GetTableId();

            shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(pIServerUserItem->GetTableId());

            if (!pIServerUserItem || pIServerUserItem->GetUserId() <= 0 || pIServerUserItem->GetChairId() == INVALID_CHAIR)
            {
                //  LOG_DEBUG << __FILE__ << __FUNCTION__ <<"pIServerUserItem == NULL OR pIServerUserItem->GetUserID() <= 0";
                return false;
            }
            pIServerUserItem->SetTrustship(false);
            pIServerUserItem->setClientReady(true);

            pITableFrame->OnUserEnterAction(pIServerUserItem, commandHeader, false);

            int64_t userId = pIServerUserItem->GetUserId();
            uint32_t tableId = pIServerUserItem->GetTableId();
            uint32_t chairId = pIServerUserItem->GetChairId();
            int64_t userScore = pIServerUserItem->GetUserScore();
            // LOG_INFO << ">>> SwitchUserItem userId: " << userId << " at tableId(" << tableId
            //         << "," << chairId << "), score:" << userScore << ", pIServerUserItem:" << pIServerUserItem.get();

        } while (0);
        //Cleanup:
        return true;
    }

    bool CMatchServer::CheckUserCanEnterRoom(int64_t userId, TcpConnectionWeakPtr &weakPtr, internal_prev_header *internal_header, Header *commandHeader)
    {
        uint32_t redisGameId = 0, redisRoomId = 0,redisTableId = 0;
        if (REDISCLIENT.GetUserOnlineInfo(userId, redisGameId, redisRoomId,redisTableId))
        {
            if (m_GameInfo.gameId != redisGameId || m_GameRoomInfo.roomId != redisRoomId)
            {
                ::GameServer::MSG_S2C_PlayInOtherRoom otherroom;
                ::Game::Common::Header *header = otherroom.mutable_header();
                header->set_sign(HEADER_SIGN);

                commandHeader->mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
                commandHeader->subId = ::GameServer::SUB_S2C_PLAY_IN_OTHERROOM;
                otherroom.set_gameid(redisGameId);
                otherroom.set_roomid(redisRoomId);

                size_t bytesize = otherroom.ByteSizeLong();
                vector<unsigned char> data(bytesize + sizeof(internal_prev_header) + sizeof(Header));
                if (otherroom.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], bytesize))
                {
                    sendData(weakPtr, data, internal_header, commandHeader);
                }

                //  LOG_WARN << "user can't enter room, because in other room, userId:" << userId
                //           << ", nGameId:" << redisGameId << ", nRoomId:" << redisRoomId;

                return false;
            }
        }
        return (true);
    }

    void CMatchServer::cmd_user_ready(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {
        //LOG_DEBUG << __FILE__ << __FUNCTION__;
    }

    void CMatchServer::cmd_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {
        //LOG_DEBUG << __FILE__ << __FUNCTION__;

        //    muduo::MutexLockGuard lock(m_lockEnter);

        int64_t userId = internal_header->userId;
        do
        {

            // LOG_DEBUG << __FILE__ << __FUNCTION__ << "USER ID = "<<userId;

            shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
            if (!pIServerUserItem)
            {
                LOG_INFO << "OnUserOffline, user not exist, skipped. userId:" << userId;
                break;
            }

            //            WRITE_LOCK(pIServerUserItem->m_list_mutex);

            // LOG_DEBUG << __FILE__ << __FUNCTION__ << "pIServerUserItem userId="<<pIServerUserItem->GetUserId();
            uint32_t table_id = pIServerUserItem->GetTableId();
            uint32_t match_id = pIServerUserItem->GetMatchId();

            shared_ptr<ITableFrame> pTableFrame = CGameTableManager::get_mutable_instance().GetTable(table_id);
            shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().GetMatch(match_id);

            if (pTableFrame)
            {
                //                muduo::RecursiveLockGuard lock(pTableFrame->m_RecursiveMutextLockEnterLeave);

                //                shared_ptr<IServerUserItem> pItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);

                pTableFrame->OnUserOffline(pIServerUserItem);
            }
            if (pMatchRoom)
            {
                clearUserIdProxyConnectionInfo(userId);
                pMatchRoom->OnUserOffline(pIServerUserItem);
            }
            else
            {
                LOG_INFO << "user not exist, skipped. match_id:" << match_id << " table_id:" << table_id;
                return;
            }
        } while (0);

        //        SavePlayGameTime(userId);
        return;
    }

    void CMatchServer::clearUserIdProxyConnectionInfo(int64_t userId)
    {
        WRITE_LOCK(m_UserIdProxyConnectionInfoMapMutex);
        m_UserIdProxyConnectionInfoMap.erase(userId);
    }

    void CMatchServer::cmd_game_message(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {

        //LOG_ERROR << " ----------- cmd_game_message!";
        Header *commandHeader = (Header *)msgdata;
        uint8_t mainId = commandHeader->mainId;
        uint8_t subId = commandHeader->subId;
        ::GameServer::MSG_CSC_Passageway msg;
        if (likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();

            int64_t userId = internal_header->userId;

            // try to get the pass data value.
            string str = msg.passdata();
            const uint8_t *buf = (const uint8_t *)str.c_str();
            uint32_t byteSize = str.size();

            // try to get the pointer content item value data now try to get the pointer content item value data.
            shared_ptr<CServerUserItem> serverUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
            if (serverUserItem)
            {
                uint32_t tableId = serverUserItem->GetTableId();
                uint32_t chairId = serverUserItem->GetChairId();
                // LOG_INFO << " >>>>>>>> process_game_message chairid:" << chairId;
                shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(tableId);
                if (pITableFrame)
                {
                    //                long tick = Utility::gettimetick();
                    // try to call the special table frame on game event now.
                    pITableFrame->OnGameEvent(chairId, subId, buf, byteSize);
                }
            }
            else
            {
                LOG_ERROR << " >>> process_game_message find user item failed!";
            }
        }
    }

    //获取比赛排行榜
    void CMatchServer::cmd_get_match_user_rank(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header)
    {
        //LOG_DEBUG << __FILE__ << __FUNCTION__;

        Header *commandHeader = (Header *)data;
        commandHeader->subId = ::MatchServer::SUB_S2C_USER_RANK_RESP;
        ::MatchServer::MSG_C2S_UserRankReq msg;
        if (likely(msg.ParseFromArray(data + sizeof(Header), commandHeader->realSize)))
        {
            ::Game::Common::Header header = msg.header();
            int64_t userId = internal_header->userId;

            ::MatchServer::MSG_S2C_UserRankResp rankResp;
            ::Game::Common::Header *resp_header = rankResp.mutable_header();
            resp_header->CopyFrom(header);

            shared_ptr<CServerUserItem> pIServerUserItem = CServerUserManager::get_mutable_instance().FindUserItem(userId);
            if (pIServerUserItem)
            {
                //LOG_WARN << " >>> match_user_rank userId:" << userId << ", MatchId:" << pIServerUserItem->GetMatchId();
                shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().GetMatch(pIServerUserItem->GetMatchId());
                if (pMatchRoom)
                {
                    pMatchRoom->GetRanks(rankResp);
                }
            }

            size_t len = rankResp.ByteSizeLong();
            vector<unsigned char> rankData(len + sizeof(internal_prev_header) + sizeof(Header));
            if (rankResp.SerializeToArray(&rankData[sizeof(internal_prev_header) + sizeof(Header)], len))
            {
                sendData(weakConn, rankData, internal_header, commandHeader);
            }
        }
    }

    void CMatchServer::notifyStopGameServerMessage(string msg)
    {
        //LOG_INFO << __FILE__ << __FUNCTION__ << "MSG="<< msg << " <<<";
    }

    void CMatchServer::notifyRefreashConfig(string msg)
    {
        //LOG_INFO << __FILE__ << __FUNCTION__ << "MSG="<< msg << " <<<";

        Json::Reader reader;
        Json::Value root;
        if (reader.parse(msg, root))
        {
            uint32_t gameId = root["GameId"].asInt();
            uint32_t roomId = root["RoomId"].asInt();

            if ((!gameId && !roomId) || (gameId == m_gameId && roomId == m_roomId))
            {
                LOG_INFO << __FILE__ << __FUNCTION__ << "refreash config" << msg << " <<<";
                LoadGameRoomKindInfo(m_gameId, m_roomId);
                m_game_logic_thread->getLoop()->cancel(m_openMatchTimer);
                m_openMatchTimer = m_game_logic_thread->getLoop()->runAfter(1, bind(&CMatchServer::openMatchTimer, this));
            }
        }
    }

    void CMatchServer::openMatchTimer()
    {
        /*
     *1024 = 10:24
     * 偶数开始时间 奇数结束时间 0开始 1结束 2开始
     */
        if (m_vecMatchOpenTime.size() == 0)
        {
            m_bCanJoinMatch = true;
            m_MatchRoomInfo.bCanJoinMatch = m_bCanJoinMatch;
            return;
        }
        //必须是偶数才能匹配
        assert(m_vecMatchOpenTime.size() % 2 == 0);
        time_t t = time(NULL);
        struct tm *T = localtime(&t);
        int64_t timeFormatNow = T->tm_hour * 3600 + T->tm_min * 60 + T->tm_sec;

        //std::cout<<T->tm_year<<"-"<<T->tm_mon<<"-"<<T->tm_mday<<std::endl;
        //std::cout<<T->tm_hour<<"-"<<T->tm_min<<"-"<<T->tm_sec<<std::endl;

        int64_t timeGap = -1;
        //timeGap在奇数后面是暂停时间，偶数后面是开放时间
        for (int16_t i = 0; i < m_vecMatchOpenTime.size(); i++)
        {
            if (timeFormatNow > (m_vecMatchOpenTime[i] / 100 * 3600 + m_vecMatchOpenTime[i] % 100 * 60))
            {
                timeGap = i;
            }
        }

        if (timeGap == -1 || timeGap % 2 == 1)
            m_bCanJoinMatch = false;
        else
            m_bCanJoinMatch = true;

        m_MatchRoomInfo.bCanJoinMatch = m_bCanJoinMatch;
        int32_t nextSecond = 0;
        //新的轮回 凌晨开始计算
        if (timeGap + 1 == m_vecMatchOpenTime.size())
        {
            //        nextSecond = m_vecMatchOpenTime[0]/100*3600+(m_vecMatchOpenTime[0]%100)*60+(24*3600-timeFormatNow);
            nextSecond = 24 * 3600 - timeFormatNow;
        }
        else
        {
            nextSecond = (m_vecMatchOpenTime[timeGap + 1] / 100) * 3600 + (m_vecMatchOpenTime[timeGap + 1] % 100) * 60 - timeFormatNow;
        }
        //开启打开或关闭比赛定时器
        m_openMatchTimer = m_game_logic_thread->getLoop()->runAfter(nextSecond + 3, bind(&CMatchServer::openMatchTimer, this));
    }

    void CMatchServer::notifyRechargeScoreToGameServerMessage(string msg)
    {
        //LOG_DEBUG << ">>> notifyRechargeScoreToGameServerMessage msg=" << msg << " <<<";
    }

    void CMatchServer::notifyExchangeScoreToGameServerMessage(string msg)
    {
        //LOG_DEBUG << ">>> notifyExchangeScoreToGameServerMessage msg=" << msg << " <<<";
    }

    void CMatchServer::cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header)
    {
    }

    // get the special user base info.
    bool CMatchServer::GetUserBaseInfo(int64_t userId, GSUserBaseInfo &baseInfo)
    {
        //    bsoncxx::document::element elem;
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto query_value = document{} << "userid" << userId << finalize;
        // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
        if (maybe_result)
        {
            // LOG_DEBUG<< "Login User DB Info" << bsoncxx::to_json(*maybe_result);;

            bsoncxx::document::view view = maybe_result->view();
            baseInfo.userId = view["userid"].get_int64();
            baseInfo.account = view["account"].get_utf8().value.to_string();
            baseInfo.agentId = view["agentid"].get_int32();
            baseInfo.lineCode = view["linecode"].get_utf8().value.to_string();
            baseInfo.headId = view["headindex"].get_int32();
            baseInfo.nickName = view["nickname"].get_utf8().value.to_string();
            baseInfo.userScore = view["score"].get_int64();
            baseInfo.status = view["status"].get_int32();
            return true;
        }
        else
        {
            LOG_ERROR << "GameServer:GetUserBaseInfo data error!, userId:" << userId;
        }
        return (false);
    }

    void CMatchServer::DBWriteRecordDailyUserOnline(shared_ptr<RedisClient> &redisClient, int nOSType, int nChannelId, int nPromoterId)
    {
    }

    void CMatchServer::ChangeGameOnLineStatus(shared_ptr<RedisClient> &redisClient, int64_t userId, int configId)
    {

        char sql[128] = {0};
        snprintf(sql, sizeof(sql), "UPDATE db_account.user_extendinfo SET isonline_game=%d WHERE userid=%d", configId, userId);
        redisClient->PushSQL(sql);
        // LOG_WARN << "Change Game Online status="<<configId << " userid:" << userId;
    }

    void CMatchServer::cmd_notifyRepairServerResp(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header)
    {
        //  LOG_DEBUG << ">>> GameServer::cmd_notifyRepairServerResp >>>>>>>>>>>>>";
        Header *common = (Header *)data;
        int32_t repaireStatus = 1;
        ::Game::Common::HttpNotifyRepairServerResp msg;
        if (msg.ParseFromArray(&data[sizeof(Header)], common->realSize))
        {
            int32_t status = msg.status();
            if (status == 0 && m_bInvaildServer && ZOK == m_zookeeperclient->deleteNode(m_szInvalidNodePath))
            {
                m_bInvaildServer = false;
                m_GameRoomInfo.serverStatus = SERVER_RUNNING;
            }
            else if (status == 1 && !m_bInvaildServer && ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, to_string(1), true))
            {
                m_bInvaildServer = true;
            }
            else if (status == 2)
            {
                m_GameRoomInfo.serverStatus = SERVER_REPAIRING;
                if (!m_bInvaildServer && ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, to_string(2), true))
                {
                    m_bInvaildServer = true;
                }
                else if (ZOK == m_zookeeperclient->setNodeValue(m_szInvalidNodePath, to_string(2)))
                {
                }
            }
            else
                repaireStatus = 0;
        }
        msg.set_status(repaireStatus);

        size_t len = msg.ByteSize();
        vector<unsigned char> rdata(len + sizeof(internal_prev_header) + sizeof(Header));
        if (msg.SerializeToArray(&rdata[sizeof(internal_prev_header) + sizeof(Header)], len))
        {
            sendData(weakConn, rdata, internal_header, common);
        }
    }

    void CMatchServer::SavePlayGameTime(int64_t userId)
    {
    }

    void CMatchServer::KickUserIdToProxyOffLine(int64_t userId, int32_t nKickType)
    {
        // LOG_DEBUG << ">>> GameServer::KickUserIdToProxyOffLine, userid:" << userId << ", kickType:" << nKickType;
    }

    void CMatchServer::updateAgentPlayers()
    {
        map<uint32_t, uint32_t> mapAgent_Players;
        CServerUserManager::get_mutable_instance().GetAgentPlayersMap(mapAgent_Players);
        Json::Value value;
        for (auto &info : mapAgent_Players)
        {
            value[to_string(info.first)] = info.second;
        }
        Json::FastWriter write;
        string str = write.write(value);
        REDISCLIENT.set("agentid:" + m_szNodeValue, str, 120);
    }

    void CMatchServer::DistributeTable()
    {
        // LOG_ERROR << "DistributeTable NO IMPL yet!";
    }

    bool CMatchServer::DelGameUserToProxy(int64_t userId, muduo::net::TcpConnectionWeakPtr pproxySock)
    {
        // LOG_ERROR << "DelGameUserToProxy NO IMPL yet!";
        return true;
    }

    bool CMatchServer::DelUserFromServer(shared_ptr<CServerUserItem> &pIServerUserItem)
    {
        if (pIServerUserItem->IsAndroidUser())
        {
            CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pIServerUserItem->GetUserId());
        }
        else
        {
            clearUserIdProxyConnectionInfo(pIServerUserItem->GetUserId());
            REDISCLIENT.DelUserOnlineInfo(pIServerUserItem->GetUserId());
            CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);
        }
    }

} // namespace Landy
