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

#include "TxhAPI.hpp"
#include "LotteryServer.h"

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

std::map<int, LotteryServer::AccessCommandFunctor>  LotteryServer::m_functor_map;

// 大唐帐转进入天下汇
#define TRANS_IN_DT2TXH  	"IN"
// 天下汇帐转进入大唐
#define TRANS_OUT_TXH2DT  	"OUT"
// 版本
#define VERSION  	"100"

// 彩票种类
#define LOTTERY_GFC  	"GFC"		//传统官方彩
#define LOTTERY_LOTTERY	"LOTTERY"	//时时彩
#define LOTTERY_LOTTO  	"LOTTO"		//香港彩
 

// 固定值
#define NOT_NEED_UPDATE  	-1

// 转帐类型
enum eTransType
{
	op_Dt2Txh		= 0, // 大唐帐转进入第三方收指定分
	op_Dt2Txh_all	= 1, // 大唐帐转进入第三方全部收回
	op_Txh2Dt		= 2, // 第三方帐转进入大唐带指定分
	op_Txh2Dt_all	= 3, // 第三方帐转进入大唐带全部分
	op_balance_all	= 4, // 查询余额
	op_recycle_all	= 5, // 回收余额
};

shared_ptr<CTxhAPI> m_TxhAPI;

LotteryServer::LotteryServer(muduo::net::EventLoop* loop,	const muduo::net::InetAddress& listenAddr,const muduo::net::InetAddress& listenAddrHttp,std::string const& strIpAddr,muduo::net::TcpServer::Option option)
		: httpServer_(loop, listenAddrHttp, "LotteryHttpServer", option)
        , threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "TimerEventLoopThread"))
        , m_pIpFinder("qqwry.dat")
        , strIpAddr_(strIpAddr)
        , m_bInvaildServer(false)
        , isStartServer(false)
{
	srand(time(nullptr));
	errcode_info_.clear();
	company_balance_.clear();
	player_plat_balance_.clear();
	// 
	 m_pattern = boost::regex("\n|\r"); 

	setHttpCallback(CALLBACK_2(LotteryServer::processHttpRequest, this));
	httpServer_.setConnectionCallback(CALLBACK_1(LotteryServer::onHttpConnection, this));
	httpServer_.setMessageCallback(CALLBACK_3(LotteryServer::onHttpMessage, this)); 
    
	// 获取Loop指针
	loopTimer_ = threadTimer_->startLoop();

 	// 操作对象
	m_TxhAPI.reset(new CTxhAPI(loopTimer_));
	m_TxhAPI->startInit();  
}

LotteryServer::~LotteryServer()
{
    m_functor_map.clear();
    quit();
}
 
void LotteryServer::quit()
{
    if(m_zookeeperclient)
        m_zookeeperclient->closeServer();
    m_redisPubSubClient->unsubscribe();
    threadPool_.stop();
}

//MongoDB/redis与线程池关联 ///
void LotteryServer::threadInit()
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

bool LotteryServer::InitZookeeper(std::string ipaddr)
{
    LOG_INFO << __FUNCTION__ <<" ipaddr["<< ipaddr <<"]";
	m_zookeeperclient.reset(new ZookeeperClient(ipaddr));
    //指定zookeeper连接成功回调 ///
    m_zookeeperclient->SetConnectedWatcherHandler(std::bind(&LotteryServer::ZookeeperConnectedHandler, this));
    //连接到zookeeper，成功后回调ZookeeperConnectedHandler ///
    if (!m_zookeeperclient->connectServer()) {
        LOG_ERROR << "InitZookeeper error";
        // abort();
    }

    LOG_INFO << "---InitZookeeper OK !";

	return true;
}

bool LotteryServer::InitRedisCluster(std::string ipaddr, std::string password)
{
	LOG_INFO <<"---ipaddr["<< ipaddr <<"],password["<<password<<"]";

	m_redisPubSubClient.reset(new RedisClient());
	if (!m_redisPubSubClient->initRedisCluster(ipaddr, password)) {
		LOG_ERROR << "InitRedisCluster error,ipaddr[" << ipaddr <<"]";
	}
	// 保存
	m_redisPassword = password;
	//更新代码消息
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_Trans_proxy_info,CALLBACK_0(LotteryServer::refreshAgentInfo,this));
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_uphold_Trans_server,CALLBACK_1(LotteryServer::repairLoginServer,this)); 
	// redis广播
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_noticfy_recycle_balance, [&](string msg) {
		LOG_INFO << "---收到回收余额消息[" << msg << "],IsMaster[" << inicfg_.IsMaster <<"]";
		if (inicfg_.IsMaster == 1)
		{
			// threadTimer_->getLoop()->runInLoop(CALLBACK_0(LotteryServer::recycle Balance,this,(int32_t)eRecycleType::one,msg)); 
		}
	});

	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_update_third_part_game,CALLBACK_0(LotteryServer::cacheThirdPartGame,this));
	// 
	m_redisPubSubClient->subscribePublishMsg(eRedisPublicMsg::bc_synchronize_score,CALLBACK_1(LotteryServer::updatePlayerPlatBalance,this));
	

	// 
	m_redisPubSubClient->startSubThread();

	m_redisIPPort = ipaddr;

	return true;
}

bool LotteryServer::InitMongoDB(std::string url)
{
	LOG_INFO << __FUNCTION__ << "---DB url " << url;
    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = url;

    return true;
}

//zookeeper连接成功回调 ///
void LotteryServer::ZookeeperConnectedHandler()
{
	LOG_INFO << __FUNCTION__ << "zookeeper连接成功回调";
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME"))
        m_zookeeperclient->createNode("/GAME", "Landy");
    //LotteryServers节点集合
    if(ZNONODE == m_zookeeperclient->existsNode("/GAME/LotteryServers"))
        m_zookeeperclient->createNode("/GAME/LotteryServers", "LotteryServers");
 
    //本地节点启动时自注册 ///
	{
        //指定网卡ip:port ///
		std::vector<std::string> vec;
		boost::algorithm::split(vec, httpServer_.ipPort(), boost::is_any_of(":"));
		
		m_szNodeValue = strIpAddr_ + ":" + vec[1];
		m_szNodePath = "/GAME/LotteryServers/" + m_szNodeValue;
		//自注册LoginServer节点到zookeeper
		if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
			m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true);
		}
		m_szInvalidNodePath = "/GAME/LotteryServersInvaild/" + m_szNodeValue;
	}  
}

//自注册服务节点到zookeeper ///
void LotteryServer::OnRepairServerNode()
{
	if (ZNONODE == m_zookeeperclient->existsNode(m_szNodePath)) {
		LOG_WARN << "--- *** " << m_szNodePath;
		m_zookeeperclient->createNode(m_szNodePath, m_szNodeValue, true); 
	}  
}

//通知停止上下分操作处理 
// 维护登录服
void LotteryServer::repairLoginServer(string msg)
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
				LOG_WARN << "---repair LotteryServer OK,当前[" << m_bInvaildServer <<"]";
			}
			else if ( m_szNodeValue.compare(ipaddr) == 0 )
			{
				m_bInvaildServer = (status == eUpholdType::op_stop);
				LOG_WARN << "---repair LotteryServer OK,当前[" << m_bInvaildServer <<"]";
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
void LotteryServer::start(int numThreads, int workerNumThreads)
{
	// 最晚一个启动的服务结束后开启变量 isStartServer = true
	threadTimer_->getLoop()->runEvery(10.0f, bind(&LotteryServer::OnRepairServerNode, this)); 
	threadTimer_->getLoop()->runAfter(1.5f, bind(&LotteryServer::refreshAgentInfo, this));
	threadTimer_->getLoop()->runAfter(1.25f, bind(&LotteryServer::cacheThirdPartGame, this));
	threadTimer_->getLoop()->runEvery(1.0f, [&]() {
		// 
		// if (inicfg_.IsMaster == 0)
		// 	return; 

		// 固定回收周期
		if (++nSecondTick_ < inicfg_.RecycleCycle){
			// 定时加载conf配置文件
			if(nSecondTick_ > 0 && (nSecondTick_ % inicfg_.ReadConfCycle == 0)){
				LoadIniConfig();
			}
			return;
		}
		nSecondTick_ = 0; 

		// 主才处理回收动作
		if (inicfg_.IsMaster == 0)
			return;  
	});
	// 加载Json
	threadTimer_->getLoop()->runAfter(0.1f, bind(&LotteryServer::loadJson, this));


	LOG_WARN << "---启动TCP监听--- ";
}

//启动HTTP业务线程 ///
//启动HTTP监听 ///
void LotteryServer::startHttpServer(int numThreads, int workerNumThreads) {
	//处理HTTP回调
	setHttpCallback(CALLBACK_2(LotteryServer::processHttpRequest,this));

	//threadInit会被调用workerNumThreads次 ///
	threadPool_.setThreadInitCallback(bind(&LotteryServer::threadInit, this));

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
void LotteryServer::onHttpConnection(const muduo::net::TcpConnectionPtr& conn)
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
void LotteryServer::onHttpMessage(const muduo::net::TcpConnectionPtr& conn,
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
		threadPool_.run(std::bind(&LotteryServer::asyncHttpHandler, this, weakConn, context->request(),addr.ipNetEndian()));
		//释放资源
		//context->reset();
	}
}

//异步回调 ///
void LotteryServer::asyncHttpHandler(const muduo::net::TcpConnectionWeakPtr& weakConn, const muduo::net::HttpRequest& req,uint32_t _ipNetEndian)
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

//解析请求 ///
bool LotteryServer::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg)
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
std::string LotteryServer::getRequestStr(muduo::net::HttpRequest const& req) {
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
std::string LotteryServer::createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr)
{
	boost::property_tree::ptree root,data;
	root.put("code", ":code");
	root.put("msg", msgstr);
	// root.put("type", ":type");

	// data.put("uid", ":uid");
	// data.put("lgname",lgname);
	data.put("dt",":dt");
	data.put("url",urlstr);
	
	root.add_child("data", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	// replace(json, ":type", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	// replace(json, ":uid", std::to_string(uid));
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
std::string LotteryServer::createJsonRsp(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,int64_t newScore)
{
	boost::property_tree::ptree root,data;
	root.put("type", ":type");
	root.put("code", ":code");
	root.put("msg", msgstr);

	// data.put("uid", ":uid");
	data.put("score",":score");
	data.put("dt",":dt");
	
	root.add_child("data", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string json = ss.str();
	replace(json, ":type", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	// replace(json, ":uid", std::to_string(uid));
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
std::string LotteryServer::createJsonString(int32_t opType,int32_t errcode,string const& dataStr)
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
/*
//username: 合作伙伴平台的用户ID,字符串长度少于或者等于20位, 由字母和数字组成
//password: 合作伙伴平台的用户原始密码，要求ＭＤ５加密, 长度为32位(注意:传统官方彩登录密码跟IG平台其它游戏的登录密码不相同，请区分开来。)
//currency:  合作伙伴的用户ID所使用的币别
//nickname:  合作伙伴的用户昵称, 字符串长度少于或者等于20位
//language:  游戏语言(CN,EN)
//line:  线路(可选参数) 　参数(1, 2, …)  默认线路为1, 注:具体线路参数ID以游戏平台为准
//gameType:  游戏参数为传统官方彩（GFC）.注：不能为空。
//gfcTray:  传统官方彩盘口可选择（A,B,C），注: 不能为空。
//lotteryTray:  时时彩盘口，如果为空则跟随传统官方彩的盘口
//lottoTray:  香港彩盘口，如果为空则跟随传统官方彩的盘口
//gfcType:  传统官方彩界面类型，PC为电脑版，MP为手机版，APP为APP版
//gfcPage:  传统官方彩游戏类型，对应跳转到确定的传统官方彩游戏页面，对应gameInfoId，如果gfcPage不传或者传非法字符，手机版则进入手机版游戏大厅，电脑版则默认进入北京赛车。
//homeUrl:  游戏返回或退出时的回调地址，一般为客户主页面地址或者客户游戏大厅地址。
//lc:  游戏背景颜色，参数值为(b/y)b代表蓝色背景，y代表橙色背景
{"hashCode":" aaaaaaa-df52-46bb-9ac1-bbbbbbbbbbbb ",
"command":"LOGIN",
"params":{ 
"username":"test02",
"password":"96e79218965eb72c92a549dd5a330112",
"nickname":"0001",
"currency":"CNY",
"language":"EN",
"line":0,
"gameType":"GFC",
"gfcTray":"A",
"gfcPage":"61",
"gfcType":"PC" ,
"lc":"b" }}


jsonstr[{"hashCode":" aaaaaaa-df52-46bb-9ac1-bbbbbbbbbbbb ","command":"LOGIN",
"params":{"username":"","nickname":"","password":"666888","currency":"TEST","language":"CN","line":"0","gameType":"","gfcTray":"A","gfcPage":0,"gfcType":"APP","homeUrl":"www.baidu.com","lc":"b"}}],
resAddUrl[http://tradesw.iasia99.com/igapiyftradesw/app/api.do]


	Json::FastWriter writer;
	Json::Value JsonParam;
	JsonParam["username"] = reqParam.lgname;
	JsonParam["nickname"] = reqParam.nickname;
	JsonParam["password"] = inicfg_.Password;
	JsonParam["currency"] = agent_info.currency;
	JsonParam["language"] = agent_info.language;
	JsonParam["line"] = (int32_t)0;
	JsonParam["gameType"] = reqParam.lotterytype;
	JsonParam["gfcTray"] = agent_info.lotteryTray;
	JsonParam["gfcPage"] = std::to_string(reqParam.gameInfoId);
	JsonParam["gfcType"] = reqParam.gfcType;
	JsonParam["homeUrl"] = reqParam.backurl;
	JsonParam["lc"] = agent_info.color;
	string jackpotMsg = writer.write(JsonParam);
	std::cout << "jackpotMsg[" << jackpotMsg << "],line[" << __LINE__ <<"]" << std::endl;

	// 整包
	Json::Value root;
	root["params"] = jackpotMsg;
	root["hashCode"] = agent_info.hashcode;
	root["command"] = "LOGIN";
	string jsonstr = writer.write(root);

	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");

    auto resp_feature = cpr::Post(cpr::Url(agent_info.ApiUrl),cpr::Body{jsonstr},cpr::Header{{"content-type", "application/json"}});
	std::cout << "整包.text[" << resp_feature.text << "],line[" << __LINE__ <<"]" << std::endl;
*/

// 检测并创建游戏帐号
// @actype 账号类型：0-试玩账号 1-真钱账号
bool LotteryServer::getCreateAccountParams(ReqParamCfg & reqParam,third_game_cfg agent_info,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_WARN << "---" << __FUNCTION__ << ",lgname[" << reqParam.lgname << "],gameInfoId[" << reqParam.gameInfoId << "],aid[" << reqParam.aid << "]";
 
	boost::property_tree::ptree root_pt,data,req_pt;// 

	root_pt.put("hashCode",agent_info.hashcode);
	root_pt.put("command", "LOGIN");
	 
	data.put("username", reqParam.lgname);
	data.put("nickname", reqParam.nickname);
	data.put("password", inicfg_.Password);
	data.put("currency", agent_info.currency);
	data.put("language", agent_info.language);
	data.put("line", ":line");
	data.put("lc", agent_info.color); //游戏背景颜色，参数值为(b/y)b代表蓝色背景，y代表橙色背景
	data.put("gameType", reqParam.lotterytype);//"GFC","Lottery","Lotto");
	data.put("homeUrl",reqParam.backurl);// "www.baidu.com"); //游戏返回或退出时的回调地址
	// 官方彩
	if (reqParam.lotterytype == LOTTERY_GFC)
	{
		data.put("gfcTray", agent_info.gfcTray);				  //传统官方彩盘口可选择（A,B,C），注: 不能为空。
		data.put("gfcPage", std::to_string(reqParam.gameInfoId)); //对应gameInfoId
		data.put("gfcType", reqParam.gfcType);					  //传统官方彩界面类型，PC为电脑版，MP为手机版，APP为APP版(MP和APP没有什么区别，可能某些机型APP适应性更好)
	}
	// 香港彩
	if (reqParam.lotterytype == LOTTERY_LOTTO)
		data.put("lottoTray", agent_info.lottoTray);//传统官方彩盘口可选择（A,B,C），注: 不能为空。
	// 时时彩
	if (reqParam.lotterytype == LOTTERY_LOTTERY)
	{
		data.put("lotteryTray", agent_info.lotteryTray);			  //传统官方彩盘口可选择（A,B,C），注: 不能为空。
		data.put("lotteryType", reqParam.gfcType);					  //界面类型，PC为电脑版，MP为手机版，APP为APP版(MP和APP没有什么区别，可能某些机型APP适应性更好)
		data.put("lotteryPage", std::to_string(reqParam.gameInfoId - 1)); //对应gameInfoId
		data.put("mobileVersion", agent_info.mobileVersion);		  //手机版本
	}

	root_pt.add_child("params", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root_pt, false);
	std::string jsonstr = ss.str();
	replace(jsonstr, ":line", std::to_string(agent_info.line));
	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");
	TirmString(jsonstr);

	string  reqHandle = reqParam.lotterytype == LOTTERY_GFC ? agent_info.gfc_login : (reqParam.lotterytype == LOTTERY_LOTTO ? agent_info.lotto_login:agent_info.lottery_login);
	// 发起请求
	resp = PostUrlRequest(jsonstr, reqHandle);
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
	string content_text;
	int64_t logid = 0;
	try
	{
		std::stringstream ss(resp.text);
		read_json(ss, req_pt);
		req_code = req_pt.get<int32_t>("errorCode");
		logid = req_pt.get<int64_t>("logId");
		if (req_code == 0)
		{
			content_text = req_pt.get<string>("params.link");
			reqParam.gamekurl = content_text;
		}
		else
		{
			errmsg = req_pt.get<string>("errorMessage");
		}
	}
	catch (exception excep)
	{
		LOG_ERROR << "---" << __FUNCTION__ << ",content_text["<< content_text <<"],what[" << excep.what()<<"]";
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
	}

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << resp.elapsed << "],logid[" << logid << "],content_text[" << content_text << "]";
	return true;
}


// 余额查询
bool LotteryServer::getBalanceIG(ReqParamCfg reqParam,third_game_cfg agent_info,int64_t & balance,int32_t & req_code,string & errmsg)
{
	// LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "]";
	boost::property_tree::ptree root_pt,data,req_pt;// 

	root_pt.put("hashCode",agent_info.hashcode);
	root_pt.put("command", "GET_BALANCE");//CHECK_REF");//
	data.put("username", reqParam.lgname);
	data.put("password", inicfg_.Password);
	root_pt.add_child("params", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root_pt, false);
	std::string jsonstr = ss.str();
	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");
	TirmString(jsonstr);
 
	cpr::Response resp = PostUrlRequest(jsonstr, agent_info.gfc_money);
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
		errmsg 		= req_pt.get<string>("errorMessage");
		req_code 	= req_pt.get<int32_t>("errorCode");
	}
	catch (exception excep)
	{
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "],resp.text[" << resp.text << "]";
		return false;
	}

	// 有可能有其它的错误返回
	if(req_code != (int32_t)eErrorCode::NoError ){
		return false;
	}

	// 获取余额
	double fbalance = req_pt.get<double>("params.balance"); //req_pt.get_child("data"); 
	string balanceStr = to_string(fbalance);
	balance = GlobalFunc::rate100(balanceStr);

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],params.balance[" << fbalance << "],balance[" << balance << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}
 // 查询订单状态
bool LotteryServer::orderInquiryParams(ReqParamCfg reqParam,third_game_cfg agent_info,string & billNo,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",billNo[" << billNo << "]";

	boost::property_tree::ptree root,req_pt;
	 
	root.put("method", "qos");					//接口类型：tc
	root.put("billNo", billNo); 				//唯一且长度为20位的订单号
	root.put("actype", ":actype");				//账号类型：0-试玩账号 1-真钱账号
	root.put("cur", inicfg_.MoneyType);			//货币种类-CNY

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	replace(jsonstr, ":actype", std::to_string(reqParam.actype)); 
	TirmString(jsonstr);

	resp = getUrlRequest(jsonstr,agent_info);//inicfg_.resAddUrl
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
string LotteryServer::createTransferParams(ReqParamCfg reqParam,third_game_cfg agent_info, string billNo, string type,string creditStr)
{
	// LOG_INFO << "---" << __FUNCTION__ << ",lgname[" << lgname << "]";
	// 返回数据 IN 转进钱包 OUT 转出钱包
	string cmdStr = (type.compare(TRANS_IN_DT2TXH) == 0) ? "DEPOSIT" : "WITHDRAW"; //WITHDRAW取款,DEPOSIT存款

	boost::property_tree::ptree root_pt,data;// 

	root_pt.put("hashCode",agent_info.hashcode);
	root_pt.put("command", cmdStr);//CHECK_REF");//
	data.put("username", reqParam.lgname);
	data.put("password", inicfg_.Password);
	data.put("ref", billNo);
	data.put("desc", "");
	data.put("amount", creditStr);
	
	root_pt.add_child("params", data);
 
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root_pt, false);
	std::string jsonstr = ss.str();
	// replace(jsonstr, ":amount", std::to_string(0));
	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");
	TirmString(jsonstr);

	return jsonstr;
}

// 游戏转账（上下分）
//type:IN: 从网站帐号转款到游戏帐号,OUT：从游戏转出 , int64_t credit
//credit(score): 转账金额，保留小数点后两位
bool LotteryServer::transferBalance(int64_t &newScore,ReqParamCfg reqParam,third_game_cfg agent_info, string billNo, string type,
									int64_t & _newbalance, int32_t &req_code, string &errmsg, cpr::Response &resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",billNo["<< billNo <<"],lgname["<< reqParam.lgname <<"],type["<< type <<"],credit["<< reqParam.score <<"]"; 

	// 1.1数字转小数点，保留小数点后两位
	string creditStr = GlobalFunc::getDigitalFormat(reqParam.score);

	// 返回数据
	std::string jsonstr = createTransferParams(reqParam,agent_info,billNo,type,creditStr);

	int64_t oldbalance = 0;
	int64_t newbalance = 0;
	boost::property_tree::ptree req_pt;

	// 2,写订单记录agent_info.platformCode
	createOrder(to_string(reqParam.gametype),reqParam.platformCode,billNo, reqParam.lgname,type,reqParam.score,reqParam.userid,reqParam.account);

	// 3,请求上下分
	resp = PostUrlRequest(jsonstr, agent_info.gfc_money);
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
		updateOrder(newScore,reqParam.userid,type,reqParam.score,billNo,(int32_t)eOrderType::OpTiemOut,resp.status_code,resp.elapsed);
		return false;
	}

	// 取结果码
	try
	{
		std::stringstream ss(resp.text);
		read_json(ss, req_pt);
		errmsg = req_pt.get<string>("errorMessage");
		req_code = req_pt.get<int32_t>("errorCode");
	}
	catch (exception excep)
	{
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "],resp.text[" << resp.text << "]";
		updateOrder(newScore,reqParam.userid,type,reqParam.score,billNo,(int32_t)eOrderType::OpJsonErr,resp.status_code,resp.elapsed);
		return false;
	}

	// 有可能有其它的错误返回
	if (req_code != (int32_t)eErrorCode::NoError)
	{
		updateOrder(newScore,reqParam.userid,type,reqParam.score,billNo,(int32_t)eOrderType::OpUndefind,resp.status_code,resp.elapsed);
		return false;
	}

	// 获取余额(存款后余额)
	double fbalance = req_pt.get<double>("params.balance");
	string balanceStr = to_string(fbalance);
	newbalance = GlobalFunc::rate100(balanceStr);
	_newbalance = newbalance;
	oldbalance = newbalance - reqParam.score;

	// 写成功后注单
	updateOrder(newScore,reqParam.userid,type,reqParam.score,billNo,(int32_t)eOrderType::OpFinish,resp.status_code,resp.elapsed,oldbalance,newbalance);

	// 5,处理成功
	LOG_INFO << "---转帐单成功---userid[" << reqParam.userid << "],txhaccount[" << reqParam.lgname << "],操作金额[" << creditStr << "],elapsed[" << resp.elapsed << "],对方帐户转前[" << oldbalance << "],转后[" << newbalance << "]";
	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],当前玩家分[" << newScore << "],status_code[" << resp.status_code << "],billNo["<< billNo <<"]";
	return true;
}
// 进入游戏
// @mh5 y：代表h5 ，n：代表pc
// @oddtype 值A（20-50000）、B（50-5000）、C（20-10000）、D（200-20000）、E（300-30000）、F（400-40000）、G（500-50000）、H（1000-100000）、I（2000-200000）
bool LotteryServer::enterGameParams(string & lgname,string mh5,string oddtype,string gametype,string & req_forwardUrl,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "],oddtype[" << oddtype <<"]";

	boost::property_tree::ptree root,req_pt; 
	root.put("loginname", lgname);
	root.put("password", inicfg_.Password);
	root.put("method", "lgg");
	root.put("mh5", mh5);						//y：代表h5 ，n：代表pc
	root.put("oddtype", oddtype);				//盘口，设定玩家可下注的范围，不传默认为A
	root.put("gametype", gametype);				//盘口，设定玩家可下注的范围，不传默认为A

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false);
	std::string jsonstr = ss.str();
	TirmString(jsonstr);

	third_game_cfg agent_info;

	resp = getUrlRequest(jsonstr, agent_info);//inicfg_.enterGameHandle
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
	// Crypto::AES_ECB_Encrypt(inicfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
	// req_forwardUrl = URL::Encode(tmpEncode);

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],forwardUrl[" << forwardUrl << "],req_forwardUrl[" << req_forwardUrl << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}

// 统一请求参数之获取
cpr::Response LotteryServer::PostUrlRequest(string & parameContent,string & reqHandle)//, cpr::Payload param)
{
	LOG_DEBUG << " 请求内容[" << parameContent << "],请求接口[" << reqHandle << "]";

    auto resp_feature = cpr::Post(cpr::Url(reqHandle),cpr::Body{parameContent},cpr::Header{{"content-type", "application/json"}});

	std::cout << "resp_feature.text[" << resp_feature.text << "],line[" << __LINE__ <<"]" << std::endl;
  
	LOG_INFO << "---" << __FUNCTION__ << " elapsed[" << resp_feature.elapsed << "],status_code[" << resp_feature.status_code << "],text[" << resp_feature.text << "]";

	// 请求超时打印
	if(resp_feature.elapsed * 1000 >= inicfg_.Timeout){
		std::cout << "============================================" << std::endl;
		LOG_ERROR << "---请求超时[" << parameContent << "]";  
		std::cout << "============================================" << std::endl;
	}
	// 统一请求参数之获取
	return resp_feature; 
}

// 
// 统一请求参数之获取
cpr::Response LotteryServer::getUrlRequest(string & parameContent,third_game_cfg agent_info)
{
	// 签名算法
	std::string strDesOut;
	Crypto::DES_ECB_Encrypt((unsigned char *)agent_info.DesCode.c_str(), (unsigned char *)"", parameContent, strDesOut);
	LOG_INFO << "---" << __FUNCTION__ << "---\n---des_key[" << agent_info.DesCode << "],strDesIn[" << parameContent << "]"
			 << "\n---md5_key[" << agent_info.MD5Code << "],strDesOut[" << strDesOut << "]";

	// 2,统一请求参数之key
	std::string strMd5, src = strDesOut + agent_info.MD5Code;
	char md5[32 + 1] = {0};
	MD5Encode32(src.c_str(), src.length(), md5, 0);
	strMd5 = string((char *)md5);

	// 3,拼接参数
	auto param = cpr::Parameters{};
	param.AddParameter({"params",URL::Encode(strDesOut)});
	param.AddParameter({"key", strMd5.c_str()});

	string _requrl = agent_info.ApiUrl;
	if (!inicfg_.resAddUrl.empty())
	{
		 _requrl = _requrl + "/" + inicfg_.resAddUrl;
	}

	LOG_INFO << "---req string ---[" << _requrl << "?" <<param.content << "]";  

	auto resp_feature = cpr::Get(cpr::Url{_requrl},cpr::Timeout{inicfg_.Timeout}, cpr::Header{{"AgentName", agent_info.agent}}, param);

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
bool LotteryServer::getJsonPtreeEx(string &jsonStr, string &text, int64_t & logId,int32_t &code, string &msg,boost::property_tree::ptree & pt)
{
	if (jsonStr.empty())
		return false;
	try
	{
		std::stringstream ss(jsonStr);
		read_json(ss, pt);
		code = pt.get<int32_t>("errorCode");
		logId = pt.get<int64_t>("logId");
		if (code == 0)
		{
			text = pt.get<string>("params.link");
		}
		else
		{
			msg = pt.get<string>("errorMessage");
		}
	}
	catch (exception excep)
	{
		LOG_ERROR << "---" << __FUNCTION__ << ",jsonStr["<< jsonStr <<"],what[" << excep.what()<<"]";
		return false;
	}
	return true;
}

// 解析请求返回结果
bool LotteryServer::getJsonPtree(string &jsonStr, int32_t &code, string &msg,boost::property_tree::ptree & pt)
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
int32_t LotteryServer::checkGameIDValid(ReqParamCfg &reqParam,string & errmsg)
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
	reqParam.lotterytype = iter->second.lotterytype;
	reqParam.platformCode = iter->second.companyCode;
	reqParam.gameInfoId = iter->second.gameInfoId;

	LOG_INFO << "---" << __FUNCTION__ << " gameid[" << iter->second.gameid << "],gamename[" << iter->second.gamename << "],gameInfoId[" << iter->second.gameInfoId << "],lotterytype[" << iter->second.lotterytype << "]";
	return (int32_t)eErrorCode::NoError;
}

// 检查请求参数
int32_t LotteryServer::checkReqParams(string & reqStr,string & errmsg,ReqParamCfg &reqParam)
{
	std::string paraValue;
	int32_t errcode = (int32_t)eErrorCode::NoError;

	// 测试使用，不带参数
	if( inicfg_.isdecrypt == 0 ) return errcode;

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

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_1)
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
			paraValue = Crypto::AES_ECB_Decrypt(inicfg_.AesKey, strURL);
			LOG_INFO << "---AES ECB Decrypt[" << paraValue << "],key[" << inicfg_.AesKey << "]";
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
					reqParam.userid = atol(useridKey->second.c_str());
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
				reqParam.actype = atol(actypeKey->second.c_str());
			}

			// 配置文件写死
			if(inicfg_.Actype < 2) 
				reqParam.actype = inicfg_.Actype;

			//lgtype
			HttpParams::const_iterator lgtypeKey = paraValueParams.find("lgtype");
			if (lgtypeKey != paraValueParams.end() && (!lgtypeKey->second.empty()))
			{
				//gfcType PC为电脑版，MP为手机版，APP为APP版
				reqParam.lgtype = lgtypeKey->second;
				if(reqParam.lgtype.compare("y") == 0)
					reqParam.gfcType = "MP";
				else if (reqParam.lgtype.compare("n") == 0)
					reqParam.gfcType = "PC";
				// else if (reqParam.lgtype.compare("a") == 0)
				// 	reqParam.gfcType = "APP";
			}

			//oddtype
			HttpParams::const_iterator oddtypeKey = paraValueParams.find("oddtype");
			if (oddtypeKey != paraValueParams.end() && (!oddtypeKey->second.empty()))
			{
				reqParam.oddtype = oddtypeKey->second;
			}

			// backurl
			HttpParams::const_iterator backurlKey = paraValueParams.find("backurl");
			if (backurlKey != paraValueParams.end() && (!oddtypeKey->second.empty()))
			{
				reqParam.backurl = backurlKey->second;
			}

			LOG_INFO << "---backurl["<< reqParam.backurl << "]";

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
				// 111021001
				reqParam.gameid = atol(gameidKey->second.c_str());
				
				// 游戏类型
				if(reqParam.gameid > 0){
					int32_t gametypetest = getTidByGameid(reqParam.gameid);
					if(reqParam.gametype != gametypetest)
						LOG_ERROR << "gametype[" << reqParam.gametype <<"],gametypetest[" << gametypetest <<"]" ;
					reqParam.gametype = getTidByGameid(reqParam.gameid);// (reqParam.gameid / 10000)%1000;
				}

				// 平台代码
				int companyID = reqParam.gameid / 10000000;//101010001

				auto iter = m_CompanyNoCodeMap.find(companyID);
				if(iter != m_CompanyNoCodeMap.end()){
					reqParam.platformCode = iter->second;
 				}
				LOG_WARN << "gameid[" << reqParam.gameid <<"],companyID[" << companyID <<"],platformCode[" << reqParam.platformCode << "],gametype[" << reqParam.gametype << "]" ;
			}

			LOG_INFO << "---gametype["<< reqParam.gametype << "],gameid["<< reqParam.gameid << "],platformCode["<< reqParam.platformCode << "]";
		}
		catch (const std::exception &e)
		{
			errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
			errmsg = "param error";
			LOG_ERROR << "Params Error failed," << e.what();
			break;
		}
	
	} while (0);

	LOG_WARN << "---errcode["<< errcode << "],errmsg["<< errmsg <<"],paraValue lgtype[" << reqParam.lgtype << "],oddtype[" << reqParam.oddtype << "],userid[" << reqParam.userid << "]";

	return errcode;
}

// 检查请求参数,三个关键参数 [操作类型,操作分数,公司代码,游戏类型]
int32_t LotteryServer::checkReqParamsEx(string & reqStr,ReqParamCfg &reqParam, string & errmsg)
{
	std::string paraValue;
	int32_t errcode = (int32_t)eErrorCode::NoError;

	// 测试使用，不带参数
	if( inicfg_.isdecrypt == 0 ) return errcode;

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

			if (inicfg_.isLog < (int32_t)eLogLvl::LVL_1)
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
			paraValue = Crypto::AES_ECB_Decrypt(inicfg_.AesKey, strURL);
			LOG_INFO << "---AES ECB Decrypt[" << paraValue << "],key[" << inicfg_.AesKey << "]";
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
					reqParam.userid = atol(useridKey->second.c_str());
				}
				else
				{
					errcode = (int32_t)eErrorCode::ParamsError; // 传输参数错误
					errmsg = "userid error,userid[" + useridKey->second + "]";
					break;
				}
			}

			// userid=250113861&aid=111228&operatetype=0&gameid=111021001&opscore=100
			// 
			HttpParams::const_iterator aidKey = paraValueParams.find("aid");
			if (aidKey != paraValueParams.end() && !aidKey->second.empty() && IsDigitStr(aidKey->second))
			{
				reqParam.aid = atol(aidKey->second.c_str());
			}

			// companyCode 
			HttpParams::const_iterator platformCodeKey = paraValueParams.find("companyCode");
			if (platformCodeKey != paraValueParams.end() && !platformCodeKey->second.empty())
			{
				reqParam.platformCode = platformCodeKey->second;
			}
			// gametype 
			HttpParams::const_iterator gametypeKey = paraValueParams.find("gametype");
			if (gametypeKey != paraValueParams.end() && !gametypeKey->second.empty())
			{
				reqParam.gametype = atol(gametypeKey->second.c_str());
			}
			// operatetype 
			HttpParams::const_iterator optypeKey = paraValueParams.find("operatetype");
			if (optypeKey != paraValueParams.end() && !optypeKey->second.empty())
			{
				reqParam.operatetype =  atol(optypeKey->second.c_str());
			}

			// opscore 
			HttpParams::const_iterator opscoreKey = paraValueParams.find("opscore");
			if (opscoreKey != paraValueParams.end() && !opscoreKey->second.empty())
			{
				reqParam.opscore = atol(opscoreKey->second.c_str());
			}

			// gameid 
			HttpParams::const_iterator gameidKey = paraValueParams.find("gameid");
			if (gameidKey != paraValueParams.end() && !gameidKey->second.empty())
			{
				// 111021001
				reqParam.gameid = atol(gameidKey->second.c_str()); // 游戏类型
				// 游戏类型
				if(reqParam.gameid > 0)
					reqParam.gametype = getTidByGameid(reqParam.gameid);// (reqParam.gameid / 10000)%1000;

				// 平台代码
				int companyID = reqParam.gameid / 10000000;//101010001

				auto iter = m_CompanyNoCodeMap.find(companyID);
				if(iter != m_CompanyNoCodeMap.end()){
					reqParam.platformCode = iter->second;
 				}
				LOG_INFO << "---gameid[" << reqParam.gameid <<"],companyID[" << companyID <<"],platformCode[" << reqParam.platformCode << "],gametype[" << reqParam.gametype << "]" ;
			}

			LOG_INFO << "---请求参数,userid["<< reqParam.userid << "],operatetype["<< reqParam.operatetype << "],opscore["<< reqParam.opscore <<"],platformCode[" << reqParam.platformCode << "]";
		}
		catch (const std::exception &e)
		{
			errcode = (int32_t)eErrorCode::ParamsError; // 参数转码或代理解密校验码错误
			errmsg = "param error";
			LOG_ERROR << "Params Error failed," << e.what();
			break;
		}
	
	} while (0);

	LOG_WARN << "---errcode["<< errcode << "],errmsg["<< errmsg <<"],userid[" << reqParam.userid << "],aid[" << reqParam.aid << "]";

	return errcode;
}
// 设置锁(使用条件:允许锁的偶尔失效)
bool LotteryServer::setNxDelay(int64_t lckuid){

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
bool LotteryServer::lockUserScore(int64_t uid,int32_t opType, int32_t & req_code,string & errmsg)
{
	try
	{
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
			// errmsg = "score lock ok";
			return true;
		}
		else{
			// 操作失败
			LOG_WARN << __FUNCTION__ << " 操作失败，玩家[" << uid <<"],操作类型[" << opType << "]";
			errmsg = "score lock failure";
			// return false;
		}
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",操作失败,玩家[" << uid <<"],操作类型[" << opType << "]";
		errmsg = "score lock failure";
		return false;
	}

	return false;
}

// 解锁用户操作
bool LotteryServer::unlockUserScore(int64_t uid,int32_t opType,int32_t & req_code,string & errmsg)
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

int32_t LotteryServer::getUserScore(int64_t &score, int64_t uid)
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

// int64_t uid, int32_t & aid, string &lgname,int64_t & score,string & da_account,
// 根据玩家Id获取用户信息 .userid,param.aid,lg_name,param.score,param.account
bool LotteryServer::getUserNameById(ReqParamCfg &reqParam,int32_t & req_code,string & errmsg)
{
	// 测试使用，不带参数
	if( inicfg_.isdecrypt == 0 ) return true; 

	try
	{
		// 加锁等待1秒
		// setNxDelay(reqParam.userid);
		//查询game_user表
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1
															 << "account" << 1 
															 << "agentid" << 1 
															 << "nickname" << 1 
															 << "score" << 1 
															 << "txhstatus" << 1 
															 << "txhaccount" << 1 << bsoncxx::builder::stream::finalize);
		mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
		bsoncxx::document::value findValue = document{} << "userid" << reqParam.userid << finalize;
		auto result = user_coll.find_one(findValue.view(), opts);
		if (!result)
		{
			req_code = (int32_t)eErrorCode::UserNotExistsError;
			errmsg = "not exist this userid[" + to_string(reqParam.userid) + "]";
			return false;
		}

		bsoncxx::document::view view = result->view();
		int32_t agentId = view["agentid"].get_int32();
		reqParam.score 			= view["score"].get_int64();
		reqParam.account 		= view["account"].get_utf8().value.to_string();
		reqParam.nickname 		= view["nickname"].get_utf8().value.to_string();
		reqParam.aid = agentId;
		// 取得唯一虚拟帐号
		reqParam.lgname = view["txhaccount"].get_utf8().value.to_string();
		reqParam.lastgameid = view["txhstatus"].get_int32();
		LOG_INFO << "---获取用户信息,lastgameid[" << reqParam.lastgameid << "],aid[" << reqParam.aid << "],score[" << reqParam.score << "],uid[" << reqParam.userid << "]";
	}
	catch (bsoncxx::exception &e)
	{
		req_code = (int32_t)eErrorCode::InsideErrorOrNonExcutive;
		errmsg = "DB error,userid[" + to_string(reqParam.userid) + "]";
		LOG_ERROR << "---" << __FUNCTION__ <<" MongoDBException: " << e.what();
		return false;
	}

	return true;
}
 
// 维护状态
bool LotteryServer::getMaintenanceStatus(int32_t & req_code,string & errmsg)
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
bool LotteryServer::getProxyXianHong(int32_t aid,string & oddtype)
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
void LotteryServer::processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
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

	int32_t req_code = (int32_t)eErrorCode::NoError; 
	std::string rspdata,resp_msg,req_msg = "ok";
	std::string reqStr = req.query();
	// 接口请求处理
	if (req.path() == "/TransHandle") 
	{
		m_TxhAPI->getWGameUrl(reqStr,rspdata);
	}
	else if (req.path() == "/LotteryHandle") 
	{
		std::string req_forwardUrl,resp_msg,lg_name = VERSION;
		try
		{
			// 参数
			bool islock = false;
			std::string billNo;
			int32_t aid = 0; 
			int64_t balance = 0,newbalance = 0; 
			cpr::Response resp;

			third_game_cfg  agentInfo;
			ReqParamCfg param;
			param.oddtype = inicfg_.oddtype;
			param.lgtype = "y"; 
			param.gfcType = "PC"; 
			
			do
			{ 
				// Check 维护状态
				if(getMaintenanceStatus(req_code,req_msg))
					break; 

				// 0,解析请求参数并获取玩家帐号信息----------------------------------------------------------------------------
				// (1)检查参数
				req_code = checkReqParams(reqStr,req_msg,param);//actype,lgType,oddtypeStr,backurl,
				if( req_code != (int32_t)eErrorCode::NoError )
					break;

				// 游戏暂未开放
				if(inicfg_.NotOpen == 1){
					req_code = (int32_t)eErrorCode::NotOpen;
					break;
				}

				// (2)检查玩家是否天下汇，是则直接退出
				if(param.userid == 0){ 
					req_code = (int32_t)eErrorCode::UserNotExistsError;
					break;
				}

				// 1.3 检查游戏参数
				if((req_code = checkGameIDValid(param ,req_msg)) != (int32_t)eErrorCode::NoError){
					LOG_ERROR << req_msg;
					break;
				}

				if(!getCfgByCode(param.platformCode,agentInfo,req_code,req_msg)){
					LOG_ERROR << req_msg;
					break;
				}

				// (2.1)玩家正在游戏中，不能跳转
				if (REDISCLIENT.ExistsUserOnlineInfo(param.userid))
				{
					req_msg = "userid " + to_string(param.userid) + " is playing!";
					req_code =(int32_t)eErrorCode::PlayerOnLine;
					LOG_ERROR << req_msg;
					break;
				}

				// (2.2)(安全性判断)
				if(!lockUserScore(param.userid,1,req_code,req_msg)){
					LOG_ERROR << "---" << __FUNCTION__ << " req_code:" << req_code << ",req_msg[" << req_msg <<"],userid[" << param.userid << "]";
					break;
				}
				// 已锁
				islock = true;

				// (3)查询game_user表
				if(!getUserNameById(param,req_code,req_msg) || (req_code != 0))
					break; 

				// 1,检测并创建游戏帐号----------------------------------------------------------------------------------------
				if(!getCreateAccountParams(param,agentInfo,req_code,req_msg,resp) || (req_code != 0))
					break;

				// 默认免转，自动带分进入
				if( param.score > 0 )
				{
					if(inicfg_.LimitTestScore > 0)
						param.score = param.score > inicfg_.LimitTestScore ? inicfg_.LimitTestScore : param.score;

					// 先创建订单
					billNo = param.platformCode + GlobalFunc::getBillNo(param.userid);

					// 3，游戏转账（上分）-----------------------------------------------------------------------------------------
					int64_t newScore = 0;
					if (!transferBalance(newScore, param, agentInfo, billNo, TRANS_IN_DT2TXH, newbalance, req_code, req_msg, resp))
						break;

					// 4，查询订单状态----------------------------------------------------------------------------------------------
					//过滤代理额度不足，404等
					if (req_code == muduo::net::HttpResponse::k404NotFound)
						break;
					else if (req_code != (int32_t)eErrorCode::NoError)
						break; 
				}

				// 5，创建跳转记录
				updatePalyingRecord(param.lgname, param.account, param.userid, param.score, newbalance, param.gameid);
				// 返回连接
				req_forwardUrl = param.gamekurl + "&burl=" + param.backurl;
			} while (0);

			// 请求时长（毫秒）
			elapsedtimd = muduo::timeDifference(muduo::Timestamp::now(), timestart)  * 1000;
			
			// 是否超时判断
			bool isTimeOut = int32_t(resp.elapsed * 1000) > inicfg_.Timeout;
			if (isTimeOut) req_msg = "req time out " + to_string(resp.elapsed)+ "," + req_msg;
			
			// 返回结果
			rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,req_code,param.userid,lg_name,elapsedtimd,req_msg,req_forwardUrl);

			// Log rspdata
			if( inicfg_.isLog < (int32_t)eLogLvl::LVL_3 )
				LOG_DEBUG << "---LotteryHandle----isdecrypt[" << inicfg_.isdecrypt << "],[" << rspdata << "]";

			// 解锁玩家分
			if( inicfg_.TestLock == 0 && islock)
				unlockUserScore(param.userid,1,req_code,req_msg);
		}
		catch (exception excep)
		{
			resp_msg = "transfer exception" ;
			LOG_ERROR << "---" << __FUNCTION__ << " " << excep.what() <<",req.query[" << req.query() <<"]";
			rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,(int32_t)eErrorCode::InsideErrorOrNonExcutive,0,lg_name,0,resp_msg,req_forwardUrl);
		} 
	} 
	else if (req.path() == "/Transfer") //第三方游戏公用转帐接口
	{
		// 参数
		bool islock = false;
		int32_t aid = 0, actype = 0;
		int64_t balance = 0, userid = 0, score = 0, newbalance = 0;
		cpr::Response resp;

		third_game_cfg agentInfo;
		ReqParamCfg param;
		
		try
		{
			do
			{
				// Check 维护状态
				if (getMaintenanceStatus(req_code, req_msg))
					break;

				// 0,解析请求参数并获取玩家帐号信息----------------------------------------------------------------------------
				// (1)检查参数
				req_code = checkReqParamsEx(reqStr, param, req_msg);
				if (req_code != (int32_t)eErrorCode::NoError)
					break;

				// 游戏暂未开放
				if (inicfg_.NotOpen == 1)
				{
					req_code = (int32_t)eErrorCode::NotOpen;
					break;
				}

				// (2)检查玩家是否OK，是则直接退出
				if (param.userid == 0)
				{
					req_code = (int32_t)eErrorCode::UserNotExistsError;
					break;
				}

				// (2.1)参数验证
				if (param.operatetype < eTransType::op_Dt2Txh || param.operatetype > eTransType::op_recycle_all)
				{
					req_code = (int32_t)eErrorCode::ParamsError;
					break;
				}

				// 1,增加频率限制
				string key = REDIS_LOGIN_3S_CHECK + to_string(param.operatetype) + to_string(param.userid);
				if (REDISCLIENT.exists(key))
				{
					req_code = (int32_t)eErrorCode::ReqFreqError; 
					break;
				}
				else
					REDISCLIENT.set(key, key, 1); //重置3秒

				// (3)查询game_user表
				if (!getUserNameById(param, req_code, req_msg) || (req_code != 0))
					break;

				// 初始化各玩家的平台余额
				initPlayerPlatBalance(param.userid);

				/* 一键查询余额 */
				if (param.operatetype == eTransType::op_balance_all)
				{
					LOG_DEBUG << "===============[一键查询余额]===============";
					req_code = get_balance_all(param,rspdata,req_msg);
					break;
				}

				// (2.2)(安全性判断)
				if(!lockUserScore(param.userid,1,req_code,req_msg)){
					LOG_ERROR << "---" << __FUNCTION__ << " req_code:" << req_code << ",req_msg[" << req_msg <<"],userid[" << param.userid << "]";
					break;
				}
				// 已锁
				islock = true;

				/* 一键回收余额 */
				if (param.operatetype == eTransType::op_recycle_all)
				{
					LOG_DEBUG << "===============[一键回收余额]===============";
					get_recycle_all(param,rspdata,req_msg);
					break;
				}

				// 操作分数
				if (param.opscore < 0){
					req_code = (int32_t)eErrorCode::OperateScoreErr;
					LOG_ERROR << "userid["<< param.userid <<"]转账金额不能为负数[" << param.opscore << "]";
					break;
				}

				// 获取平台信息
				if ((getCfgByCode(param.platformCode, agentInfo, req_code, req_msg) == false) && (param.gameid == 0))
				{
					LOG_ERROR << req_msg; break;
				}

				/* 普通转帐操作 */
				if (param.operatetype < eTransType::op_balance_all)
				{
					LOG_DEBUG << " ===============[普通转帐操作]===============,GameID[" <<param.gameid << "],分值[" <<param.opscore << "]";
					// IG转帐
					if (param.platformCode.compare(COMPANY_CODE_IG) == 0 || param.platformCode.compare(COMPANY_CODE_PJ) == 0)
					{
						if (TransferIG(param, agentInfo, newbalance, req_code, req_msg))
							rspdata = writeJson4balance(newbalance, req_code, req_msg, param.platformCode, param.gametype);
					}

					if (param.platformCode.compare(COMPANY_CODE_WG) == 0)
					{
						string TRANS_OUT_OR_IN,errormsg;
						// 从第三方转出
						if (param.operatetype >= eTransType::op_Txh2Dt && param.operatetype != eTransType::op_balance_all)
						{
							// 2，余额查询
							bool status_online = false;
							cpr::Response resp;
							if (m_TxhAPI->getBalance(param.lgname,balance,req_code,errormsg,resp,status_online)){
								if (req_code != 0 || balance == 0)
									break;
								if(status_online == true){
									req_code = (int32_t)eErrorCode::PlayerOnLine;
									break;
								}
							}
							// 余额是否全部回收
							TRANS_OUT_OR_IN = TRANS_OUT_TXH2DT;
							param.score = param.operatetype == eTransType::op_Txh2Dt_all ? balance : param.opscore;
							param.score = param.operatetype == eTransType::op_recycle_all ? balance : param.score;
						}
						// 转到第三方
						else if (param.operatetype >= eTransType::op_Dt2Txh && param.operatetype < eTransType::op_Txh2Dt)
						{
							if ((param.score <= 0) || (param.operatetype == eTransType::op_Dt2Txh) && (param.opscore <= 0))
								break;
							// 余额是否全部转出
							TRANS_OUT_OR_IN = TRANS_IN_DT2TXH;
							param.score = param.operatetype == eTransType::op_Dt2Txh_all ? param.score : param.opscore;
						}
						// 先创建订单
						string billNo = GlobalFunc::getBillNo(userid);
						int64_t curScore = 0;
						if (m_TxhAPI->transferBalance(param.gametype,param.platformCode,curScore, param.lgname, billNo, TRANS_OUT_OR_IN, param.userid, param.score, newbalance, req_code, req_msg, param.account, resp))
						{
							// 更新缓存玩家分
							savePlayerBalance(param.userid,newbalance,param.gametype,param.platformCode);
						}
						
						rspdata = writeJson4balance(newbalance, req_code, req_msg, param.platformCode, param.gametype);
						LOG_DEBUG <<"视讯" << rspdata <<"]"; 
					}

					// 已经成功回收玩家分，则可清除标志 (req_code == (int32_t)eErrorCode::NoError) &&
					if ( (param.gameid > 0) && (param.operatetype == eTransType::op_Txh2Dt_all))
						updatePalyingStatus(param.userid);
				}

			} while (0);

			// 返回结果
			if ((req_code != (int32_t)eErrorCode::NoError))//&& (param.operatetype < eTransType::op_balance_all)
				rspdata = createJsonRsp(param.operatetype, req_code, param.userid, param.lgname, 0, req_msg, NOT_NEED_UPDATE);

			// 解锁玩家分
			if( inicfg_.TestLock == 0 && islock)
				unlockUserScore(param.userid,1,req_code,req_msg);
		}
		catch (exception excep)
		{
			resp_msg = "transfer exception";
			rspdata = createJsonRsp(param.operatetype, (int32_t)eErrorCode::InsideErrorOrNonExcutive, 0, param.lgname, 0, resp_msg, NOT_NEED_UPDATE);
			LOG_ERROR << "---" << __FUNCTION__ << " " << excep.what() << ",req.query[" << reqStr << "]";
		}
		
		// 向TG发送信息返回码
		if (req_code != (int32_t)eErrorCode::NoError )
		{
			auto it = errcode_info_.find(req_code);
			if (it != errcode_info_.end())
				req_msg = it->second; 
			string strtmp = inicfg_.isDemo == 0 ? "线上" : "试玩";
			GlobalFunc::sendMessage2TgEx(getTgMsg(param.userid, param.lgname, "00001", req_code, req_msg, elapsedtimd, strtmp), inicfg_.botToken, inicfg_.botChairId);
		}
	}
	else
	{
		int32_t num = m_reciv_num.incrementAndGet();
		rsp.setContentType("text/html;charset=utf-8"); //"text/plain;charset=utf-8");
		rspdata = "time:" + to_string(num) + "<html><head><title>httpServer</title></head>"
											   "<body><h1>Not Found</h1>"
											   "</body></html>";
		rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
	}

	// 填充
	rsp.setBody(rspdata);

	// 统计毫秒
	if (inicfg_.isLog < (int32_t)eLogLvl::LVL_3)
		LOG_WARN << "---处理耗时(毫秒)[" << elapsedtimd << "],MsgCount[" << nReqCount_ << "],OKCount[" << nReqOKCount_ << "],ErrCount[" << nReqErrCount_ << "]";
}

// 回收所有的彩票
int32_t LotteryServer::get_recycle_all(ReqParamCfg param,string &rspdata,string &req_msg)
{
	int32_t req_code = 0;
	int64_t balance = 0,newbalance = 0;
	string errormsg;
	// 回收余额
	for (auto iter = agent_info_.begin(); iter != agent_info_.end(); iter++)
	{
		// 获取平台信息
		auto it = agent_info_.find(iter->first);
		if (it == agent_info_.end())
			continue;
		
		// 回收IG和葡京彩票的余额
		if (iter->first.compare(COMPANY_CODE_IG) == 0 || iter->first.compare(COMPANY_CODE_PJ) == 0)
		{
			param.platformCode = iter->first;
			param.gametype = iter->second.gametype;
			TransferIG(param, iter->second, newbalance, req_code, errormsg);
			req_msg += errormsg + ".";
			LOG_INFO << "回收[" << param.platformCode <<"],Type[" << param.gametype <<"],ret["<< writeJson4balance(newbalance, req_code, req_msg, param.platformCode, param.gametype) << "]";
		}

		// 视讯
		if (iter->first.compare(COMPANY_CODE_WG) == 0 )
		{
			if (m_TxhAPI->recycleBalanceEx(newbalance, param.userid, req_code,req_msg) == 0 && req_code == (int32_t)eErrorCode::NoError)
			{
				// 更新缓存玩家分
				savePlayerBalance(param.userid,0,it->second.gametype,iter->first);
				LOG_INFO << "---WG视讯余额回收完成[" << newbalance << "]";
			}
		}
	}
	
	// 打包数据
	createCompanyBalanceJson(param.userid,param.operatetype,rspdata,req_msg);

	// 此值暂时没意义
	return req_code;
}

int32_t LotteryServer::savePlayerBalance(int64_t uid,int64_t balance,int32_t gametype,string companyCode)
{
	string key = companyCode + to_string(gametype);
	auto player_iter = player_plat_balance_.find(uid);
	if (player_iter == player_plat_balance_.end())
	{
		// 复制公共
		std::map<string, company_balance_t> company_balance_map = company_balance_;
		for (auto iter = company_balance_map.begin(); iter != company_balance_map.end(); iter++)
		{
			if (iter->first == key)
			{
				iter->second.balance = balance;
				LOG_DEBUG << "---找到对应的平台信息balance[" <<balance  << "]";
				break;
			}
		}

		player_plat_balance_[uid] = company_balance_map;
		LOG_DEBUG << "---" << __FUNCTION__ << " uid[" << uid << "],key[" << key << "],code[" <<companyCode  << "],gametype[" <<gametype  << "],balance[" <<balance  << "]";
		return 0;
	}

	LOG_DEBUG << "---" << __FUNCTION__ << " uid[" << uid << "],key[" << key << "],code[" << player_iter->first  << "]";

	auto company_iter = player_iter->second.find(key);
	if (company_iter != player_iter->second.end()){
		company_iter->second.balance = balance;// company_balance_t

		string msgStr = strIpAddr_ + "," + to_string(uid) + "," + key + "," + to_string(balance);
		LOG_INFO << "---" << __FUNCTION__ << " 发广播,用户ID[" << uid << "],key[" << key << "],消息内容[" << msgStr  << "]";
		REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_synchronize_score, msgStr);
	}

	return 0;
}

// 同步更新玩家缓存分数
bool LotteryServer::updatePlayerPlatBalance(string msgStr)
{
	LOG_INFO << "---" << __FUNCTION__ << " 收到广播,本机IP[" << strIpAddr_  << "],消息内容[" << msgStr  << "]";
	
	int64_t uid = 0,balance = 0;
	string key;
	{
		MutexLockGuard lock(m_sync_Lock);
		std::vector<std::string> addrVec;
		addrVec.clear();
		boost::algorithm::split(addrVec, msgStr, boost::is_any_of(","));
		if (addrVec.size() == 4)
		{
			// 不是自己
			if (strIpAddr_.compare(addrVec[0]) != 0)
			{
				uid = atoll(addrVec[1].c_str());
				balance = atoll(addrVec[3].c_str());
				key = addrVec[2];
			}
		}
	}

	// ee
	if (!key.empty() && (uid > 0))
	{
		auto player_iter = player_plat_balance_.find(uid);
		auto company_iter = player_iter->second.find(key);
		if (company_iter != player_iter->second.end())
		{
			company_iter->second.balance = balance;
			LOG_INFO << "---" << __FUNCTION__ << "　同步缓存,用户ID[" << uid << "],key[" << key << "],余额[" << balance << "]";
		}
	}
}
// 创建玩家个人平台余额信息
bool LotteryServer::initPlayerPlatBalance(int64_t uid)
{
	auto player_iter = player_plat_balance_.find(uid);
	if (player_iter == player_plat_balance_.end())
	{
		// 复制公共
		player_plat_balance_[uid] = company_balance_;
		LOG_INFO << "---" << __FUNCTION__ << " 用户ID[" << uid << "]";
	}

	return true;
}


// 打包各平台余额
int32_t LotteryServer::createCompanyBalanceJson(int64_t uid,int32_t operatetype,std::string & rspdata, std::string & req_msg,int32_t req_code)
{
	auto player_iter = player_plat_balance_.find(uid);
	if (player_iter == player_plat_balance_.end())
	{
		LOG_ERROR << "---" << __FUNCTION__ << " uid[" << uid << "]";
		LOG_ERROR << "---" << __FUNCTION__ << " uid[" << uid << "]";
		LOG_ERROR << "---" << __FUNCTION__ << " uid[" << uid << "]";
		LOG_ERROR << "---" << __FUNCTION__ << " uid[" << uid << "]";
		return 0;
	}
	// 带make_pair第一个参数为空串则为数组[]，否则为{}
	boost::property_tree::ptree array_pt;
	boost::property_tree::ptree root;
	root.put("type", ":type");
	root.put("code", ":code");
	for (auto iter = player_iter->second.begin(); iter != player_iter->second.end(); iter++)
	{
		boost::property_tree::ptree pt;
		pt.put("companyCode", iter->second.companyCode);
		pt.put("gametype", iter->second.gametype);
		pt.put("score", iter->second.balance);
		// 添加到数组
		array_pt.push_back(std::make_pair("", pt));
	}

	root.put("msg", req_msg);
	root.add_child("data", array_pt);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false); //, false 是否格式化
	std::string jsonstr = ss.str();
	replace(jsonstr, ":code", std::to_string(req_code));
	replace(jsonstr, ":type", std::to_string(operatetype));
	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");
	TirmString(jsonstr);
	rspdata = jsonstr;

	std::cout << "jsonstr:\n"  << jsonstr << '\n';
	return 0;
}

// 查询所有平台的余额
int32_t LotteryServer::get_balance_all(ReqParamCfg param,string &rspdata,string &req_msg)
{
	int32_t req_code = 0;
	int64_t balance = 0;
	string errormsg;

	// 2，余额查询-----
	for (auto iter = agent_info_.begin(); iter != agent_info_.end(); iter++)
	{
		// 获取平台信息
		auto it = agent_info_.find(iter->first);
		if (it == agent_info_.end())
			continue;
		
		// 平台信息
		errormsg = "";
		string key = iter->first + to_string(it->second.gametype);// third_game_cfg agentInfoTmp = it->second;
		
		// 回收IG和葡京彩票的余额
		if (iter->first.compare(COMPANY_CODE_IG) == 0 || iter->first.compare(COMPANY_CODE_PJ) == 0)
		{
			if (getBalanceIG(param, it->second, balance, req_code, errormsg) && (req_code == 0))
			{
				req_msg += "." + errormsg;
				// 更新缓存玩家分
				savePlayerBalance(param.userid,balance,it->second.gametype,it->second.platformCode);
			}
		}
		else if (iter->first.compare(COMPANY_CODE_WG) == 0)
		{ 
			bool status_online = false;
			cpr::Response resp;
			if (m_TxhAPI->getBalance(param.lgname,balance,req_code,errormsg,resp,status_online) && (req_code == 0) )
			{
				// 保存玩家分数
				savePlayerBalance(param.userid,balance,it->second.gametype,iter->first);
			}
			
			if (status_online == true)
				req_msg += "." + errormsg + "."+ "user." + to_string(param.userid) + " is playing game!";
			
			 
		}
	}
	// 打包数据
	createCompanyBalanceJson(param.userid,param.operatetype,rspdata,req_msg);

	return 0;
}

// 转帐回收
bool LotteryServer::TransferIG(ReqParamCfg param, third_game_cfg agentInfo,int64_t & newbalance, int32_t & req_code, string & req_msg)
{
	//本次请求开始时间戳(微秒)
	muduo::Timestamp timestart = muduo::Timestamp::now();
	int32_t elapsedtimd = 0;

	LOG_INFO << "---" << __FUNCTION__ << " ,userid[" <<param.userid << "],score[" <<param.score << "],gametype[" <<param.gametype << "],lgname[" <<param.lgname << "]";
	LOG_INFO << "---" << __FUNCTION__ << " ,操作分数[" <<param.opscore << "],操作类型[" <<param.operatetype << "],平台简称[" <<param.platformCode << "],平台简称A[" <<agentInfo.platformCode << "]";

	// 参数
	bool islock = false;
	int32_t actype = 0;
	int64_t balance = 0, newScore = 0;
	cpr::Response resp;
	string TRANS_OUT_OR_IN;

	do
	{
		// 从第三方转出
		if (param.operatetype >= eTransType::op_Txh2Dt && param.operatetype != eTransType::op_balance_all)
		{
			// 2，余额查询-------------------------------------------------------------------------------------------------
			if (!getBalanceIG(param, agentInfo, balance, req_code, req_msg) || (req_code != 0) || (balance == 0))
			{
				if(req_code != 0)
					break;

				if (balance == 0)
				{
					req_code = (int32_t)eErrorCode::NotNeedUpdate;
					req_msg = "平台方余额为0，不需要取款";
					break;
				}
			}

			TRANS_OUT_OR_IN = TRANS_OUT_TXH2DT;
			// 余额是否全部回收
			param.score = param.operatetype == eTransType::op_Txh2Dt_all ? balance : param.opscore;
			param.score = param.operatetype == eTransType::op_recycle_all ? balance : param.score;
		}
		// 转到第三方
		else if (param.operatetype >= eTransType::op_Dt2Txh && param.operatetype < eTransType::op_Txh2Dt)
		{
			if(param.score <= 0)
			{
				req_code = (int32_t)eErrorCode::ScoreInsufficient;
				req_msg = "玩家当前余额不足,不能操作转帐";
				break;
			}

			if(param.operatetype == eTransType::op_Dt2Txh && param.opscore <= 0)
			{
				req_code = (int32_t)eErrorCode::OperateScoreErr;
				req_msg = "操作金额不能为0";
				break;
			}

			TRANS_OUT_OR_IN = TRANS_IN_DT2TXH;
			// 余额是否全部转出
			param.score = param.operatetype == eTransType::op_Dt2Txh_all ? param.score : param.opscore;
		}

		// 3，游戏转账（上下分）
		// 先创建订单
		string billNo = param.platformCode + GlobalFunc::getBillNo(param.userid);
		if (!transferBalance(newScore, param, agentInfo, billNo, TRANS_OUT_OR_IN, newbalance, req_code, req_msg, resp))
			break;

	} while (0);

	// 更新缓存玩家分
	if (req_code == (int32_t)eErrorCode::NoError)
	{
		// 保存玩家分
		savePlayerBalance(param.userid,newbalance,param.gametype,param.platformCode);
	}

	// 请求时长（毫秒）
	elapsedtimd = muduo::timeDifference(muduo::Timestamp::now(), timestart) * 1000;

	// 是否超时判断
	bool isTimeOut = int32_t(resp.elapsed * 1000) > inicfg_.Timeout;
	if (isTimeOut)
		req_msg = "req time out " + to_string(resp.elapsed) + "," + req_msg;

	// 获取余额(存款后余额)
	LOG_INFO << "游戏转账完成,userid:" << param.userid << ",aid:" << agentInfo.aid << ",当前玩家分值:" << newScore << ",下分后第三方余额:" << newbalance << ",耗秒:" << elapsedtimd;

	return true;
}
// 
string LotteryServer::writeJson4balance(int64_t score,int32_t req_code,string req_msg,string companyCode,int32_t gametype )
{
	// 带make_pair第一个参数为空串则为数组[]，否则为{}
	boost::property_tree::ptree array_pt;
	boost::property_tree::ptree root;
	root.put("code", ":code");

	boost::property_tree::ptree pt;
	pt.put("companyCode", companyCode);
	pt.put("gametype", gametype);
	pt.put("score", score);
	array_pt.push_back(std::make_pair("", pt));

	root.put("msg", req_msg);
	root.add_child("data", array_pt);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, root, false); //, false 是否格式化
	std::string jsonstr = ss.str();
	replace(jsonstr, ":code", std::to_string(req_code));
	jsonstr = boost::regex_replace(jsonstr, boost::regex("\n|\r"), "");
	TirmString(jsonstr);
	return jsonstr;
}

// 通过公司编码获取平台代理信息
bool LotteryServer::getCfgByCode(string platformcode,third_game_cfg & agent_info, int32_t & req_code,string & errmsg)
{
	auto it = agent_info_.find(platformcode);
	if (it != agent_info_.end())
	{
	   	agent_info = it->second;
		LOG_WARN << __FUNCTION__ <<"---游戏对应平台配置信息---platformcode["<< platformcode <<"],platformName[" << agent_info.platformName <<"]";
	   	return true;
	}

	errmsg = "platform code err." + platformcode;
	req_code = (int32_t)eErrorCode::PlatframCfgErr;
	return false;
}
 
//刷新所有agent_info信息 ///
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
bool LotteryServer::refreshAgentInfo()
{   
	LOG_WARN << "---加载代理信息---"<<__FUNCTION__;
	agent_info_.clear();
	company_balance_.clear();
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
				string configjson;
				if(doc["configjson"] && doc["configjson"].type() != bsoncxx::type::k_null)
					configjson = doc["configjson"].get_utf8().value.to_string();
				if (configjson.empty())
				{
					LOG_ERROR << "---" << __FUNCTION__ << " 没配置好在字段[" << configjson << "]";
					continue;
				}

				int32_t gametype = 0;
				if(doc["gametype"])  gametype = doc["gametype"].get_int32();

				// 每个种类的配置
				third_game_cfg agent_info;
				
				agent_info.gametype = gametype;
				agent_info.status = status;
				agent_info.configjson = configjson;
				agent_info.platformName = doc["platformname"].get_utf8().value.to_string();
				agent_info.platformCode = doc["platformcode"].get_utf8().value.to_string();

				// 取结果
				boost::property_tree::ptree req_pt;
				std::stringstream ss(configjson);
				read_json(ss, req_pt);

				// std::cout << "aid:" << agent_info.aid << std::endl;
				std::cout << "gametype:" << agent_info.gametype << std::endl;
				std::cout << "platformName:" << agent_info.platformName << std::endl;
				std::cout << "platformCode:" << agent_info.platformCode << std::endl;
				std::cout << "agent_explanation:" << explanation << std::endl;
				
				company_balance_t company_balance;
				string key = agent_info.platformCode + to_string(agent_info.gametype);
				company_balance.companyCode = agent_info.platformCode;
				company_balance.gametype = agent_info.gametype;
				company_balance.balance = 0;
				company_balance_[key] = company_balance;

				std::cout << "key:" << key << std::endl;

				if(agent_info.platformCode.compare(COMPANY_CODE_WG) == 0)
				{
					agent_info.ApiUrl = req_pt.get<string>("apiurl");
					agent_info.DesCode = req_pt.get<string>("deskey");
					agent_info.MD5Code = req_pt.get<string>("md5key");
					agent_info.agent = req_pt.get<string>("agent");
					agent_info_[agent_info.platformCode] = agent_info;
					continue;
				}
				else if((agent_info.platformCode.compare(COMPANY_CODE_IG) == 0) || (agent_info.platformCode.compare(COMPANY_CODE_PJ) == 0))
				{
					agent_info.gfc_login = req_pt.get<string>("gfc_login","");
					agent_info.lotto_login = req_pt.get<string>("lotto_login","");
					agent_info.lottery_login = req_pt.get<string>("lottery_login","");
					agent_info.gfc_money = req_pt.get<string>("gfc_money","");
					agent_info.hashcode = req_pt.get<string>("hashcode","");
					agent_info.lotteryTray = req_pt.get<string>("lotteryTray","A");
					agent_info.lottoTray = req_pt.get<string>("lottoTray","A");
					agent_info.gfcTray = req_pt.get<string>("gfcTray","A");
					agent_info.currency = req_pt.get<string>("currency","CNY");
					agent_info.language = req_pt.get<string>("language","CN");
					agent_info.color = req_pt.get<string>("lc","b");
					agent_info.mobileVersion = req_pt.get<string>("mobileVersion","new");
					agent_info.line = req_pt.get<int32_t>("line",1);
					agent_info_[agent_info.platformCode] = agent_info;
					continue;
				}
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
	std::cout << "======================================服务启动完成 [" << ((inicfg_.isDemo == 1) ? "试玩" : "正式") << "]==============================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;
	std::cout << "=======================================================================================================" << std::endl;

	return true;
} 

//=====config=====
bool LotteryServer::LoadIniConfig()
{
    if(!boost::filesystem::exists("./conf/lottery.conf")){
        LOG_ERROR << "./conf/lottery.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/lottery.conf", pt);

    IniConfig inicfg;
    inicfg.integralvalue 	= pt.get<int>("LotteryServer.Integralvalue", 6);// 送积分数
    inicfg.changeRatevalue 	= pt.get<int>("LotteryServer.ChangeRatevalue", 500);
    inicfg.session_timeout 	= pt.get<int>("LotteryServer.SessionTimeout", 1000);
    inicfg.descode 			= pt.get<std::string>("LotteryServer.Descode","111362EE140F157D");
    inicfg.apiadd 			= pt.get<std::string>("LotteryServer.ApiAdd", "");
    inicfg.isDemo 			= pt.get<int>("LotteryServer.IsDemo", 0);
    inicfg.TestLock 		= pt.get<int>("LotteryServer.TestLock", 0);
    inicfg.isWhiteIp 		= pt.get<int>("LotteryServer.IsWhiteIp", 1);
    inicfg.defaultScore 	= pt.get<int64_t>("LotteryServer.DefaultScore", 10000);
    inicfg.ip_timeout 		= pt.get<int>("LotteryServer.IpTimeout", 200);
    inicfg.paramCount 		= pt.get<int>("LotteryServer.ParamCount", 5);
    inicfg.isdecrypt 		= pt.get<int>("LotteryServer.Isdecrypt", 0);
    inicfg.isLog 			= pt.get<int>("LotteryServer.IsLog", 0);
    inicfg.isLocal 			= pt.get<int>("LotteryServer.IsLocal", 0);
    inicfg.agentId 			= pt.get<int>("LotteryServer.AgentId", 10000);
    inicfg.maxConnCount 	= pt.get<int>("LotteryServer.MaxConnCount", 2000);
    inicfg.maxConnFreqCount = pt.get<int>("LotteryServer.MaxConnFreqCount",500);
    inicfg.maxGameid 		= pt.get<int>("LotteryServer.MaxGameid", 10000);
    inicfg.maxLoginTypeid 	= pt.get<int>("LotteryServer.MaxLoginTypeid", 4);
    inicfg.expireDate 		= pt.get<int>("LotteryServer.ExpireDate", 4*60*60); //4小时过期时间 

    inicfg.AesKey 			= pt.get<std::string>("LotteryServer.AesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.des_key 			= pt.get<std::string>("LotteryServer.Deskey", "GSwydAem");
    inicfg.md5_key 			= pt.get<std::string>("LotteryServer.Md5key", "4q8JXdwT");
    inicfg.reqUrl 			= pt.get<std::string>("LotteryServer.ReqUrl", "https://api.txhbet.net");
    inicfg.resAddUrl 		= pt.get<std::string>("LotteryServer.ResAdd", "doBusiness.do");
    inicfg.enterGameHandle 	= pt.get<std::string>("LotteryServer.EnterGameHandle", "forwardGame.do");
    inicfg.AgentName 		= pt.get<std::string>("LotteryServer.AgentName", "txwl");
    inicfg.Timeout 			= pt.get<int>("LotteryServer.Timeout", 3000);
    inicfg.MethodName 		= pt.get<std::string>("LotteryServer.Method", "lg");
    inicfg.Password 		= pt.get<std::string>("LotteryServer.Password", "96e79218965eb72c92a549dd5a330112");
    inicfg.MoneyType 		= pt.get<std::string>("LotteryServer.MoneyType", "CNY");
    inicfg.interval 		= pt.get<int>("LotteryServer.interval", 10);		//两次查询间隔
    inicfg.IsMaster 		= pt.get<int>("LotteryServer.IsMaster", 0);		//两次查询间隔
    inicfg.RecycleCycle 	= pt.get<int>("LotteryServer.RecycleCycle", 60);	//两次查询间隔
    inicfg.Actype 			= pt.get<int>("LotteryServer.Actype", 1);			//真钱帐号
    inicfg.ReadConfCycle 	= pt.get<int>("LotteryServer.ReadConfCycle", 10);	//重加载配置时间(秒)
    inicfg.IsPrintConfLog 	= pt.get<int>("LotteryServer.IsPrintConfLog", 1);	//重加载配置时间(秒)
    inicfg.AutoRecycle 		= pt.get<int>("LotteryServer.AutoRecycle", 1);		//自动回收

    inicfg.botUrl 			= pt.get<std::string>("LotteryServer.botUrl", "https://api.telegram.org/bot");
    inicfg.botToken 		= pt.get<std::string>("LotteryServer.botToken", "1288332211:AAEpsGpE6XFNZTzNF4Ca2emhLLlvFMM7OSw");
    inicfg.botChairId 		= pt.get<int64_t>("LotteryServer.botChairId", -437735957);
    inicfg.IsSendTgMsg 		= pt.get<int64_t>("LotteryServer.IsSendTgMsg", 0);
    inicfg.NotOpen 			= pt.get<int64_t>("LotteryServer.NotOpen", 0);
    inicfg.DemoPlayUrl 		= pt.get<std::string>("LotteryServer.DemoPlayUrl", "http://tryplay.txh-demo.com/#/p/preload");//试玩环境地址
    inicfg.oddtype 			= pt.get<std::string>("LotteryServer.oddtype", "A");
    inicfg.LimitTestScore 	= pt.get<int>("LotteryServer.LimitTestScore", 100);

	
	inicfg.WhiteUID.clear();
	vector<string> uidVec;
	string temp = pt.get<std::string>("LotteryServer.WhiteUID", "");
	if (!temp.empty()){
		boost::algorithm::split(uidVec, temp, boost::is_any_of(","));
		if(uidVec.size() > 0){
			int idxCount = 0;
			for(string uid : uidVec)
    		{
				inicfg.WhiteUID.push_back(atol(uid.c_str()));
				// std::cout << "---WhiteUID[" << uid << "] --- " << ++idxCount<< std::endl;
			}
		}
	}

	inicfg_ = inicfg;
	if (inicfg.IsPrintConfLog == 1)
	{
		std::cout << "---resAddUrl " << inicfg.resAddUrl << " AesKey:" << inicfg.AesKey << " md5_key " << inicfg.md5_key <<" AutoRecycle " << inicfg.AutoRecycle << std::endl;
		std::cout << "---IsMaster " << inicfg.IsMaster << " AgentName:" << inicfg.AgentName << " " << inicfg.enterGameHandle << " isdecrypt " << inicfg.resAddUrl<< std::endl;
		std::cout << "---固定回收周期[" << inicfg_.RecycleCycle << "],重加载配置时间[" << inicfg_.ReadConfCycle <<"]"<< std::endl;
		std::cout << "---DemoPlayUrl[" << inicfg.DemoPlayUrl <<"],TestLock[" << inicfg.TestLock  << "],WhiteUID[" << inicfg_.WhiteUID.size() << "]"<< std::endl;
	}
	
	return true;
}

 // 判断该代理是否允许开放
// 返回true为允许开放
bool LotteryServer::isPermitTransfer(int32_t _agentId)
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
void LotteryServer::createOrder(string gametype,string companyCode,string & billNo,string & lgname,string type,int64_t credit,int64_t userid,string da_account,int64_t oldmoney,int64_t newmoney)
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
											   << "gametype" << gametype
											   << "companyCode" << companyCode
											   << "orderid" << billNo
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
void LotteryServer::updateOrder(int64_t &newScore, int64_t userid, string type, int64_t credit, string &billNo, int32_t stauts, int32_t status_code, double elapsedtime, int64_t oldmoney, int64_t newmoney)
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
												// << "$set" << open_document
												// << "txhstatus" << bsoncxx::types::b_int32{1 - nScoreChangeType} //1 跳转到天下汇，0返回大唐
												// << close_document
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
				int32_t agentid 	= view["agentid"].get_int32();
				// 写帐变记录
				string linecode = view["linecode"].get_utf8().value.to_string();
				string account = view["account"].get_utf8().value.to_string();
				int64_t beforeScore = newScore - opScore;

				// std::string::size_type spos = billNo.find_first_of(COMPANY_CODE_IG);
				// if (spos == std::string::npos)
				// LOG_WARN << " >>> , char ip "<< ip;
				string reason = billNo.substr(0, 2);
				AddScoreChangeRecordToDB(userid, account, opScore, agentid, linecode, beforeScore, eUserScoreChangeType::op_third_lottery,reason);
				LOG_INFO << "---写帐变记录userid["<< userid << "] type[" << type << "],newScore[" << newScore <<"],beforeScore[" << beforeScore <<"],credit["<< credit <<"]reason[" << reason <<"]";
			}
		}

	
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",billNo[" << billNo << "]";
	}
}

// 跳转记录，登录成功后则更新状态
void LotteryServer::updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t gameid,int32_t online)
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
											   << "stauts" << gameid			//状态，0已经返回大唐，1没有返回大唐 ,其它值则为对应游戏
											   << "stauts_online" << online			//状态，0已经返回大唐，1没有返回大唐
											   << "newmoney" << _newbalance	//跳转后天下汇余额
											   << "starttime" << (int64_t)time(nullptr)//跳转时间
											   << "starttimeISODate" << bsoncxx::types::b_date{chrono::system_clock::now()} //跳转时间
											   << close_document << finalize;
		// 没有记录内插入
		playing_coll.update_one(findValue.view(), updateValue.view(),options.upsert(true));

		//查询game_user表更新 进入游戏ID
		updatePalyingStatus(userid,gameid);
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",userid[" << userid << "],lgname["<< lgname << "],score["<< credit << "]";
	}
}

void LotteryServer::updatePalyingStatus(int64_t userid,int32_t gameid)
{
	LOG_INFO <<"---"<< __FUNCTION__ <<"---更新游戏状态---gameid[" << gameid << "],userid[" << userid << "]";
	try
	{
		//查询game_user表更新 进入游戏ID
		auto update_doc = document{} << "$set" << open_document << "txhstatus" << gameid << close_document << finalize;
		MONGODBCLIENT["gamemain"]["game_user"].update_one(document{} << "userid" << userid << finalize,update_doc.view());
	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",userid[" << userid << "]";
	}
}
  

bool LotteryServer::cacheThirdPartGame()
{
    try
    {
        MutexLockGuard lock(m_thirdpart_game_Lock);
        m_GameIdListMap.clear();
		m_CompanyNoCodeMap.clear();

        mongocxx::collection third_part_game_coll = MONGODBCLIENT["gameconfig"]["third_part_game_config"];
        mongocxx::cursor info = third_part_game_coll.find({});
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
            int32_t gametype = doc["gametype"].get_int32();
            if (status == eDefaultStatus::op_ON )//&& gametype == eThirdPartGameType::op_lottery
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

				// 11 + 102 + 2 + 001 = 111022001
				int32_t lotterytype = (gameItem.gameid / 1000) % 10;
				if(lotterytype == eLotteryType::op_IG_GFC)
					gameItem.lotterytype = LOTTERY_GFC; //传统官方彩
				else if(lotterytype == eLotteryType::op_IG_SSC)
					gameItem.lotterytype = LOTTERY_LOTTERY; //彩
				else if(lotterytype == eLotteryType::op_IG_HKC)
					gameItem.lotterytype = LOTTERY_LOTTO; //彩
				
				// 第三方公司的真实ID
				gameItem.gameInfoId = gameItem.gameid % 1000;
                
				// 分ID进行装游戏项目
				m_GameIdListMap[gameItem.gameid] = gameItem;
				// 公司编号与编码映射
				m_CompanyNoCodeMap[gameItem.companyNo] = gameItem.companyCode;
                LOG_INFO << "---CompanyNoCodeMap[" << gameItem.companyNo << "],companyCode[" << gameItem.companyCode << "]";
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
 
// 获取查询连接参数
// @bOnlineCheck是否获取在线状态，true,获取
string LotteryServer::balanceInquiryParamsWG(string & lgname,bool bOnlineCheck,third_game_cfg agent_info)
{
	boost::property_tree::ptree root;
	root.put("cagent", agent_info.agent);
	root.put("loginname", lgname);
	root.put("password", "666888");///inicfg_.Password
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
bool LotteryServer::getBalanceWG(ReqParamCfg reqParam,third_game_cfg agent_info, int64_t & balance,int32_t & req_code,string & errmsg,bool & status_online)
{
	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << reqParam.lgname << "]";
	
	std::string jsonstr = balanceInquiryParamsWG(reqParam.lgname,true,agent_info);
	boost::property_tree::ptree req_pt;

	cpr::Response resp = getUrlRequest(jsonstr, agent_info);
	if (resp.status_code != 200)
	{
		req_code = (int32_t)eErrorCode::StatusCodeNot200Error;
		std::stringstream ssss;
		ssss << "余额查询错误,耗时[" << resp.elapsed << "],状态码[" << resp.status_code << "]";
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
		if(req_code == (int32_t)eErrorCode::NoError)
			status_online = req_pt.get<bool>("data.status"); // 玩家在线状态
	}
	catch (exception excep)
	{
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "余额查询解析错误,耗时[" << resp.elapsed << "],状态码[" << resp.status_code << "],返回内容[" << resp.text << "]";
		return false;
	}

	// 有可能有其它的错误返回
	if(req_code != (int32_t)eErrorCode::NoError ){
		return false;
	}
	// 
	double fbalance = req_pt.get<double>("data.balance"); //req_pt.get_child("data"); 
	string balanceStr = to_string(fbalance);
	balance = GlobalFunc::rate100(balanceStr);

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],data.balance[" << fbalance << "],余额[" << balance << "],耗时[" << resp.elapsed << "],状态码[" << resp.status_code << "]";

	return true;
}

 
bool LotteryServer::AddScoreChangeRecordToDB(int64_t userId,string &account,int64_t addScore,int32_t agentId,string linecode,int64_t beforeScore,int32_t roomId,string reason)
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
            << "changescorereason" << reason
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
// 加载Json文件
bool LotteryServer::loadJson()
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
		
bool LotteryServer::PartGame()
{
    try
    {
        MutexLockGuard lock(m_thirdpart_game_Lock);
        m_GameTypeListMap.clear();
        //map<string,map<int32_t,vector<GameItemInfo>>> m_GameTypeListMap

        mongocxx::collection third_part_game_coll = MONGODBCLIENT["gameconfig"]["third_part_game_config"];
        mongocxx::cursor info = third_part_game_coll.find({});
        for (auto &doc : info)
        {
            int32_t status = doc["status"].get_int32();
			int32_t gametype = doc["gametype"].get_int32();
			// 只取电子类
            if (status == eDefaultStatus::op_ON && gametype == eThirdPartGameType::op_lottery)
            {
                // 添加
                GameItemInfo gameItem;
                gameItem.repairstatus = doc["repairstatus"].get_int32();

				string gamelistjson = doc["details"].get_utf8().value.to_string();
        		LOG_DEBUG <<" ============>>> gamelistjson[" << gamelistjson<<"]";
				if (!gamelistjson.empty())
				{
					boost::property_tree::ptree root_pt, list_pt;
					std::stringstream sstream(gamelistjson);
					boost::property_tree::json_parser::read_json(sstream, root_pt);

					int32_t gametype2 = root_pt.get<int32_t>("gametype");
					string companyCode = root_pt.get<string>("companyCode");
					string name = root_pt.get<string>("name");

					LOG_INFO << "---gametype[" << gametype << "],gametype2[" << gametype2 << "],companyCode[" << companyCode << "],name[" << name << "]";

					list_pt = root_pt.get_child("list");
					for (boost::property_tree::ptree::iterator it = list_pt.begin(); it != list_pt.end(); ++it)
					{
						//  {"code": "gfc","tid": 1,"id": 1,"name": "广东快乐十分","st":0},
						boost::property_tree::ptree sconde_pt = it->second;
						//遍历读出数据
						string key = it->first;
						string code = sconde_pt.get<string>("code");
						string gamename = sconde_pt.get<string>("name");
						int32_t gameid = sconde_pt.get<int32_t>("id");
						int32_t tid = sconde_pt.get<int32_t>("tid");
						int32_t st = sconde_pt.get<int32_t>("st");
						LOG_INFO << "key:" << key << " code:" << code << " name2:" << name2 << " tid:" << tid;

						GameItemInfo gameItem;
						gameItem.companyCode = companyCode;
						gameItem.companyName = name;
						gameItem.gameid = gameid;
						gameItem.repairstatus = st;
						gameItem.tid = tid;
						gameItem.gamename = gamename;
					}
				}

				// 分公司进行装游戏项目
                // m_GameTypeListMap[gametype].push_back(gameItem);

                // LOG_INFO << "---gameid[" << gameItem.gameid << "],companyCode[" << gameItem.companyCode << "],GameItemInfo[" << gameItem.companyNo << "],size[" << m_GameTypeListMap.size() << "],companyName[" << gameItem.companyName << "],companyCode[" << gameItem.companyCode << "]";
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

*/
