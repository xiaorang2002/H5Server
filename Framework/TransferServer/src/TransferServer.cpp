#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp> 
#include <boost/property_tree/json_parser.hpp>

#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/json.hpp"
#include "bsoncxx/types.hpp"
#include "bsoncxx/oid.hpp"
#include "bsoncxx/stdx/optional.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/stdx.hpp"
#include "mongocxx/uri.hpp"
#include "mongocxx/instance.hpp"

#include "Globals.h"
#include "gameDefine.h"
#include "GlobalFunc.h"
#include "ThreadLocalSingletonMongoDBClient.h" 
#include "TraceMsg/FormatCmdId.h"
#include "SubNetIP.h"
#include "json/json.h"
#include "mymd5.h"
#include "base64.h"
#include "htmlcodec.h"
#include "crypto.h"
#include "urlcodec.h" 
#include "TransferServer.h"

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

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

std::map<int, TransferServer::AccessCommandFunctor>  TransferServer::m_functor_map;

// 大唐帐转进入天下汇
#define TRANS_IN  	"IN"
// 天下汇帐转进入大唐
#define TRANS_OUT  	"OUT"
// 版本
#define VERSION  	"100"

#define NOT_NEED_UPDATE  	-1




TransferServer::TransferServer(muduo::net::EventLoop* loop,	const muduo::net::InetAddress& listenAddr,const muduo::net::InetAddress& listenAddrHttp,std::string const& strIpAddr,muduo::net::TcpServer::Option option)
		: httpServer_(loop, listenAddrHttp, "TransferHttpServer", option)
        , threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "TimerEventLoopThread"))
        , m_pIpFinder("qqwry.dat")
        , strIpAddr_(strIpAddr)
        , m_bInvaildServer(false)
        , isStartServer(false)
{
	errcode_info_.clear();
	// 
	 m_pattern= boost::regex("\n|\r"); 

	setHttpCallback(CALLBACK_2(TransferServer::processHttpRequest, this));
	httpServer_.setConnectionCallback(CALLBACK_1(TransferServer::onHttpConnection, this));
	httpServer_.setMessageCallback(CALLBACK_3(TransferServer::onHttpMessage, this)); 
    
	threadTimer_->startLoop();
	srand(time(nullptr));
}

TransferServer::~TransferServer()
{
    m_functor_map.clear();
    quit();
}
 
void TransferServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
    threadPool_.stop();
}

//MongoDB/redis与线程池关联 ///
void TransferServer::threadInit()
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

bool TransferServer::InitZookeeper(std::string ipaddr)
{
    LOG_INFO << __FUNCTION__ <<" ipaddr["<< ipaddr <<"]";
	m_zookeeperclient.reset(new ZookeeperClient(ipaddr));
    //指定zookeeper连接成功回调 ///
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&TransferServer::ZookeeperConnectedHandler, this));
    //连接到zookeeper，成功后回调ZookeeperConnectedHandler ///
    if (!m_zookeeperclient->connectServer()) {
        LOG_ERROR << "InitZookeeper error";
        // abort();
    }

    LOG_INFO << "---InitZookeeper OK !";

	return true;
}

bool TransferServer::InitRedisCluster(std::string ipaddr, std::string password)
{
	LOG_INFO <<"---ipaddr["<< ipaddr <<"],password["<<password<<"]";

	m_redisPubSubClient.reset(new RedisClient());
	if (!m_redisPubSubClient->initRedisCluster(ipaddr, password)) {
		LOG_ERROR << "InitRedisCluster error,ipaddr[" << ipaddr <<"]";
	}
	// 保存
	m_redisPassword = password;
	//更新代码消息
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_Trans_proxy_info,CALLBACK_0(TransferServer::refreshAgentInfo,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_Trans_server,CALLBACK_1(TransferServer::repairLoginServer,this)); 
	// redis广播
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_noticfy_recycle_balance, [&](string msg) {
		LOG_INFO << "---收到回收余额消息[" << msg << "],IsMaster[" << initcfg_.IsMaster <<"]";
		if (initcfg_.IsMaster == 1)
		{
			threadTimer_->getLoop()->runInLoop(CALLBACK_0(TransferServer::recycleBalance,this,(int32_t)eRecycleType::one,msg)); 
		}
	});

	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_third_part_game,CALLBACK_0(TransferServer::cacheThirdPartGame,this));

	

	// 
	m_redisPubSubClient->startSubThread();

	m_redisIPPort = ipaddr;

	return true;
}

bool TransferServer::InitMongoDB(std::string url)
{
	LOG_INFO << __FUNCTION__ << "---DB url " << url;
    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}

//zookeeper连接成功回调 ///
void TransferServer::ZookeeperConnectedHandler()
{
	LOG_INFO << __FUNCTION__ << "zookeeper连接成功回调";
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");
    //TransferServers节点集合
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/TransferServers"))
        m_zookeeperclient->createNode("/GAME/TransferServers", "TransferServers");
 
    //本地节点启动时自注册 ///
	{
        //指定网卡ip:port ///
		std::vector<std::string> vec;
		boost::algorithm::split(vec, httpServer_.ipPort(), boost::is_any_of(":"));
		
		m_szNodeValue = strIpAddr_ + ":" + vec[1];
		m_szNodePath = "/GAME/TransferServers/" + m_szNodeValue;
		//自注册LoginServer节点到zookeeper
		if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
			m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
		}
		m_szInvalidNodePath = "/GAME/TransferServersInvaild/" + m_szNodeValue;
	}  
}

//自注册服务节点到zookeeper ///
void TransferServer::OnRepairServerNode()
{
	if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
		LOG_WARN << "--- *** " << m_szNodePath;
		m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true); 
	}  
}

//通知停止上下分操作处理 
// 维护登录服
void TransferServer::repairLoginServer(string msg)
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
				LOG_WARN << "---repair TransferServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else if ( m_szNodeValue.compare(ipaddr) == 0 )
			{
				m_bInvaildServer = (status == eUpholdType::op_stop);
				LOG_WARN << "---repair TransferServer OK,当前[" << m_bInvaildServer <<"]";
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
void TransferServer::start(int numThreads, int workerNumThreads)
{
	// 最晚一个启动的服务结束后开启变量 isStartServer = true
	threadTimer_->getLoop()->runEvery(10.0f, bind(&TransferServer::OnRepairServerNode, this)); 
	threadTimer_->getLoop()->runAfter(1.5f, bind(&TransferServer::refreshAgentInfo, this));
	threadTimer_->getLoop()->runAfter(1.25f, bind(&TransferServer::cacheThirdPartGame, this));
	threadTimer_->getLoop()->runEvery(1.0f, [&]() {
		// 
		// if (initcfg_.IsMaster == 0)
		// 	return; 

		// 固定回收周期
		if (++nSecondTick_ < initcfg_.RecycleCycle){
			// 定时加载conf配置文件
			if(nSecondTick_ > 0 && (nSecondTick_ % initcfg_.ReadConfCycle == 0)){
				LoadIniConfig();
			}
			return;
		}
		nSecondTick_ = 0; 

		// 主才处理回收动作
		if (initcfg_.IsMaster == 0)
			return; 

		// 收到更新信息
		if( initcfg_.AutoRecycle == 1 )
			recycleBalance((int32_t)eRecycleType::all,"");
	});
	// 加载Json
	threadTimer_->getLoop()->runAfter(0.1f, bind(&TransferServer::loadJson, this));
 
	LOG_WARN << "---启动TCP监听--- ";
}

//启动HTTP业务线程 ///
//启动HTTP监听 ///
void TransferServer::startHttpServer(int numThreads, int workerNumThreads) {
	//处理HTTP回调
	setHttpCallback(CALLBACK_2(TransferServer::processHttpRequest,this));

	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&TransferServer::threadInit, this));

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
void TransferServer::onHttpConnection(const muduo::net::TcpConnectionPtr& conn)
{  
	if (conn->connected())
	{
		conn->setContext(muduo::net::HttpContext());
		nConnections_ = numConnections_.incrementAndGet();
		// 网络连接数
		if(initcfg_.isLog <= (int32_t)eLogLvl::LVL_0) {
			LOG_INFO << "---网络连接数["<< nConnections_ <<"],WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN");
		}
		//最大连接数限制
		if (nConnections_ > initcfg_.maxConnCount) {
			//HTTP应答包
			string jsonResStr = createJsonString(0,(int32_t)eErrorCode::MaxConnError,"maxConnCount:" + to_string(initcfg_.maxConnCount));
			muduo::net::HttpResponse rsp(false);
			setFailedResponse(rsp,muduo::net::HttpResponse::k404NotFound,jsonResStr);
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
			conn->forceClose();//强制关闭连接

			LOG_ERROR << "---最大连接数限制["<< nConnections_ <<"]["<<initcfg_.maxConnCount<<"]";
			return;
		}
	}
	else {
		nConnections_ = numConnections_.decrementAndGet();
		if(initcfg_.isLog <= (int32_t)eLogLvl::LVL_0) {
			LOG_INFO << "---网络连接数["<< nConnections_ <<"],WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN");
		}
	}
}

//接收HTTP网络消息回调 ///
void TransferServer::onHttpMessage(const muduo::net::TcpConnectionPtr& conn,
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

		if (initcfg_.isLog < (int32_t)eLogLvl::LVL_3)
			LOG_INFO << "---网络消息数[" << nReqCount_ << "],每秒连接数[" << nConnFreqCount << "]";

		// 防止攻击处理
		int64_t nCurTickTime = time(nullptr);
		if( nCurTickTime - nLastTickTime >= 1 ){
			
			nLastTickTime = time(nullptr);
			nLastnReqCount = nReqCount_; 

			// 日志级别恢复
			if( nConnFreqCount < 100 && nLogLevelCopy != 0 ) {
				initcfg_.isLog = nLogLevelCopy;
				nLogLevelCopy = 0;
			}
		}

		// 频率太高则关闭(每秒1000连接)
		if ( nConnFreqCount > initcfg_.maxConnFreqCount)
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
				nLogLevelCopy = initcfg_.isLog;
				initcfg_.isLog = (int32_t)eLogLvl::LVL_2;
			}

			LOG_ERROR << "---每秒连接数太大,关了连接["<< nConnFreqCount <<"],isLog["<< initcfg_.isLog <<"]";
			return;
		}

		// 白名单过滤
		string peerIp = context->request().getHeader("X-Forwarded-For");
        if(peerIp.empty()){
            peerIp = conn->peerAddress().toIp();
        }
		else{
			if (initcfg_.isLog < (int32_t)eLogLvl::LVL_2)
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
		if (initcfg_.isLog < (int32_t)eLogLvl::LVL_2)
       	   LOG_INFO <<"--->>> onHttpMessage peerAddress["<< peerIp <<"],ipNetEndian["<<addr.ipNetEndian()<<"]whiteIP["<<initcfg_.isWhiteIp<<"]";

		//IP访问白名单检查
		if ( initcfg_.isWhiteIp == 1 ) {
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
			
       	 		if (initcfg_.isLog < (int32_t)eLogLvl::LVL_1)
					LOG_WARN <<"--->>> 不在IP访问白名单中["<< peerIp <<"],ipNetEndian["<< addr.ipNetEndian() <<"]";
				return;
			}
    	} 

		//扔给任务消息队列处理
		muduo::net::TcpConnectionWeakPtr weakConn(conn);
		threadPool_.run(std::bind(&TransferServer::asyncHttpHandler, this, weakConn, context->request(),addr.ipNetEndian()));
		//释放资源
		//context->reset();
	}
}

//异步回调 ///
void TransferServer::asyncHttpHandler(const muduo::net::TcpConnectionWeakPtr& weakConn, const muduo::net::HttpRequest& req,uint32_t _ipNetEndian)
{
	const string& connection = req.getHeader("Connection");

	// 试玩环境下对连接太频繁关掉
	if ( initcfg_.isDemo == 1 && initcfg_.ip_timeout > 0 ) 
	{
		muduo::net::TcpConnectionPtr conn(weakConn.lock());
		if( conn ){
			//直接强制关闭连接
			string ipStr = to_string(_ipNetEndian);
			string ipKey = REDIS_KEY_ID + to_string((int)eRedisKey::str_demo_ip_limit) + "_" + ipStr;

			if(initcfg_.isLog < (int32_t)eLogLvl::LVL_1) LOG_WARN << "---ipKey " << ipKey <<" ip_timeout "<<initcfg_.ip_timeout;
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
				REDISCLIENT.resetExpiredEx(ipKey,initcfg_.ip_timeout);
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

//解析请求 ///
bool TransferServer::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg)
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
std::string TransferServer::getRequestStr(muduo::net::HttpRequest const& req) {
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
std::string TransferServer::createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr)
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
std::string TransferServer::createJsonRsp(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,int64_t newScore)
{
	boost::property_tree::ptree root,data;
	root.put("code", ":code");
	root.put("msg", msgstr);

	data.put("uid", ":uid");
	data.put("type", ":type");
	data.put("lgname",lgname);
	data.put("dt",":dt");
	data.put("score",":score");
	
	root.add_child("data", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	replace(json, ":type", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	replace(json, ":uid", std::to_string(uid));
	replace(json, ":score", std::to_string(newScore));
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
std::string TransferServer::createJsonString(int32_t opType,int32_t errcode,string const& dataStr)
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
 
// ptree image_array = pt.get_child("images"); // get_child得到数组对象
// 遍历数组
// BOOST_FOREACH (boost::property_tree::ptree::value_type &v, image_array)
// {
// 	std::stringstream s;
// 	write_json(s, v.second);
// 	std::string image_item = s.str();
// }

// 检测并创建游戏帐号
// @actype 账号类型：0-试玩账号 1-真钱账号
bool TransferServer::getCreateAccountParams(string & lgname, int32_t actype, int32_t aid,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_WARN << "---"<<__FUNCTION__<<",lgname[" << lgname << "],帐号类型[" << (actype == 0 ? "试玩帐号":"真人帐号") << "],aid[" << aid << "]";

	boost::property_tree::ptree root,req_pt;
	root.put("cagent", agent_info_.agent);
	root.put("loginname", lgname);
	root.put("password", initcfg_.Password);
	root.put("method", "lg");
	root.put("cur", initcfg_.MoneyType);
	root.put("actype", ":actype");	//
	root.put("agentId", ":aid");	//代理ID

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	replace(jsonstr, ":actype", std::to_string(actype)); 
	replace(jsonstr, ":aid", std::to_string(aid)); 
	TirmString(jsonstr);

	// 发起请求
	resp = getUrlRequest(jsonstr, initcfg_.resAddUrl);
	if (resp.status_code != 200)
	{
		req_code = resp.status_code;
		// 不等于404则
		if (resp.status_code != muduo::net::HttpResponse::k404NotFound){
			req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		}
		std::stringstream ssss;
		ssss << "account Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		errmsg = ssss.str();
		return false;
	}

	// 取结果码
	if (!getJsonPtree(resp.text, req_code, errmsg, req_pt))
	{
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "account Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		return false;
	}

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}

// 获取查询连接参数
// @bOnlineCheck是否获取在线状态，true,获取
string TransferServer::balanceInquiryParams(string & lgname,bool bOnlineCheck)
{
	boost::property_tree::ptree root;
	root.put("cagent", agent_info_.agent);
	root.put("loginname", lgname);
	root.put("password", initcfg_.Password);
	// 加个 这个参数 queryStatus
	if (bOnlineCheck){
		// 要查游戏状态的话就多传一个这个参数，参数是布尔类型  传个true 过来
		root.put("queryStatus", ":bool");
	}
	root.put("method", "gb");

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	if (bOnlineCheck){
		replace(jsonstr, ":bool", bOnlineCheck?"true":"false"); 
	}
	TirmString(jsonstr);

	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "],jsonstr[" << jsonstr << "]";

	return jsonstr;
}
// 余额查询
bool TransferServer::getBalance(string & lgname, int64_t & balance,int32_t & req_code,string & errmsg,cpr::Response & resp,bool & status_online)
{
	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "]";
	
	std::string jsonstr = balanceInquiryParams(lgname,true);
	boost::property_tree::ptree req_pt;

	resp = getUrlRequest(jsonstr, initcfg_.resAddUrl);
	if (resp.status_code != 200)
	{
		req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		std::stringstream ssss;
		ssss << "---balance Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		errmsg = ssss.str();
		return false;
	}
  
	// 取结果码
	try
	{
		std::stringstream ss(resp.text);
		read_json(ss, req_pt);
		errmsg 		= req_pt.get<string>("msg");
		req_code 	= req_pt.get<int32_t>("code");
		// 玩家在线状态
		status_online = req_pt.get<bool>("data.status");
	}
	catch (exception excep)
	{
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "],resp.text[" << resp.text << "]";
		return false;
	}
	
	/*
	// 取结果码  
	if (!getJsonPtree(resp.text, req_code,errmsg,req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		return false;
	}
	*/

	// 有可能有其它的错误返回
	if(req_code != (int32_t)eErrorCode::NoError ){
		return false;
	}
	// 
	double fbalance = req_pt.get<double>("data.balance"); //req_pt.get_child("data"); 
	string balanceStr = to_string(fbalance);
	balance = GlobalFunc::rate100(balanceStr);

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],data.balance[" << fbalance << "],balance[" << balance << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}
 // 查询订单状态
bool TransferServer::orderInquiryParams(string & billNo,int32_t actype,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",billNo[" << billNo << "]";

	boost::property_tree::ptree root,req_pt;
	root.put("cagent", agent_info_.agent);		//代理账号
	root.put("method", "qos");					//接口类型：tc
	root.put("billNo", billNo); 				//唯一且长度为20位的订单号
	root.put("actype", ":actype");				//账号类型：0-试玩账号 1-真钱账号
	root.put("cur", initcfg_.MoneyType);			//货币种类-CNY

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	replace(jsonstr, ":actype", std::to_string(actype)); 
	TirmString(jsonstr);

	resp = getUrlRequest(jsonstr, initcfg_.resAddUrl);
	if (resp.status_code != 200)
	{
		req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		std::stringstream ssss;
		ssss << "---order Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		errmsg = ssss.str();
		return false;
	}

	// 取结果码  
	if (!getJsonPtree(resp.text, req_code, errmsg,req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "order Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		return false;
	}

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}
 

 // 获取查询连接参数
string TransferServer::createTransferParams(string lgname, string billNo, string type,string creditStr)
{
	LOG_INFO << "---" << __FUNCTION__ << ",lgname[" << lgname << "]";
	// 返回数据
	boost::property_tree::ptree root;
	root.put("cagent", agent_info_.agent); //代理账号
	root.put("loginname", lgname);				 //游戏帐号
	root.put("password", initcfg_.Password);		 //游戏密码
	root.put("method", "tc");					 //接口类型：tc
	root.put("type", type);						 //IN: 从网站帐号转款到游戏帐号,OUT：从游戏转出
	root.put("billNo", billNo);					 //唯一且长度为20位的订单号
	root.put("credit", creditStr);				 //转账金额，保留小数点后两位

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	TirmString(jsonstr);

	return jsonstr;
}

// 游戏转账（上下分）
//type:IN: 从网站帐号转款到游戏帐号,OUT：从游戏转出
//credit(score): 转账金额，保留小数点后两位
bool TransferServer::transferBalance(int64_t &newScore,string &lgname, string billNo, string type, int64_t userid, int64_t credit,
									int64_t & _newbalance, int32_t &req_code, string &errmsg,string &da_account, cpr::Response &resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",billNo["<< billNo <<"],lgname["<< lgname <<"],type["<< type <<"],credit["<< credit <<"]"; 

	// 1.1数字转小数点，保留小数点后两位
	string creditStr = GlobalFunc::getDigitalFormat(credit);

	// 返回数据
	std::string jsonstr = createTransferParams(lgname,billNo,type,creditStr);

	int64_t oldbalance = 0;
	int64_t newbalance = 0;
	boost::property_tree::ptree req_pt;

	// 2,写订单记录
	createOrder(billNo,lgname,type,credit,userid,da_account);

	// 3,请求上下分
	resp = getUrlRequest(jsonstr, initcfg_.resAddUrl);
	if (resp.status_code != 200)
	{
		req_code = resp.status_code;
		// 不等于404则
		// if (resp.status_code != muduo::net::HttpResponse::k404NotFound){
		// 	req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		// }
		std::stringstream ssss;
		ssss << "---order Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		errmsg = ssss.str(); 
		updateOrder(newScore,userid,type,credit,billNo,(int32_t)eOrderType::OpTiemOut,resp.status_code,resp.elapsed);
		return false;
	}

	// 4,取结果码
	if (!getJsonPtree(resp.text,req_code,errmsg, req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "transfer Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		updateOrder(newScore,userid,type,credit,billNo,(int32_t)eOrderType::OpJsonErr,resp.status_code,resp.elapsed);
		return false;
	}

	// 5,处理成功
	if(req_code == 0){
		double oldMoney = req_pt.get<double>("data.oldMoney");
		double newMoney = req_pt.get<double>("data.newMoney");

		string oldMoneyStr = to_string(oldMoney);
		oldbalance = GlobalFunc::rate100(oldMoneyStr);

		string newMoneyStr = to_string(newMoney);
		newbalance = GlobalFunc::rate100(newMoneyStr); 
		_newbalance = newbalance;
		updateOrder(newScore,userid,type,credit,billNo,(int32_t)eOrderType::OpFinish,resp.status_code,resp.elapsed,oldbalance,newbalance);
		
		LOG_INFO << "---转帐单成功---userid[" << userid << "],txhaccount[" << lgname << "],回收余额["
								 << creditStr << "],elapsed[" << resp.elapsed << "],对方帐户转前[" << oldbalance << "],转后[" << newbalance << "]";
	}
	else{
		updateOrder(newScore,userid,type,credit,billNo,(int32_t)eOrderType::OpUndefind,resp.status_code,resp.elapsed);
	}

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "],billNo["<< billNo <<"]";
	return true;
}
// 进入游戏
// @mh5 y：代表h5 ，n：代表pc
// @oddtype 值A（20-50000）、B（50-5000）、C（20-10000）、D（200-20000）、E（300-30000）、F（400-40000）、G（500-50000）、H（1000-100000）、I（2000-200000）
bool TransferServer::enterGameParams(string & lgname,string mh5,string oddtype,string gametype,string & req_forwardUrl,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "],oddtype[" << oddtype <<"]";

	boost::property_tree::ptree root,req_pt;
	root.put("cagent", agent_info_.agent);
	root.put("loginname", lgname);
	root.put("password", initcfg_.Password);
	root.put("method", "lgg");
	root.put("mh5", mh5);						//y：代表h5 ，n：代表pc
	root.put("oddtype", oddtype);				//盘口，设定玩家可下注的范围，不传默认为A
	root.put("gametype", gametype);				//盘口，设定玩家可下注的范围，不传默认为A

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	TirmString(jsonstr);

	resp = getUrlRequest(jsonstr, initcfg_.enterGameHandle);
	if (resp.status_code != 200)
	{
		req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		std::stringstream ssss;
		ssss << "---order Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		errmsg = ssss.str();
		return false;
	}

	// 取结果码
	if (!getJsonPtree(resp.text, req_code, errmsg, req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "enterGame Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		return false;
	}
	// 跳转连接
	std::string forwardUrl = req_pt.get<string>("data.forwardUrl");
	// 返回
	req_forwardUrl = forwardUrl;

	// // 加密forwardUrl返回
	// Crypto::AES_ECB_Encrypt(initcfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
	// req_forwardUrl = URL::Encode(tmpEncode);

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],forwardUrl[" << forwardUrl << "],req_forwardUrl[" << req_forwardUrl << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}
// 
// 统一请求参数之获取
cpr::Response TransferServer::getUrlRequest(string & parameContent,string & reqHandle)
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
	param.AddParameter({"params",URL::Encode(strDesOut)});
	param.AddParameter({"key", strMd5.c_str()});

	string _requrl = agent_info_.ApiUrl;
	if (!reqHandle.empty())
	{
		 _requrl = _requrl + "/" + reqHandle;
	}

	LOG_INFO << "---req string ---[" << _requrl << "?" <<param.content << "]";  

	auto resp_feature = cpr::Get(cpr::Url{_requrl},cpr::Timeout{initcfg_.Timeout}, cpr::Header{{"AgentName", agent_info_.agent}}, param);

	LOG_INFO << "---Request elapsed[" << resp_feature.elapsed << "],status_code[" << resp_feature.status_code << "],text[" << resp_feature.text << "]";

	// 请求超时打印
	if(resp_feature.elapsed * 1000 >= initcfg_.Timeout){
		std::cout << "============================================" << std::endl;
		LOG_ERROR << "---请求超时[" << parameContent << "]";  
		std::cout << "============================================" << std::endl;
	}
	// 统一请求参数之获取
	return resp_feature; 
}

// 解析请求返回结果
bool TransferServer::getJsonPtree(string &jsonStr, int32_t &code, string &msg,boost::property_tree::ptree & pt)
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

// 检查游戏有效性
int32_t TransferServer::checkGameIDValid(ReqParamCfg &reqParam,string & errmsg)
{
	auto iter = m_GameIdListMap.find(reqParam.gameid);
	if (iter == m_GameIdListMap.end())
	{
		errmsg = "没有找到对应游戏ID." + to_string(reqParam.gameid) + ",gametype." + to_string(reqParam.gametype);
		return (int32_t)eErrorCode::NotExist;
	}

	if (iter->second.repairstatus == eUpholdType::op_stop)
	{
		errmsg = "游戏维护中";
		return (int32_t)eErrorCode::ServerStopError;
	}

	reqParam.gamecode = iter->second.gamecode;
	reqParam.companyCode = iter->second.companyCode;
	reqParam.platformCode = iter->second.companyCode; 

	LOG_INFO << "---" << __FUNCTION__ << " gameid[" << iter->second.gameid << "],gamename[" << iter->second.gamename << "],gameInfoId[" << iter->second.gameInfoId << "],lotterytype[" << iter->second.lotterytype << "]";
	return (int32_t)eErrorCode::NoError;
}

// 检查请求参数
int32_t TransferServer::checkReqParams(string & reqStr,int64_t & uid,int32_t & actype,string & lgtype,string & oddtype,string & backurl, string & errmsg,ReqParamCfg &reqParam)
{
	std::string paraValue;
	int32_t errcode = (int32_t)eErrorCode::NoError;

	// 测试使用，不带参数
	if( initcfg_.isdecrypt == 0 ) return errcode;

	bool IsInValid = (reqStr.length() <= 1) ? true : false; // 空判断
	if (IsInValid)
		errmsg = "req str is null" + reqStr;

	do
	{
		try
		{
			//解析参数校检IsInValid && 
			HttpParams params;
			if (!parseQuery(reqStr, params, errmsg))
			{
				errcode =  (int32_t)eErrorCode::ParamsError;
				break;
			}

			if (initcfg_.isLog < (int32_t)eLogLvl::LVL_1)
				LOG_INFO << "--- TransHandle[" << reqStr << "],size[" << params.size() << "]";

			//paraValue
			HttpParams::const_iterator paramValueKey = params.find("paraValue");
			if (paramValueKey == params.end() || paramValueKey->second.empty())
			{
				errcode =  (int32_t)eErrorCode::ParamsError; // 传输参数错误
				errmsg = "paraValue invalid";
				break;
			}
			paraValue = paramValueKey->second;

			//paraValue Decode
			std::string strURL = URL::MultipleDecode(paraValue);
			LOG_INFO << "---paraValue[" << paraValue << "],Decode[" << strURL << "]";
			strURL = boost::regex_replace(strURL, m_pattern, "");

			//AES校验
			paraValue = Crypto::AES_ECB_Decrypt(initcfg_.AesKey, strURL);
			LOG_INFO << "---AES ECB Decrypt[" << paraValue << "],key[" << initcfg_.AesKey << "]";
			if (paraValue.empty())
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
				errmsg = "DESDecrypt failed, AES_set_decrypt_key error";
				break;
			}

			//解析参数 paraValue
			HttpParams paraValueParams;
			if (!parseQuery(paraValue, paraValueParams, errmsg))
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 传输参数错误
				errmsg = "paraValue ok, but bad request paramValue";
				break;
			}

			//userid
			HttpParams::const_iterator useridKey = paraValueParams.find("userid");
			if (useridKey == paraValueParams.end() || useridKey->second.empty())
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 必传输参数
				errmsg = "userid error,userid[" + useridKey->second + "]";
				break;
			}
			else
			{
				if (IsDigitStr(useridKey->second))
				{
					uid = atol(useridKey->second.c_str());
				}
				else
				{
					errcode = (int32_t)eErrorCode::ParamsError; // 传输参数错误
					errmsg = "userid error,userid[" + useridKey->second + "]";
					break;
				}
			}

			// actype 可不传
			HttpParams::const_iterator actypeKey = paraValueParams.find("actype");
			if (actypeKey != paraValueParams.end() && !actypeKey->second.empty() && IsDigitStr(actypeKey->second))
			{
				actype = atol(actypeKey->second.c_str());
			}

			// 配置文件写死
			if(initcfg_.Actype < 2) 
				actype = initcfg_.Actype;

			//lgtype
			HttpParams::const_iterator lgtypeKey = paraValueParams.find("lgtype");
			if (lgtypeKey != paraValueParams.end() && (!lgtypeKey->second.empty()))
			{
				lgtype = lgtypeKey->second;
			}

			//oddtype
			HttpParams::const_iterator oddtypeKey = paraValueParams.find("oddtype");
			if (oddtypeKey != paraValueParams.end() && (!oddtypeKey->second.empty()))
			{
				oddtype = oddtypeKey->second;
			}

			// backurl
			HttpParams::const_iterator backurlKey = paraValueParams.find("backurl");
			if (backurlKey != paraValueParams.end() && (!oddtypeKey->second.empty()))
			{
				backurl = backurlKey->second;
			}

			LOG_INFO << "---backurl["<< backurl << "]";

			// gametype
			HttpParams::const_iterator gametypeKey = paraValueParams.find("gametype");
			if (gametypeKey != paraValueParams.end() && (!gametypeKey->second.empty()))
			{
				reqParam.gametype = atol(gametypeKey->second.c_str());
			}

			// gameid
			HttpParams::const_iterator gameidKey = paraValueParams.find("gameid");
			if (gameidKey != paraValueParams.end() && (!gameidKey->second.empty()))
			{
				reqParam.gameid = atol(gameidKey->second.c_str());
				if(reqParam.gameid > 0){
					int32_t gametypeTest = (reqParam.gameid/100)%1000;
					if (reqParam.gametype != gametypeTest)
					{
						LOG_ERROR << "参数错误,gametype[" << reqParam.gametype <<" != " << gametypeTest << "]";
					}
					
				}
			}

			LOG_INFO << "---gametype["<< reqParam.gametype << "],gameid["<< reqParam.gameid << "]";

		}
		catch (const std::exception &e)
		{
			errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
			errmsg = "param error";
			LOG_ERROR << "Params Error failed," << e.what();
			break;
		}
	
	} while (0);

	LOG_WARN << "---errcode["<< errcode << "],errmsg["<< errmsg <<"],paraValue lgtype[" << lgtype << "],oddtype[" << oddtype << "],userid[" << uid << "],actype[" << actype << "]";

	return errcode;
}



// 检查请求参数
int32_t TransferServer::checkReqParamsEx(string & reqStr,int64_t & uid,int32_t & aid, string & errmsg)
{
	std::string paraValue;
	int32_t errcode = (int32_t)eErrorCode::NoError;

	// 测试使用，不带参数
	if( initcfg_.isdecrypt == 0 ) return errcode;

	bool IsInValid = (reqStr.length() <= 1) ? true : false; // 空判断
	if (IsInValid)
		errmsg = "req str is null" + reqStr;

	do
	{
		try
		{
			//解析参数校检IsInValid && 
			HttpParams params;
			if (!parseQuery(reqStr, params, errmsg))
			{
				errcode =  (int32_t)eErrorCode::ParamsError;
				break;
			}

			if (initcfg_.isLog < (int32_t)eLogLvl::LVL_1)
				LOG_INFO << "--- TransHandle[" << reqStr << "],size[" << params.size() << "]";

			//paraValue
			HttpParams::const_iterator paramValueKey = params.find("paraValue");
			if (paramValueKey == params.end() || paramValueKey->second.empty())
			{
				errcode =  (int32_t)eErrorCode::ParamsError; // 传输参数错误
				errmsg = "paraValue invalid";
				break;
			}
			paraValue = paramValueKey->second;

			//paraValue Decode
			std::string strURL = URL::MultipleDecode(paraValue);
			LOG_INFO << "---paraValue[" << paraValue << "],Decode[" << strURL << "]";
			strURL = boost::regex_replace(strURL, m_pattern, "");

			//AES校验
			paraValue = Crypto::AES_ECB_Decrypt(initcfg_.AesKey, strURL);
			LOG_INFO << "---AES ECB Decrypt[" << paraValue << "],key[" << initcfg_.AesKey << "]";
			if (paraValue.empty())
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
				errmsg = "DESDecrypt failed, AES_set_decrypt_key error";
				break;
			}

			//解析参数 paraValue
			HttpParams paraValueParams;
			if (!parseQuery(paraValue, paraValueParams, errmsg))
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 传输参数错误
				errmsg = "paraValue ok, but bad request paramValue";
				break;
			}

			//userid
			HttpParams::const_iterator useridKey = paraValueParams.find("userid");
			if (useridKey == paraValueParams.end() || useridKey->second.empty())
			{
				errcode = (int32_t)eErrorCode::ParamsError; // 必传输参数
				errmsg = "userid error,userid[" + useridKey->second + "]";
				break;
			}
			else
			{
				if (IsDigitStr(useridKey->second))
				{
					uid = atol(useridKey->second.c_str());
				}
				else
				{
					errcode = (int32_t)eErrorCode::ParamsError; // 传输参数错误
					errmsg = "userid error,userid[" + useridKey->second + "]";
					break;
				}
			}

			// actype 可不传
			HttpParams::const_iterator aidKey = paraValueParams.find("aid");
			if (aidKey != paraValueParams.end() && !aidKey->second.empty() && IsDigitStr(aidKey->second))
			{
				aid = atol(aidKey->second.c_str());
			}
		}
		catch (const std::exception &e)
		{
			errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
			errmsg = "param error";
			LOG_ERROR << "Params Error failed," << e.what();
			break;
		}
	
	} while (0);

	LOG_WARN << "---errcode["<< errcode << "],errmsg["<< errmsg <<"],userid[" << uid << "],aid[" << aid << "]";

	return errcode;
}
// 设置锁(使用条件:允许锁的偶尔失效)
bool TransferServer::setNxDelay(int64_t lckuid){

	int repeatCount = 50;//重试50次后放弃限制
    std::string lockStr = to_string(::getpid());
    do
    {
		std::string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::str_lockId_change_score)+ "_" + std::to_string(lckuid);
        int ret = REDISCLIENT.setnxCmdEx(keyName, lockStr, 100); //100毫秒
        if (ret == -1)
        {
			LOG_ERROR << "redis连接失败,keyName[" << keyName << "],lockStr[" << lockStr << "]";
            return true;
        }
		// 说明可操作
        if (ret == 1)
        {
            // 解锁(不是很有效)
            if (!REDISCLIENT.delnxCmdEx(keyName, lockStr))
            {
                LOG_ERROR << "解锁失败,keyName[" << keyName << "],lockStr[" << lockStr << "]";
            }

            return true;
        }
        else
        {
           usleep(20 * 1000); // 20毫秒
           LOG_WARN << __FUNCTION__ << " 20毫秒延时 " << repeatCount;
        }
    } 
    while (--repeatCount > 0);

	return true;
}

// 锁定用户操作
bool TransferServer::lockUserScore(int64_t uid,int32_t opType, int32_t & req_code,string & errmsg)
{
	try
	{
		errmsg = "score lock failure";
		req_code = (int32_t)eErrorCode::LockScore;
		// db
		mongocxx::collection lock_user_coll = MONGODBCLIENT["gamemain"]["lock_user_downscore"];

		//1, 创建用户，插入game_user表，新增用户
		bsoncxx::document::value inset_value = bsoncxx::builder::stream::document{}
											   << "userid" << bsoncxx::types::b_int64{uid}
											   << "optype" << bsoncxx::types::b_int32{opType}
											   << "optime" << bsoncxx::types::b_date{chrono::system_clock::now()} //订单操作开始时间
											   << bsoncxx::builder::stream::finalize;
		bsoncxx::stdx::optional<mongocxx::result::insert_one> resultInsert = lock_user_coll.insert_one(inset_value.view());
		if ( resultInsert ){
			LOG_WARN << __FUNCTION__ << " 操作成功，锁定玩家[" << uid <<"],操作类型[" << opType << "]";
			req_code = (int32_t)eErrorCode::NoError;
			errmsg = "score lock ok";
			return true;
		}
		else{
			// 操作失败
			LOG_WARN << __FUNCTION__ << " 操作失败，玩家[" << uid <<"],操作类型[" << opType << "]";
			// return false;
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",操作失败,玩家[" << uid <<"],操作类型[" << opType << "]";
		return false;
	}

	return false;
}

// 解锁用户操作
bool TransferServer::unlockUserScore(int64_t uid,int32_t opType,int32_t & req_code,string & errmsg)
{
	try
	{
		errmsg = "unlock score failure";
		req_code = (int32_t)eErrorCode::UnLockFailer;
		// db
		mongocxx::collection lock_user_coll = MONGODBCLIENT["gamemain"]["lock_user_downscore"];

		// 更新玩家状态
		auto findValue = document{} << "userid" << uid << finalize;
		auto find_delete = lock_user_coll.find_one_and_delete(findValue.view());
		if ( find_delete ){
			LOG_WARN << __FUNCTION__ << " 解锁玩家分成功，玩家[" << uid <<"],操作类型[" << opType << "]";
			req_code = (int32_t)eErrorCode::NoError;
			errmsg = "unlock score ok";
			return true;
		}
		else{
			// 操作失败
			LOG_WARN << __FUNCTION__ << " 解锁玩家分失败，玩家[" << uid <<"],操作类型[" << opType << "]";
			// return false;
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",解锁玩家分操作失败,玩家[" << uid <<"],操作类型[" << opType << "]";
	}

	return false;
}

int32_t TransferServer::getUserScore(int64_t &score, int64_t uid)
{
	//查询game_user表
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "score" << 1 << bsoncxx::builder::stream::finalize);
	mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
	auto result = user_coll.find_one(document{} << "userid" << uid << finalize, opts);
	if (!result)
	{
		LOG_ERROR << "not exist this userid[" + to_string(uid) + "]";
		return 1;
	}

	score = result->view()["score"].get_int64();
}

// 根据玩家Id获取用户信息
bool TransferServer::getUserNameById(int64_t uid, int32_t & aid, string &lgname,int64_t & score,string & da_account,int32_t & req_code,string & errmsg)
{
	// 测试使用，不带参数
	if( initcfg_.isdecrypt == 0 ) return true; 

	try
	{
		// 加锁等待1秒
		setNxDelay(uid);
		//查询game_user表
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1
															 << "account" << 1 
															 << "agentid" << 1 
															 << "score" << 1 
															 << "txhstatus" << 1 
															 << "txhaccount" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
		bsoncxx::document::value findValue = document{} << "userid" << uid << finalize;
		auto result = user_coll.find_one(findValue.view(), opts);
		if (!result)
		{
			req_code = (int32_t)eErrorCode::UserNotExistsError;
			errmsg = "not exist this userid[" + to_string(uid) + "]";
			return false;
		}

		bsoncxx::document::view view = result->view();
		int32_t agentId = view["agentid"].get_int32();
		score 			= view["score"].get_int64();
		da_account 		= view["account"].get_utf8().value.to_string();
		aid = agentId;

		LOG_INFO << "---获取用户信息,account[" << da_account << "],aid[" << aid << "],score[" << score << "],uid[" << uid << "]";

		std::string strMd5Account, src; 
		int32_t txhstatus = 0;
		// 不存在,增加虚拟帐号
		if (!view["txhaccount"] || (view["txhaccount"].type() == bsoncxx::type::k_null))
		{
			src = da_account + to_string(agentId);
			strMd5Account = GlobalFunc::getMd5Encode(src,1) + GlobalFunc::getLast4No(uid);
			LOG_WARN << "---update userid["<< uid <<"],src["<< src <<"],strMd5Account[" << strMd5Account <<"]";
			bsoncxx::document::value updateValue = document{}
													<< "$set" << open_document
													<< "txhaccount" << strMd5Account
													<< "txhstatus" << bsoncxx::types::b_int32{0}
													<< close_document << finalize;
			//update strMd5Account
			if (!user_coll.update_one(findValue.view(), updateValue.view()))
				LOG_ERROR << "---更新玩家txhaccount信息失败:" <<"userid [" << uid <<  "] update txhaccount error";
		}
		else{
			strMd5Account = view["txhaccount"].get_utf8().value.to_string();
			txhstatus = view["txhstatus"].get_int32();
		}
		// 取得唯一虚拟帐号
		lgname = strMd5Account; 
	}
	catch (bsoncxx::exception &e)
	{
		req_code = (int32_t)eErrorCode::InsideErrorOrNonExcutive;
		errmsg = "DB error,userid[" + to_string(uid) + "]";
		LOG_ERROR << "---" << __FUNCTION__ <<" MongoDBException: " << e.what();
		return false;
	}

	return true;
}

// 检查连接有效性
bool TransferServer::checkUrlValid( string reqHandle ,int32_t & req_code,string & errmsg)
{
	string _requrl = agent_info_.ApiUrl;
	if (!reqHandle.empty())
	{
		 _requrl = _requrl + "/" + reqHandle;
	}

	LOG_INFO << "---req string ---[" << _requrl << "]";  

	auto resp = cpr::Head(cpr::Url{_requrl}, cpr::Header{{"AgentName", agent_info_.agent}});
	req_code = resp.status_code;
	if (resp.status_code != 200)
	{
		errmsg = "连接无效不可达";
		// 不等于200则
		LOG_ERROR << "account Request Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		return false;
	}

	return true;
}

// 维护状态
bool TransferServer::getMaintenanceStatus(int32_t & req_code,string & errmsg)
{
	// m_bInvaildServer = true;
	req_code = 0;
	if(m_bInvaildServer){
		req_code = (int32_t)eErrorCode::ServerStopError;
		errmsg = "平台维护中";
		LOG_ERROR << errmsg;
	}

	return m_bInvaildServer;
}

// 获取代理视讯限红
bool TransferServer::getProxyXianHong(int32_t aid,string & oddtype)
{
	bool ret = false;
	try
	{
		// 默认值
		oddtype = initcfg_.oddtype;
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
void TransferServer::processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
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
	if (req.path() == "/TransHandle") 
	{
		std::string rspdata,req_forwardUrl,resp_msg,lg_name = VERSION;
		try
		{
			// 参数
			bool islock = false;
			std::string billNo,backurl,req_msg = "ok";
			int32_t aid = 0,actype = 0,req_code = (int32_t)eErrorCode::NoError; 
			int64_t balance = 0,userid = 0,score = 0,newbalance = 0;
			std::string da_account = "",lgType = "y", oddtypeStr = initcfg_.oddtype,reqStr = req.query();
			cpr::Response resp;

			ReqParamCfg reqParam;
			do
			{ 
				// Check 维护状态
				if(getMaintenanceStatus(req_code,req_msg))
					break; 

				// 检查连接是否有效
				// if(checkUrlValid( initcfg_.resAddUrl,req_code,req_msg))
				//	break; 

				// 0,解析请求参数并获取玩家帐号信息----------------------------------------------------------------------------
				// (1)检查参数
				req_code = checkReqParams(reqStr,userid,actype,lgType,oddtypeStr,backurl,req_msg,reqParam);
				if( req_code != (int32_t)eErrorCode::NoError )
					break;

				// 游戏暂未开放
				if(initcfg_.NotOpen == 1){
					req_code = (int32_t)eErrorCode::NotOpen;
					break;
				}

				// (2)检查玩家是否天下汇，是则直接退出
				if(userid == 0){ 
					req_code = (int32_t)eErrorCode::UserNotExistsError;
					break;
				}

				// 1.3 检查游戏参数
				if((req_code = checkGameIDValid( reqParam ,req_msg)) != (int32_t)eErrorCode::NoError){
					LOG_ERROR << req_msg;
					break;
				}

				// (2.1)玩家正在游戏中，不能跳转
				if (REDISCLIENT.ExistsUserOnlineInfo(userid))
				{
					req_msg = "userid " + to_string(userid) + " is playing!";
					req_code =(int32_t)eErrorCode::PlayerOnLine;
					LOG_ERROR << req_msg;
					break;
				}

				// (2.2)(安全性判断)
				if(!lockUserScore(userid,1,req_code,req_msg)){
					LOG_ERROR << "---" << __FUNCTION__ << " req_code:" << req_code << ",req_msg[" << req_msg <<"],userid[" << userid << "]";
					break;
				}
				// 已锁
				islock = true;

				// (3)查询game_user表
				if(!getUserNameById(userid,aid,lg_name,score,da_account,req_code,req_msg) || (req_code != 0))
					break;

				// (4)检查代理权限 
				if(!isPermitTransfer(aid)){
					// 是否UID白名单
					if(initcfg_.WhiteUID.size() > 0)
					{
						auto result = std::find(initcfg_.WhiteUID.begin( ),initcfg_.WhiteUID.end( ), userid); //查找1
						if ( result == initcfg_.WhiteUID.end()){ //找到 
							req_code = (int32_t)eErrorCode::PermissionDenied;
							req_msg = "agent permission denied!";
							break;
						}
						else
						{
							LOG_WARN << "---" << __FUNCTION__ << " userid[" << userid << "] 为白名单访问用户。";
						}
					}
					else
					{
						req_code = (int32_t)eErrorCode::PermissionDenied;
						req_msg = "agent permission denied!";
						break;
					}
				}

				// 1,检测并创建游戏帐号----------------------------------------------------------------------------------------
				if(!getCreateAccountParams(lg_name,actype,aid,req_code,req_msg,resp) || (req_code != 0))
					break;

				// 2，余额查询-------------------------------------------------------------------------------------------------
				bool status_online = false;
				// 余额不足
				if(!getBalance(lg_name,balance,req_code,req_msg,resp,status_online) || (req_code != 0) || (balance == 0) || (status_online == true))
				{
					if(status_online == true){
						req_msg = "user " + to_string(userid) + " is playing video game!";
						req_code =(int32_t)eErrorCode::PlayerOnLine;
						LOG_ERROR << req_msg;
						break;
					}
				}			 

				// 先创建订单
				billNo = GlobalFunc::getBillNo(userid);
				
				int64_t newScore = 0;
				// 3，游戏转账（上下分）-----------------------------------------------------------------------------------------
				if(!transferBalance(newScore,lg_name,billNo,TRANS_IN,userid,score,newbalance,req_code,req_msg,da_account,resp))
					break;
				
				// 4，查询订单状态----------------------------------------------------------------------------------------------
				//过滤代理额度不足，404等
				if (req_code == muduo::net::HttpResponse::k404NotFound){
					break;
				}
				else if (req_code == (int32_t)eErrorCode::GetJsonInfoError) // req_code > (int32_t)eErrorCode::StatusCodeNot200Error
				{
					// 超时或者
					if (!orderInquiryParams(billNo, actype, req_code, req_msg, resp) || (req_code != 0)) //账号类型：0-试玩账号 1-真钱账号
						break;
				}
				else if(req_code != (int32_t)eErrorCode::NoError){
					break;
				}

				// 5，创建跳转记录
				updatePalyingRecord(lg_name,da_account,userid,score,newbalance,reqParam.gameid,reqParam.gametype);

				// 5.1 获取代理盘口值
				getProxyXianHong(aid,oddtypeStr);

				// 6，进入游戏---------------------------------------------------------------------------------------------------
				if(!enterGameParams(lg_name,lgType,oddtypeStr,reqParam.gamecode,req_forwardUrl,req_code,req_msg,resp) || (req_code != 0))
					break;

				LOG_INFO << "DesCode:" << agent_info_.DesCode <<" AgentId:" << agent_info_.aid;
			

				std::string _gamebackurl = URL::MultipleDecode(backurl);
				//  过滤gameid=3字符
				std::string::size_type spos = _gamebackurl.find_last_of("&");
				if (spos != std::string::npos)
				{
					string gameidstr = _gamebackurl.substr(spos);
					LOG_INFO << "===find_last_of pos[" << spos << "],gameidstr[" << gameidstr << "]";
					// 把gameid=3替换成gameid=0
					if (gameidstr.compare("&gameid=3") == 0)
					{
						_gamebackurl.replace(spos + 8, 1, "0");
						LOG_INFO << "==========发生了替换========";
						LOG_INFO << _gamebackurl;
					}
				}

				backurl = URL::Encode(_gamebackurl);
				req_forwardUrl = req_forwardUrl + "&burl=" + backurl;
				
				// LOG_DEBUG << "_gamebackurl[" << _gamebackurl <<"]\n req_forwardUrl[" << req_forwardUrl<<"]";
				LOG_DEBUG << "req_forwardUrl[" << req_forwardUrl<<"]";

				// 加密forwardUrl返回
				std::string tmpEncode,forwardUrl = req_forwardUrl;
				Crypto::AES_ECB_Encrypt(initcfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
				req_forwardUrl = URL::Encode(tmpEncode);
			
			} while (0);

			// 兼容试玩环境
			if (initcfg_.isDemo == 1 && req_code == (int32_t)eErrorCode::PermissionDenied)
			{
				req_code = (int32_t)eErrorCode::NoError;
				// 加密forwardUrl返回
				std::string tmpEncode,forwardUrl = initcfg_.DemoPlayUrl;
				Crypto::AES_ECB_Encrypt(initcfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
				req_forwardUrl = URL::Encode(tmpEncode);
			}

			// 请求时长（毫秒）
			elapsedtimd = muduo::timeDifference(muduo::Timestamp::now(), timestart)  * 1000;
			
			// 是否超时判断
			bool isTimeOut = int32_t(resp.elapsed * 1000) > initcfg_.Timeout;
			if (isTimeOut)
				req_msg = "req time out " + to_string(resp.elapsed)+ "," + req_msg;
 
			// 向TG发送信息返回码
			if( req_code != (int32_t)eErrorCode::NoError || isTimeOut ){

				auto it = errcode_info_.find(req_code);
				if (it != errcode_info_.end()) {
					req_msg = it->second ;//errcode_info_[req_code];
				}

			 	string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
				GlobalFunc::sendMessage2TgEx(getTgMsg(userid,lg_name,billNo,req_code,req_msg,elapsedtimd,strtmp),initcfg_.botToken,initcfg_.botChairId);
			}
			
			// 返回结果
			rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,req_code,userid,lg_name,elapsedtimd,req_msg,req_forwardUrl);

			// Log rspdata
			if( initcfg_.isLog < (int32_t)eLogLvl::LVL_3 )
				LOG_DEBUG << "---TransHandle----isdecrypt[" << initcfg_.isdecrypt << "],[" << rspdata << "]";

			// 解锁玩家分
			if( initcfg_.TestLock == 0 && islock)
				unlockUserScore(userid,1,req_code,req_msg);
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
	else if (req.path() == "/Recycle")
	{ //回收余额

		std::string rspdata, req_forwardUrl, resp_msg, lg_name = VERSION;
		int64_t recycleScore = NOT_NEED_UPDATE;
		try
		{
			// 参数
			bool islock = false;
			std::string req_msg = "ok";
			int32_t aid = 0, actype = 0, req_code = (int32_t)eErrorCode::NoError;
			int64_t balance = 0, userid = 0, score = 0, newbalance = 0;
			std::string da_account = "",  reqStr = req.query();
			cpr::Response resp;

			ReqParamCfg reqParam;
			do
			{
				// Check 维护状态
				if (getMaintenanceStatus(req_code, req_msg))
					break;

				// 0,解析请求参数并获取玩家帐号信息----------------------------------------------------------------------------
				// (1)检查参数
				req_code = checkReqParamsEx(reqStr, userid, aid, req_msg);
				if (req_code != (int32_t)eErrorCode::NoError)
					break;

				// 游戏暂未开放
				if (initcfg_.NotOpen == 1)
				{
					req_code = (int32_t)eErrorCode::NotOpen;
					break;
				}

				// (2)检查玩家是否天下汇，是则直接退出
				if (userid == 0)
				{
					req_code = (int32_t)eErrorCode::UserNotExistsError;
					break;
				}
  
				// (4)检查代理权限
				if (!isPermitTransfer(aid))
				{
					// 是否UID白名单
					if (initcfg_.WhiteUID.size() > 0)
					{
						auto result = std::find(initcfg_.WhiteUID.begin(), initcfg_.WhiteUID.end(), userid); //查找1
						if (result == initcfg_.WhiteUID.end())
						{ //找到
							req_code = (int32_t)eErrorCode::PermissionDenied;
							req_msg = "agent permission denied!";
							break;
						}
						else
						{
							LOG_WARN << "---" << __FUNCTION__ << " userid[" << userid << "] 为白名单访问用户。";
						}
					}
					else
					{
						req_code = (int32_t)eErrorCode::PermissionDenied;
						req_msg = "agent permission denied!";
						break;
					}
				}
				// 
				recycleBalanceEx(recycleScore,userid,req_code,req_msg);
				LOG_INFO << "recycleBalanceEx,userid:" << userid << ",aid:" << agent_info_.aid << ",recycleScore:" << recycleScore;

			} while (0);

			 
			// 请求时长（毫秒）
			elapsedtimd = muduo::timeDifference(muduo::Timestamp::now(), timestart) * 1000;

			// 是否超时判断
			bool isTimeOut = int32_t(resp.elapsed * 1000) > initcfg_.Timeout;
			if (isTimeOut)
				req_msg = "req time out " + to_string(resp.elapsed) + "," + req_msg;

			// 向TG发送信息返回码
			if (req_code != (int32_t)eErrorCode::NoError || isTimeOut)
			{
				auto it = errcode_info_.find(req_code);
				if (it != errcode_info_.end())
				{
					req_msg = it->second; //errcode_info_[req_code];
				}

				string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
				GlobalFunc::sendMessage2TgEx(getTgMsg(userid, lg_name, "00001", req_code, req_msg, elapsedtimd, strtmp), initcfg_.botToken, initcfg_.botChairId);
			}

			// 返回结果
			rspdata = createJsonRsp((int32_t)eReqType::OpTransfer, req_code, userid, lg_name, elapsedtimd, req_msg, recycleScore);

			// Log rspdata
			if (initcfg_.isLog < (int32_t)eLogLvl::LVL_3)
				LOG_DEBUG << "---Recycle----isdecrypt[" << initcfg_.isdecrypt << "],[" << rspdata << "]";

		}
		catch (exception excep)
		{
			resp_msg = "transfer exception";
			LOG_ERROR << "---" << __FUNCTION__ << " " << excep.what() << ",req.query[" << req.query() << "]";
			rspdata = createJsonRsp((int32_t)eReqType::OpTransfer, (int32_t)eErrorCode::InsideErrorOrNonExcutive, 0, lg_name, 0, resp_msg, recycleScore);
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
	if (initcfg_.isLog < (int32_t)eLogLvl::LVL_3)
	{
		LOG_WARN << "---处理耗时(毫秒)[" << elapsedtimd << "],MsgCount[" << nReqCount_ << "],OKCount[" << nReqOKCount_ << "],ErrCount[" << nReqErrCount_ << "]";
	}
}
 
//刷新所有agent_info信息 ///
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
/*
"configjson" : "{\n\t\"apiurl\":\"http://txhapi.txh-api.com:8899\",\n\t\"resolveurl\":\"https://detail.txh-api.com\",\n\t\"agent\":\"DTQP\",\n\t\"deskey\":\"P1LrKkzw\",\n\t\"md5key\":\"nIMCnN0R\"\n}",
*/
bool TransferServer::refreshAgentInfo()
{   
	LOG_WARN << "---加载代理信息---"<<__FUNCTION__;
	// agent_info_ = {0};
	agent_ids.clear();
	try
	{
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_platform_config"];
		mongocxx::cursor cursor = coll.find({});
		for (auto &doc : cursor)
		{
			LOG_INFO << "---QueryResult:" << bsoncxx::to_json(doc);
			int32_t status = doc["status"].get_int32();
			if (status == eDefaultStatus::op_ON)
			{
				string explanation = doc["explanation"].get_utf8().value.to_string();
				string configjson = doc["configjson"].get_utf8().value.to_string();
				if (configjson.empty())
				{
					LOG_ERROR << "---" << __FUNCTION__ << " 没配置好在字段[" << configjson << "]";
					continue;
				}
				
				agent_info_.status = status;
				agent_info_.configjson = configjson;
				agent_info_.PlatformName = doc["platformname"].get_utf8().value.to_string();
				agent_info_.PlatformCode = doc["platformcode"].get_utf8().value.to_string();

				if(agent_info_.PlatformCode.compare(COMPANY_CODE_WG) != 0)
				{
					continue;
				}

				// 取结果
				boost::property_tree::ptree req_pt;
				std::stringstream ss(configjson);
				read_json(ss, req_pt);

				agent_info_.ApiUrl = req_pt.get<string>("apiurl");
				agent_info_.DesCode = req_pt.get<string>("deskey");
				agent_info_.MD5Code = req_pt.get<string>("md5key");
				agent_info_.agent = req_pt.get<string>("agent");
				
				std::cout << "agent_explanation:" << explanation << std::endl;
				std::cout << "aid:" << agent_info_.aid << std::endl;
				std::cout << "MD5Code:" << agent_info_.MD5Code << std::endl;
				std::cout << "DesCode:" << agent_info_.DesCode << std::endl;
				std::cout << "ApiUrl:" << agent_info_.ApiUrl << std::endl;
				std::cout << "PlatformName:" << agent_info_.PlatformName << std::endl;
				std::cout << "PlatformCode:" << agent_info_.PlatformCode << std::endl;
				std::cout << "agent:" << agent_info_.agent << std::endl;
			}
		}
	}
	catch (bsoncxx::exception & e) {
		LOG_ERROR << "---MongoDBException: " << e.what();
		return false;
	}
	
	// 开启服务
	isStartServer = true;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "======================================服务启动完成 [" << ((initcfg_.isDemo == 1) ? "试玩" : "正式") << "]==============================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;

	return true;
} 

//=====config=====
bool TransferServer::LoadIniConfig()
{
    if(!boost::filesystem::exists("./conf/trans.conf")){
        LOG_ERROR << "./conf/trans.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/trans.conf", pt);

    IniConfig inicfg;
    inicfg.integralvalue 	= pt.get<int>("TransferServer.Integralvalue", 6);// 送积分数
    inicfg.changeRatevalue 	= pt.get<int>("TransferServer.ChangeRatevalue", 500);
    inicfg.session_timeout 	= pt.get<int>("TransferServer.SessionTimeout", 1000);
    inicfg.descode 			= pt.get<std::string>("TransferServer.Descode","111362EE140F157D");
    inicfg.apiadd 			= pt.get<std::string>("TransferServer.ApiAdd", "");
    inicfg.isDemo 			= pt.get<int>("TransferServer.IsDemo", 0);
    inicfg.TestLock 		= pt.get<int>("TransferServer.TestLock", 0);
    inicfg.isWhiteIp 		= pt.get<int>("TransferServer.IsWhiteIp", 1);
    inicfg.defaultScore 	= pt.get<int64_t>("TransferServer.DefaultScore", 10000);
    inicfg.ip_timeout 		= pt.get<int>("TransferServer.IpTimeout", 200);
    inicfg.paramCount 		= pt.get<int>("TransferServer.ParamCount", 5);
    inicfg.isdecrypt 		= pt.get<int>("TransferServer.Isdecrypt", 0);
    inicfg.isLog 			= pt.get<int>("TransferServer.IsLog", 0);
    inicfg.isLocal 			= pt.get<int>("TransferServer.IsLocal", 0);
    inicfg.agentId 			= pt.get<int>("TransferServer.AgentId", 10000);
    inicfg.maxConnCount 	= pt.get<int>("TransferServer.MaxConnCount", 2000);
    inicfg.maxConnFreqCount = pt.get<int>("TransferServer.MaxConnFreqCount",500);
    inicfg.maxGameid 		= pt.get<int>("TransferServer.MaxGameid", 10000);
    inicfg.maxLoginTypeid 	= pt.get<int>("TransferServer.MaxLoginTypeid", 4);
    inicfg.expireDate 		= pt.get<int>("TransferServer.ExpireDate", 4*60*60); //4小时过期时间 

    inicfg.AesKey 			= pt.get<std::string>("TransferServer.AesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.des_key 			= pt.get<std::string>("TransferServer.Deskey", "GSwydAem");
    inicfg.md5_key 			= pt.get<std::string>("TransferServer.Md5key", "4q8JXdwT");
    inicfg.reqUrl 			= pt.get<std::string>("TransferServer.ReqUrl", "https://api.txhbet.net");
    inicfg.resAddUrl 		= pt.get<std::string>("TransferServer.ResAdd", "doBusiness.do");
    inicfg.enterGameHandle 	= pt.get<std::string>("TransferServer.EnterGameHandle", "forwardGame.do");
    inicfg.AgentName 		= pt.get<std::string>("TransferServer.AgentName", "txwl");
    inicfg.Timeout 			= pt.get<int>("TransferServer.Timeout", 3000);
    inicfg.MethodName 		= pt.get<std::string>("TransferServer.Method", "lg");
    inicfg.Password 		= pt.get<std::string>("TransferServer.Password", "666888");
    inicfg.MoneyType 		= pt.get<std::string>("TransferServer.MoneyType", "CNY");
    inicfg.interval 		= pt.get<int>("TransferServer.interval", 10);		//两次查询间隔
    inicfg.IsMaster 		= pt.get<int>("TransferServer.IsMaster", 0);		//两次查询间隔
    inicfg.RecycleCycle 	= pt.get<int>("TransferServer.RecycleCycle", 60);	//两次查询间隔
    inicfg.Actype 			= pt.get<int>("TransferServer.Actype", 1);			//真钱帐号
    inicfg.ReadConfCycle 	= pt.get<int>("TransferServer.ReadConfCycle", 10);	//重加载配置时间(秒)
    inicfg.IsPrintConfLog 	= pt.get<int>("TransferServer.IsPrintConfLog", 1);	//重加载配置时间(秒)
    inicfg.AutoRecycle 		= pt.get<int>("TransferServer.AutoRecycle", 1);		//自动回收

    inicfg.botUrl 			= pt.get<std::string>("TransferServer.botUrl", "https://api.telegram.org/bot");
    inicfg.botToken 		= pt.get<std::string>("TransferServer.botToken", "1288332211:AAEpsGpE6XFNZTzNF4Ca2emhLLlvFMM7OSw");
    inicfg.botChairId 		= pt.get<int64_t>("TransferServer.botChairId", -437735957);
    inicfg.IsSendTgMsg 		= pt.get<int64_t>("TransferServer.IsSendTgMsg", 0);
    inicfg.NotOpen 			= pt.get<int64_t>("TransferServer.NotOpen", 0);
    inicfg.DemoPlayUrl 		= pt.get<std::string>("TransferServer.DemoPlayUrl", "http://tryplay.txh-demo.com/#/p/preload");//试玩环境地址
    inicfg.oddtype 			= pt.get<std::string>("TransferServer.oddtype", "A");

	
	inicfg.WhiteUID.clear();
	vector<string> uidVec;
	string temp = pt.get<std::string>("TransferServer.WhiteUID", "");
	if (!temp.empty()){
		boost::algorithm::split(uidVec, temp, boost::is_any_of(","));
		if(uidVec.size() > 0){
			int idxCount = 0;
			for(string uid : uidVec)
    		{
				inicfg.WhiteUID.push_back(atol(uid.c_str()));
				std::cout << "---WhiteUID[" << uid << "] --- " << ++idxCount<< std::endl;
			}
		}
	}

	initcfg_ = inicfg;
	if (inicfg.IsPrintConfLog == 1)
	{
		std::cout << "---resAddUrl " << inicfg.resAddUrl << " AesKey:" << inicfg.AesKey << " md5_key " << inicfg.md5_key <<" AutoRecycle " << inicfg.AutoRecycle << std::endl;
		std::cout << "---IsMaster " << inicfg.IsMaster << " AgentName:" << inicfg.AgentName << " " << inicfg.enterGameHandle << " isdecrypt " << inicfg.resAddUrl<< std::endl;
		std::cout << "---固定回收周期[" << initcfg_.RecycleCycle << "],重加载配置时间[" << initcfg_.ReadConfCycle <<"]"<< std::endl;
		std::cout << "---DemoPlayUrl[" << inicfg.DemoPlayUrl <<"],TestLock[" << inicfg.TestLock  << "],WhiteUID[" << initcfg_.WhiteUID.size() << "]"<< std::endl;
	}
	
	return true;
}

 // 判断该代理是否允许开放
// 返回true为允许开放
bool TransferServer::isPermitTransfer(int32_t _agentId)
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

// 创建订单
void TransferServer::createOrder(string & billNo,string & lgname,string type,int64_t credit,int64_t userid,string da_account,int64_t oldmoney,int64_t newmoney)
{
	LOG_WARN <<"---"<< __FUNCTION__ <<",创建订单---userid[" << userid << "],credit[" << credit << "],billNo[" << billNo << "]";
	try
	{
		// 加锁等待1秒
		// setNxDelay(userid);
		//IN 转入天下汇，转出大唐
		int32_t nScoreChangeType = (type.compare("IN") == 0) ? 0 : 1;

		// 1，写订单信息
		mongocxx::collection order_coll = MONGODBCLIENT["gamemain"]["txh_orders_info"];
		bsoncxx::document::value inset_value = bsoncxx::builder::stream::document{}
											   << "orderid" << billNo
											   << "userid" << bsoncxx::types::b_int64{ userid }
											   << "account" << da_account
											   << "txhaccount" << lgname
											   << "ordertype" << nScoreChangeType  
											   << "score" << credit //COIN_RATE
											   << "oldmoney" << oldmoney //COIN_RATE
											   << "newmoney" << newmoney //COIN_RATE
											   << "stauts" << (int32_t)eOrderType::OpDoing //订单状态
											   << "status_code" << 0				//订单操作返回码
											   << "starttime" << bsoncxx::types::b_date{chrono::system_clock::now()} //订单操作开始时间
											   << "endtime" << bsoncxx::types::b_date{std::chrono::seconds(0)}//订单操作结束时间
											   << "elapsedtime" << bsoncxx::types::b_double{0}//网络延时时间
											   << bsoncxx::builder::stream::finalize;
		if (!order_coll.insert_one(inset_value.view()))
			LOG_ERROR << "---创建订单失败,billNo[" << billNo << "],lgname["<< lgname << "],type["<< type << "],credit["<< credit << "]";
 
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",billNo[" << billNo << "],lgname["<< lgname << "],type["<< type << "],credit["<< credit << "]";
	}
}

// 更新订单状态
// @stauts 0,处理中;1,完成；2,超时；3,未知异常 
void TransferServer::updateOrder(int64_t &newScore,int64_t userid,string type,int64_t credit,string & billNo,int32_t stauts,int32_t status_code,double elapsedtime,int64_t oldmoney,int64_t newmoney)
{
	// LOG_INFO <<"---"<< __FUNCTION__ <<"---更新订单状态---";
	try
	{
		//IN 转入天下汇，转出大唐
		int32_t nScoreChangeType = (type.compare("IN") == 0) ? 0 : 1;

		// 1, 更新订单状态
		mongocxx::collection order_coll = MONGODBCLIENT["gamemain"]["txh_orders_info"]; 
		bsoncxx::document::value findValue = document{} << "orderid" << billNo << finalize;
		bsoncxx::document::value updateValue = document{}
											   << "$set" << open_document
											   << "oldmoney" << oldmoney //
											   << "newmoney" << newmoney //
											   << "stauts" << stauts
											   << "status_code" << status_code
											   << "endtime" << bsoncxx::types::b_date{chrono::system_clock::now()}
											   << "elapsedtime" << bsoncxx::types::b_double{elapsedtime}
											   << close_document << finalize;
		if (!order_coll.update_one(findValue.view(), updateValue.view()))
			LOG_ERROR << "---更新orderid信息失败:billNo[" << billNo << "],stauts["<< stauts << "],elapsedtime["<< elapsedtime << "]";


		// 对成功的操作才进行加减分，否则对方返回的结果中req_code值不为0
		if (stauts == (int32_t)eOrderType::OpFinish || stauts == (int32_t)eOrderType::OpJsonErr)
		{
			// 2,上下分操作 nScoreChangeType = 0 转出大唐
			int64_t opScore = (nScoreChangeType == 0) ? (-credit) : credit;

			// 3，更新game_user表玩家状态，修改玩家数
			bsoncxx::document::value findValue = document{} << "userid" << userid << finalize;
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
			bsoncxx::document::value updateValue = document{}
												<< "$set" << open_document
												<< "txhstatus" << bsoncxx::types::b_int32{1 - nScoreChangeType} //1 跳转到天下汇，0返回大唐
												<< close_document
												<< "$inc" << open_document
												<< "score" << opScore
												<< close_document
												<< finalize;

			// 设置需要返回修改之后的数据
			mongocxx::options::find_one_and_update options;
			options.return_document(mongocxx::options::return_document::k_after);
			auto updateView = user_coll.find_one_and_update(findValue.view(), updateValue.view(),options);
			if(!updateView){
				LOG_ERROR <<"---更新玩家分失败---";
				LOG_INFO << "---update document:"<< bsoncxx::to_json(findValue);
				LOG_INFO << "---update document:"<< bsoncxx::to_json(updateValue);
			}
			else{
				auto view = updateView->view();
				newScore 	= view["score"].get_int64();
				LOG_INFO << "---更新玩家登录状态 userid["<< userid << "] txhstatus["<< 1 - nScoreChangeType << "],type[" << type << "],newScore[" << newScore <<"]";
			}
		}

	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",billNo[" << billNo << "]";
	}
}

// 跳转记录，登录成功后则更新状态
void TransferServer::updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t gameid,int32_t gametype,int32_t online)
{
	LOG_INFO <<"---"<< __FUNCTION__ <<"---更新跳转记录状态---lgname[" << lgname << "],account[" << account << "],userid[" << userid << "],gameid[" << gameid << "],online[" << online << "]";
	try
	{
		 //update options
		mongocxx::options::update options = mongocxx::options::update{};
		// 1，跳转记录
		bsoncxx::document::value findValue = document{} << "userid" << userid << finalize;//玩家大唐用户ID
		mongocxx::collection playing_coll = MONGODBCLIENT["gamemain"]["txh_playing_info"];
		bsoncxx::document::value updateValue = document{}
											   << "$set" << open_document
											   << "account" << account	//玩家天下汇账号
											   << "txhaccount" << lgname	//玩家天下汇账号
											   << "score" << credit 		//带过去的分值，放大了100倍存储
											   << "stauts" << gameid			//状态，0已经返回大唐，1没有返回大唐
											   << "gametype" << gametype			//状态，0已经返回大唐，1没有返回大唐
											   << "stauts_online" << online			//状态，0已经返回大唐，1没有返回大唐
											   << "newmoney" << _newbalance	//跳转后天下汇余额
											   << "starttime" << (int64_t)time(nullptr)//跳转时间
											   << "starttimeISODate" << bsoncxx::types::b_date{chrono::system_clock::now()} //跳转时间
											   << close_document << finalize;
		// 没有记录内插入
		if(!playing_coll.update_one(findValue.view(), updateValue.view(),options.upsert(true)))
			LOG_ERROR << "---创建跳转记录,userid[" << userid << "],lgname["<< lgname << "],score["<< credit << "]";

	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",userid[" << userid << "],lgname["<< lgname << "],score["<< credit << "]";
	}
}
 
// 签名算法
cpr::Parameters TransferServer::createSignatureParam(string parameContent)
{
	std::string strDesOut;
	Crypto::DES_ECB_Encrypt((unsigned char *)agent_info_.DesCode.c_str(), (unsigned char *)"", parameContent, strDesOut);

	// 2,统一请求参数之key
	std::string strMd5, src = strDesOut + agent_info_.MD5Code;
	char md5[32 + 1] = {0};
	MD5Encode32(src.c_str(), src.length(), md5, 0);
	strMd5 = string((char *)md5);

	// 3,拼接参数
	auto param = cpr::Parameters{};
	param.AddParameter({"params", URL::Encode(strDesOut)});
	param.AddParameter({"key", strMd5.c_str()});

	return param;
}

// 回收余额
// @useridStr用户ID
void TransferServer::recycleBalance(int type ,string useridStr)
{	
	// 写加锁
	WRITE_LOCK(playing_list_mutex_);
	
    muduo::Timestamp timestart = muduo::Timestamp::now();
	playing_list_.clear();

	//查询txh playing info表
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1
														 << "stauts" << 1
														 << "score" << 1
														 << "account" << 1
														 << "gametype" << 1
														 << "starttime" << 1
														 << "txhaccount" << 1 << bsoncxx::builder::stream::finalize);
	// 统一查询
	bsoncxx::document::value query_value  = document{} <<  "gametype" << (int32_t)eThirdPartGameType::op_vedio << "stauts" << open_document << "$gte" << 1 << close_document << finalize;
	// 回收指定帐号
	if(type == (int32_t)eRecycleType::one && (!useridStr.empty())){
		// 
        query_value = document{} << "userid" << atol(useridStr.c_str()) << finalize;
		LOG_INFO << "---收到redis通知,回收指定帐号,useridStr[" << useridStr << "]";
	}

	mongocxx::collection playing_coll = MONGODBCLIENT["gamemain"]["txh_playing_info"];
	mongocxx::cursor cursor= playing_coll.find(query_value.view(), opts);
	try
	{
		for (auto &doc : cursor)
		{
			playing_list_t playing_ls_ = {0};
			playing_ls_.status_online = false;
			playing_ls_.userid 		= doc["userid"].get_int64();
			playing_ls_.score 		= doc["score"].get_int64();
			playing_ls_.lasttime 	= doc["starttime"].get_int64(); //上次查询时间
			playing_ls_.stauts 		= doc["stauts"].get_int32();
			playing_ls_.gametype 	= doc["gametype"].get_int32();
			playing_ls_.txhaccount 	= doc["txhaccount"].get_utf8().value.to_string();

			if(doc["account"])
				playing_ls_.account = doc["account"].get_utf8().value.to_string();

			playing_list_.push_back(playing_ls_);
			LOG_INFO << "用户ID[" << playing_ls_.userid << "],Account[" << playing_ls_.account << "],帐户[" << playing_ls_.txhaccount << "]";
		}
	}
	catch (const std::exception &e)
	{
		playing_list_.clear();
		std::cerr << __FUNCTION__ << " " << e.what() << '\n';
	}

	// 没有数据时则不打印
	if( playing_list_.size() == 0 ) {
		if(initcfg_.isDemo == 1) 
			std::cout << "---没有要回收的玩家,type[" << type << "],userid[" << useridStr << "]"<< std::endl;
		return;
	}

    LOG_INFO << "---加载玩家耗时(秒)[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) << "],用户数量[" << playing_list_.size() << "],type[" << type <<"]";

	std::cout << "=======================" << std::endl;
	std::cout << "=======回收余额========" << std::endl;
	std::cout << "=======================" << std::endl;

	int32_t recycleOK = 0;
	int64_t curTime = time(nullptr);
	bool bIsRecycle = false;
	// 
	// for (int32_t idx = 0; idx < playing_list_.size();idx++)
		// playing_list_t & item = playing_list_[idx];
	for (auto & item : playing_list_)
	{
		// 两次查询间隔有时间限制,批量查询的时间
		if(type == (int32_t)eRecycleType::all && (curTime - item.lasttime < initcfg_.interval)) {
			LOG_INFO << "---回收时间间隔太短---["<< initcfg_.interval <<",[" << curTime << " - " << item.lasttime << " = " << curTime - item.lasttime << "]";
			continue;
		}

		// 有回收操作
		bIsRecycle = true;

		// 签名算法
		auto param = createSignatureParam(balanceInquiryParams(item.txhaccount,true));

		// 请求串
		string _requrl = agent_info_.ApiUrl + "/" + initcfg_.resAddUrl;
		LOG_INFO << "---req string ---[" << _requrl << "?" << param.content << "]";

		// 查询余额回调
		auto balanceCB = [&](cpr::Response response) 
		{
			bool ret = false;
			int32_t req_code = 0;
			stringstream ss;
			do
			{
				// 查询成功
				if (200 != response.status_code)
					break;

				string errmsg;
				boost::property_tree::ptree req_pt;

				// 取结果码
				try
				{
					std::stringstream ss(response.text);
					read_json(ss, req_pt);
					errmsg 		= req_pt.get<string>("msg");
					req_code 	= req_pt.get<int32_t>("code");
					if (req_code != 0)
					{
						LOG_ERROR << "errmsg[" << errmsg << "],status_code[" << response.status_code << "],response.text[" << response.text << "]";
						break;
					}
					// 玩家在线状态
					item.status_online = req_pt.get<bool>("data.status");
				}
				catch (exception excep)
				{
					req_code = (int32_t)eErrorCode::GetJsonInfoError;
					LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << response.elapsed << "],status_code[" << response.status_code << "],response.text[" << response.text << "]";
					
					string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
					GlobalFunc::sendMessage2TgEx(strtmp + "\n" + to_string(response.status_code) + "\n" + response.text, initcfg_.botToken, initcfg_.botChairId);
					return response.status_code;
				}

				LOG_INFO << "---查询余额返回[" << response.text << "]";

				// 玩家在线也退出
				if (item.status_online == true){
					req_code = (int32_t)eErrorCode::PlayerOnLine;
					LOG_INFO << "---玩家当前在线,userid[" << item.userid << "],txhaccount[" << item.txhaccount << "],account[" << item.account << "]";
					break;
				}

				// 有可能有其它的错误返回
				if (req_code != (int32_t)eErrorCode::NoError )
					break;

				double fbalance = req_pt.get<double>("data.balance");
				string balanceStr = to_string(fbalance);
				int64_t balance = GlobalFunc::rate100(balanceStr);
				item.recycleScore = balance;

				LOG_WARN << "---查询余额结果[" << req_code << "],errmsg[" << errmsg << "],data.balance[" << fbalance << "],balance[" << balance << "],elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";

				// 转帐,分数大于0,且玩家不在线
				if ( balance > 0 && item.status_online == false )
				{
					int64_t newbalance = 0;
					int64_t newScore = 0; 
					// 创建订单号
					string billNo = GlobalFunc::getBillNo(item.userid);
					// 拉取余额
					cpr::Response resp_;
					if (transferBalance(newScore,item.txhaccount, billNo, TRANS_OUT, item.userid, balance, newbalance, req_code, errmsg, item.account, resp_))
					{
						// 更新跳转记录
						updatePalyingRecord(item.txhaccount,item.account,item.userid, balance, newbalance, 0,item.gametype,0);
					}
					else
					{
						// 请求超时打印，和发送消息
						if (resp_.elapsed * 1000 >= initcfg_.Timeout)
						{
							stringstream sstmp;
							sstmp << "拉取余额请求超时,userid[" << item.userid << "],billNo[" << billNo << "],elapsed[" << resp_.elapsed << "]";
							std::cout << "============================================" << std::endl;
							LOG_ERROR << sstmp.str();
							std::cout << "============================================" << std::endl;
							string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
							GlobalFunc::sendMessage2TgEx(strtmp + "\n" + sstmp.str(), initcfg_.botToken, initcfg_.botChairId);
						}
					}

					// 有可能有其它的错误返回
					if (req_code != (int32_t)eErrorCode::NoError )
						break;
				}
				else
				{
					// 更新玩家状态
					auto findValue = document{} << "userid" << item.userid << finalize;
					auto updateValue = document{} << "$set" << open_document << "stauts" << 0 << close_document << finalize;
					if (!playing_coll.find_one_and_update(findValue.view(), updateValue.view()))
					{
						LOG_ERROR << "---txh_playing_info find_one_and_update error,userid[" << item.userid << "]";
					}
					LOG_INFO << "---余额为0,无需转帐---userid[" << item.userid << "],txhaccount[" << item.txhaccount << "]";
				}
				
				// 执行完成 
				recycleOK++;
				ret = true;
			} while (0);

			//错误则发送消息
			if (!ret && item.status_online == false )
			{
				ss << "余额回收失败,状态码[" << response.status_code << "],耗时[" << response.elapsed << "],返回内容[" << response.text << "],用户ID[" << item.userid << "],帐号[" << item.txhaccount << "]";
				LOG_ERROR << "---" << ss.str();
				string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
				GlobalFunc::sendMessage2TgEx(strtmp + "\n" + ss.str(), initcfg_.botToken, initcfg_.botChairId);
			}

			// 返回状态
			return response.status_code;
		};

		//发送查询请求
		auto future =  cpr::GetCallback(balanceCB,cpr::Url{_requrl},cpr::Header{{"AgentName", agent_info_.agent}}, param);
		if(future.get() != 200){
			LOG_INFO << "---回收查询请求失败---userid[" <<  item.userid << "],txhaccount[" << item.txhaccount << "],status_code[" << future.get() << "]";
		}
	}

	// 拉取余额耗时统计
	int32_t icount = 0,allCount = playing_list_.size();
	auto elaspTime =  muduo::timeDifference(muduo::Timestamp::now(), timestart) ;
	stringstream ss;
	ss << "回收数量[" << allCount << "],成功数[" << recycleOK << "],耗时[" << elaspTime << "]\n";
	for (auto item : playing_list_)
	{
		ss << "uid[" << item.userid << "],lgname[" <<  item.txhaccount << "],score[" << item.recycleScore << "]";
		if(++icount < allCount)
			ss << std::endl;
	}
	
	// 固定周期回收时才发送
	if(initcfg_.IsSendTgMsg == 1 && type == (int32_t)eRecycleType::all && bIsRecycle ){
		string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
		GlobalFunc::sendMessage2TgEx(strtmp + "\n" + ss.str(), initcfg_.botToken, initcfg_.botChairId);
	}

	LOG_INFO << "---拉取余额耗时(秒)[" << elaspTime << "]," << ss.str();
	std::cout << "=======================" << std::endl;
	std::cout << "=======回收结束========" << std::endl;
	std::cout << "=======================" << std::endl;

}



// 回收余额
// @useridStr用户ID
int32_t TransferServer::recycleBalanceEx(int64_t & recycleScore ,int64_t userid, int32_t &reqCode, string &errmsg)
{	
	int32_t retCode = 0;

	recycleScore = NOT_NEED_UPDATE;
	// 写加锁
	WRITE_LOCK(playing_list_mutex_);
	
    muduo::Timestamp timestart = muduo::Timestamp::now();
	//查询txh playing info表
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1
														 << "stauts" << 1
														 << "score" << 1
														 << "account" << 1
														 << "gametype" << 1
														 << "starttime" << 1
														 << "txhaccount" << 1 << bsoncxx::builder::stream::finalize);
	// 统一查询
	bsoncxx::document::value query_value = document{} << "userid" << userid << "gametype" << (int32_t)eThirdPartGameType::op_vedio << finalize;
	// LOG_INFO << "---收到redis通知,回收指定帐号,userid[" << userid << "]";

	playing_list_t playing_ls_ = {0};
	mongocxx::collection playing_coll = MONGODBCLIENT["gamemain"]["txh_playing_info"];
	try
	{
		auto find_result = playing_coll.find_one(query_value.view(), opts);
		if (!find_result)
		{
			reqCode = (int32_t)eErrorCode::NotNeedUpdate;
			return retCode;
		}
		auto doc = find_result->view();
		playing_ls_.status_online = false;
		playing_ls_.userid = doc["userid"].get_int64();
		playing_ls_.score = doc["score"].get_int64();
		playing_ls_.lasttime = doc["starttime"].get_int64(); //上次查询时间
		playing_ls_.stauts = doc["stauts"].get_int32();
		playing_ls_.txhaccount = doc["txhaccount"].get_utf8().value.to_string();
		playing_ls_.gametype 	= doc["gametype"].get_int32();

		if (doc["account"])
			playing_ls_.account = doc["account"].get_utf8().value.to_string();

		LOG_INFO << "用户ID[" << playing_ls_.userid << "],Account[" << playing_ls_.account << "],帐户[" << playing_ls_.txhaccount << "]";
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " " << e.what() << '\n';
		reqCode = (int32_t)eErrorCode::InsideErrorOrNonExcutive;
		return retCode;
	}

	if(playing_ls_.stauts == eOnTxhGame::op_DT)
	{
		// 默认
		reqCode = (int32_t)eErrorCode::NotNeedUpdate;
		LOG_INFO << "用户ID[" << playing_ls_.userid << "],Account[" << playing_ls_.account << "],帐户[" << playing_ls_.txhaccount << "]没有在WG视讯";
		return retCode;
	}	

	std::cout << "=======回收余额========" << std::endl;
	int64_t curTime = time(nullptr);
	// 签名算法
	auto param = createSignatureParam(balanceInquiryParams(playing_ls_.txhaccount, true));

	// 请求串
	string _requrl = agent_info_.ApiUrl + "/" + initcfg_.resAddUrl;
	LOG_INFO << "---请求串---[" << _requrl << "?" << param.content << "]";

	// 查询余额回调
	auto balanceCB = [&](cpr::Response response) {
		bool ret = false;
		int32_t req_code = 0;
		stringstream ss;
		do
		{
			// 查询成功
			if (200 != response.status_code){
				req_code = (int32_t)eErrorCode::TimeOut;
				break;
			}

			string errmsg;
			boost::property_tree::ptree req_pt;

			// 取结果码
			try
			{
				std::stringstream ss(response.text);
				read_json(ss, req_pt);
				errmsg = req_pt.get<string>("msg");
				req_code = req_pt.get<int32_t>("code");
				if (req_code != 0)
				{
					LOG_ERROR << "errmsg[" << errmsg << "],status_code[" << response.status_code << "],response.text[" << response.text << "]";
					break;
				}
				// 玩家在线状态
				playing_ls_.status_online = req_pt.get<bool>("data.status");
			}
			catch (exception excep)
			{
				req_code = (int32_t)eErrorCode::GetJsonInfoError;
				LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << response.elapsed << "],status_code[" << response.status_code << "],response.text[" << response.text << "]";

				string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
				GlobalFunc::sendMessage2TgEx(strtmp + "\n" + to_string(response.status_code) + "\n" + response.text, initcfg_.botToken, initcfg_.botChairId);
				return response.status_code;
			}

			LOG_INFO << "---查询余额返回[" << response.text << "]";

			// 玩家在线也退出
			if (playing_ls_.status_online == true)
			{
				req_code = (int32_t)eErrorCode::PlayerOnLine;
				LOG_INFO << "---玩家当前在线,userid[" << playing_ls_.userid << "],txhaccount[" << playing_ls_.txhaccount << "],account[" << playing_ls_.account << "]";
				break;
			}

			// 有可能有其它的错误返回
			if (req_code != (int32_t)eErrorCode::NoError)
				break;

			double fbalance = req_pt.get<double>("data.balance");
			string balanceStr = to_string(fbalance);
			int64_t balance = GlobalFunc::rate100(balanceStr);
			playing_ls_.recycleScore = balance;

			LOG_WARN << "---查询余额结果[" << req_code << "],errmsg[" << errmsg << "],data.balance[" << fbalance << "],balance[" << balance << "],elapsed[" << response.elapsed << "],status_code[" << response.status_code << "]";

			// 转帐,分数大于0,且玩家不在线
			if (balance > 0 && playing_ls_.status_online == false)
			{
				int64_t newbalance = 0;
				int64_t newScore = 0;  
				// 创建订单号
				string billNo = GlobalFunc::getBillNo(playing_ls_.userid);
				// 拉取余额
				cpr::Response resp_;
				if (transferBalance(newScore,playing_ls_.txhaccount, billNo, TRANS_OUT, playing_ls_.userid, balance, newbalance, req_code, errmsg, playing_ls_.account, resp_))
				{
					// 取到更新后的玩家分数
					recycleScore = newScore;
					// 更新跳转记录
					updatePalyingRecord(playing_ls_.txhaccount, playing_ls_.account, playing_ls_.userid, balance, newbalance, 0,playing_ls_.gametype, 0);
				}
				else
				{
					// 请求超时打印，和发送消息
					if (resp_.elapsed * 1000 >= initcfg_.Timeout)
					{
						stringstream sstmp;
						sstmp << "拉取余额请求超时,userid[" << playing_ls_.userid << "],billNo[" << billNo << "],elapsed[" << resp_.elapsed << "]";
						std::cout << "============================================" << std::endl;
						LOG_ERROR << sstmp.str();
						std::cout << "============================================" << std::endl;
						string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
						GlobalFunc::sendMessage2TgEx(strtmp + "\n" + sstmp.str(), initcfg_.botToken, initcfg_.botChairId);
					}
				}

				// 有可能有其它的错误返回
				if (req_code != (int32_t)eErrorCode::NoError)
					break;
			}
			else
			{
				// req_code = (int32_t)eErrorCode::NotNeedUpdate;
				// 更新玩家状态
				auto findValue = document{} << "userid" << playing_ls_.userid << finalize;
				auto updateValue = document{} << "$set" << open_document << "stauts" << 0 << close_document << finalize;
				if (!playing_coll.update_one(findValue.view(), updateValue.view()))
				{
					LOG_ERROR << "---txh_playing_info find_one_and_update error,userid[" << playing_ls_.userid << "]";
				}
				LOG_INFO << "---余额为0,无需转帐---userid[" << playing_ls_.userid << "],txhaccount[" << playing_ls_.txhaccount << "]";
			}

			// 执行完成 
			ret = true;
			
		} while (0);

		//错误则发送消息
		if (!ret && playing_ls_.status_online == false)
		{
			ss << "余额回收失败,状态码[" << response.status_code << "],耗时[" << response.elapsed << "],返回内容[" << response.text << "],用户ID[" << playing_ls_.userid << "],帐号[" << playing_ls_.txhaccount << "]";
			LOG_ERROR << "---" << ss.str();
			string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
			GlobalFunc::sendMessage2TgEx(strtmp + "\n" + ss.str(), initcfg_.botToken, initcfg_.botChairId);
		}
		
		// 更新返回状态
		reqCode = req_code;

		// 返回状态
		return response.status_code;
	};

	//发送查询请求
	auto future = cpr::GetCallback(balanceCB, cpr::Url{_requrl}, cpr::Header{{"AgentName", agent_info_.agent}}, param);
	if (future.get() != 200)
	{
		LOG_INFO << "---回收查询请求失败---userid[" << playing_ls_.userid << "],txhaccount[" << playing_ls_.txhaccount << "],status_code[" << future.get() << "]";
	}

	// 拉取余额耗时统计
	auto elaspTime = muduo::timeDifference(muduo::Timestamp::now(), timestart);

	LOG_INFO << "---拉取余额耗时(秒)[" << elaspTime << "]";
	std::cout << "=======================" << std::endl;
	std::cout << "=======回收结束========" << std::endl;
	std::cout << "=======================" << std::endl;

	return retCode;
}

bool TransferServer::cacheThirdPartGame()
{
    try
    {
        MutexLockGuard lock(m_thirdpart_game_Lock);
        m_GameIdListMap.clear();
        // std::map<int32_t,vector<GameItemInfo>> GameItemInfoList;

        mongocxx::collection third_part_game_coll = MONGODBCLIENT["gameconfig"]["third_part_game_config"];
        mongocxx::cursor info = third_part_game_coll.find({});
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
            int32_t gametype = doc["gametype"].get_int32();
            if (status == eDefaultStatus::op_ON && gametype == eThirdPartGameType::op_vedio)
            {
                // 添加
                GameItemInfo gameItem;
                gameItem.repairstatus = doc["repairstatus"].get_int32();
                gameItem.gameid = doc["gameid"].get_int32();
                gameItem.gamecode = doc["gamecode"].get_utf8().value.to_string();
                gameItem.gamename = doc["gamename"].get_utf8().value.to_string();
                gameItem.companyNo = doc["companyNo"].get_int32();
                gameItem.companyCode = doc["companyCode"].get_utf8().value.to_string();
                gameItem.companyName = doc["companyName"].get_utf8().value.to_string();

				// 分ID进行装游戏项目
				m_GameIdListMap[gameItem.gameid] = gameItem;

                LOG_INFO << "---GameItemInfo[" << gameItem.companyNo << "],gameid[" << gameItem.gameid << "],gamename[" << gameItem.gamename << "],companyName[" << gameItem.companyName << "],companyCode[" << gameItem.companyCode << "]";
            }
        }
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}


// 加载Json文件
bool TransferServer::loadJson()
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

	// if (!boost::filesystem::exists("./conf/game_map.json"))
	// {
	// 	LOG_ERROR << "./conf/game_map.json not exists";
	// 	return false;
	// }
 
	// boost::property_tree::ptree root2;//创建一个结构对象    
    // boost::property_tree::ptree items2;
	// LOG_ERROR << "./conf/game_map.json not exists";
    // boost::property_tree::read_json<boost::property_tree::ptree>("./conf/game_map.json", root2);//读文件存放到root中
	// for (int32_t i = 101; i < 105; i++)
	// {
	// 	items2 = root2.get_child(to_string(i)));
	// 	for (auto it = items2.begin(); it != items2.end(); ++it)
	// 	{
	// 		GameCfg GameCfg_;
	// 		//遍历读出数据  
	// 		string key = it->first; //1011
	// 		// string name = it->second.get<string>("name");
	// 		// string code = it->second.get<string>("code");
	// 		// int32_t gameid = it->second.get<int>("id");
			
	// 		LOG_INFO << key;// << " " << name << " " << code << " " << gameid ;

	// 		// GameCfg_.gameid = gameid;
	// 		// GameCfg_.name = name;
	// 		// GameCfg_.code = code;
	// 		// // 存储错误码
	// 		// if(!key.empty()) 
	// 		// 	game_map_info_[key] = GameCfg_;
	// 	}
	// }
	

	return true;
}

/*
        // string ID = it->second.get<string>("ID");
        // string Name = it->second.get<string>("Name");
        // std::cout << key << " " << ID << " " << Name << std::endl;
*/
