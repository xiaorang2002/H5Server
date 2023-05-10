#ifndef HALLSERVER_H
#define HALLSERVER_H

#include <map>
#include <list>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <vector>
#include <string>
#include <functional> // std::greater

#include <sys/types.h>
#include <sys/socket.h>
 #include <sys/ioctl.h> 
#include <sys/timeb.h>

#include <stdlib.h> 
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sstream>
#include <time.h>
#include <fstream>
#include <ios>
#include <chrono>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
 
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <muduo/base/Logging.h>
#include "muduo/base/TimeZone.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "public/RedisClient/RedisClient.h"
#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/ThreadLocalSingletonRedisClient.h"

#include "public/gameDefine.h"
#include "public/Globals.h"
#include "public/GlobalFunc.h" 
#include "public/TaskService.h"
#include "public/StdRandom.h"
//#include "ConnectionPool.h"
//#include "public/CommonDB.h"
#include "IPFinder.h"
#include "LuckyGame.h"

// #include "TraceMsg/FormatCmdId.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

//extern int g_nConfigId;
// bool cmpValueEx(const pair<string, int32_t> &a,const pair<string, int32_t> &b)
// {
//     return  a.second < b.second;
// } 

#define ONE_DAY_SECOND  86400

 
namespace Landy
{

const int LOOP_THREAD_COUNT = 4;
const int WORK_THREAD_COUNT = 10;
const int SCORE_ORDER_THREAD_COUNT = 4;

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
 


class HallServer : public boost::noncopyable
{
public:
    typedef function<void(muduo::net::TcpConnectionWeakPtr &, uint8_t *, int, internal_prev_header *)> AccessCommandFunctor;
    typedef shared_ptr<muduo::net::Buffer> BufferPtr;

public:
    HallServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, int idleSeconds, string networkcarname);
    ~HallServer();

    void setThreadNum(int numThreads);
    bool loadKey();
    bool loadRSAPriKey();
    string decryDBKey(string password, int db_password_real_len);

    bool InitZookeeper(string ip);
    bool InitRedisCluster(string ip, string password);
    bool InitMongoDB(string url);
    bool startThreadPool(int threadCount);
    bool InitRocketMQ(string &ip);
    bool startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize);
    // uint32_t RocketMQBroadcastCallback(const vector<MQMessageExt> &mqe);
    // uint32_t ConsumerClusterScoreOrderCallback(const vector<MQMessageExt> &mqe);
    //    uint32_t ConsumerClusterScoreOrderSubCallback(const vector<MQMessageExt> &mqe);
    void stopConsumerClusterScoreOrder();
    void start(int threadCount);

    void OnRepairGameServerNode();
    void ZookeeperConnectedHandler();
    void GetProxyServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                               const string &path, void *context);
    void GetGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                              const string &path, void *context);
    void GetInvaildGameServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                     const string &path, void *context);

    void GetMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                               const string &path, void *context);
    void GetInvaildMatchServersChildrenWatcherHandler(int type, int state, const shared_ptr<ZookeeperClient> &zkClientPtr,
                                                      const string &path, void *context);



private:
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buf,
                   muduo::Timestamp receiveTime);

    void processRequest(muduo::net::TcpConnectionWeakPtr &conn,
                        BufferPtr &buf,
                        muduo::Timestamp receiveTime);
    void ProcessScoreOrderAdd(string &tag,string &key, string body);
    int32_t AddOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId);
    void ProcessScoreOrderSub(string &tag,string &key, string body);
    int32_t SubOrderScore(int64_t userId, string &account, uint32_t agentId, int64_t score, string &orderId);

private:
    void init();
    void threadInit();

    void UpdateConfig(string msg);
    void RefreshGameInfo();
    void RefreshMatchInfo();
    void RefreshMatchSwitchInfo();
    void InitTasks();
    void RefreshRoomPlayerNum();
    void updateRoomPlayerNum();
    bool sendData(TcpConnectionWeakPtr &weakConn, vector<unsigned char> &data, internal_prev_header *internal_header, Header *commandHeader);
    void cmd_keep_alive_ping(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

    //login
    string get_random_account();
    void cmd_login_servers(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    bool GetUserLoginInfoFromRedis(const string &session, int64_t &userId, string &account, uint32_t &agentId);

    // 删除玩家登录信息
    void deleteUserLoginInfo(int64_t userid);
    void updatePalyingRecord(int64_t userid);
    //    bool DeleteUserLoginInfoFromRedis(const string &session);
    //    bool ResetExpiredUserLoginInfoFromRedis(const string &session);
    bool Login3SCheck(const string &session);
    bool DBAddUserLoginLog(int64_t userId, string &strIp, string &address, uint32_t status, uint32_t agentId);
    bool DBUpdateUserLoginInfo(int64_t userId, string &strIp, int64_t days);
    //    bool DBWriteRecordDailyUserOnline(shared_ptr<RedisClient> &redisClient, int nOSType, int nChannelId, int nPromoterId);

    void getUserResponeLogginServer();
    void cmd_get_game_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_match_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_playing_game_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_game_server_message(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_romm_player_nums_info(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    string getRandomGameServer(uint32_t gameId, uint32_t roomId,int32_t agentId);

    string getMoneyTypeGameServer(uint32_t gameId, uint32_t roomId,int32_t currency);

    string getRandomMatchServer(uint32_t gameId, uint32_t roomId);

    void cmd_on_user_offline(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_repair_hallserver(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    bool DBUpdateUserOnlineStatusInfo(int64_t userId, int32_t status);
    bool DBAddUserLogoutLog(int64_t userId, chrono::system_clock::time_point &startTime);

    void cmd_set_headId(TcpConnectionWeakPtr &weakConn, uint8_t *data, int msgdataLen, internal_prev_header *internal_header);
    void cmd_set_nickName(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_get_user_score(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_play_record(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_play_record_detail(TcpConnectionWeakPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header);
    void cmd_get_play_record_detail_match(WeakTcpConnectionPtr& weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header* internal_header);
    void cmd_get_match_record(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_match_best_record(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    // 轮盘逻辑
    void cmd_switch_luckyGame(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_luckyGame_logic(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    //Task
    void cmd_get_task_list(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_task_award(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);
    //
    void cmd_safe_box(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_extrange_order(TcpConnectionWeakPtr &weakConn, uint8_t *msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_band_bank(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_rebate(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_set_min_rebate(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_banklist(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_activeItem_list(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_signin_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_sign_in(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_customer_service(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_set_card_default(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);


    void cmd_get_mails(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_bulletin(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_set_bulletinmails_status(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_get_recharge_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_platform_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_get_vip_rewardcolumn(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_receive_vip_reward(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    void cmd_get_reg_reward(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_recharge_order(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_recharge_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_third_part_game_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_dividend_level(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_dividend(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);
    void cmd_get_route_record(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);


    //获取红包雨信息
    void cmd_get_red_envelope_info(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    //抢红包
    void cmd_get_grab_envelope(TcpConnectionWeakPtr &weakConn, uint8_t* msgdata, int msgdataLen, internal_prev_header *internal_header);

    void ChangeGameOnLineStatusOffline(int64_t userId);
    void reLoginHallServer(TcpConnectionWeakPtr &weakConn, internal_prev_header *internal_header);
    void WriteDailyBindMobileRecord(int64_t userId);

    int32_t getRewardConfig(int32_t agentid,int32_t &regReward,int32_t &reqCount,int32_t &reqReward,int32_t &minCoin,int32_t &maxCoin);
    // 
	// 刷新一次维护代理的ID
	void refreshRepairAgentInfo();
	// 检查维护代理的ID
	bool checkAgentStatus(int32_t agentid);
	void test();
    bool readWithoutNameJsonArrayString(std::string strWithNameJson, std::string & strWithoutNameJson);
    bool cacheMemberBonusInfo(int32_t level,int32_t agentid,int32_t mapIdx, string & channelbonusJson);
    void cacheChannleWY(int32_t agentid,int32_t level, vector<string> & bankcardidVec);
    void cacheChannleThirdParty(int32_t agentid,int32_t level, int32_t paytype,vector<string> & paychannelVec);
    void cacheChannleOnline(int32_t agentid,int32_t level, int32_t paytype, vector<string> & onlineidVec,string paymentchannel);
    bool cacheThirdPartGame(int32_t agentid);

    void createEmailDefault(int32_t agentid,int64_t userid);

    void markGetFreeCoin(int64_t uid);

    bool get_platform_global_config(int32_t agent_id,int32_t type_id,string & details);
    int32_t cacheSignInInfo(int32_t agentid);

    std::map<int32_t,string> m_PicthPathMap;
    int32_t cachePicPathIP(int32_t agentid,string & picPath);
    // 检查银行卡的合法性
    bool checkCard(string card);

    inline string makeRedisKey(int64_t uid,eRedisKey keyId)
    {
        return REDIS_KEY_ID + to_string((int32_t)keyId) + "_" + to_string(uid);
    };

    //判断是否开始红包雨
    int WhetherStartRedEnvelopeRain(int32_t &status,int32_t agentid,int64_t &sendscore,int64_t userId,int64_t &needbet,int32_t &lefttimes);
	
    bool getGameServerIp(int32_t agentid,string & gameserverIp);
	bool getGameServerIp_agentidVec(int64_t userid);
    bool getGameServer_MoneyType(int32_t moneytype,string & gameserverIp);

    bool cacheActiveItem(int32_t agentid);
    bool cacheRegPoorItem(int32_t agentid);
    int32_t getExchangeInfo(int32_t agentid, int32_t &bank_min, int32_t &bank_bonus, int32_t &zfb_min, int32_t &zfb_bonus,
                            int32_t &exchangefreetime, int32_t &exchangefee, int32_t &forceFee, int32_t &aliforceFee);

    //只是简单返回是不是红包雨状态
    int GetRedEnvelopeRainStatus(int64_t &lefttime,int32_t agentid,int64_t &allscores,int64_t &nestopentime ,int64_t &nestclosetime,int64_t &activeopentime,int64_t &activeclosetime,int64_t userid);
private:
    void OnLoadParameter(); 
    void OnSendNoticeMessage();
    bool IsMasterHallServer();
    void SendNoticeMessage();
    void OnInit();

public:
    void quit();

protected:
    bool CanSubMoney(int64_t userId);

private:
    tagGameReplay m_replay; //game replay
    tagConfig m_lkGame_config;
    // 获取牌局ID
    string GetNewRoundId(int64_t userId, int roomId)
    {
        string strRoundId = to_string(roomId) + "-";
        int64_t seconds_since_epoch = chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        strRoundId += to_string(seconds_since_epoch) + "-";
        strRoundId += to_string(::getpid()) + "-";
        strRoundId += to_string(userId) + "-";
        strRoundId += to_string(rand() % 10);
        return strRoundId;
    }
    //判断是否幸运转盘
    bool IsLkGame(int32_t gameId){return m_lkGame_config.nGameId == gameId;}
    void SaveReplay(tagGameReplay &replay);
    bool UpdateUserScoreToDB(int64_t userId, int64_t nBetScore, int64_t addScore);
    bool AddUserGameLogToDB(int64_t userId, string &account, string &strRoundId, int64_t addScore, int32_t agentId, int32_t gameId, int32_t roomId);
    bool AddScoreChangeRecordToDB(int64_t userId, string &account, int64_t addScore, int32_t agentId,string linecode, int64_t beforeScore, int32_t roomId);
    bool JiFenChangeRecordToDB(int64_t userId,string account,int32_t agentId,int64_t incrintegral,int32_t before_integral,eJiFenChangeType changetype,string reason);

    bool AddUserGameInfoToDB(int64_t userId, string &account, string &strRoundId, string &cardValue, int32_t currency,int64_t beforeScore, int64_t nBetScore, int64_t lWinScore, int32_t agentId, int32_t gameId, int32_t roomId,string linecode);
    void SaveLKBoardCastMsg(string msgStr, int32_t currency = 0); 
    int  GetGameType(string msgStr); //判断游戏类型[百人类；1:对战类；2:其他]


    void WriteGameInfoToMysql(string roundid,int64_t userid,string account,
                              int32_t agentid,string linecode,int32_t gameid
                             ,int32_t roomid,string cardvalue
                             ,int64_t beforescore,int64_t betscore,int64_t winscore);
    void WriteScoreRecordToMysql(int64_t userid,string account,
                              int32_t agentid,string linecode
                             ,int32_t roomid
                             ,int64_t addscore,int64_t beforeScore);

    void WriteIntegralToMysql(int64_t userid,string account,
                              int32_t agentid,int32_t changetype
                             ,int64_t incrintegral,int64_t before_integral
                             ,string  reason);

    void WriteGameLogToMysql(int64_t userid,string gamelogid,
                             int32_t agentid,string account,int32_t gameid,int32_t roomid
                            ,int64_t addscore,int64_t revenue);

    void WriteLogOutLogToMysql(int64_t userid,int64_t logintime,
                               int64_t logouttime,int32_t agentid,int64_t playseconds);

    void WriteEvelopeToMysql(int64_t userid,int32_t agentid,
                             int64_t hbScore,string dbaccount);

    void WriteVipToMysql(int64_t userid,int32_t agentid,
                         int64_t dbwelfarescore,int64_t dbrewardType ,int64_t dbaddscore ,int32_t dbnewlevel,
                         int64_t dbvalidbet ,string dbaccount );
private:
    const static size_t kHeaderLen = sizeof(int16_t);

private:
    ::HallServer::GetGameMessageResponse m_response;
    ::HallServer::GetMatchMessageResponse m_MatchResp;
    ::HallServer::GetServerPlayerNumResponse m_roomPlayerNums;
    mutable boost::shared_mutex m_game_info_mutex;
    mutable boost::shared_mutex m_match_info_mutex;
    mutable boost::shared_mutex m_room_player_num_mutex;

private:
    //map[matchid]=list<agentId>
    mutable boost::shared_mutex m_shieldAgentIds_mutex;
    std::map<int32_t, std::vector<uint32_t>> m_shieldAgentIds;

    //桌子ID，房间ID和IP的映射
    std::map<int32_t,string> m_tableIpMap; //<roomid|tableid , IP>


    std::map<int32_t,tagProxyGamesInfo> m_proxyInfoMap;
    std::map<int32_t,::HallServer::GetGameMessageResponse> m_proxyInfoSendMap;

    std::map<int32_t,::HallServer::GetGameMessageResponse> m_crrentInfoSendMap;
    std::map<int32_t,int32_t> m_proxyUserMap;
    // 银行绑定信息
    std::map<int32_t,vector<map<string,string>>> m_bankNameMap;

     // 代理等级分红信息
    struct dividendItem
    {
        int32_t level;			 // 等级
        int32_t share;			 // 份额
        string title;		     // 标题名称.
        string name;		     // 会员等级名称.
    };
    std::map<int32_t,vector<dividendItem>> m_dividendMap;
   // 代理等级分红配置信息
    struct dividendCfg
    {
        int32_t rate;			 // 比例
        int32_t limit;			 // 最低要求
    };
    // 代理,配置
    std::map<int32_t,dividendCfg> m_dividendCfgMap;

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

   // 每日签到信息（缓存代理签到信息）
    struct DaliySignItem
    {
        int32_t sortid;         // 排序ID. 值越小越靠上
        int32_t giftType;       // 赠送类型
        int32_t giftValue;      // 赠送价值
        int32_t minBet;         // 要求最小流水值
        string  title;          // 标题
    };
    std::map<int32_t,std::vector<DaliySignItem>> m_SignInItemMap;

    // 客服问题信息
    struct FQAItem
    {
        int32_t sortid;		 // 排序ID. 值越小越靠上
        string question;	 // 问题标题名称.
        string answer;	     // 问题解答
    };
    std::map<int32_t,std::vector<FQAItem>> m_FQAItemMap;

    // 客服配置信息
    struct KeFuCfgItem
    {
        int32_t type;       // 客服类型(0,客服反馈 1,代理咨询 2,举报有奖)
        int32_t isjump;     // 是否外部跳转(0,内嵌 1,外跳)
        int32_t weight;     // 跳转权重
        int32_t viewtype;   // 是否在网页上显示
        string titlename;   // 客服连接名字
        string href;        // 跳转连接
    };
    std::map<int32_t,std::vector<KeFuCfgItem>> m_KeFuCfgItemMap;


    //图片配置信息
    struct ImgCfgItem
    {
        int32_t type;       // 类型
        string href;        // 连接
    };
    std::map<int32_t,std::vector<ImgCfgItem>> m_ImgCfgItemMap;
 
    // <公司编号,游戏信息 > 类型
    typedef map<int32_t,vector<GameItemInfo>> GAME_INFO_MAP;

    // <游戏类型,游戏信息 > //游戏类型,"101":视讯游戏;"102":彩票;"103":电子;"104":体育
    GAME_INFO_MAP m_GameTypeListMap; //每个游戏类型信息

    // 代理号与游戏服对应关系
    std::map<int32_t,vector<string>> m_proxy_gameServer_ip;

    // 币种和游戏服务器相对应关系
    std::map<int32_t,vector<string>> m_moneyType_gameServer_ip;
    //
    vector<string> proxyIPVec;

    vector<string> moneyTypeVec;
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
 

    // platform配置信息
    struct PlatformCfg
    {
        string picPath;   // pic full path
    };
    std::map<int32_t,PlatformCfg> m_PlatformCfgMap;


   // LevelCfg玩家等级配置信息
    struct LevelCfg
    {
        int32_t levelnumber;    // levelnumber  1
        string levelname;       // levelname    "VIP1"
        int64_t upgradevalue;   // upgradevalue 100
        int64_t upgradereward;  // upgradereward  1
    };
    std::map<int32_t,vector<LevelCfg>> m_LevelCfgMap;
    MutexLock level_mutex;

    // 代理对应分红手续费比例(platform_dividend_receive_config)
    // Key : agentid << 8 | level
    struct tollscaleCfg
    {
        double tollscale;       // 手续费率
        double scale;           // 分摊比例
        int32_t level;          // 等级
        string levelname;       // 等级名字
    };
    std::map<int32_t,tollscaleCfg> m_dividend_receive_configMap;
    // 存放每个代理线路的分红手续费配置
    std::map<int32_t,vector<tollscaleCfg>> m_dividend_fee_vectorMap;

    // 
    struct platform_user_dividends
    {
        int64_t userid;         //玩家id
        int64_t selfBetting;    //自营业绩
        int64_t teamBetting;    //团队业绩
        int64_t userProfit;     //本周获得的分红钱数
        int64_t lastAllBetting; //平台总业绩
        int64_t allDividends;   //平台总分红钱数
        int64_t dateTime;       //上周一零分零秒时间戳
        int32_t level;          //等级
        double selfShares;     //自己的分红份额=团队分红份额-下级分红份额
        double teamShares;     //团队分红份额，每w多少份*多少w =分红份额
        double moneyPerShare;  //每份股份的钱数
        string nikeName;        //昵称
    };

    struct ProxyLevelInfo
    {
        ProxyLevelInfo() {}
        int64_t bettingScore; //本级别需要的有效下注
        int32_t upgradeScore; //升上本级别奖励钱数
        int32_t viplevel;        //级别数
    };

    struct RedEnvelopeInfo
    {
        RedEnvelopeInfo()
        {
            beginTime=0;
            endTime=0;
            scoreMin=0;
            scoreMax=0;
            totalScore=0;
            upScoreLimit=0;
            upScoreMul=0;
            needBet=0;
            allTimes=0;
            activityTimeStart=0;
            activityTimeEnd=0;
        }
        int64_t beginTime;
        int64_t endTime;
        int64_t scoreMin;
        int64_t scoreMax;
        int64_t totalScore;
        int64_t upScoreLimit;
        int32_t upScoreMul;
        int64_t needBet;
        int32_t allTimes;
        int64_t activityTimeStart;
        int64_t activityTimeEnd;
    };
    // 签到赠送类型
    enum eGiftType
    {
        integral    = 0,     //积分      
        coin        = 1,     //金币      
    };

    inline int32_t getTidByGameid(int32_t gameid)
    {
        return (gameid/1000)%10;
    }
  

    inline string InitialConversion(int64_t timeSeconds)
    {
        time_t rawtime = (time_t)timeSeconds;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);

        int Year = timeinfo->tm_year + 1900;
        int Mon = timeinfo->tm_mon + 1;
        int Day = timeinfo->tm_mday;
        int Hour = timeinfo->tm_hour;
        int Minuts = timeinfo->tm_min;
        int Seconds = timeinfo->tm_sec;
        string strYear;
        string strMon;
        string strDay;
        string strHour;
        string strMinuts;
        string strSeconds;
        
        if (Seconds < 10)
            strSeconds = "0" + to_string(Seconds);
        else
            strSeconds = to_string(Seconds);
        if (Minuts < 10)
            strMinuts = "0" + to_string(Minuts);
        else
            strMinuts = to_string(Minuts);
        if (Hour < 10)
            strHour = "0" + to_string(Hour);
        else
            strHour = to_string(Hour);
        if (Day < 10)
            strDay = "0" + to_string(Day);
        else
            strDay = to_string(Day);
        if (Mon < 10)
            strMon = "0" + to_string(Mon);
        else
            strMon = to_string(Mon);

        return to_string(Year) + "-" + strMon + "-" + strDay + " " + strHour + ":" + strMinuts + ":" + strSeconds;
    }
    inline string InitialConversionNoDate(int64_t timeSeconds)
    {
        time_t rawtime = (time_t)timeSeconds;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);

        int Year = timeinfo->tm_year + 1900;
        int Mon = timeinfo->tm_mon + 1;
        int Day = timeinfo->tm_mday;
        int Hour = timeinfo->tm_hour;
        int Minuts = timeinfo->tm_min;
        int Seconds = timeinfo->tm_sec;
        string shi;
        string minuts;
        if(Hour<10)
        {
            shi="0"+to_string(Hour);
        }
        else
        {
            shi=to_string(Hour);
        }
        if(Minuts<10)
        {
            minuts="0"+to_string(Minuts);
        }
        else
        {
            minuts=to_string(Minuts);
        }
        return shi + ":" + minuts;
    }

    inline string InitialConversionNoTime(int64_t timeSeconds)
    {
        time_t rawtime = (time_t)timeSeconds;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);

        int Year = timeinfo->tm_year + 1900;
        int Mon = timeinfo->tm_mon + 1;
        int Day = timeinfo->tm_mday;
        int Hour = timeinfo->tm_hour;
        int Minuts = timeinfo->tm_min;
        int Seconds = timeinfo->tm_sec;

        return to_string(Year) + "-" + to_string(Mon) + "-" + to_string(Day);
    }
    // 当前日期零点时间戳
    inline time_t getZeroTimeStamp(tm *tp)
    {
        tm tpNew;
        tpNew.tm_year = tp->tm_year;
        tpNew.tm_mon = tp->tm_mon;
        tpNew.tm_mday = tp->tm_mday;
        tpNew.tm_hour = 0;
        tpNew.tm_min = 0;
        tpNew.tm_sec = 0;
        return mktime(&tpNew);
    }
    void CalculatePlayerLevel(int64_t histrybet, int32_t lastLevel, int64_t &score, int32_t &viplevel, vector<int64_t> betlev, map<int32_t, int64_t> levscore,int64_t &upgradeamount);
    bool getLevelInfo(int64_t totalvalidbet,int32_t aid,int32_t & viplevel,int32_t & levelrate);
    int32_t get_user_dividend_status(int32_t agentid,int64_t userId,platform_user_dividends & user_dividends);
    void pickDividendfee(int64_t uid,int32_t agentid,int64_t _userProfit,int32_t & pickCount,int64_t & selfProfit,int32_t feeItemCount,stringstream & ss);
private:
    map<int32_t, vector<ProxyLevelInfo>> m_ProxyLevelStandard;    //代理的vip等级表
    map<int32_t, vector<RedEnvelopeInfo>> m_ProxyRedStandard;      //代理的红包配置
    map<int32_t, vector<int64_t>> m_ProxyStartTime;              //代理开始发红包时间点容器，可以排序
    map<int32_t, vector<int64_t>> m_ProxyEndTime;              //代理开始发红包时间点容器，可以排序

    shared_ptr<ZookeeperClient> m_zookeeperclient;
    string m_szNodePath, m_szNodeValue, m_szInvalidNodePath, m_szUserOrderScorePath;
    string m_netCardName;

    shared_ptr<RedisClient> m_redisPubSubClient;
    string m_redisIPPort;
    string m_redisPassword;

    string m_mongoDBServerAddr;

    //    RocketMQ m_ConsumerBroadcast;
    // RocketMQ m_ConsumerClusterScoreOrder;
    //    RocketMQ m_ConsumerClusterScoreOrderSub;
    // RocketMQ m_Producer;

    CIpFinder m_pIpFinder;

    shared_ptr<LuckyGame> m_LuckyGame;
    shared_ptr<TaskService> m_TaskService;
    // 存放玩家分数
    map<int64_t, UserScoreInfo> m_UserScore_map;
    MutexLock m_mutex;
    MutexLock m_mutex_save;
 
    MutexLock m_t_mutex;

    map<int64_t, int64_t> m_get_task_tick;
    // 
    typedef map<int32_t, vector<tagUserTaskInfo>>  TASK_TYPE;
    map<int64_t, TASK_TYPE> m_user_tasks;
    MutexLock m_user_task_mutex;

    MutexLock m_mutex_ids;
    vector<int32_t> agent_ids;      //维护名单
private:
    static map<int, AccessCommandFunctor> m_functor_map;
    list<string> m_game_servers;
    MutexLock m_game_servers_mutex;

    vector<string> m_invaild_game_server;

    map<uint32_t, vector<string>> m_room_servers;

    vector<string> m_invaild_match_server;
    list<string> m_match_servers;
    MutexLock m_match_servers_mutex;

    list<string> m_proxy_servers;
    MutexLock m_proxy_servers_mutex;

    MutexLock m_send_encode_data_mutex;

private:
    string m_pri_key;
    MutexLock m_lock;

private:
    MutexLock m_ChangeScoreMutexLock;

 private:
    map<int64_t, int32_t> m_userId_agentId_map;
	map<int32_t, int32_t> m_gameserverIP_AgentIdVec;	//分游戏服IP，代理获取对应服的代理ID,
	map<int64_t, int32_t> m_userId_agentId_map_getroute; //请求百人游戏实时路单时用的
    MutexLock m_userId_agentId_MutexLock;
    MutexLock m_thirdpart_game_Lock;
    MutexLock m_proxy_game_ip;
    MutexLock m_currnecy_game_ip;
    MutexLock m_dividend_MutexLock;

public:
    muduo::net::TcpServer m_server;
    muduo::ThreadPool m_thread_pool;
    AtomicInt32 m_connection_num;

    shared_ptr<EventLoopThread> m_timer_thread;
    shared_ptr<EventLoopThread> m_game_sqldb_thread;
    bool m_bInvaildServer;
	map<int32_t, int32_t> m_gameType_map;
	map<int32_t, int32_t> m_gameRoomTableCount_map;
    //    muduo::ThreadPool m_score_order_thread_pool;
    STD::Random m_random;

    static bool  SortFuc(RedEnvelopeInfo x ,RedEnvelopeInfo y)
    {
        return x.beginTime<y.beginTime;
    }
	map<int64_t, int32_t> m_userId_current_map;	//玩家ID对应的币种
	map<int32_t, int64_t> m_lastTickmap;
	map<int32_t, vector<string>> m_msgListsmap;
};

} // namespace Landy

#endif // HALLSERVER_H
