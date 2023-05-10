#ifndef LOGINSERVER_H
#define LOGINSERVER_H

#include <map>
#include <string>
#include <list>
#include <deque>
#include <vector>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <iostream>
#include <math.h>
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
#include <cpr/cpr.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/TcpClient.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/TimerId.h" 
#include "muduo/base/AsyncLogging.h"
#include "muduo/base/TimeZone.h" 
#include "muduo/base/Mutex.h"
#include "muduo/base/ThreadLocal.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/Logging.h"

#include "zookeeperclient/zookeeperclient.h"
#include "RedisClient/RedisClient.h"
#include "RedisClient/redlock.h"

#include "IPFinder.h"
#include "ErrorCodeSet.h"

using namespace muduo;

#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const boost::shared_ptr<T> &x)
{
    return boost::hash_value(x.get());
}
} // namespace boost
#endif

struct PlayerCfg
{
	int64_t userid;
	int32_t agentid;
	string account;
};
 
/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class TransferServer : public muduo::noncopyable
{
public:
	typedef std::map<std::string, muduo::net::TcpConnectionWeakPtr> WeakConnMap;
	typedef std::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
	typedef std::map<std::string, TcpClientPtr> TcpClientMap;

	typedef shared_ptr<muduo::net::Buffer> BufferPtr;

	typedef std::map<std::string, std::string> HttpParams;

    typedef function<void(
		muduo::net::TcpConnectionWeakPtr&,
		uint8_t*, int, internal_prev_header*)> AccessCommandFunctor;

	typedef std::function<void(
		const muduo::net::HttpRequest&,
		muduo::net::HttpResponse&)> HttpCallback;


	muduo::net::EventLoop* getLoop() const { return httpServer_.getLoop(); }

public:
	TransferServer(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& strIpAddr,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    ~TransferServer();
	
	/// Not thread safe, callback be registered before calling start().
	void setHttpCallback(const HttpCallback& cb)
	{
		httpCallback_ = cb;
	}
  
    void quit();
public:
	//MongoDB/redis与线程池关联 ///
	void threadInit();
	bool InitZookeeper(std::string ipaddr);
	bool InitRedisCluster(std::string ipaddr, std::string password);
	bool InitMongoDB(std::string url);
	// 加载配置
	bool LoadIniConfig();
	//zookeeper
public:
	//zookeeper连接成功回调 ///
	void ZookeeperConnectedHandler();

	//自注册服务节点到zookeeper ///
	void OnRepairServerNode();

	//HttpServer通知停止上下分操作处理 ///
	void repairLoginServer(string msg); 

	//ProxyServer节点变更处理回调 ///
	void GetProxyServersChildrenWatcherHandler(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	
    //登录服[C]端 -> 网关服[S]端 ///
	void startAllProxyServerConnection();

	//登录服[C]端 -> 网关服[S]端 ///
	void startProxyServerConnection(std::string ipaddr);

	//登录服[C]端 -> 网关服[S]端 ///
	void processProxyIPServer(std::vector<std::string>& newProxyServers); 

private:
	std::list<std::string> proxyServerIps_;
	muduo::MutexLock mutexProxyServerIps_;
	mutable boost::shared_mutex white_list_mutex_;
	//IP访问白名单信息 ///
	struct whiteIp_info_t {
		// int64_t		score;              //代理分数 
		int32_t		status;             //是否被禁用 0正常 1停用
		bool		isapiuse;            //agentId
	}; 
	std::map<in_addr_t, whiteIp_info_t> white_list_;
	
	//tcpclient 
public:
	//启动TCP业务线程 ///
	//启动TCP监听 ///
	void start(int numThreads, int workerNumThreads);
	
	//连接事件来自 网关服(ProxyServer) ///
	//Connected/closed事件 ///
    //登录服[C]端 -> 网关服[S]端 ///
	void onConnection(const muduo::net::TcpConnectionPtr& conn);

	//接收TCP网络消息回调 ///
	//处理来自 网关服(ProxyServer) 消息 ///
    //网关服[S]端 -> 登录服[C]端 ///
	void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	//处理来自 网关服(ProxyServer) 消息 ///
	void processRequest(muduo::net::TcpConnectionWeakPtr& conn, BufferPtr& buf, muduo::Timestamp receiveTime);

private:
	//weakConns_[ip:port:port:pid]= conn ///
	WeakConnMap weakConns_;
	muduo::MutexLock mutexWeakConns_;

	//clientMap_[ip:port:port:pid]= tcpclient ///
	TcpClientMap clientMap_;
	muduo::MutexLock mutexClientMap_;
	//处理TcpClient网络IO(connector/recv/send) ///
	// std::shared_ptr<Landy::MyEventLoopThreadPool> clientThreadPool_;
	//httpserver
	AtomicInt32    m_reciv_num;
public:
	//启动HTTP业务线程 ///
	//启动HTTP监听 ///
	void startHttpServer(int numThreads, int workerNumThreads);

	//Connected/closed事件 ///
	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);

	//接收HTTP网络消息回调 ///
	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	//异步回调 ///
	void asyncHttpHandler(const muduo::net::TcpConnectionWeakPtr& weakConn, const muduo::net::HttpRequest& req,uint32_t _ipNetEndian);

	//处理HTTP回调 ///
	void processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp);

	//请求字符串 ///
	static std::string getRequestStr(muduo::net::HttpRequest const& req);

	//设置任务消息队列大小 ///
	inline void setMaxQueueSize(int maxSize) { threadPool_.setMaxQueueSize(maxSize); }

	//解析请求 ///
	//strQuery string req.query()
	static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg);

private:
	struct IniConfig
	{
		int32_t agentId;
		int32_t isLocal; //0，随机帐号；1，固定帐号
		int32_t isLog;
		int32_t isdecrypt;
		int32_t isDemo;
		int32_t isWhiteIp;
		int32_t integralvalue;
		int32_t changeRatevalue;
		int32_t session_timeout;
		int32_t ip_timeout;
		int32_t paramCount;
		int32_t maxConnCount;
		int32_t maxConnFreqCount;
		int32_t maxGameid;
		int32_t maxLoginTypeid;
		int64_t defaultScore; //
		int64_t expireDate;	  //
		string descode;
		string apiadd;
		string secondTokenHandleAesKey;
		string AesKey;
		string des_key;
		string md5_key;
		string reqUrl;
		string botUrl;
		string botToken;
		int64_t botChairId;
		string enterGameHandle;
		string resAddUrl;
		string AgentName;
		string Password;
		string MethodName;
		string MoneyType;
		int32_t Timeout;
		int64_t interval;
		int32_t IsMaster;
		int32_t RecycleCycle;
		int32_t Actype;
		int32_t ReadConfCycle;
		int32_t IsPrintConfLog;
		int32_t AutoRecycle;
		int32_t IsSendTgMsg;
		int32_t NotOpen;
		string DemoPlayUrl;
		string oddtype;
		int32_t TestLock;
		vector<int64_t> WhiteUID;
	};

	//指定网卡ipaddr
	std::string strIpAddr_;
	//监听HTTP请求
	muduo::net::TcpServer httpServer_;
	//业务线程池，内部自实现任务消息队列
	muduo::ThreadPool threadPool_;
	//请求处理回调，但非线程安全的
	HttpCallback httpCallback_;

	//统计TCP/HTTP连接数量，"Connection:keep-alive"
	muduo::AtomicInt64 numConnections_;
	muduo::AtomicInt64 numReqCount_;
	muduo::AtomicInt64 numReqOKCount_;
	muduo::AtomicInt64 numReqErrCount_;

	int64_t nConnections_ = 0;
	int64_t nReqCount_ = 0;
	int64_t nReqOKCount_ = 0;
	int64_t nReqErrCount_ = 0;

	int64_t nLastTickTime =0;
	int64_t nLastnReqCount =0;
	int32_t nLogLevelCopy =0;
	// 
	int64_t nSecondTick_ = 0;


private:
	// 获取代理限红
	bool getProxyXianHong(int32_t aid,string & oddtype);
	// 锁定用户 
	bool lockUserScore(int64_t uid,int32_t opType, int32_t & req_code,string & errmsg);
	// 解锁用户 
	bool unlockUserScore(int64_t uid,int32_t opType,int32_t & req_code,string & errmsg);
	// 检测并创建游戏帐号
 	bool getCreateAccountParams(string & lgname,int32_t actype, int32_t aid,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 查询订单状态
	bool orderInquiryParams(string & billNo,int32_t actype,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 获取参数
	string balanceInquiryParams(string & lgname,bool bOnlineCheck = false);
	// 查询余额
	bool getBalance(string & lgname, int64_t & balance,int32_t & req_code,string & errmsg,cpr::Response & resp,bool & status_online);
	// 游戏转账（上下分）参数
	string createTransferParams(string lgname, string billNo, string type,string creditStr);
	// 游戏转账（上下分）
	bool transferBalance(int64_t &newScore,string & lgname,string billNo, string type,int64_t userid,int64_t credit,int64_t & _newbalance,int32_t & req_code,string & errmsg,string & da_account,cpr::Response & resp);
	// 进入游戏
	bool enterGameParams(string & lgname,string mh5,string oddtype,string gametype,string & req_forwardUrl,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 统一请求参数
	cpr::Response getUrlRequest(string & parameContent,string & reqHandle);
	// 检查请求参数
	int32_t checkReqParams(string & reqStr,int64_t & uid,int32_t & actype,string & lgtype,string & oddtype,string & backurl, string & errmsg,ReqParamCfg &reqParam);
	int32_t checkReqParamsEx(string & reqStr,int64_t & uid,int32_t & aid, string & errmsg);
	// 解析返回结果
	bool getJsonPtree( string & jsonStr,int32_t & code,string & msg,boost::property_tree::ptree & pt);
	// 获取用户信息
	bool getUserNameById(int64_t uid, int32_t & aid, string &lgname,int64_t & score,string & da_account, int32_t & req_code,string & errmsg);
	// 创建订单
	void createOrder(string & billNo,string & lgname,string type,int64_t credit,int64_t userid,string da_account,int64_t oldmoney = 0,int64_t newmoney = 0); 
	// 更新订单状态
	void updateOrder(int64_t &newScore,int64_t userid,string type,int64_t credit,string & billNo,int32_t stauts,int32_t status_code,double elapsedtime,int64_t oldmoney = 0,int64_t newmoney = 0); 
	// 设置锁(使用条件:允许锁的偶尔失效)
	bool setNxDelay(int64_t lckuid);
	// 检查连接有效性
	bool checkUrlValid( string reqHandle,int32_t & req_code,string & errmsg);
	// 创建跳转成功记录
	void updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t gameid,int32_t gametype,int32_t online = 1);
	// 获取维护状态
	bool getMaintenanceStatus(int32_t & req_code,string & errmsg);
	// 查询余额状态,type 0 统一回收;1 指定帐号回收
	void recycleBalance(int type,string account);
	int32_t recycleBalanceEx(int64_t & recycleScore ,int64_t userid, int32_t &reqCode, string &errmsg);

	int32_t getUserScore(int64_t &score, int64_t uid);

	// 是否允许访问
	bool isPermitTransfer(int32_t _agentId);
	// 加载Json文件
	bool loadJson();
	// 签名算法
	cpr::Parameters createSignatureParam(string parameContent);
	// 构造返回结果 ///
	string createJsonString(int32_t opType,int32_t errcode,string const& dataStr);
	string createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr);
	string createJsonRsp(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,int64_t newScore);


	bool cacheThirdPartGame();
	int32_t checkGameIDValid(ReqParamCfg &reqParam,string & errmsg);

	// 
	//判断是否数字组成的字符串 ///
    inline bool IsDigitStr(std::string const& str) {
        boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
		return boost::regex_match(str, reg);
    }

	inline void TirmString(std::string &str)
	{
		boost::replace_all<std::string>(str, "\\", "");
		str = boost::regex_replace(str, boost::regex("\n|\r"), "");
	} 
	inline string getTgMsg(int64_t uid,string lgname,string billNo,int32_t code,string msg,double elapsedtimd,string isDemo){
		stringstream ss;
		ss << isDemo
		<< "\nreqCode:" << code
		<< "\nmsg:"<< msg
		<< "\nuid:" << uid
		<< "\nname:" << lgname
		<< "\nelapsed:" << elapsedtimd
		<< "\nbillNo:" << billNo
		<< "\ndate:" << Timestamp::now().toFormattedString(false);
		return ss.str();
	}

	inline void setFailedResponse(muduo::net::HttpResponse& rsp,
	muduo::net::HttpResponse::HttpStatusCode code = muduo::net::HttpResponse::k200Ok,
	std::string const& msg = "") {
		rsp.setStatusCode(code);
		rsp.setStatusMessage("OK");
		rsp.addHeader("Server", "DTQP");
		rsp.setContentType("text/plain;charset=utf-8");
		rsp.setBody(msg);
	}
   
public:
	//刷新所有agent_info信息 ///
	//1.web后台更新代理通知刷新
	//2.游戏启动刷新一次
	bool refreshAgentInfo();     
	// 
	struct playing_list_t {
		int32_t status_online;      //stauts 
		int32_t gametype;        	//gametype 
		int32_t stauts;        		//stauts 
		int64_t userid;            	//userid 
		int64_t score;            	//score 
		int64_t recycleScore;       //recycleScore 
		int64_t lasttime;           //timestamp 
		string account;          	//account 
		string txhaccount;          //txhaccount 
	};
	vector<playing_list_t> playing_list_;
	mutable boost::shared_mutex playing_list_mutex_;

	//禁止访问的白名单
 	vector<int32_t> agent_ids;      
	MutexLock agent_ids_mutex_;
    MutexLock m_thirdpart_game_Lock;
 

	//代理信息map[agentId]=agent_info_
	third_game_cfg agent_info_;
	// std::map<int32_t, third_game_cfg> agent_info_;
	mutable boost::shared_mutex agent_info_mutex_;
	// 存储用户是否白名单
	vector<int64_t> vec_user_white_list_; // 
	mutable boost::shared_mutex user_white_list_mutex_;
	// 存储500个玩家id信息
	vector<int64_t> vec_userid_; // 
	std::map<int64_t, PlayerCfg> player_info_;
	mutable boost::shared_mutex player_info_mutex_;

	bool isStartServer;   
	bool isdecrypt_;   
	// 版本信息 
	string version_info_; 
	IniConfig initcfg_;

	std::map<int32_t, string> errcode_info_;

	struct GameCfg
	{
		int32_t gameid;
		string name; //
		string code;
	};
	std::map<string, GameCfg> game_map_info_;

    // <游戏类型,游戏信息 > //游戏类型,"101":视讯游戏;"102":彩票;"103":电子;"104":体育 
    std::map<int32_t,GameItemInfo> m_GameIdListMap;//
 
private:
    const static size_t kHeaderLen = sizeof(int16_t);

private:
    std::shared_ptr<ZookeeperClient> m_zookeeperclient;
    std::string m_szNodePath, m_szNodeValue, m_szInvalidNodePath, m_szUserOrderScorePath;

    std::shared_ptr<RedisClient> m_redisPubSubClient;
    std::string m_redisIPPort;
    std::string m_redisPassword;

    std::string m_mongoDBServerAddr;

    CIpFinder m_pIpFinder;
	// 
	boost::regex m_pattern;

private:
    static std::map<int, AccessCommandFunctor> m_functor_map;
    bool m_bInvaildServer;
private:

    std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
};

#endif // LOGINSERVER_H
