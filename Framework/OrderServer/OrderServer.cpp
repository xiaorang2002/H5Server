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

#include <muduo/base/Exception.h>
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
#include <iomanip>
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

#include "json/json.h"
//#include "crypto/crypto.h"

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
//#include "crypto.h"
#include "aes.h"
#include "./public/urlcodec.h"

// #ifdef __cplusplus
// }
// #endif
// #include "TxhAPI.hpp"
#include "OrderServer.h"

extern int g_EastOfUtc;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
//using bsoncxx::to_json;

// using namespace mongocxx;
// using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

// shared_ptr<CTxhAPI> m_TxhAPI;

OrderServer::Entry::~Entry() {
	muduo::net::TcpConnectionPtr conn(weakConn_.lock());
	if (conn) {
		conn->getLoop()->assertInLoopThread();
#ifdef _DEBUG_BUCKETS_
		LOG_WARN << __FUNCTION__ << " 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] timeout closing";
#endif
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		LOG_WARN << __FUNCTION__ << " Entry::dtor[" << entryContext->getSession() << "]";
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
		//conn->forceCloseWithDelay(0.2f);
#endif
	}
}

OrderServer::OrderServer(
	muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrHttp,
	muduo::net::TcpServer::Option option)
		: httpServer_(loop, listenAddrHttp, "OrderHttpServer", option)
		//, server_(loop, listenAddr, "OrderServer")
        , threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "TimerEventLoopThread"))
        , m_pIpFinder("qqwry.dat")
        , m_bInvaildServer(false)
		//, threadPool_(new muduo::net::EventLoopThreadPool(loop, "TcpClientEventLoopThread"))
#ifdef _TCP_NOTIFY_CLIENT_
	, clientThreadPool_(new Landy::MyEventLoopThreadPool("TcpClientEventLoopThread"))
#endif
	, isdecrypt_(false)
	, whiteListControl_(eWhiteListCtrl::Close)
	//, strIpAddr_(strIpAddr)
	, kTimeoutSeconds_(3)
	, kMaxConnections_(15000)
#ifdef _STAT_ORDER_QPS_
	, deltaTime_(10)
#endif
	, server_state_(ServiceRunning)
	, ttlUserLockSeconds_(1000)
	, ttlAgentLockSeconds_(500)
	, orderIdExpiredSeconds_(30 * 60)
	, redLockContinue_(false)
{
	httpServer_.setConnectionCallback(
		std::bind(&OrderServer::onHttpConnection, this, std::placeholders::_1));
	httpServer_.setMessageCallback(
		std::bind(&OrderServer::onHttpMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpServer_.setWriteCompleteCallback(
		std::bind(&OrderServer::onWriteComplete, this, std::placeholders::_1));
    //server_.setConnectionCallback(bind(&OrderServer::onConnection, this, std::placeholders::_1));
    //server_.setMessageCallback(bind(&OrderServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  
	initModuleHandlers();

	// 获取Loop指针
	muduo::net::EventLoop * loopTimer_ = threadTimer_->startLoop();

	// 操作对象
	// m_TxhAPI.reset(new CTxhAPI(loopTimer_));
	// m_TxhAPI->startInit();  
}

OrderServer::~OrderServer()
{
//    m_functor_map.clear();
    quit();
}

//注册handler ///
void OrderServer::initModuleHandlers()
{
	//m_functor_map[(::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER << 8) | ::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER]
	//	= bind(&OrderServer::cmd_repair_OrderServer, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
}

void OrderServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
#ifdef SHARED_THREADPOOL
    threadPool_.stop();
#else
	for (int i = 0; i < threadPool_.size(); ++i) {
		threadPool_[i]->stop();
	}
#endif
}

//截取double小数点后bit位，直接截断
static double floorx(double d, int bit) {
	int rate = pow(10, bit);
	int64_t x = int64_t(d * rate);
	return (((double)x) / rate);
}

//截取double小数点后bit位，四舍五入
static double roundx(double d, int bit) {
	int rate = pow(10, bit);
	int64_t x = int64_t(d * rate + 0.5f);
	return (((double)x) / rate);
}

//截取double小数点后bit位，直接截断
static double floorx(double d) {
#if 0
	int64_t x = int64_t(d * 100);
	return (((double)x) / 100);
#elif 0
	char c[20] = { 0 };
	snprintf(c, sizeof c, "%.2f", d);
	return atof(c);
#endif
}

//截取double小数点后2位，直接截断
static double floors(std::string const& s) {
	std::string::size_type npos = s.find_first_of('.');
	if (npos != std::string::npos) {
		std::string dot;
		std::string prefix = s.substr(0, npos);
		std::string sufix = s.substr(npos + 1, -1);
		if (!sufix.empty()) {
			if (sufix.length() >= 2) {
				dot = sufix.substr(0, 2);
			}
			else {
				dot = sufix.substr(0, 1);
			}
		}
		std::string x = prefix + "." + dot;
		return atof(x.c_str());
	}
	else {
		return atof(s.c_str());
	}
}

//截取double小数点后2位，直接截断并乘以100转int64_t
static int64_t rate100(std::string const& s) {
	std::string::size_type npos = s.find_first_of('.');
	if (npos != std::string::npos) {
		std::string prefix = s.substr(0, npos);
		std::string sufix = s.substr(npos + 1, -1);
		std::string x;
		if (!sufix.empty()) {
			if (sufix.length() >= 2) {
				x = prefix + sufix.substr(0, 2);
			}
			else {
				x = prefix + sufix.substr(0, 1) + "0";
			}
		}
		return atoll(x.c_str());
	}
	return atoll(s.c_str()) * 100;
}

static __thread mongocxx::database* dbgamemain_;

//MongoDB/redis与线程池关联 ///
void OrderServer::threadInit()
{
	LOG_INFO << __FUNCTION__;
	if (!REDISCLIENT.initRedisCluster(m_redisIPPort, m_redisPassword))
	{
		LOG_FATAL << "RedisClient Connection ERROR!";
		return;
	}
	//http://mongocxx.org/mongocxx-v3/tutorial/
	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
	static __thread mongocxx::database db = MONGODBCLIENT["gamemain"];
	dbgamemain_ = &db;

	//redis分布式锁 ///
	for (std::vector<std::string>::const_iterator it = redlockVec_.begin();
		it != redlockVec_.end(); ++it) {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, *it, boost::is_any_of(":"));
		LOG_INFO << __FUNCTION__ << " --- *** " << "\nredisLock " << vec[0].c_str() << ":" << vec[1].c_str();
		REDISLOCK.AddServerUrl(vec[0].c_str(), atol(vec[1].c_str()));
	}
}

bool OrderServer::InitZookeeper(std::string ipaddr)
{
    LOG_INFO << __FUNCTION__;
    m_zookeeperclient.reset(new ZookeeperClient(ipaddr));
	//指定zookeeper连接成功回调 ///
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&OrderServer::ZookeeperConnectedHandler, this));
    //连接到zookeeper，成功后回调ZookeeperConnectedHandler ///
	if (!m_zookeeperclient->connectServer()) {
		LOG_FATAL << "InitZookeeper error";
		abort();
	}
	return true;
}

bool OrderServer::InitRedisCluster(std::string ipaddr, std::string password)
{
	LOG_INFO << __FUNCTION__;
	m_redisPubSubClient.reset(new RedisClient());
	if (!m_redisPubSubClient->initRedisCluster(ipaddr, password)) {
		LOG_FATAL << "InitRedisCluster error,ipaddr[" << ipaddr <<"]";
		// abort();
	}
	m_redisPassword = password;
	//redis广播刷新agent_info消息
    // m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_ update_ proxy _info,CALLBACK_1(OrderServer::refreshAgentInfo, this));
    m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_proxy_info,[&](string msg){
		LOG_DEBUG << "bc update proxy info:"<< msg;
		refreshAgentInfo();
    });
	// 
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_single_proxy,CALLBACK_0(OrderServer::refreshRepairAgentInfo,this));
	//redis广播刷新IP白名单消息
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_white_list, boost::bind(&OrderServer::refreshWhiteList, this));
	//redis广播节点维护/开启服务
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_apiserver_repair, boost::bind(&OrderServer::repairApiServerNotify, this, boost::placeholders::_1));
	//m_redisPubSubClient->subsreibeRefreashConfigMessage(boost::bind(&OrderServer::UpdateConfig, this, boost::placeholders::_1));
	//m_redisPubSubClient->subscribeRechargeScoreMessage(boost::bind(&OrderServer::notifyRechargeScoreMessage, this, _1));
	//m_redisPubSubClient->subscribeExchangeScoreMessage(boost::bind(&OrderServer::notifyExchangeScoreMessage, this, _1));
	m_redisPubSubClient->startSubThread();

	m_redisIPPort = ipaddr;

	return true;
}

bool OrderServer::InitMongoDB(std::string url)
{
	//http://mongocxx.org/mongocxx-v3/tutorial/
	LOG_INFO << __FUNCTION__ << " --- *** " << url;
    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}



//=====config=====
bool OrderServer::LoadIniConfig()
{
	LOG_WARN <<"---"<< __FUNCTION__ ;

    if(!boost::filesystem::exists("./conf/game.conf")){
        LOG_ERROR << "./conf/game.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/game.conf", pt); 

    IniConfig inicfg;
    inicfg.addtime 			= pt.get<int>("OrderServer.addtime", 10);// 倍数
    inicfg.auditThreshold 	= pt.get<int>("OrderServer.auditThreshold", 10000000);//分值
    inicfg.minscore 		= pt.get<int>("OrderServer.minscore", 100000);// 最小分数
	
	inicfg_ = inicfg;
    LOG_DEBUG << "---auditThreshold["<< inicfg_.auditThreshold <<"],addtime:"<<inicfg_.addtime <<"],minscore:"<<inicfg_.minscore <<"]";
	return true;
}

// bool OrderServer::InitRocketMQ(std::string &ipaddr)
// {
// 	//注册上下分操作处理回调 ///
// 	m_ConsumerClusterScoreOrder.SetMesageCallback(
// 		std::bind(&OrderServer::ConsumerClusterScoreOrderCallback, this, std::placeholders::_1));
// 	try
// 	{
//         //生产者 ///
// 		m_Producer.Start(ipaddr, "TianXiaRet");
//         //消费者 ///
// 		m_ConsumerClusterScoreOrder.Start(ipaddr, "TianXia", "ScoreOrder", "*", MessageModel::CLUSTERING);
// 	}
// 	catch (std::exception & e)
// 	{
// 		LOG_DEBUG << "InitRocketMQ Error:" << e.what();
// 		abort();
// 	}
// 	return true;
// }

// uint32_t OrderServer::RocketMQBroadcastCallback(const std::vector<MQMessageExt> &mqe)
// {
// 	for (vector<MQMessageExt>::const_iterator it = mqe.begin();
// 		it != mqe.end(); ++it) {
// 
// 	}
// 	return 0;
// }

//上下分操作处理回调 ///
// uint32_t OrderServer::ConsumerClusterScoreOrderCallback(const std::vector<MQMessageExt> &mqe)
// {
// 	for (vector<MQMessageExt>::const_iterator it = mqe.begin();
// 		it != mqe.end(); ++it) {
// 		std::string tag = it->getTags();
// 		std::string key = it->getKeys();
// 		std::string body = it->getBody();
// 		if (key == "Add") {
// 			// 	LOG_INFO << __FUNCTION__ << " --- *** " << "处理上分请求"
// 			// 		<< "\ntag = " << tag
// 			// 		<< "\nkey = " << key
// 			// 		<< "\nbody = " << body;
// 			//处理上分
// 			//threadPool_.run(bind(&OrderServer::ProcessScoreOrderAdd, this, tag, key, body));
// 		}
// 		else if (key == "Sub") {
// 			// 	LOG_INFO << __FUNCTION__ << " --- *** " << "处理下分请求"
// 			// 		<< "\ntag = " << tag
// 			// 		<< "\nkey = " << key
// 			// 		<< "\nbody = " << body;
// 			//处理下分
// 			//threadPool_.run(bind(&OrderServer::ProcessScoreOrderSub, this, tag, key, body));
// 		}
// 	}
//     return 0;
// }

//停止上下分操作处理 ///
// void OrderServer::stopConsumerClusterScoreOrder()
// {
//     m_ConsumerClusterScoreOrder.shutdownConsumer();
// }

//zookeeper连接成功回调 ///
void OrderServer::ZookeeperConnectedHandler()
{
	LOG_INFO << __FUNCTION__;
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");
    //ApiServers节点集合
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/ApiServers"))
        m_zookeeperclient->createNode("/GAME/ApiServers", "ApiServers");
	//ApiServers节点集合(维护中)
	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/ApiServersInvalid"))
		m_zookeeperclient->createNode("/GAME/ApiServersInvalid", "ApiServersInvalid");
	//ProxyServers节点集合
// 	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/ProxyServers"))
// 		m_zookeeperclient->createNode("/GAME/ProxyServers", "ProxyServers");
	//HallServers节点集合
// 	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/HallServers"))
// 		m_zookeeperclient->createNode("/GAME/HallServers", "HallServers");
	//GameServers节点集合
// 	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServers"))
// 		m_zookeeperclient->createNode("/GAME/GameServers", "GameServers");
	//MatchServers节点集合
// 	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/MatchServers"))
// 		m_zookeeperclient->createNode("/GAME/MatchServers", "MatchServers");
	//GameServersInvalid节点集合
// 	if (ZNONODE == m_zookeeperclient->existsNode("/GAME/GameServersInvalid"))
// 		m_zookeeperclient->createNode("/GAME/GameServersInvalid", "GameServersInvalid");

#if 0
	//上下分加锁操作
	m_szUserOrderScorePath = "/GAME/UserOrderScore";
	if (ZNONODE == m_zookeeperclient->existsNode(m_szUserOrderScorePath)) {
		m_zookeeperclient->createNode(m_szUserOrderScorePath, "UserOrderScore");
	}
#endif

    //本地节点启动时自注册 ///
	{
        //指定网卡ip:port ///
		std::vector<std::string> vec;
		boost::algorithm::split(vec, httpServer_.ipPort(), boost::is_any_of(":"));
		
		m_szNodeValue = strIpAddr_ + ":" + vec[1];
		m_szNodePath = "/GAME/ApiServers/" + m_szNodeValue;
		//自注册OrderServer节点到zookeeper
		if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
			m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
		}
		m_szInvalidNodePath = "/GAME/ApiServersInvalid/" + m_szNodeValue;
	}
    //网关服 ip:port:port:pid 值列表 ///
	{
		std::vector<std::string> newProxyServers;
		if (ZOK == m_zookeeperclient->getClildren(
			"/GAME/ProxyServers", newProxyServers,
			std::bind(&OrderServer::GetProxyServersChildrenWatcherHandler, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
			muduo::MutexLockGuard lock(mutexProxyServerIps_);
			proxyServerIps_.assign(newProxyServers.begin(), newProxyServers.end());
		}
	}

	startAllProxyServerConnection();
}

//自注册服务节点到zookeeper ///
void OrderServer::OnRepairServerNode()
{
	if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
		LOG_DEBUG << "--- *** " << m_szNodePath;
		m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
	}
}

//HttpServer通知停止上下分操作处理 ///
// void OrderServer::cmd_repair_OrderServer(muduo::net::WeakTcpConnectionPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header)
// {
// 	LOG_INFO << __FUNCTION__;
// 	Header* common = (Header*)msgdata;
// 	//返回状态码
// 	int32_t repaireStatus = 1;
// 	::Game::Common::HttpNotifyRepairServerResp  msg;
// 	if (msg.ParseFromArray(&msgdata[sizeof(Header)], common->realSize)) {
// 		//维护状态
// 		int32_t status = msg.status();
// 		//如果需要维护，但是非维护状态
// 		if (status == 1 && !m_bInvaildServer) {
// 			//创建zookeeper维护节点
// 			if (ZOK == m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true)) {
// 				m_bInvaildServer = true;
// 				//server_.getLoop()->runInLoop(std::bind(&OrderServer::stopConsumerClusterScoreOrder, this));//上下分维护
// 			}
// 			else {
// 				//维护节点创建失败
// 				repaireStatus = 2;
// 			}
// 		}
// 		else {
// 			repaireStatus = 0;
// 		}
// 	}
// 	msg.set_status(repaireStatus);
// 	size_t len = msg.ByteSize();
// 	//std::vector<unsigned char> data(len + sizeof(internal_prev_header) + sizeof(Header));
// 	//if (msg.SerializeToArray(&data[sizeof(internal_prev_header) + sizeof(Header)], len))
// 	//{
// 	//	sendData(weakConn, data, internal_header, common);
// 	//}
// }

//ProxyServer节点变更处理回调 ///
void OrderServer::GetProxyServersChildrenWatcherHandler(
	int type, int state,
	const std::shared_ptr<ZookeeperClient> &zkClientPtr,
	const std::string &path, void *context)
{
	//新增节点
	std::vector<std::string> newProxyServers;
	if (ZOK == m_zookeeperclient->getClildren(
		"/GAME/ProxyServers", newProxyServers,
		std::bind(&OrderServer::GetProxyServersChildrenWatcherHandler, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
#ifdef _TCP_NOTIFY_CLIENT_
		//
		processProxyIPServer(newProxyServers);
#endif
	}
	//
	{
		std::string ipaddrs;
		for (std::string ipaddr : newProxyServers) {
			ipaddrs += "\n[" + ipaddr + "]";
		}
		LOG_DEBUG << "--- *** "
			<< "\npath = " << path
			<< "\ntype = " << type << " state = " << state << ipaddrs;
	}
}

//订单服[C]端 -> 网关服[S]端 ///
void OrderServer::startAllProxyServerConnection() {
	
	std::string ipaddrs;
	{
		muduo::MutexLockGuard lock(mutexProxyServerIps_);
		for (std::string ipaddr : proxyServerIps_) {
			ipaddrs += "\n[" + ipaddr + "]";
#ifdef _TCP_NOTIFY_CLIENT_
			startProxyServerConnection(ipaddr);
#endif
		}
	}
	LOG_DEBUG << "--- *** " << ipaddrs;
}

#ifdef _TCP_NOTIFY_CLIENT_
//订单服[C]端 -> 网关服[S]端 ///
void OrderServer::startProxyServerConnection(std::string ipaddr) {

	muduo::MutexLockGuard lock(mutexClientMap_);
	TcpClientMap::const_iterator it = clientMap_.find(ipaddr);
	if (it == clientMap_.end()) {
		
		std::vector<std::string> vec;
		boost::algorithm::split(vec, ipaddr, boost::is_any_of(":"));
		
		//vec：ip:port:port:pid ///
		muduo::net::InetAddress serverAddr(vec[0], stoi(vec[2]));
		
		LOG_INFO << __FUNCTION__ << " --- *** >>> 网关服[" << vec[0] << ":" << vec[1] << ":" << vec[2] << ":" << vec[3] << "]";
		//
		//和httpserver_共享一个IO线程池，FIXED ME: NotInLoopThread !!!
#if 0
		//确保已经启动线程池 ///
		assert(httpServer_.threadPool()->started());
		
		//创建TcpClient连接对象 ///
		TcpClientPtr tcpClient(
			new muduo::net::TcpClient(httpServer_.threadPool()->getNextLoop(), serverAddr, ipaddr));
#else
		TcpClientPtr tcpClient(
			new muduo::net::TcpClient(clientThreadPool_->getNextLoop(), serverAddr, ipaddr));
#endif
		//指定处理回调函数 ///
		tcpClient->setConnectionCallback(
			bind(&OrderServer::onConnection, this, std::placeholders::_1));
		tcpClient->setMessageCallback(
			bind(&OrderServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		
		//若断线则重连 ///
		tcpClient->enableRetry();
		
		//发起异步连接 ///
		tcpClient->connect();
		
		//保留全局的shared_ptr唯一引用计数，防止TcpClient析构dtor ///
		//因为是异步连接，无法保证clientMap_中的都会连接成功，真正有效连接保存在weakConns_中 //
		clientMap_[ipaddr] = tcpClient;
	}
	else {
		std::shared_ptr<muduo::net::TcpClient>const& tcpClient = it->second;
		if (tcpClient &&
			tcpClient->connection() &&
			tcpClient->connection()->connected()) {
		}
	}
}

//订单服[C]端 -> 网关服[S]端 ///
void OrderServer::processProxyIPServer(std::vector<std::string>& newProxyServers) {
	//LOG_INFO << __FUNCTION__;
	std::vector<std::string> addIP;
	std::set<std::string> newIPSet(newProxyServers.begin(), newProxyServers.end());
	std::set<std::string> oldIPSet;
	{
		muduo::MutexLockGuard lock(mutexProxyServerIps_);
		for (std::string ipaddr : proxyServerIps_) {
			oldIPSet.emplace(ipaddr);
		}
	}
	//查找proxyServerIps_中有，而newProxyServers中没有的 ///
	std::vector<std::string> diffIPS(oldIPSet.size());
	std::vector<std::string>::iterator it;
	it = std::set_difference(oldIPSet.begin(), oldIPSet.end(), newIPSet.begin(), newIPSet.end(), diffIPS.begin());
	diffIPS.resize(it - diffIPS.begin());
	//无效节点连接资源需要释放掉 ///
	{
		muduo::MutexLockGuard lock(mutexClientMap_);
		for (std::string ipaddr : diffIPS) {
			auto it = clientMap_.find(ipaddr);
			if (clientMap_.end() != it) {
				std::shared_ptr<muduo::net::TcpClient> client(it->second);
				if (client && client->connection() && client->connection()->connected()) {
					addIP.push_back(ipaddr);
				}
				else {
					//dtor forceClose
					clientMap_.erase(it);
				}
			}
		}
	}
	//查找newProxyServers中有，而proxyServerIps_中没有的 ///
	diffIPS.clear();
	diffIPS.resize(newIPSet.size());
	it = std::set_difference(newIPSet.begin(), newIPSet.end(), oldIPSet.begin(), oldIPSet.end(), diffIPS.begin());
	diffIPS.resize(it - diffIPS.begin());
	//连接新增节点 ///
	for (std::string ipaddr : diffIPS) {
		startProxyServerConnection(ipaddr);
	}
	//保存newProxyServers到proxyServerIps_中 ///
	{
		muduo::MutexLockGuard lock(mutexProxyServerIps_);
		proxyServerIps_.assign(newProxyServers.begin(), newProxyServers.end());
		//包括原有存活节点 ///
		for (int i = 0; i < addIP.size(); ++i) {
			proxyServerIps_.push_back(addIP[i]);
		}
	}
}
#endif

//启动TCP业务线程 ///
//启动TCP监听 ///
void OrderServer::start(int numThreads, int workerNumThreads)
{
	//threadTimer_->getLoop()->runEvery(5.0f, std::bind(&OrderServer::OnRepairServerNode, this));
	threadTimer_->getLoop()->runAfter(3, std::bind(&OrderServer::refreshAgentInfo, this));
	threadTimer_->getLoop()->runAfter(3, std::bind(&OrderServer::refreshWhiteList, this));
	threadTimer_->getLoop()->runAfter(1, bind(&OrderServer::refreshRepairAgentInfo, this));

#if 0
	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&OrderServer::threadInit, this));

	//指定业务线程数量，开启业务线程，处理来自TCP逻辑业务
	threadPool_.start(workerNumThreads);

	//网络IO线程数量
	server_.setThreadNum(numThreads);
	LOG_INFO << __FUNCTION__ << " --- *** "
		<< "\nOrderServer = " << server_.ipPort()
		<< " 网络IO线程数 = " << numThreads
		<< " worker 线程数 = " << workerNumThreads;

	//启动TCP监听
	server_.start();
#else
	//assert(0 <= numThreads);
	//threadPool_->setThreadNum(numThreads);
	//threadPool_->start();

#ifdef _TCP_NOTIFY_CLIENT_
	clientThreadPool_->setThreadNum(numThreads);
	clientThreadPool_->start();
#endif

#endif
}

//启动HTTP业务线程 ///
//启动HTTP监听 ///
void OrderServer::startHttpServer(int numThreads, int numWorkerThreads, int maxSize) {
	
	//网络I/O线程数
	numThreads_ = numThreads;
	//worker线程数，最好 numWorkerThreads = n * numThreads
	workerNumThreads_ = numWorkerThreads;

	//创建若干worker线程，启动worker线程池
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&OrderServer::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	//网络IO线程数量
	httpServer_.setThreadNum(numThreads);
	LOG_INFO << __FUNCTION__ << " --- *** "
		<< "\nHttpServer = " << httpServer_.ipPort()
		<< " 网络I/O线程数 = " << numThreads
		<< " worker线程数 = " << numWorkerThreads;
	
	//开启了IP访问白名单功能 ///
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	if (whiteListControl_ == eWhiteListCtrl::OpenAccept) {
		//开启IP访问白名单检查
		httpServer_.setConditionCallback(std::bind(&OrderServer::onHttpCondition, this, std::placeholders::_1));
	}

	//启动HTTP监听
	httpServer_.start();

	//等server_所有的网络I/O线程都启动起来
	//sleep(2);

	//获取网络I/O模型EventLoop池
	std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool = httpServer_.threadPool();
	std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();
	
	//为各网络I/O线程绑定Bucket
	for (size_t index = 0; index < loops.size(); ++index) {
#if 0
		ConnBucketPtr bucket(new ConnBucket(
			loops[index], index, kTimeoutSeconds_));
		bucketsPool_.emplace_back(std::move(bucket));
#else
		bucketsPool_.emplace_back(
			ConnBucketPtr(new ConnBucket(
				loops[index], index, kTimeoutSeconds_)));
#endif
		loops[index]->setContext(EventLoopContextPtr(new EventLoopContext(index)));
	}
	//为每个网络I/O线程绑定若干worker线程(均匀分配)
	{
		int next = 0;
		for (size_t index = 0; index < threadPool_.size(); ++index) {
			EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(loops[next]->getContext());
			assert(context);
			context->addWorkerIndex(index);
			if (++next >= loops.size()) {
				next = 0;
			}
		}
	}
	//启动连接超时定时器检查，间隔1s
	for (size_t index = 0; index < loops.size(); ++index) {
		{
			assert(bucketsPool_[index]->index_ == index);
			assert(bucketsPool_[index]->loop_ == loops[index]);
		}
		{
			EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(loops[index]->getContext());
			assert(context->index_ == index);
		}
		loops[index]->runAfter(1.0f, std::bind(&ConnBucket::onTimer, bucketsPool_[index].get()));
	}
	//输出性能指标日志
	//threadQPSLog_ = std::make_shared<muduo::ThreadPool>("QPSLogThread:" + std::to_string(0));
	//threadQPSLog_->start(1);
}

//白名单检查 ///
bool OrderServer::onHttpCondition(const InetAddress& peerAddr) {
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	//开启了IP访问白名单功能 ///
	assert(whiteListControl_ == eWhiteListCtrl::OpenAccept);
	//安全断言 ///
	httpServer_.getLoop()->assertInLoopThread();
	{
		//管理员挂维护/恢复服务 ///
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
		if (it != admin_list_.end()) {
			return true;
		}
	}
	{
		//192.168.2.21:3640 192.168.2.21:3667
		std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(peerAddr.ipNetEndian());
		//在IP访问白名单中 ///
		return (it != white_list_.end()) && (eApiVisit::Enable == it->second);
	}
#if 0
	//节点维护中 ///
	if (server_state_ == ServiceRepairing) {
		return false;
	}
#endif
	return true;
}

//Connected/closed事件 ///
void OrderServer::onHttpConnection(const muduo::net::TcpConnectionPtr& conn)
{
	conn->getLoop()->assertInLoopThread();

	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;

		//累计接收请求数
		numTotalReq_.incrementAndGet();

		//最大连接数限制
		if (num > kMaxConnections_) {
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
				"HTTP/1.1 600 访问量限制(" + std::to_string(kMaxConnections_) + ")\r\n\r\n");
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);

			//延迟0.2s强制关闭连接
			//conn->forceCloseWithDelay(0.2f);
#endif
				//会调用onHttpMessage函数
			assert(conn->getContext().empty());

			//累计未处理请求数
			numTotalBadReq_.incrementAndGet();
			return;
		}
		//最好不要在网络IO线程中做逻辑处理
#if 0
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == eWhiteListCtrl::Open) {
			bool is_ip_allowed = false;
			{
				READ_LOCK(white_list_mutex_);
				std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(conn->peerAddress().ipNetEndian());
				//在IP访问白名单中 ///
				is_ip_allowed = ((it != white_list_.end()) && (eApiVisit::Enable == it->second));
			}
			//不在IP访问白名单中 ///
			if (!is_ip_allowed) {
				//强制关闭 ///
#if 0
				conn->shutdown();
#elif 1
				conn->forceClose();
#else
				conn->forceCloseWithDelay(0.2f);
#endif
				return;
			}
		}
#endif
		EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
		assert(context);

		EntryPtr entry(new Entry(muduo::net::WeakTcpConnectionPtr(conn)));
			
		//指定conn上下文信息
		ContextPtr entryContext(new Context(WeakEntryPtr(entry), muduo::net::HttpContext()));
		conn->setContext(entryContext);
		{
			//给新conn绑定一个worker线程，与之相关所有逻辑业务都在该worker线程中处理
			int index = context->allocWorkerIndex();
			assert(index >= 0 && index < threadPool_.size());

			//对于HTTP请求来说，每一个conn都应该是独立的，指定一个独立线程处理即可，避免锁开销与多线程竞争抢占共享资源带来的性能损耗
			entryContext->setWorkerIndex(index);
		}
		{
			//获取EventLoop关联的Bucket
			int index = context->getBucketIndex();
			assert(index >= 0 && index < bucketsPool_.size());
				
			//连接成功，压入桶元素
			conn->getLoop()->runInLoop(
				std::bind(&ConnBucket::pushBucket, bucketsPool_[index].get(), entry));
		}
		{
			//TCP_NODELAY
			conn->setTcpNoDelay(true);
		}
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
}

void OrderServer::onWriteComplete(const muduo::net::TcpConnectionPtr& conn) {
	
	LOG_WARN << __FUNCTION__;

	conn->getLoop()->assertInLoopThread();
#if 1
	//延迟0.2s强制关闭连接
	conn->forceCloseWithDelay(0.2f);
#endif
}

//接收HTTP网络消息回调 ///
void OrderServer::onHttpMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime)
{
	conn->getLoop()->assertInLoopThread();

	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		//LOG_ERROR << __FUNCTION__ << " --- *** " << "TcpConnectionPtr.conn max";
		return;
	}
	
	//LOG_ERROR << __FUNCTION__ << " --- *** ";
	//printf("----------------------------------------------\n");
	//printf("%.*s\n", buf->readableBytes(), buf->peek());

	//先确定是HTTP数据报文，再解析
	//assert(buf->readableBytes() > 4 && buf->findCRLFCRLF());

	ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
	assert(entryContext);
	muduo::net::HttpContext* httpContext = boost::any_cast<muduo::net::HttpContext>(entryContext->getMutableContext());
	assert(httpContext);

	//解析HTTP数据包
	if (!httpContext->parseRequest(buf, receiveTime)) {
		//发生错误
		LOG_ERROR << __FUNCTION__ << " 解析HTTP发生错误";
	}
	else if (httpContext->gotAll()) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == eWhiteListCtrl::Open) {
			std::string ipaddr;
			{
				std::string ipaddrs = httpContext->request().getHeader("X-Forwarded-For");
				if (ipaddrs.empty()) {
					ipaddr = conn->peerAddress().toIp();
				}
				else {
#if 0
					//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP
					std::string::size_type spos = ipaddrs.find_first_of(',');
					if (spos == std::string::npos) {
					}
					else {
						ipaddr = ipaddrs.substr(0, spos);
					}
#else
					boost::replace_all<std::string>(ipaddrs, " ", "");
					std::vector<std::string> vec;
					boost::algorithm::split(vec, ipaddrs, boost::is_any_of(","));
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
								ipaddr = *it;
								break;
							}
						}
					}
#endif
				}
			}
			muduo::net::InetAddress peerAddr(muduo::StringArg(ipaddr), 0, false);
			bool is_ip_allowed = false;
			{
				//管理员挂维护/恢复服务
				std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
				is_ip_allowed = (it != admin_list_.end());
			}
			if(!is_ip_allowed) {
				READ_LOCK(white_list_mutex_);
				std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(peerAddr.ipNetEndian());
				is_ip_allowed = ((it != white_list_.end()) && (eApiVisit::Enable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 1
				//直接强制关闭连接
				conn->forceClose();
#else
				//HTTP应答包(header/body)
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.2f);
#endif
				//累计未处理请求数
				numTotalBadReq_.incrementAndGet();
				return;
			}
		}
		
		EntryPtr entry(entryContext->getWeakEntryPtr().lock());
		if (likely(entry)) {
			{
				EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
				assert(context);
				int index = context->getBucketIndex();
				assert(index >= 0 && index < bucketsPool_.size());

				//收到消息包，更新桶元素
				conn->getLoop()->runInLoop(std::bind(&ConnBucket::updateBucket, bucketsPool_[index].get(), entry));
			}
			{
				//获取绑定的worker线程
				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());
				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&OrderServer::asyncHttpHandler,
						this, muduo::net::WeakTcpConnectionPtr(conn), receiveTime));
			}
			return;
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		LOG_ERROR << __FUNCTION__ << " --- *** " << "entry invalid";
		return;
	}
	//发生错误
	//HTTP应答包(header/body)
	muduo::net::HttpResponse rsp(false);
	setFailedResponse(rsp,
		muduo::net::HttpResponse::k404NotFound,
		"HTTP/1.1 400 Bad Request\r\n\r\n");
	muduo::net::Buffer buffer;
	rsp.appendToBuffer(&buffer);
	conn->send(&buffer);
	//释放HttpContext资源
	httpContext->reset();
#if 0
	//不再发送数据
	conn->shutdown();
#elif 0
	//直接强制关闭连接
	conn->forceClose();
#else
	//延迟0.2s强制关闭连接
	//conn->forceCloseWithDelay(0.2f);
#endif
	//累计未处理请求数
	numTotalBadReq_.incrementAndGet();
}

//异步回调 ///
void OrderServer::asyncHttpHandler(muduo::net::WeakTcpConnectionPtr const& weakConn, muduo::Timestamp receiveTime)
{
	//MY_TRY()
	//LOG_ERROR << __FUNCTION__;
	//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
	//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
	//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
	//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
	//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn) ///
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (conn) {
#if 0
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == eWhiteListCtrl::Open) {

			bool is_ip_allowed = false;
			{
				READ_LOCK(white_list_mutex_);
				std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(conn->peerAddress().ipNetEndian());
				is_ip_allowed = ((it != white_list_.end()) && (eApiVisit::Enable == it->second));
			}
			if (!is_ip_allowed) {
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
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.2f);
#endif
				return;
			}
		}
#endif
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		//获取HttpContext对象
		muduo::net::HttpContext* httpContext = boost::any_cast<muduo::net::HttpContext>(entryContext->getMutableContext());
		assert(httpContext);
		assert(httpContext->gotAll());
		const string& connection = httpContext->request().getHeader("Connection");
		//是否保持HTTP长连接
		bool close = false;//(connection == "close") ||
			//(httpContext->request().getVersion() == muduo::net::HttpRequest::kHttp10 && connection != "Keep-Alive");
		//HTTP应答包(header/body)
		muduo::net::HttpResponse rsp(close);
		//请求处理回调，但非线程安全的
		processHttpRequest(conn, httpContext->request(), rsp, conn->peerAddress(), receiveTime);
		//应答消息
		{
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
		}
		//非HTTP长连接则关闭
		if (rsp.closeConnection()) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 0
			//直接强制关闭连接
			conn->forceClose();
#else
			//延迟0.2s强制关闭连接
			//conn->forceCloseWithDelay(0.2f);
#endif
		}
		//释放HttpContext资源
		httpContext->reset();
	}
	else {
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		LOG_ERROR << __FUNCTION__ << " --- *** " << "TcpConnectionPtr.conn invalid";
	}
}

//解析请求 ///
//strQuery string req.query()
bool OrderServer::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg)
{
	params.clear();
	/*LOG_WARN*/LOG_DEBUG << "--- *** " << "\n" << queryStr;
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
				//skip '=' ///
				std::string val = subStr.substr(kpos + 1, std::string::npos);
				params[key] = val;
				break;
			}
			else if (kpos < spos) {
				std::string key = subStr.substr(0, kpos);
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
		keyValues += "\n--- **** " + param.first + "=" + param.second;
	}
	LOG_DEBUG << "--- *** " << keyValues;
	return true;
}

//请求字符串 ///
std::string OrderServer::getRequestStr(muduo::net::HttpRequest const& req) {
	std::string headers;
	for (std::map<string, string>::const_iterator it = req.headers().begin();
		it != req.headers().end(); ++it) {
		headers += it->first + ": " + it->second + "\n";
	}
	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		<< "<xs:head>" << headers << "</xs:head>"
		<< "<xs:body>"
		<< "<xs:method>" << req.methodString() << "</xs:method>"
		<< "<xs:path>" << req.path() << "</xs:path>"
		<< "<xs:query>" << HTML::Encode(req.query()) << "</xs:query>"
		<< "</xs:body>"
		<< "</xs:root>";
	return ss.str();
}

#ifdef _TCP_NOTIFY_CLIENT_
//连接事件来自 网关服(ProxyServer) ///
//Connected/closed事件 ///
//订单服[C]端 -> 网关服[S]端 ///
void OrderServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		//int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "订单服[" << conn->localAddress().toIpPort() << "] -> 网关服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN")/* << " " << num*/;
	}
	else
	{
		//int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "订单服[" << conn->localAddress().toIpPort() << "] -> 网关服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN")/* << " " << num*/;
	}
	//截取conn->name() ipaddr:ip:port#1 得到ipaddr：ip:port:port:pid
	std::vector<std::string> vec;
	boost::algorithm::split(vec, conn->name(), boost::is_any_of(":"));
	std::string ipaddr(vec[0] + ":" + vec[1] + ":" + vec[2] + ":" + vec[3]);
	
	LOG_DEBUG << "--- *** " << "ipaddr = " << ipaddr;
	
	//连接成功，保存weakConns_中 ///
	{
		//为了安全，仅保留弱指针 ///
		muduo::MutexLockGuard lock(mutexWeakConns_);
		muduo::net::WeakTcpConnectionPtr weakConn(conn);
		weakConns_[ipaddr] = weakConn;
	}
}

//接收TCP网络消息回调 ///
//处理来自 网关服(ProxyServer) 消息 ///
//网关服[S]端 -> 订单服[C]端 ///
void OrderServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime)
{
	//解析TCP数据包，先解析包头(header)，再解析包体(body)，避免粘包出现 kHeaderLen == 4
	while (buf->readableBytes() >= kHeaderLen) {

		// FIXME: use Buffer::peekInt32()
		const uint16_t len = buf->peekInt16();

		//数据包太大或异常的话则拒绝服务 ///
		//buf中可读数据(readableBytes)足够，len比包头(kHeaderLen)还小，明显异常了 ///
		if (len > PACKET_SIZE || len < kHeaderLen) {
			//认为发生错误，强制关闭连接
			if (conn) {
				conn->shutdown();  // FIXME: disable reading
			}
			break;
		}
		//数据包不足够解析，等待下次接收再解析
		else if (len > buf->readableBytes()) {
			break;
		}
		else /*if (len <= buf->readableBytes())*/ {
			BufferPtr buffer(new muduo::net::Buffer(len));
			buffer->append(buf->peek(), static_cast<size_t>(len));
			buf->retrieve(len);
			//扔给任务消息队列处理
			
		}
	}
}

//处理来自 网关服(ProxyServer) 消息 ///
void OrderServer::processRequest(muduo::net::WeakTcpConnectionPtr& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime)
{
	int32_t size = buf->readableBytes();
	if (size < sizeof(internal_prev_header) + sizeof(Header)) {
		return;
	}

	internal_prev_header* pre_header = (internal_prev_header*)buf->peek();
	std::string session(pre_header->session, sizeof(pre_header->session));
	int64_t userId = pre_header->userId;
	//std::string aesKey(pre_header->aesKey, sizeof(pre_header->aesKey));
	Header* commandHeader = (Header*)(buf->peek() + sizeof(internal_prev_header));
	int headLen = sizeof(internal_prev_header) + sizeof(Header);
	uint16_t crc = GlobalFunc::GetCheckSum((uint8_t*)buf->peek() + sizeof(internal_prev_header) + 4, commandHeader->len - 4);
	if (commandHeader->len == size - sizeof(internal_prev_header) && commandHeader->crc == crc
		&& commandHeader->ver == PROTO_VER && commandHeader->sign == HEADER_SIGN)
	{
		TRACE_COMMANDID(commandHeader->mainId, commandHeader->subId);
		switch (commandHeader->mainId)
		{
			//case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
			//case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL:
		case Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER:
		{
			switch (commandHeader->encType)
			{
			case PUBENC__PROTOBUF_NONE:
			{
				int mainid = commandHeader->mainId;
				int subid = commandHeader->subId;
				int id = (mainid << 8) | subid;
				if (likely(m_functor_map.count(id)))
				{
					AccessCommandFunctor functor = m_functor_map[id];
					functor(weakConn, (uint8_t*)buf->peek() + sizeof(internal_prev_header), commandHeader->len, pre_header);
				}
				else
				{

				}
				break;
			}
			case PUBENC__PROTOBUF_RSA:  //RSA
			{
				break;
			}
			case PUBENC__PROTOBUF_AES:  //AES
			{

				std::string aesKey = "0000000000000000";
				int len = 0;
				std::vector<unsigned char> data(commandHeader->len + 16);
				int ret = Landy::Crypto::aesDecrypt(aesKey, (unsigned char*)(buf->peek() + sizeof(internal_prev_header) + sizeof(Header)),
					buf->readableBytes() - sizeof(internal_prev_header) - sizeof(Header), &data[sizeof(Header)], len);
				if (ret > 0)
				{
					int mainid = commandHeader->mainId;
					int subid = commandHeader->subId;
					int id = (mainid << 8) | subid;
					if (likely(m_functor_map.count(id)))
					{
						AccessCommandFunctor functor = m_functor_map[id];
						memcpy(&data[0], buf->peek() + sizeof(internal_prev_header), sizeof(Header));
						functor(weakConn, (uint8_t*)&data[0], commandHeader->len, pre_header);
					}
					else
					{

					}
				}


				break;
			}
			default:
				break;
			}
		}
		break;
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
			break;
		default:
			break;
		}
	}
	else
	{

	}
}
#endif

//按照占位符来替换 ///
static void replace(std::string& json, const std::string& placeholder, const std::string& value) {
	boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
}

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
std::string OrderServer::createResponse(
	int32_t opType,
	std::string const& orderId,
	uint32_t agentId,
	std::string account, double score,
	int errcode, std::string const& errmsg, bool debug)
{
#if 0
	Json::Value root, data;
	if (debug) data["orderid"] = orderId;
	if (debug) data["agentid"] = agentId;
	data["account"] = account;
	data["score"] = (uint32_t)score;
	data["code"] = (int32_t)errcode;
	if (debug) data["errmsg"] = errmsg;
	// 外层json
	root["maintype"] = "/GameHandle";
	root["type"] = opType;
	root["data"] = data;
	Json::FastWriter writer;
	std::string json = writer.write(root);
	boost::replace_all<std::string>(json, "\\", "");
	return json;
#else
	boost::property_tree::ptree root, data;
	if (debug) data.put("orderid", orderId);
	if (debug) data.put("agentid", ":agentid");
	data.put("account", account);
	data.put("score", ":score");
	data.put("code", ":code");
	if (debug) data.put("errmsg", errmsg);
	// 外层json
	root.put("maintype", "/GameHandle");
	root.put("type", ":type");
	root.add_child("data", data);
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	if (debug) replace(json, ":agentid", std::to_string(agentId));
	replace(json, ":score", std::to_string(score));
	replace(json, ":code", std::to_string(errcode));
	replace(json, ":type", std::to_string(opType));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
#endif
}

//最近一次请求(上分或下分操作的elapsed detail)
void OrderServer::createLatestElapsed(
	boost::property_tree::ptree& latest,
	std::string const& op, std::string const& key, double elapsed) {
	//{"op":"mongo.collect", "dt":1000, "key":}
	//{"op":"mongo.insert", "dt":1000, "key":}
	//{"op":"mongo.update", "dt":1000, "key":}
	//{"op":"mongo.query", "dt":1000, "key":}
	//{"op":"redis.insert", "dt":1000, "key":}
	//{"op":"redis.update", "dt":1000, "key":}
	//{"op":"redis.query", "dt":1000, "key":}
#if 0
	boost::property_tree::ptree data, item;
	data.put("op", op);
	data.put("key", key);
	data.put("dt", ":dt");
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, data, false);
	std::string json = s.str();
	replace(json, ":dt", std::to_string(elapsed));
	std::stringstream ss(json);
	boost::property_tree::json_parser::read_json(ss, item);
	latest.push_back(std::make_pair("", item));
#else
	boost::property_tree::ptree item;
	item.put("op", op);
	item.put("key", key);
	item.put("dt", elapsed);
	latest.push_back(std::make_pair("", item));
#endif
}

//监控数据
std::string OrderServer::createMonitorData(
	boost::property_tree::ptree const& latest, double totalTime, int timeout,
	int64_t requestNum, int64_t requestNumSucc, int64_t requestNumFailed, double ratio,
	int64_t requestNumTotal, int64_t requestNumTotalSucc, int64_t requestNumTotalFailed, double ratioTotal, int testTPS) {
	boost::property_tree::ptree root, stat, history;
	//最近一次请求 latest
	root.add_child("latest", latest);
	//统计间隔时间 totalTime
	root.put("stat_dt", ":stat_dt");
	//估算每秒请求处理数 testTPS
	root.put("test_TPS", ":test_TPS");
	//请求超时时间 kTimeoutSeconds_
	root.put("req_timeout", ":req_timeout");
	{
		//统计请求次数 requestNum
		stat.put("stat_total", ":stat_total");
		//统计成功次数 requestNumSucc
		stat.put("stat_succ", ":stat_succ");
		//统计失败次数 requestNumFailed
		stat.put("stat_fail", ":stat_fail");
		//统计命中率 ratio
		stat.put("stat_ratio", ":stat_ratio");
		root.add_child("stat", stat);
	}
	{
		//历史请求次数 requestNumTotal
		history.put("total", ":total");
		//历史成功次数 requestNumTotalSucc
		history.put("succ", ":succ");
		//历史失败次数 requestNumTotalFailed
		history.put("fail", ":fail");
		//历史命中率 ratioTotal
		history.put("ratio", ":ratio");
		root.add_child("history", history);
	}
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	replace(json, ":stat_dt", std::to_string(totalTime));
	replace(json, ":test_TPS", std::to_string(testTPS));
	replace(json, ":req_timeout", std::to_string(timeout));
	replace(json, ":stat_total", std::to_string(requestNum));
	replace(json, ":stat_succ", std::to_string(requestNumSucc));
	replace(json, ":stat_fail", std::to_string(requestNumFailed));
	replace(json, ":stat_ratio", std::to_string(ratio));
	replace(json, ":total", std::to_string(requestNumTotal));
	replace(json, ":succ", std::to_string(requestNumTotalSucc));
	replace(json, ":fail", std::to_string(requestNumTotalFailed));
	replace(json, ":ratio", std::to_string(ratioTotal));
	return json;
}

//输出性能指标日志 ///
void OrderServer::refreshQPSLog(std::string const& str) {
	LOG_ERROR << __FUNCTION__ << " --- *** "
		<< "\n--- *** ------------------------------------------------------\n" << str;
}

//处理HTTP回调 ///
void OrderServer::processHttpRequest(
	const muduo::net::TcpConnectionPtr& conn,
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp, muduo::net::InetAddress const& peerAddr, muduo::Timestamp receiveTime)
{
	//LOG_INFO << __FUNCTION__ << " --- *** " << getRequestStr(req);
	rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
	rsp.setStatusMessage("OK");
	//注意要指定connection状态
	rsp.setCloseConnection(true);
	rsp.addHeader("Server", "TXQP");

	if (req.path() == "/") {
#if 0
		rsp.setContentType("text/html;charset=utf-8");
		std::string now = muduo::Timestamp::now().toFormattedString();
		rsp.setBody("<html><body>Now is " + now + "</body></html>");
#else
		//HTTP应答包(header/body)
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#endif
	}
	else if (req.path() == "/GameHandle") {
		std::string rspdata;
		boost::property_tree::ptree latest;
		int errcode = eApiErrorCode::NoError;
		std::string errmsg;
		int testTPS = 0;
#ifdef _STAT_ORDER_QPS_
		
		//起始时间戳(微秒) ///
		static muduo::Timestamp orderTimeStart_;
		//统计请求次数 ///
		static muduo::AtomicInt32 numOrderRequest_;
		//历史请求次数 ///
		static muduo::AtomicInt32 numOrderRequestTotal_;
		//统计成功次数 ///
		static muduo::AtomicInt32 numOrderRequestSucc_;
		//统计失败次数 ///
		static muduo::AtomicInt32 numOrderRequestFailed_;
		//历史成功次数 ///
		static muduo::AtomicInt32 numOrderRequestTotalSucc_;
		//历史失败次数 ///
		static muduo::AtomicInt32 numOrderRequestTotalFailed_;
		//原子操作判断 ///
		{
			static volatile long value = 0;
			if (0 == __sync_val_compare_and_swap(&value, 0, 1)) {
				//起始时间戳(微秒)
				orderTimeStart_ = muduo::Timestamp::now();
			}
		}
		//本次请求开始时间戳(微秒)
		muduo::Timestamp timestart = muduo::Timestamp::now();
#endif
		//节点维护中不提供上下分服务 ///
		if (server_state_ == ServiceRepairing) {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 405 服务维护中\r\n\r\n");
		}
		//请求处理逻辑 ///
		else {
			rspdata = OrderProcess(conn, req.query(), receiveTime, errcode, errmsg, latest, testTPS);

#ifdef _STAT_ORDER_QPS_
			if (errcode == eApiErrorCode::NoError) {
				//统计成功次数 ///
				numOrderRequestSucc_.incrementAndGet();
				//历史成功次数 ///
				numOrderRequestTotalSucc_.incrementAndGet();
			}
			else {
				//统计失败次数 ///
				numOrderRequestFailed_.incrementAndGet();
				//历史失败次数 ///
				numOrderRequestTotalFailed_.incrementAndGet();
			}
#endif
			rsp.setContentType("application/json;charset=utf-8");
			rsp.setBody(rspdata);
		}
#ifdef _STAT_ORDER_QPS_
		//本次请求结束时间戳(微秒)
		muduo::Timestamp timenow = muduo::Timestamp::now();
		
		//统计请求次数 ///
		numOrderRequest_.incrementAndGet();
		//历史请求次数 ///
		numOrderRequestTotal_.incrementAndGet();

		//原子操作判断 ///
		static volatile long value = 0;
		if (0 == __sync_val_compare_and_swap(&value, 0, 1)) {
			//间隔时间(s)打印一次
			//static int deltaTime_ = 10;
			//统计间隔时间(s)
			double totalTime = muduo::timeDifference(timenow, orderTimeStart_);
			//if (totalTime >= (double)deltaTime_) {
				//最近一次请求耗时(s)
				double timdiff = muduo::timeDifference(timenow, timestart);
				char pid[128] = { 0 };
				snprintf(pid, sizeof(pid), "PID[%07d]", getpid());
				//统计请求次数 ///
				int64_t	requestNum = numOrderRequest_.get();
				//统计成功次数 ///
				int64_t requestNumSucc = numOrderRequestSucc_.get();
				//统计失败次数 ///
				int64_t requestNumFailed = numOrderRequestFailed_.get();
				//统计命中率 ///
				double ratio = (double)(requestNumSucc) / (double)(requestNum);
				//历史请求次数 ///
				int64_t	requestNumTotal = numOrderRequestTotal_.get();
				//历史成功次数 ///
				int64_t requestNumTotalSucc = numOrderRequestTotalSucc_.get();
				//历史失败次数 ///
				int64_t requestNumTotalFailed = numOrderRequestTotalFailed_.get();
				//历史命中率 ///
				double ratioTotal = (double)(requestNumTotalSucc) / (double)(requestNumTotal);
				//平均请求耗时(s) ///
				double avgTime = totalTime / requestNum;
				//每秒请求次数(QPS) ///
				int64_t avgNum = (int64_t)(requestNum / totalTime);
				std::stringstream s;
				boost::property_tree::json_parser::write_json(s, latest, true);
				std::string json = s.str();
#if 1
				LOG_ERROR << __FUNCTION__ << " --- *** "
					<< "\n--- *** ------------------------------------------------------\n"
					<< json
					<< "--- *** " << pid << "[注单]I/O线程数[" << numThreads_ << "] 业务线程数[" << workerNumThreads_ << "] 累计接收请求数[" << numTotalReq_.get() << "] 累计未处理请求数[" << numTotalBadReq_.get() << "]\n"
					<< "--- *** " << pid << "[注单]本次统计间隔时间[" << totalTime << "]s 请求超时时间[" << kTimeoutSeconds_ << "]s\n"
					<< "--- *** " << pid << "[注单]本次统计请求次数[" << requestNum << "] 成功[" << requestNumSucc << "] 失败[" << requestNumFailed << "] 命中率[" << ratio << "]\n"
					<< "--- *** " << pid << "[注单]最近一次请求耗时[" << timdiff * muduo::Timestamp::kMicroSecondsPerSecond / 1000 << "]ms [" << errmsg << "]\n"
					<< "--- *** " << pid << "[注单]平均请求耗时[" << avgTime * muduo::Timestamp::kMicroSecondsPerSecond / 1000 << "]ms\n"
					<< "--- *** " << pid << "[注单]每秒请求次数(QPS) = [" << avgNum << "] 单线程每秒请求处理数(TPS) = [" << testTPS << "] 预计每秒请求处理总数(TPS) = [" << testTPS * workerNumThreads_ << "]\n"
					<< "--- *** " << pid << "[注单]历史请求次数[" << requestNumTotal << "] 成功[" << requestNumTotalSucc << "] 失败[" << requestNumTotalFailed << "] 命中率[" << ratioTotal << "]\n\n";
#else
				std::stringstream ss; ss << json
					<< "--- *** " << pid << "[注单]I/O线程数[" << numThreads_ << "] 业务线程数[" << workerNumThreads_ << "] 累计接收请求数[" << numTotalReq_.get() << "] 累计未处理请求数[" << numTotalBadReq_.get() << "]\n"
					<< "--- *** " << pid << "[注单]本次统计间隔时间[" << totalTime << "]s 请求超时时间[" << kTimeoutSeconds_ << "]s\n"
					<< "--- *** " << pid << "[注单]本次统计请求次数[" << requestNum << "] 成功[" << requestNumSucc << "] 失败[" << requestNumFailed << "] 命中率[" << ratio << "]\n"
					<< "--- *** " << pid << "[注单]最近一次请求耗时[" << timdiff * muduo::Timestamp::kMicroSecondsPerSecond / 1000 << "]ms [" << errmsg << "]\n"
					<< "--- *** " << pid << "[注单]平均请求耗时[" << avgTime * muduo::Timestamp::kMicroSecondsPerSecond / 1000 << "]ms\n"
					<< "--- *** " << pid << "[注单]每秒请求次数(QPS) = [" << avgNum << "] 单线程每秒请求处理数(TPS) = [" << testTPS << "] 预计每秒请求处理总数(TPS) = [" << testTPS * workerNumThreads_ << "]\n"
					<< "--- *** " << pid << "[注单]历史请求次数[" << requestNumTotal << "] 成功[" << requestNumTotalSucc << "] 失败[" << requestNumTotalFailed << "] 命中率[" << ratioTotal << "]\n\n";
				//输出性能指标日志 ///
				//threadQPSLog_->run(std::bind(&OrderServer::refreshQPSLog, this, ss.str()));
#endif
				if (totalTime >= (double)deltaTime_) {
					//更新redis监控字段
					std::string monitordata = createMonitorData(latest, totalTime, kTimeoutSeconds_,
						requestNum, requestNumSucc, requestNumFailed, ratio,
						requestNumTotal, requestNumTotalSucc, requestNumTotalFailed, ratioTotal, testTPS);
					REDISCLIENT.set("s.monitor.order", monitordata);
				}
				//重置起始时间戳(微秒) ///
				orderTimeStart_ = timenow;
				//重置统计请求次数 ///
				numOrderRequest_.getAndSet(0);
				//重置统计成功次数 ///
				numOrderRequestSucc_.getAndSet(0);
				//重置统计失败次数 ///
				numOrderRequestFailed_.getAndSet(0);
			//}
			__sync_val_compare_and_swap(&value, 1, 0);
		}
#endif
#if 1
		//rsp.setBody(rspdata);
#else
		rsp.setContentType("application/xml;charset=utf-8");
		rsp.setBody(getRequestStr(req));
#endif
	}
	//刷新代理信息agent_info ///
	else if (req.path() == "/refreshAgentInfo") {
		//管理员挂维护/恢复服务 ///
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			if (!refreshAgentInfo()) {
				rsp.setBody("failed");
			}
			else {
				rsp.setBody("success");
			}
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//刷新白名单信息white_list ///
	else if (req.path() == "/refreshWhiteList") {
		//管理员挂维护/恢复服务 ///
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			refreshWhiteList();
			rsp.setBody("success");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务 ///
	else if (req.path() == "/repairApiServer") {
		//管理员挂维护/恢复服务 ///
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			if (!repairApiServer(req.query())) {
				rsp.setBody("failed");
			}
			else {
				rsp.setBody("success");
			}
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	// else if (req.path() == "/help") {
	// 	//管理员挂维护/恢复服务 ///
	// 	std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipNetEndian());
	// 	if (it != admin_list_.end()) {
	// 		rsp.setContentType("text/html;charset=utf-8");
	// 		rsp.setBody("<html>"
	// 			"<head><title>help</title></head>"
	// 			"<body>"
	// 			"<h4>/refreshAgentInfo</h4>"
	// 			"<h4>/refreshWhiteList</h4>"
	// 			"<h4>/repairApiServer?status=0|1(status=0挂维护 status=1恢复服务)</h4>"
	// 			"</body>"
	// 			"</html>");
	// 	}
	// 	else {
	// 		//HTTP应答包(header/body)
	// 		setFailedResponse(rsp,
	// 			muduo::net::HttpResponse::k404NotFound,
	// 			"HTTP/1.1 504 权限不够\r\n\r\n");
	// 	}
	// }
	else {
#if 1
		//HTTP应答包(header/body)
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#else
		rsp.setBody("<html><head><title>httpServer</title></head>"
			"<body><h1>Not Found</h1>"
			"</body></html>");
		//rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
#endif
	}
}

//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务 ///
void OrderServer::repairApiServerNotify(std::string const& msg) {
	int status;
	do {
		if (msg.empty() || (status = atol(msg.c_str())) < 0) {
			break;
		}
		//请求挂维护 ///
		if (status == ServiceRepairing) {
			/* 如果之前服务中, 尝试挂维护中, 并返回之前状态
			* 如果返回服务中, 说明刚好挂维护成功, 否则说明之前已被挂维护 */
			if (ServiceRunning == __sync_val_compare_and_swap(&server_state_, ServiceRunning, ServiceRepairing)) {
				//添加 "/GAME/ApiServersInvalid/192.168.2.93:8080"
				if (ZNONODE == m_zookeeperclient->existsNode(m_szInvalidNodePath)) {
					m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建节点 " << m_szInvalidNodePath;
				}
				//删除 "/GAME/ApiServers/192.168.2.93:8080"
				if (ZNONODE != m_zookeeperclient->existsNode(m_szNodePath)) {
					m_zookeeperclient->deleteNode(m_szNodePath);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除节点 " << m_szNodePath;
				}
			}
		}
		//请求恢复服务 ///
		else if (status == ServiceRunning) {
			/* 如果之前挂维护中, 尝试恢复服务, 并返回之前状态
			* 如果返回挂维护中, 说明刚好恢复服务成功, 否则说明之前已恢复服务 */
			if (ServiceRepairing == __sync_val_compare_and_swap(&server_state_, ServiceRepairing, ServiceRunning)) {
				//添加 "/GAME/ApiServers/192.168.2.93:8080"
				if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
					m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建节点 " << m_szNodePath;
				}
				//删除 "/GAME/ApiServersInvalid/192.168.2.93:8080"
				if (ZNONODE != m_zookeeperclient->existsNode(m_szInvalidNodePath)) {
					m_zookeeperclient->deleteNode(m_szInvalidNodePath);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除节点 " << m_szInvalidNodePath;
				}
			}
		}
	} while (0);
}

//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务 ///
bool OrderServer::repairApiServer(std::string const& reqStr) {
	std::string errmsg;
	int status;
	do {
		//解析参数 ///
		HttpParams params;
		if (!parseQuery(reqStr, params, errmsg)) {
			break;
		}
		HttpParams::const_iterator statusKey = params.find("status");
		if (statusKey == params.end() || statusKey->second.empty() ||
			(status = atol(statusKey->second.c_str())) < 0) {
			break;
		}
		//请求挂维护 ///
		if (status == ServiceRepairing) {
			/* 如果之前服务中, 尝试挂维护中, 并返回之前状态
			* 如果返回服务中, 说明刚好挂维护成功, 否则说明之前已被挂维护 */
			if (ServiceRunning == __sync_val_compare_and_swap(&server_state_, ServiceRunning, ServiceRepairing)) {
				//添加 "/GAME/ApiServersInvalid/192.168.2.93:8080"
				if (ZNONODE == m_zookeeperclient->existsNode(m_szInvalidNodePath)) {
					m_zookeeperclient->createNode(m_szInvalidNodePath, m_szNodeValue, true);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建节点 " << m_szInvalidNodePath;
				}
				//删除 "/GAME/ApiServers/192.168.2.93:8080"
				if (ZNONODE != m_zookeeperclient->existsNode(m_szNodePath)) {
					m_zookeeperclient->deleteNode(m_szNodePath);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除节点 " << m_szNodePath;
				}
			}
			return true;
		}
		//请求恢复服务 ///
		else if (status == ServiceRunning) {
			/* 如果之前挂维护中, 尝试恢复服务, 并返回之前状态
			* 如果返回挂维护中, 说明刚好恢复服务成功, 否则说明之前已恢复服务 */
			if (ServiceRepairing == __sync_val_compare_and_swap(&server_state_, ServiceRepairing, ServiceRunning)) {
				//添加 "/GAME/ApiServers/192.168.2.93:8080"
				if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
					m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建节点 " << m_szNodePath;
				}
				//删除 "/GAME/ApiServersInvalid/192.168.2.93:8080"
				if (ZNONODE != m_zookeeperclient->existsNode(m_szInvalidNodePath)) {
					m_zookeeperclient->deleteNode(m_szInvalidNodePath);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除节点 " << m_szInvalidNodePath;
				}
			}
			return true;
		}
	} while (0);
	return false;
}

// 检查代理线是否维护waa
void OrderServer::refreshRepairAgentInfo()
{
	uphold_agent_ids.clear();
	try
	{
		LOG_WARN << "---" << __FUNCTION__ << "---------------";

		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["maintain_proxy"];
		auto findvalue = coll.find_one(document{} << "apiswitch" << 0 << finalize);
		if (findvalue)
		{
			auto findview = findvalue->view();
			auto t_agent_ids = findview["proxylist"].get_array();
			LOG_INFO << "-------t agent ids get_array--------";
			{
				MutexLockGuard lock(m_mutex_ids);
				for (auto &agent_id : t_agent_ids.value)
				{
					uphold_agent_ids.push_back(agent_id.get_int32());
					LOG_INFO << "agent ids:" << agent_id.get_int32();
				}
			}

			LOG_WARN << "---" << __FUNCTION__ << " end---------------";
		}
	}
	catch (bsoncxx::exception &e)
	{
		LOG_ERROR << "---refresh Repair Agent Info: " << e.what();
	}
}


//刷新所有agent_info信息 ///
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
//3.redis广播通知刷新一次
bool OrderServer::refreshAgentInfo()
{
	//LOG_DEBUG << __FUNCTION__;
	WRITE_LOCK(agent_info_mutex_);
	agent_info_.clear();
	try
	{
		//读取proxy_info
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["proxy_info"];
		//查询proxy_info表，获取agentid，score,status,cooperationtype,md5code,descode
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{}
			<< "agentid" << 1 << "score" << 1
			<< "status" << 1 << "cooperationtype" << 1
			<< "md5code" << 1 << "descode" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::cursor cursor = coll.find({}, opts);
		for (auto& doc : cursor) {
			//LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(doc);
			int32_t agentId = doc["agentid"].get_int32();
			agent_info_t& _agent_info = agent_info_[agentId];
			//agentid
			_agent_info.agentId = agentId;
			//score
			switch (doc["score"].type()) {
			case bsoncxx::type::k_int64:
				_agent_info.score = doc["score"].get_int64();
				break;
			case bsoncxx::type::k_int32:
				_agent_info.score = doc["score"].get_int32();
				break;
			default:
				_agent_info.score = 0;
				break;
			}
			_agent_info.status = doc["status"].get_int32();
			_agent_info.cooperationtype = doc["cooperationtype"].get_int32();
			_agent_info.md5code = doc["md5code"].get_utf8().value.to_string();
			_agent_info.descode = doc["descode"].get_utf8().value.to_string();
		}
	}
	catch (const bsoncxx::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const muduo::Exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const std::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (...) {
		LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught ";
		throw;
	}
	//std::string format;
	for (std::map<int32_t, agent_info_t>::const_iterator it = agent_info_.begin();
		it != agent_info_.end(); ++it) {
		//std::stringstream ss;
		LOG_DEBUG/*ss*/ << "--- *** " << "代理信息\n"
			<<"--- *** agentId[" << it->second.agentId
			<< "] score[" << it->second.score
			<< "] status[" << it->second.status
			<< "] md5code[" << it->second.md5code
			<< "] descode[" << it->second.descode
			<< "] cooperationtype[" << it->second.cooperationtype << "]";
		//format += ss.str().c_str();
	}
	//std::cout << format << std::endl;
	//LOG_DEBUG << "--- *** " << "代理信息" << format;
	return true;
}


//刷新所有IP访问白名单信息 ///
//1.web后台更新白名单通知刷新
//2.游戏启动刷新一次
//3.redis广播通知刷新一次 ///
void OrderServer::refreshWhiteList() {
	//开启了IP访问白名单功能 ///
	if (whiteListControl_ == eWhiteListCtrl::OpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查 ///
		httpServer_.getLoop()->runInLoop(std::bind(&OrderServer::refreshWhiteListInLoop, this));
	}
	else if (whiteListControl_ == eWhiteListCtrl::Open) {
		//同步刷新IP访问白名单
		refreshWhiteListSync();
	}
}

//同步刷新IP访问白名单
bool OrderServer::refreshWhiteListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	assert(whiteListControl_ == eWhiteListCtrl::Open);
	WRITE_LOCK(white_list_mutex_);
	white_list_.clear();
	try
	{
		//读取ip_white_list
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["ip_white_list"];
		//查询ip_white_list表，获取ipaddress，ipstatus
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "ipaddress" << 1 << "ipstatus" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::cursor cursor = coll.find(/*{}*/
			bsoncxx::builder::stream::document{}
			<< "isapiuse" << bsoncxx::types::b_bool{ 1 }
			<< "ipstatus" << bsoncxx::types::b_int32{ 0 }
			<< bsoncxx::builder::stream::finalize, opts);
		for (auto& doc : cursor) {
			std::string ipaddr;
			//IP访问状态 ///
			eApiVisit status = eApiVisit::Disable;
			// LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(doc);
			//ipaddress
			if (doc["ipaddress"]) {
				switch (doc["ipaddress"].type()) {
				case bsoncxx::type::k_utf8:
					ipaddr = doc["ipaddress"].get_utf8().value.to_string();
					break;
				}
			}
			//ipstatus
			if (doc["ipstatus"]) {
				switch (doc["ipstatus"].type()) {
				case bsoncxx::type::k_int32:
					//0允许访问 1禁止访问
					status = (doc["ipstatus"].get_int32() > 0) ?
						eApiVisit::Disable : eApiVisit::Enable;
					break;
				}
			}
			//检查ipaddr有效性 ///
			if (!ipaddr.empty() &&
				boost::regex_match(ipaddr,
				boost::regex(
					"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
					"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				white_list_[addr.ipNetEndian()] = status;
			}
		}
	}
	catch (const bsoncxx::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const muduo::Exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const std::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (...) {
		LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught ";
		return false;
	}
	//std::string format;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.begin();
		it != white_list_.end(); ++it) {
		//std::stringstream ss;
		LOG_DEBUG/*ss*/ << "--- *** " << "IP访问白名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
		//format += ss.str().c_str();
	}
	//std::cout << format << std::endl;
	//LOG_DEBUG << "--- *** " << "IP访问白名单" << format;
	return true;
}

bool OrderServer::refreshWhiteListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	assert(whiteListControl_ == eWhiteListCtrl::OpenAccept);
	//安全断言 ///
	httpServer_.getLoop()->assertInLoopThread();
	white_list_.clear();
	try
	{
		//读取ip_white_list
		mongocxx::collection coll = MONGODBCLIENT["gamemain"]["ip_white_list"];
		//查询ip_white_list表，获取ipaddress，ipstatus
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "ipaddress" << 1 << "ipstatus" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::cursor cursor = coll.find({}, opts);
		for (auto& doc : cursor) {
			std::string ipaddr;
			//IP访问状态 ///
			eApiVisit status = eApiVisit::Disable;
			// LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(doc);
			//ipaddress
			if (doc["ipaddress"]) {
				switch (doc["ipaddress"].type()) {
				case bsoncxx::type::k_utf8:
					ipaddr = doc["ipaddress"].get_utf8().value.to_string();
					break;
				}
			}
			//ipstatus
			if (doc["ipstatus"]) {
				switch (doc["ipstatus"].type()) {
				case bsoncxx::type::k_int32:
					//0允许访问 1禁止访问
					status = (doc["ipstatus"].get_int32() > 0) ?
						eApiVisit::Disable : eApiVisit::Enable;
					break;
				}
			}
			//检查ipaddr有效性 ///
			if (!ipaddr.empty() &&
				boost::regex_match(ipaddr,
					boost::regex(
						"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				white_list_[addr.ipNetEndian()] = status;
			}
		}
	}
	catch (const bsoncxx::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const muduo::Exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (const std::exception & e) {
		LOG_DEBUG << "MongoDBException: " << e.what();
		return false;
	}
	catch (...) {
		LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught ";
		return false;
	}
	//std::string format;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.begin();
		it != white_list_.end(); ++it) {
		//std::stringstream ss;
		LOG_DEBUG/*ss*/ << "--- *** " << "IP访问白名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
		//format += ss.str().c_str();
	}
	//std::cout << format << std::endl;
	//LOG_DEBUG << "--- *** " << "IP访问白名单" << format;
	return true;
}

//redis获取用户登陆信息 ///
bool OrderServer::userLoginProxyInfo(const string& session, int64_t& userId, string& account, uint32_t& agentId)
{
	std::string value;
	bool ret = REDISCLIENT.get(session, value);
	if (ret) {
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

#ifdef _TCP_NOTIFY_CLIENT_
//订单服[C]端 -> 网关服[S]端 ///
//发送网关用户数据 先查询Redis得到userId的网关服再发送 ///
void OrderServer::sendUserData(int64_t userId, uint8_t mainId, uint8_t subId, char const* msgdata, size_t dataLen)
{
	assert(msgdata && dataLen);
	//////////////////////////////////////////////////////////////////////////
	//玩家登陆网关服信息
	//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
	//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
	//网关服ID格式：session|ip:port:port:pid
	//第一个ip:port是网关服监听客户端的标识
	//第二个ip:port是网关服监听订单服的标识
	//pid标识网关服进程id
	//////////////////////////////////////////////////////////////////////////
	std::string proxyInfo;
	REDISCLIENT.get("s.uid:" + std::to_string(userId) + ":proxy", proxyInfo);
	if (!proxyInfo.empty()) {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, proxyInfo, boost::is_any_of("|"));
		std::string const& session = vec[0];
		std::string const& ipaddr = vec[1];
		LOG_DEBUG << "--- *** " << "ipaddr = " << ipaddr;
		//获取弱指针 ///
		muduo::net::WeakTcpConnectionPtr weakConn;
		{
			//userId所在ProxyServer weakConn对象 ///
			//根据ProxyServer servid 检索，即 ipaddr=ip:port:port:pid ///
			muduo::MutexLockGuard lock(mutexWeakConns_);
			WeakConnMap::const_iterator it = weakConns_.find(ipaddr);
			if (it != weakConns_.end()) {
				//muduo::net::WeakTcpConnectionPtr& weakConn = it->second;
				weakConn = it->second;
			}
		}
		//发送数据 ///
		muduo::net::TcpConnectionPtr conn(weakConn.lock());
		if (conn) {
			//整包大小sizeof(internal_prev_header) + sizeof(Header) + dataLen ///
			std::vector<uint8_t> data(sizeof(internal_prev_header) + sizeof(Header) + dataLen);
			//偏移量sizeof(internal_prev_header) + sizeof(Header) SerializeToArray ///
			memcpy(&data[sizeof(internal_prev_header) + sizeof(Header)], msgdata, dataLen);
			LOG_DEBUG << "--- *** " << "整包大小 = " << data.size();
			//内部消息头internal_prev_header ///
			internal_prev_header internal_header = { 0 };
			//整包大小sizeof(internal_prev_header) + sizeof(Header) + dataLen ///
			internal_header.len = data.size();
			//userId ///
			internal_header.userId = userId;
			//session ///
			memcpy(internal_header.session, session.c_str(),
				std::min(sizeof(internal_header.session), session.length()));
			internal_prev_header* internal_header_ = (internal_prev_header*)(&data[0]);
			memcpy(internal_header_, (char const*)(&internal_header), sizeof(internal_prev_header));
			//命令消息头Header ///
			Header command_header = { 0 };
			command_header.len = data.size() - sizeof(internal_prev_header);
			command_header.ver = PROTO_VER;
			command_header.sign = HEADER_SIGN;
			command_header.mainId = mainId;
			command_header.subId = subId;
			command_header.encType = PUBENC__PROTOBUF_NONE;
			//用户protobuf大小 除去内部消息头(internal_prev_header) + 命令消息头(Header) ///
			command_header.realSize = data.size() - sizeof(internal_prev_header) - sizeof(Header);
			Header* command_header_ = (Header*)(&data[sizeof(internal_prev_header)]);
			memcpy(command_header_, (char const*)(&command_header), sizeof(Header));
			//CRC校验位 command_header.ver ~ command_header.realSize + 用户protobuf ///
			command_header.crc = GlobalFunc::GetCheckSum(&data[sizeof(internal_prev_header) + 4], command_header.len - 4);
			command_header_->crc = command_header.crc;
			TRACE_COMMANDID(command_header.mainId, command_header.subId);
			conn->send(data.data(), data.size());
			//LOG_DEBUG << "--- *** " 
			//	<< "\nuserid = " << userId << " session = " << session << " proxy.servid = " << ipaddr;
		}
		else {
			LOG_DEBUG << "--- *** " << "conn invalid"
				<< "\nuserid = " << userId << " session = " << session << " proxy.servid = " << ipaddr;
		}
	}
	else {
		LOG_DEBUG << "--- *** " << "offline userid = " << userId;
	}
	
}
#endif


// 检查代理线是否维护
bool OrderServer::checkAgentStatus(int32_t agentid)
{
	MutexLockGuard lock(m_mutex_ids);
    // 遍历判断
    for (int32_t _agentId : uphold_agent_ids)
    {
        if (agentid == _agentId)
        {
            LOG_WARN << "---代理线[" << agentid <<"]维护中！";
            return false;
        }
    }

	return true;
}

//订单处理函数 ///
std::string OrderServer::OrderProcess(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& reqStr, muduo::Timestamp receiveTime, int& errcode, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS) {
	int opType = 0;
	int agentId = 0;
#ifdef _NO_LOGIC_PROCESS_
	int64_t userId = 0;
#endif
	std::string account;
	std::string orderId;
	double score = 0;
	int64_t scoreI64 = 0;
	std::string timestamp;
	std::string paraValue, key;
	agent_info_t /*_agent_info = { 0 },*/* p_agent_info = NULL;
	//
 
	do {
		//解析参数 ///
		HttpParams params;
		if (!parseQuery(reqStr, params, errmsg)) {
			// 传输参数错误
			errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "bad request";
			break;
		}
		//明文请求，不解码 ///
		if (!isdecrypt_) {
			//type
			HttpParams::const_iterator typeKey = params.find("type");
			if (typeKey == params.end() || typeKey->second.empty() ||
				(opType = atol(typeKey->second.c_str())) < 0) {
				// 操作类型参数错误
				errcode = eApiErrorCode::GameHandleOperationTypeError;
				errmsg += "type ";
			}
			//2.上分 3.下分
			if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
				// 操作类型参数错误
				errcode = eApiErrorCode::GameHandleOperationTypeError;
				errmsg += "type value ";
			}
			//agentid
			HttpParams::const_iterator agentIdKey = params.find("agentid");
			if (agentIdKey == params.end() || agentIdKey->second.empty() ||
				(agentId = atol(agentIdKey->second.c_str())) <= 0) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "agentid ";
			} 

#ifdef _NO_LOGIC_PROCESS_
			//userid
			HttpParams::const_iterator userIdKey = params.find("userid");
			if (userIdKey == params.end() || userIdKey->second.empty() ||
				(userId = atoll(userIdKey->second.c_str())) <= 0) {
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "userid ";
			}
#endif
			//account
			HttpParams::const_iterator accountKey = params.find("account");
			if (accountKey == params.end() || accountKey->second.empty()) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "account ";
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = params.find("orderid");
			if (orderIdKey == params.end() || orderIdKey->second.empty()) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "orderid ";
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = params.find("score");

			if (scoreKey == params.end() || scoreKey->second.empty() || !IsDigitStr(scoreKey->second) ||
				(score = floors(scoreKey->second.c_str())) <= 0 || NotScore(score)) {

				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "score ";
			}
			//有错误发生 ///
			if (errcode != 0) {
				errmsg += "invalid";
				break;
			}
			// 获取当前代理数据
			//agent_info_t _agent_info = { 0 };
			{
				READ_LOCK(agent_info_mutex_);
				std::map<int32_t, agent_info_t>::/*const_*/iterator it = agent_info_.find(agentId);
				if (it == agent_info_.end()) {
					// 代理ID不存在或代理已停用
					errcode = eApiErrorCode::GameHandleProxyIDError;
					errmsg = "agent_info not found";
					break;
				}
				else {
					p_agent_info = &it->second;
				}
			}
			// 没有找到代理，判断代理的禁用状态(0正常 1停用)
			if (p_agent_info->status == 1) {
				// 代理ID不存在或代理已停用
				errcode = eApiErrorCode::GameHandleProxyIDError;
				errmsg = "agent.status error";
				break;
			}
			scoreI64 = rate100(scoreKey->second);
#ifndef _NO_LOGIC_PROCESS_
			//上下分操作 ///
			errcode = doOrderExecute(opType, account, score, scoreI64, *p_agent_info, orderId, errmsg, latest, testTPS);
#endif
			break;
		}
		//type
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty() ||
			(opType = atol(typeKey->second.c_str())) < 0) {
			// 操作类型参数错误
			errcode = eApiErrorCode::GameHandleOperationTypeError;
			errmsg = "type invalid";
			break;
		}
		//2.上分 3.下分
		if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
			// 操作类型参数错误
			errcode = eApiErrorCode::GameHandleOperationTypeError;
			errmsg = "type value invalid";
			break;
		}
		//agentid
		HttpParams::const_iterator agentIdKey = params.find("agentid");
		if (agentIdKey == params.end() || agentIdKey->second.empty() ||
			(agentId = atol(agentIdKey->second.c_str())) <= 0) {
			// 传输参数错误
			errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "agentid invalid";
			break;
		}

		// 单线路维护过滤
		if(!checkAgentStatus(agentId)){
			errcode = eApiErrorCode::GameMaintenanceError;
			errmsg = "agentid is in maintenance ";
			break;
		}
		
		//timestamp
		HttpParams::const_iterator timestampKey = params.find("timestamp");
		if (timestampKey == params.end() || timestampKey->second.empty() ||
			atol(timestampKey->second.c_str()) <= 0) {
			// 传输参数错误
			errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "timestamp invalid";
			break;
		}
		else {
			timestamp = timestampKey->second;
		}
		//paraValue
		HttpParams::const_iterator paramValueKey = params.find("paraValue");
		if (paramValueKey == params.end() || paramValueKey->second.empty()) {
			// 传输参数错误
			errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "paraValue invalid";
			break;
		}
		else {
			paraValue = paramValueKey->second;
		}
		//key
		HttpParams::const_iterator keyKey = params.find("key");
		if (keyKey == params.end() || keyKey->second.empty()) {
			// 传输参数错误
			errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "key invalid";
			break;
		}
		else {
			key = keyKey->second;
		}
		// 获取当前代理数据
		//agent_info_t _agent_info = { 0 };
		{
			READ_LOCK(agent_info_mutex_);
			std::map<int32_t, agent_info_t>::/*const_*/iterator it = agent_info_.find(agentId);
			if (it == agent_info_.end()) {
				// 代理ID不存在或代理已停用
				errcode = eApiErrorCode::GameHandleProxyIDError;
				errmsg = "agent_info not found";
				break;
			}
			else {
				p_agent_info = &it->second;
			}
		}
		// 没有找到代理，判断代理的禁用状态(0正常 1停用)
		if (p_agent_info->status == 1) {
			// 代理ID不存在或代理已停用
			errcode = eApiErrorCode::GameHandleProxyIDError;
			errmsg = "agent.status error";
			break;
		}
#if 0
		agentId = 10000;
		p_agent_info->md5code = "334270F58E3E9DEC";
		p_agent_info->descode = "111362EE140F157D";
		timestamp = "1579599778583";
		paraValue = "0RJ5GGzw2hLO8AsVvwORE2v16oE%2fXSjaK78A98ct5ajN7reFMf9YnI6vEnpttYHK%2fp04Hq%2fshp28jc4EIN0aAFeo4pni5jxFA9vg%2bLOx%2fek%3d";
		key = "C6656A2145BAEF7ED6D38B9AF2E35442";
#elif 0
		agentId = 111169;
		p_agent_info->md5code = "8B56598D6FB32329";
		p_agent_info->descode = "D470FD336AAB17E4";
		timestamp = "1580776071271";
		paraValue = "h2W2jwWIVFQTZxqealorCpSfOwlgezD8nHScU93UQ8g%2FDH1UnoktBusYRXsokDs8NAPFEG8WdpSe%0AY5rtksD0jw%3D%3D";
		key = "a7634b1e9f762cd4b0d256744ace65f0";
#elif 0
		agentId = 111190;
		timestamp = "1583446283986";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		paraValue = "KDtKjjnaaxKWNuK%252BBRwv9f2gBxLkSvY%252FqT4HBaZY2IrxqGMK3DYlWOW4dHgiMZV8Uu66h%252BHjH8MfAfpQIE5eIHoRZMplj7dljS7Tfyf3%252BlM%253D";
		key = "4F6F53FDC4D27EC33B3637A656DD7C9F";
#elif 0
		agentId = 111149;
		timestamp = "1583448714906";
		p_agent_info->md5code = "7196FD6921347DB1";
		p_agent_info->descode = "A5F8C07B7843C73E";
		paraValue = "nu%252FtdBhN6daQdad3aOVOgzUr6bHVMYNEpWE4yLdHkKRn%252F%252Fn1V3jIFR%252BI7wawXWNyjR3%252FW0M9qzcdzM8rNx8xwe%252BEW9%252BM92ZN4hlpUAhFH74%253D";
		key = "9EEC240FAFAD3E5B6AB6B2DDBCDFE1FF";
#elif 0
		agentId = 10033;
		timestamp = "1583543899005";
		p_agent_info->md5code = "5998F124C180A817";
		p_agent_info->descode = "38807549DEA3178D";
		paraValue = "9303qk%2FizHRAszhN33eJxG2aOLA2Wq61s9f96uxDe%2Btczf2qSG8O%2FePyIYhVAaeek9m43u7awgra%0D%0Au8a8b%2FDchcZSosz9SVfPjXdc7h4Vma2dA8FHYZ5dJTcxWY7oDLlSOHKVXYHFMIWeafVwCN%2FU5fzv%0D%0AHWyb1Oa%2FWJ%2Bnfx7QXy8%3D";
		key = "2a6b55cf8df0cd8824c1c7f4308fd2e4";
#elif 0
		agentId = 111190;
		timestamp = "1583543899005";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		paraValue = "%252FPxIlQ9UaP6WljAYhfYZpBO6Hz2KTrxGN%252Bmdffv9sZpaii2avwhJn3APpIOozbD7T3%252BGE1rh5q4OdfrRokriWNEhlweRKzC6%252FtABz58Kdzo%253D";
		key = "9F1E44E8B61335CCFE2E17EE3DB7C05A";
#elif 0
		p_agent_info->descode = "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw";
		paraValue = "7q9Mu4JezLGpXaJcaxBUEmnQgrH%2BO0Wi%2F85z30vF%2FIdphHU13EMp2f%2FE5%2BfHtIXYFLbyNqnwDx8SyGP1cSYssZN6gniqouFeB3kBcwSXYZbFYhNBU6162n6BaFYFVbH6KMc43RRvjBtmbkMgCr5MlRz0Co%2BQEsX9Pt3zFJiXnm12oEdLeFBSSVIcDsqd3ize9do1pbxAm9Bb45KRvPhYvA%3D%3D";
#endif
		//解码 ///
		std::string decrypt;
		{
			//根据代理商信息中存储的md5密码，结合传输参数中的agentid和timestamp，生成判定标识key
			std::string src = std::to_string(agentId) + timestamp + p_agent_info->md5code;
			char md5[32 + 1] = { 0 };
			MD5Encode32(src.c_str(), src.length(), md5, 1);
			if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
				// 代理MD5校验码错误
				errcode = eApiErrorCode::GameHandleProxyMD5CodeError;
				errmsg = "md5 check error";
				break;
			}

			//HTML::Decode ///
			paraValue = HTML::Decode(paraValue);
			LOG_DEBUG << "--- *** " << "HTML::Decode >>> " << paraValue;

			//URL::Decode 1次或2次解码 ///
			for (int c = 1; c < 3; ++c) {
				//URL::MultipleDecode
				paraValue = URL::MultipleDecode(paraValue);
#if 1
				//"\r\n"
				boost::replace_all<std::string>(paraValue, "\r\n", "");
				//"\r"
				boost::replace_all<std::string>(paraValue, "\r", "");
				//"\r\n"
				boost::replace_all<std::string>(paraValue, "\n", "");
#else
				paraValue = boost::regex_replace(paraValue, boost::regex("\r\n|\r|\n"), "");
#endif
				LOG_DEBUG << "--- *** " << "URL::Decode[" << c << "] >>> " << paraValue/*strURL*/;

				std::string const& strURL = paraValue;
				decrypt = Crypto::AES_ECBDecrypt(strURL, p_agent_info->descode);
				LOG_DEBUG << "--- *** " << "ECBDecrypt[" << c
					<< "] >>> md5code[" << p_agent_info->md5code
					<< "] descode[" << p_agent_info->descode << "] [" << decrypt << "]";

				if (!decrypt.empty()) {
					//成功
					break;
				}
			}

			//Base64Decode ///
			//std::string strBase64 = Base64::Decode(strURL);
			//LOG_DEBUG << "--- *** " << "base64Decode\n" << strBase64;

			//AES校验 ///
#if 0
			//decrypt = Landy::Crypto::ECBDecrypt(p_agent_info->descode, (unsigned char const*)strURL.c_str());
			//"\006\006\006\006\006\006"
			//boost::replace_all<std::string>(decrypt, "\006", "");
#endif
			//decrypt = Crypto::AES_ECBDecrypt(strURL, p_agent_info->descode);
			//LOG_DEBUG << "--- *** " << "ECBDecrypt >>> " << decrypt;

			if (decrypt.empty()) {
				// 参数转码或代理解密校验码错误
				errcode = eApiErrorCode::GameHandleProxyDESCodeError;
				errmsg = "DESDecrypt failed, AES_set_decrypt_key error";
				break;
			}
		}
		//解析内部参数 ///
		{
			HttpParams decryptParams;
			if (!parseQuery(decrypt, decryptParams, errmsg)) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg = "DESDecrypt ok, but bad request paramValue";
				break;
			}
			//agentid
			//HttpParams::const_iterator agentIdKey = decryptParams.find("agentid");
			//if (agentIdKey == decryptParams.end() || agentIdKey->second.empty()) {
			//	break;
			//}
#ifdef _NO_LOGIC_PROCESS_
			//userid
	 		HttpParams::const_iterator userIdKey = decryptParams.find("userid");
	 		if (userIdKey == decryptParams.end() || userIdKey->second.empty() ||
	 			(userId = atoll(userIdKey->second.c_str())) <= 0) {
				errcode = eApiErrorCode::GameHandleParamsError;
	 			errmsg += "userid ";
	 		}
#endif
			//account
			HttpParams::const_iterator accountKey = decryptParams.find("account");
			if (accountKey == decryptParams.end() || accountKey->second.empty()) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "account ";
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = decryptParams.find("orderid");
			if (orderIdKey == decryptParams.end() || orderIdKey->second.empty()) {
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "orderid ";
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = decryptParams.find("score");
#if 0
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !IsDigitStr(scoreKey->second) ||
				(score = atoll(scoreKey->second.c_str())) <= 0) {
#else
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !IsDigitStr(scoreKey->second) ||
				(score = floors(scoreKey->second.c_str())) <= 0 || NotScore(score)) {
#endif
				// 传输参数错误
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "score ";
			}
			//有错误发生 ///
			if (errcode != 0) {
				errmsg += "invalid";
				break;
			}
			scoreI64 = rate100(scoreKey->second);
		}
		//conn session orderid
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		entryContext->setSession(orderId);
#ifndef _NO_LOGIC_PROCESS_
		//上下分操作 ///
		errcode = doOrderExecute(opType, account, score, scoreI64, *p_agent_info, orderId, errmsg, latest, testTPS);
#endif
	} while (0);

#ifdef _TCP_NOTIFY_CLIENT_
#ifdef _NO_LOGIC_PROCESS_
	//上下分操作通知客户端 ///
#if 0
	int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_HALL;
	int subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_ORDERMESSAGE_NOTIFY;
	::HallServer::OrderNotifyMessage rspdata;
	::Game::Common::Header* header = rspdata.mutable_header();
	header->set_sign(HEADER_SIGN);
	rspdata.set_addorsubflag(int(opType));//2.上分 3.下分
	rspdata.set_userid(userId);//userId
	rspdata.set_score(score);
	rspdata.set_retcode(errcode);
	rspdata.set_errmsg(errmsg);
	std::string content = rspdata.SerializeAsString();
	sendUserData(userId, mainId, subId, content.data(), content.length());
#elif 1
	int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
	int subId = ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE;
	::Game::Common::ProxyNotifyOrderScoreMessage rspdata;
	::Game::Common::Header* header = rspdata.mutable_header();
	header->set_sign(HEADER_SIGN);
	rspdata.set_userid(userId);//userId
	rspdata.set_score(score);
	std::string content = rspdata.SerializeAsString();
	sendUserData(userId, mainId, subId, content.data(), content.length());
#endif
#endif
#endif
	//调试模式下，打印从接收网络请求(receive)到处理完逻辑业务所经历时间dt(s) ///
	//if (g_bisDebug) {
		char ch[50] = { 0 };
		snprintf(ch, sizeof(ch), " dt(%.6fs)", muduo::timeDifference(muduo::Timestamp::now(), receiveTime));
		errmsg += ch;
	//}
	//json格式应答消息 ///
	std::string json = createResponse(opType, orderId, agentId, account, score, errcode, errmsg, g_bisDebug);
	/*LOG_WARN*/LOG_DEBUG << "--- *** " << "\n" << json;
	
	return json;
}

//上下分操作 ///
int OrderServer::doOrderExecute(int32_t opType, std::string const& account, double score, int64_t scoreI64, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS)
{
	int errcode = eApiErrorCode::NoError;
	do {
		//上分操作
		if (opType == int(eApiType::OpAddScore)) {

			/*LOG_WARN*/LOG_INFO << __FUNCTION__ << " --- *** " << "上分REQ "
				<< "orderId[" << orderId << "] "
				<< "account[" << account << "]" << " agentId[" << _agent_info.agentId << "]"
				<< " deltascore[" << score << "]";

			//处理上分请求 ///
			errcode = AddOrderScore(account, scoreI64/*int64_t(score * 100)*/, _agent_info, orderId, errmsg, latest, testTPS);

			/*LOG_WARN*/LOG_INFO << __FUNCTION__ << " --- *** " << "上分RSP "
				<< "\norderId[" << orderId << "] " << "account[" << account << "] status[" << errcode << "] errmsg[" << errmsg << "]";
		}
		//下分操作
		else /*if (opType == int(eApiType::OpSubScore))*/ {

			LOG_INFO << __FUNCTION__ << " --- *** " << "下分REQ " << "orderId[" << orderId << "] " << "account[" << account << "]" << " agentId[" << _agent_info.agentId << "]"
				<< " deltascore[" << score << "]";

			int32_t dbstatus = 0;

			mongocxx::collection userCollection = (*dbgamemain_)["game_user"];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1 << "score" << 1 << "status" << 1 << bsoncxx::builder::stream::finalize);
			auto find_result = userCollection.find_one(bsoncxx::builder::stream::document{}
					<< "agentid" << bsoncxx::types::b_int32{_agent_info.agentId}
					<< "account" << account
					<< bsoncxx::builder::stream::finalize,opts);
			if (find_result)
			{
                bsoncxx::document::view view = find_result->view();
				int64_t userId = view["userid"].get_int64();
				int64_t beforeScore = view["score"].get_int64();

				dbstatus    = view["status"].get_int32();

				// 3，增加玩家视讯游戏在线状态判断(API版本已经去掉视讯了)
				// int32_t txhstatus = 0;
				// if (view["txhstatus"]) {
				// 	txhstatus = view["txhstatus"].get_int32();
				// }
				// LOG_INFO << "---"<< __FUNCTION__ <<" userId["<< userId <<"],beforeScore[" << beforeScore << "],txhstatus[" << txhstatus <<"]";
				// int64_t recycleScore = 0;
				// if( txhstatus > 0 && beforeScore == 0 ){
				// 	READ_LOCK(recycle_mutex_);
				// 	m_TxhAPI->recycleBalance((int32_t)eRecycleType::one,to_string(userId),recycleScore);
				// 	LOG_INFO << "---recycleBalance finish recycleScore[" << recycleScore << "]";
				// }
			}
			
			// 如果玩家已经冻结
			if( dbstatus == 1 )
			{
				errcode = eApiErrorCode::GameAccountFreezeError;
				errmsg = "account[" + account + "] is freeze.";
				break;
			}

			//处理下分请求 ///
			errcode = SubOrderScore(account, scoreI64 /*int64_t(score * 100)*/, _agent_info, orderId, errmsg, latest, testTPS);


			LOG_INFO << __FUNCTION__ << " --- *** "
								  << "下分RSP "
								  << "orderId[" << orderId << "] "
								  << "account[" << account << "] status[" << errcode << "] errmsg[" << errmsg << "]";
		}
	} while (0);
	return errcode;
}

//上分写db操作 ///
int OrderServer::AddOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId,
	std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS)
{
	int errcode = 0;
	errmsg.clear();
	int32_t onlinestatus = 0;
	int64_t beforeScore = 0;
	int64_t userId = 0;
	std::string linecode;
	mongocxx::client_session sessiondb = MONGODBCLIENT.start_session();
	bool bStartedTransaction = false;
#ifdef _STAT_ORDER_QPS_DETAIL_
	//std::stringstream ss;
	muduo::Timestamp st_collect = muduo::Timestamp::now();
#endif
	do {
		try
		{
#ifdef _USE_SCORE_ORDERS_
			mongocxx::collection addScoreColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["score_orders"];
#else
			mongocxx::collection addScoreColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["add_score_order"];
#endif
			mongocxx::collection userCollection = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["game_user"];
			mongocxx::collection agentCollection = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["proxy_info"];
			mongocxx::collection scoreChangeColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["user_score_record"];
			mongocxx::collection scoreAddLastColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["user_score_add_last_record"];

#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_collect = muduo::Timestamp::now();
			//ss << "\n0.[mongo]getCollection elapsed " << muduo::timeDifference(et_collect, st_collect) << "s";
			createLatestElapsed(latest, "mongo.getCollection", "", muduo::timeDifference(et_collect, st_collect));
			muduo::Timestamp st_user_q = et_collect;
#endif
			//查询game_user表，获取指定userid的score，onlinestatus
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1 << "score" << 1 << "onlinestatus" << 1 << "linecode" << 1
				<< bsoncxx::builder::stream::finalize);
			bsoncxx::stdx::optional<bsoncxx::document::value> result = userCollection.find_one(
				bsoncxx::builder::stream::document{}
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "account" << account
				<< bsoncxx::builder::stream::finalize, opts);
			if (!result) {
				{
					//用户不存在
					std::stringstream ss;
					ss << "orderid." << orderId << " query game_user agentid." << _agent_info.agentId << ".account." << account << " invalid";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家不存在
					errcode = eApiErrorCode::AddScoreHandleUserNotExistsError;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_q = muduo::Timestamp::now();
				//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
				createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
#endif
				break;
			}
			//bsoncxx::to_json(doc) bsoncxx::to_json(view)
			//LOG_DEBUG << "--- *** " << "QueryResult: " << bsoncxx::to_json(*result);
			bsoncxx::document::view view = result->view();
			//userid
			userId = view["userid"].get_int64();
			//玩家当前积分
			switch (view["score"].type()) {
			case bsoncxx::type::k_int64:
				beforeScore = view["score"].get_int64();
				break;
			case bsoncxx::type::k_int32:
				beforeScore = view["score"].get_int32();
				break;
			}
			//玩家在线状态 没有这个字段会抛异常 ///
			if (view["onlinestatus"]) {
				switch (view["onlinestatus"].type()) {
				case bsoncxx::type::k_int32:
					onlinestatus = view["onlinestatus"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					onlinestatus = view["onlinestatus"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					break;
				case bsoncxx::type::k_bool:
					onlinestatus = view["onlinestatus"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					onlinestatus = atol(view["onlinestatus"].get_utf8().value.to_string().c_str());
					break;
				}
			}
			//linecode ///
			if (view["linecode"]) {
				switch (view["linecode"].type()) {
				case bsoncxx::type::k_utf8:
					linecode = view["linecode"].get_utf8().value.to_string();
					break;
				}
			}
 

#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_q = muduo::Timestamp::now();
			//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
			createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
			muduo::Timestamp st_redislock_q = et_user_q;
#endif
			//redis分布式加锁 ttl指定1s锁时长 ///
			//1.避免玩家HTTP短连接重复Api请求情况 2.避免玩家同时多Api节点请求情况 ///
			std::string strLockKey = "lock.uid:" + std::to_string(userId) + ".order";
			RedisLock::CGuardLock lock(REDISLOCK, strLockKey.c_str(), ttlUserLockSeconds_, redLockContinue_);
			if (!lock.IsLocked()) {
				{
					std::stringstream ss;
					ss << "orderid." << orderId << " userid." << userId
						<< ".account." << account << " redisLock failed";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家上分等待超时
					errcode = eApiErrorCode::AddScoreHandleInsertDataOutTime;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
				//命中失败，说明该玩家重复操作 ///
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
			//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
			createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
			muduo::Timestamp st_addorder_q = et_redislock_q;
#endif
#if 0
			//查询add_score_order表，判断注单orderid是否已经存在
			bsoncxx::document::value query_value =
				bsoncxx::builder::stream::document{} << "orderid" << orderId << bsoncxx::builder::stream::finalize;
			result = addScoreColl.find_one(query_value.view());
			if (result) {
				{
					//注单已经存在
					std::stringstream ss;
					ss << "orderid." << orderId << " query add_score_order existed";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家上分订单已存在
					errcode = eApiErrorCode::AddScoreHandleInsertDataOrderIDExists;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
				//ss << "\n3.[mongo]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
				createLatestElapsed(latest, "mongo.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));
#endif
				break;
			}
#else
			//const_cast<std::string&>(orderId) = "666666888888";
			//redis查询注单orderid是否已经存在，mongodb[add_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			if (REDISCLIENT.exists("s.order:" + orderId + ":add")) {
				{
					//注单已经存在
					std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
					ss << "orderid." << orderId << " query score_orders existed";
#else
					ss << "orderid." << orderId << " query add_score_order existed";
#endif
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家上分订单已存在
					errcode = eApiErrorCode::AddScoreHandleInsertDataOrderIDExists;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
				//ss << "\n3.[redis]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
#ifdef _USE_SCORE_ORDERS_
				createLatestElapsed(latest, "redis.query", "score_orders", muduo::timeDifference(et_addorder_q, st_addorder_q));
#else
				createLatestElapsed(latest, "redis.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));
#endif
#endif
				break;
			}
#endif
			// 合作模式判断 1 买分 2 信用
			// 0，判断是否超过代理的库存分数，超过则退出 ///
			if (_agent_info.cooperationtype == (int)eCooType::buyScore && score > _agent_info.score) {
				{
					std::stringstream ss;
					ss << "orderid." << orderId
						<< " account." << account << ".score." << score << " less than agentid." << _agent_info.agentId
						<< ".score." << _agent_info.score;
					// 玩家上分超出代理现有总分值
					errcode = eApiErrorCode::AddScoreHandleInsertDataOverScore;
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
				}
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
			//ss << "\n3.[redis]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
#ifdef _USE_SCORE_ORDERS_
			createLatestElapsed(latest, "redis.query", "score_orders", muduo::timeDifference(et_addorder_q, st_addorder_q));
#else
			createLatestElapsed(latest, "redis.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));
#endif
			muduo::Timestamp st_addorder_i = et_addorder_q;
#endif
			std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
			//开始事务处理 ///
			sessiondb.start_transaction();
			bStartedTransaction = true;
			//插入add_score_order表，新增用户订单项(上分注单)，mongodb[add_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			bsoncxx::document::value inset_value = bsoncxx::builder::stream::document{}
				<< "orderid" << orderId		//订单id
				<< "userid" << userId		//userid
				<< "account" << account		//account
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "score" << score			//用户上分
				<< "status" << bsoncxx::types::b_int32{ 1 }
				<< "finishtime" << bsoncxx::types::b_date{ time_point_now }
#ifdef _USE_SCORE_ORDERS_
				<< "scoretype" << bsoncxx::types::b_int32{ 1 } //上分操作
#endif
				<< bsoncxx::builder::stream::finalize;
			bsoncxx::stdx::optional<mongocxx::result::insert_one> resultInsert = addScoreColl.insert_one(inset_value.view());
			//插入失败
			if (!resultInsert) {
				{
					//失败的话事务回滚 ///
					sessiondb.abort_transaction();
					std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
					ss << "orderid." << orderId << " insert score_orders error";
#else
					ss << "orderid." << orderId << " insert add_score_order error";
#endif
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家上分失败
					errcode = eApiErrorCode::AddScoreHandleInsertDataError;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_i = muduo::Timestamp::now();
				//ss << "\n4.[mongo]insert add_score_order elapsed " << muduo::timeDifference(et_addorder_i, st_addorder_i) << "s";
#ifdef _USE_SCORE_ORDERS_
				createLatestElapsed(latest, "mongo.insert", "score_orders", muduo::timeDifference(et_addorder_i, st_addorder_i));
#else
				createLatestElapsed(latest, "mongo.insert", "add_score_order", muduo::timeDifference(et_addorder_i, st_addorder_i));
#endif
#endif
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_addorder_i = muduo::Timestamp::now();
			//ss << "\n4.[mongo]insert add_score_order elapsed " << muduo::timeDifference(et_addorder_i, st_addorder_i) << "s";
#ifdef _USE_SCORE_ORDERS_
			createLatestElapsed(latest, "mongo.insert", "score_orders", muduo::timeDifference(et_addorder_i, st_addorder_i));
#else
			createLatestElapsed(latest, "mongo.insert", "add_score_order", muduo::timeDifference(et_addorder_i, st_addorder_i));
#endif
			muduo::Timestamp st_user_u = et_addorder_i;
#endif
			//更新game_user表，修改玩家积分
			bsoncxx::stdx::optional<mongocxx::result::update> resultUpdate = userCollection.update_one(
				bsoncxx::builder::stream::document{} << "userid" << userId << bsoncxx::builder::stream::finalize,
				bsoncxx::builder::stream::document{} << "$inc" << bsoncxx::builder::stream::open_document
				<< "score" << score                     //用户积分+score
				<< "addscoretimes" << bsoncxx::types::b_int32{ 1 }      //上分次数+1
				<< "alladdscore" << score               //累计上分+score
				<< bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
			//更新失败
			if (!resultUpdate || resultUpdate->modified_count() == 0) {
					{
						//失败的话事务回滚 ///
						sessiondb.abort_transaction();
						std::stringstream ss;
						ss << "orderid." << orderId << " update game_user error";
						ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
						errmsg = ss.str();
						// 玩家上分失败
						errcode = eApiErrorCode::AddScoreHandleInsertDataError;
					}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_u = muduo::Timestamp::now();
				//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
				createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
#endif
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_u = muduo::Timestamp::now();
			//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
			createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
			muduo::Timestamp st_record_i = et_user_u;
#endif
			//插入user_score_record表，添加积分变更操作日志 ///
			bsoncxx::document::value recordChangeValue = bsoncxx::builder::stream::document{} << "userid" << userId
				<< "account" << account
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "changetype" << bsoncxx::types::b_int32{ 1 } //上分操作
				<< "changemoney" << score		//上分+score
				<< "beforechange" << beforeScore//变更前积分
				<< "afterchange" << beforeScore + score//变更后积分
				<< "linecode" << linecode
				<< "logdate" << bsoncxx::types::b_date{ time_point_now }
			<< bsoncxx::builder::stream::finalize;
			resultInsert = scoreChangeColl.insert_one(recordChangeValue.view());
			//插入失败
			if (!resultInsert) {
				{
					//失败的话事务回滚 ///
					sessiondb.abort_transaction();
					std::stringstream ss;
					ss << "orderid." << orderId << " insert user_score_record error";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家上分失败
					errcode = eApiErrorCode::AddScoreHandleInsertDataError;
				}


#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_record_i = muduo::Timestamp::now();
				//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
				createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
				break;
			}


			
				/**************************************
				 *			记录玩家最后上分值
				**************************************/
				LOG_INFO << "------------记录玩家最后上分值1---------";

				auto query_value = document{} << "userid" << userId << finalize;
                auto seq_updateValue = document{} << "$set" << open_document << "userid" << userId << "account" << account
                                                  << "changetype" << 1 << "agentid" << _agent_info.agentId << "changemoney" << score << "beforechange" << beforeScore << "afterchange"
                                                  << beforeScore + score << "linecode" << linecode << "logdate" << bsoncxx::types::b_date{ time_point_now } << close_document << finalize;
                //update options
                mongocxx::options::update options = mongocxx::options::update{};
                if (scoreAddLastColl.update_one(query_value.view(), seq_updateValue.view(), options.upsert(true)))
                {
					LOG_DEBUG << "最后上分记录表增加记录成功";
				}
				/**************************************/

				LOG_INFO << "----------记录玩家最后上分值2-----------";

#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_record_i = muduo::Timestamp::now();
			//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
			createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
			// 合作模式判断 1 买分 2 信用
			if (_agent_info.cooperationtype == (int)eCooType::buyScore) {
				
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp st_redislock_q = et_record_i;
#endif
			 
#ifdef _STAT_ORDER_QPS_DETAIL_
				//	muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//	//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				//	createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
				//	//命中失败，说明该代理操作繁忙 ///
				//	break;
				//}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				//createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
				muduo::Timestamp st_agent_u = et_redislock_q;
#endif
                // 设置需要返回修改之后的数据
                mongocxx::options::find_one_and_update options;
                options.return_document(mongocxx::options::return_document::k_after);
                //更新agent_info表，修改代理积分
				auto resultfindUpdate = agentCollection.find_one_and_update(
					bsoncxx::builder::stream::document{} << "agentid" << _agent_info.agentId << bsoncxx::builder::stream::finalize,
					bsoncxx::builder::stream::document{} << "$inc" << bsoncxx::builder::stream::open_document
					<< "score" << bsoncxx::types::b_int64{ -score } //代理积分-score
                << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize,options);
				//更新失败
				if (!resultfindUpdate) {
						{
							//失败的话事务回滚 ///
							sessiondb.abort_transaction();
							std::stringstream ss;
							ss << "orderid." << orderId << " update agent_info error";
							ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
							errmsg = ss.str();
							// 玩家上分失败
							errcode = eApiErrorCode::AddScoreHandleInsertDataError;
						}
#ifdef _STAT_ORDER_QPS_DETAIL_
					muduo::Timestamp et_agent_u = muduo::Timestamp::now();
					//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
					createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
					break;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_agent_u = muduo::Timestamp::now();
				//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
				createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
				bsoncxx::document::view view = resultfindUpdate->view();
				if (view["score"]) {
					switch (view["score"].type()) {
					case bsoncxx::type::k_int64:
						//更新agent_info代理积分 ///
						_agent_info.score = view["score"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						//更新agent_info代理积分 ///
						_agent_info.score = view["score"].get_int32();
						break;
					}
				}
				//更新agent_info代理积分 ///
				//_agent_info.score -= score;
			}
			//提交事务处理 ///
			sessiondb.commit_transaction();
			//缓存add_score_order orderid到redis，30分钟之后过期，mongodb[add_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			//static int EXPIRED_TIME = 30 * 60;
			boost::property_tree::ptree root;
			root.put("orderid", orderId);//订单id
			root.put("userid", ":userid");
			root.put("account", account);
			root.put("agentid", ":agentid");
			root.put("score", ":score");//用户下分
			root.put("status", ":status");
			std::stringstream s;
			boost::property_tree::json_parser::write_json(s, root, false);
			std::string json = s.str();
			replace(json, ":userid", std::to_string(userId));
			replace(json, ":agentid", std::to_string(_agent_info.agentId));
			replace(json, ":score", std::to_string(score));
			replace(json, ":status", std::to_string(1));
			REDISCLIENT.set("s.order:" + orderId + ":add", json, orderIdExpiredSeconds_);
		}
		catch (const bsoncxx::exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query add_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家上分订单已存在
				errcode = eApiErrorCode::AddScoreHandleInsertDataOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (const muduo::Exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query add_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家上分订单已存在
				errcode = eApiErrorCode::AddScoreHandleInsertDataOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (const std::exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query add_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家上分订单已存在
				errcode = eApiErrorCode::AddScoreHandleInsertDataOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (...) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			std::stringstream ss;
			ss << "orderid." << orderId << " Unknownexception: "
				<< __FUNCTION__ << "(" << __LINE__ << ")";
			errmsg = ss.str();
			// 内部异常或未执行任务
			errcode = eApiErrorCode::InsideErrorOrNonExcutive;
			break;
		}
		//上分成功 ///
		errmsg = "success";
	} while (0);
#ifdef _STAT_ORDER_QPS_DETAIL_
	//估算每秒请求处理数 errcode == eApiErrorCode::NoError
	testTPS = (int)(1 / muduo::timeDifference(muduo::Timestamp::now(), st_collect));
	//ss << "\n9.total elapsed " << muduo::timeDifference(muduo::Timestamp::now(), st_collect) << "s";
	//LOG_WARN << __FUNCTION__ << " --- *** " << ss.str().c_str() << "\n\n";
#endif
	//玩家在线的话，通知玩家
	if (onlinestatus && errcode == 0) {

		LOG_DEBUG << "--- *** 上分操作通知客户端 成功";

		//上分操作通知客户端 ///
#ifdef _TCP_NOTIFY_CLIENT_
#if 0
		int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_HALL;
		int subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_ORDERMESSAGE_NOTIFY;
		::HallServer::OrderNotifyMessage rspdata;
		::Game::Common::Header* header = rspdata.mutable_header();
		header->set_sign(HEADER_SIGN);
		rspdata.set_addorsubflag(int(eApiType::OpAddScore));//2.上分 3.下分
		rspdata.set_userid(userId);//userId
		rspdata.set_score(beforeScore + score);
		rspdata.set_retcode(errcode);
		rspdata.set_errmsg(errmsg);
		std::string content = rspdata.SerializeAsString();
		sendUserData(userId, mainId, subId, content.data(), content.length());
#elif 1
		int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
		int subId = ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE;
		::Game::Common::ProxyNotifyOrderScoreMessage rspdata;
		::Game::Common::Header* header = rspdata.mutable_header();
		header->set_sign(HEADER_SIGN);
		rspdata.set_userid(userId);//userId
		rspdata.set_score(beforeScore + score);
		std::string content = rspdata.SerializeAsString();
		sendUserData(userId, mainId, subId, content.data(), content.length());
#endif
#else
		//redis广播通知客户端 ///
		Json::Value value;
		value["userId"] = userId;
		value["score"] = beforeScore + score;
		Json::FastWriter writer;
		REDISCLIENT.publishOrderScoreMessage(writer.write(value));
#endif
	}
	else {
		LOG_DEBUG << "--- *** 上分操作通知客户端 失败 onlinestatus = "
			<< onlinestatus << " errcode = " << errcode;
	}
	return errcode;
}

//下分写db操作 ///
int OrderServer::SubOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId
	, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS)
{
	MutexLockGuard lock(update_score_mutex_);
	int errcode = eApiErrorCode::NoError;
	errmsg.clear();
	string msginfo;
	int32_t req_code = 0;
	int32_t onlinestatus = 0;
	int64_t beforeScore = 0;
	int64_t userId = 0;
	std::string linecode;
	mongocxx::client_session sessiondb = MONGODBCLIENT.start_session();
	bool bStartedTransaction = false;
#ifdef _STAT_ORDER_QPS_DETAIL_
	//std::stringstream ss;
	muduo::Timestamp st_collect = muduo::Timestamp::now();
#endif
	do {
		try
		{
#ifdef _USE_SCORE_ORDERS_
			mongocxx::collection subScoreColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["score_orders"];
#else
			mongocxx::collection subScoreColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["sub_score_order"];
#endif
			mongocxx::collection userCollection = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["game_user"];
			mongocxx::collection agentCollection = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["proxy_info"];
			mongocxx::collection scoreChangeColl = /*MONGODBCLIENT["gamemain"]*/(*dbgamemain_)["user_score_record"];

#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_collect = muduo::Timestamp::now();
			//ss << "\n0.[mongo]getCollection elapsed " << muduo::timeDifference(et_collect, st_collect) << "s";
			createLatestElapsed(latest, "mongo.getCollection", "", muduo::timeDifference(et_collect, st_collect));
			muduo::Timestamp st_user_q = et_collect;
#endif
			//uint32_t redisGameId = 0, redisRoomId = 0;

			//查询game_user表，获取指定userid的score，onlinestatus
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1 << "score" << 1 << "onlinestatus" << 1 << "linecode" << 1 <<"currency"<< 1 << bsoncxx::builder::stream::finalize);
			bsoncxx::stdx::optional<bsoncxx::document::value> result = userCollection.find_one(
				bsoncxx::builder::stream::document{}
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "account" << account
				<< bsoncxx::builder::stream::finalize, opts);
			if (!result) {
				{
					//用户不存在
					std::stringstream ss;
					ss << "orderid." << orderId << " query game_user agentid." << _agent_info.agentId << ".account." << account << " invalid";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家不存在
					errcode = eApiErrorCode::SubScoreHandleUserNotExistsError;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_q = muduo::Timestamp::now();
				//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
				createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
#endif
				break;
			}
			//bsoncxx::to_json(doc) bsoncxx::to_json(view)
			// LOG_DEBUG << "--- *** " << "QueryResult: " << bsoncxx::to_json(*result);
			bsoncxx::document::view view = result->view();
			//userid
			userId = view["userid"].get_int64();
			//玩家当前积分
			switch (view["score"].type()) {
			case bsoncxx::type::k_int64:
				beforeScore = view["score"].get_int64();
				break;
			case bsoncxx::type::k_int32:
				beforeScore = view["score"].get_int32();
				break;
			}
			//玩家在线状态 没有这个字段会抛异常 ///
			if (view["onlinestatus"]) {
				switch (view["onlinestatus"].type()) {
				case bsoncxx::type::k_int32:
					onlinestatus = view["onlinestatus"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					onlinestatus = view["onlinestatus"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					break;
				case bsoncxx::type::k_bool:
					onlinestatus = view["onlinestatus"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					onlinestatus = atol(view["onlinestatus"].get_utf8().value.to_string().c_str());
					break;
				}
			}

			// 玩家币种信息，0人民币,非0为外币
			int32_t currency = 0;
			if (view["currency"]) {
				currency = view["currency"].get_int32();
			}

			// 1，下分前判断
			if( beforeScore < 0 ){
				errmsg = "Not enough score now ";
				errcode = eApiErrorCode::SubScoreHandleInsertDataError;
				break;
			}

			/**************************************
			****************分数太大拦截判断******************
			**************************************/
			// int64_t uid,int64_t curscore,int32_t agentid,string account
			if((!hugeProfitCheck(userId,beforeScore,_agent_info.agentId,account)) && currency == 0)
			{
				errmsg = "too huge score now ";
				errcode = eApiErrorCode::SubScoreTooLargeError;
				break;
			}
			/**************************************/
		 
	
			
			// 2，锁定玩家分
			if(!lockUserScore(userId,0,req_code,msginfo)){
				errmsg = msginfo;
				errcode = eApiErrorCode::SubScoreHandleUserOpScore;// 玩家正在游戏中
				break;
			}

		
			//linecode ///
			if (view["linecode"]) {
				switch (view["linecode"].type()) {
				case bsoncxx::type::k_utf8:
					linecode = view["linecode"].get_utf8().value.to_string();
					break;
				}
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_q = muduo::Timestamp::now();
			//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
			createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
			muduo::Timestamp st_redislock_q = et_user_q;
#endif
			//redis分布式加锁 ttl指定1s锁时长 ///
			//1.避免玩家HTTP短连接重复Api请求情况 2.避免玩家同时多Api节点请求情况 ///
			std::string strLockKey = "lock.uid:" + std::to_string(userId) + ".order";
			RedisLock::CGuardLock lock(REDISLOCK, strLockKey.c_str(), ttlUserLockSeconds_, redLockContinue_);
			if (!lock.IsLocked()) {
				{
					std::stringstream ss;
					ss << "orderid." << orderId << " userid." << userId
						<< ".account." << account << " redisLock failed";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
				}
				// 玩家下分等待超时
				errcode = eApiErrorCode::SubScoreHandleInsertDataOutTime;
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
				//命中失败，说明该玩家重复操作 ///
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
			//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
			createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));

			muduo::Timestamp st_suborder_q = et_redislock_q;
#endif
#if 0
			//查询sub_score_order表，判断注单orderid是否已经存在，耗时
			bsoncxx::document::value query_value =
				bsoncxx::builder::stream::document{} << "orderid" << orderId << bsoncxx::builder::stream::finalize;
			result = subScoreColl.find_one(query_value.view());
			if (result) {
				{
					//注单已经存在
					std::stringstream ss;
					ss << "orderid." << orderId << " query sub_score_order existed";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家下分订单已存在
					errcode = eApiErrorCode::SubScoreHandleInsertOrderIDExists;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
				//ss << "\n3.[mongo]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
				createLatestElapsed(latest, "mongo.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
#endif
				break;
			}
#else
			//const_cast<std::string&>(orderId) = "666666888888";
			//redis查询注单orderid是否已经存在，mongodb[sub_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			if (REDISCLIENT.exists("s.order:" + orderId + ":sub")) {
				{
					//注单已经存在
					std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
					ss << "orderid." << orderId << " query score_orders existed";
#else
					ss << "orderid." << orderId << " query sub_score_order existed";
#endif
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家下分订单已存在
					errcode = eApiErrorCode::SubScoreHandleInsertOrderIDExists;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
#ifdef _USE_SCORE_ORDERS_
				//ss << "\n3.[redis]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
				createLatestElapsed(latest, "redis.query", "score_orders", muduo::timeDifference(et_suborder_q, st_suborder_q));
#else
				createLatestElapsed(latest, "redis.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
#endif
#endif
				break;
			}
#endif
#if 0		//比赛那边用到，所以暂时留着
			//根据userid进行加锁，和比赛服zklock相对应 ///
			std::string zkPath = m_szUserOrderScorePath + "/" + std::to_string(userId);
			if (ZOK == m_zookeeperclient->existsNode(zkPath)) {
				std::stringstream ss;
				ss << "orderid." << orderId << " userid." << userId
					<< ".account." << account << " zkLock node invalid";
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家正在游戏中，不能下分
				errcode = eApiErrorCode::SubScoreHandleInsertDataUserInGaming;
				//zk锁节点无效 ///
				break;
			}
			//zk分布式加锁，命中失败，说明该玩家重复操作 ///
			zkLockGuard zkLock(m_zookeeperclient, zkPath, std::to_string(userId));
#endif
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
#ifdef _USE_SCORE_ORDERS_
			//ss << "\n3.[redis]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
			createLatestElapsed(latest, "redis.query", "score_orders", muduo::timeDifference(et_suborder_q, st_suborder_q));
#else
			createLatestElapsed(latest, "redis.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
#endif
			muduo::Timestamp st_suborder_i = et_suborder_q;
#endif
			//玩家游戏中不能下分操作 ///
			//if (redisRoomId != 0 || redisGameId != 0) {
			//if(REDISCLIENT.ExistsUserLoginInfo(userId)) {
			if (REDISCLIENT.ExistsUserOnlineInfo(userId)) {
				std::stringstream ss;
				ss << "orderid." << orderId << " " << account << " is playing "/* << redisGameId << "." << redisRoomId*/;
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家正在游戏中，不能下分
				errcode = eApiErrorCode::SubScoreHandleInsertDataUserInGaming;
				break;
			}
			//积分不足，不能下分 ///
			if (beforeScore < score) {
				std::stringstream ss;
				ss << "orderid." << orderId << " " << "score not enough, score." << beforeScore << " < deltascore." << score;
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家下分超出玩家现有总分值
				errcode = eApiErrorCode::SubScoreHandleInsertDataOverScore;
				break;
			}
			std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
			//std::time_t t = std::chrono::system_clock::to_time_t(time_point_now);
			//std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S.%z%Z");
			//开始事务处理 ///
			sessiondb.start_transaction();
			bStartedTransaction = true;
			//插入sub_score_order表，新增用户订单项(下分注单)，mongodb[sub_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			bsoncxx::document::value inset_value = bsoncxx::builder::stream::document{}
				<< "orderid" << orderId        //订单id
				<< "userid" << userId
				<< "account" << account
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "score" << score            //用户下分
				<< "status" << bsoncxx::types::b_int32{ 1 }
				<< "finishtime" << bsoncxx::types::b_date{ time_point_now }
#ifdef _USE_SCORE_ORDERS_
				<< "scoretype" << bsoncxx::types::b_int32{ 2 } //下分操作
#endif
				<< bsoncxx::builder::stream::finalize;
			bsoncxx::stdx::optional<mongocxx::result::insert_one> resultInsert = subScoreColl.insert_one(inset_value.view());
			//插入失败
			if (!resultInsert) {
				{
					//失败的话事务回滚 ///
					sessiondb.abort_transaction();
					std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
					ss << "orderid." << orderId << " insert score_orders error";
#else
					ss << "orderid." << orderId << " insert sub_score_order error";
#endif
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家下分失败
					errcode = eApiErrorCode::SubScoreHandleInsertDataError;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_suborder_i = muduo::Timestamp::now();
				//ss << "\n4.[mongo]insert sub_score_order elapsed " << muduo::timeDifference(et_suborder_i, st_suborder_i) << "s";
#ifdef _USE_SCORE_ORDERS_
				createLatestElapsed(latest, "mongo.insert", "score_orders", muduo::timeDifference(et_suborder_i, st_suborder_i));
#else
				createLatestElapsed(latest, "mongo.insert", "sub_score_order", muduo::timeDifference(et_suborder_i, st_suborder_i));
#endif
#endif
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_suborder_i = muduo::Timestamp::now();
			//ss << "\n4.[mongo]insert sub_score_order elapsed " << muduo::timeDifference(et_suborder_i, st_suborder_i) << "s";
#ifdef _USE_SCORE_ORDERS_
			createLatestElapsed(latest, "mongo.insert", "score_orders", muduo::timeDifference(et_suborder_i, st_suborder_i));
#else
			createLatestElapsed(latest, "mongo.insert", "sub_score_order", muduo::timeDifference(et_suborder_i, st_suborder_i));
#endif
			muduo::Timestamp st_user_u = et_suborder_i;
#endif
			//更新game_user表，修改玩家积分
			bsoncxx::stdx::optional<mongocxx::result::update> resultUpdate = userCollection.update_one(
				bsoncxx::builder::stream::document{} << "userid" << userId << bsoncxx::builder::stream::finalize,
				bsoncxx::builder::stream::document{} << "$inc" << bsoncxx::builder::stream::open_document
				<< "score" << -score                //用户积分-score
				<< "subscoretimes" << b_int32{ 1 }  //下分次数+1
				<< "allsubscore" << score           //累计下分+score
				<< bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
			//更新失败
			if (!resultUpdate || resultUpdate->modified_count() == 0) {
					{
						//失败的话事务回滚 ///
						sessiondb.abort_transaction();
						std::stringstream ss;
						ss << "orderid." << orderId << " update game_user error";
						ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
						errmsg = ss.str();
						// 玩家下分失败
						errcode = eApiErrorCode::SubScoreHandleInsertDataError;
					}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_u = muduo::Timestamp::now();
				//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
				createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
#endif
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_u = muduo::Timestamp::now();
			//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
			createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
			muduo::Timestamp st_record_i = et_user_u;
#endif
			//插入user_score_record表，添加积分变更操作日志 ///
			bsoncxx::document::value recordChangeValue = bsoncxx::builder::stream::document{} << "userid" << userId
				<< "account" << account
				<< "agentid" << bsoncxx::types::b_int32{ _agent_info.agentId }
				<< "changetype" << bsoncxx::types::b_int32{ 2 } //下分操作
				<< "changemoney" << -score      //下分-score
				<< "beforechange" << beforeScore//变更前积分
				<< "afterchange" << beforeScore - score//变更后积分
				<< "linecode" << linecode
				<< "logdate" << bsoncxx::types::b_date{ time_point_now }
			<< bsoncxx::builder::stream::finalize;
			resultInsert = scoreChangeColl.insert_one(recordChangeValue.view());
			//插入失败
			if (!resultInsert) {
				{
					//失败的话事务回滚 ///
					sessiondb.abort_transaction();
					std::stringstream ss;
					ss << "orderid." << orderId << " insert user_score_record error";
					ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
					errmsg = ss.str();
					// 玩家下分失败
					errcode = eApiErrorCode::SubScoreHandleInsertDataError;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_record_i = muduo::Timestamp::now();
				//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
				createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
				break;
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_record_i = muduo::Timestamp::now();
			//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
			createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
			// 合作模式判断 1 买分 2 信用
			if (_agent_info.cooperationtype == (int)eCooType::buyScore) {
				
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp st_redislock_q = et_record_i;
#endif
		 
#ifdef _STAT_ORDER_QPS_DETAIL_
				//	muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//	//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				//	createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
				//	//命中失败，说明该代理操作繁忙 ///
				//	break;
				//}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				//createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
				muduo::Timestamp st_agent_u = et_redislock_q;
#endif
				//更新agent_info表，修改代理积分
				auto resultfindUpdate = agentCollection.find_one_and_update(
					bsoncxx::builder::stream::document{} << "agentid" << _agent_info.agentId << bsoncxx::builder::stream::finalize,
					bsoncxx::builder::stream::document{} << "$inc" << bsoncxx::builder::stream::open_document
					<< "score" << bsoncxx::types::b_int64{ score } //代理积分+score
				<< bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
				//更新失败
				if (!resultfindUpdate) {
						{
							//失败的话事务回滚 ///
							sessiondb.abort_transaction();
							std::stringstream ss;
							ss << "orderid." << orderId << " update agent_info error";
							ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
							errmsg = ss.str();
							// 玩家下分失败
							errcode = eApiErrorCode::SubScoreHandleInsertDataError;
						}
#ifdef _STAT_ORDER_QPS_DETAIL_
					muduo::Timestamp et_agent_u = muduo::Timestamp::now();
					//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
					createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
					break;
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_agent_u = muduo::Timestamp::now();
				//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
				createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
				bsoncxx::document::view view = resultfindUpdate->view();
				if (view["score"]) {
					switch (view["score"].type()) {
					case bsoncxx::type::k_int64:
						//更新agent_info代理积分 ///
						_agent_info.score = view["score"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						//更新agent_info代理积分 ///
						_agent_info.score = view["score"].get_int32();
						break;
					}
				}
				//更新agent_info代理积分 ///
				//_agent_info.score += score;
			}
			//提交事务处理 ///
			sessiondb.commit_transaction();
			//缓存sub_score_order orderid到redis，30分钟之后过期，mongodb[sub_score_order]需要建立唯一约束 UNIQUE(orderid) ///
			//static int EXPIRED_TIME = 30 * 60;
			boost::property_tree::ptree root;
			root.put("orderid", orderId);//订单id
			root.put("userid", ":userid");
			root.put("account", account);
			root.put("agentid", ":agentid");
			root.put("score", ":score");//用户下分
			root.put("status", ":status");
			std::stringstream s;
			boost::property_tree::json_parser::write_json(s, root, false);
			std::string json = s.str();
			replace(json, ":userid", std::to_string(userId));
			replace(json, ":agentid", std::to_string(_agent_info.agentId));
			replace(json, ":score", std::to_string(score));
			replace(json, ":status", std::to_string(1));
			REDISCLIENT.set("s.order:" + orderId + ":sub", json, orderIdExpiredSeconds_); 

	 
			
		}
		catch (const bsoncxx::exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query sub_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家下分订单已存在
				errcode = eApiErrorCode::SubScoreHandleInsertOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (const muduo::Exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query sub_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家下分订单已存在
				errcode = eApiErrorCode::SubScoreHandleInsertOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (const std::exception & e) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			int errno_ = getMongoDBErrno(e.what());
			switch (errno_) {
			case 11000: {
				//注单已经存在
				std::stringstream ss;
#ifdef _USE_SCORE_ORDERS_
				ss << "orderid." << orderId << " query score_orders existed";
#else
				ss << "orderid." << orderId << " query sub_score_order existed";
#endif
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 玩家下分订单已存在
				errcode = eApiErrorCode::SubScoreHandleInsertOrderIDExists;
				break;
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << orderId << " MongoDBexception: " << e.what();
				ss << " " << __FUNCTION__ << "(" << __LINE__ << ")";
				errmsg = ss.str();
				// 内部异常或未执行任务
				errcode = eApiErrorCode::InsideErrorOrNonExcutive;
				break;
			}
			}
			break;
		}
		catch (...) {
			if (bStartedTransaction) {
				//失败的话事务回滚 ///
				sessiondb.abort_transaction();
			}
			std::stringstream ss;
			ss << "orderid." << orderId << " Unknownexception: "
				<< __FUNCTION__ << "(" << __LINE__ << ")";
			errmsg = ss.str();
			// 内部异常或未执行任务
			errcode = eApiErrorCode::InsideErrorOrNonExcutive;
			break;
		}
		//下分成功 ///
		errmsg = "success";
	} while (0);

	// 解锁玩家分
	unlockUserScore(userId,0,req_code,msginfo);

#ifdef _STAT_ORDER_QPS_DETAIL_
	//估算每秒请求处理数 errcode == eApiErrorCode::NoError
	testTPS = (int)(1 / muduo::timeDifference(muduo::Timestamp::now(), st_collect));
	//ss << "\n9.total elapsed " << muduo::timeDifference(muduo::Timestamp::now(), st_collect) << "s";
	//LOG_WARN << __FUNCTION__ << " --- *** " << ss.str().c_str() << "\n\n";
#endif
	//玩家在线的话，通知玩家
	if (onlinestatus && errcode == 0) {

		LOG_DEBUG << "--- *** 下分操作通知客户端 成功";

		//下分操作通知客户端 ///
#ifdef _TCP_NOTIFY_CLIENT_
#if 0
		int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_HALL;
		int subId = ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_ORDERMESSAGE_NOTIFY;
		::HallServer::OrderNotifyMessage rspdata;
		::Game::Common::Header* header = rspdata.mutable_header();
		header->set_sign(HEADER_SIGN);
		rspdata.set_addorsubflag(int(eApiType::OpSubScore));//2.上分 3.下分
		rspdata.set_userid(userId);//userId
		rspdata.set_score(beforeScore - score);
		rspdata.set_retcode(errcode);
		rspdata.set_errmsg(errmsg);
		std::string content = rspdata.SerializeAsString();
		sendUserData(userId, mainId, subId, content.data(), content.length());
#elif 1
		int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
		int subId = ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE;
		::Game::Common::ProxyNotifyOrderScoreMessage rspdata;
		::Game::Common::Header* header = rspdata.mutable_header();
		header->set_sign(HEADER_SIGN);
		rspdata.set_userid(userId);//userId
		rspdata.set_score(beforeScore - score);
		std::string content = rspdata.SerializeAsString();
		sendUserData(userId, mainId, subId, content.data(), content.length());
#endif
#else
		//redis广播通知客户端 ///
		Json::Value value;
		value["userId"] = userId;
		value["score"] = beforeScore - score;
		Json::FastWriter writer;
		REDISCLIENT.publishOrderScoreMessage(writer.write(value));
#endif
	}
	else {
		LOG_DEBUG << "--- *** 下分操作通知客户端 失败 onlinestatus = "
			<< onlinestatus << " errcode = " << errcode;
	}
	return errcode;
}

// 更新审核表 type拦截类型
bool OrderServer::auditform(bool isOnly, int32_t agentid, int64_t uid, int32_t type, int32_t status, string account,int64_t curscore,int64_t lastAddScoreValue,int64_t tempScore,int32_t addtime)
{
	mongocxx::collection auditformColl = MONGODBCLIENT["gamemain"]["audit_form_record"];

	if (isOnly)
	{
		auto update_doc = make_document(kvp("status", status));
		auditformColl.update_one(make_document(kvp("userid", uid)),make_document(kvp("$set", update_doc.view())));
	}
	else
	{
		mongocxx::options::update options = mongocxx::options::update{};
		auto update_doc = make_document(kvp("agentid", agentid), kvp("userid", uid), kvp("account", account), kvp("type", type), kvp("status", status),kvp("curscore", curscore), kvp("lastscore", lastAddScoreValue),kvp("tempscore", tempScore), kvp("addtime", addtime), kvp("optime",b_date{chrono::system_clock::now()}));
		auditformColl.update_one(make_document(kvp("userid", uid)), make_document(kvp("$set", update_doc.view())), options.upsert(true));
	}
}

// 巨额分数拦截判断
/*
ret true 正常，可下分；fale 异常，不可下分
*/
bool OrderServer::hugeProfitCheck(int64_t uid,int64_t curscore,int32_t agentid,string account)//, int32_t & req_code,string & errmsg)
{
	bool ret = true;

	do
	{
		// 分数小于一定值则直接不拦截
		if(curscore < inicfg_.minscore) 
			return true;
		
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "afterchange" << 1  << bsoncxx::builder::stream::finalize);
		// 1.查询上分表获取最后上分值
		mongocxx::collection auditformColl = MONGODBCLIENT["gamemain"]["audit_form_record"];
		mongocxx::collection scoreAddLastColl = MONGODBCLIENT["gamemain"]["user_score_add_last_record"];
		auto findValue = bsoncxx::builder::stream::document{} << "userid" << uid << bsoncxx::builder::stream::finalize;
		auto result = scoreAddLastColl.find_one(findValue.view(),opts);
		if (!result){ //玩家没有上过分，属于没有上分则下分
			if(0){//curscore >  0){
				ret = false;//拦截  
				LOG_ERROR <<  "玩家["<< uid <<"]没有上过分，但有下分分数[" << curscore <<"]";
				auditform(false,agentid,uid,eHeadOffType::other,eAuditType::error,account,curscore,0);
			}
			break;
		}

		bsoncxx::document::view view = result->view();
		int64_t lastAddScoreValue = view["afterchange"].get_int64();
		int64_t tempScore = curscore - lastAddScoreValue;
		int32_t auditType = eHeadOffType::tenTimes;
		int32_t addtime = tempScore / lastAddScoreValue;

		bool needAudit = false;
		// 倍数大于addtime 或者分数大于 auditThreshold
		if (tempScore >= inicfg_.auditThreshold || (addtime >= inicfg_.addtime && curscore >= inicfg_.minscore))
		{
			// 如果是分数太大
			if (tempScore >= inicfg_.auditThreshold)
				auditType = eHeadOffType::toolarge;

			needAudit = true;
		}

		LOG_DEBUG <<  "玩家["<< uid <<"]当前分数[" << curscore <<"],最后上分值[" << lastAddScoreValue <<"],tempScore[" << tempScore <<"],addtime[" << addtime <<"],minscore[" << inicfg_.minscore <<"]";

		// 1.1查询拦截表
		mongocxx::options::find opts2 = mongocxx::options::find{};
		opts2.projection(bsoncxx::builder::stream::document{} << "status" << 1 << bsoncxx::builder::stream::finalize);

		auto auditRet = auditformColl.find_one(document{} << "userid" << uid << finalize, opts2);
		//没有找到有拦截记录，则玩家是第一次下分被拦截
		if (!auditRet)
		{
			// 倍数大于addtime 或者分数大于 auditThreshold
			if(needAudit)
			{ 
				LOG_INFO << "玩家[" << uid << "]没有上过分，当前分数[" << curscore << "],最后上分值[" << lastAddScoreValue << "],没有找到有拦截记录，玩家是第一次下分被拦截";
				ret = false; //拦截
				auditform(false, agentid, uid, auditType, eAuditType::error, account, curscore, lastAddScoreValue, tempScore, addtime);
			}
			
			break;
		}

		// 找到记录
		auto auditview = auditRet->view();
		int32_t statusValue = auditview["status"].get_int32();

		// 倍数大于addtime 或者分数大于 auditThreshold
		if(needAudit && (statusValue == eAuditType::pass))
		{ 
			LOG_INFO << "玩家[" << uid << "]没有上过分，当前分数[" << curscore << "],最后上分值[" << lastAddScoreValue << "],有找到拦截记录，玩家不是第一次下分被拦截";
			ret = false; //拦截
			auditform(false, agentid, uid, auditType, eAuditType::error, account, curscore, lastAddScoreValue, tempScore, addtime);
			break;
		}
		else if (statusValue > eAuditType::auditOK)
		{
			LOG_INFO << "玩家[" << uid << "]当前分数[" << curscore << "],最后上分值[" << lastAddScoreValue << "],status[" << statusValue << "]有异常或者没有审核通过";
			ret = false; //有异常或者没有审核通过
			auditform(false, agentid, uid, auditType, eAuditType::error, account, curscore, lastAddScoreValue, tempScore, addtime);
			break;
		}
		else if (statusValue == eAuditType::auditOK)
		{
			LOG_WARN << "玩家[" << uid << "]当前分数[" << curscore << "],最后上分值[" << lastAddScoreValue << "],status[" << statusValue << "],审核通过";
			// 1.更新拦截表audit_form_record,status:0,2,3
			auditform(true,agentid,uid,auditType,eAuditType::pass,account,curscore,lastAddScoreValue);
		}

	} while (0);

	return ret;
}

// 锁定用户操作
bool OrderServer::lockUserScore(int64_t uid,int32_t opType, int32_t & req_code,string & errmsg)
{
	try
	{
		errmsg = "score lock failer";
		req_code = 15;
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
			req_code = 0;
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
	}

	return false;
}

// 解锁用户操作
bool OrderServer::unlockUserScore(int64_t uid,int32_t opType,int32_t & req_code,string & errmsg)
{
	try
	{
		errmsg = "unlock score failer";
		req_code = 16;
		// db
		mongocxx::collection lock_user_coll = MONGODBCLIENT["gamemain"]["lock_user_downscore"];

		// 更新玩家状态
		auto findValue =  bsoncxx::builder::stream::document{} << "userid" << uid << bsoncxx::builder::stream::finalize;
		auto find_delete = lock_user_coll.find_one_and_delete(findValue.view());
		if ( find_delete ){
			LOG_WARN << __FUNCTION__ << " 解锁玩家分成功，玩家[" << uid <<"],操作类型[" << opType << "]";
			req_code = 0;
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
