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

#include "proto/HallServer.Message.pb.h"

#include "cpr/cpr.h" //<cpr/cpr.h>
#include "cpr/curlholder.h"//<cpr/curlholder.h>

#include "IPFinder.h"
#include "ErrorCodeSet.h"

using namespace muduo;
using namespace muduo::net;

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
	int32_t maxLoginTypeid; 
	int64_t defaultScore;	//
	int64_t expireDate;	//
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
	int32_t isOnService;
	string DemoPlayUrl;
	string oddtype;
	int32_t TestLock;
	vector<int64_t> WhiteUID;
};

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
class PayServer : public muduo::noncopyable
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
	PayServer(EventLoop* loop,const InetAddress& listenAddr,const InetAddress& listenAddrHttp,string const& strIpAddr,string netcardName,
		TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    ~PayServer();
	
	/// Not thread safe, callback be registered before calling start().
	void setHttpCallback(const HttpCallback& cb)
	{
		httpCallback_ = cb;
	}
  
    void quit();
    void init();
public:
	//MongoDB/redis与线程池关联 ///
	void threadInit();
	bool InitZookeeper(std::string ipaddr);
	bool InitRedisCluster(std::string ipaddr, std::string password);
	bool InitMongoDB(std::string url);
	// 加载配置
	bool LoadIniConfig();

	void threadInitTcp();
	bool startThreadPool(int threadCount);
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
    string m_netCardName;

		 /*
    取数据的表
    onlinepaydeploy   在线支付（尊享、VIP也在此取）
    membertype_channel  在线支付（第三方在线支付）
    platform_bank  银行卡转帐
    */
    struct PaymentItem
    {
        string  paymentchannel ;//"wy" 网银 "kf"
        string  channel;        //支付代码编号
        string  channelname;    //"网银支付"
        string  paymentcode;    //支付通道代码编号
        string  bankjson;       //银行信息JSON  在线支付、银行卡转帐、第三方在线支付
        string  bankUrl;        //支付渠道的银行卡或充值跳转连接
        //银行转帐
        string bankcode;            //"wy" 网银 "kf"
        string bankname;            //"中国农业银行",
        string banknumber;          //"5665465465465456",
        string bankaccount;         //"王大锤",
        string bankaddress;         //"北京中南海",
        string bankiconherf;         //
		
        int32_t bonusProp;           //支付通道优惠百分比（2%,值则为2）
        int32_t      minquota  ;
        int32_t      maxquota  ;
        vector<int32_t> solidQuota; //支付通道的快速输入按钮上的值
    };

    typedef map<string,vector<PaymentItem>> PAY_MENT_MAP;
    // 缓存渠道代理对应关系
    std::map<int32_t,vector<PAY_MENT_MAP>> m_ChannleAgentMap;

    struct ChannleBonusItem
    {
        string      channel ;       //"wy" 网银 "kf"
        string      channelname;    //"网银支付"  "VIP转账",
        double      bonusprop;      //支付通道优惠百分比（2%,值则为2）
        double      washtotal;      //
        double      integral;       //
        int32_t     bonuslimit;     // 限额 200
    };
    // 缓存会员层级对应渠道优惠
    std::map<int32_t,std::map<string, ChannleBonusItem>> m_BonusItemMap; 

    typedef std::map<int32_t,vector<map<string,string>>> INT_STR_MAP;
	// 银行绑定信息
    INT_STR_MAP m_bankIconMap;

	// 在线支付配置
	struct OnlinePayConfig
	{
		string requrl;// = "http://api.cbddxe.com/Api/UnionPay/getPlatformProductList";
		string app_id;// = "GC99KtrmVcwVVWkIRdZIRC21";
		string user_recharge_amount;// = "0";
		string platform_product_ids;// = "1,3,5";
		string app_key;// = "2snPI9Xop6oEoZwmRE9gmRV67uJjVAw7HqzRjhP52hvkhD6gHsM857KYMDizPzjVHl1IWxQWL8EhTsgIOaJxvjosaIaEDAcg6MZ";
		string return_type;
		string notifyurl;
		string prepayurl;
		string prenotifyurl;
		string notifyIp;
		string device;
	};

    std::map<int32_t,OnlinePayConfig> m_OnlinePayCfgMap;

    std::map<int32_t,string> m_PicthPathMap;
public:
	bool loadbanklist(int32_t agentid ,string codeIcon,string & herf);
	bool get_platform_global_config(int32_t agent_id,int32_t type_id,string & details);
	int32_t cachePicPathIP(int32_t agentid,string & picPath);
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
	void processHttpRequest(const muduo::net::HttpRequest &req, muduo::net::HttpResponse &rsp);


	//请求字符串 ///
	static std::string getRequestStr(muduo::net::HttpRequest const& req);

	//设置任务消息队列大小 ///
	inline void setMaxQueueSize(int maxSize) { threadPool_.setMaxQueueSize(maxSize); }
	

	//解析请求 ///
	//strQuery string req.query()
	static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg);


private:
	void cmd_get_recharge_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
	int32_t loadRechargeInfo(int64_t userId, int32_t agentid, int32_t paytype, int32_t &level,map<string, ChannleBonusItem> & memberbonusMap);
	cpr::Payload getSignPost(string uid,OnlinePayConfig & onlinecfg);
	cpr::Parameters getSign(string & app_id,string & uid,string & user_recharge_amount,string & platform_product_ids,string & sign);
	 
	int32_t queryZxWebUrl(string & web_url,int32_t & status, string requrl,string & errmsg, cpr::Payload param);

	bool readWithoutNameJsonArrayString(std::string strWithNameJson, std::string & strWithoutNameJson);
    bool cacheMemberBonusInfo(int32_t level,int32_t agentid,int32_t mapIdx, string & channelbonusJson);
    void cacheChannleWY(int32_t agentid,int32_t level, vector<string> & bankcardidVec);
    void cacheChannleThirdParty(int32_t agentid,int32_t level, int32_t paytype,vector<string> & paychannelVec);
    void cacheChannleOnline(int32_t agentid,int32_t level, int32_t paytype, vector<string> & onlineidVec,string paymentchannel);

	bool sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data,internal_prev_header *internal_header, Header *commandHeader);

private:
	//指定网卡ipaddr
	std::string strIpAddr_;
	//监听HTTP请求
	muduo::net::TcpServer httpServer_;
	//业务线程池，内部自实现任务消息队列
	muduo::ThreadPool threadPool_;
	//请求处理回调，但非线程安全的
	HttpCallback httpCallback_;

	muduo::net::TcpServer m_server;
    muduo::ThreadPool m_thread_pool;

	//统计TCP/HTTP连接数量，"Connection:keep-alive"
	muduo::AtomicInt64 numConnections_;
	muduo::AtomicInt64 numReqCount_;
	muduo::AtomicInt64 numReqOKCount_;
	muduo::AtomicInt64 numReqErrCount_;

	AtomicInt32 m_connection_num;

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
	string createPayParams(string lgname, string billNo, string type,string creditStr);
	// 游戏转账（上下分）
	bool transferBalance(string & lgname,string billNo, string type,int64_t userid,int64_t credit,int64_t & _newbalance,int32_t & req_code,string & errmsg,string & da_account,cpr::Response & resp);
	// 进入游戏
	bool enterGameParams(string & lgname,string mh5,string oddtype,string & req_forwardUrl,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 统一请求参数
	cpr::Response getUrlRequest(string & parameContent,string & reqHandle);
	// 检查请求参数
	int32_t checkReqParams(string & reqStr,int64_t & uid,int32_t & actype,string & lgtype,string & oddtype,string & backurl, string & errmsg);
	// 解析返回结果
	bool getJsonPtree( string & jsonStr,int32_t & code,string & msg,boost::property_tree::ptree & pt);
	// 获取用户信息
	bool getUserNameById(int64_t uid, int32_t & aid, string &lgname,int64_t & score,string & da_account, int32_t & req_code,string & errmsg);
	// 创建订单
	void createOrder(string & billNo,string & lgname,string type,int64_t credit,int64_t userid,string da_account,int64_t oldmoney = 0,int64_t newmoney = 0); 
	// 更新订单状态
	void updateOrder(int64_t userid,string type,int64_t credit,string & billNo,int32_t stauts,int32_t status_code,double elapsedtime,int64_t oldmoney = 0,int64_t newmoney = 0); 
	// 设置锁(使用条件:允许锁的偶尔失效)
	bool setNxDelay(int64_t lckuid);
	// 检查连接有效性
	bool checkUrlValid( string reqHandle,int32_t & req_code,string & errmsg);
	// 创建跳转成功记录
	void updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t type,int32_t online = 1);
	// 获取维护状态
	bool getMaintenanceStatus(int32_t & req_code,string & errmsg);
	// 查询余额状态,type 0 统一回收;1 指定帐号回收
	void recycleBalance(int type,string account);
	// 是否允许访问
	bool isPermitPay(int32_t _agentId);
	// 加载Json文件
	bool loadJson();
	// 签名算法
	cpr::Parameters createSignatureParam(string parameContent);
	// 构造返回结果 ///
	string createJsonString(int32_t opType,int32_t errcode,string const& dataStr);
	string createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr);

	int32_t cacheOnlinePayInfo(int32_t agentid,string paymentconfig,OnlinePayConfig & onlinecfg_);
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
 
	// 代理信息
	struct agent_info_t {
		//https://api.txhbet.net 
		std::string ApiUrl;            
		//GSwydAem 
		std::string DesCode;      
		//4q8JXdwT       
		std::string MD5Code;     
		//天下滙真人视讯       
		std::string PlatformName;    
		//"TXH"   
		std::string PlatformCode;  
		//"txwl     
		std::string MerchantNum;       
		int64_t 	aid;            	//agentid 
	};

	//代理信息map[agentId]=agent_info_
	agent_info_t agent_info_;
	// std::map<int32_t, agent_info_t> agent_info_;
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
	IniConfig inicfg_;

	std::map<int32_t, string> errcode_info_;
 
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

    shared_ptr<EventLoopThread> threadTimer_;
	shared_ptr<EventLoopThread> m_tcp_timer_thread;
};

#endif // LOGINSERVER_H
