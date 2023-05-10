#ifndef REGTISTESERVER_H
#define REGTISTESERVER_H

#include <map>
#include <list>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include "zookeeperclient/zookeeperclient.h"
#include "RedisClient/RedisClient.h"
#include "RedisClient/redlock.h"

#define _USE_REDLOCK_
#ifdef _USE_REDLOCK_
#include "RedisClient/redlock.h"
#endif

#include "IPFinder.h"
#include "RocketMQ/RocketMQ.h"

#include "muduo/base/Logging.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"

#include <map>
#include <string>

#include "BackCodeSet.h"

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

struct IniConfig
{
	int32_t agentId;
	int32_t isLocal;//0，随机帐号；1，固定帐号
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
	int32_t giveCoin; 			//注册送金
	int32_t maxLoginTypeid; 
	int64_t defaultScore;	//
	int64_t expireDate;	//
	int64_t testScore;	//
	string descode;
	string apiadd;
	string secondTokenHandleAesKey;
	string AesKey;
	string oddtype;
	string url_front;
	int32_t useTestData;
};

struct PlayerCfg
{
	int64_t userid;
	int32_t agentid;
	string account;
};

struct LoginField
{
	int32_t visitmode;	//登录模式(1手机登录，0游客登录，2微信登录)
	int32_t isguest;	//身份模式(0游客身份登录，1正式身份登录)
	int64_t userid;		//玩家ID
	int64_t superior;	//玩家上级代理
	int64_t inviter;	//玩家上级代理(快速登录手动输入)
	int32_t agentid;	//商户ID
	int32_t mbtype;		//visitmode=1手机登录时必填，1登录，2,注册，3忘记密码
	int32_t gender;		//微信性别 1 为男性，2 为女性
	string vcode;		//验证码
	string weixinid;	//微信ID（openid	普通用户的标识，对当前开发者帐号唯一）
	string wxnickName;	//微信昵称
	string headurl;		//微信头像
	string password;	//密码
	string srcpwd;		//原始密码
	string mobile;		//手机号
	string sysver;		//系统版本
	string devinfo;		//设备信息
	string devSN;		//设备唯一序列信息
	string ip;			//本设备IP信息
	string unionid;		//用户统一标识。针对一个微信开放平台帐号下的应用，同一用户的 unionid 是唯一的。
	string country;		//国家，如中国为 CN
	string city;		//城市
	string province;	//省份
	string details;		//细节，包含类似qq,生日，等
	// 兼容原平台字段
	string account;		//帐号
	string linecode;	//linecode
	string nickname;	//nickname
	string lastloginip;	//lastloginip
	int32_t headindex;	//ID
	int32_t isWhite;	//ID
};
 
/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class RegisteServer : public muduo::noncopyable
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
	RegisteServer(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& strIpAddr,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    ~RegisteServer();
	
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
	bool refreshWhiteList();
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
	
    // 代理活动信息
    struct ActiveItem
    {
        int32_t status;			 // 状态 -1 是草稿  0 开启
        int32_t sortid;			 // 排序ID. 值越小越靠上
        int32_t type;			 // 红包类型
        string title;		     // 活动标题名称.
        string activityurl;	     // 不同代理下载活动图片的完整url 192.168.2.214:8066
    };
    std::map<int32_t,std::vector<ActiveItem>> m_ActiveItemMap;

    // 代理活动信息
    struct RegPoorItem
    {
        int64_t rgtReward;			 // 状态 -1 是草稿  0 开启
        int64_t rgtIntegral;		 // 排序ID. 值越小越靠上
        int32_t handselNo;			 // 红包类型
        int64_t reqReward;			 // 红包类型
        int64_t minCoin;			 // 红包类型
    };
    std::map<int32_t,RegPoorItem> m_RegPoorMap;
 
    MutexLock m_t_mutex;

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
private:
	string RegisteToken(std::string const& reqStr);
	string DemoHandle(std::string const& reqStr);
	void noticfyTransferServerRecycleBalance(int64_t uid, string strMd5Account, string token_session);
	bool upsertTxhAccount(int64_t userid,int32_t agentId,string account);
	// 创建用户json
	string GetSessionJson(int64_t userid, int32_t agentId, string &account,string & nickname);
	// 构造返回结果 ///
	string createJsonString(int32_t opType,int32_t errcode,string const& account, int64_t userid,string nickname,string const& dataStr,string msg);
	//
	void createResponse(
		int32_t opType,
		int32_t &errcode,
		int32_t agentId,
		int32_t isWhite,
		int32_t logintype,
		int64_t userid,
		string const &descode,
		string const &account,
		string nickname,
		string const &token_session,
		string &jsonRespStr);

	// 
	//判断是否数字组成的字符串 ///
    inline bool IsDigitStr(std::string const& str) {
        boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
		return boost::regex_match(str, reg);
    }

	inline bool IsDigit(std::string const& str) {
        boost::regex reg("^[0-9]+$");
		return boost::regex_match(str, reg);
    }

	//判断是否字符组成的字符串 ///
    inline bool IsLetter(std::string const& str) {
        boost::regex reg("^[a-zA-Z]+$");
		return boost::regex_match(str, reg);
    }

	inline string getMd5Code(std::string src)
	{
		char md5[32 + 1] = {0};
		MD5Encode32(src.c_str(), src.length(), md5, 1);
		string pwdMd5(md5);
		return pwdMd5;
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
	// 刷新游戏版本
	bool refreshGameVerInfo();
	// 加载临时用户
	void loadTempPlayerInfo();
	// 
	void loadUserWhiteList();
	// 登录处理函数
	std::string LoginProcess(std::string const& reqStr);
	 
	// 记录玩家登录情况
	void recordUserLoginInfo(eRecordType recordType,int64_t userid,string & tokensession,string & loginIp);

	// 更新玩家所在代理限红
	bool updateProxyXianHong(int32_t aid,string & oddtype);
 
	int ApplyLoginExAccount(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField);
	int ApplyLoginExVisitor(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField);
	int ApplyLoginExWechat(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField);
	int ApplyLoginExPhone(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField);
	bool createNewUser(int64_t & userid ,LoginField &loginField,int32_t &errcode ,string &errmsg);
	void createEmailDefault(int32_t agentid,int64_t userid);
	void incTeamCount(int64_t superior_uid,bool isUp = false);

	// 注册送金信息
	bool getRegRewardCoin(int32_t agentid,int64_t & regReWard,int64_t & reg_Integral);
	bool cacheActiveItem(int32_t agentid);
	bool cacheRegPoorItem(int32_t agentid);
	bool get_platform_proxy_config(int32_t agent_id,int32_t level,int32_t & brokerage);
	bool get_platform_global_config(int32_t agent_id,int32_t type_id,string & details);
	bool get_level_membership_type(int32_t agent_id, int32_t & level);
	inline string connectString(string src ,int64_t userid,string mobile){
		return (src + "_" + to_string(userid) + "_" + mobile);
	}
	/******************************
	 * APP登录接口
	******************************/
	bool accountLoginCheck(LoginField &loginfield, int32_t &reqcode, string &errmsg);
	bool phoneLoginCheck(LoginField & loginfield,int32_t & reqcode,string & errmsg);
	bool weChatLoginCheck(string wxid,string headurl,int32_t & reqcode,string & errmsg);
	bool LoginLogic(HttpParams params,int32_t agentId,int64_t & uid,int32_t & errcode,string & account,string & nickname,string & token_session,string & errmsg);
	bool getsubagents(vector<int64_t> & subagents,int64_t superior);

	bool AddScoreChangeRecordToDB(int64_t userId,string &account,int64_t addScore,int32_t agentId,string linecode,int64_t beforeScore,int32_t roomId);
	bool JiFenChangeRecordToDB(int64_t userId,string account,int32_t agentId,int64_t incrintegral,int32_t before_integral,eJiFenChangeType changetype,string reason);
	string getDBaccount(int32_t agentid,string account);
	// 
	struct game_domain_group_list_t {
		string groupid;            //groupid 
		string groupname;          //groupname 
		string domaintype;         //domaintype 
		vector<string> domainname; //domainname 
	};
	map<string, game_domain_group_list_t> game_domain_group_list_;
	mutable boost::shared_mutex domain_list_mutex_;

	/*
	proxyinfo.domainid = game_domain.ObjectId
	*/
	struct game_domain_t {
		string ObjectId;           //ObjectId 
		string domain_game;        //domain_game 游戏地址（对应原proxyinfo 中 backurl）
		string domainname;         //domainname   主域名名称（组标识，不是用于访问的）
		string domain_api;         //domain_api   api地址，对外对接时，此地址发送给对接对象，构建原始api请求链接
		string domain_front;       //domain_front   前端网关地址（替换现有api中domain参数值）
		string domain_front_port;  //domain_front_port   前端网关地址端口号（对应现有api中 port参数值
	};
	map<string, game_domain_t> game_domain_;
	mutable boost::shared_mutex game_domain_mutex_;
 
	// 代理信息
	struct agent_info_t {
		int64_t		score;              //代理分数 
		int32_t		status;             //是否被禁用 0正常 1停用
		int32_t		agentId;            //agentId
		int32_t		cooperationtype;    //合作模式  1 买分 2 信用
		std::string domainid;            //domainid 
		std::string descode;            //descode 
		std::string md5code;            //MD5  
		std::string domaintype;         //domaintype 
		std::string prefix;         //prefix 
	};
	//代理信息map[agentId]=agent_info_
	std::map<int32_t, agent_info_t> agent_info_;
	mutable boost::shared_mutex agent_info_mutex_;
	// 存储用户是否白名单
	vector<int64_t> vec_user_white_list_; // 
	mutable boost::shared_mutex user_white_list_mutex_;
	// 存储500个玩家id信息
	vector<int64_t> vec_userid_; // 
	std::map<int64_t, PlayerCfg> player_info_;
	mutable boost::shared_mutex player_info_mutex_;

	// 临时存在视讯代理盘口值
	map<int32_t, std::string> proxy_txh_oddtype_;
	mutable boost::shared_mutex proxy_txh_mutex_;

	bool isStartServer;   
	bool isdecrypt_;   
	// 版本信息 
	string version_info_; 
	IniConfig inicfg_;
 
    MutexLock m_mutex_agents;
  
    MutexLock m_mutex_proxy;
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
