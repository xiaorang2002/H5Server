#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>
#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/InetAddress.h>

#include <muduo/net/http/HttpContext.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>

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

#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp> 
#include <bsoncxx/builder/stream/array.hpp> 
#include <bsoncxx/builder/basic/kvp.hpp>

#include "json/json.h"

#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/GlobalFunc.h"
#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/RedisClient/RedisClient.h"
#include "public/TraceMsg/FormatCmdId.h"
#include "public/SubNetIP.h"


extern int g_bisDebug;

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "mymd5.h"
#include "base64.h"
#include "htmlcodec.h"
#include "crypto.h"
#include "urlcodec.h" 
// #include "aes.h"

// #ifdef __cplusplus
// }
// #endif

#include "FunClass.h"
#include "RegisteServer.h"

extern int g_EastOfUtc;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

// using namespace bsoncxx::builder::basic;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
//using bsoncxx::to_json;

// using namespace mongocxx;
using namespace bsoncxx::types;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array; 

std::map<int, RegisteServer::AccessCommandFunctor>  RegisteServer::m_functor_map;

// shared_ptr<CFunClass> m_FunClass;

RegisteServer::RegisteServer(
	muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrHttp,
        std::string const& strIpAddr,
	muduo::net::TcpServer::Option option)
		: httpServer_(loop, listenAddrHttp, "LoginHttpServer", option)
        , threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "TimerEventLoopThread"))
        , m_pIpFinder("qqwry.dat")
        , strIpAddr_(strIpAddr)
        , m_bInvaildServer(false)
        , isStartServer(false)
{
	// 
	 m_pattern= boost::regex("\n|\r"); 

	setHttpCallback(CALLBACK_2(RegisteServer::processHttpRequest, this));
	httpServer_.setConnectionCallback(CALLBACK_1(RegisteServer::onHttpConnection, this));
	httpServer_.setMessageCallback(CALLBACK_3(RegisteServer::onHttpMessage, this));
	
    //server_.setConnectionCallback(bind(&RegisteServer::onConnection, this, std::placeholders::_1));
    //server_.setMessageCallback(bind(&RegisteServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
	threadTimer_->startLoop();
	srand(time(nullptr));

	proxy_txh_oddtype_.clear();

	m_ActiveItemMap.clear();
    m_RegPoorMap.clear();

	// 操作对象
	// m_FunClass.reset(new CFunClass(this));
}

RegisteServer::~RegisteServer()
{
    m_functor_map.clear();
    quit();
}
 
void RegisteServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
    threadPool_.stop();
}

//MongoDB/redis与线程池关联 ///
void RegisteServer::threadInit()
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

bool RegisteServer::InitZookeeper(std::string ipaddr)
{
    LOG_INFO << __FUNCTION__ <<" ipaddr["<< ipaddr <<"]";
	m_zookeeperclient.reset(new ZookeeperClient(ipaddr));
    //指定zookeeper连接成功回调 ///
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&RegisteServer::ZookeeperConnectedHandler, this));
    //连接到zookeeper，成功后回调ZookeeperConnectedHandler ///
    if (!m_zookeeperclient->connectServer()) {
        LOG_ERROR << "InitZookeeper error";
        // abort();
    }

    LOG_INFO << "---InitZookeeper OK !";

	return true;
}

bool RegisteServer::InitRedisCluster(std::string ipaddr, std::string password)
{
	LOG_INFO << __FUNCTION__ <<" ipaddr["<< ipaddr <<"]password["<<password<<"]";
	m_redisPubSubClient.reset(new RedisClient());
	if (!m_redisPubSubClient->initRedisCluster(ipaddr, password)) {
		LOG_ERROR << "InitRedisCluster error,ipaddr[" << ipaddr <<"]";
		// abort();
	}
	m_redisPassword = password;
	//更新代码消息
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_proxy_info,CALLBACK_0(RegisteServer::refreshAgentInfo,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_game_ver,CALLBACK_0(RegisteServer::refreshGameVerInfo,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_login_server,CALLBACK_1(RegisteServer::repairLoginServer,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_white_list, CALLBACK_0(RegisteServer::refreshWhiteList, this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_user_white_list, CALLBACK_0(RegisteServer::loadUserWhiteList, this));
	// m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_temp_player_info, CALLBACK_0(RegisteServer::loadTempPlayerInfo, this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_load_login_server_cfg, CALLBACK_0(RegisteServer::LoadIniConfig, this));
	// 更新活动内容时
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_active_item_list,[&](string msg){ 
		MutexLockGuard lock(m_t_mutex);
        m_ActiveItemMap.clear();
        m_RegPoorMap.clear();
        LOG_ERROR << "-------------更新活动内容------------"<< msg;
    }); 
	
	// 
	m_redisPubSubClient->startSubThread();

	m_redisIPPort = ipaddr;

	return true;
}

bool RegisteServer::InitMongoDB(std::string url)
{
	LOG_INFO << __FUNCTION__ << "---DB url " << url;
    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}

//zookeeper连接成功回调 ///
void RegisteServer::ZookeeperConnectedHandler()
{
	LOG_INFO << __FUNCTION__ << "zookeeper连接成功回调";
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");
    //RegisteServers节点集合
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/RegisteServers"))
        m_zookeeperclient->createNode("/GAME/RegisteServers", "RegisteServers");
 
    //本地节点启动时自注册 ///
	{
        //指定网卡ip:port ///
		std::vector<std::string> vec;
		boost::algorithm::split(vec, httpServer_.ipPort(), boost::is_any_of(":"));
		
		m_szNodeValue = strIpAddr_ + ":" + vec[1];
		m_szNodePath = "/GAME/RegisteServers/" + m_szNodeValue;
		//自注册LoginServer节点到zookeeper
		if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
			m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
		}
		m_szInvalidNodePath = "/GAME/RegisteServersInvaild/" + m_szNodeValue;
	}  
}

//自注册服务节点到zookeeper ///
void RegisteServer::OnRepairServerNode()
{
	if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
		LOG_WARN << "--- *** " << m_szNodePath;
		m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true); 
	}  
}

//通知停止上下分操作处理 
// 维护登录服
void RegisteServer::repairLoginServer(string msg)
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
				LOG_WARN << "---repair RegisteServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else if ( m_szNodeValue.compare(ipaddr) == 0 )
			{
				m_bInvaildServer = (status == eUpholdType::op_stop);
				LOG_WARN << "---repair RegisteServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else
				LOG_ERROR << "---repair error,ipaddr[" << ipaddr << "],localIp[" << m_szNodeValue << "]";

			// if (status == eUpholdType::op_stop ){
			// 	//创建zookeeper维护节点
			// 	if (ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true)){
			// 	}
			// }
			
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

//刷新所有IP访问白名单信息 ///
//1.web后台更新白名单通知刷新
//2.游戏启动刷新一次
//3.redis广播通知刷新一次 ///
bool RegisteServer::refreshWhiteList() {

	if ( inicfg_.isWhiteIp == 0 ){
		LOG_WARN << "---不加载白名单---"<<__FUNCTION__;
		return true;
	}


	LOG_WARN << "---加载白名单---"<<__FUNCTION__;
	try
	{
		WRITE_LOCK(white_list_mutex_);
		white_list_.clear();
		//读取ip_white_list
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["ip_white_list"];
		//查询ip_white_list表，获取ipaddress，ipstatus
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "ipaddress" << 1 << "ipstatus" << 1 << "isapiuse" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::cursor cursor = coll.find({}, opts);
        int32_t icount = 0;
		
		for (auto& doc : cursor) {
			std::string ipaddr = doc["ipaddress"].get_utf8().value.to_string(); 
			//0允许访问 1禁止访问
			int32_t status = doc["ipstatus"].get_int32();
			bool isapiuse = doc["isapiuse"].get_bool();
			// LOG_INFO << "---ipaddr["<<ipaddr<<"],status["<<status<<"],isapiuse["<< isapiuse <<"]";
			//检查ipaddr有效性
			if (!ipaddr.empty() && status == eApiVisit::Enable && isapiuse) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				whiteIp_info_t& white_list = white_list_[addr.ipNetEndian()];
				white_list.status = status;
				white_list.isapiuse = isapiuse;
			}
		}

		LOG_WARN << "---IP访问白名单----" << white_list_.size();
	}
	catch (bsoncxx::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	/**/
	std::string ss;
	std::string format;
	for (std::map<in_addr_t, whiteIp_info_t>::const_iterator it = white_list_.begin();
		it != white_list_.end(); ++it) { 
		LOG_INFO << "---" << Inet2Ipstr(it->first) << " status "<< it->second.status << " "<< it->second.isapiuse;
	}
	
	return true;
}
 
//启动TCP业务线程 ///
//启动TCP监听 ///
void RegisteServer::start(int numThreads, int workerNumThreads)
{
	// 最晚一个启动的服务结束后开启变量 isStartServer = true
	threadTimer_->getLoop()->runEvery(5.0f, bind(&RegisteServer::OnRepairServerNode, this));
	threadTimer_->getLoop()->runAfter(4.0f, bind(&RegisteServer::refreshGameVerInfo, this));
	threadTimer_->getLoop()->runAfter(3.5f, bind(&RegisteServer::refreshAgentInfo, this));
	threadTimer_->getLoop()->runAfter(2.5f, bind(&RegisteServer::refreshWhiteList, this));
	// threadTimer_->getLoop()->runAfter(1.5f, bind(&RegisteServer::loadTempPlayerInfo, this));
	threadTimer_->getLoop()->runAfter(1.0f, bind(&RegisteServer::loadUserWhiteList, this)); 
}

//启动HTTP业务线程 ///
//启动HTTP监听 ///
void RegisteServer::startHttpServer(int numThreads, int workerNumThreads) {
	//处理HTTP回调
	setHttpCallback(CALLBACK_2(RegisteServer::processHttpRequest,this));

	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&RegisteServer::threadInit, this));

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
void RegisteServer::onHttpConnection(const muduo::net::TcpConnectionPtr& conn)
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
			string jsonResStr = createJsonString(0,eLoginErrorCode::GameHandleMaxConnError,"",0,"","maxConnCount:" + to_string(inicfg_.maxConnCount),"too many connections");
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
void RegisteServer::onHttpMessage(const muduo::net::TcpConnectionPtr& conn,
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
			string jsonResStr = createJsonString(0,eLoginErrorCode::GameHandleMaxFreqError,"",0,"",to_string(nConnFreqCount),"fuck max freq error");
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
            // std::string::size_type spos = peerIp.find_first_of(',');
            // if (spos != std::string::npos) {
            //     peerIp = peerIp.substr(0, spos);
            //     LOG_WARN << " >>> substr For ip " << peerIp <<" spos "<<spos;
            // }
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
				string jsonResStr = createJsonString(0,eLoginErrorCode::GameHandleWhiteIPError,"",0,"",peerIp,"IP禁止访问");
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
		threadPool_.run(std::bind(&RegisteServer::asyncHttpHandler, this, weakConn, context->request(),addr.ipNetEndian()));
		//释放资源
		//context->reset();
	}
}

//异步回调 ///
void RegisteServer::asyncHttpHandler(const muduo::net::TcpConnectionWeakPtr& weakConn, const muduo::net::HttpRequest& req,uint32_t _ipNetEndian)
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
				string jsonResStr = createJsonString(0,eLoginErrorCode::GameHandleHighFreqError,"",0,"","exists ipKey" + ipKey,"fuck req!");
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

//解析请求 ///
bool RegisteServer::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg)
{
	params.clear();
	try
	{
		LOG_DEBUG << "--- 解析字符串:\n" << queryStr;
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
 
//按照占位符来替换 ///
static void replace(string& json, const string& placeholder, const string& value) {
	boost::replace_all<string>(json, "\"" + placeholder + "\"", value);
}


// 构造返回结果
std::string RegisteServer::createJsonString(int32_t opType,int32_t errcode,string const& account, int64_t userid,string  nickname,string const& dataStr,string msg)
{
	boost::property_tree::ptree root,data;
	data.put("account", account);
	data.put("nickname", nickname);
	data.put("msg", msg);
	data.put("userid", ":userid");
	data.put("code", ":code");
	data.put("url", dataStr);
 	root.put("maintype", "/RegisteHandle");
	root.put("type", ":type");
	root.add_child("data", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	replace(json, ":type", std::to_string(opType));
	replace(json, ":userid", std::to_string(userid));
	replace(json, ":code", std::to_string(errcode));
	// replace(json, ":url", dataStr);
	boost::replace_all<std::string>(json, "\\", "");

	if(errcode == 0){
		nReqOKCount_ = numReqOKCount_.incrementAndGet();
	}
	else{
		nReqErrCount_ = numReqErrCount_.incrementAndGet();
	}
	 
	return json;
}

 
// 二次登录
std::string RegisteServer::RegisteToken(std::string const& reqStr) {
	
	if( inicfg_.isLog < (int32_t)eLogLvl::LVL_1 )
		LOG_INFO << __FUNCTION__ << "\nreqStr[" << reqStr<<"]";
	//解析参数
	stringstream ss; 
	int64_t timeElapse = 0;
	int32_t errcode = 0;
	string token_msg;
	string errmsg;
	HttpParams token_params;
	if (!parseQuery(reqStr, token_params, errmsg)){
		ss << "errcode=" << eLoginErrorCode::URLParamError;
		return ss.str();
	}

	//token
	HttpParams::const_iterator _tokenKey = token_params.find("token");
	if (_tokenKey == token_params.end() || _tokenKey->second.empty()){
		ss << "errcode=" << eLoginErrorCode::UrlTokenParamError;;
		return ss.str();
	}
	else{
		token_msg = _tokenKey->second;
	}

	LOG_INFO <<"---isdecrypt["<< inicfg_.isdecrypt <<  "]token_msg\n" << token_msg;

	// 解析token 参数
	string _agentid,_redisGuid,_descode,_urlDecode,_htmlDecode,decrptUrl,_token,_loginip;
	int64_t _timestamp = 0,uid = 0 ;

	try
	{
		// html解码
		_htmlDecode = HTML::HtmlDecode(token_msg);
		LOG_INFO << "\nhtmlDecode\n" << _htmlDecode;
		// url解码
		_urlDecode = boost::regex_replace(URL::MultipleDecode(_htmlDecode), m_pattern, "");
		LOG_INFO << "\nurlDecode\n" << _urlDecode << "\nAesKey " << inicfg_.secondTokenHandleAesKey;
		
		//对token第一次Aes解密
		decrptUrl = (inicfg_.isdecrypt == 0) ? reqStr : Crypto::AES_ECB_Decrypt(inicfg_.secondTokenHandleAesKey, _urlDecode);

		// 解密成功
		if ((inicfg_.isdecrypt == 0) || (decrptUrl.compare(token_msg) != 0))
		{
			LOG_INFO << "\ndecrptUrl " << decrptUrl;

			do
			{
				//解析参数
				HttpParams params;
				if (!parseQuery(decrptUrl, params, errmsg))
				{
					errcode = eLoginErrorCode::TokenHandleParamError;
					break;
				}

				//token
				HttpParams::const_iterator tokenKey = params.find("token");
				if (tokenKey == params.end() || tokenKey->second.empty())
				{
					errcode = eLoginErrorCode::TokenParamError;
					break;
				}
				else
				{
					_token = tokenKey->second;
				}

				LOG_INFO << "---_token " << _token;

				//agentid直接从参数中读出来，不用加密(前端已经解密)
				HttpParams::const_iterator agentidKey = params.find("aid");
				if (agentidKey == params.end() || agentidKey->second.empty() )// || !IsDigitStr(agentidKey->second))
				{
					errcode = eLoginErrorCode::TimestampParamError;
					break;
				}
				else{
					_agentid = agentidKey->second;
				}

				// 非空
				if( !_agentid.empty() )
				{
					if (inicfg_.isdecrypt == 1)
					{
						// 解密agentId
						string strAgentId = URL::MultipleDecode(_agentid);
						strAgentId = boost::regex_replace(strAgentId, m_pattern, "");
						//Aes解密
						_agentid = Crypto::AES_ECB_Decrypt(inicfg_.secondTokenHandleAesKey, strAgentId);
						LOG_INFO << "---encode agentId[" << strAgentId << "],agentid[" << _agentid << "]";
					}

					READ_LOCK(agent_info_mutex_);
					auto it = agent_info_.find((int32_t)atol(_agentid.c_str()));
					if (it != agent_info_.end()){
						_descode = it->second.descode;
					}
				}

				LOG_INFO << "---_agentid[" << _agentid <<"],_descode["<<_descode<<"]";

				// html解码
				string htmlDecode = HTML::HtmlDecode(_token);
				LOG_INFO << "\nhtmlDecode\n" << htmlDecode;

				string strURL = URL::MultipleDecode(htmlDecode);
				strURL = boost::regex_replace(strURL, m_pattern, "");
				LOG_INFO << "---strURL\n"  << strURL;
				 
				//进行二次Aes解密
				string decrptString = Crypto::AES_ECB_Decrypt(_descode, strURL);
				LOG_INFO << "---decrptString " << decrptString;

				// 写入用于帐号登录的session
				if ((decrptString.compare(_token) != 0))
				{ 
					//  {"userid":470925,"agentid":10000,"time":1586840115,"account":"text_979189481"} 
					std::stringstream ss(decrptString);
					boost::property_tree::ptree pt;
				
					try
					{
						read_json(ss, pt);
 	 
						_timestamp = pt.get<int64_t>("time");	
						timeElapse = time(nullptr) - _timestamp;
						LOG_INFO << "---timeElapse[" << timeElapse << "],time(nullptr)[" << time(nullptr) << "],timestamp[" << _timestamp << "],expireData["<<inicfg_.expireDate<<"]";

						uid = pt.get<int64_t>("userid");	
						
						// 有效时间内
						if(  timeElapse < inicfg_.expireDate )
						{
							_redisGuid = "ss_" + Crypto::generateUuid();
							REDISCLIENT.set(_redisGuid, decrptString);
							REDISCLIENT.resetExpired(_redisGuid, inicfg_.session_timeout);
							LOG_INFO << "---session create OK,timeElapse[" << timeElapse << "]";
						}
						else
							LOG_ERROR << "---session create ERROR,timeElapse to large[" << timeElapse << "]";
					}
					catch (const std::exception &e){
						errcode = eLoginErrorCode::TokenTryCatchError;
						LOG_ERROR << "---ptree_error read_json ERROR ";
					}


					// 记录二次登录请求
	    			if( uid > 0 ) recordUserLoginInfo(eRecordType::tokenhandle,uid,decrptString,_loginip);
					
				}
				else{
					errcode = eLoginErrorCode::TokenParamCompareError;
					LOG_ERROR << "---session create ERROR,errcode[" << errcode <<"]";
				}

			} while (0);
		} 
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " " << e.what();
		errcode = eLoginErrorCode::TokenTryCatchError;
	}

	// 拼接返回
	if (_redisGuid.empty())
		ss << "errcode="<< errcode <<"&timepara=" << _timestamp << "&timeelapse=" << timeElapse;
	else
		ss << "token=" << _redisGuid;
	
	_redisGuid = ss.str();
	// LOG_INFO << "_redisGuid:" << _redisGuid;

	return _redisGuid;
}

// ptree image_array = pt.get_child("images"); // get_child得到数组对象
// 遍历数组
// BOOST_FOREACH (boost::property_tree::ptree::value_type &v, image_array)
// {
// 	std::stringstream s;
// 	write_json(s, v.second);
// 	std::string image_item = s.str();
// }

// 时间差
// std::string now = muduo::Timestamp::now().toFormattedString();
// muduo::Timestamp start = muduo::Timestamp::now();
// LOG_INFO  << now << "  now ";
// LOG_INFO  << start.toString() << "  muduo::Timestamp ";
// LOG_INFO  << time(time(nullptr) << "  time(nullptr) ";
// muduo::Timestamp start_ = Timestamp(atol(_timestamp.c_str()));
// LOG_INFO  << start_.toString() << "  muduo::Timestamp_start_ ";


// 创建session json
std::string RegisteServer::GetSessionJson(int64_t userid,int32_t agentId,string & account,string & nickname)
{
	boost::property_tree::ptree root;
	root.put("userid", ":userid");
	root.put("agentid", ":agentid");
	root.put("time", ":time");
	root.put("account", account);
	root.put("nickname", nickname);
	
	stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	string session = s.str();
	GlobalFunc::replaceChar(session,'\n');
	replace(session, ":userid", std::to_string(userid));
	replace(session, ":agentid", std::to_string(agentId));
	replace(session, ":time",  std::to_string(time(nullptr)));
	return session;
}
 
//订单处理函数
std::string RegisteServer::LoginProcess(std::string const& reqStr) {
	int32_t opType = 0,agentId = 0,gameid = 0; 
	std::string account,nickname,linecode,regip,lastloginip,token_session;
	std::string gamebackurl,timestamp,paraValue,key,orderId;
	int64_t score = 0,uid = 0;
	int32_t headindex = 0;		//缺省 0
	int32_t logintype = 1;		//缺省 1
	int32_t isWhite = 0;		//是否白名单 0 否
	int32_t visitmode = (int32_t)eVisitMode::h5login;
	
	agent_info_t _agent_info = { 0 };
	//
	int32_t errcode = eLoginErrorCode::NoError;
	string errmsg;
	stringstream ss_query;
	do {
		
		// 挂了维护则可直接返回
		if( m_bInvaildServer ){
			errcode = eLoginErrorCode::ServerStopError;
			errmsg = "Server Stop Error";
			break;
		}
		
		//解析参数
		HttpParams params;
		if (!parseQuery(reqStr, params, errmsg)) {
			errcode = eLoginErrorCode::GameHandleOperationTypeError;
			errmsg = "bad request params "  + to_string((int)errcode);
			break;
		}
		if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) ss_query << "params.size[" << params.size() <<"]";
		// 判断解析参数数量
		if(  params.size() < inicfg_.paramCount ){
			errcode = eLoginErrorCode::GameHandleOperationTypeError;
			errmsg = "paramCount Error";
			LOG_ERROR << "---paramCount--" << inicfg_.paramCount <<" "<< params.size();
			break;
		}

		//type
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty() || !IsDigitStr(typeKey->second)
			|| (opType = atol(typeKey->second.c_str())) != int(eApiType::OpLogin)){
			errcode = eLoginErrorCode::GameHandleOperationTypeError; //操作类型参数错误
			errmsg = "type invalid " + to_string(opType);
			break;
		}
        if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) ss_query << "type[" << opType <<"]";
		//agentid
		HttpParams::const_iterator agentIdKey = params.find("agentid");
		if (agentIdKey == params.end() || agentIdKey->second.empty() || !IsDigitStr(agentIdKey->second)
			|| (agentId = atol(agentIdKey->second.c_str())) <= 0) {
			errcode = eLoginErrorCode::GameHandleParamsError;
			errmsg = "agentid invalid";
			break;
		}

		// agentId 测试写死
		// agentId = 10000;//111228;

        if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) ss_query << "agentid[" << agentId <<"]";
		//timestamp
		HttpParams::const_iterator timestampKey = params.find("timestamp");
		if (timestampKey == params.end() || timestampKey->second.empty() ||
			!IsDigitStr(timestampKey->second)) {
			errcode = eLoginErrorCode::GameHandleParamsError;
			errmsg = "timestamp invalid";
			break;
		}
		else {
			timestamp = timestampKey->second;
		}
		if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) ss_query << "timestamp[" << timestamp <<"]";

		//paraValue
		HttpParams::const_iterator paramValueKey = params.find("paraValue");
		if (paramValueKey == params.end() || paramValueKey->second.empty()) {
			errcode = eLoginErrorCode::GameHandleParamsError; // 传输参数错误
			errmsg = "paraValue invalid";
			break;
		}
		else {
			paraValue = paramValueKey->second;
		}
		//key
		HttpParams::const_iterator keyKey = params.find("key");
		if (keyKey == params.end() || keyKey->second.empty()) {
			errcode = eLoginErrorCode::GameHandleParamsError; // 传输参数错误
			errmsg = "key invalid";
			break;
		}
		else {
			key = keyKey->second;
		}
		if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) ss_query << "key[" << key <<"]";
		// 获取当前代理数据
		//agent_info_t _agent_info = { 0 };
		{
			READ_LOCK(agent_info_mutex_);
			std::map<int32_t, agent_info_t>::const_iterator it = agent_info_.find(agentId);
			if (it == agent_info_.end()) {
				errcode = eLoginErrorCode::GameHandleProxyIDError;
				errmsg = "agent_info not found";
				break;
			}
			else {
				_agent_info = it->second;
			}
		}
		// 没有找到代理，判断代理的禁用状态(0正常 1停用)
		if ( _agent_info.status == 1 ) {// 代理ID不存在或代理已停用 
			errcode = eLoginErrorCode::GameHandleProxyIDError;
			errmsg = "agent.status error";
			break;
		}

		//解码
		std::string decrypt;
		{
			//根据代理商信息中存储的md5密码，结合传输参数中的agentid和timestamp，生成判定标识key
			std::string src = to_string(agentId) + timestamp + _agent_info.md5code;
			if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1)ss_query << "md5[" << src <<"]";
			char md5[32 + 1] = { 0 };
			MD5Encode32(src.c_str(), src.length(), md5, 1);
			if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
				errcode = eLoginErrorCode::GameHandleProxyMD5CodeError;// 代理MD5校验码错误
				errmsg = "md5 check error";
				break;
			}

			string strHtml = HTML::HtmlDecode(paraValue);
			LOG_INFO << "---strHtmlDecode[" << strHtml<<"]";

			//paraValue UrlDecode 
			std::string strURL = URL::MultipleDecode(strHtml);
			LOG_INFO << "---strURLDecode[" << strURL<<"],descode[" << _agent_info.descode << "]";
			
			strURL = boost::regex_replace(strURL, m_pattern, "");

			//AES校验 
			decrypt = Crypto::AES_ECB_Decrypt(_agent_info.descode, strURL); 
			LOG_INFO <<  "---AES ECB Decrypt[" << decrypt<<"]"; 
			
			if ( decrypt.empty() ) {
				errcode = eLoginErrorCode::GameHandleProxyDESCodeError; // 参数转码或代理解密校验码错误
				errmsg = "DESDecrypt failed, AES_set_decrypt_key error";
				break;
			}
		}

		// 视讯默认盘口A
		string oddtype = inicfg_.oddtype;

		//解析参数 paraValue
		HttpParams decryptParams;
		if (!parseQuery(decrypt, decryptParams, errmsg)) {
			errcode = eLoginErrorCode::GameHandleParamsError;// 传输参数错误
			errmsg = "DESDecrypt ok, but bad request paramValue";
			break;
		}

		//APP 注册登录部分
		if((opType == 0) && LoginLogic(decryptParams,agentId,uid,errcode,account,nickname,token_session,errmsg) == false){
			LOG_ERROR << "---errcode[" << errcode << "],msg[" << errmsg << "]";
			break;
		}

	} while (0);

	if(inicfg_.isLog < (int32_t)eLogLvl::LVL_1) LOG_WARN << "---errcode["<< errcode <<"],ss_query{===="<< ss_query.str()<<"====}token_session["<< token_session << "]";

	// 构造返回参数
	std::string jsonRespStr;
	if (errcode == eLoginErrorCode::NoError)
	{
		createResponse(opType,errcode,agentId,isWhite,logintype,uid,_agent_info.descode,account,nickname,token_session,jsonRespStr);
		
		// 增加构造参数错误
		if( jsonRespStr.empty() ){
            errcode = (errcode == 0)? eLoginErrorCode::GameHandleUnKnowParamsError:errcode;
		}
	}
	
	// 构造参数不对或者其它错误
	if( errcode != eLoginErrorCode::NoError )
	{
		LOG_ERROR << "---create Response errcode--[" << errcode <<"],errmsg:" << errmsg;
		string datastr = errmsg;
		jsonRespStr = createJsonString(opType,errcode,account,uid,nickname,datastr,errmsg);
	}
	else{
	    recordUserLoginInfo(eRecordType::gamehandle,uid,token_session,lastloginip);
	}
	
	return jsonRespStr;
}

bool RegisteServer::LoginLogic(HttpParams params,int32_t agentId,int64_t & uid,int32_t & errcode,string & account,string & nickname,string & token_session,string & errmsg)
{
	int32_t opType = 0,gameid = 0; 
	std::string linecode,regip,lastloginip;
	int32_t headindex = 0;		//缺省 0
	int32_t logintype = 1;		//缺省 1
	int32_t isWhite = 0;		//是否白名单 0 否
	int32_t visitmode = (int32_t)eVisitMode::h5login;

	//VisitorMode
	HttpParams::const_iterator visitmodeKey = params.find("visitmode");
	if (visitmodeKey != params.end())
	{
		if (IsDigitStr(visitmodeKey->second))
		{
			visitmode = atol(visitmodeKey->second.c_str());
			if (visitmode < (int32_t)eVisitMode::visitor || visitmode > (int32_t)eVisitMode::account)
			{
				errcode = eLoginErrorCode::LoginHandleVisitModeError; // VisitorMode参数错误
				errmsg = "visitor mode error," + to_string(visitmode);
				return false;
			}
		}
	}

	if(visitmode == (int32_t)eVisitMode::h5login){
		errcode = eLoginErrorCode::LoginHandleVisitModeError; // VisitorMode参数错误
		errmsg = "visitor mode error," + to_string(visitmode);
		return false;
	}
	//
	LoginField _login_field = {0};

	// 要么是-1，要么是其它 
	_login_field.visitmode = visitmode;
	// DT APP new field
	_login_field.agentid = agentId;
	_login_field.gender = 1; //默认男性

	LOG_INFO << "---visitmode[" << visitmode << "],agentid[" << _login_field.agentid << "]";

	//userid
	HttpParams::const_iterator useridKey = params.find("userid");
	if (useridKey != params.end())
	{
		if (IsDigitStr(useridKey->second))
			_login_field.userid = atoll(useridKey->second.c_str());
	}
  
	// lastloginip
	HttpParams::const_iterator ipKey = params.find("ip");
	if (ipKey != params.end())
		_login_field.lastloginip = ipKey->second;

	//devSN
	HttpParams::const_iterator devSNKey = params.find("devSN");
	if (devSNKey != params.end()){
		if(devSNKey->second.empty()){
			int64_t nCurTickTime = time(nullptr);
			_login_field.devSN = to_string(nCurTickTime % 1000000);
		}
		else
			_login_field.devSN = devSNKey->second;
	}

	// 设置SN号
	_login_field.devSN = to_string(_login_field.agentid) + _login_field.devSN ;

	//devinfo
	HttpParams::const_iterator devinfoKey = params.find("devinfo");
	if (devinfoKey != params.end())
		_login_field.devinfo = devinfoKey->second;

	//sysver
	HttpParams::const_iterator sysKey = params.find("sysver");
	if (sysKey != params.end())
		_login_field.sysver = sysKey->second;

	//account
	HttpParams::const_iterator accountKey = params.find("account");
	if (accountKey != params.end() && (!accountKey->second.empty()))
		_login_field.account = accountKey->second;

	//mbtype
	HttpParams::const_iterator mbtypeKey = params.find("mbtype");
	if (mbtypeKey != params.end())
	{
		if (IsDigitStr(mbtypeKey->second))
			_login_field.mbtype = atoll(mbtypeKey->second.c_str());
	}

	// 推荐玩家号ID（代理上级）
	HttpParams::const_iterator superiorKey = params.find("superior");
	if (superiorKey != params.end() && (!superiorKey->second.empty()))
	{
		if (IsDigitStr(superiorKey->second))
			_login_field.superior = atoll(superiorKey->second.c_str());
	} 
	// 推荐玩家号ID（代理上级,快速登录手动输入值）
	HttpParams::const_iterator inviterKey = params.find("inviter");
	if (inviterKey != params.end() && (!inviterKey->second.empty()))
	{
		if (IsDigitStr(inviterKey->second))
			_login_field.inviter = atoll(inviterKey->second.c_str());
	} 
	
	//password
	HttpParams::const_iterator passwordKey = params.find("pwd");
	if (passwordKey != params.end())
	{
		_login_field.srcpwd = passwordKey->second;
		_login_field.password = passwordKey->second;
		string pwdMd5 = getMd5Code(_login_field.password);
		LOG_INFO << "---密码登录[" << _login_field.mobile << "],pwd[" << _login_field.password << "],pwdMd5[" << pwdMd5 << "]";
		_login_field.password = pwdMd5;
	}

	//mobile
	HttpParams::const_iterator mobileKey = params.find("mobile");
	if (mobileKey != params.end())
		_login_field.mobile = mobileKey->second;

	// 帐号登录
	if (_login_field.visitmode == (int32_t)eVisitMode::account)
	{
		//帐号登录检查（先验证是否注册过或者帐号是否存在）
		if(!accountLoginCheck(_login_field, errcode, errmsg)){
			LOG_INFO << "---帐号登录失败mobile[" << _login_field.mobile << "],pwd[" << _login_field.password << "],account[" << _login_field.account << "],errmsg[" << errmsg << "]";
			return false;
		}
	}
	// 手机登录
	else if (_login_field.visitmode == (int32_t)eVisitMode::phone)
	{
		//vcode
		HttpParams::const_iterator vcodeKey = params.find("vcode");
		if (vcodeKey != params.end())
			_login_field.vcode = vcodeKey->second;

		// 手机登录检查（先验证验证码和是否注册过，如果没绑定则不能登录）
		if(!phoneLoginCheck(_login_field, errcode, errmsg)){
			LOG_INFO << "---手机登录失败[" << _login_field.mobile << "],pwd[" << _login_field.password << "],errmsg[" << errmsg << "]";
			return false;
		}

		// 增加前缀
		string dbmobile = getDBaccount(_login_field.agentid,_login_field.mobile);
		// 使用手机号用帐号
		_login_field.account = dbmobile;
	}
	else if (_login_field.visitmode == (int32_t)eVisitMode::wechat) // 微信登录 
	{
		//weixinid (openid)
		HttpParams::const_iterator weixinidKey = params.find("wxid");
		if (weixinidKey != params.end())
			_login_field.weixinid = weixinidKey->second;

		//wxnickName
		HttpParams::const_iterator wxnickNameKey = params.find("wxnickName");
		if (wxnickNameKey != params.end())
			_login_field.wxnickName = wxnickNameKey->second;

		//headurl
		HttpParams::const_iterator headurlKey = params.find("headurl");
		if (headurlKey != params.end())
			_login_field.headurl = headurlKey->second;

		//province
		HttpParams::const_iterator provinceKey = params.find("province");
		if (provinceKey != params.end())
			_login_field.province = provinceKey->second;

		//gender
		HttpParams::const_iterator genderKey = params.find("gender");
		if (genderKey != params.end())
		{
			if (IsDigitStr(genderKey->second))
				_login_field.gender = atoll(genderKey->second.c_str());
		}

		//city
		HttpParams::const_iterator cityKey = params.find("city");
		if (cityKey != params.end())
			_login_field.city = cityKey->second;

		//country
		HttpParams::const_iterator countryKey = params.find("country");
		if (countryKey != params.end())
			_login_field.country = countryKey->second;

		//unionid
		HttpParams::const_iterator unionidKey = params.find("unionid");
		if (unionidKey != params.end())
			_login_field.unionid = unionidKey->second;		

		// 使用微信信息
		_login_field.account = _login_field.weixinid;
		_login_field.nickname = _login_field.wxnickName;
	}

	// 公共部分初始化
	_login_field.linecode = to_string(agentId) + "_1";
	_login_field.headindex = 0;
	_login_field.isWhite = 0;

	string session;
	//登录申请
	if (_login_field.visitmode == (int32_t)eVisitMode::visitor) // 游客模式
	{
		_login_field.mobile = "";
		errcode = ApplyLoginExVisitor(session, errmsg, uid, _login_field);
	}
	else if (_login_field.visitmode == (int32_t)eVisitMode::phone) // 手机登录模式
	{
		errcode = ApplyLoginExPhone(session, errmsg, uid, _login_field);
	}
	else if (_login_field.visitmode == (int32_t)eVisitMode::wechat) //微信模式
	{
		_login_field.mobile = "";
		errcode = ApplyLoginExWechat(session, errmsg, uid, _login_field);
	}
	else if (_login_field.visitmode == (int32_t)eVisitMode::account) //帐号快速登录模式
	{
		errcode = ApplyLoginExAccount(session, errmsg, uid, _login_field);
	}
	// 
	token_session = session;
	account = _login_field.account;
	nickname = _login_field.nickname;

	LOG_INFO << "---Apply Login end------errcode[" << errcode << "],agentid[" << _login_field.agentid << "],weixinid[" << _login_field.weixinid << "],session[" << session <<"]";

	return (errcode == (int32_t)eLoginErrorCode::NoError);
}

//帐号登录写db操作
int RegisteServer::ApplyLoginExAccount(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField)
{
	int32_t errcode = eLoginErrorCode::NoError;
	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;
	errmsg.clear();
	do
	{
		try
		{
			LOG_WARN << "---[帐号]visitmode[" << loginField.visitmode << "],mobile[" << loginField.mobile << "],devSN[" << loginField.devSN << "],userid[" << loginField.userid << "],superior[" << loginField.superior << "],account[" << loginField.account << "]";
			stringstream ss;
			// 查找方式
			bsoncxx::stdx::optional<bsoncxx::document::value> result;
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "userid" << 1 << "isguest" << 1 << "account" << 1 << "nickname" << 1 << "agentid" << 1 << "devSN" << 1 << "status" << 1 << finalize);
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
 
			// 帐号模式
			// 1登录，2,注册
			if (loginField.mbtype == (int32_t)eMbtype::login)
			{
				// 真正帐号增加代理前缀
				loginField.account = getDBaccount(loginField.agentid,loginField.account);

				// 绑定或者登录成功
				result = user_coll.find_one(document{} << "account" << loginField.account << "agentid" << loginField.agentid << finalize, opts); //
				if (result)
				{ 
					bsoncxx::document::view view = result->view();
					userid = view["userid"].get_int64();
					int32_t aid = view["agentid"].get_int32();
					int32_t status = view["status"].get_int32();
					if (view["isguest"])
						isguest = view["isguest"].get_int32();
					if (view["account"])
						loginField.account = view["account"].get_utf8().value.to_string();
					if (view["nickname"])
						loginField.nickname = view["nickname"].get_utf8().value.to_string();

					LOG_INFO << "---找到玩家---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "]";


					// 封停帐号禁止登录
					if (status == (int32_t)ePlayerStatus::op_NO)
					{
						errmsg = to_string(userid) + " is freeze.";
						errcode = eLoginErrorCode::KickOutGameUserOffLineUserAccountError;
						break;
					}
				}
			}
			else if (loginField.mbtype == (int32_t)eMbtype::reg)  //2,注册
			{
				LOG_INFO << "---创建新玩家 mobile[" << loginField.mobile << "],devSN[" << loginField.devSN << "],account[" << loginField.account << "],superior[" << loginField.superior << "],inviter[" << loginField.inviter << "]";
				if(loginField.inviter > 0)
				{
					auto opts2 = mongocxx::options::find{};
					opts2.projection(document{} << "userid" << 1 << finalize);
					// inviter 值有效则推荐玩家使用输入值
					if(user_coll.find_one(make_document(kvp("userid", loginField.inviter)),opts2))
					{
						loginField.superior = loginField.inviter;
					}
				}
				// 创建新玩家
				if (!createNewUser(userid, loginField, errcode, errmsg))
					break;
			}

			// 2,写入用于帐号登录的session
			token_session = GetSessionJson(userid, loginField.agentid, loginField.account, loginField.nickname);
			// 返回uid
			uid = userid;

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
				LOG_INFO << "---userid:" << userid << " token_session " << token_session << " isguest[" << isguest << "],nickname[" << loginField.nickname << "]";
		}
		catch (bsoncxx::exception &e)
		{
			std::stringstream ss;
			ss << "account " << loginField.account << " agentId " << loginField.agentid << " insert error,MongoDBexception: " << e.what();
			errmsg = ss.str();
			errcode = eLoginErrorCode::InsideErrorOrNonExcutive;
			break;
		}

		errmsg = "success"; //成功
	} while (0);

	//操作失败
	if (errcode != 0)
		LOG_ERROR << "--- *** 登录操作失败 errcode = " << errcode;

	return errcode;
}
//手机登录写db操作
int RegisteServer::ApplyLoginExPhone(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField)
{
	int32_t errcode = eLoginErrorCode::NoError;
	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;
	errmsg.clear();
	do
	{
		try
		{
			LOG_WARN << "---[手机]visitmode[" << loginField.visitmode << "],mobile[" << loginField.mobile << "],devSN[" << loginField.devSN << "],userid[" << loginField.userid << "],superior[" << loginField.superior << "]";
			stringstream ss;
			// 查找方式
			bsoncxx::stdx::optional<bsoncxx::document::value> result;
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "userid" << 1 << "isguest" << 1 << "account" << 1 << "nickname" << 1 << "agentid" << 1 << "devSN" << 1 << "status" << 1 << finalize);
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
 
			// 手机模式
			// 1登录，2,注册(绑定)，3忘记密码
			if ((loginField.mbtype == (int32_t)eMbtype::login) || (loginField.mbtype == (int32_t)eMbtype::getpwd))
			{
				result = user_coll.find_one(document{} << "mobile" << loginField.mobile << "agentid" << loginField.agentid << finalize, opts); //
			}
			else if (loginField.mbtype == (int32_t)eMbtype::reg)  //2,注册(绑定)
			{
				// 绑定当前玩家
				if( loginField.userid > 0 )
				{
					// 如果是此帐号已经被绑定，则修改设备号
					loginField.devSN = connectString(loginField.devSN ,loginField.userid,loginField.mobile);

					// 判断注册送金
					int64_t giveCoin = 0,integralvalue = 0;
					getRegRewardCoin(loginField.agentid,giveCoin,integralvalue);
					// 积分转换
					integralvalue = integralvalue * inicfg_.changeRatevalue * COIN_RATE;

					LOG_INFO << "---绑定当前玩家userid[" << loginField.userid << "],giveCoin[" << giveCoin <<"]";

					// 给当前游客绑定帐号/手机号/密码/绑定状态
					auto seq_updateValue = document{} << "$set" << open_document
										  << "mobile" << loginField.mobile  << "password" << loginField.password << "agentid" << loginField.agentid 
										  << "devSN" << loginField.devSN << "account" << loginField.mobile  << "isguest" << 1 << close_document 
										  << "$inc" << open_document << "score" << giveCoin << "integralvalue" << integralvalue << close_document  << finalize;

					// auto inc_doc = make_document(kvp("score", giveCoin));
					// auto set_doc = make_document(kvp("mobile", loginField.mobile), kvp("password", loginField.password),
					// 							 kvp("agentid", loginField.agentid), kvp("devSN", loginField.devSN),
					// 							 kvp("account", loginField.mobile), kvp("isguest", 1));
					// auto seq_updateValue = make_document(kvp("$inc", inc_doc.view(),kvp("$set", set_doc.view())));

					// 设置需要返回修改之后的数据
					mongocxx::options::find_one_and_update options;
            		options.return_document(mongocxx::options::return_document::k_after);

					// 玩家为游客
					result = user_coll.find_one_and_update(document{} << "userid" << loginField.userid << "isguest" << 0 << finalize, seq_updateValue.view(),options);
					if (!result) // 没有玩家信息
					{
						LOG_INFO << "---绑定当前玩家失败userid[" << loginField.userid << "],mobile[" << loginField.mobile << "]";
						errcode = eLoginErrorCode::LoginHandlePhoneBandError;
						errmsg = "band phnoe error,"+to_string(loginField.userid);
						break;
					}

					string linecode = result->view()["linecode"].get_utf8().value.to_string();
					int64_t beforeScore = result->view()["score"].get_int64();
					// 写帐变记录
					AddScoreChangeRecordToDB(loginField.userid,loginField.mobile,giveCoin,loginField.agentid,linecode,beforeScore,eUserScoreChangeType::op_reg_reward);
				}
				else
				{
					//注意 此逻辑块 result 值 为null

					// 防止传错参数
					if(loginField.userid < 0){
						errcode = eLoginErrorCode::GameHandleUserNotExistsError;
						errmsg = "user not exists error,"+to_string(loginField.userid);
						break;
					}

					LOG_INFO << "---create New User mobile[" << loginField.mobile << "],devSN[" << loginField.devSN << "],userid[" << loginField.userid << "],superior[" << loginField.superior << "]";
					// 创建新玩家 (此处 loginField.userid = 0)
					if(!createNewUser(userid,loginField,errcode,errmsg))
						break;
				}
			}
			
			// 绑定或者登录成功
			if (result) 
			{
				bsoncxx::document::view view = result->view();
				userid 		= view["userid"].get_int64();
				int32_t aid = view["agentid"].get_int32();
				int32_t status = view["status"].get_int32();
				if(view["isguest"]) isguest = view["isguest"].get_int32();
				if(view["account"]) loginField.account = view["account"].get_utf8().value.to_string();
				if(view["nickname"]) loginField.nickname = view["nickname"].get_utf8().value.to_string();
				LOG_INFO << "---找到玩家---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "]";

				// 封停帐号禁止登录
				if(status == (int32_t)ePlayerStatus::op_NO){
					errmsg = to_string(userid) + " is freeze." ;
					errcode = eLoginErrorCode::KickOutGameUserOffLineUserAccountError;
					break;
				}
			}

			// 2,写入用于帐号登录的session
			token_session = GetSessionJson(userid, loginField.agentid, loginField.account, loginField.nickname);
			// 返回uid
			uid = userid;

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
				LOG_INFO << "---userid:" << userid << " token_session " << token_session << " isguest[" << isguest << "],nickname[" << loginField.nickname << "]";
		}
		catch (bsoncxx::exception &e)
		{
			std::stringstream ss;
			ss << "account " << loginField.account << " agentId " << loginField.agentid << " insert error,MongoDBexception: " << e.what();
			errmsg = ss.str();
			errcode = eLoginErrorCode::InsideErrorOrNonExcutive;
			break;
		}

		errmsg = "success"; //成功
	} while (0);

	//操作失败
	if (errcode != 0)
		LOG_ERROR << "--- *** 登录操作失败 errcode = " << errcode;

	return errcode;
}

//游客登录写db操作
int RegisteServer::ApplyLoginExVisitor(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField)
{
	int32_t errcode = eLoginErrorCode::NoError;
	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;
	errmsg.clear();

	do
	{
		try
		{
			LOG_WARN << "---[游客]visitmode[" << loginField.visitmode << "],userid[" << loginField.userid << "],mobile[" << loginField.mobile << "],devSN[" << loginField.devSN << "]";
 
			stringstream ss;
			// 查找方式
			bsoncxx::stdx::optional<bsoncxx::document::value> result;
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "userid" << 1 << "isguest" << 1 << "account" << 1 << "nickname" << 1 << "agentid" << 1 << "devSN" << 1 << "status" << 1 <<finalize);
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];

			// 游客模式
			if(loginField.userid == 0)
			{
				result = user_coll.find_one(document{} << "devSN" << loginField.devSN << finalize, opts);
			}
			else
			{
				// 带用户ID
				result = user_coll.find_one(document{} << "userid" << loginField.userid << finalize, opts);
				if (!result)
				{
					errcode = eLoginErrorCode::GameHandleUserNotExistsError;
					ss << "big error,userid:" << loginField.userid;
					errmsg = ss.str();
					break;
				}
			}

			// 没有玩家信息
			if (!result)
			{
				if(!createNewUser(userid,loginField,errcode,errmsg))
					break;
			}
			else
			{
				// 取userid
				bsoncxx::document::view view = result->view();
				userid 		= view["userid"].get_int64();
				int32_t aid = view["agentid"].get_int32();
				int32_t status = view["status"].get_int32();
				if(view["isguest"]) isguest = view["isguest"].get_int32();
				if(view["account"]) loginField.account = view["account"].get_utf8().value.to_string();
				if(view["nickname"]) loginField.nickname = view["nickname"].get_utf8().value.to_string();

				// 封停帐号禁止登录
				if(status == (int32_t)ePlayerStatus::op_NO){
					errmsg = to_string(userid) + " is freeze." ;
					errcode = eLoginErrorCode::KickOutGameUserOffLineUserAccountError;
					break;
				}

				// 校验数据
				if(aid != loginField.agentid){
					errmsg = to_string(aid) + "!=" + to_string(loginField.agentid);
					errcode = eLoginErrorCode::GameHandleUserNotExistsError;
					break;
				}

				// 测试环境游客默认送分
				if (inicfg_.isDemo == 1 && inicfg_.defaultScore > 0)
				{
					auto find_doc = make_document(kvp("userid", userid));
					auto set_doc = make_document(kvp("score", inicfg_.defaultScore),kvp("safebox",  (int64_t)0));
					if (!user_coll.update_one(find_doc.view(), make_document(kvp("$set", set_doc.view()))))
					{
						LOG_ERROR << "--- *** 测试环境游客默认送分失败 userid = " << userid;
					}
				}

				LOG_INFO << "---找到玩家---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "]";
			}

			// 2,写入用于帐号登录的session
			token_session = GetSessionJson(userid, loginField.agentid, loginField.account, loginField.nickname);
			// 返回uid
			uid = userid;

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
				LOG_INFO << "---userid:" << userid << " token_session " << token_session << " isguest[" << isguest << "],nickname[" << loginField.nickname << "]";
		}
		catch (bsoncxx::exception &e)
		{
			std::stringstream ss;
			ss << "account " << loginField.account << " agentId " << loginField.agentid << " insert error,MongoDBexception: " << e.what();
			errmsg = ss.str();
			errcode = eLoginErrorCode::InsideErrorOrNonExcutive;
			break;
		}

		errmsg = "success"; //成功
	} while (0);

	//操作失败
	if (errcode != 0)
		LOG_ERROR << "--- *** 登录操作失败 errcode = " << errcode;

	return errcode;
}

//微信登录写db操作
int RegisteServer::ApplyLoginExWechat(string &token_session, std::string &errmsg,int64_t &uid,LoginField &loginField)
{
	int32_t errcode = eLoginErrorCode::NoError;
	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;
	errmsg.clear();
	do
	{
		try
		{
			LOG_WARN << "---[微信]visitmode[" << loginField.visitmode << "],nickname[" << loginField.nickname << "],weixinid[" << loginField.weixinid << "],devSN[" << loginField.devSN << "],userid[" << loginField.userid << "]";
			stringstream ss;
			// 查找方式
			bsoncxx::stdx::optional<bsoncxx::document::value> result;
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "userid" << 1 << "isguest" << 1 << "account" << 1 << "nickname" << 1 << "agentid" << 1 << "devSN" << 1 << "status" << 1 << finalize);
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];

			if(loginField.weixinid.empty()){
				errcode = eLoginErrorCode::GameHandleUnKnowParamsError;
				errmsg = "weixinid null";
				break;
			}

			// 微信模式
			result = user_coll.find_one(document{} << "weixinid" << loginField.weixinid << "agentid" << loginField.agentid << finalize, opts); 
			if (!result) // 没有玩家信息
			{
				if(loginField.userid == 0)
				{
					if (!createNewUser(userid, loginField, errcode, errmsg))
						break; //创建失败退出
				}
				else
				{
					auto seq_findValue = document{} << "userid" << loginField.userid << "isguest" << 0  << "agentid" << loginField.agentid << finalize;

					auto seq_updateValue = document{} << "$set" << open_document << "weixinid" << loginField.weixinid << "headurl" << loginField.headurl
										  << "account" << loginField.weixinid  << "nickname" << loginField.wxnickName << close_document << finalize;

					// 玩家为游客
            		// auto doc = coll.find_one_and_replace(criteria.view(), replacement.view(), options);
					
					// 设置需要返回修改之后的数据
					mongocxx::options::find_one_and_update options;
            		options.return_document(mongocxx::options::return_document::k_after);

					auto resultfind = user_coll.find_one_and_update(seq_findValue.view(), seq_updateValue.view(),options);
					if (!resultfind) // 没有玩家信息
					{
						if (!createNewUser(userid, loginField, errcode, errmsg))
							break; //创建失败退出
					}
					else
					{
						// 取玩家信息 
						bsoncxx::document::view view = resultfind->view();
						userid 			= view["userid"].get_int64();
						if(view["isguest"]) isguest = view["isguest"].get_int32();
						if(view["account"]) loginField.account = view["account"].get_utf8().value.to_string();
						if(view["nickname"]) loginField.nickname = view["nickname"].get_utf8().value.to_string();

						LOG_INFO << "---找到临时玩家更新微信信息---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "],agentid[" << loginField.agentid << "]";
					}
				}
			}
			else
			{
				// 取userid
				bsoncxx::document::view view = result->view();
				userid 		= view["userid"].get_int64();
				int32_t aid = view["agentid"].get_int32();
				int32_t status = view["status"].get_int32();
				if(view["isguest"]) isguest = view["isguest"].get_int32();
				if(view["account"]) loginField.account = view["account"].get_utf8().value.to_string();
				if(view["nickname"]) loginField.nickname = view["nickname"].get_utf8().value.to_string();
				LOG_INFO << "---找到玩家---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "]";

				// 封停帐号禁止登录
				if(status == (int32_t)ePlayerStatus::op_NO){
					errmsg = to_string(userid) + " is freeze." ;
					errcode = eLoginErrorCode::KickOutGameUserOffLineUserAccountError;
					break;
				}

			}

			LOG_INFO << "---玩家信息---userid[" << userid << "],isguest[" << isguest << "],account[" << loginField.account << "],nickname[" << loginField.nickname << "]";
			// 2,写入用于帐号登录的session
			token_session = GetSessionJson(userid, loginField.agentid, loginField.account, loginField.nickname);
			// 返回uid
			uid = userid;

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_2)
				LOG_INFO << "---userid:" << userid << " token_session " << token_session << " isguest[" << isguest << "],nickname[" << loginField.nickname << "]";
		}
		catch (bsoncxx::exception &e)
		{
			std::stringstream ss;
			ss << "account " << loginField.account << " agentId " << loginField.agentid << " insert error,MongoDBexception: " << e.what();
			errmsg = ss.str();
			errcode = eLoginErrorCode::InsideErrorOrNonExcutive;
			break;
		}

		errmsg = "success"; //成功
	} while (0);

	//操作失败
	if (errcode != 0)
		LOG_ERROR << "--- *** 登录操作失败 errcode = " << errcode;

	return errcode;
}

// 
bool RegisteServer::getRegRewardCoin(int32_t agentid,int64_t & regReWard,int64_t & reg_Integral)
{
	int64_t giveCoin = 0;
	// 低保送金等活动开关
	if (!cacheActiveItem(agentid))
		LOG_ERROR << "------agentid["<<agentid<<"]----活动开关配置有误----------";

	if (!cacheRegPoorItem(agentid))
		LOG_ERROR << "------agentid["<<agentid<<"]----低保送金等配置有误----------";

	// 初始化
	regReWard = 0;
	reg_Integral = 0;

	// 找到活动缓存信息
	auto it = m_ActiveItemMap.find(agentid);
	if (it != m_ActiveItemMap.end())
	{
		for (auto activeItem : it->second)
		{
			if (activeItem.type == (int32_t)eActiveType::op_reg_poor)
			{
				// 设置有注册送金大于0
				auto regit = m_RegPoorMap.find(agentid);
				if (regit != m_RegPoorMap.end())
				{
					regReWard = regit->second.rgtReward;
					reg_Integral = regit->second.rgtIntegral;
				}
				// 找到则返回
				return true;
			}
		}
	}

	return false;
}

// 插入玩家
bool RegisteServer::createNewUser(int64_t & userid ,LoginField &loginField,int32_t &errcode ,string &errmsg)
{
	try
	{
		std::string strMd5Account;
		stringstream ss;
		do
		{
			LOG_WARN << "---插入玩家信息userid[" << loginField.userid << "],devSN[" << loginField.devSN << "]";
			// 设置需要返回修改之后的数据
			mongocxx::options::find_one_and_update options;
			options.return_document(mongocxx::options::return_document::k_after);
			//查询sequence_userid表
			mongocxx::collection seq_user_coll = MONGODBCLIENT["gamemain"]["sequence_userid"];
			auto seq_findValue = document{} << "sequenceMark"
											<< "userid" << finalize;
			auto seq_updateValue = document{} << "$inc" << open_document
											<< "sequenceValue" << bsoncxx::types::b_int64{1} //默认增加1数值
											<< close_document << finalize;
			auto seq_result = seq_user_coll.find_one_and_update(seq_findValue.view(), seq_updateValue.view(),options);
			if (!seq_result)
			{
				errcode = eLoginErrorCode::DemoHandleIncUserIdError;
				ss << "sequenceValue error,errcode " << errcode << " inc userid error";
				errmsg = ss.str();
				LOG_ERROR << errmsg;
				break;
			}

			// 取值
			auto viewfind = seq_result->view();
			userid = viewfind["sequenceValue"].get_int64();
			//
			std::string src = loginField.account + to_string(loginField.agentid);
			if (loginField.visitmode == (int32_t)eVisitMode::visitor)
				src = loginField.devSN + to_string(loginField.agentid);

			// 随机帐号
			strMd5Account = GlobalFunc::getMd5Encode(src, 1) + GlobalFunc::getLast4No(userid);
			LOG_WARN << "----------find one and update OK-------------userid[" << userid << "],src[" << src << "],strMd5Account[" << strMd5Account << "]";

			// 是否绑定标志
			int32_t isguest = eGuestMode::guest, nrebate = 0;
			// 根据配置判断是否赠送积分
			int64_t giveCoin = 0,integralvalue = 0; 

			// 游客使用
			if (loginField.visitmode == (int32_t)eVisitMode::visitor)
			{
				loginField.userid = userid;
				loginField.account = strMd5Account;
				loginField.nickname = strMd5Account;
				
				// 测试环境游客默认送分
				if( inicfg_.isDemo  == 1 ){
					giveCoin = inicfg_.defaultScore;
				}
			}
			else if (loginField.visitmode == (int32_t)eVisitMode::phone)
			{
				loginField.nickname = loginField.mobile;//strMd5Account;
				// 如果是此帐号已经被绑定，则修改设备号
				loginField.devSN = connectString(loginField.devSN,userid,loginField.mobile);
				// 设置为已经绑定状态
				isguest = eGuestMode::formal;
				getRegRewardCoin(loginField.agentid,giveCoin,integralvalue);

				LOG_INFO << "---superior,userid[" << loginField.userid << "],superior[" << loginField.superior << "],strMd5Account[" << strMd5Account << "]，giveCoin[" << giveCoin <<"]";
			}
			else if (loginField.visitmode == (int32_t)eVisitMode::account)
			{ 
				loginField.nickname = loginField.account;
				// 真正帐号增加代理前缀
				loginField.account = getDBaccount(loginField.agentid,loginField.account);
				// 如果是此帐号已经被绑定，则修改设备号
				loginField.devSN = connectString(loginField.devSN,userid,loginField.mobile);
				// 设置为已经绑定状态
				isguest = eGuestMode::formal;
				getRegRewardCoin(loginField.agentid,giveCoin,integralvalue);

				LOG_INFO << "---superior,userid[" << loginField.userid << "],superior[" << loginField.superior << "],nickname[" << loginField.nickname << "]，giveCoin[" << giveCoin <<"]";
			}

			// 获取上级代理
			vector<int64_t> subagents;
			if(!getsubagents(subagents, loginField.superior)){
				loginField.superior = 0;
				LOG_INFO << "------获取上级代理完成[" << subagents.size() << "]";
			}

			// 通过代理获取默认佣金比率
			int32_t minrebaterate = 60;
			get_platform_proxy_config(loginField.agentid,1,minrebaterate);
			
			// 通过会员层级表获取默认的层级值
			int32_t level = 1;
			get_level_membership_type(loginField.agentid,level);




			//1, 创建用户，插入game_user表，新增用户
			bsoncxx::builder::stream::document builder{};
			builder << "userid" << b_int64{userid};							   //用户ID
			builder << "account" << loginField.account;						   //用户帐号
			builder << "agentid" << b_int32{loginField.agentid};			   //代理号
			builder << "linecode" << loginField.linecode;					   //站点编码
			builder << "nickname" << loginField.nickname;					   //用户昵称
			builder << "renamecount" << 0;					   					//用户昵称修改次数
			builder << "headindex" << loginField.headindex;					   //用户头像
			builder << "registertime" << b_date{chrono::system_clock::now()};  //用户注册时间
			builder << "regip" << loginField.lastloginip;					   //regip	 新玩家注册时，lastloginip作为注册IP
			builder << "lastlogintime" << b_date{chrono::system_clock::now()}; //登录时间
			builder << "lastloginip" << loginField.lastloginip;				   //登录IP
			builder << "activedays" << 0;
			builder << "keeplogindays" << 0;
			builder << "alladdscore" << b_int64{0};
			builder << "allsubscore" << b_int64{0};
			builder << "addscoretimes" << 0;
			builder << "subscoretimes" << 0;
			builder << "gamerevenue" << b_int64{0};
			builder << "winorlosescore" << b_int64{0};
			builder << "score" << b_int64{giveCoin}; //用户分数，注册是否送金
			builder << "status" << b_int32{0};
			builder << "onlinestatus" << b_int32{0};
			builder << "score_double" << b_double{0};
			builder << "alladdscore_double" << b_double{0};
			builder << "allsubscore_double" << b_double{0};
			builder << "gamerevenue_double" << b_double{0};
			builder << "winorlosescore_double" << b_double{0};
			builder << "gender" << loginField.gender;
			builder << "unionid" << loginField.unionid;															//用户统一标识。针对一个微信开放平台帐号下的应用，同一用户的 unionid 是唯一的。
			builder << "country" << loginField.country;															//国家，如中国为 CN
			builder << "city" << loginField.city;																//城市
			builder << "province" << loginField.province;														//省份
			builder << "details" << loginField.details;															//玩家后续扩展字段
			builder << "integralvalue" << b_int64{integralvalue * inicfg_.changeRatevalue * COIN_RATE}; //默认6积分
			builder << "integralvalue_double" << b_double{integralvalue};
			builder << "totalvalidbet" << b_int64{0};
			builder << "totalvalidbet_double" << b_double{0};
			builder << "lastneedbet" << b_int64{0};
			builder << "txhaccount" << strMd5Account;				  //分配唯一虚拟帐号
			builder << "txhstatus" << b_int32{0};	  //跳转天下汇方向,0没跳转出，非0已跳转出
			builder << "weixinid" << loginField.weixinid;			  //分配唯一虚拟帐号
			builder << "headurl" << loginField.headurl;				  //分配唯一虚拟帐号
			builder << "password" << loginField.password;			  //分配唯一虚拟帐号
			builder << "mobile" << loginField.mobile;				  //分配唯一虚拟帐号
			builder << "sysver" << loginField.sysver;				  //分配唯一虚拟帐号
			builder << "devSN" << loginField.devSN;					  //分配唯一虚拟帐号
			builder << "devinfo" << loginField.devinfo;				  //分配唯一虚拟帐号
			builder << "isguest" << b_int32{isguest} ;						  //玩家性质（默认为0游客；1为正式）loginField.visitmode == 1 ? 1 :
			builder << "safebox" << b_int64{0};						  //保险箱，默认为0
			builder << "invitcode" << to_string(userid);			  //邀请码
			builder << "superior" << b_int64{loginField.superior};	  //上级代理ID，默认为0
			builder << "freeze" << b_int64{0};						  //冻结分数，用于玩家兑换时，默认为0
			builder << "rebate" << b_int64{nrebate};				  //玩家佣金，默认为0
			builder << "alreadyuserebate" << b_int64{0};			  //玩家已经领取佣金，默认为0
			builder << "gradesweek" << b_int64{0};					  //本周业绩，默认为0
			builder << "gradesday" << b_int64{0};					  //今日业绩，默认为0
			builder << "minrebaterate" << b_int32{minrebaterate};	  //玩家佣金保底值（万份之几），默认为0
			builder << "basicwashbet" << b_int64{0};				  //提现最低流水要求，默认为0
			builder << "grandtotalbet" << b_int64{0};				  //上次提现打码量，默认为0
			builder << "level" << b_int32{level};					  //玩家层级，默认为0
			builder << "viplevel" << b_int32{0};					  //玩家等级，默认为0
			builder << "isdtapp" << b_int32{eDTapp::op_YES};		  //是否大唐app玩家，1 为大唐APP
			builder << "teamcount" << b_int32{0};					  //团队人数，默认为0 绑定成功+1
			builder << "subordinate" << b_int32{0};					  //下属人数，默认为0 绑定成功+1
			auto insert_value = builder << "subagents" << open_array; //上级代理ID,添加即添加维护(不能包含自己)
			if (loginField.superior > 0)
			{ 
				// 增加上级
				insert_value << b_int64{loginField.superior};
				// 还有上级的代理
				if (subagents.size() > 0)
				{
					for (auto agentItem : subagents){
						insert_value << b_int64{agentItem}; 
					}
				} 
			}
			insert_value << close_array;

        	LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(builder.view());

			//插入文档
			mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
			if (!user_coll.insert_one(builder.view()))
			{
				std::stringstream ss;
				ss << "account " << loginField.account << " agentId " << loginField.agentid << " insert error";
				errmsg = ss.str();
				errcode = eLoginErrorCode::GameUserHandleCreateUserError;
				LOG_ERROR << "---创建玩家失败----msg[" << errmsg << "]";
				break;
			}

			// 增加代理团队人数和下属人数统计
			if(loginField.superior > 0)
				incTeamCount(loginField.superior,true);

			// 创建邮件
			createEmailDefault(loginField.agentid, userid);

			// 写帐变记录
			if(giveCoin > 0)
				AddScoreChangeRecordToDB(userid, loginField.account, giveCoin, loginField.agentid, loginField.linecode, 0, eUserScoreChangeType::op_reg_reward);

			// 完成注册赠送
			if(integralvalue > 0){
				 // 积分要转换存储 积分 * 500 * 100 缩放
                integralvalue = integralvalue * inicfg_.changeRatevalue * COIN_RATE;
				JiFenChangeRecordToDB(userid,loginField.account,loginField.agentid,integralvalue,0,eJiFenChangeType::op_reg_reward,"完成注册赠送积分");
			}

			LOG_WARN << "---创建玩家----account[" << loginField.account << "],agentId[" << loginField.agentid << "],userid[" << userid << "],nickname[" << loginField.nickname << "]";

		} while (0);

	}
	catch(const std::exception& e)
	{
		LOG_ERROR << "---创建玩家失败----msg[" << errmsg << "],what[" << e.what() << "]";
		std::cerr << e.what() << '\n';
	}
	
	return (errcode == (int32_t)eLoginErrorCode::NoError);
}

// 增加代理团队人数和下属人数统计
void RegisteServer::incTeamCount(int64_t superior_uid,bool isUp)
{
	try
	{
		auto inc_doc = isUp ? make_document(kvp("teamcount", 1), kvp("subordinate", 1)) : make_document(kvp("teamcount", 1));
		mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
		auto update_doc = user_coll.find_one_and_update(make_document(kvp("userid", superior_uid)), make_document(kvp("$inc", inc_doc.view())));
		if (!update_doc)
		{
			LOG_ERROR << "---递归找不到玩家[" << superior_uid << "]";
			return;
		}

		auto update_view = update_doc->view();
		if (update_view["subagents"])
		{
			auto subagents_ = update_view["subagents"].get_array();
			for (auto &sub_uid : subagents_.value)
				incTeamCount(sub_uid.get_int64());
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 增加代理团队人数和下属人数统计失败----" << e.what();
	}
}

bool RegisteServer::get_level_membership_type(int32_t agent_id, int32_t & level)
{
	try
	{
		//1, 从用户表中查询出会员层级
		mongocxx::collection men_coll = MONGODBCLIENT["gamemain"]["membership_type"];
		auto find_doc_type = make_document(kvp("isDefault", 1), kvp("agentid", agent_id));
		auto find_type_result = men_coll.find_one(find_doc_type.view());
		if (!find_type_result)
		{
			level = 1;
			LOG_ERROR << "---" << __FUNCTION__ << " 从用户表中查询出会员层级失败,agent_id[" << agent_id << "]";
			return false;
		}
		// 1.1 取出结果
		auto view = find_type_result->view();
		int32_t status = view["status"].get_int32();
		level = view["orderNo"].get_int32();
		if (status != eDefaultStatus::op_ON)
		{
			level = 1;
			LOG_ERROR << "---" << __FUNCTION__ << " 代理没开放此功能[" << agent_id << "]";
			return false;
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 获取失败----" << e.what();
		return false;
	}

	return true;
}

//
bool RegisteServer::get_platform_proxy_config(int32_t agent_id,int32_t level,int32_t & brokerage)
{
	try
	{
		//1, 从用户表中查询出会员层级
		mongocxx::collection platform_proxy_coll = MONGODBCLIENT["gamemain"]["platform_proxy"];
		auto find_result = platform_proxy_coll.find_one(document{} << "agentid" << agent_id << "level" << level << finalize);
		if (!find_result)
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 找不到此代理信息[" << agent_id << "]";
			return false;
		}

		// 1.1 取出结果
		bsoncxx::document::view view = find_result->view();
		int32_t status = view["status"].get_int32();
		brokerage = view["brokerage"].get_int32();
		if (status == eDefaultStatus::op_OFF)
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 代理没开放此功能[" << agent_id << "]";
			return false;
		}
	}
	catch(const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 获取代理返佣失败----" << e.what();
		return false;
	}

	return true;
}

//
bool RegisteServer::get_platform_global_config(int32_t agent_id,int32_t type_id,string & details)
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
 
// 创建默认邮件
void RegisteServer::createEmailDefault(int32_t agentid,int64_t userid)
{
	try
	{
		string details;
		if (!get_platform_global_config(agentid,(int32_t)eAgentGlobalCfgType::op_cfg_mails,details))
		{
			LOG_ERROR << "---" << __FUNCTION__ << " 代理没开放此功能[" << agentid << "]";
			return;
		}

		string title, content, sender, createuser;

		// 取值
		boost::property_tree::ptree pt;
		std::stringstream ss(details);
		read_json(ss, pt);
		title = pt.get<string>("title");
		content = pt.get<string>("content");
		sender = pt.get<string>("sender");
		createuser = pt.get<string>("createuser");

		int64_t curStamp = chrono::system_clock::now().time_since_epoch().count() / 1000;

		bsoncxx::builder::stream::document builder{};
		builder << "agentid" << b_int32{agentid};						//代理号
		builder << "userid" << b_int64{userid};							//用户ID
		builder << "title" << title;									//regip	 新玩家注册时，lastloginip作为注册IP
		builder << "content" << content;								//regip	 新玩家注册时，lastloginip作为注册IP
		builder << "status" << b_int32{0};								//status 0未读 1 已读
		builder << "timestamp" << b_int64{curStamp};					// chrono::system_clock::now()
		builder << "sender" << sender;									//发送者
		builder << "createtime" << b_date{chrono::system_clock::now()}; //登录时间
		builder << "createuser" << createuser;							//登录IP
		builder << "sendtime" << b_date{chrono::system_clock::now()};	//登录时间
		builder << "viewtime" << b_date{std::chrono::seconds(0)};		//读取时间

		LOG_INFO << "---" << __FUNCTION__ << " curStamp [" << curStamp << "] insert_one Document:" << bsoncxx::to_json(builder);
		//插入文档
		mongocxx::collection user_email_coll = MONGODBCLIENT["gamemain"]["gameuser_mail"];
		if (!user_email_coll.insert_one(builder.view()))
			LOG_ERROR << "---玩家创建邮件失败,用户ID[" << userid << "]";
	}
	catch(const std::exception &e)
	{
		LOG_ERROR << __FUNCTION__ << " 玩家创建邮件失败----" << e.what();
	}
}

// 构造返回结构
void RegisteServer::createResponse(
	int32_t opType, 
	int32_t & errcode,
	int32_t agentId,
	int32_t isWhite,
	int32_t logintype,
	int64_t userid,
	string  const & descode,
	string const & account,
	string nickname,
	string const & token_session,
	string & jsonRespStr)
{
	// 游戏跳转url地址
	string mainURL = "";	// 官网地址
	string domainid = "";	// domainid
	string api_add = "";	// 前端访问地址 
	string domain_front = "";	// 前端访问地址 
	string domain_front_port = "";	// domain_front_port
	 
	string token = "";
	stringstream ss;
	do
	{
		// 执行成功，没有出错
		{
			READ_LOCK(agent_info_mutex_);
			map<int32_t, agent_info_t>::iterator it = agent_info_.find(agentId);
			if (it != agent_info_.end()) { 
				domainid = it->second.domainid;
			}
			else{
				LOG_ERROR << "---can't find  agentId[" << agentId <<"]";
				errcode = eLoginErrorCode::GameHandleProxyIDError;
				break;
			}
		}
	
		{
			READ_LOCK(game_domain_mutex_);
			std::map<string, game_domain_t>::iterator it = game_domain_.find(domainid);
			if (it != game_domain_.end()) { 
				api_add 			= it->second.domain_api;
				domain_front 		= it->second.domain_front;
				domain_front_port   = it->second.domain_front_port;
				mainURL  			= it->second.domain_game;
			}
			else{
				LOG_ERROR << "---can't find game_domain_t by domainid[" << domainid <<"]";
				errcode = eLoginErrorCode::GameHandleProxyDomainTypeError;
				break;
			}
		}

		LOG_INFO << "---mainURL["<< mainURL <<"],domainURL["<<domainid<<"]";
		if (domain_front.empty() || mainURL.empty()|| api_add.empty())
		{
			LOG_ERROR << "---mainURL or domainURL or api_add empty------";
			errcode = eLoginErrorCode::GameHandleProxyMainURLError;
			break;
		}

		LOG_INFO << "---Response---[" << descode <<"] token_session["<< token_session <<"]";
		if (descode.empty())
		{
			LOG_ERROR << "---descode empty------";
			errcode = eLoginErrorCode::GameHandleParamsError;
			break;
		}

		/*注册登录返回url */
		string strEncrypt;	// = AesCrypto::AES_ECBEncrypt( token_session,descode);
		Crypto::AES_ECB_Encrypt(descode,(unsigned char *)token_session.c_str(),strEncrypt);

		if (strEncrypt.empty()){
			LOG_ERROR << "---AES_ECB_Encrypt error,strEncrypt empty------";
			errcode = eLoginErrorCode::GameHandleProxyDESCodeError;
			break;
		}
 
		token =  URL::Encode(strEncrypt);
		LOG_INFO << "---token[" << token<< "]" << ",strEncrypt[" << strEncrypt<<"],AesKey[" << inicfg_.AesKey << "]";

		//  过滤api_add未尾的非法字符
		std::string::size_type spos = api_add.find_last_of('/');
		if (spos + 1 == api_add.length()) {
			api_add = api_add.substr(0, spos);
		}

		// 临时变量
		string strTempEncrypt; 
 
		string strAgentId = to_string(agentId);
		string ApiAdd 		= api_add + "/RegisteToken";
		string front_port 	= domain_front + ":" + domain_front_port;
		LOG_INFO << "---token 1 [" << front_port << "],ApiAdd[" << ApiAdd << "],agentId[" << strAgentId <<"]";

		// 如果要加密
		if (inicfg_.isdecrypt == 1)
		{
			// 加密代理
			Crypto::AES_ECB_Encrypt(inicfg_.AesKey, (unsigned char *)strAgentId.c_str(), strTempEncrypt);
			strAgentId = URL::Encode(strTempEncrypt);
			// 加密domain地址
			Crypto::AES_ECB_Encrypt(inicfg_.AesKey, (unsigned char *)front_port.c_str(), strTempEncrypt);
			front_port = URL::Encode(strTempEncrypt);
			// 加密apiadd地址
			Crypto::AES_ECB_Encrypt(inicfg_.AesKey, (unsigned char *)ApiAdd.c_str(), strTempEncrypt);
			ApiAdd = URL::Encode(strTempEncrypt);

			LOG_INFO << "---token 2 [" << front_port << "],ApiAdd[" << ApiAdd << "],agentId[" << strAgentId <<"]";
		}

		// 拼接结果 
		ss.clear();
		ss << mainURL << "?domain=" << front_port  << "&token=" << token << "&apiadd=" << ApiAdd << "&aid=" << strAgentId << "&gamebackurl=" << inicfg_.url_front
		   << "&versions=" << version_info_ << "&logintype=" << logintype;

	}while(0);

	if(errcode != 0){
		LOG_ERROR << "---createrespone error,errcode=[" << errcode<<"]";
		ss << "create respone error,errcode:"<<errcode;
	}

	//创建json格式应答消息
	string dataStr = ss.str(); 
	jsonRespStr = createJsonString(opType,errcode,account,userid,nickname,dataStr,"OK");
}

//处理HTTP回调
void RegisteServer::processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
{
	rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
	rsp.setStatusMessage("OK");
	rsp.setCloseConnection(true);//注意要指定connection状态
	rsp.addHeader("Server", "DTQP");
	rsp.addHeader("Access-Control-Allow-Origin", "*");
	rsp.addHeader("Cache-Control", "no-cache");

	// 服务启动没完成
	if( !isStartServer ){
		rsp.setContentType("text/html;charset=utf-8");
		std::string now = muduo::Timestamp::now().toFormattedString();
		rsp.setBody("<html><body>Service is starting!\n[" + now + "]</body></html>");
		return;
	}

	//本次请求开始时间戳(微秒)
	muduo::Timestamp timestart = muduo::Timestamp::now();

	string 	rspdata;
	if (req.path() == "/RegisteHandle") {
		// 参数
		std::string reqStr = req.query();
		std::string errmsg = "bad req str";
		std::string lineTypeStr;
		int32_t errcode = eLoginErrorCode::GameHandleParamsError;
		// 空判断
		bool  IsInValid = ( reqStr.length() <= 1 ) ? true : false;
		if( IsInValid ) { 
			errcode = eLoginErrorCode::NoError;
			errmsg = "req str is null " + reqStr;
			//
			if ( inicfg_.isDemo == 1 ){
				IsInValid = false;
			}
		}

		//解析参数
		HttpParams params;
		if ( !IsInValid && parseQuery(reqStr, params, errmsg) ) {

			if( inicfg_.isLog < (int32_t)eLogLvl::LVL_1 ){
				LOG_INFO << "--- RegisteHandle[" << reqStr << "],[" <<  params.size()  << "]";
			}

			// 正式环境
			rspdata = LoginProcess(reqStr);
		}
		else{ 
			rspdata = createJsonString(eApiType::OpUnknown,errcode,"",0,"","",errmsg);
		}
		
		if( inicfg_.isLog < (int32_t)eLogLvl::LVL_3 )
			LOG_WARN << "---RegisteHandle---------isDemo[" << inicfg_.isDemo << "],"<< lineTypeStr << "\n" <<rspdata;
		// 长度
        rsp.setContentType("application/json;charset=utf-8");
		rsp.setBody(rspdata);
	}
	else if (req.path() == "/RegisteToken") {
		rspdata = RegisteToken(req.query());
		if( inicfg_.isLog < (int32_t)eLogLvl::LVL_4 )LOG_WARN << "---Token--------" << "\n" << rspdata;
		rsp.setContentType("application/json;charset=utf-8");
		rsp.setBody(rspdata); 
	}
	else if (req.path() == "/Register") {
		// rspdata = Register(req.query());
		if( inicfg_.isLog < (int32_t)eLogLvl::LVL_4 )LOG_WARN << "---Register--------" << "\n" << rspdata;
		rsp.setContentType("application/json;charset=utf-8");
		rsp.setBody(rspdata); 
	}
	else if (req.path() == "/GetVCode") {
		// rspdata = GetVCode(req.query()); registered
		// if( inicfg_.isLog < (int32_t)eLogLvl::LVL_4 )LOG_WARN << "---GetVCode--------" << "\n" << rspdata;
		// rsp.setContentType("application/json;charset=utf-8");
		// rsp.setBody(rspdata); 
	}
	else if (req.path() == "/RepairStatus") { //维护状态
		rsp.setContentType("text/html;charset=utf-8");
		rsp.setBody("repair status:"+to_string((m_bInvaildServer)?1:0));
	} 
	else {
		rsp.setContentType("text/html;charset=utf-8");
		rsp.setBody("<html><head><title>httpServer</title></head>"
			"<body><h1>Not Found</h1>"
			"</body></html>");
		rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
	}

	// 统计
	if (inicfg_.isLog < (int32_t)eLogLvl::LVL_3)
	{
		muduo::Timestamp timenow = muduo::Timestamp::now();
		double timdiff = muduo::timeDifference(timenow, timestart);
		LOG_WARN << "---处理耗时(秒)[" << timdiff << "],MsgCount[" << nReqCount_ << "],OKCount[" << nReqOKCount_ << "],ErrCount[" << nReqErrCount_ << "]";
	}
}

//读取game_version
bool RegisteServer::refreshGameVerInfo()
{
	LOG_WARN << "---加载游戏版本";
	try
	{
		version_info_.clear();
		int32_t idx = 0;
		std::stringstream ss;
		ss.clear();
		//读取game_version
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_version"];
		mongocxx::cursor cursor = coll.find({});

		for (auto& doc : cursor){
			string verStr = doc["gameversionname"].get_utf8().value.to_string() ;
			if ( 0 == verStr.compare("hall") || 0 == verStr.compare("client")|| 0 == verStr.compare("update_url"))
			{
				ss << doc["gameversionname"].get_utf8().value.to_string() <<"-" << doc["gameversion"].get_utf8().value.to_string() << ",";
				idx++; 
			}
		}
		// 
		version_info_ = ss.str();
		
		// 移除最后一个逗号
		string::size_type nPos1 = std::string::npos;
		nPos1 = version_info_.find_last_of(",");
		int32_t strlen = version_info_.length();
		if((nPos1 + 1) == strlen){
			version_info_ = version_info_.substr(0,strlen - 1);
		}

		LOG_WARN << "---get game version end---[" << version_info_<<"]";
	}
	catch (bsoncxx::exception & e) {
		LOG_ERROR << "---get game version info MongoDBException: " << e.what();
		return false;
	}
	// 开启服务
	isStartServer = true;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "======================================服务启动完成 [" << ((inicfg_.isDemo == 1) ? "试玩" : "正式") << "]==============================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	return true;
}


//刷新所有agent_info信息 ///
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
bool RegisteServer::refreshAgentInfo()
{
	LOG_WARN << "---加载代理信息---"<<__FUNCTION__;
	WRITE_LOCK(agent_info_mutex_);
	agent_info_.clear();
	game_domain_.clear();
	try
	{
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "agentid" << 1 << "score" << 1 << "status" << 1 << "cooperationtype" << 1
															 << "descode" << 1 << "md5code" << 1 << "domainid" << 1 << "prefix" << 1
															 << bsoncxx::builder::stream::finalize);
		//读取match_switch
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["proxy_info"];
		mongocxx::cursor cursor = coll.find({},opts);
		for (auto& doc : cursor)
		{
			// LOG_ERROR << "---QueryResult:" << bsoncxx::to_json(doc);
			int32_t agentId = doc["agentid"].get_int32(); 
			int32_t _status = doc["status"].get_int32(); 

			//是否被禁用 0正常 1停用
			if( _status == 0 )
			{
				agent_info_t &_agent_info = agent_info_[agentId];
				_agent_info.score = doc["score"].get_int64();
				_agent_info.status = _status; //doc["status"].get_int32();
				_agent_info.cooperationtype = doc["cooperationtype"].get_int32();
				_agent_info.md5code = doc["md5code"].get_utf8().value.to_string();
				_agent_info.descode = doc["descode"].get_utf8().value.to_string();
				if( doc["domainid"] ){
					_agent_info.domainid = doc["domainid"].get_utf8().value.to_string();
				}
				// 代理前缀
				if( doc["prefix"] && doc["prefix"].type() != bsoncxx::type::k_null){
					_agent_info.prefix = doc["prefix"].get_utf8().value.to_string();
				}

				LOG_INFO << "--- proxy info agentid["<< agentId << "],md5code["<< _agent_info.md5code << "],descode["<< _agent_info.descode << "],prefix[" << _agent_info.prefix <<"]";
			}
		}

		LOG_WARN << "---load proxy info end---------------";
		// read game_domaint
		mongocxx::collection gd_coll = MONGODBCLIENT["gamemain"]["game_domain"];
		cursor = gd_coll.find({});
		for (auto& doc : cursor)
		{
			string _id = doc["_id"].get_oid().value.to_string();

			game_domain_t& _list_info = game_domain_[_id]; 
			_list_info.domain_front 	= doc["domain_front"].get_utf8().value.to_string();
			_list_info.domain_front_port= doc["domain_front_port"].get_utf8().value.to_string();
			_list_info.domain_api 		= doc["domain_api"].get_utf8().value.to_string();
			_list_info.domain_game 		= doc["domain_game"].get_utf8().value.to_string();
			_list_info.domainname 		= doc["domainname"].get_utf8().value.to_string();

			LOG_WARN << "---game_domaint,_id " << _id;
			LOG_WARN << "---game_domaint,domain_game " << _list_info.domain_game;
			LOG_WARN << "---game_domaint,domainname " << _list_info.domainname ;
		}
		LOG_WARN << "---load game domaint end---------------";
	}
	catch (bsoncxx::exception & e) {
		LOG_ERROR << "---MongoDBException: " << e.what();
		return false;
	}

	std::stringstream ss;
	std::string format;
	for (std::map<int32_t, agent_info_t>::const_iterator it = agent_info_.begin();
		it != agent_info_.end(); ++it) {
		ss.clear();
		ss << "\nagentId[" << it->second.agentId
			<< "] score[" << it->second.score
			<< "] domainid[" << it->second.domainid
			<< "] status[" << it->second.status
			<< "] md5code[" << it->second.md5code
			<< "] descode[" << it->second.descode 
			<< "] cooperationtype[" << it->second.cooperationtype << "]";
		format += ss.str().c_str();
	}
	LOG_WARN << "---agent_info --- " << format;

	format.clear();
	for (map<string, game_domain_t>::const_iterator it = game_domain_.begin();
		it != game_domain_.end(); ++it) {
		ss.clear();
		ss << "\ndomain_api[" << it->second.domain_api
			<< "] domain_front[" << it->second.domain_front
			<< "] domain_front_port[" << it->second.domain_front_port
			<< "] domain_game[" << it->second.domain_game
			<< "] domainname[" << it->second.domainname << "]";
		format += ss.str().c_str();
	}
	LOG_WARN << "---game_domain_ --- " << format;

	return true;
} 

//=====config=====
bool RegisteServer::LoadIniConfig()
{
	LOG_WARN <<"---"<< __FUNCTION__ ;

    if(!boost::filesystem::exists("./conf/registe.conf")){
        LOG_ERROR << "./conf/registe.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/registe.conf", pt);

    IniConfig inicfg;
    inicfg.integralvalue 	= pt.get<int>("RegisteServer.Integralvalue", 6);// 送积分数
    inicfg.changeRatevalue 	= pt.get<int>("RegisteServer.ChangeRatevalue", 500);
    inicfg.session_timeout 	= pt.get<int>("RegisteServer.SessionTimeout", 1000);
    inicfg.descode 			= pt.get<std::string>("RegisteServer.Descode","111362EE140F157D");
    inicfg.apiadd 			= pt.get<std::string>("RegisteServer.ApiAdd", "http://192.168.2.214/RegisteToken");
    inicfg.isDemo 			= pt.get<int>("RegisteServer.IsDemo", 0);
    inicfg.isWhiteIp 		= pt.get<int>("RegisteServer.IsWhiteIp", 1);
    inicfg.defaultScore 	= pt.get<int64_t>("RegisteServer.DefaultScore", 0);//游客默认送分
    inicfg.ip_timeout 		= pt.get<int>("RegisteServer.IpTimeout", 200);
    inicfg.paramCount 		= pt.get<int>("RegisteServer.ParamCount", 5);
    inicfg.isdecrypt 		= pt.get<int>("RegisteServer.Isdecrypt", 0);
    inicfg.isLog 			= pt.get<int>("RegisteServer.IsLog", 0);
    inicfg.isLocal 			= pt.get<int>("RegisteServer.IsLocal", 0);
    inicfg.agentId 			= pt.get<int>("RegisteServer.AgentId", 10000);
    inicfg.maxConnCount 	= pt.get<int>("RegisteServer.MaxConnCount", 2000);
    inicfg.maxConnFreqCount = pt.get<int>("RegisteServer.MaxConnFreqCount",500);
    inicfg.maxGameid 		= pt.get<int>("RegisteServer.MaxGameid", 10000);
    inicfg.maxLoginTypeid 	= pt.get<int>("RegisteServer.MaxLoginTypeid", 4);
    inicfg.expireDate 		= pt.get<int>("RegisteServer.ExpireDate", 4*60*60); //4小时过期时间
    inicfg.secondTokenHandleAesKey = pt.get<std::string>("RegisteServer.SecondTokenHandleAesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.AesKey 			= pt.get<std::string>("RegisteServer.AesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.oddtype 			= pt.get<std::string>("RegisteServer.oddtype", "A");
    inicfg.giveCoin 		= pt.get<int>("RegisteServer.GiveCoin", 0); //注册送金值
    inicfg.useTestData 		= pt.get<int>("RegisteServer.UseTestData", 1); //是否使用测试数据
    inicfg.testScore 		= pt.get<int64_t>("RegisteServer.TestScore", 1000000); //游客默认送1W分
    inicfg.url_front 		= pt.get<string>("RegisteServer.url_front", "https://jjvip.zbftq.com"); //游客默认送1W分
	
	
	inicfg_ = inicfg;
    LOG_WARN << "---Api Add "<< inicfg.apiadd <<" TokenHandleAesKey:"<<inicfg.secondTokenHandleAesKey <<" descode "<<inicfg.descode <<" giveCoin "<<inicfg.giveCoin;
    LOG_WARN << "---isDemo "<< inicfg.isDemo <<" isWhiteIp:"<<inicfg.isWhiteIp <<" "<< inicfg.ip_timeout << " isdecrypt " <<inicfg.isdecrypt << " oddtype " << inicfg.oddtype;
	return true;
}
 
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
void RegisteServer::loadUserWhiteList()
{
	LOG_WARN <<"---后台更新代理通知信息---"<<__FUNCTION__ ;
	try
	{
		//读取user_white_list
		vec_user_white_list_.clear(); 
		mongocxx::collection user_white_list_coll = MONGODBCLIENT["gamemain"]["user_white_list"];
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} <<"userid" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::cursor cursor = user_white_list_coll.find({}, opts);
		for (auto& doc : cursor)
		{
			int64_t usrId = doc["userid"].get_int64();
			vec_user_white_list_.push_back(usrId);
			LOG_INFO << "---user_white["<< usrId << "]"<< vec_user_white_list_.size();
		}
		LOG_WARN << "---load User White List end --- " << vec_user_white_list_.size();
	}
	catch (bsoncxx::exception & e) {
		LOG_ERROR << "---MongoDBException: " << e.what();
	}
} 
// 获取
bool RegisteServer::getsubagents(vector<int64_t> & subagents,int64_t superior)
{
	if(superior == 0) return false;
	// 查找方式
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(document{} << "subagents" << 1 << finalize);
	mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
	bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(document{} << "userid" << superior << finalize, opts);
	if (result)
	{
		bsoncxx::document::view view = result->view();
		// 上级代理
		auto t_subagents = view["subagents"].get_array();
		LOG_INFO << "-------subagents get_array--------";
		{
			MutexLockGuard lock(m_mutex_agents);
			subagents.clear();
			for (auto &userid : t_subagents.value)
				subagents.push_back(userid.get_int64());
		}
		return true;
	}

	return false;
}
 
/*
	1. 在配置文件中设置开关,开关的作用是判定当前环境是否开启试玩账号
	2. 获取客户端请求的IP地址(可用于防止同一IP频繁刷取试玩账号)
	3. 在配置文件中设置试玩账号的初始分值(每个玩家进入的时候初始分值,避免该试玩账号输完金币)
	4. 从数据集合(game_playuser)中随机抽取一个玩家账号,该批次账号的代理ID全部都是10000
	5. 构建玩家注册返回的信息,同玩家注册登录一致
	*/
// 记录玩家登录情况
// @recordType 0,记录gamehandletime;1,记录tokenhandletime
void RegisteServer::recordUserLoginInfo(eRecordType recordType, int64_t userid, string &tokensession, string &loginIp)
{
	std::string updatefield = (recordType == eRecordType::gamehandle) ? "gamehandletime" : "tokenhandletime";

	auto seq_findValue = document{} << "userid"
									<< userid << finalize;

	auto seq_updateValue = document{} << "$set" << open_document
									  << updatefield << bsoncxx::types::b_date{chrono::system_clock::now()}
									  << close_document << finalize;

	try
	{
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_login_check_list"];

		auto find_result = coll.find_one_and_update(seq_findValue.view(), seq_updateValue.view());
		if (!find_result)
		{
			LOG_INFO << "---find one and update failure,userid[" << userid << "],tokensession[" << tokensession << "],type[" << recordType << "]";

			// 如果是gamehandle请求才写入记录
			if (recordType == eRecordType::gamehandle)
			{
				// 创建用户，插入user_login_check_list表，新增用户
				bsoncxx::document::value insert_value = bsoncxx::builder::stream::document{}
														<< "userid" << bsoncxx::types::b_int64{userid}
														<< "session" << tokensession
														<< "gamehandletime" << bsoncxx::types::b_date{chrono::system_clock::now()}
														<< "tokenhandletime" << bsoncxx::types::b_date{std::chrono::seconds(0)}
														<< "loginip" << loginIp
														<< bsoncxx::builder::stream::finalize;
				bsoncxx::stdx::optional<mongocxx::result::insert_one> resultInsert = coll.insert_one(insert_value.view());
				//创建成功
				if (!resultInsert)
					LOG_ERROR << "---增加玩家登录情况记录失败---" << __FUNCTION__;
			}
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what();
	}
}

// 拼接前缀
string RegisteServer::getDBaccount(int32_t agentid,string account)
{
	string dbaccount = account;
	
	/*
	// 真正帐号增加代理前缀
	MutexLockGuard lock(m_mutex_proxy);
	auto it = agent_info_.find(agentid);
	if (it != agent_info_.end())
	{
		dbaccount = it->second.prefix + account;
		// LOG_INFO << "------代理[" << agentid << "],帐号[" << account << "],前缀[" << it->second.prefix << "],dbaccount[" << dbaccount << "]";
	}
	*/

	return dbaccount;
}
// 验证帐号是否存在
bool RegisteServer::accountLoginCheck(LoginField &loginfield, int32_t &reqcode, string &errmsg)
{
	// 0,非法判断
	if ((loginfield.account.empty() || loginfield.password.empty()) || ((loginfield.mbtype == eMbtype::reg) && loginfield.mobile.empty()))
	{
		LOG_ERROR << "对不起，参数错误";
		reqcode = eLoginErrorCode::LoginHandlePhoneOrPwdError;
		errmsg = "account or password is empty";
		return false; //false 终止往后走
	}

	// 注册成功则直接返回,走后面流程进行注册
	if (loginfield.mbtype == (int32_t)eMbtype::reg)
	{
		// 对简单密码处理
		if (IsDigit(loginfield.srcpwd) || IsLetter(loginfield.password))
		{
			LOG_ERROR << "对不起，密码需要字母与数字组合";
			reqcode = eLoginErrorCode::LoginHandlePwdNeedCharOrDigit;
			errmsg = "password is so simple";
			return false; //false 终止往后走
		}

		// 对简单帐号处理
		if (IsDigit(loginfield.account) || IsLetter(loginfield.account))
		{
			LOG_ERROR << "对不起，帐号需要字母与数字组合";
			reqcode = eLoginErrorCode::LoginHandleAccNeedCharOrDigit;
			errmsg = "account is so simple";
			return false; //false 终止往后走
		}
	}

	// 1,增加频率限制
	string key = REDIS_LOGIN_3S_CHECK + loginfield.account;
	if (REDISCLIENT.exists(key))
	{
		reqcode = eLoginErrorCode::LoginHandleHightFreq;
		errmsg = "slowly please";
		LOG_ERROR << "对不起，您登陆太频繁，请稍后再试. account=" << loginfield.account;
		return false; //false 终止往后走
	}
	else
	{
		REDISCLIENT.set(key, key, 3); //重置3秒
	}
	

	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;

	// 接帐号
	string dbaccount = getDBaccount(loginfield.agentid,loginfield.account);

	// 3, 查询game_user表验证绑定情况
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1 << "isguest" << 1 << "password" << 1 << "mobile" << 1 << bsoncxx::builder::stream::finalize);
	mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
	bsoncxx::document::value findValue = make_document(kvp("account", bsoncxx::types::b_regex{dbaccount,"i"}),kvp("agentid", loginfield.agentid));
	
	// auto cursor = collection.find(make_document( kvp("Data", bsoncxx::types::b_regex{"^14-09-2017"})));
	// bsoncxx::document::value findValue = make_document(make_document(kvp("account", make_document(kvp("$regex", dbaccount), kvp("$options", 'i')))), kvp("agentid", loginfield.agentid));
 	// bsoncxx::document::value findValue = make_document(   kvp("account",make_document(kvp("$regex":dbaccount),kvp("$options","i")))  );
	// auto findValue =   make_document(kvp("account",make_document(kvp("$regex",dbaccount),kvp("$options","i"))));
	LOG_DEBUG << bsoncxx::to_json(findValue.view());

	// 4，帐号有校性判断
	auto result = user_coll.find_one(findValue.view(), opts);
	if (!result)
	{
		// 注册成功则直接返回,走后面流程进行注册
		if (loginfield.mbtype == (int32_t)eMbtype::reg)
		{
			// 防止手机号被重复注册
			if (user_coll.find_one(document{} << "mobile" << loginfield.mobile << finalize, opts))
			{
				reqcode = eLoginErrorCode::LoginHandlePhoneRegisted;
				errmsg = "the mobile had been registed";
				LOG_ERROR << "对不起，手机号已经被注册." << loginfield.mobile;
				return false; //false 终止往后走
			}
			
			return true; //true 需要往后走
		}

		if (loginfield.mbtype == (int32_t)eMbtype::login)
		{
			reqcode = eLoginErrorCode::LoginHandlePhoneOrPwdError;
			errmsg = "the account error";
			LOG_ERROR << "对不起，帐号不正确." << loginfield.account;
			return false; //false 终止往后走
		}
	} 

	// 5，【注册】注册成功则直接返回,走后面流程进行注册
	if (loginfield.mbtype == (int32_t)eMbtype::reg)
	{
		reqcode = eLoginErrorCode::LoginHandleAccountRegisted;
		errmsg = "the account had been registed";
		LOG_ERROR << "对不起，帐号已经被注册." << loginfield.account;
		return false; //false 终止往后走
	}

	// 取isguest
	bsoncxx::document::view view = result->view();

	// 虚拟帐号
	userid = view["userid"].get_int64();
	isguest = view["isguest"].get_int32();
	string mobile = view["mobile"].get_utf8().value.to_string();
	string password = view["password"].get_utf8().value.to_string();

	LOG_INFO << __FUNCTION__ << " userid[" << userid << "],isguest[" << isguest << "],password[" << password << "],mobile[" << loginfield.mobile << "],account[" << loginfield.account << "],dbmobile[" << mobile << "]";

	// 试不玩环境增加兼容
	string pwdMd5;
	if (inicfg_.isDemo == 1)
	{
		pwdMd5 = getMd5Code(password);
		if ((pwdMd5.compare(loginfield.password) == 0))
			return true;
	}

	//1 正式且密码校检通过,密码一致判断
	if ((password.compare(loginfield.password) == 0))
		return true;

	reqcode = eLoginErrorCode::LoginHandlePhoneOrPwdError;
	errmsg = "the phone is no registered or pwd error," + loginfield.mobile;
	LOG_ERROR << __FUNCTION__ << " userid[" << userid << "],isguest[" << isguest << "],reqcode[" << reqcode << "]";
	return false; //false 终止往后走
}
 
// 增加手机号检索引
bool RegisteServer::phoneLoginCheck(LoginField &loginfield, int32_t &reqcode, string &errmsg)
{
	// 0,非法判断
	if (loginfield.mobile.empty())
	{
		LOG_ERROR << "对不起，您手机号为空";
		reqcode = eLoginErrorCode::LoginHandlePhoneIsEmpty;
		errmsg = "mobile phone is empty";
		return false; //false 终止往后走
	}

	// 
	if (loginfield.mbtype == (int32_t)eMbtype::reg)
	{
		// 对简单密码处理
		if (IsDigit(loginfield.srcpwd) || IsLetter(loginfield.srcpwd))
		{
			LOG_ERROR << "对不起，密码需要字母与数字组合";
			reqcode = eLoginErrorCode::LoginHandlePwdNeedCharOrDigit;
			errmsg = "password is so simple";
			return false; //false 终止往后走
		}
	}

	// 1,增加频率限制
	string key = REDIS_LOGIN_3S_CHECK + loginfield.mobile;
	if (REDISCLIENT.exists(key))
	{
		reqcode = eLoginErrorCode::LoginHandleHightFreq;
		errmsg = "slowly please";
		LOG_ERROR << "对不起，您登陆太频繁，请稍后再试. mobile=" << loginfield.mobile;
		return false; //false 终止往后走
	}
	else
	{
		REDISCLIENT.set(key, key, 3); //重置3秒
	}


	int64_t userid = 0;
	int32_t isguest = eGuestMode::guest;

	// 3，检查校检码
	if (loginfield.mbtype > (int32_t)eMbtype::login) //1登录，2,注册(绑定)，3忘记密码
	{
		string rs_VCode;
		string verifyCodeKey = "verify_" + to_string(loginfield.agentid) + ":" + loginfield.mobile;
		bool ret = REDISCLIENT.get(verifyCodeKey,rs_VCode);
		rs_VCode = boost::regex_replace(rs_VCode,boost::regex("\""),"");
		if(rs_VCode.empty())
			LOG_ERROR << "见鬼了，校检码不正确，居然为空,redis_VCode=" << rs_VCode;

		// 校检码
		LOG_INFO << "校检码 VCode=" << loginfield.vcode << ",redis_VCode=" << rs_VCode <<",ret=" << ret << ",n_rs_VCode=" << atol(rs_VCode.c_str());

		if (!ret || (rs_VCode.compare(loginfield.vcode) != 0))
		{
			reqcode = eLoginErrorCode::LoginHandleVCodeError;
			errmsg =  loginfield.vcode  + " is bad verify code";
			LOG_ERROR << "对不起，校检码不正确. VCode=" << loginfield.vcode << ",redis_VCode=" << rs_VCode;
			return false; //false 终止往后走
		}
	}

	// 4, 查询game_user表验证绑定情况
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1 << "isguest" << 1 << "password" << 1 << bsoncxx::builder::stream::finalize);
	mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
	bsoncxx::document::value findValue = document{} << "mobile" << loginfield.mobile << "agentid" << loginfield.agentid << finalize;

	// 5，【注册/绑定】注册则直接返回,走后面流程进行注册
	if (loginfield.mbtype == (int32_t)eMbtype::reg)
	{
		if (user_coll.find_one(findValue.view(), opts))
		{
			reqcode = eLoginErrorCode::LoginHandlePhoneRegisted;
			errmsg = "the phone had been registed";
			LOG_ERROR << "对不起，手机号已经被注册." << loginfield.mobile;
			return false; //false 终止往后走
		}

		return true; //true 需要往后走
	}

	// 6，【忘记密码】找回密码,直接更新密码
	if (loginfield.mbtype == (int32_t)eMbtype::getpwd)
	{
		// 设置需要返回修改之后的数据
		mongocxx::options::find_one_and_update options;
		options.return_document(mongocxx::options::return_document::k_after);

		auto seq_updateValue = document{} << "$set" << open_document << "password" << loginfield.password << close_document << finalize;
		auto findresult = user_coll.find_one_and_update(findValue.view(), seq_updateValue.view(),options);
		if (!findresult)
		{
			reqcode = eLoginErrorCode::LoginHandleFindPwdfailure;
			errmsg = "find pwd failure";
			LOG_ERROR << "---找回密码,直接更新密码失败. mobile=" << loginfield.mobile;
			return false;//false 终止往后走,统一返回
		}

		auto findView = findresult->view();
		loginfield.userid = findView["userid"].get_int64();
		loginfield.nickname = findView["nickname"].get_utf8().value.to_string();

		reqcode = eLoginErrorCode::NoError;
		errmsg = "find pwd ok";
		return true; //统一返回 
	}

	// 7，【登录】手机登录判断
	auto result = user_coll.find_one(findValue.view(), opts);
	if (result)
	{
		// 取isguest
		bsoncxx::document::view view = result->view();

		// 虚拟帐号 
		userid = view["userid"].get_int64();
		isguest = view["isguest"].get_int32();
		string password = view["password"].get_utf8().value.to_string();
		
		LOG_INFO << __FUNCTION__ << " userid[" << userid << "],isguest[" << isguest << "],password[" << password << "],mobile[" << loginfield.mobile << "]";

		// 需要重绑定
		if(isguest != eGuestMode::formal){
			reqcode = eLoginErrorCode::LoginHandlePhoneNoBand;
			errmsg = "the phone no band";
			LOG_ERROR << __FUNCTION__ << " userid[" << userid << "],isguest[" << isguest << "],mobile[" << loginfield.mobile << "],非绑定用户，请检查！";
			return false; //false 终止往后走
		} 

		// 试不玩环境增加兼容(前期试玩环境没有对密码加密)
		string pwdMd5;
		if(inicfg_.isDemo == 1)
		{
			pwdMd5 = getMd5Code(password);
			if ((pwdMd5.compare(loginfield.password) == 0))
				return true;
		}

		//1 正式且密码校检通过,密码一致判断
		if ((password.compare(loginfield.password) == 0))
			return true;
	}

	reqcode = eLoginErrorCode::LoginHandlePhoneOrPwdError;
	errmsg = "the phone is no registered or pwd error," + loginfield.mobile;
	LOG_INFO << __FUNCTION__ << " userid[" << userid << "],isguest[" << isguest << "],reqcode[" << reqcode << "]";
	return false; //false 终止往后走
}

bool RegisteServer::JiFenChangeRecordToDB(int64_t userId,string account,int32_t agentId,int64_t incrintegral,int32_t before_integral,eJiFenChangeType changetype,string reason)
{
    bool bSuccess = false;

    auto insertValue = bsoncxx::builder::stream::document{}
                       << "userid" << userId                                   //玩家ID
                       << "agentid" << agentId                                 //代理ID
                       << "account" << account                                 //玩家账号
                       << "changetype" << (int32_t)changetype                            //账变类型
                       << "changeamount" << incrintegral                       //账变额度signInfo.giftValue
                       << "beforechange" << before_integral                     //变前额度
                       << "afterchange" << b_int32{before_integral  + incrintegral}               //变后额度
                       << "logdate" << b_date{chrono::system_clock::now()}      //账变时间
                       << "changereason" << reason                              //账变原因
                       << bsoncxx::builder::stream::finalize;

    // LOG_DEBUG << "Insert Score Change Record:"<<bsoncxx::to_json(insert_value);

    try
    {
        mongocxx::collection integral_coll = MONGODBCLIENT["gamemain"]["user_integral_record"];
        if (!integral_coll.insert_one(insertValue.view()))
            LOG_INFO << "---账变记录失败,userId[" << userId << ",reason[" << reason << "]";
        else
             bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}
bool RegisteServer::AddScoreChangeRecordToDB(int64_t userId,string &account,int64_t addScore,int32_t agentId,string linecode,int64_t beforeScore,int32_t roomId)
{
    bool bSuccess = false;
 
    auto insert_value = bsoncxx::builder::stream::document{}
            << "userid" << userId
            << "account" << account
            << "agentid" << agentId
            << "linecode" << linecode
            << "changetype" << roomId
            << "changemoney" << addScore
            << "beforechange" << beforeScore
            << "afterchange" << beforeScore + addScore
            << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
            << bsoncxx::builder::stream::finalize;

    // LOG_DEBUG << "Insert Score Change Record:"<<bsoncxx::to_json(insert_value);

    try{
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(insert_value.view());
        if(_result)
            bSuccess = true; 
    }
    catch (const std::exception &e)
    { 
        LOG_ERROR << "================" << __FUNCTION__ << " " << e.what();
    }

    return bSuccess;
}

// 所有活动的配置
bool RegisteServer::cacheActiveItem(int32_t agentid)
{
    // 找到缓存信息
    if (m_ActiveItemMap.find(agentid) != m_ActiveItemMap.end())
        return true;

    try
    {
        vector<ActiveItem> activeIteminfo;
        mongocxx::collection activity_coll = MONGODBCLIENT["gamemain"]["platform_activity"];
        mongocxx::cursor info = activity_coll.find(document{} << "agentid" << agentid << finalize);
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
            if (status == eDefaultStatus::op_ON)
            {
                int32_t sortid = doc["sort"].get_int32();
                int32_t ntype = doc["type"].get_int32();
                string title = doc["title"].get_utf8().value.to_string();

                ActiveItem ActiveItem_ = {0};
                ActiveItem_.title = title;
                ActiveItem_.sortid = sortid;
                ActiveItem_.status = status;
                ActiveItem_.type = ntype;

                activeIteminfo.push_back(ActiveItem_);
                LOG_INFO << "---ActiveItem[" << title << "],size[" << activeIteminfo.size() << "]";
            }
        }

        // 缓存到内存
        if (activeIteminfo.size() > 0)
		{
            MutexLockGuard lock(m_t_mutex);
            m_ActiveItemMap[agentid] = activeIteminfo;
		}
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}

// 
bool RegisteServer::cacheRegPoorItem(int32_t agentid)
{
    // 找到缓存信息
    if (m_RegPoorMap.find(agentid) != m_RegPoorMap.end())
        return true;

    try
    { 
        mongocxx::collection activity_coll = MONGODBCLIENT["gamemain"]["platform_activity_register"];
        auto find_ret = activity_coll.find_one(document{} << "agentid" << agentid << finalize);
        if(!find_ret)
        {
            LOG_ERROR << "---" << __FUNCTION__ << " 没有配置注册送金活动";
            return false;
        }

        auto findView = find_ret->view();

        int64_t rgtReward = findView["rgtReward"].get_int64();
        int64_t rgtIntegral = findView["rgtIntegral"].get_int64();
        int64_t minCoin = findView["minCoin"].get_int64();
        int32_t handselNo = findView["handselNo"].get_int32();
        int64_t reqReward = findView["reqReward"].get_int64();

        RegPoorItem ActiveItem_ = {0};
        ActiveItem_.rgtReward = rgtReward;// / COIN_RATE;
        ActiveItem_.rgtIntegral = (rgtIntegral  / inicfg_.changeRatevalue) / COIN_RATE; //转化之后 的积分值，使用时需要除500 再缩小100倍
        ActiveItem_.handselNo = handselNo;
        ActiveItem_.reqReward = reqReward;// / COIN_RATE;
        ActiveItem_.minCoin = minCoin;// / COIN_RATE;

        // 缓存到内存
		{
            MutexLockGuard lock(m_t_mutex);
        	m_RegPoorMap[agentid] = ActiveItem_;
		}
        LOG_INFO << "---RegPoorItem[" << agentid << "],rgtReward[" << rgtReward << "],rgtIntegral[" << rgtIntegral << "],handselNo[" << handselNo << "],reqReward[" << reqReward << "]";
    }
    catch (exception excep)
    {
        LOG_ERROR << "---" << __FUNCTION__ << ",what[" << excep.what() << "]";
        return false;
    }

    return true;
}


/*
string _urlDecode = "5%2F1B01ReacqoSPj%2BBRJZcvWnmaj5ZxTWmeCXQ81jUL72ssYYrKeVvhqvW2e4kxmdLmwgKh66ls89IZGab%2BNPX%2BX0k5vtZ8tcDyVYADPP6v3adwfdo%2FHUrZVC2oVapNsGNCjnGuIsGYnOJQHyl4EEyuxhSY%2FzqpIOoVfOUCRtUggDoWGHwlEmudkPR9SrQDBblhyv13LgV9hH3y8m1DKylw%3D%3D";
// html解码
string _htmlDecode = HTML::HtmlDecode(_urlDecode);
LOG_INFO << "\nhtmlDecode\n"  << _htmlDecode;
// url解码
_urlDecode = boost::regex_replace(URL::MultipleDecode(_htmlDecode), m_pattern, "");
LOG_INFO << "\nurlDecode\n"  << _urlDecode << "\nAesKey " << inicfg_.secondTokenHandleAesKey;

//对token第一次Aes解密
string decrptUrl =  Crypto::AES_ECB_Decrypt(inicfg_.secondTokenHandleAesKey, _urlDecode);
LOG_INFO << "\ndecrptUrl\n"  << decrptUrl;

string errmsg,_token,_descode;
//解析参数
HttpParams params;
if (!parseQuery(decrptUrl, params, errmsg)){
}

//token
HttpParams::const_iterator tokenKey = params.find("token");
if (tokenKey == params.end() || tokenKey->second.empty()){
}
else{
	_token = tokenKey->second;
}

//descode
HttpParams::const_iterator descodeKey = params.find("descode");
if (descodeKey == params.end() || descodeKey->second.empty()){
}
else{
	_descode = descodeKey->second;
}

// html解码
string htmlDecode = HTML::HtmlDecode(_token);
LOG_INFO << "\nhtmlDecode 2\n"  << htmlDecode;

string strURL = URL::MultipleDecode(htmlDecode);
strURL = boost::regex_replace(strURL, m_pattern, "");
LOG_INFO << "---strURL\n"  << strURL;

string decrptString = Crypto::AES_ECB_Decrypt(_descode, strURL);
LOG_WARN << "---decrptString " << decrptString;
*/


/*
#if 0
	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&RegisteServer::threadInit, this));

	//指定业务线程数量，开启业务线程，处理来自TCP逻辑业务
	threadPool_.start(workerNumThreads);

	//网络IO线程数量
	server_.setThreadNum(numThreads);
	LOG_INFO << __FUNCTION__ << "--- *** "
		<< "\nLoginServer = " << server_.ipPort()
		<< " 网络IO线程数 = " << numThreads
		<< " worker 线程数 = " << workerNumThreads;

	//启动TCP监听
	server_.start();
#else
	//assert(0 <= numThreads);
	//threadPool_->setThreadNum(numThreads);
	//threadPool_->start(); 
#endif
*/


// string RegisteServer::checkNullChar(string ){
// 	switch (bsType) {
// 	case bsoncxx::type::k_int64:
// 		_agent_info.score = doc["score"].get_int64();
// 		break;
// 	case bsoncxx::type::k_int32:
// 		_agent_info.score = doc["score"].get_int32();
// 		break;
// 	default:
// 		_agent_info.score = 0;
// 		break;
// 	}

// 	bsoncxx::type::k
// 	return "";
// }
