#ifndef ORDERSERVER_H
#define ORDERSERVER_H

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

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include <cpr/cpr.h>

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "RedisClient/RedisClient.h"
#include "RedisClient/redlock.h"

#include "IPFinder.h"
#include "RocketMQ/RocketMQ.h"

#include "muduo/base/Logging.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"

#include <map>
#include <string>

#include "public/MyEventLoopThreadPool.h"

#include "BackCodeSet.h"

#include <ctime>
#include <iostream>
#include <iomanip>

#ifndef NotScore
#define NotScore(a) ((a)<0.01f)
#endif

//IP访问白名单控制 ///
enum eWhiteListCtrl {
	Close      = 0,
	Open       = 1,//应用层IP截断
	OpenAccept = 2,//网络底层IP截断
};

// 审核状态　status
enum eAuditType {
	pass      = 0,//恢复正常
	auditOK = 1,//审核通过
	error       = 2,//有异常需要审核
	auditNo = 3,//审核不通过
};

// 拦截类型　type	//1, type拦截类型,0分数大于10倍，1,分数大于10W ，3，其它异常
enum eHeadOffType {
	tenTimes  =  0,//分数大于N倍
	toolarge = 1,//分数太大，多于N W分
	other = 2,//其它异常
};

//数据库性能评测：整体性能对比
//https://cloud.tencent.com/developer/article/1005399
//阿里双11秒杀系统如何设计
//https://www.jianshu.com/p/09693cc14d49

//创建 Collection
//db.createCollection("add_score_order")
//db.createCollection("sub_score_order")
//db.createCollection("user_score_record")

//#define _STAT_ORDER_QPS_			//间隔deltaTime统计QPS
//#define _STAT_ORDER_QPS_DETAIL_	//上下分QPS细节，单个函数处理性能指标
//#define _NO_LOGIC_PROCESS_		//不带逻辑空请求
//#define _EVENTLOOP_CONTEXT_       //最高效的cpu利用率，没有线程切换开销，没有共享锁竞态 ///
//#define _TCP_NOTIFY_CLIENT_       //走TCP通知客户端，否则走redis广播

/*
	HTTP/1.1 400 Bad Request\r\n\r\n
	HTTP/1.1 404 Not Found\r\n\r\n
	HTTP/1.1 405 服务维护中\r\n\r\n
	HTTP/1.1 500 IP访问限制\r\n\r\n
	HTTP/1.1 504 权限不够\r\n\r\n
	HTTP/1.1 505 timeout\r\n\r\n
	HTTP/1.1 600 访问量限制(1500)\r\n\r\n
*/

#define REDISLOCK RedisLock::ThreadLocalSingleton::instance()

#define MY_TRY()	\
	try {

#define MY_CATCH() \
	} \
catch (const bsoncxx::exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
} \
catch (const muduo::Exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
	} \
catch (const std::exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
} \
catch (...) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught "; \
	throw; \
} \

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

static void setFailedResponse(muduo::net::HttpResponse& rsp,
	muduo::net::HttpResponse::HttpStatusCode code = muduo::net::HttpResponse::k200Ok,
	std::string const& msg = "") {
	rsp.setStatusCode(code);
	rsp.setStatusMessage("OK");
	rsp.addHeader("Server", "TXQP");
#if 0
	rsp.setContentType("text/html;charset=utf-8");
	rsp.setBody("<html><body>" + msg + "</body></html>");
#elif 0
	rsp.setContentType("application/xml;charset=utf-8");
	rsp.setBody(msg);
#else
	rsp.setContentType("text/plain;charset=utf-8");
	rsp.setBody(msg);
#endif
}

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class OrderServer : public muduo::noncopyable
{
public:
    typedef std::map<std::string, muduo::net::TcpConnectionWeakPtr> WeakConnMap;
	typedef std::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
	typedef std::map<std::string, TcpClientPtr> TcpClientMap;

	typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

	typedef std::map<std::string, std::string> HttpParams;

	typedef std::function<void(
		const muduo::net::HttpRequest&,
		muduo::net::HttpResponse&, muduo::Timestamp receiveTime)> HttpCallback;

	muduo::net::EventLoop* getLoop() const { return httpServer_.getLoop(); }

public:
	OrderServer(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrHttp,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    ~OrderServer();
	
	//////////////////////////////////////////////////////////////////////////
	//@@ Entry 避免恶意连接占用系统sockfd资源不请求处理也不关闭fd情况，超时强行关闭连接
	struct Entry : public muduo::noncopyable {
	public:
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn)
			: weakConn_(weakConn) {
		}
		inline muduo::net::WeakTcpConnectionPtr const& getWeakConnPtr() {
			return weakConn_;
		}
#if 0
		~Entry() {
			muduo::net::TcpConnectionPtr conn(weakConn_.lock());
			if (conn) {
				conn->getLoop()->assertInLoopThread();
				LOG_WARN << __FUNCTION__ << " Entry::dtor";
#ifdef _DEBUG_BUCKETS_
				LOG_WARN << __FUNCTION__ << " 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
					<< conn->localAddress().toIpPort() << "] timeout closing";
#endif
#if 0
				//不再发送数据
				conn->shutdown();
#elif 0
				//直接强制关闭连接
				conn->forceClose();
#else
				//HTTP应答包(header/body)
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 505 timeout\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.4f);
#endif
			}
		}
#else
		~Entry();
#endif
		muduo::net::WeakTcpConnectionPtr weakConn_;
	};

	typedef std::shared_ptr<Entry> EntryPtr;
	typedef std::weak_ptr<Entry> WeakEntryPtr;
	//boost::unordered_set改用std::unordered_set
	typedef std::unordered_set<EntryPtr> Bucket;
	typedef boost::circular_buffer<Bucket> WeakConnList;

	//////////////////////////////////////////////////////////////////////////
	//@@ ConnBucket
	struct ConnBucket : public muduo::noncopyable {
		explicit ConnBucket(muduo::net::EventLoop* loop, int index, size_t size)
			:loop_(CHECK_NOTNULL(loop)), index_(index) {
			//指定时间轮盘大小(bucket桶大小)
			//即环形数组大小(size) >=
			//心跳超时清理时间(timeout) >
			//心跳间隔时间(interval)
			buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
			LOG_WARN << __FUNCTION__ << " loop[" << index << "] timeout = " << size << "s";
#endif
		}
		//tick检查，间隔1s，踢出超时conn
		void onTimer() {
			loop_->assertInLoopThread();
			buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
			LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "]";
#endif
			//重启连接超时定时器检查，间隔1s
			loop_->runAfter(1.0f, std::bind(&ConnBucket::onTimer, this));
		}
		//连接成功，压入桶元素
		void pushBucket(EntryPtr const& entry) {
			loop_->assertInLoopThread();
			if (likely(entry)) {
				muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				if (likely(conn)) {
					assert(conn->getLoop() == loop_);
					//必须使用shared_ptr，持有entry引用计数(加1)
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
						<< conn->localAddress().toIpPort() << "]";
#endif
				}
			}
			else {
				//assert(false);
			}
		}
		//收到消息包，更新桶元素
		void updateBucket(EntryPtr const& entry) {
			loop_->assertInLoopThread();
			if (likely(entry)) {
				muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				if (likely(conn)) {
					assert(conn->getLoop() == loop_);
					//必须使用shared_ptr，持有entry引用计数(加1)
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
						<< conn->localAddress().toIpPort() << "]";
#endif
				}
			}
			else {
				//assert(false);
			}
		}
		//bucketsPool_下标
		int index_;
		WeakConnList buckets_;
		muduo::net::EventLoop* loop_;
	};

	typedef std::unique_ptr<ConnBucket> ConnBucketPtr;

	//////////////////////////////////////////////////////////////////////////
	//@@ Context
	struct Context : public muduo::noncopyable {
		explicit Context()
			: index_(0XFFFFFFFF) {
			reset();
		}
		explicit Context(WeakEntryPtr const& weakEntry)
			: index_(0XFFFFFFFF), weakEntry_(weakEntry) {
			reset();
		}
		explicit Context(const boost::any& context)
			: index_(0XFFFFFFFF), context_(context) {
			assert(!context_.empty());
			reset();
		}
		explicit Context(WeakEntryPtr const& weakEntry, const boost::any& context)
			: index_(0XFFFFFFFF), weakEntry_(weakEntry), context_(context) {
			assert(!context_.empty());
			reset();
		}
		~Context() {
			//LOG_WARN << __FUNCTION__ << " Context::dtor";
			reset();
		}
		//reset
		inline void reset() {
			ipaddr_ = 0;
			session_.clear();
		}
		inline void setWorkerIndex(int index) {
			index_ = index;
			assert(index_ >= 0);
		}
		inline int getWorkerIndex() const {
			return index_;
		}
		inline void setContext(const boost::any& context) {
			context_ = context;
		}
		inline const boost::any& getContext() const {
			return context_;
		}
		inline boost::any& getContext() {
			return context_;
		}
		inline boost::any* getMutableContext() {
			return &context_;
		}
		inline WeakEntryPtr const& getWeakEntryPtr() {
			return weakEntry_;
		}
		//ipaddr
		inline void setFromIp(in_addr_t inaddr) { ipaddr_ = inaddr; }
		inline in_addr_t getFromIp() { return ipaddr_; }
		//session
		inline void setSession(std::string const& session) { session_ = session; }
		inline std::string const& getSession() const { return session_; }
	public:
		//threadPool_下标
		int index_;
		uint32_t ipaddr_;
		std::string session_;
		WeakEntryPtr weakEntry_;
		boost::any context_;
};

	typedef std::shared_ptr<Context> ContextPtr;

	//////////////////////////////////////////////////////////////////////////
	//@@ EventLoopContext
	class EventLoopContext : public muduo::noncopyable {
	public:
		explicit EventLoopContext()
			: index_(0xFFFFFFFF) {
		}
		explicit EventLoopContext(int index)
			: index_(index) {
			assert(index_ >= 0);
		}
#if 0
		explicit EventLoopContext(EventLoopContext const& ref) {
			index_ = ref.index_;
			pool_.clear();
#if 0
			std::copy(ref.pool_.begin(), ref.pool_.end(), pool_.begin());
#else
			std::copy(ref.pool_.begin(), ref.pool_.end(), std::back_inserter(pool_));
#endif
		}
#endif
		inline void setBucketIndex(int index) {
			index_ = index;
			assert(index_ >= 0);
		}
		inline int getBucketIndex() const {
			return index_;
		}
		inline void addWorkerIndex(int index) {
			pool_.emplace_back(index);
		}
		inline int allocWorkerIndex() {
			int index = nextPool_.getAndAdd(1) % pool_.size();
			//nextPool_溢出
			if (index < 0) {
				nextPool_.getAndSet(-1);
				index = nextPool_.addAndGet(1);
			}
			assert(index >= 0 && index < pool_.size());
			return pool_[index];
		}
		~EventLoopContext() {
		}
	public:
		//bucketsPool_下标
		int index_;
		//threadPool_下标集合
		std::vector<int> pool_;
		//pool_游标
		muduo::AtomicInt32 nextPool_;
	};

	typedef std::shared_ptr<EventLoopContext> EventLoopContextPtr;

	//Bucket池处理连接超时conn对象
	std::vector<ConnBucketPtr> bucketsPool_;

	/// Not thread safe, callback be registered before calling start().
	void setHttpCallback(const HttpCallback& cb)
	{
		httpCallback_ = cb;
	}

    //注册handler ///
	void initModuleHandlers();

    void quit();
public:
	//MongoDB/redis与线程池关联 ///
	void threadInit();
	bool InitZookeeper(std::string ipaddr);
	bool InitRedisCluster(std::string ipaddr, std::string password);
	bool InitMongoDB(std::string url);
	//bool InitRocketMQ(std::string& ipaddr);
	//uint32_t RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe);
	//上下分操作处理回调 ///
	//uint32_t ConsumerClusterScoreOrderCallback(const vector<MQMessageExt> &mqe);
	//停止上下分操作处理 ///
	//void stopConsumerClusterScoreOrder();

	//////////////////////////////////////////////////////////////////////////
	//zookeeper
public:
	//zookeeper连接成功回调 ///
	void ZookeeperConnectedHandler();

	//自注册服务节点到zookeeper ///
	void OnRepairServerNode();

	//HttpServer通知停止上下分操作处理 ///
    //void cmd_repair_OrderServer(muduo::net::TcpConnectionWeakPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header);

	//ProxyServer节点变更处理回调 ///
	void GetProxyServersChildrenWatcherHandler(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	
    //订单服[C]端 -> 网关服[S]端 ///
	void startAllProxyServerConnection();

#ifdef _TCP_NOTIFY_CLIENT_
	//订单服[C]端 -> 网关服[S]端 ///
	void startProxyServerConnection(std::string ipaddr);

	//订单服[C]端 -> 网关服[S]端 ///
	void processProxyIPServer(std::vector<std::string>& newProxyServers);
#endif

	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务 ///
	bool repairApiServer(std::string const& queryStr);
	
	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务 ///
	void repairApiServerNotify(std::string const& msg);
	
	//服务状态 ///
	volatile long server_state_;
private:
	std::list<std::string> proxyServerIps_;
	muduo::MutexLock mutexProxyServerIps_;

	//////////////////////////////////////////////////////////////////////////
	//tcpclient 
public:
	//启动TCP业务线程 ///
	//启动TCP监听 ///
	void start(int numThreads, int workerNumThreads);
	
#ifdef _TCP_NOTIFY_CLIENT_
	//连接事件来自 网关服(ProxyServer) ///
	//Connected/closed事件 ///
    //订单服[C]端 -> 网关服[S]端 ///
	void onConnection(const muduo::net::TcpConnectionPtr& conn);

	//接收TCP网络消息回调 ///
	//处理来自 网关服(ProxyServer) 消息 ///
    //网关服[S]端 -> 订单服[C]端 ///
	void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	//处理来自 网关服(ProxyServer) 消息 ///
    void processRequest(muduo::net::TcpConnectionWeakPtr& conn, BufferPtr& buf, muduo::Timestamp receiveTime);
#endif

#ifdef _TCP_NOTIFY_CLIENT_
private:
	//weakConns_[ip:port:port:pid]= conn ///
	WeakConnMap weakConns_;
	muduo::MutexLock mutexWeakConns_;

	//clientMap_[ip:port:port:pid]= tcpclient ///
	TcpClientMap clientMap_;
	muduo::MutexLock mutexClientMap_;
	//处理TcpClient网络IO(connector/recv/send) ///
	std::shared_ptr<Landy::MyEventLoopThreadPool> clientThreadPool_;
#endif
	//////////////////////////////////////////////////////////////////////////
	//httpserver
public:
	//启动HTTP业务线程 ///
	//启动HTTP监听 ///
	void startHttpServer(int numThreads, int workerNumThreads, int maxSize);

	//白名单检查 ///
	bool onHttpCondition(const InetAddress& peerAddr);

	//Connected/closed事件 ///
	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);

	//接收HTTP网络消息回调 ///
	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void onWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	
	//异步回调 ///
	void asyncHttpHandler(muduo::net::WeakTcpConnectionPtr const& weakConn, muduo::Timestamp receiveTime);
	
	//处理HTTP回调 ///
	void processHttpRequest(const muduo::net::TcpConnectionPtr& conn, const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp, muduo::net::InetAddress const& peerAddr, muduo::Timestamp receiveTime);

	//请求字符串 ///
	static std::string getRequestStr(muduo::net::HttpRequest const& req);

	//解析请求 ///
	//strQuery string req.query()
	static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg);
public:
	//I/O线程数，worker线程数
	int numThreads_, workerNumThreads_;
	//指定网卡ipaddr
	std::string strIpAddr_;
	//监听HTTP请求
	muduo::net::TcpServer httpServer_;
#ifdef SHARED_THREADPOOL
	//业务线程池，内部自实现任务消息队列
	muduo::ThreadPool threadPool_;
#else
	//threadPool_游标 ///
	muduo::AtomicInt32 nextThreadPool_;
	//业务线程池，内部自实现任务消息队列
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
#endif
	//请求处理回调，但非线程安全的
	HttpCallback httpCallback_;

	//统计TCP/HTTP连接数量，"Connection:keep-alive"
	muduo::AtomicInt32 numConnected_;
	//累计接收请求数，累计未处理请求数 ///
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;

	//最大连接数限制 ///
	int kMaxConnections_;

	//指定时间轮盘大小(bucket桶大小) ///
	//即环形数组大小(size) >=
	//心跳超时清理时间(timeout) >
	//心跳间隔时间(interval)
	int kTimeoutSeconds_;

	//输出性能指标日志 ///
	void refreshQPSLog(std::string const& str);
	//std::shared_ptr<muduo::ThreadPool> threadQPSLog_;
private:
	/* 返回格式 ///
	{
		"maintype": "/GameHandle",
			"type": 2,
			"data":
			{
				"orderid":"",
				"agentid": 10000,
				"account": "999",
				"score": 10000,
				"code": 0,
				"errmsg":"",
			}
	}
	*/
	// 构造返回结果 ///
	std::string createResponse(
		int32_t opType,
		std::string const& orderId,
		uint32_t agentId,
		std::string account, double score,
		int errcode, std::string const& errmsg, bool debug);

	//最近一次请求(上分或下分操作的elapsed detail)
	static void createLatestElapsed(
		boost::property_tree::ptree& latest,
		std::string const& op, std::string const& key, double elapsed);
	
	//监控数据
	static std::string createMonitorData(
		boost::property_tree::ptree const& latest, double totalTime, int timeout,
		int64_t requestNum, int64_t requestNumSucc, int64_t requestNumFailed, double ratio,
		int64_t requestNumTotal, int64_t requestNumTotalSucc, int64_t requestNumTotalFailed, double ratioTotal, int testTPS);

	//判断是否数字组成的字符串 ///
	static inline bool IsDigitStr(std::string const& str) {
#if 0
		boost::regex reg("^[1-9]\d*\,\d*|[1-9]\d*$");
#elif 1
		boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
#elif 0
		boost::regex reg("^[+-]?(0|([1-9]\d*))(\.\d+)?$");
#elif 0
		boost::regex reg("[1-9]\d*\.?\d*)|(0\.\d*[1-9]");
#endif
		return boost::regex_match(str, reg);
	}
	
	//E11000 duplicate key
	static inline int getMongoDBErrno(std::string const& errmsg) {
		if (!errmsg.empty() && errmsg[0] == 'E') {
			std::string::size_type npos = errmsg.find_first_of(' ');
			if (npos != std::string::npos && npos > 2) {
				std::string numStr = errmsg.substr(1, npos - 1);
				if (boost::regex_match(numStr, boost::regex("^[1-9]*[1-9][0-9]*$"))) {
					return atol(numStr.c_str());
				}
			}
		}
		return 0XFFFFFFFF;
	}

	//redis获取用户登陆信息 ///
	bool userLoginProxyInfo(const string& session, int64_t& userId, string& account, uint32_t& agentId);
    
#ifdef _TCP_NOTIFY_CLIENT_
	//订单服[C]端 -> 网关服[S]端 ///
    //先查询Redis得到userId的网关服，然后发送
    void sendUserData(int64_t userId, uint8_t mainId, uint8_t subId, char const* msgdata, size_t dataLen);
#endif

public:

	// 代理信息 ///
	struct agent_info_t {
		int64_t		score;              //代理分数 
		int32_t		status;             //是否被禁用 0正常 1停用
		int32_t		agentId;            //agentId
		int32_t		cooperationtype;    //合作模式  1 买分 2 信用
		std::string descode;            //descode 
		std::string md5code;            //MD5 
	};

	struct IniConfig
	{
		int32_t addtime;
		int32_t auditThreshold;
		int32_t minscore;
	};

	//刷新所有agent_info信息 ///
	//1.web后台更新代理通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次
	bool refreshAgentInfo();
	
	//刷新所有IP访问白名单信息 ///
	//1.web后台更新白名单通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次 ///
	void refreshWhiteList();
	//同步刷新IP访问白名单
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();
	
	//订单处理函数 ///
	std::string OrderProcess(
		const muduo::net::TcpConnectionPtr& conn,
		std::string const& reqStr, muduo::Timestamp receiveTime, int& errcode, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//上下分操作 ///
	int doOrderExecute(int32_t opType, std::string const& account, double score, int64_t scoreI64, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//上分写db操作 ///
	int AddOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//下分写db操作 ///
	int SubOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);

	// 锁定用户 
	bool lockUserScore(int64_t uid,int32_t opType, int32_t & req_code,string & errmsg);

	// 解锁用户 
	bool unlockUserScore(int64_t uid,int32_t opType,int32_t & req_code,string & errmsg);
	// 
	bool checkAgentStatus(int32_t agentid);
	// 刷新一次维护代理的ID
	void refreshRepairAgentInfo();
	// 拦截分数判断
	bool hugeProfitCheck(int64_t uid,int64_t curscore,int32_t agentid,string account);
	bool auditform(bool isOnly,int32_t agentid,int64_t uid,int32_t type,int32_t status,string account,int64_t curscore,int64_t lastAddScoreValue,int64_t tempScore =0,int32_t addtime=0);
	bool LoadIniConfig();


	MutexLock m_mutex_ids;
    vector<int32_t> uphold_agent_ids;      //代理维护名单

	//代理信息map[agentId]=agent_info_
	std::map<int32_t, agent_info_t> agent_info_;
	mutable boost::shared_mutex agent_info_mutex_;
	bool isdecrypt_;

	//IP访问白名单信息 ///
	std::map<in_addr_t, eApiVisit> white_list_;
	mutable boost::shared_mutex white_list_mutex_;
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	eWhiteListCtrl whiteListControl_;
	//管理员挂维护/恢复服务 ///
	std::map<in_addr_t, eApiVisit> admin_list_;

	//redis分布式锁 ///
	std::vector<std::string> redlockVec_;
	//上下分操作间隔时间(针对用户/代理) ///
	int ttlUserLockSeconds_, ttlAgentLockSeconds_;
	//订单缓存过期时间
	int orderIdExpiredSeconds_;
	//是否连续锁
	bool redLockContinue_;

	IniConfig inicfg_;

	//mutable boost::shared_mutex update_score_mutex_;
    MutexLock update_score_mutex_;

	// 回收加锁
	mutable boost::shared_mutex recycle_mutex_;
private:
    const static size_t kHeaderLen = sizeof(int16_t);

private:
    std::shared_ptr<ZookeeperClient> m_zookeeperclient;
    std::string m_szNodePath, m_szNodeValue, m_szInvalidNodePath, m_szUserOrderScorePath;

    std::shared_ptr<RedisClient> m_redisPubSubClient;
    std::string m_redisIPPort;
    std::string m_redisPassword;

    std::string m_mongoDBServerAddr;

    //RocketMQ m_ConsumerClusterScoreOrder;
    //RocketMQ m_Producer;

    CIpFinder m_pIpFinder;

private:
    bool m_bInvaildServer;
private:

    std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
public:
#ifdef _STAT_ORDER_QPS_
	//性能测试指标 间隔输出时间(s) ///
	int deltaTime_;
#endif
};

#endif // ORDERSERVER_H
