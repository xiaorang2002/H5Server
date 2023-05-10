/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/


// 仅用于上下分服拉取天下汇分数


/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/


#ifndef TXH_API_H
#define TXH_API_H 

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
// #include "RedisClient/RedisClient.h"
// #include "RedisClient/redlock.h"

#include "./public/ErrorCodeSet.h"
#include "./public/cryptopublic.h"
#include "./public/urlcodec.h"

// 大唐帐转进入天下汇
#define TRANS_IN  	"IN"
// 天下汇帐转进入大唐
#define TRANS_OUT  	"OUT"

using namespace std;
using namespace muduo;

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


struct InitConfig
{
	int32_t agentId;
	int32_t isLocal;//0，随机帐号；1，固定帐号 单身汪们，咋就没声音了呢，机会来了，别怂了，摆脱
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
	string DemoPlayUrl;
};



class CTxhAPI
{
   public:
    CTxhAPI(muduo::net::EventLoop * loop);  // 这是构造函数

	void startInit();

	// 获取维护状态
	bool getMaintenanceStatus(int32_t & req_code,string & errmsg);
	// 查询余额状态,type 0 统一回收;1 指定帐号回收
	bool recycleBalance(int type,string account,int64_t & recycleScore);
    
private:
	// 初始化Redis
	bool InitRedisCluster(std::string ipaddr, std::string password);
	// 初始化数据库
	bool InitMongoDB(std::string url);
	// 加载配置
	bool LoadInitConfig();
	// 获取分数
	void processGetScore(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp);
    
	// 检测并创建游戏帐号
 	bool getCreateAccountParams(string & lgname,int32_t actype, int32_t aid,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 查询订单状态
	bool orderInquiryParams(string & billNo,int32_t actype,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 获取参数
	string balanceInquiryParams(string & lgname,bool bOnlineCheck = false);
	// 查询余额
	bool getBalance(string & lgname, int64_t & balance,int32_t & req_code,string & errmsg,cpr::Response & resp);
	// 游戏转账（上下分）参数
	string createTransferParams(string lgname, string billNo, string type,string creditStr);
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
	bool checkUrlValid(string reqHandle,int32_t & req_code,string & errmsg);
	// 创建跳转成功记录
	void updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t type,int32_t online = 1);

	// 是否允许访问
	bool isPermitTransfer(int32_t _agentId);
	// 加载Json文件
	bool loadJson();
	// 签名算法
	cpr::Parameters createSignatureParam(string parameContent);
	// 构造返回结果 ///
	string createJsonString(int32_t opType,int32_t errcode,string const& dataStr);
	string createJsonStringEx(int32_t opType,int32_t errcode,int64_t uid,string & lgname,int32_t elapsedtimd, string & msgstr,string & urlstr);

	//按照占位符来替换 ///
	inline void replace(string& json, const string& placeholder, const string& value) {
		boost::replace_all<string>(json, "\"" + placeholder + "\"", value);
	}
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

	//刷新所有agent_info信息 1.web后台更新代理通知刷新 2.游戏启动刷新一次
	bool refreshAgentInfo();

   private:
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

	//指定网卡ipaddr
	std::string strIpAddr_;
	//监听HTTP请求
	// muduo::net::TcpServer httpServer_;
	//业务线程池，内部自实现任务消息队列
    // muduo::ThreadPool threadPool_;

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

	bool isStartServer;   
	bool isdecrypt_;   
	bool m_bInvaildServer;   
	
    InitConfig initcfg_;

	std::map<int32_t, string> errcode_info_;

	muduo::net::EventLoop * threadTimer_;
};
 
// 成员函数定义，包括构造函数
CTxhAPI::CTxhAPI(muduo::net::EventLoop * loop)
{
	threadTimer_ = loop;
    cout << "CTxhAPI Object is being create" << endl;
}

void CTxhAPI::startInit()
{
	// 先加载一次配置
	LoadInitConfig();

	if(!threadTimer_) {
    	cout << "startInit threadTimer_ is nullptr" << endl;
		return;
	}

	// 加载代理信息
	threadTimer_->runAfter(0.5f, bind(&CTxhAPI::refreshAgentInfo, this));
	// 周期加载配置
	threadTimer_->runEvery(1.0f, [&]() {
		//
		// if (initcfg_.IsMaster == 0)
		// 	return;

		// 固定回收周期
		if (++nSecondTick_ < initcfg_.RecycleCycle)
		{
			// 定时加载conf配置文件
			if (nSecondTick_ > 0 && (nSecondTick_ % initcfg_.ReadConfCycle == 0))
			{
				LoadInitConfig();
			}
			return;
		}
		nSecondTick_ = 0;
		// 收到更新信息
		//if( inicfg_.AutoRecycle == 1 )
		//	recycleBalance(eRecycleType::all,"");
	});
	// 加载Json
	threadTimer_->runAfter(0.1f, bind(&CTxhAPI::loadJson, this));

	LOG_WARN << "--- IsSendTgMsg[" << initcfg_.IsSendTgMsg << "]";
	// 固定周期回收时才发送
	if(initcfg_.IsSendTgMsg == 1 ){ //&& type == (int32_t)eRecycleType::all && bIsRecycle
		string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
		GlobalFunc::sendMessage2Tg(strtmp + "\nCTxhAPI start roll", initcfg_.botToken, initcfg_.botChairId);
	}

	LOG_WARN << "--- CTxhAPI start is being load";
}


// 构造返回结果
std::string CTxhAPI::createJsonString(int32_t opType,int32_t errcode,string const& dataStr)
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
 
// 检测并创建游戏帐号
// @actype 账号类型：0-试玩账号 1-真钱账号
bool CTxhAPI::getCreateAccountParams(string & lgname, int32_t actype, int32_t aid,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_WARN << "---"<<__FUNCTION__<<",lgname[" << lgname << "],帐号类型[" << (actype == 0 ? "试玩帐号":"真人帐号") << "],aid[" << aid << "]";

	boost::property_tree::ptree root,req_pt;
	root.put("cagent", agent_info_.MerchantNum);
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
string CTxhAPI::balanceInquiryParams(string & lgname,bool bOnlineCheck)
{
	boost::property_tree::ptree root;
	root.put("cagent", agent_info_.MerchantNum);
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
bool CTxhAPI::getBalance(string & lgname, int64_t & balance,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",lgname[" << lgname << "]";
	
	std::string jsonstr = balanceInquiryParams(lgname);
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
	if (!getJsonPtree(resp.text, req_code,errmsg,req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
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

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],data.balance[" << fbalance << "],balance[" << balance << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";

	return true;
}
 // 查询订单状态
bool CTxhAPI::orderInquiryParams(string & billNo,int32_t actype,int32_t & req_code,string & errmsg,cpr::Response & resp)
{
	LOG_INFO << "---"<<__FUNCTION__<<",billNo[" << billNo << "]";

	boost::property_tree::ptree root,req_pt;
	root.put("cagent", agent_info_.MerchantNum);		//代理账号
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
string CTxhAPI::createTransferParams(string lgname, string billNo, string type,string creditStr)
{
	LOG_INFO << "---" << __FUNCTION__ << ",lgname[" << lgname << "]";
	// 返回数据
	boost::property_tree::ptree root;
	root.put("cagent", agent_info_.MerchantNum); //代理账号
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
bool CTxhAPI::transferBalance(string &lgname, string billNo, string type, int64_t userid, int64_t credit,
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
		updateOrder(userid,type,credit,billNo,(int32_t)eOrderType::OpTiemOut,resp.status_code,resp.elapsed); 
		return false;
	}

	// 4,取结果码
	if (!getJsonPtree(resp.text,req_code,errmsg, req_pt)){
		req_code = (int32_t)eErrorCode::GetJsonInfoError;
		LOG_ERROR << "transfer Request getJsonPtree Error elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "]";
		updateOrder(userid,type,credit,billNo,(int32_t)eOrderType::OpJsonErr,resp.status_code,resp.elapsed);
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
		updateOrder(userid,type,credit,billNo,(int32_t)eOrderType::OpFinish,resp.status_code,resp.elapsed,oldbalance,newbalance);
		
		LOG_INFO << "---转帐单成功---userid[" << userid << "],txhaccount[" << lgname << "],回收余额["
								 << creditStr << "],elapsed[" << resp.elapsed << "],对方帐户转前[" << oldbalance << "],转后[" << newbalance << "]";
	}
	else{
		updateOrder(userid,type,credit,billNo,(int32_t)eOrderType::OpUndefind,resp.status_code,resp.elapsed);
	}

	LOG_WARN << "---req_code[" << req_code << "],errmsg[" << errmsg << "],elapsed[" << resp.elapsed << "],status_code[" << resp.status_code << "],billNo["<< billNo <<"]";
	return true;
}

// 统一请求参数之获取
cpr::Response CTxhAPI::getUrlRequest(string & parameContent,string & reqHandle)
{
	// 签名算法
	std::string strDesOut;
	CryptoPublic::DES_ECB_Encrypt((unsigned char *)agent_info_.DesCode.c_str(), (unsigned char *)"", parameContent, strDesOut);
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
	param.AddParameter({"key", strMd5});

	string _requrl = agent_info_.ApiUrl;
	if (!reqHandle.empty())
	{
		 _requrl = _requrl + "/" + reqHandle;
	}

	LOG_INFO << "---req string ---[" << _requrl << "?" <<param.content << "]";  

	auto resp_feature = cpr::Get(cpr::Url{_requrl},cpr::Timeout{initcfg_.Timeout}, cpr::Header{{"AgentName", agent_info_.MerchantNum}}, param);

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
bool CTxhAPI::getJsonPtree(string &jsonStr, int32_t &code, string &msg,boost::property_tree::ptree & pt)
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
  

// 根据玩家Id获取用户信息
bool CTxhAPI::getUserNameById(int64_t uid, int32_t & aid, string &lgname,int64_t & score,string & da_account,int32_t & req_code,string & errmsg)
{
	// 测试使用，不带参数
	if( initcfg_.isdecrypt == 0 ) return true; 

	try
	{
		// 加锁等待1秒
		//setNxDelay(uid);
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
bool CTxhAPI::checkUrlValid( string reqHandle ,int32_t & req_code,string & errmsg)
{
	string _requrl = agent_info_.ApiUrl;
	if (!reqHandle.empty())
	{
		 _requrl = _requrl + "/" + reqHandle;
	}

	LOG_INFO << "---req string ---[" << _requrl << "]";  

	auto resp = cpr::Head(cpr::Url{_requrl}, cpr::Header{{"AgentName", agent_info_.MerchantNum}});
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
bool CTxhAPI::getMaintenanceStatus(int32_t & req_code,string & errmsg)
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

//处理HTTP回调
// void CTxhAPI::processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
void CTxhAPI::processGetScore(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp)
{
	//本次请求开始时间戳(微秒)
	muduo::Timestamp timestart = muduo::Timestamp::now();
	int32_t elapsedtimd = 0;

	std::string rspdata,req_forwardUrl,resp_msg,lg_name = "aaa123";
	try
	{
		// 参数
		std::string billNo,backurl,req_msg = "ok";
		int32_t aid = 0,actype = 0,req_code = (int32_t)eErrorCode::NoError; 
		int64_t balance = 0,userid = 0,score = 0,newbalance = 0;
		std::string da_account = "",lgType = "y", oddtypeStr = "A",reqStr = req.query();
		cpr::Response resp;

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
			// req_code = checkReqParams(reqStr,userid,actype,lgType,oddtypeStr,backurl,req_msg);
			// if( req_code != (int32_t)eErrorCode::NoError )
			// 	break;

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

			// (3)查询game_user表
			// if(!getUserNameById(userid,aid,lg_name,score,da_account,req_code,req_msg) || (req_code != 0))
			// 	break;

			// (4)检查代理权限 
			if(!isPermitTransfer(aid)){
				req_code = (int32_t)eErrorCode::PermissionDenied;
				req_msg = "agent permission denied!";
				break;
			}

			// (5)检查玩家是否天下汇而来，是则直接退出
			// if(agent_info_.aid == aid){ 
			// 	req_code = (int32_t)eErrorCode::UserCanNotBeEnterGame;
			// 	break;
			// }

			// 1,检测并创建游戏帐号----------------------------------------------------------------------------------------
			if(!getCreateAccountParams(lg_name,actype,aid,req_code,req_msg,resp) || (req_code != 0))
				break;

			// 2，余额查询-------------------------------------------------------------------------------------------------
			if( score == 0 ){
				// 余额不足
				if(!getBalance(lg_name,balance,req_code,req_msg,resp) || (req_code != 0) || (balance == 0)){
					if( (req_code == 0) && (balance == 0)){
						req_code = (int32_t)eErrorCode::UserInsufficientBalance;
						req_msg = "insufficient balance";
					}
					break;
				}
			}

			// 先创建订单
			billNo = GlobalFunc::getBillNo(userid);
			
			// 3，游戏转账（上下分）-----------------------------------------------------------------------------------------
			if(!transferBalance(lg_name,billNo,TRANS_IN,userid,score,newbalance,req_code,req_msg,da_account,resp))
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
			updatePalyingRecord(lg_name,da_account,userid,score,newbalance,1);

			// 6，进入游戏---------------------------------------------------------------------------------------------------
			// if(!enterGameParams(lg_name,lgType,oddtypeStr,req_forwardUrl,req_code,req_msg,resp) || (req_code != 0))
				// break;

			LOG_DEBUG << "DesCode:" << agent_info_.DesCode <<" AgentId:" << agent_info_.aid;
		
			// 7，拼接返回地址
			// 7.1 加密 backurl，转码
			// std::string _gamebackurl_Des;
			// std::string _gamebackurl_decode = URL::MultipleDecode(backurl);

			// LOG_DEBUG << "_gamebackurl_decode:" << _gamebackurl_decode;

			// CryptoPublic::DES_ECB_Encrypt((unsigned char *)agent_info_.DesCode.c_str(), (unsigned char *)"", _gamebackurl_decode, _gamebackurl_Des);
			// std::string _gamebackurl = URL::Encode(_gamebackurl_Des);

			// LOG_DEBUG << "_gamebackurl_Des:" << _gamebackurl_Des;
			// 7.2 拼接返回地址
			// req_forwardUrl = req_forwardUrl + "&burl=" + _gamebackurl;

			std::string _gamebackurl = URL::MultipleDecode(backurl);
			//  过滤gameid=3字符
			std::string::size_type spos = _gamebackurl.find_last_of("&");
			if (spos != std::string::npos)
			{
				string gameidstr = _gamebackurl.substr(spos);
				std::cout << "===find_last_of pos[" << spos << "],gameidstr[" << gameidstr << "]" << std::endl;
				// 把gameid=3替换成gameid=0
				if (gameidstr.compare("&gameid=3") == 0)
				{
					_gamebackurl.replace(spos + 8, 1, "0");
					std::cout << "==========发生了替换========" << std::endl;
					std::cout << _gamebackurl << std::endl;
					std::cout << std::endl;
				}
			}

			backurl = URL::Encode(_gamebackurl);
			req_forwardUrl = req_forwardUrl + "&burl=" + backurl;
			
			// LOG_DEBUG << "_gamebackurl[" << _gamebackurl <<"]\n req_forwardUrl[" << req_forwardUrl<<"]";
			LOG_DEBUG << "req_forwardUrl[" << req_forwardUrl<<"]";

			// 加密forwardUrl返回
			std::string tmpEncode,forwardUrl = req_forwardUrl;
			CryptoPublic::AES_ECB_Encrypt(initcfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
			req_forwardUrl = URL::Encode(tmpEncode);
		
		} while (0);

		// 兼容试玩环境
		if (initcfg_.isDemo == 1 && req_code == (int32_t)eErrorCode::PermissionDenied)
		{
			req_code = (int32_t)eErrorCode::NoError;
			// 加密forwardUrl返回
			std::string tmpEncode,forwardUrl = initcfg_.DemoPlayUrl;
			CryptoPublic::AES_ECB_Encrypt(initcfg_.AesKey, (unsigned char *)forwardUrl.c_str(), tmpEncode);
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
			if(initcfg_.IsSendTgMsg == 1 )GlobalFunc::sendMessage2Tg(getTgMsg(userid,lg_name,billNo,req_code,req_msg,elapsedtimd,strtmp),initcfg_.botToken,initcfg_.botChairId);
		}
		
		// 返回结果
		//rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,req_code,userid,lg_name,elapsedtimd,req_msg,req_forwardUrl);

		// Log rspdata
		if( initcfg_.isLog < (int32_t)eLogLvl::LVL_3 )
			LOG_DEBUG << "---TransHandle----isdecrypt[" << initcfg_.isdecrypt << "],[" << rspdata << "]";
	}
	catch (exception excep)
	{
		resp_msg = "exception" ;
		LOG_ERROR << "---" << __FUNCTION__ << " " << excep.what() <<",req.query[" << req.query() <<"]";
		//rspdata = createJsonStringEx((int32_t)eReqType::OpTransfer,(int32_t)eErrorCode::InsideErrorOrNonExcutive,0,lg_name,0,resp_msg,req_forwardUrl);
	}
	
	// 统计毫秒
	if (initcfg_.isLog < (int32_t)eLogLvl::LVL_3)
	{
		LOG_WARN << "---处理耗时(毫秒)[" << elapsedtimd << "]";
	}
}
 
//刷新所有agent_info信息 ///
//1.web后台更新代理通知刷新
//2.游戏启动刷新一次
bool CTxhAPI::refreshAgentInfo()
{   
	LOG_WARN << "---加载代理信息---"<<__FUNCTION__;
	// agent_info_ = {0};
	agent_ids.clear();
	try
	{
		mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["livevideo_config"];
		mongocxx::cursor cursor = coll.find({});
		for (auto &doc : cursor)
		{
			// LOG_INFO << "---QueryResult:" << bsoncxx::to_json(doc);
			agent_info_.aid = 0;
			agent_info_.MD5Code = doc["MD5Code"].get_utf8().value.to_string();
			agent_info_.DesCode = doc["DesCode"].get_utf8().value.to_string();
			agent_info_.ApiUrl = doc["ApiUrl"].get_utf8().value.to_string();
			agent_info_.PlatformName = doc["PlatformName"].get_utf8().value.to_string();
			agent_info_.PlatformCode = doc["PlatformCode"].get_utf8().value.to_string();
			agent_info_.MerchantNum = doc["MerchantNum"].get_utf8().value.to_string();
			if(doc["AgentId"])
			{
				if(doc["AgentId"].type() ==  bsoncxx::type::k_int32)
					agent_info_.aid = doc["AgentId"].get_int32();
			}

			// 加载代理白名单
			auto t_agent_ids = doc["Shield"].get_array();
			for (auto agent_id : t_agent_ids.value)
			{
				agent_ids.push_back(agent_id.get_int32());
			}

			std::cout << "agent_ids_size:" << agent_ids.size() << std::endl;
			std::cout << "aid:" << agent_info_.aid << std::endl;
			std::cout << "MD5Code:" << agent_info_.MD5Code << std::endl;
			std::cout << "DesCode:" << agent_info_.DesCode << std::endl;
			std::cout << "ApiUrl:" << agent_info_.ApiUrl << std::endl;
			std::cout << "PlatformName:" << agent_info_.PlatformName << std::endl;
			std::cout << "PlatformCode:" << agent_info_.PlatformCode << std::endl;
			std::cout << "MerchantNum:" << agent_info_.MerchantNum << std::endl;
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
bool CTxhAPI::LoadInitConfig()
{
    if(!boost::filesystem::exists("./conf/txh_api.conf")){
        LOG_ERROR << "./conf/txh_api.conf not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/txh_api.conf", pt);

    InitConfig inicfg;
    inicfg.integralvalue 	= pt.get<int>("CTxhAPI.Integralvalue", 6);// 送积分数
    inicfg.changeRatevalue 	= pt.get<int>("CTxhAPI.ChangeRatevalue", 500);
    inicfg.session_timeout 	= pt.get<int>("CTxhAPI.SessionTimeout", 1000);
    inicfg.descode 			= pt.get<std::string>("CTxhAPI.Descode","111362EE140F157D");
    inicfg.apiadd 			= pt.get<std::string>("CTxhAPI.ApiAdd", "");
    inicfg.isDemo 			= pt.get<int>("CTxhAPI.IsDemo", 0);
    inicfg.isWhiteIp 		= pt.get<int>("CTxhAPI.IsWhiteIp", 1);
    inicfg.defaultScore 	= pt.get<int64_t>("CTxhAPI.DefaultScore", 10000);
    inicfg.ip_timeout 		= pt.get<int>("CTxhAPI.IpTimeout", 200);
    inicfg.paramCount 		= pt.get<int>("CTxhAPI.ParamCount", 5);
    inicfg.isdecrypt 		= pt.get<int>("CTxhAPI.Isdecrypt", 0);
    inicfg.isLog 			= pt.get<int>("CTxhAPI.IsLog", 0);
    inicfg.isLocal 			= pt.get<int>("CTxhAPI.IsLocal", 0);
    inicfg.agentId 			= pt.get<int>("CTxhAPI.AgentId", 10000);
    inicfg.maxConnCount 	= pt.get<int>("CTxhAPI.MaxConnCount", 2000);
    inicfg.maxConnFreqCount = pt.get<int>("CTxhAPI.MaxConnFreqCount",500);
    inicfg.maxGameid 		= pt.get<int>("CTxhAPI.MaxGameid", 10000);
    inicfg.maxLoginTypeid 	= pt.get<int>("CTxhAPI.MaxLoginTypeid", 4);
    inicfg.expireDate 		= pt.get<int>("CTxhAPI.ExpireDate", 4*60*60); //4小时过期时间 

    inicfg.AesKey 			= pt.get<std::string>("CTxhAPI.AesKey", "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw");
    inicfg.des_key 			= pt.get<std::string>("CTxhAPI.Deskey", "GSwydAem");
    inicfg.md5_key 			= pt.get<std::string>("CTxhAPI.Md5key", "4q8JXdwT");
    inicfg.reqUrl 			= pt.get<std::string>("CTxhAPI.ReqUrl", "https://api.txhbet.net");
    inicfg.resAddUrl 		= pt.get<std::string>("CTxhAPI.ResAdd", "doBusiness.do");
    inicfg.enterGameHandle 	= pt.get<std::string>("CTxhAPI.EnterGameHandle", "forwardGame.do");
    inicfg.AgentName 		= pt.get<std::string>("CTxhAPI.AgentName", "txwl");
    inicfg.Timeout 			= pt.get<int>("CTxhAPI.Timeout", 3000);
    inicfg.MethodName 		= pt.get<std::string>("CTxhAPI.Method", "lg");
    inicfg.Password 		= pt.get<std::string>("CTxhAPI.Password", "666888");
    inicfg.MoneyType 		= pt.get<std::string>("CTxhAPI.MoneyType", "CNY");
    inicfg.interval 		= pt.get<int>("CTxhAPI.interval", 10);		//两次查询间隔
    inicfg.IsMaster 		= pt.get<int>("CTxhAPI.IsMaster", 0);		//两次查询间隔
    inicfg.RecycleCycle 	= pt.get<int>("CTxhAPI.RecycleCycle", 60);	//两次查询间隔
    inicfg.Actype 			= pt.get<int>("CTxhAPI.Actype", 1);			//真钱帐号
    inicfg.ReadConfCycle 	= pt.get<int>("CTxhAPI.ReadConfCycle", 10);	//重加载配置时间(秒)
    inicfg.IsPrintConfLog 	= pt.get<int>("CTxhAPI.IsPrintConfLog", 1);	//重加载配置时间(秒)
    inicfg.AutoRecycle 		= pt.get<int>("CTxhAPI.AutoRecycle", 1);		//自动回收

    inicfg.botUrl 			= pt.get<std::string>("CTxhAPI.botUrl", "https://api.telegram.org/bot");
    inicfg.botToken 		= pt.get<std::string>("CTxhAPI.botToken", "1288332211:AAEpsGpE6XFNZTzNF4Ca2emhLLlvFMM7OSw");
    inicfg.botChairId 		= pt.get<int64_t>("CTxhAPI.botChairId", -437735957);
    inicfg.IsSendTgMsg 		= pt.get<int64_t>("CTxhAPI.IsSendTgMsg", 0);
    inicfg.NotOpen 			= pt.get<int64_t>("CTxhAPI.NotOpen", 0);
    inicfg.DemoPlayUrl 		= pt.get<std::string>("CTxhAPI.DemoPlayUrl", "http://tryplay.txh-demo.com/#/p/preload");//试玩环境地址

	initcfg_ = inicfg;
	if (inicfg.IsPrintConfLog == 1)
	{
		LOG_WARN << "---resAddUrl " << inicfg.resAddUrl << " AesKey:" << inicfg.AesKey << " md5_key " << inicfg.md5_key <<" AutoRecycle " << inicfg.AutoRecycle;
		LOG_WARN << "---IsMaster " << inicfg.IsMaster << " AgentName:" << inicfg.AgentName << " " << inicfg.enterGameHandle << " isdecrypt " << inicfg.resAddUrl;
		LOG_INFO << "---固定回收周期[" << initcfg_.RecycleCycle << "],重加载配置时间[" << initcfg_.ReadConfCycle <<"]";
		LOG_WARN << "---DemoPlayUrl[" << inicfg.DemoPlayUrl <<"]";
	}
	
	return true;
}

 // 判断该代理是否允许开放
// 返回true为允许开放
bool CTxhAPI::isPermitTransfer(int32_t _agentId)
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
void CTxhAPI::createOrder(string & billNo,string & lgname,string type,int64_t credit,int64_t userid,string da_account,int64_t oldmoney,int64_t newmoney)
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
void CTxhAPI::updateOrder(int64_t userid,string type,int64_t credit,string & billNo,int32_t stauts,int32_t status_code,double elapsedtime,int64_t oldmoney,int64_t newmoney)
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

			if(!user_coll.update_one(findValue.view(), updateValue.view())){
				LOG_ERROR <<"---更新玩家分失败---";
				LOG_INFO << "---update document:"<< bsoncxx::to_json(findValue);
				LOG_INFO << "---update document:"<< bsoncxx::to_json(updateValue);
			}
			else{
				LOG_INFO << "---更新玩家登录状态 userid["<< userid << "] game_user txhstatus["<< 1 - nScoreChangeType << "],type[" << type << "]";
			}
		}

	}
	catch (const std::exception &e)
	{
		LOG_ERROR << "---" << __FUNCTION__ << " exception:" << e.what() << ",billNo[" << billNo << "]";
	}
}

// 跳转记录，登录成功后则更新状态
void CTxhAPI::updatePalyingRecord(string & lgname,string & account,int64_t userid,int64_t credit,int64_t _newbalance,int32_t type,int32_t online)
{
	LOG_INFO <<"---"<< __FUNCTION__ <<"---更新跳转记录状态---lgname[" << lgname << "],account[" << account << "],userid[" << userid << "],type[" << type << "],online[" << online << "]";
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
											   << "stauts" << type			//状态，0已经返回大唐，1没有返回大唐
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
cpr::Parameters CTxhAPI::createSignatureParam(string parameContent)
{
	LOG_INFO << "---"<<__FUNCTION__<<",parameContent[" << parameContent << "]";

	std::string strDesOut;
	CryptoPublic::DES_ECB_Encrypt((unsigned char *)agent_info_.DesCode.c_str(), (unsigned char *)"", parameContent, strDesOut);

	LOG_INFO << "---"<<__FUNCTION__<<",DesCode[" << agent_info_.DesCode << "],MD5Code["<<agent_info_.MD5Code<<"],strDesOut[" << strDesOut << "]";

	// 2,统一请求参数之key
	std::string strMd5, src = strDesOut + agent_info_.MD5Code;
	char md5[32 + 1] = {0};
	MD5Encode32(src.c_str(), src.length(), md5, 0);
	strMd5 = string((char *)md5);

	// 3,拼接参数
	auto param = cpr::Parameters{};
	param.AddParameter({"params", URL::Encode(strDesOut)});
	param.AddParameter({"key", strMd5});

	return param;
}

// 回收余额，仅用于上下分服拉取天下汇分数
// @useridStr用户ID
bool CTxhAPI::recycleBalance(int type ,string useridStr,int64_t & recycleScore)
{	
	// 写加锁
	// WRITE_LOCK(playing_list_mutex_);
	
    muduo::Timestamp timestart = muduo::Timestamp::now();
	playing_list_.clear();

	//查询txh playing info表
	mongocxx::options::find opts = mongocxx::options::find{};
	opts.projection(bsoncxx::builder::stream::document{} << "userid" << 1
														 << "stauts" << 1
														 << "score" << 1
														 << "account" << 1
														 << "starttime" << 1
														 << "txhaccount" << 1 << bsoncxx::builder::stream::finalize);
	// 统一查询
	bsoncxx::document::value query_value  = document{} << "stauts" << open_document << "$gte" << 1 << close_document << finalize;
	// 回收指定帐号
	if(type == (int32_t)eRecycleType::one && (!useridStr.empty())){
		// 
        query_value = document{} << "userid" << atol(useridStr.c_str()) << finalize;
		LOG_INFO << "---回收指定帐号,useridStr[" << useridStr << "]";
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
			
			if(doc["txhaccount"])
				playing_ls_.txhaccount 	= doc["txhaccount"].get_utf8().value.to_string();

			if(doc["account"])
				playing_ls_.account = doc["account"].get_utf8().value.to_string();

			playing_list_.push_back(playing_ls_);
			LOG_INFO << "用户ID[" << playing_ls_.userid << "],Account[" << playing_ls_.account << "],帐户[" << playing_ls_.txhaccount << "],状态[" << playing_ls_.stauts << "]";
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
			LOG_INFO << "---没有要回收的玩家,type[" << type << "],userid[" << useridStr << "]";
		return false;
	}

    LOG_INFO << "---加载玩家耗时(秒)[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) << "],用户数量[" << playing_list_.size() << "],type[" << type <<"]";

	LOG_INFO << "=======================";
	LOG_INFO << "=======回收余额========";
	LOG_INFO << "=======================";

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
					// 玩家在线状态
					item.status_online = req_pt.get<bool>("data.status");
				}
				catch (exception excep)
				{
					req_code = (int32_t)eErrorCode::GetJsonInfoError;
					LOG_ERROR << "balance Request getJsonPtree Error elapsed[" << response.elapsed << "],status_code[" << response.status_code << "],response.text[" << response.text << "]";
					
					string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
					if(initcfg_.IsSendTgMsg == 1 )
						GlobalFunc::sendMessage2Tg(strtmp + "\n" + to_string(response.status_code) + "\n" + response.text, initcfg_.botToken, initcfg_.botChairId);
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
					// 创建订单号
					string billNo = GlobalFunc::getBillNo(item.userid);
					// 拉取余额
					cpr::Response resp_;
					if (transferBalance(item.txhaccount, billNo, TRANS_OUT, item.userid, balance, newbalance, req_code, errmsg, item.account, resp_))
					{
						// 更新跳转记录
						updatePalyingRecord(item.txhaccount,item.account,item.userid, balance, newbalance, 0, 0);

						// 返回读取结果
						if( type == (int32_t)eRecycleType::one )
							recycleScore = balance;
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
							if(initcfg_.IsSendTgMsg == 1 )
								GlobalFunc::sendMessage2Tg(strtmp + "\n" + sstmp.str(), initcfg_.botToken, initcfg_.botChairId);
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
				if(initcfg_.IsSendTgMsg == 1 )
					GlobalFunc::sendMessage2Tg(strtmp + "\n" + ss.str(), initcfg_.botToken, initcfg_.botChairId);
			}

			// 返回状态
			return response.status_code;
		};

		//发送查询请求
		auto future =  cpr::GetCallback(balanceCB,cpr::Url{_requrl},cpr::Header{{"AgentName", agent_info_.MerchantNum}}, param);
		if(future.get() != 200){
			LOG_INFO << "---回收查询请求失败---userid[" <<  item.userid << "],txhaccount[" << item.txhaccount << "],status_code[" << future.get() << "]";
		}
	}

	// 拉取余额耗时统计
	int32_t icount = 0,allCount = playing_list_.size();
	auto elaspTime =  muduo::timeDifference(muduo::Timestamp::now(), timestart) ;
	stringstream ss;
	ss << "API回收数量[" << allCount << "],成功数[" << recycleOK << "],耗时[" << elaspTime << "]\n";
	for (auto item : playing_list_)
	{
		ss << "uid[" << item.userid << "],lgname[" <<  item.txhaccount << "],score[" << item.recycleScore << "]";
		if(++icount < allCount)
			ss << std::endl;
	}
	
	// 固定周期回收时才发送
	if(initcfg_.IsSendTgMsg == 1 ){ //&& type == (int32_t)eRecycleType::all && bIsRecycle
		string strtmp = initcfg_.isDemo == 0 ? "线上" : "试玩";
		GlobalFunc::sendMessage2Tg(strtmp + "\n" + ss.str(), initcfg_.botToken, initcfg_.botChairId);
	}

	LOG_INFO << "---拉取余额耗时(秒)[" << elaspTime << "]," << ss.str();
	LOG_INFO << "=======================";
	LOG_INFO << "=======回收结束========";
	LOG_INFO << "=======================";

	// 执行大于0则成功，否则没有回收
	return  (recycleOK > 0);
}

// 加载Json文件
bool CTxhAPI::loadJson()
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


#endif // TXH_API_H
