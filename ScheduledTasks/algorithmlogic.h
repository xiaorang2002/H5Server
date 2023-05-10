#ifndef ALGORITHMLOGIC_H
#define ALGORITHMLOGIC_H
#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Logging.h"

#include <ctime>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <boost/date_time.hpp>
#include <bsoncxx/validate.hpp>
#include <mongocxx/database.hpp>
#include "boost/date_time/parse_format_base.hpp"


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Logging.h>

#include "ConsoleClr.h"
#include <time.h>
#include <vector>
#include <map>



#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include "ThreadLocalSingletonMongoDBClient.h"
using namespace std::chrono;
#define MONGODBCLIENT ThreadLocalSingletonMongoDBClient::instance()

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using namespace std;
using namespace muduo;
using namespace muduo::net;



//一天包含的秒数
#define ONE_DAY_HAS_SECONDS   86400

//一天包含的毫秒数
#define ONE_DAY_HAS_MILLISECONDS   86400000


struct NewUserAccumulate
{
    NewUserAccumulate()
    {
        userid = 0;
        directlyAdd = 0;
        teamAdd = 0;
        agentid = 0;
        upUserid = 0;
        uplist.clear();
    }
    int64_t userid;         //玩家id
    int32_t directlyAdd;    //直属新增玩家
    int32_t teamAdd;        //团队新增玩家
    int64_t upUserid;       //直接上级id
    int32_t agentid;        //代理id
    vector<int64_t> uplist;
};


//当天有下注的玩家信息汇总
struct StruserInformation
{
    StruserInformation()
    {
        userid = 0;
        bettingScore = 0;
        winScore = 0;
        revent = 0;
        agentid = 0;
        superiorAgent.clear();
    }
    int64_t userid;
    int64_t bettingScore;
    int64_t winScore;
    int64_t revent;
    int32_t agentid;
    vector<int64_t>   superiorAgent;
 };
//玩家这周产生的分红信息表
struct StrUserWeekInfo
{
    StrUserWeekInfo()
    {
        selfBetting=0;    //自营业绩
        teamBetting=0;    //团队业绩
        agentid=0;        //代理id
        userid=0;         //玩家id
        upUserid=0;       //直接上级id
        selfShares=0.0;     //分红份额，每w多少份*多少w =分红份额
        userProfit=0;     //本周获得的分红钱数
        teamShares=0.0;     //团队分红份额
        belowUser.clear();//直属下级id
        level = 0;
        nikeName = "";
        lastAllBetting = 0;
        allDividends =0;
        moneyPerShare =0;
    }
    int64_t allDividends;   //总分红数额
    int64_t lastAllBetting; //上周的平台总业绩
    int64_t selfBetting;    //自营业绩
    int64_t teamBetting;    //团队业绩
    int32_t agentid;        //代理id
    int64_t userid;         //玩家id
    int64_t upUserid;       //直接上级id
    double teamShares;     //团队分红份额，每w多少份*多少w =分红份额
    double selfShares;     //自己的分红份额=团队分红份额-下级分红份额
    int64_t userProfit;     //本周获得的分红钱数
    double moneyPerShare;  //每份股份的钱数
    string  nikeName;       //昵称
    int32_t level;          //业绩等级
    vector<int64_t> belowUser;//直属下级id

 };
//当天产生佣金的玩家信息，有些有下级，可能没下注也产生佣金
struct StrIncomeUser
{
    StrIncomeUser()
    {
        selfBetting = 0.0;
        teamBetting = 0.0;
        agentid = 0;
        userid = 0;
        guarantee = 0;
        selfProfit = 0;
        teamProfit = 0;
        newUserZhi = 0;
        newUserTuan = 0;
        contribution = 0;
        upUserid = 0;
    }
    int64_t selfBetting;    //自营业绩
    int64_t teamBetting;    //团队业绩
    int32_t agentid;        //代理id
    int64_t userid;         //玩家id
    int64_t upUserid;       //直接上级id
    int32_t guarantee;      //自身保底
    int64_t selfProfit;     //自营利润
    int64_t teamProfit;     //团队利润
    int64_t newUserZhi;     //直属新增玩家
    int64_t newUserTuan;    //团队新增玩家
    int64_t contribution;   //给上级的贡献值
};

//代理信息表
struct StrProxyInfo
{
    StrProxyInfo() {}
    int32_t level;       //本代理下的级别
    int64_t minvalidbet; //本级别下的最低分
    int64_t maxvalidbet; //本级别下的最高分
    int32_t brokerage;   //本级别的利润抽成比列
};
//周分红总配置表
struct WeekInformation
{
    WeekInformation()
    {
        agentid = 0;
        minperformance = 0;
        dividendScale = 30.0;
        AllDividendsProfit =0;
        AllShares =0;
        moneyPerShare =0;
        AllProfit =0;
        AllBeting =0;
    }
    int32_t agentid;        //代理号
    int64_t minperformance; //最低分红条件-30w注单量
    double_t dividendScale; //总盈利的百分比数作为分红-30%
    int64_t AllProfit;      //本周平台总盈利
    int64_t AllBeting;      //平台总业绩
    int64_t AllDividendsProfit; //本周平台用于分红的总盈利
    int64_t AllShares;      //本周平台总的分红份额
    double moneyPerShare;  //平台本周每股值多少钱
};
//周分红条件表
struct WeekDividendCondition
{
    WeekDividendCondition()
    {
        proxyid = 0;
        agentname = "";
        level = 0;
        share = 0;
        minPerformance = 0;
        maxPerformance = 0;
    }
    int64_t proxyid;    //代理id
    string  agentname;  //代理名字
    int32_t level;      //级别
    int32_t share;      //所在级别的每万所占份额
    int64_t minPerformance;//所在级别的最低条件
    int64_t maxPerformance;//所在级别的最高条件
};

//玩家周分红信息
struct userWeekDividendInfo
{
    userWeekDividendInfo()
    {
        userid = 0;
        agentid = 0;
        totalbet =0;
        totalvalidbet =0;
        totalwinlose =0;
        totalrevenue =0;
        superiorAgent.clear();
    }
    int64_t userid;     //玩家id
    int64_t agentid;    //代理id
    int64_t totalbet;
    int64_t totalvalidbet;
    int64_t totalwinlose;
    int64_t totalrevenue;
    vector<int64_t>   superiorAgent;
};
class AlgorithmLogic
{
public:
    AlgorithmLogic();
    ~AlgorithmLogic();


    shared_ptr <EventLoopThread>     timelooper;
    //开始日期和结束日期
    tm     m_StartDate;
    tm     m_EndDate;

    tm     m_InsertDate;
    std::map<int64_t ,StruserInformation>  userMap;       //有下注的玩家列表

    std::map<int64_t ,StrIncomeUser>       incomeUserMap; //有业绩的玩家列表

    std::map<int64_t ,NewUserAccumulate>   NewUserMap;    //新增玩家的集合

    std::map<int64_t ,NewUserAccumulate>   HaveAddNewUserMap;    //产生新增玩家的集合(下属或者直属)

    std::map<int32_t ,vector<StrProxyInfo>>        proxyinfoMap; //代理信息表

    std::vector<int32_t> proxyEffectivevct;     //有效的平台号，区别于原来的代理

    std::map<int32_t ,vector<WeekDividendCondition>> weekCondictionMap;

    std::map<int64_t ,userWeekDividendInfo>  userWeekInfo;       //玩家周信息表

    std::map<int64_t ,StrUserWeekInfo>  userResultInfo;       //转换后 的玩家周信息表

    std::map<int64_t ,WeekInformation>  OverallCondition;       //分红总配置表
    muduo::net::TimerId                         StartProgramTime;

    int64_t                                 StartTimeTnterval;


    int32_t                     jetLag;                         //时差
    int32_t                     isOpenAlltime;                  //开启统计任意时间段的
    int32_t                     m_iyear;                        //需要统计的开始日期  —年
    int32_t                     m_imonth;                       //需要统计的开始日期  —月
    int32_t                     m_iday;                         //需要统计的开始日期  —日


    int32_t                     m_dyear;                        //需要删除的开始日期  —年
    int32_t                     m_dmonth;                       //需要删除的开始日期  —月
    int32_t                     m_dday;                         //需要删除的开始日期  —日

    int32_t                     m_insertDistanceTime;           //要统计的时间段长度，从截止时间往前推N个小时
    int32_t                     m_deleteDistanceTime;           //要删除的时间段注单，从开始时间往后推N个小时

    int32_t                     isOpenDelete;                   //开启删除任意时间段注单

    int32_t                     onlyDelete;                     //只跑删除程序


    string m_mongoDBServerAddr;

    time_t                      startPoint;
    time_t                      endPoint;

    time_t                      weekHighest;                    //时间戳周一零点，从这个时间往前算一周的注单，大于等于一周之前的时间戳且小于这个值
    time_t                      todayHighest;
public:

    void InitDate();
    void ReadPlayDB();
    //汇总当天玩家投注
    void AggregatePlayerBets();

    //统计所有产生业绩的玩家列表
    void CalculatAllBenefitsUser();
    //查出所有代理的保底金额
    void AgentGuarantee();
    //算出已经下注的玩家(包括上级玩家)的团队贡献值
    void  UsersContribution();

    //插入玩家的当天的收益表
    void InsertUserProfitTable();
    //通过日期获取一个time_point
    time_point<system_clock> getTimePoint(tm t);

    //计算启动时间
    int64_t GetStartTimeSeconds();

    //计算下次启动的时间，并且启动定时器
    void StartGameNestTime();


    //定时器事件
    void FucTimer();

    //计算好需要汇总的注单，开始时间和结束时间范围
    void StartTimeAndEndTime();


    //删除设定时间段的注单,时间是设定的时间以及往前推的时间
    void DeleteUserInsertList();

    //汇总新注册玩家
    void NewRegisteredUser();

    //汇总所有产生新增直属玩家或者新增团队玩家的玩家集合
    void HaveAddNewRegisteredUser();


    void ReadProxyInfo();

    bool  OnMakeDir();

    void  openLog(const char *p, ...);
    /////////////////////////////////////
    bool InItAgorithm();

    void ThreadInitCallback();

    inline string  InitialConversion(int64_t timeSeconds)
    {
        time_t rawtime =(time_t) timeSeconds;
        struct tm * timeinfo;
        timeinfo = localtime ( &rawtime );


        int Year = timeinfo->tm_year+1900;

        int Mon = timeinfo->tm_mon+1;

        int Day = timeinfo->tm_mday;

        int Hour = timeinfo->tm_hour;

        int Minuts = timeinfo->tm_min;

        int Seconds = timeinfo->tm_sec;

        return to_string(Year)+"-"+to_string(Mon)+"-"+to_string(Day)
                +" "+to_string(Hour)+":"+to_string(Minuts)+":"+to_string(Seconds);
    }

    // 当前日期零点时间戳
    inline time_t getZeroTimeStamp(tm *tp)
    {
        tm *tpNew = new tm;
        tpNew->tm_year = tp->tm_year;
        tpNew->tm_mon = tp->tm_mon;
        tpNew->tm_mday = tp->tm_mday;
        tpNew->tm_hour = 0;
        tpNew->tm_min = 0;
        tpNew->tm_sec = 0;
        return mktime(tpNew)/*-8*60*60*/;
        delete tpNew;
    }
    //周分红总函数，在定时器里跑
    void WeekTotalFunction();
    //分红总配置表
    void ReadWeekInformation();
    //周注单汇总,把每天玩家汇总数据，汇总成周的数据,通过注单和设定公式，计算出每个人的周分红值
    void WeekDividends();

    //读取周分红配置
    void ReadWeekCondiction();

    //统计周业绩,份额等
    void weekCalculate();
};

#endif
