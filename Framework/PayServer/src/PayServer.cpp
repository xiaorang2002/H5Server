#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp> 
#include <boost/property_tree/json_parser.hpp>

#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>

#include "Globals.h"
#include "gameDefine.h"
#include "GlobalFunc.h"
#include "ThreadLocalSingletonMongoDBClient.h" 
#include "TraceMsg/FormatCmdId.h"
#include "SubNetIP.h"
#include "json/json.h"
// #include "crypto/crypto.h"
#include "mymd5.h"
#include "base64.h"
#include "htmlcodec.h"
#include "crypto.h"
#include "urlcodec.h" 
#include "PayServer.h"

#include "../../proto/Game.Common.pb.h"
#include "../../proto/HallServer.Message.pb.h"
#include "../../public/cryptopublic.h"
// #ifdef __cplusplus
// extern "C" {
// #endif 

// #ifdef __cplusplus
// }
// #endif

extern int g_bisDebug;
extern int g_EastOfUtc;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;
 
using namespace bsoncxx::types;
using namespace muduo::net;


using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

std::map<int, PayServer::AccessCommandFunctor>  PayServer::m_functor_map;

// 大唐帐转进入天下汇
#define TRANS_IN  	"IN"
// 天下汇帐转进入大唐
#define TRANS_OUT  	"OUT"
// 版本
#define VERSION  	"100"

 
PayServer::PayServer(EventLoop *loop, const InetAddress &listenAddr, const InetAddress &listenAddrHttp, string const &strIpAddr, string netcardName, TcpServer::Option option)
	: httpServer_(loop, listenAddrHttp, "PayHttpServer", option),  /*http 服务*/
	m_server(loop, listenAddr, "PayServer"), /*tcp 服务*/
	threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "HttpTimerEventLoopThread")), 
	m_tcp_timer_thread(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "TcpTimerEventLoopThread")), 
	m_pIpFinder("qqwry.dat"), 
	strIpAddr_(strIpAddr), 
	m_bInvaildServer(false), 
	isStartServer(false)
{
	errcode_info_.clear();
	// 
	 m_pattern= boost::regex("\n|\r"); 

	init();

	setHttpCallback(CALLBACK_2(PayServer::processHttpRequest, this)); 
	httpServer_.setConnectionCallback(CALLBACK_1(PayServer::onHttpConnection, this));
	httpServer_.setMessageCallback(CALLBACK_3(PayServer::onHttpMessage, this)); 

	m_server.setConnectionCallback(CALLBACK_1(PayServer::onConnection, this));
    m_server.setMessageCallback(CALLBACK_3(PayServer::onMessage, this)); 
    
	threadTimer_->startLoop();
	m_tcp_timer_thread->startLoop();

	srand(time(nullptr));
}

PayServer::~PayServer()
{
    m_functor_map.clear();
    quit();
}

void PayServer::init()
{
	int32_t cmdId = (::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PAY_SERVER << 8) | ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_INFO_REQ;
    m_functor_map[cmdId] = CALLBACK_4(PayServer::cmd_get_recharge_info, this);

}
 
void PayServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
    threadPool_.stop();
}

//MongoDB/redis与线程池关联 ///
void PayServer::threadInit()
{
	LOG_INFO << __FUNCTION__;
	if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
	{
		LOG_ERROR << "RedisClient Connection ERROR!";
		return;
	}

	ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
	mongocxx::database db = MONGODBCLIENT["gamemain"];
}

bool PayServer::InitZookeeper(std::string ipaddr)
{
    LOG_INFO << __FUNCTION__ <<" ipaddr["<< ipaddr <<"]";
	m_zookeeperclient.reset(new ZookeeperClient(ipaddr));
    //指定zookeeper连接成功回调 ///
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&PayServer::ZookeeperConnectedHandler, this));
    //连接到zookeeper，成功后回调ZookeeperConnectedHandler ///
    if (!m_zookeeperclient->connectServer()) {
        LOG_ERROR << "InitZookeeper error";
        // abort();
    }

    LOG_INFO << "---InitZookeeper OK !";

	return true;
}

bool PayServer::InitRedisCluster(std::string ipaddr, std::string password)
{
	LOG_INFO <<"---ipaddr["<< ipaddr <<"],password["<<password<<"]";

	m_redisPubSubClient.reset(new RedisClient());
	if (!m_redisPubSubClient->initRedisCluster(ipaddr, password)) {
		LOG_ERROR << "InitRedisCluster error,ipaddr[" << ipaddr <<"]";
	}
	// 保存
	m_redisPassword = password;
	//更新代码消息
	// m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_Trans_proxy_info,CALLBACK_0(PayServer::refreshAgentInfo,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_Trans_server,CALLBACK_1(PayServer::repairLoginServer,this)); 
	// redis广播
	// m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_noticfy_recycle_balance, [&](string msg) {
	// 	LOG_INFO << "---收到回收余额消息[" << msg << "],IsMaster[" << inicfg_.IsMaster <<"]";
	// 	if (inicfg_.IsMaster == 1)
	// 	{
	// 		threadTimer_->getLoop()->runInLoop(CALLBACK_0(PayServer::recycleBalance,this,(int32_t)eRecycleType::one,msg)); 
	// 	}
	// });

	 // 更新代理银行信息时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_bank_list_info,[&](string msg){ 
        m_bankIconMap.clear();
        LOG_ERROR << "-------------update bank list------------"<< msg;
    }); 

	// 更新渠道信息时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_pay_channel_list,[&](string msg){ 
        m_ChannleAgentMap.clear();
        m_BonusItemMap.clear();
        m_OnlinePayCfgMap.clear();
        m_PicthPathMap.clear();
        LOG_ERROR << "-------------update channle list 更新渠道信息、会员层级信息------------"<< msg;
    }); 

	// 
	m_redisPubSubClient->startSubThread();

	m_redisIPPort = ipaddr;

	return true;
}

bool PayServer::InitMongoDB(std::string url)
{
	LOG_INFO << __FUNCTION__ << "---DB url " << url;
    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}

//zookeeper连接成功回调 ///
void PayServer::ZookeeperConnectedHandler()
{
	LOG_DEBUG << __FUNCTION__ << "=======zookeeper 连接 成功 回调================>>>>>";
    if(ZNONODE == m_zookeeperclient->existsNode(ZK_ROOT_NODE))
        m_zookeeperclient->createNode(ZK_ROOT_NODE, "Landy");

	string PayServersNode = ZK_ROOT_NODE + PAY_SERVER;
    //PayServers节点集合
    if(ZNONODE == m_zookeeperclient->existsNode(PayServersNode))
        m_zookeeperclient->createNode(PayServersNode, PAY_SERVER);

	//PayServersInvalid节点集合
	string PayServersInvalidNode = ZK_ROOT_NODE + PAY_SERVER_INVAILD;
    if(ZNONODE == m_zookeeperclient->existsNode(PayServersInvalidNode))
        m_zookeeperclient->createNode(PayServersInvalidNode, PAY_SERVER_INVAILD);
 
    //本地节点启动时自注册 ///
	{
        //指定网卡ip:port ///
		std::vector<std::string> vec;
		boost::algorithm::split(vec, m_server.ipPort(), boost::is_any_of(":"));
		
		m_szNodeValue = strIpAddr_ + ":" + vec[1];
		m_szNodePath =   PayServersNode + "/" + m_szNodeValue;// "/GAME/PayServers/" + m_szNodeValue;
		//自注册LoginServer节点到zookeeper
		if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
			m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
			LOG_DEBUG << __FUNCTION__ << "=======zookeeper 创建临时节点========[" << m_szNodeValue << "]";
		}
		m_szInvalidNodePath = ZK_ROOT_NODE + PAY_SERVER_INVAILD + "/" + m_szNodeValue;// "/GAME/PayServersInvaild/" + m_szNodeValue;

	}  
	LOG_DEBUG << __FUNCTION__ << "=======zookeeper 连接 回调 完成================>>>>>";
}

//自注册服务节点到zookeeper ///
void PayServer::OnRepairServerNode()
{
	if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
		LOG_WARN << "--- *** " << m_szNodePath;
		m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true); 
	}  
}

//通知停止上下分操作处理 
// 维护登录服
void PayServer::repairLoginServer(string msg)
{
	LOG_WARN << "---bc_uphold_login_server:"<< msg;

	string ipaddr;
	int32_t status = eUpholdType::op_start; //维护状态
	 
	Json::Reader reader;
    Json::Value root;

	try
    {
		if (reader.parse(msg, root))
		{
			status = root["status"].asInt();
			ipaddr = root["ip"].asString();
			GlobalFunc::trimstr(ipaddr);

			//如果需要维护，但是非维护状态
			if( ipaddr.empty() ){
				// 全部维护
				m_bInvaildServer = (status == eUpholdType::op_stop);
				LOG_WARN << "---repair PayServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else if ( m_szNodeValue.compare(ipaddr) == 0 )
			{
				m_bInvaildServer = (status == eUpholdType::op_stop);
				LOG_WARN << "---repair PayServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else
				LOG_ERROR << "---repair error,ipaddr[" << ipaddr << "],localIp[" << m_szNodeValue << "]"; 
			
			LOG_ERROR << "---0开启;1维护,结果[" << m_bInvaildServer<<"]";
		}
		else
		{
			LOG_ERROR << "---Json msg parse error,operate failure :" << msg;
		}
	}
	catch(exception excep)
    {
        LOG_ERROR << __FUNCTION__ <<" "<< excep.what();
    } 
} 
 
//启动TCP业务线程 ///
//启动TCP监听 ///
void PayServer::start(int numThreads, int workerNumThreads)
{
	// 最晚一个启动的服务结束后开启变量 isStartServer = true
	m_tcp_timer_thread->getLoop()->runEvery(5.0f, bind(&PayServer::OnRepairServerNode, this)); 
	// m_tcp_timer_thread->getLoop()->runAfter(1.5f, bind(&PayServer::refreshAgentInfo, this));
	m_tcp_timer_thread->getLoop()->runEvery(1.0f, [&]() {

		// 挂维护判断
		if (inicfg_.isOnService == 1)
		{
			if (!m_bInvaildServer)
			{
				LOG_DEBUG << "---挂维护---[" << m_szInvalidNodePath << "],[" << m_szNodeValue << "]";
				if (ZNONODE == m_zookeeperclient->existsNode(m_szInvalidNodePath))
				{
					m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true);
					m_bInvaildServer = true;
					LOG_ERROR << "---挂维护成功---[" << m_szInvalidNodePath << "],[" << m_szNodeValue << "]";
				}
				else
				{
					LOG_INFO << "存在节点[" << m_szInvalidNodePath << "]";
				}
			}
		}
		else
		{
			if(m_bInvaildServer)
			{
				m_bInvaildServer = false;

				if(ZOK == m_zookeeperclient->deleteNode(m_szInvalidNodePath)){
					LOG_ERROR << "---启用成功---[" << m_szInvalidNodePath << "],[" << m_szNodeValue << "]";
				}
			}
		} 

		// 固定回收周期
		if (++nSecondTick_ < inicfg_.RecycleCycle){
			// 定时加载conf配置文件
			if(nSecondTick_ > 0 && (nSecondTick_ % inicfg_.ReadConfCycle == 0)){
				LoadIniConfig();
			}
			return;
		}
		nSecondTick_ = 0; 
	
	});
	// 加载Json
	m_tcp_timer_thread->getLoop()->runAfter(0.1f, bind(&PayServer::loadJson, this));

	LOG_WARN << "---启动TCP监听--- ";
	m_server.setThreadNum(numThreads);
	LOG_INFO << __FUNCTION__ << "--- *** "
			 << "\nTcpServer = " << m_server.ipPort()
			 << " 网络IO线程数 = " << numThreads;

	m_server.start();
}

//启动HTTP业务线程 ///
//启动HTTP监听 ///
void PayServer::startHttpServer(int numThreads, int workerNumThreads) {
	//处理HTTP回调
	setHttpCallback(CALLBACK_2(PayServer::processHttpRequest,this)); 

	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&PayServer::threadInit, this));

	//指定业务线程数量，开启业务线程，处理来自HTTP逻辑业务
	threadPool_.start(workerNumThreads);

	//网络IO线程数量
	httpServer_.setThreadNum(numThreads);
	LOG_INFO << __FUNCTION__ << "--- *** "
		<< "\nHttpServer = " << httpServer_.ipPort()
		<< " 网络IO线程数 = " << numThreads
		<< " worker 线程数 = " << workerNumThreads;

	//启动HTTP监听
	httpServer_.start();
}

//Connected/closed事件 ///
void PayServer::onHttpConnection(const muduo::net::TcpConnectionPtr& conn)
{  
	if (conn->connected())
	{
		conn->setContext(muduo::net::HttpContext());
		nConnections_ = numConnections_.incrementAndGet();
		// 网络连接数
		if(inicfg_.isLog <= (int32_t)eLogLvl::LVL_0) {
			LOG_INFO << "---网络连接数["<< nConnections_ <<"],WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN");
		}
		//最大连接数限制
		if (nConnections_ > inicfg_.maxConnCount) {
			//HTTP应答包
			string jsonResStr = createJsonString(0,(int32_t)eErrorCode::MaxConnError,"maxConnCount:" + to_string(inicfg_.maxConnCount));
			muduo::net::HttpResponse rsp(false);
			setFailedResponse(rsp,muduo::net::HttpResponse::k404NotFound,jsonResStr);
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
			conn->forceClose();//强制关闭连接

			LOG_ERROR << "---最大连接数限制["<< nConnections_ <<"]["<<inicfg_.maxConnCount<<"]";
			return;
		}
	}
	else {
		nConnections_ = numConnections_.decrementAndGet();
		if(inicfg_.isLog <= (int32_t)eLogLvl::LVL_0) {
			LOG_INFO << "---网络连接数["<< nConnections_ <<"],WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN");
		}
	}
}

//接收HTTP网络消息回调 ///
void PayServer::onHttpMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime)
{
	muduo::net::HttpContext* context = boost::any_cast<muduo::net::HttpContext>(conn->getMutableContext());
	//解析HTTP数据包
	if (!context->parseRequest(buf, receiveTime)) {
		//发生错误
		conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
		conn->shutdown();
	}
	else if (context->gotAll()) { 
		// 统计
		nReqCount_ = numReqCount_.incrementAndGet();
		int64_t nConnFreqCount = nReqCount_ - nLastnReqCount;

		if (inicfg_.isLog < (int32_t)eLogLvl::LVL_3)
			LOG_INFO << "---网络消息数[" << nReqCount_ << "],每秒连接数[" << nConnFreqCount << "]";

		// 防止攻击处理
		int64_t nCurTickTime = time(nullptr);
		if( nCurTickTime - nLastTickTime >= 1 ){
			
			nLastTickTime = time(nullptr);
			nLastnReqCount = nReqCount_; 

			// 日志级别恢复
			if( nConnFreqCount < 100 && nLogLevelCopy != 0 ) {
				inicfg_.isLog = nLogLevelCopy;
				nLogLevelCopy = 0;
			}
		}

		// 频率太高则关闭(每秒1000连接)
		if ( nConnFreqCount > inicfg_.maxConnFreqCount)
		{
			string jsonResStr = createJsonString(0,(int32_t)eErrorCode::MaxFreqError,to_string(nConnFreqCount));
			muduo::net::HttpResponse rsp(false);
			setFailedResponse(rsp,muduo::net::HttpResponse::k200Ok,jsonResStr);
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
			conn->forceClose();//直接强制关闭连接

			// 提高日志级别
			if( nLogLevelCopy == 0 ) {
				nLogLevelCopy = inicfg_.isLog;
				inicfg_.isLog = (int32_t)eLogLvl::LVL_2;
			}

			LOG_ERROR << "---每秒连接数太大,关了连接["<< nConnFreqCount <<"],isLog["<< inicfg_.isLog <<"]";
			return;
		}

		// 白名单过滤
		string peerIp = context->request().getHeader("X-Forwarded-For");
        if(peerIp.empty()){
            peerIp = conn->peerAddress().toIp();
        }
		else{
			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
       	  		LOG_DEBUG <<"--->>> X-Forwarded-For["<< peerIp <<"]";

			//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP 
			boost::replace_all<std::string>(peerIp, " ", "");
			std::vector<std::string> vec;
			boost::algorithm::split(vec, peerIp, boost::is_any_of(","));
			for (std::vector<std::string>::const_iterator it = vec.begin();
				it != vec.end(); ++it) {
				if (!it->empty() &&
					boost::regex_match(*it, boost::regex(
							"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
					
					if (strncasecmp(it->c_str(), "10.", 3) != 0 &&
						strncasecmp(it->c_str(), "192.168", 7) != 0 &&
						strncasecmp(it->c_str(), "172.16.", 7) != 0) {
						peerIp = *it;
           				LOG_WARN << " >>> find default ip[" << peerIp <<"]";
						break;
					}
					else{
						peerIp = vec[0];
           				LOG_WARN << " >>> can't find ip,get default ip[" << peerIp <<"]";
						break;
					}
				}
			}
		}
		
		muduo::net::InetAddress addr(muduo::StringArg(peerIp), 0, false);
		if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
       	   LOG_INFO <<"--->>> onHttpMessage peerAddress["<< peerIp <<"],ipNetEndian["<<addr.ipNetEndian()<<"]whiteIP["<<inicfg_.isWhiteIp<<"]";

		//IP访问白名单检查
		if ( inicfg_.isWhiteIp == 1 ) {
			READ_LOCK(white_list_mutex_);
			std::map<in_addr_t, whiteIp_info_t>::const_iterator it = white_list_.find(addr.ipNetEndian());
			//不在IP访问白名单中 //0允许访问 1禁止访问
			if (it == white_list_.end()) {
				//HTTP应答包
				string jsonResStr = createJsonString(0,(int32_t)eErrorCode::WhiteIPError,peerIp);
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,muduo::net::HttpResponse::k200Ok,jsonResStr);
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);
				conn->forceClose();//直接强制关闭连接
			
       	 		if (inicfg_.isLog < (int32_t)eLogLvl::LVL_1)
					LOG_WARN <<"--->>> 不在IP访问白名单中["<< peerIp <<"],ipNetEndian["<< addr.ipNetEndian() <<"]";
				return;
			}
    	} 

		//扔给任务消息队列处理
		muduo::net::TcpConnectionWeakPtr weakConn(conn);
		threadPool_.run(std::bind(&PayServer::asyncHttpHandler, this, weakConn, context->request(),addr.ipNetEndian()));
		//释放资源
		//context->reset();
	}
}

//异步回调 ///
void PayServer::asyncHttpHandler(const muduo::net::TcpConnectionWeakPtr& weakConn, const muduo::net::HttpRequest& req,uint32_t _ipNetEndian)
{
	const string& connection = req.getHeader("Connection");

	// 试玩环境下对连接太频繁关掉
	if ( inicfg_.isDemo == 1 && inicfg_.ip_timeout > 0 ) 
	{
		muduo::net::TcpConnectionPtr conn(weakConn.lock());
		if( conn ){
			//直接强制关闭连接
			string ipStr = to_string(_ipNetEndian);
			string ipKey = REDIS_KEY_ID + to_string((int)eRedisKey::str_demo_ip_limit) + "_" + ipStr;

			if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) LOG_WARN << "---ipKey " << ipKey <<" ip_timeout "<<inicfg_.ip_timeout;
			if (REDISCLIENT.exists(ipKey)){
				LOG_WARN << "---exists ipKey " << ipKey;
				//HTTP应答包
				string jsonResStr = createJsonString(0,(int32_t)eErrorCode::HighFreqError,"exists ipKey" + ipKey);
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp, muduo::net::HttpResponse::k404NotFound,jsonResStr);
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);
				conn->forceClose();
				return;
			}
			else{
				REDISCLIENT.set(ipKey, ipStr);
				REDISCLIENT.resetExpiredEx(ipKey,inicfg_.ip_timeout);
			}
		}
	}

	//是否保持HTTP长连接
	bool close = (connection == "close") ||
		(req.getVersion() == muduo::net::HttpRequest::kHttp10 && connection != "Keep-Alive");
	//HTTP应答包(header/body)
	muduo::net::HttpResponse rsp(close);
	//请求处理回调，但非线程安全的 processHttpRequest
	{
		httpCallback_(req, rsp);
	}
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (conn) {
		//应答消息
		{
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
		}
		//非HTTP长连接则关闭
		if (rsp.closeConnection()) {
			conn->forceClose();
            // conn->forceCloseWithDelay(0.1);
		}
		muduo::net::HttpContext* context = boost::any_cast<muduo::net::HttpContext>(conn->getMutableContext());
		//释放资源
		context->reset();
	}
	else
	{
		LOG_ERROR << "--- conn lose error !";
	}
}

bool PayServer::startThreadPool(int threadCount)
{
   LOG_INFO << __FUNCTION__ << threadCount << " work threads.";
    m_thread_pool.setThreadInitCallback(bind(&PayServer::threadInitTcp, this));
    m_thread_pool.start(threadCount);
    return true;
}

void PayServer::threadInitTcp()
{
    //===============RedisClient=============
    if(!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
    {
        LOG_ERROR<<"RedisClient Connection ERROR!";
        return;
    }

    ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
    mongocxx::database db = MONGODBCLIENT["gamemain"];
}

void PayServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        int32_t num = m_connection_num.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->peerAddress().toIpPort() << "] -> 大厅服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
    }  
    else
    {
        int32_t num = m_connection_num.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->peerAddress().toIpPort() << "] -> 大厅服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
    }
}

void PayServer::onMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buf,muduo::Timestamp receiveTime)
{
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
        const uint16_t len = buf->peekInt16();
        if(unlikely(len > PACKET_SIZE || len < sizeof(int16_t)))
        {
           LOG_ERROR << "==== len > PACKET_SIZE Invalid length ====[" << len << "]";
            if(conn)
                conn->shutdown();  // FIXME: disable reading
            break;
        }
        else if(likely(buf->readableBytes() >= len))
        {
            BufferPtr buffer(new muduo::net::Buffer(len));
            buffer->append(buf->peek(), static_cast<size_t>(len));
            buf->retrieve(len);
            TcpConnectionWeakPtr weakPtr(conn);
            m_thread_pool.run(bind(&PayServer::processRequest, this, weakPtr, buffer, receiveTime));
        }
        else
        {
           LOG_ERROR << "==== len > PACKET_SIZE Invalid length ====[" << len << "]";
            break;
        }
    }
}

void PayServer::processRequest(TcpConnectionWeakPtr &weakConn,BufferPtr &buf,muduo::Timestamp receiveTime)
{
    LOG_INFO <<__FUNCTION__  << " receiveTime[" << time(nullptr) <<"]";

    int32_t bufSize = buf->readableBytes();
    if(bufSize < sizeof(internal_prev_header) + sizeof(Header))
    {
        LOG_ERROR << ">>> " <<  __FUNCTION__ << " BUFFER IS NULL !!!!!!!";
        return;
    }

    // 取出从网关传过来组装的头文件
    internal_prev_header *pre_header = (internal_prev_header *)buf->peek();

    string session(pre_header->session, sizeof(pre_header->session));
    int64_t userId = pre_header->userId;
    string aesKey(pre_header->aesKey, sizeof(pre_header->aesKey));

    // 取出从客户端发送上来的头文件
    Header *commandHeader = (Header *)(buf->peek() + sizeof(internal_prev_header));
    
    // 两个协议头长度
    int headLen = sizeof(internal_prev_header) + sizeof(Header);

    // 取得数据校检
    uint16_t crc = GlobalFunc::GetCheckSum((uint8_t*)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);

    // LOG_INFO <<__FUNCTION__  << " 两个协议头长度[" << headLen <<"],校检[" << crc <<"]";

	// 校验数据
	if (commandHeader->len != bufSize - sizeof(internal_prev_header) || commandHeader->crc != crc || commandHeader->ver != PROTO_VER || commandHeader->sign != HEADER_SIGN)
	{
		LOG_ERROR << " " << __FUNCTION__ << " BUFFER LEN ERROR ! header->len=" << commandHeader->len
				  << " bufferLen=" << bufSize - sizeof(internal_prev_header) << " header->crc=" << commandHeader->crc << " buffer crc=" << crc;
		return;
	}

	//本次请求开始时间戳(微秒)
	muduo::Timestamp timestart = muduo::Timestamp::now();

	LOG_INFO << __FUNCTION__ << " 主命令[" << commandHeader->mainId << "],子命令[" << commandHeader->subId << "],加密类型[" << commandHeader->encType << "]";

	switch (commandHeader->mainId)
	{
	case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
	case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL:
	case Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER:
	case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PAY_SERVER:
	{
		int mainid = commandHeader->mainId;
		int subid = commandHeader->subId;
		int id = (mainid << 8) | subid;

		switch (commandHeader->encType)
		{
		case PUBENC__PROTOBUF_AES:
		{
			if (likely(m_functor_map.count(id)))
			{
				try
				{
					//解密
					vector<uint8_t> DecryptData;
					int32_t encodeDataLen = commandHeader->len - sizeof(Header);
					// LOG_WARN << "===开始解密userId[" << userId << "],subid[" << subid << "],aesKey[" << aesKey << "],DataLen[" << encodeDataLen << "]";

					int ret = Crypto::AES_ECB_Decrypt_EX(aesKey, (unsigned char *)buf->peek() + headLen, (int)encodeDataLen, DecryptData);
					if (ret > 0)
					{
						vector<uint8_t> DecodeData;
						DecodeData.resize(sizeof(Header) + DecryptData.size());
						memcpy(&DecodeData[0], (uint8_t *)buf->peek() + sizeof(internal_prev_header), sizeof(Header));
						memcpy(&DecodeData[sizeof(Header)], (uint8_t *)&DecryptData[0], DecryptData.size());

						AccessCommandFunctor functor = m_functor_map[id];
						functor(weakConn, (uint8_t *)&DecodeData[0], DecodeData.size(), pre_header);
					}
					else
						LOG_ERROR << "===<<< AES 解密失败 >>>===userId[" << userId << "],subid[" << subid << "],aesKey[" << aesKey << "],DataLen[" << encodeDataLen << "]";
				}
				catch (exception &e)
				{
					LOG_ERROR << __FUNCTION__ << " >>> Exception:[" << e.what() << "],userId[" << userId << "],subid[" << subid << "],aesKey[" << aesKey << "]";
				}

				// 打印耗时日志
				if (subid > ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)
				{
					muduo::Timestamp timenow = muduo::Timestamp::now();
					double timdiff = muduo::timeDifference(timenow, timestart);
					if (timdiff > 0.5)
						LOG_ERROR << ">>> subid[" << subid << "],time elapse[" << timdiff << "]";
					else if (timdiff > 0.1)
						LOG_DEBUG << ">>> subid[" << subid << "],time elapse[" << timdiff << "]";
				}
			}
			else
				LOG_ERROR << "Not Define Command: mainId=" << mainid << " subId=" << subid;

			break;
		}
		case PUBENC__PROTOBUF_NONE: //AES
		{
			if (subid != 1)
				LOG_WARN << "++++++++++++++ PUBENC__PROTOBUF_NONE ++++++++++++++ mainId=" << mainid << " subId=" << subid;

			if (likely(m_functor_map.count(id)))
			{
				if (subid != 1)
					LOG_WARN << "===<<< PROTOBUF_NONE 开始处理 >>> subid[" << subid << "],aesKey[" << aesKey << "]";
				AccessCommandFunctor functor = m_functor_map[id];
				functor(weakConn, (uint8_t *)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
			}
			else
			{
				LOG_ERROR << "Not Define Command: mainId=" << mainid << " subId=" << subid;
			}
			break;
		}
		default:
			break;
		}
	}
	break;
	default:
		break;
	}
}

//解析请求 ///
bool PayServer::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg)
{
	params.clear();
	try
	{
		do {
			std::string subStr;
			std::string::size_type npos = queryStr.find_first_of('?');
			if (npos != std::string::npos) {
				//skip '?' ///
				subStr = queryStr.substr(npos + 1, std::string::npos);
			}
			else {
				subStr = queryStr;
			}
			if (subStr.empty()) {
				break;
			}
			for (;;) {
				//key value separate ///
				std::string::size_type kpos = subStr.find_first_of('=');
				if (kpos == std::string::npos) {
					break;
				}
				//next start ///
				std::string::size_type spos = subStr.find_first_of('&');
				if (spos == std::string::npos) {
					std::string key = subStr.substr(0, kpos);
					GlobalFunc::trimstr(key);
					//skip '=' ///
					std::string val = subStr.substr(kpos + 1, std::string::npos);
					params[key] = val;
					break;
				}
				else if (kpos < spos) {
					std::string key = subStr.substr(0, kpos);
					GlobalFunc::trimstr(key);
					//skip '=' ///
					std::string val = subStr.substr(kpos + 1, spos - kpos - 1);
					params[key] = val;
					//skip '&' ///
					subStr = subStr.substr(spos + 1, std::string::npos);
				}
				else {
					break;
				}
				
			}
		} while (0);


		std::string keyValues;
		for (auto param : params) {
			keyValues += "\n" + param.first + "=" + param.second;
		}

		LOG_WARN << __FUNCTION__ << " --- " << keyValues;

	}
	catch (std::exception & e)
	{
		LOG_ERROR << "---parseQuery--- " << e.what();
		errmsg = e.what();
		return false;
	}
	return true;
}

//请求字符串
std::string PayServer::getRequestStr(muduo::net::HttpRequest const& req) {
	std::stringstream ss;
	for (std::map<string, string>::const_iterator it = req.headers().begin();
		it != req.headers().end(); ++it) {
		ss << it->first << ": " << it->second <<"\n";
	}

	ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?><br/>" \
		"<xs:body xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"><br/>" \
		"<xs:method>" << req.methodString() << "</xs:method><br/>" \
		"<xs:path>" << req.path() << "</xs:path><br/>" \
		"<xs:query>" << req.query() << "</xs:query><br/>" \
		"</xs:body><br/>";

	return ss.str();
}

//按照占位符来替换 ///
static void replace(string& json, const string& placeholder, const string& value) {
	boost::replace_all<string>(json, "\"" + placeholder + "\"", value);
}

// 构造返回结果
std::string PayServer::createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr)
{
	boost::property_tree::ptree root,data;
	root.put("code", ":code");
	root.put("msg", msgstr);

	data.put("uid", ":uid");
	data.put("type", ":type");
	data.put("lgname",lgname);
	data.put("dt",":dt");
	data.put("url",urlstr);
	
	root.add_child("data", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	replace(json, ":type", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	replace(json, ":uid", std::to_string(uid));
	replace(json, ":dt", std::to_string(elapsedtimd));
	boost::replace_all<std::string>(json, "\\", "");
	json = boost::regex_replace(json, boost::regex("\n|\r"), "");

	if(errcode == 0){
		nReqOKCount_ = numReqOKCount_.incrementAndGet();
	}
	else{
		if(errcode != (int32_t)eErrorCode::ServerStopError)
			nReqErrCount_ = numReqErrCount_.incrementAndGet();
	}
	 
	return json;
}

// 构造返回结果
std::string PayServer::createJsonString(int32_t opType,int32_t errcode,string const& dataStr)
{
	boost::property_tree::ptree root;
	root.put("code", ":code");
	root.put("msg", dataStr);
	root.put("type", ":type");
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	replace(json, ":type", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	boost::replace_all<std::string>(json, "\\", "");
	json = boost::regex_replace(json, boost::regex("\n|\r"), "");

	if(errcode == 0){
		nReqOKCount_ = numReqOKCount_.incrementAndGet();
	}
	else{
		nReqErrCount_ = numReqErrCount_.incrementAndGet();
	}
	 
	return json;
}
 
// 统一请求参数之获取
cpr::Response PayServer::getUrlRequest(string & parameContent,string & reqHandle)
{
	// 签名算法
	std::string strDesOut;
	Crypto::DES_ECB_Encrypt((unsigned char *)agent_info_.DesCode.c_str(), (unsigned char *)"", parameContent, strDesOut);
	LOG_INFO << "---" << __FUNCTION__ << "---\n---des_key[" << agent_info_.DesCode << "],strDesIn[" << parameContent << "]"
			 << "\n---md5_key[" << agent_info_.MD5Code << "],strDesOut[" << strDesOut << "]";

	// 2,统一请求参数之key
	std::string strMd5, src = strDesOut + agent_info_.MD5Code;
	char md5[32 + 1] = {0};
	MD5Encode32(src.c_str(), src.length(), md5, 0);
	strMd5 = string((char *)md5);
	// LOG_INFO << "---MD5 Encode ---[" << strMd5 << "]";

	// 3,拼接参数
	auto param = cpr::Parameters{};
	param.AddParameter({"params",URL::Encode(strDesOut).c_str()});
	param.AddParameter({"key", strMd5.c_str()});

	string _requrl = agent_info_.ApiUrl;
	if (!reqHandle.empty())
	{
		 _requrl = _requrl + "/" + reqHandle;
	}

	LOG_INFO << "---req string ---[" << _requrl << "?" <<param.content << "]";  

	auto resp_feature = cpr::Get(cpr::Url{_requrl},cpr::Timeout{inicfg_.Timeout}, cpr::Header{{"AgentName", agent_info_.MerchantNum}}, param);

	LOG_INFO << "---Request elapsed[" << resp_feature.elapsed << "],status_code[" << resp_feature.status_code << "],text[" << resp_feature.text << "]";

	// 请求超时打印
	if(resp_feature.elapsed * 1000 >= inicfg_.Timeout){
		std::cout << "============================================" << std::endl;
		LOG_ERROR << "---请求超时[" << parameContent << "]";  
		std::cout << "============================================" << std::endl;
	}
	// 统一请求参数之获取
	return resp_feature; 
}

// 解析请求返回结果
bool PayServer::getJsonPtree(string &jsonStr, int32_t &code, string &msg,boost::property_tree::ptree & pt)
{
	if (jsonStr.empty())
		return false;
	try
	{
		std::stringstream ss(jsonStr);
		read_json(ss, pt);
		msg = pt.get<string>("msg");
		code = pt.get<int32_t>("code");
	}
	catch (exception excep)
	{
		LOG_ERROR << "---" << __FUNCTION__ << ",jsonStr["<< jsonStr <<"],what[" << excep.what()<<"]";
		return false;
	}
	return true;
}

// 获取代理视讯限红
bool PayServer::getProxyXianHong(int32_t aid,string & oddtype)
{
	bool ret = false;
	try
	{
		// 默认值
		oddtype = inicfg_.oddtype;
		// opts
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "oddtype" << 1 << bsoncxx::builder::stream::finalize);
		// db
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["proxy_txh_oddtype"];
		bsoncxx::document::value findValue = document{} << "agentid" << aid << finalize;
		auto find_result = coll.find_one(findValue.view(), opts);
		if ( find_result )
		{
			bsoncxx::document::view view = find_result->view();
			oddtype 		= view["oddtype"].get_utf8().value.to_string();
			ret = true;
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",获取代理限红操作失败,代理[" << aid <<"]";
	}

	LOG_INFO << "---" << __FUNCTION__ << " 获取代理视讯限红,aid[" << aid << "],oddtype[" << oddtype << "],ret[" << ret << "]";

	return ret;
}

//处理HTTP回调
void PayServer::processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
{
	rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
	rsp.setStatusMessage("OK");
	rsp.setCloseConnection(true);//注意要指定connection状态
	rsp.addHeader("Server", "DTQP");
	rsp.addHeader("Access-Control-Allow-Origin", "*");
	rsp.addHeader("Cache-Control", "no-cache");
	rsp.setContentType("application/json;charset=utf-8");

	// 服务启动没完成
	if( !isStartServer ){
		rsp.setContentType("text/html;charset=utf-8");
		std::string now = muduo::Timestamp::now().toFormattedString();
		rsp.setBody("<html><body>Service is starting!\n[" + now + "]</body></html>");
		return;
	}

	//本次请求开始时间戳(微秒)
	muduo::Timestamp timestart = muduo::Timestamp::now();
	int32_t elapsedtimd = 0;
	// 接口请求处理
	if (req.path() == "/PayHandle") 
	{
		std::string rspdata,req_forwardUrl,resp_msg,lg_name = VERSION;
		try
		{
			// 参数
			std::string billNo,backurl,req_msg = "ok";
			int32_t aid = 0,actype = 0,req_code = (int32_t)eErrorCode::NoError; 
			int64_t balance = 0,userid = 0,score = 0,newbalance = 0;
			std::string da_account = "",lgType = "y", oddtypeStr = inicfg_.oddtype,reqStr = req.query();
			cpr::Response resp;

			// LOG_INFO << "postTesttext[" << postTest() << "]";

			string newrequrl = "http://192.168.2.215:9090/LoginHandle?agentid=111228&timestamp=1608625661893&key=89c60a23f4019880baa1764a2180c95d&type=0&paraValue=aYelI1PPefcCscNdsStdHAaVa1y1RLTV7iWu8SHeKIxkTGRKp2n6%2FmEb4VSuK75P0slEkW%2FU%2Fhe3acap6ylKYxKWvwXahylwMpNVQHowEDKIkOsq%2F1SOMjCUIH3MJcoWZw8iEAn%2B35yu%2F8Lf%2Ffq5VRcbhDCwX4FuAzGtjzzptJsEtVEl8PVo4JoMAU%2BB9AmRWv0vyOHiquskWV87Tsw9vKmra%2F8fTAYBEy8VNTskiV6QyBJ2WhPDh0SAw75XQNCzv3%2BJEDHvRv2iAsdTeMO%2FhiMFURjTfulRVyvy9kZSdWws63qrV2DCs8XZbziuF6%2FsYWK%2Fpie%2BpuYxfb%2BC0Vzj1HeZCI%2BRSDVO8Gw%2FzkdiwphSHvRNJdM0Mg2kFv05Vd%2BLjnI7Le8x%2BzqP5CaGpP3h2NBHUe4NJe%2FCpxCeMeRJiHS98CkyyW7llu8%2F%2FQC0VSza";
			cpr::Response cpr_req = cpr::Get(cpr::Url(newrequrl));
			LOG_INFO << "cpr_req.text[" << cpr_req.text << "]";
 

			// 返回结果
			rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,req_code,userid,lg_name,elapsedtimd,req_msg,req_forwardUrl);

			 
		}
		catch (exception excep)
		{
			resp_msg = "transfer exception" ;
			LOG_ERROR << "---" << __FUNCTION__ << " " << excep.what() <<",req.query[" << req.query() <<"]";
			rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,(int32_t)eErrorCode::InsideErrorOrNonExcutive,0,lg_name,0,resp_msg,req_forwardUrl);
		}
		// 填充
		rsp.setBody(rspdata);
	}
	else if (req.path() == "/RepairStatus") { //维护状态
		rsp.setContentType("text/html;charset=utf-8");
		rsp.setBody("repair status:"+to_string((m_bInvaildServer)?1:0));
	} 
	else {
		int32_t num = m_reciv_num.incrementAndGet();
		rsp.setContentType("text/html;charset=utf-8"); //"text/plain;charset=utf-8");
		rsp.setBody("time:"+to_string(num) +"<html><head><title>httpServer</title></head>"
			"<body><h1>Not Found</h1>"
			"</body></html>");
		rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
	}

	// 统计毫秒
	if (inicfg_.isLog < (int32_t)eLogLvl::LVL_3)
	{
		LOG_WARN << "---处理耗时(毫秒)[" << elapsedtimd << "],MsgCount[" << nReqCount_ << "],OKCount[" << nReqOKCount_ << "],ErrCount[" << nReqErrCount_ << "]";
	}
}
  
//=====config=====
bool PayServer::LoadIniConfig()
{
    if(!boost::filesystem::exists("./conf/pay.conf")){
        LOG_ERROR << "./conf/pay.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/pay.conf", pt);

    IniConfig inicfg;
    inicfg.integralvalue 	= pt.get<int>("PayServer.Integralvalue", 6);// 送积分数
    inicfg.changeRatevalue 	= pt.get<int>("PayServer.ChangeRatevalue", 500);
    inicfg.session_timeout 	= pt.get<int>("PayServer.SessionTimeout", 1000);
    inicfg.descode 			= pt.get<std::string>("PayServer.Descode","111362EE140F157D");
    inicfg.apiadd 			= pt.get<std::string>("PayServer.ApiAdd", "");
    inicfg.isDemo 			= pt.get<int>("PayServer.IsDemo", 0);
    inicfg.TestLock 		= pt.get<int>("PayServer.TestLock", 0);
    inicfg.isWhiteIp 		= pt.get<int>("PayServer.IsWhiteIp", 1);
    inicfg.defaultScore 	= pt.get<int64_t>("PayServer.DefaultScore", 10000);
    inicfg.ip_timeout 		= pt.get<int>("PayServer.IpTimeout", 200);
    inicfg.paramCount 		= pt.get<int>("PayServer.ParamCount", 5);
    inicfg.isdecrypt 		= pt.get<int>("PayServer.Isdecrypt", 0);
    inicfg.isLog 			= pt.get<int>("PayServer.IsLog", 0);
    inicfg.isLocal 			= pt.get<int>("PayServer.IsLocal", 0);
    inicfg.agentId 			= pt.get<int>("PayServer.AgentId", 10000);
    inicfg.maxConnCount 	= pt.get<int>("PayServer.MaxConnCount", 2000);
    inicfg.maxConnFreqCount = pt.get<int>("PayServer.MaxConnFreqCount",500);
    inicfg.maxGameid 		= pt.get<int>("PayServer.MaxGameid", 10000);
    inicfg.maxLoginTypeid 	= pt.get<int>("PayServer.MaxLoginTypeid", 4);
    inicfg.expireDate 		= pt.get<int>("PayServer.ExpireDate", 4*60*60); //4小时过期时间 

    inicfg.AesKey 			= pt.get<std::string>("PayServer.AesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.des_key 			= pt.get<std::string>("PayServer.Deskey", "GSwydAem");
    inicfg.md5_key 			= pt.get<std::string>("PayServer.Md5key", "4q8JXdwT");
    inicfg.reqUrl 			= pt.get<std::string>("PayServer.ReqUrl", "https://api.txhbet.net");
    inicfg.resAddUrl 		= pt.get<std::string>("PayServer.ResAdd", "doBusiness.do");
    inicfg.enterGameHandle 	= pt.get<std::string>("PayServer.EnterGameHandle", "forwardGame.do");
    inicfg.AgentName 		= pt.get<std::string>("PayServer.AgentName", "txwl");
    inicfg.Timeout 			= pt.get<int>("PayServer.Timeout", 3000);
    inicfg.MethodName 		= pt.get<std::string>("PayServer.Method", "lg");
    inicfg.Password 		= pt.get<std::string>("PayServer.Password", "666888");
    inicfg.MoneyType 		= pt.get<std::string>("PayServer.MoneyType", "CNY");
    inicfg.interval 		= pt.get<int>("PayServer.interval", 10);		//两次查询间隔
    inicfg.IsMaster 		= pt.get<int>("PayServer.IsMaster", 0);		//两次查询间隔
    inicfg.RecycleCycle 	= pt.get<int>("PayServer.RecycleCycle", 60);	//两次查询间隔
    inicfg.Actype 			= pt.get<int>("PayServer.Actype", 1);			//真钱帐号
    inicfg.ReadConfCycle 	= pt.get<int>("PayServer.ReadConfCycle", 10);	//重加载配置时间(秒)
    inicfg.IsPrintConfLog 	= pt.get<int>("PayServer.IsPrintConfLog", 1);	//重加载配置时间(秒)
    inicfg.AutoRecycle 		= pt.get<int>("PayServer.AutoRecycle", 1);		//自动回收

    inicfg.botUrl 			= pt.get<std::string>("PayServer.botUrl", "https://api.telegram.org/bot");
    inicfg.botToken 		= pt.get<std::string>("PayServer.botToken", "1288332211:AAEpsGpE6XFNZTzNF4Ca2emhLLlvFMM7OSw");
    inicfg.botChairId 		= pt.get<int64_t>("PayServer.botChairId", -437735957);
    inicfg.IsSendTgMsg 		= pt.get<int64_t>("PayServer.IsSendTgMsg", 0);
    inicfg.NotOpen 			= pt.get<int64_t>("PayServer.NotOpen", 0);
    inicfg.DemoPlayUrl 		= pt.get<std::string>("PayServer.DemoPlayUrl", "http://tryplay.txh-demo.com/#/p/preload");//试玩环境地址
    inicfg.oddtype 			= pt.get<std::string>("PayServer.oddtype", "A");
    inicfg.isOnService  	= pt.get<int>("PayServer.On",0);

	inicfg.WhiteUID.clear();
	vector<string> uidVec;
	// string temp = pt.get<std::string>("PayServer.WhiteUID", "");
	// if (!temp.empty()){
	// 	boost::algorithm::split(uidVec, temp, boost::is_any_of(","));
	// 	if(uidVec.size() > 0){
	// 		int idxCount = 0;
	// 		for(string uid : uidVec)
    // 		{
	// 			inicfg.WhiteUID.push_back(atol(uid.c_str()));
	// 			LOG_INFO << "---WhiteUID[" << uid << "] --- " << ++idxCount;
	// 		}
	// 	}
	// }

	inicfg_ = inicfg;
	if (inicfg.IsPrintConfLog == 1)
	{
		// LOG_WARN << "---resAddUrl " << inicfg.resAddUrl << " AesKey:" << inicfg.AesKey << " md5_key " << inicfg.md5_key <<" AutoRecycle " << inicfg.AutoRecycle;
		// LOG_WARN << "---IsMaster " << inicfg.IsMaster << " AgentName:" << inicfg.AgentName << " " << inicfg.enterGameHandle << " isdecrypt " << inicfg.resAddUrl;
		LOG_INFO << "---固定回收周期[" << inicfg_.RecycleCycle << "],重加载配置时间[" << inicfg_.ReadConfCycle <<"]";
		LOG_WARN << "---是否维护[" << inicfg.isOnService <<"],TestLock[" << inicfg.TestLock  << "],WhiteUID[" << inicfg_.WhiteUID.size() << "]";
	}
	
	return true;
}

 // 判断该代理是否允许开放
// 返回true为允许开放
bool PayServer::isPermitPay(int32_t _agentId)
{
    MutexLockGuard lock(agent_ids_mutex_);
    // 遍历判断
    for (int32_t agentId : agent_ids)
    {
        if (agentId == _agentId)
        {
            LOG_WARN << "agentId :" << _agentId <<" 不允许开放！";
            return false;
        }
    }

    return true;
}
 
// 加载Json文件
bool PayServer::loadJson()
{
	if(!boost::filesystem::exists("./conf/code_info.json")){
        LOG_ERROR << "./conf/code_info.json not exists";
        return false;
    }

	LOG_WARN << "---加载code_info文件--- ";

	boost::property_tree::ptree root;//创建一个结构对象    
    boost::property_tree::ptree items;
    boost::property_tree::read_json<boost::property_tree::ptree>("./conf/code_info.json", root);//读文件存放到root中

	items = root.get_child("msgCode");
    for (boost::property_tree::ptree::iterator it = items.begin(); it != items.end(); ++it)
    {
		 //遍历读出数据  
        string key = it->first;
        string text = it->second.get<string>("text");
		// LOG_INFO << key << " " << text;
		// 存储错误码
		if(!key.empty()) 
			errcode_info_[atoi(key.c_str())] = text;
    }

	// 
	for (auto it = errcode_info_.begin(); it != errcode_info_.end(); ++it) { 
		LOG_INFO << "---key[" << it->first << "],text["<< it->second << "]";
	}
	
	return true;
}

// 签名算法
cpr::Parameters PayServer::getSign(string & app_id,string & uid,string & user_recharge_amount,string & platform_product_ids,string & app_key)
{
	string return_type = "1"; 						//返回数据结构类型 固定值 1
	string device = "3";  							//设备标识 Int 类型 1. IOS 2.Android 3.其他
	string timestamp = to_string(time(nullptr));

	// 1,拼接参数
	auto param = cpr::Parameters{};
	//  parameters.AddParameter({"hello", "world"}, holder);
	 
	param.AddParameter({"app_id", app_id.c_str()});
	param.AddParameter({"device", device.c_str()});
	param.AddParameter({"out_uid", uid.c_str()});
	param.AddParameter({"platform_product_ids", platform_product_ids.c_str()});
	param.AddParameter({"return_type", return_type.c_str()});
	param.AddParameter({"timestamp", timestamp.c_str()});
	param.AddParameter({"user_recharge_amount", user_recharge_amount.c_str()});

	string keyParam = param.content;

	LOG_INFO << "---"<<__FUNCTION__<<",uid[" << uid << "],keyParam[" << param.content << "]";

	// 2,统一请求参数之key
	std::string strMd5, signStr = param.content + app_key;

	LOG_INFO << "---"<<__FUNCTION__<<",uid[" << uid << "],signStr[" << signStr << "]";

	char md5[32 + 1] = {0};
	MD5Encode32(signStr.c_str(), signStr.length(), md5, 0);
	strMd5 = string((char *)md5);

	param.AddParameter({"sign", strMd5.c_str()});

	LOG_INFO << "---"<<__FUNCTION__<<",uid[" << uid << "],strMd5["<< strMd5 << "],param.content[" << param.content << "]";
	return param;
}

// 签名算法
cpr::Payload PayServer::getSignPost(string uid,OnlinePayConfig & onlinecfg)
{
	string timestamp = to_string(time(nullptr));

	// 1,拼接参数
	auto param = cpr::Payload{};
	param.AddPair({"app_id", onlinecfg.app_id.c_str()});
	param.AddPair({"device", onlinecfg.device.c_str()});//设备标识 Int 类型 1. IOS 2.Android 3.其他
	param.AddPair({"out_uid", uid.c_str()});
	param.AddPair({"platform_product_ids", onlinecfg.platform_product_ids.c_str()});
	param.AddPair({"return_type", onlinecfg.return_type.c_str()});
	param.AddPair({"timestamp", timestamp.c_str()});
	param.AddPair({"user_recharge_amount", onlinecfg.user_recharge_amount.c_str()});

	string keyParam = param.content;

	LOG_INFO << "---"<<__FUNCTION__<<",uid[" << uid << "],param.content[" << param.content << "]";

	// 2,统一请求参数之key
	std::string strMd5, signStr = param.content + onlinecfg.app_key;

	char md5[32 + 1] = {0};
	MD5Encode32(signStr.c_str(), signStr.length(), md5, 0);
	strMd5 = string((char *)md5);
	// 添加签名
	param.AddPair({"sign", strMd5.c_str()});

	LOG_INFO << "---"<<__FUNCTION__<<",uid[" << uid << "],signStr[" << signStr << "],strMd5["<< strMd5 << "],param.content[" << param.content << "]";
	return param;
}

// 获取连接
// @useridStr用户ID
int32_t PayServer::queryZxWebUrl(string & web_url, int32_t & status, string requrl,string & errmsg, cpr::Payload param)
{	
	std::cout << "=======================" << std::endl;
	std::cout << "=======获取尊享支付接口========" << std::endl;
	std::cout << "=======================" << std::endl;
 
    muduo::Timestamp timestart = muduo::Timestamp::now();
	int64_t curTime = time(nullptr);
	string _requrl = requrl;
	LOG_INFO << "---req string ---[" << _requrl << "?" << param.content <<"]";

	// 查询余额回调
	auto callback = [&](cpr::Response response) 
	{
		bool ret = false;
		int32_t req_code = 0,status = 0,error_level = 0,platform_product_info_status = 0;
		int64_t microtime = 0;
		string extend,msg;
		stringstream ss;
		do
		{
			// 查询成功
			if (200 != response.status_code){
				LOG_ERROR << "查询返回,elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";
				break;
			}

			string errmsg;
			boost::property_tree::ptree req_pt;

			// std::cout << "\r\n\r\n---查询接口返回[" << response.text << "]\r\n\r\n\r\n";

			if(response.text.empty()){
				req_code = (int32_t)eErrorCode::GetJsonInfoError;
				LOG_ERROR << "查询接口返回数据为空 elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";
				break;
			}

			// 取结果
			try
			{
				std::stringstream ss(response.text);
				read_json(ss, req_pt);
				errmsg = req_pt.get<string>("msg");
				extend = req_pt.get<string>("extend");
				status = req_pt.get<int32_t>("status");
				microtime = req_pt.get<int64_t>("microtime");
				error_level = req_pt.get<int32_t>("error_level");
				// 平台维护状态
				platform_product_info_status = req_pt.get<int32_t>("data.enjoy_pay.platform_product_info.status");
				web_url = req_pt.get<string>("data.enjoy_pay.extra.url_info.web_url");

				// 1开启 2关闭 3维护
				status = platform_product_info_status;
				// 玩家在线状态 
				LOG_INFO << "---查询结果,product_info[" << platform_product_info_status << "],web_url[" << web_url << "],errmsg[" << errmsg << "],status_code[" << response.status_code << "]";
			}
			catch (exception excep)
			{
				req_code = (int32_t)eErrorCode::GetJsonInfoError;
				std::cout << "\r\n\r\n---查询返回[" << response.text << "]\r\n\r\n\r\n";
				LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";
				string strtmp = inicfg_.isDemo == 0 ? "线上" : "试玩";
				GlobalFunc::sendMessage2TgEx(strtmp + "\n" + to_string(response.status_code) + "\n" + response.text, inicfg_.botToken, inicfg_.botChairId);
				return response.status_code;
			}

			// 1开启 2关闭 3维护
			if (platform_product_info_status > eZXUpholdType::op_zx_start)
				web_url = ""; //前端　根据 此值为空来区别不显示左边菜单

			LOG_WARN << "---拉取尊享支付结果[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";
  		  
			// 执行完成
			ret = true;
		} while (0);

		//错误则发送消息
		if (!ret)
		{
			ss << "拉取尊享支付失败,状态码[" << response.status_code << "],耗时[" << response.elapsed << "],返回内容[" << response.text << "]";
			LOG_ERROR << "---" << ss.str();
			string strtmp = inicfg_.isDemo == 0 ? "线上" : "试玩";
			GlobalFunc::sendMessage2TgEx(strtmp + "\n" + ss.str(), inicfg_.botToken, inicfg_.botChairId);
		}

		// 返回状态
		return response.status_code;
	};

	// 发送查询请求 
	std::string contentType = "application/x-www-form-urlencoded;charset=utf-8";
	auto future = cpr::PostCallback(callback, cpr::Url{_requrl},cpr::Header{{"content-type",contentType.c_str()}}, cpr::Timeout{10000}, param);
	if (future.get() != 200)
		LOG_ERROR << "---回收查询请求失败--,status_code[" << future.get() << "]";

	// 拉取余额耗时统计
	auto elaspTime =  muduo::timeDifference(muduo::Timestamp::now(), timestart) ;
	stringstream ss;
	ss << "耗时[" << elaspTime << "]\n";
	

	// 固定周期回收时才发送
	if(inicfg_.IsSendTgMsg == 1 ){
		string strtmp = inicfg_.isDemo == 0 ? "线上" : "试玩";
		GlobalFunc::sendMessage2TgEx(strtmp + "\n" + ss.str(), inicfg_.botToken, inicfg_.botChairId);
	}


	LOG_INFO << "---拉取支付数据," << ss.str();
	std::cout << "=======================" << std::endl;
	std::cout << "=======拉取结束========" << std::endl;
	std::cout << "=======================" << std::endl;
}
 
// 缓存在线支付信息
int32_t PayServer::cacheOnlinePayInfo(int32_t agentid,string paymentconfig,OnlinePayConfig & onlinecfg_)
{
	try
	{
		auto oline_itor = m_OnlinePayCfgMap.find(agentid);
		if (oline_itor != m_OnlinePayCfgMap.end())
		{
			onlinecfg_ = oline_itor->second;
			return 0;
		}
 
		if(paymentconfig.empty()){
			LOG_ERROR << "---" << __FUNCTION__ << " 没配置好在线充值字段[" << agentid << "]";
			return 3;
		}
		
		LOG_DEBUG << "---" << __FUNCTION__ << " 配置字段[" << paymentconfig << "]";

		OnlinePayConfig onlinecfg;
		// 取结果
		boost::property_tree::ptree req_pt;
		std::stringstream ss(paymentconfig);
		read_json(ss, req_pt);
		onlinecfg.requrl = req_pt.get<string>("initUrl");
		onlinecfg.app_id = req_pt.get<string>("appId");
		onlinecfg.platform_product_ids = req_pt.get<string>("products");
		onlinecfg.app_key = req_pt.get<string>("secretkey");
		onlinecfg.return_type = req_pt.get<string>("return_type");
		onlinecfg.user_recharge_amount = "0";
		onlinecfg.device = "3";
		onlinecfg.notifyurl = req_pt.get<string>("notifyurl");
		onlinecfg.prepayurl = req_pt.get<string>("prepayurl");
		onlinecfg.prenotifyurl = req_pt.get<string>("prenotifyurl");
		onlinecfg.notifyIp = req_pt.get<string>("notifyIp");

		m_OnlinePayCfgMap[agentid] = onlinecfg;
		onlinecfg_ = onlinecfg;

		return 0;
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 加载在线充值配置失败----" << e.what();
		return 5;
	}
}

// 获取充值信息(4重 for 循环 要优化下)
void PayServer::cmd_get_recharge_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header)
{ 
    internal_header->nOK = 0;
    Header *commandHeader = (Header*)msgdata;
    commandHeader->subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_RECHARGE_INFO_RES;

	muduo::Timestamp timestart = muduo::Timestamp::now();
    // request
    ::HallServer::GetRechargeInfoMessage msg;
    if(likely(msg.ParseFromArray(msgdata + sizeof(Header), commandHeader->realSize)))
    {
        ::Game::Common::Header header = msg.header();
        // response
        ::HallServer::GetRechargeInfoResponse response;
        ::Game::Common::Header *resp_header = response.mutable_header();
        resp_header->CopyFrom(header);

        int64_t userId = internal_header->userId;
        int32_t retCode         = 0;                // 结果码
        int32_t agentid         = msg.agentid();    // 代理ID
		int32_t paytype = 1;						// 1 为移动端 0　为PC端
		int32_t level = 1;							//会员层级
        int32_t webRetCode         = 0;                // 结果码
		string zxWebUrl,errmsg;

		LOG_INFO << "---" << __FUNCTION__ << " agentid[" << agentid << "],userId[" << userId << "],维护状态[" << m_bInvaildServer << "]";
		// 
		std::map<string, ChannleBonusItem> memberbonusMap;
		retCode = loadRechargeInfo(userId,agentid,paytype,level,memberbonusMap);
 
		LOG_INFO << "---" << __FUNCTION__ << "retCode[" << retCode << "], level[" << level << "],memberbonusMap[" << memberbonusMap.size() << "],耗时[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) <<"]";
		 
		// 4.0 从缓存中寻找信息 std::map<int32_t,vector<PAY_MENT_MAP>>  m_ChannleAgentMap
		auto channle_agent_itor = m_ChannleAgentMap.find(agentid << 8 | level);
		if (channle_agent_itor != m_ChannleAgentMap.end())
		{
			LOG_WARN << "---" << __FUNCTION__ << " 充值渠道信息找到缓存[" << agentid << "],渠道数量[" << channle_agent_itor->second.size() << "]";

			//4.1 通过代理信息拿到对应的支付配置映射集 second : vector<PAY_MENT_MAP>
			for (auto &channleinfo : channle_agent_itor->second)
			{
				//4.2 遍历支付配置映射集(wy,ysf,kf等)  读缓存 PAY_MENT_MAP : map<string,vector<PaymentItem>>
				for (auto iter = channleinfo.begin(); iter != channleinfo.end(); iter++)
				{
					// 4.3 按各支付方式打包
					::HallServer::ChannleItemInfo *channleItem = response.add_channleitem();
					channleItem->set_paymentchannel(iter->first);

					// 4.3.1 各个渠道的优惠值（此处必须保证会员层级中所有渠道优惠信息都有） //&& memberbonusMap[iter->first].bonusprop
					if (memberbonusMap.size() > 0)
						channleItem->set_bonusprop((int32_t)GlobalFunc::rate100(to_string(memberbonusMap[iter->first].bonusprop)));

					LOG_INFO << "-----渠道[" << iter->first << "],支付方式数量[" << iter->second.size() << "],优惠[" << channleItem->bonusprop() << "]";

					// 4.4 从各支付方式中取出对应配置数据
					for (auto &pay : iter->second)
					{
						// 4.4.1 取出支付方式结构体信息
						::HallServer::PaymentItem *payment = channleItem->add_payitem();
						payment->set_paymentchannel(pay.paymentchannel); //渠道支付方式名称
						payment->set_channelname(pay.channelname);		 //渠道名称
						payment->set_channel(pay.channel);				 //渠道名称
						payment->set_paymentcode(pay.paymentcode);		 //支付方式代码

						// 4.4.2 填充支付方式结构体
						if (pay.paymentchannel.compare(PAY_CODE_ZX) != 0){
							payment->set_bankjson(pay.bankjson); //银行JSON信息
							payment->set_bankurl(pay.bankUrl);	 //
						}

						// 最大最小支付值
						payment->set_minquota(pay.minquota); //
						payment->set_maxquota(pay.maxquota); //

						// 不同支付方式区别填充
						if (pay.paymentchannel.compare(PAY_CODE_WY) == 0)
						{
							// 4.4.2.1 获取网银支付
							payment->set_bankaccount(pay.bankaccount); //
							payment->set_bankaddress(pay.bankaddress); //
							payment->set_bankcode(pay.bankcode);	   //
							payment->set_banknumber(pay.banknumber);   //
							payment->set_bankname(pay.bankname);	   //
							payment->set_bankiconherf(pay.bankiconherf);	   //
						}
						else if (pay.paymentchannel.compare(PAY_CODE_ZX) == 0)
						{
							int32_t status = 0,code_var = 0;
							OnlinePayConfig onlinecfg;
							code_var = cacheOnlinePayInfo(agentid, pay.bankjson, onlinecfg);

							// 4.4.2.2 获取尊享支付
							if( eRetCode::YES == code_var && !onlinecfg.requrl.empty())
							{
								// 获取尊享支付客服连接
								webRetCode = queryZxWebUrl(zxWebUrl, status,onlinecfg.requrl, errmsg, getSignPost(to_string(userId), onlinecfg));

								// 替换Http 为https
								std::string::size_type npos = zxWebUrl.find_first_of("http:");
								if (npos != std::string::npos)
								{
									boost::replace_all<std::string>(zxWebUrl, "http:", "https:");
									// json = boost::regex_replace(json, boost::regex("\n|\r"), "");
								}
							}
							
							// 4.4.2.2.1 设置尊享客服地址
							payment->set_bankurl(zxWebUrl);	 //
							LOG_DEBUG << "---设置尊享,webRetCode["<< webRetCode <<"],尊享状态["<< status << "],web url[" << zxWebUrl << "],在线配置码[" << code_var << "]";
						} 

						// 填充押分筹码值
						for (auto solid : pay.solidQuota)
							payment->add_solidquota(solid);

						LOG_INFO << "[" << pay.paymentchannel << "] paymentcode[" << pay.paymentcode << "] channelname[" << pay.channelname << "] channel[" << pay.channel << "],耗时[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) <<"]";
					}
				}
			}
		}

		response.set_retcode(retCode); //结果码
		size_t len = response.ByteSize();
		vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
		if (response.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
		{
			sendData(weakConn, data, internal_header, commandHeader);
		}
	}
}

// 缓存会员层级信息
bool PayServer::cacheMemberBonusInfo(int32_t level,int32_t agentid,int32_t mapIdx,string & channelbonusJson)
{
    try
    {
        if (m_BonusItemMap.find(mapIdx) != m_BonusItemMap.end())
        {
            return true;
        }

        // 从缓存中寻找会员层级优惠信息
        LOG_INFO << "---" << __FUNCTION__ << " 获取层级会员优惠值原始JSON[" << channelbonusJson << "]";

        // 2.2 优惠数组
        string strWithoutJsonArray;
        // 解析优惠json字符串
        if (readWithoutNameJsonArrayString(channelbonusJson, strWithoutJsonArray))
        {
            //同一层级存放各个支付渠道优惠的map
            std::map<string, ChannleBonusItem> channlebonusMap;

            boost::property_tree::ptree req_pt;
            std::stringstream sstream(strWithoutJsonArray);
            boost::property_tree::json_parser::read_json(sstream, req_pt);
            BOOST_FOREACH (boost::property_tree::ptree::value_type &v, req_pt)
            {
                ChannleBonusItem bonusItem;
                boost::property_tree::ptree p = v.second;

                bonusItem.channelname = p.get<std::string>("channelname");
                bonusItem.channel = p.get<std::string>("channel");
                bonusItem.bonusprop = p.get<double>("bonusprop");
                bonusItem.washtotal = p.get<double>("washtotal");
                bonusItem.integral = p.get<double>("integral");
                bonusItem.bonuslimit = p.get<int32_t>("bonuslimit");

                channlebonusMap[bonusItem.channel] = bonusItem;

                LOG_INFO << "---层级[" << level << "],渠道名称[" << bonusItem.channelname << "],渠道类型[" << bonusItem.channel << "],优惠百分比[" << bonusItem.bonusprop << "],数量[" << channlebonusMap.size() << "]";
            }
            
            // 缓存
            m_BonusItemMap[mapIdx] = channlebonusMap;
            LOG_INFO << "---代理[" << agentid << "],层级[" << level << "],缓存数量[" << m_BonusItemMap.size() << "]";
        }
        
    }
    catch (boost::property_tree::ptree_bad_path &e)
    {
        LOG_ERROR << "---" << __FUNCTION__ << " 获取层级会员优惠值JSON解析出错,what[" << e.what() <<"]";
        return false;
    }

    return true;
}

bool PayServer::get_platform_global_config(int32_t agent_id,int32_t type_id,string & details)
{
	try
	{
		//1, 从用户表中查询出会员层级
		mongocxx::collection global_config_coll = MONGODBCLIENT["gamemain"]["platform_global_config"];
		auto find_result = global_config_coll.find_one(document{} << "agentid" << agent_id << "typeid" << type_id << finalize);
		if (!find_result)
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agent_id << "]";
			return false;
		}

		// 1.1 取出结果
		bsoncxx::document::view view = find_result->view();
		int32_t status = view["status"].get_int32();
		details = view["details"].get_utf8().value.to_string();
		if (status != eDefaultStatus::op_ON || details.empty())
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 代理没开放此功能[" << agent_id << "]";
			return false;
		}
	}
	catch(const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 玩家创建邮件失败----" << e.what();
		return false;
	}

	return true;
}

// 缓存银行ICON信息
bool PayServer::loadbanklist(int32_t agentid ,string codeIcon,string & herf)
{
	LOG_ERROR << "---银行ICON[" << __FUNCTION__ << "],agentid[" << agentid << "],codeIcon[" << codeIcon << "]";
	// 找到缓存信息
	if (m_bankIconMap.find(agentid) == m_bankIconMap.end())
	{
		try
		{
			vector<map<string, string>> bankIconinfo;
			mongocxx::collection bankdeploy_coll = MONGODBCLIENT["gamemain"]["bankdeploy"];
			mongocxx::cursor info = bankdeploy_coll.find({}); //document{} << "agentid" << agentid << finalize
			for (auto &doc : info)
			{ 
				string pictureurl,bankcode;
				if (doc["bankcode"] && doc["bankcode"].type() != bsoncxx::type::k_null){
					bankcode = doc["bankcode"].get_utf8().value.to_string();
				}
				if (doc["pictureurl"] && doc["pictureurl"].type() != bsoncxx::type::k_null){
					pictureurl = doc["pictureurl"].get_utf8().value.to_string();
				}

				//空判处理
				if (bankcode.empty())
					continue;

				map<string, string> bankitemTmp;
				bankitemTmp[bankcode] = pictureurl;
				bankIconinfo.push_back(bankitemTmp);

				LOG_INFO << "---pictureurl[" << pictureurl << "],bankcode[" << bankcode << "],size[" << bankIconinfo.size() << "]";
			}
			// 缓存到内存
			m_bankIconMap[agentid] = bankIconinfo;
		}
		catch (exception &e)
		{
			LOG_ERROR << " >>> Exception: " << e.what();
			return false;
		}
	}
 
	auto it = m_bankIconMap.find(agentid);
	if (it != m_bankIconMap.end())
	{
		for (auto bankIconinfo : it->second)
		{
			if(codeIcon.compare(bankIconinfo.begin()->first) == 0){
			   herf = bankIconinfo.begin()->second;
			   break;
			}
			// LOG_DEBUG << " >>> 0 bankIconinfo: " << bankIconinfo.begin()->first << ", " << bankIconinfo.begin()->second;
		}
	}

	return true;
}

// 加载PicPath值配置,图片连接
int32_t PayServer::cachePicPathIP(int32_t agentid,string & picPath)
{
	try
	{
		auto itor = m_PicthPathMap.find(agentid);
		if (itor != m_PicthPathMap.end())
		{
			picPath = itor->second;
			return 0;
		}

		string details;
		if (get_platform_global_config(agentid, (int32_t)eAgentGlobalCfgType::op_cfg_file, details))
		{
			boost::property_tree::ptree pt;
			std::stringstream ss(details);
			read_json(ss, pt);
			picPath = pt.get<string>("fullpath");
			LOG_INFO << "---" << __FUNCTION__ << "---图片连接,agentid[" << agentid << "],picPath[" << picPath << "]";
		}
		else
			LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agentid << "]";

		m_PicthPathMap[agentid] = picPath;

		return 0;
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 加载PicPath值配置失败----" << e.what();
		return 5;
	}
}

void PayServer::cacheChannleWY(int32_t agentid,int32_t level, vector<string> & bankcardidVec)
{
     auto _array = bsoncxx::builder::basic::array{}; //make_array("A", "D")
     for (auto channleName : bankcardidVec)
     {
         _array.append(channleName);
     } 

	string picPath;
	if(cachePicPathIP(agentid,picPath) != eRetCode::YES)
		LOG_ERROR << "---" << __FUNCTION__ << " 找不到picPath信息[" << agentid << "]";

	 // 渠道信息
    std::map<string, std::vector<PaymentItem>> channleItemMap;

	//  objid银行信息
    mongocxx::collection pay_coll = MONGODBCLIENT["gamemain"]["platform_bank"];
    mongocxx::cursor cursor = pay_coll.find(make_document(kvp("agentid", agentid), kvp("objid", make_document(kvp("$in", _array)))));
    for (auto &doc : cursor)
    {
        int32_t status =  doc["status"].get_int32();
        if (status == 0)
        {
            string agentname = doc["agentname"].get_utf8().value.to_string();     //"澳门银河",
            // 银行信息
            PaymentItem paymentInfo;
            paymentInfo.paymentchannel = PAY_CODE_WY;
            paymentInfo.bankname = doc["bankname"].get_utf8().value.to_string();
            paymentInfo.banknumber = doc["banknumber"].get_utf8().value.to_string();
            paymentInfo.bankaccount = doc["bankaccount"].get_utf8().value.to_string();
            paymentInfo.bankaddress = doc["bankaddress"].get_utf8().value.to_string();
            paymentInfo.bankcode = doc["bankcode"].get_utf8().value.to_string();
            paymentInfo.minquota = doc["minquota"].get_int32();
            paymentInfo.maxquota = doc["maxquota"].get_int32();

            LOG_INFO << "---层级[" << level << "],代理[" << agentid << "],支付商[" << agentname << "],银行名[" << paymentInfo.bankname << "],银行ADD[" << paymentInfo.bankaddress << "],代码[" << paymentInfo.bankcode << "]";

            if (doc["bankQuota"])
            {
                auto t_solidQuota = doc["bankQuota"].get_array();

                for (auto &solidQuotaItem : t_solidQuota.value)
                {
                    paymentInfo.solidQuota.push_back(solidQuotaItem.get_int64());
                }
            }
            else
                LOG_ERROR << "---" << __FUNCTION__ << ",代理["<< agentid <<"],银行卡转帐没有配置便捷输入数值!";

			// 获取银行ICON 连接地址
	 		loadbanklist(agentid,paymentInfo.bankcode, paymentInfo.bankiconherf);
			paymentInfo.bankiconherf = picPath + paymentInfo.bankiconherf;

            LOG_INFO << "---" << __FUNCTION__ << ",bankcode["<< paymentInfo.bankcode <<"],bankiconherf["<< paymentInfo.bankiconherf <<"]!";

            // 添加到列表
            channleItemMap[paymentInfo.paymentchannel].push_back(paymentInfo);
            LOG_INFO << "---" << __FUNCTION__ << ",代理[" << agentid << "],渠道信息[" << paymentInfo.paymentchannel << "],渠道数量[" << channleItemMap[paymentInfo.paymentchannel].size() << "]";
        }
    }

    // 分代理存缓存
    m_ChannleAgentMap[(agentid << 8 | level)].push_back(channleItemMap);
    LOG_ERROR << "---" << __FUNCTION__ << ",代理["<< agentid <<"],Channle Agent Map,size[" << m_ChannleAgentMap[(agentid << 8 | level)].size() << "]";
}

// 缓存在线支付渠道信息
void PayServer::cacheChannleOnline(int32_t agentid,int32_t level, int32_t paytype, vector<string> & onlineidVec,string paymentchannel)
{
    // 根据支付渠道去取支持的支付方式
    auto _array = bsoncxx::builder::basic::array{}; //make_array("A", "D")
    for (auto onlineid : onlineidVec)
    {
        _array.append(onlineid);
    } 

    LOG_ERROR << "---" << __FUNCTION__ << ",代理["<< agentid <<"],paytype["<< paytype <<"],level["<< level <<"],paymentchannel["<< paymentchannel <<"]";

	// 渠道信息
    std::map<string, std::vector<PaymentItem>> channleItemMap;
    
    // paytype 1 为移动端 0　为PC端 bonusprop
    mongocxx::collection pay_coll = MONGODBCLIENT["gamemain"]["onlinepaydeploy"];
    mongocxx::cursor cursor = pay_coll.find(make_document(kvp("agentid", agentid), kvp("objid", make_document(kvp("$in", _array)))));
    for (auto &doc : cursor)
    {
        int32_t status = doc["status"].get_int32();
        if(status != eDefaultStatus::op_ON) continue;

        string  mbchannel = doc["mbchannel"].get_utf8().value.to_string();
        string  paychannel = doc["paychannel"].get_utf8().value.to_string();

        string agentname = doc["agentname"].get_utf8().value.to_string();
        string paymentcode = doc["paymentcode"].get_utf8().value.to_string();   //"MAFG",
        string paymentname = doc["paymentname"].get_utf8().value.to_string();   //"蚂蜂支付",
        string paymenturl = doc["paymenturl"].get_utf8().value.to_string();     //"pay.dt1888.com",
        string paymentconfig = doc["paymentconfig"].get_utf8().value.to_string();


        bool isfind = false;
        vector<string> payDicVec;
        string paychannelStr = (paytype == eClinetStatus::op_MB) ? mbchannel : paychannel;
        boost::algorithm::split(payDicVec, paychannelStr, boost::is_any_of(","));
        for (auto paycode : payDicVec) //22,23,24,27
        {
            int32_t nPayCode = std::stoi(paycode) % 10;

            // 尊享
            if(paymentchannel.compare(PAY_CODE_ZX) == 0)
            { 
                if(nPayCode == 1) // 此条记录为尊享所有
                { 
    				// LOG_DEBUG << "---" << __FUNCTION__ << ",代理["<< agentid <<"],paytype["<< paytype <<"],paycode["<< paycode <<"],nPayCode["<< nPayCode <<"],paymentconfig[" << paymentconfig <<"]";
                    isfind = true;
                    break;
                }
            }
            //VIP
            else if( paymentchannel.compare(PAY_CODE_VIP) == 0)
            {
                if (nPayCode == 2) // 此条记录为VIP所有
                {
                    isfind = true;
                    break;
                }
            }
        }

        if( !isfind )  continue;

        // 尊享
        PaymentItem paymentInfo;
        paymentInfo.paymentchannel = paymentchannel;
        paymentInfo.channelname = paymentname;
        paymentInfo.paymentcode = paymentcode;
        // 银行信息
        paymentInfo.bankjson = paymentconfig;
        paymentInfo.bankUrl = paymenturl;
        
		// for (auto &solidQuotaItem : t_solidQuota.value)
        //     paymentInfo.solidQuota.push_back(solidQuotaItem.get_int32());

		// 添加到列表
        channleItemMap[paymentchannel].push_back(paymentInfo);
        LOG_INFO << "---" << __FUNCTION__ << ",代理["<< agentid <<"],渠道信息[" << paymentchannel << "],size[" << channleItemMap.size() << "]";

		if (paymentchannel.compare(PAY_CODE_ZX) == 0)
    		LOG_DEBUG << "---" << __FUNCTION__ << ",代理["<< agentid <<"],支付商名字["<< paymentname <<"],尊享配置[" << paymentconfig << "]";
    }

    // 存缓存
    m_ChannleAgentMap[(agentid << 8 | level)].push_back(channleItemMap);
    LOG_ERROR << "---" << __FUNCTION__ << ",代理["<< agentid <<"],Channle Agent Map,size[" << m_ChannleAgentMap[(agentid << 8 | level)].size() << "]";
}

// 缓存渠道信息
void PayServer::cacheChannleThirdParty(int32_t agentid,int32_t level, int32_t paytype,vector<string> & paychannelVec)
{
    // 根据支付渠道去取支持的支付方式
    auto _array = bsoncxx::builder::basic::array{}; //make_array("A", "D")
    for (auto channleName : paychannelVec)
    {
        _array.append(channleName);
    }

    // 渠道信息
    std::map<string, std::vector<PaymentItem>> channleItemMap;

    // paytype 1 为移动端 0　为PC端 bonusprop
    mongocxx::collection pay_coll = MONGODBCLIENT["gamemain"]["membertype_channel"];
    mongocxx::cursor cursor = pay_coll.find(make_document(kvp("agentid", agentid), kvp("paytype", paytype), kvp("membertypeno", level), kvp("paymentchannel", make_document(kvp("$in", _array)))));
    for (auto &doc : cursor)
    {
        int32_t status = doc["status"].get_int32();
        if(status != eDefaultStatus::op_ON){
            continue;
        }

        int32_t agentid = doc["agentid"].get_int32();
        int32_t minquota = 0,maxquota = 0;

        //玩家当前积分
        switch (doc["minquota"].type())
        {
        case bsoncxx::type::k_utf8:
        {
            string temp = doc["minquota"].get_utf8().value.to_string();
            minquota = atol(temp.c_str());
            }
            break;
        case bsoncxx::type::k_int32:
            minquota = doc["minquota"].get_int32();
            break;
        }

         //玩家当前积分
        switch (doc["maxquota"].type())
        {
        case bsoncxx::type::k_utf8:
        {
            string temp = doc["maxquota"].get_utf8().value.to_string();
            maxquota = atol(temp.c_str());
            }
            break;
        case bsoncxx::type::k_int32:
            maxquota = doc["maxquota"].get_int32();
            break;
        }

        string channel = doc["channel"].get_utf8().value.to_string();
        double bonusProp = doc["bonusprop"].get_double();
        string agentname = doc["agentname"].get_utf8().value.to_string();
        string channelname = doc["channelname"].get_utf8().value.to_string();
        string paymentchannel = doc["paymentchannel"].get_utf8().value.to_string();
        string paymentcode = doc["paymentcode"].get_utf8().value.to_string();
        string paymentname = doc["paymentname"].get_utf8().value.to_string();
        
        
        LOG_INFO << "---优惠[" << bonusProp << "],代理[" << agentid << "],支付商[" << agentname << "],渠道名[" << channelname << "],渠道码[" << paymentchannel << "],支付方式[" << paymentname << "],channel[" << channel << "]";

        PaymentItem paymentInfo;
        paymentInfo.paymentchannel = paymentchannel;
        paymentInfo.channelname = channelname;
        paymentInfo.bonusProp = (int32_t)GlobalFunc::rate100(to_string(bonusProp));
        paymentInfo.paymentcode = paymentcode;
        paymentInfo.channel = channel;
        paymentInfo.minquota = minquota;
        paymentInfo.maxquota = maxquota;

        if (doc["solidQuota"])
        {
            auto t_solidQuota = doc["solidQuota"].get_array();

            for (auto &solidQuotaItem : t_solidQuota.value)
            {
                paymentInfo.solidQuota.push_back(solidQuotaItem.get_int64());
                LOG_INFO << "---渠道[" << paymentchannel << "],支付商[" << channelname << "],支付编码["<< paymentcode <<"],值[" << solidQuotaItem.get_int64() << "]";
            }
        }
        else
        {
            LOG_ERROR << "---" << __FUNCTION__ << ",代理[" << agentid << "],第三方转帐没有配置便捷输入数值!";
        }

        // 银行信息
        // paymentInfo.bankjson = "";

        // 添加到列表
        channleItemMap[paymentchannel].push_back(paymentInfo);
        LOG_INFO << "---" << __FUNCTION__ << ",代理["<< agentid <<"],渠道信息[" << channelname << "],渠道数量[" << channleItemMap[paymentchannel].size() << "]";
    }

    // 存缓存
    m_ChannleAgentMap[(agentid << 8 | level)].push_back(channleItemMap);
    LOG_ERROR << "---" << __FUNCTION__ << ",代理["<< agentid <<"],Channle Agent Map,size[" << m_ChannleAgentMap[(agentid << 8 | level)].size() << "]";
}


//带名字数组jons字符串转换成空数组json字符串
bool PayServer::readWithoutNameJsonArrayString(std::string strWithNameJson, std::string & strWithoutNameJson)
{
	if (strWithNameJson.size() == 0)
	{
		return false;
	}
	strWithoutNameJson = strWithNameJson;
	boost::iterator_range<std::string::iterator> result = boost::algorithm::find_first(strWithoutNameJson, "[");
	if (!result.empty())
	{
		std::string strTemp = strWithoutNameJson.substr(result.begin() - strWithoutNameJson.begin());
		strWithoutNameJson = strTemp;
		result = boost::algorithm::find_last(strWithoutNameJson, "]");
		//result = boost::algorithm::ifind_first(strWithoutNameJson, "]");
		if (!result.empty())
		{
			strTemp = strWithoutNameJson.substr(0, (result.begin() - strWithoutNameJson.begin() + 1));
			strWithoutNameJson = strTemp;
		}
	}
	return true;
}


// 加载玩家充值数据
int32_t PayServer::loadRechargeInfo(int64_t userId,int32_t agentid,int32_t paytype,int32_t & level,map<string, ChannleBonusItem> & memberbonusMap)
{
	int32_t	retCode=0;
	try
	{
		// 同一层级各个支付渠道优惠
		memberbonusMap.clear();
		do
		{
			//1, 从用户表中查询出会员层级
			auto opts = mongocxx::options::find{};
			opts.projection(document{} << "level" << 1 << "status" << 1 << "isguest" << 1 << "agentid" << 1 << finalize);
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];

			auto find_doc = make_document(kvp("userid", userId));
			auto find_result = user_coll.find_one(find_doc.view(), opts);
			if (!find_result)
			{
				retCode = 1;
				LOG_ERROR << "---" << __FUNCTION__ << " 找不到玩家[" << userId << "]";
				break;
			}

			// 1.1 取出结果
			bsoncxx::document::view view = find_result->view();
			int32_t status = view["status"].get_int32();
			int32_t isguest = view["isguest"].get_int32();
			level = view["level"].get_int32();
			if (status == (int32_t)ePlayerStatus::op_NO) // || isguest == eGuestMode::guest)
			{
				retCode = 1;
				break;
			}

			LOG_INFO << "---" << __FUNCTION__ << " 会员层级[" << level << "],状态[" << status << "]";

			// 2.0 根据层级和代理号去查支付渠道
			mongocxx::collection men_coll = MONGODBCLIENT["gamemain"]["membership_type"];
			auto find_doc_type = make_document(kvp("orderNo", level), kvp("agentid", agentid));
			auto find_type_result = men_coll.find_one(find_doc_type.view());
			if (!find_type_result)
			{
				retCode = 2;
				LOG_ERROR << "---" << __FUNCTION__ << " 根据层级和代理号去查支付渠道失败,userId[" << userId << "]";
				break;
			}

			// 2.1 渠道类型有效性判断
			bsoncxx::document::view type_view = find_type_result->view();
			status = type_view["status"].get_int32();
			if (status != eDefaultStatus::op_ON) // 
			{
				retCode = 2;
				LOG_ERROR << "---" << __FUNCTION__ << " 层级禁用,userId[" << userId << "]";
				break;
			}

			vector<string> thirdpartchannelVec, paychannelVec, bankcardVec, onlinePayVec;
			string bankcardIdString, onlinepayIdString, paychannelString, channelbonusJson;

			if (type_view["onlinepayId"] && type_view["onlinepayId"].type() != bsoncxx::type::k_null)
				onlinepayIdString = type_view["onlinepayId"].get_utf8().value.to_string(); // 分割在线支付信息
			if (type_view["paymentchannel"] && type_view["paymentchannel"].type() != bsoncxx::type::k_null)
				paychannelString = type_view["paymentchannel"].get_utf8().value.to_string(); // 分割渠道类型
			if (type_view["channelbonus"] && type_view["channelbonus"].type() != bsoncxx::type::k_null)
				channelbonusJson = type_view["channelbonus"].get_utf8().value.to_string(); // 会员层级对应优惠彩金值
			if (type_view["bankcardId"] && type_view["bankcardId"].type() != bsoncxx::type::k_null)
				bankcardIdString = type_view["bankcardId"].get_utf8().value.to_string(); // 分割银行帐号信息

			boost::algorithm::split(paychannelVec, paychannelString, boost::is_any_of(","));
			boost::algorithm::split(bankcardVec, bankcardIdString, boost::is_any_of(","));
			boost::algorithm::split(onlinePayVec, onlinepayIdString, boost::is_any_of(","));

			LOG_INFO << "---代理[" << agentid << "],会员层级[" << level << "],渠道类型[" << paychannelString << "],银行卡[" << bankcardIdString << "],在线支付[" << onlinepayIdString << "]";

			// 2.2从缓存中寻找会员层级优惠信息
			int32_t mapIdx = (agentid << 8 | level);
			if (!cacheMemberBonusInfo(level, agentid, mapIdx, channelbonusJson))
			{
				LOG_ERROR << "---mapIdx[" << mapIdx << "],会员[" << userId << "],从缓存中寻找会员层级优惠信息失败";
				retCode = 4;
				break;
			}

			// 2.3 缓存中取出会员层级信息
			auto bouns_itor = m_BonusItemMap.find(mapIdx);
			if (bouns_itor != m_BonusItemMap.end())
				memberbonusMap = bouns_itor->second;

			LOG_INFO << "---代理[" << agentid << "],会员层级[" << level << "],渠道类型[" << paychannelString << "],缓存信息量[" << memberbonusMap.size() << "]";

			// 3 先从缓存中寻找信息
			auto channle_agent_itor = m_ChannleAgentMap.find(agentid << 8 | level);
			if (channle_agent_itor != m_ChannleAgentMap.end())
			{
				// 读缓存
				if (channle_agent_itor->second.size() > 0)
				{
					LOG_INFO << "---" << __FUNCTION__ << " 充值信息找到缓存[" << agentid << "],size[" << channle_agent_itor->second.size() << "]";
					break;
				}
			}

			// 上面没有缓存，则这里缓存
			for (auto channleName : paychannelVec)
			{
				if ((channleName.compare(PAY_CODE_ZX) == 0) || (channleName.compare(PAY_CODE_VIP) == 0))
				{
					// 在线充值　尊享支付．VIP转帐支付
					cacheChannleOnline(agentid, level, paytype, onlinePayVec, channleName);
				}
				else if (channleName.compare(PAY_CODE_WY) == 0)
				{
					// 银行卡转帐支付
					cacheChannleWY(agentid, level, bankcardVec);
				}
				else
				{
					thirdpartchannelVec.push_back(channleName);
				}
			}

			// 第三方支付（银联支付，京东支付，微信，支付宝，云闪付）
			if (thirdpartchannelVec.size() > 0)
			{
				cacheChannleThirdParty(agentid, level, paytype, thirdpartchannelVec);
			}

		} while (0);
	}
	catch (exception &e)
	{
		retCode = 5;
		LOG_ERROR << " >>> Exception: " << e.what();
	}

	return retCode;
}

//=============================work thread=========================
bool PayServer::sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data,
                          internal_prev_header *internal_header, Header *commandHeader)
{
    bool bSuccess = false;
    TcpConnectionPtr conn(weakConn.lock());
    if(likely(conn))
    {
        int size = data.size();

        // 需要加密
        if (commandHeader->encType == PUBENC__PROTOBUF_AES)
        {
            int32_t headLen = sizeof(internal_prev_header) + sizeof(Header);
            string aesKey(internal_header->aesKey, sizeof(internal_header->aesKey));

            // LOG_INFO << ">>> 加密前长度[" << size <<"],aesKey[" << aesKey << "],uid[" << internal_header->userId << "],mainid[" << commandHeader->mainId <<"],subid[" << commandHeader->subId <<"]";
            // MutexLockGuard lock(m_send_encode_data_mutex);

            vector<unsigned char> encode_data;
            int ret = Crypto::AES_ECB_Encrypt_EX(aesKey, (unsigned char *)&data[0] + headLen, (size - headLen), encode_data);
            if (ret > 0)
            {
                size_t newSize = headLen + encode_data.size();
                vector<unsigned char> senddata(newSize);
                // LOG_INFO << ">>> 加密后总长度[" << newSize <<"],加密数据长度[" << encode_data.size() <<"]";

                // 复制网关客户端协议头
                memcpy(&senddata[0], &data[0], headLen);
                memcpy(&senddata[headLen], &encode_data[0], encode_data.size());

                // 复制网关协议头
                internal_prev_header *send_internal_header = (internal_prev_header *)(&senddata[0]);
                memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
                send_internal_header->len = newSize;

                // 复制客户端协议头
                Header *commandSendHeader = (Header *)(&senddata[sizeof(internal_prev_header)]);
                memcpy(commandSendHeader, commandHeader, sizeof(Header));

                // 重设置长度
                commandSendHeader->len = newSize - sizeof(internal_prev_header);
                commandSendHeader->realSize = newSize - headLen;
                // 校验数据
                commandSendHeader->crc = GlobalFunc::GetCheckSum(&senddata[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);

                /*
                string strtemp,strbase64;
                strtemp.assign(encode_data.begin(),encode_data.end());
                Landy::Crypto::base64Encode(strtemp,strbase64);
                LOG_WARN << ">>> strbase64 <<< [" << strbase64 << "]";  
                // LOG_INFO << ">>> 加密成功 <<< realSize[" << encode_data.size() << "],crc[" << commandSendHeader->crc <<"],newSize[" << newSize <<"],len["<< commandSendHeader->len <<"]";  
                // LOG_ERROR << ">>> data.size <<< [" << senddata.size() << "]";  
                */

                // 发送数据
                conn->send(&senddata[0], senddata.size());

                bSuccess = true;
            }
            else
            {
                string strtemp, strbase64;
                strtemp.assign(encode_data.begin(), encode_data.end());
                Crypto::base64Encode(strtemp, strbase64);
                LOG_ERROR << ">>> 加密数据失败 <<< size[" << encode_data.size() << "],strbase64[" << strbase64 << "],uid[" << internal_header->userId << "],mainid[" << commandHeader->mainId <<"],subid[" << commandHeader->subId <<"]";
            }
        }
        else if (commandHeader->encType == PUBENC__PROTOBUF_NONE)
        {
            internal_prev_header *send_internal_header = (internal_prev_header *)(&data[0]);
            memcpy(send_internal_header, internal_header, sizeof(internal_prev_header));
            send_internal_header->len = size;

            Header *commandSendHeader = (Header *)(&data[sizeof(internal_prev_header)]);
            memcpy(commandSendHeader, commandHeader, sizeof(Header));

            commandSendHeader->len = size - sizeof(internal_prev_header);
            commandSendHeader->realSize = size - sizeof(internal_prev_header) - sizeof(Header);
            commandSendHeader->crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], commandSendHeader->len - 4);

            conn->send(&data[0], data.size());
            if(commandSendHeader->subId != 2) 
                LOG_INFO << ">>> 不加密数据 <<< size[" << data.size() << "],mainid[" << commandSendHeader->mainId <<"],subid[" << commandSendHeader->subId <<"]";
        }
    }

    return bSuccess;
}

