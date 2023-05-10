#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : noncopyable
{
 public:
  typedef function<void(uint8_t*, int, internal_prev_header*)> AccessCommandFunctor;

  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  EventLoop* getLoop() const { return server_.getLoop(); }


  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();
public:
  bool StartEventThread();
  bool InitZookeeper(string ip);
  bool InitRedisCluster(string ip, string password);
  bool startThreadPool(int threadCount);
  void threadInit();

  void ZookeeperConnectedHandler();
  void startAllHallServerConnection();
  void startHallServerConnection(string ip);
  void ProcessHallIPServer(vector<string> &servers);
  //void RefreshHallServers();
  void GetHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                         const string &path, void *context);

  void GetInvaildHallServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                         const string &path, void *context);

  void startAllMatchServerConnection();
  void startMatchServerConnection(string ip);
  void ProcessMatchIPServer(vector<string> &gameServers);
  //void RefreshMatchServers();

  void startAllGameServerConnection();
  void startGameServerConnection(string ip);
  void ProcessGameIPServer(vector<string>& gameServers);
  //void RefreshGameServers();

  //普通游戏服务
  void GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                         const string &path, void *context);
  void GetInvaildGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                   const string &path, void *context);
  //比赛服务
  void GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                         const string &path, void *context);
  void GetInvaildMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                   const string &path, void *context);

  void onHallServerConnection(const TcpConnectionPtr& conn);
  void onHallServerMessage(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp receiveTime);

  void onGameServerConnection(const TcpConnectionPtr& conn);
  void onGameServerMessage(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp receiveTime);

  void onMatchServerConnection(const TcpConnectionPtr& conn);
  void onMatchServerMessage(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp receiveTime);

private:
  void MessageHandler(const HttpRequest& req,HttpResponse& resp);

  void allGameServerInfo(string &str);
  bool repairGameServer(const string& str);
  void allGamePlayerInfo(string &str);

  void allMatchServerInfo(string &str);
  bool repairMatchServer(const string& str);
  void allMatchPlayerInfo(string &str);


  void allHallServerInfo(string &str);
  bool repairHallServer(const string& str);
  bool agentGamePlayers(string &str);
private:
     typedef shared_ptr<Buffer> BufferPtr;
private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);

  void onRequest(const TcpConnectionWeakPtr& weakConn, const HttpRequest& req);
  bool waitForResqBuffer(const TcpConnectionPtr& conn,BufferPtr& buf,string& session);

public:
  void parseQuery(const string &str,map<string,string>& mapQuery);
  bool parseQuery(std::string const& queryStr, map<string,string>& params, std::string& errmsg);
  
private:
  TcpServer server_;
private:
  map<int, AccessCommandFunctor>  m_functor_map;
  shared_ptr<ZookeeperClient> m_zookeeperclient;
  string m_szNodePath, m_szNodeValue;
  string m_netCardName;

  //shared_ptr<RedisClient>  m_redisPubSubClient;
  string m_redisIPPort;
  string m_redisPassword;
  //==========hall server list==========
  list<string>  m_hallServerIps;
  MutexLock m_hallServerIps_mutex;
  //==========game server list==========
  list<string>  m_gameServerIps;
  MutexLock m_gameServerIps_mutex;
  //vector<string> m_gameServers;
  //==========match server list==========
  list<string>  m_matchServerIps;
  MutexLock m_matchServerIps_mutex;
  //vector<string> m_matchServers;

  //==========game invaild server list==========
  list<string>  m_invaildGameServerIps;
  MutexLock m_invaildGameServerIps_mutex;

  MutexLock m_room_servers_mutex;
  map<uint32_t,vector<string>> m_room_servers;

  /*
   *    想来想去还是把比赛跟普通房间分开放
   *    后台获取比赛数据可以直接分开（不知道啥需求全程靠猜，先这样搞）
   *    比赛服跟游戏服的连接还是放在一起
  */
  list<string>  m_invaildMatchServerIps;
  MutexLock m_invaildMatchServerIps_mutex;

  MutexLock m_match_servers_mutex;
  map<uint32_t,vector<string>> m_match_servers;
  //vector<string> matchServers;


  //==========hall ip to TcpClient========
  map<string, std::shared_ptr<TcpClient> > m_hallIPServerMap;
  list<string>  m_invaildHallServerIps;
  MutexLock m_hallIPServerMapMutex;

  //==========game ip to TcpClient========
  map<string, std::shared_ptr<TcpClient> > m_gameIPServerMap;
  MutexLock m_gameIPServerMapMutex;

  const static size_t kHeaderLen = sizeof(int16_t);

  shared_ptr<EventLoopThread> m_hall_loop_thread;
  shared_ptr<EventLoopThread> m_game_loop_thread;

  ThreadPool                  m_thread_pool;
private:
  //map<uint32_t,map<uint32_t,uint32_t>> m_agentGamePlayersMap;
};


#endif  // MUDUO_NET_HTTP_HTTPSERVER_H
