#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#include <boost/filesystem.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>


#include "public/GlobalFunc.h"
#include "Random.h"

#include "AndroidUserManager.h"
#include "GameServer/AndroidUserItem.h"
#include "MatchRoomManager.h"


#include "public/ThreadLocalSingletonMongoDBClient.h"


#include "proto/MatchServer.Message.pb.h"


//using namespace Landy;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

CAndroidUserManager::CAndroidUserManager() :
    m_pMatchInfo(NULL)
    , m_pGameRoomInfo(NULL)
    , m_game_logic_thread(NULL)
    , m_maxAndroidCountPerTable(0)
    , m_maxAndroidMaxUserCountPerTable(0)
    , m_pfnCreateAndroid(NULL)
{
//    m_TimerLoopThread.startLoop();
}

CAndroidUserManager::~CAndroidUserManager()
{
    m_AllAndroidUserVec.clear();
    m_AndroidUserItemFreeList.clear();
    m_AndroidUserItemUsedList.clear();

    m_pMatchInfo = NULL;
    m_pGameRoomInfo = NULL;
    m_game_logic_thread = NULL;
}

void CAndroidUserManager::Init(MatchRoomInfo* pMatchInfo, tagGameRoomInfo * pGameRoomInfo, shared_ptr<EventLoopThread> pILogicThread)
{
    m_pMatchInfo  = pMatchInfo;
    m_pGameRoomInfo  = pGameRoomInfo;
    m_game_logic_thread  = pILogicThread;

    //开始加载机器人模块
    string strDllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
    strDllPath.append("/");
    std::string strDllName = GlobalFunc::clearDllPrefix(pMatchInfo->servicename);
    strDllName.append("_score_android");
    strDllName.insert(0, "./lib");
    strDllName.append(".so");
    strDllName.insert(0, strDllPath);

    // load library.
    void* dp = 0;
    char* err = 0;
    dp = dlopen(strDllName.c_str(), RTLD_LAZY);
    if(dp == 0)
    {
        err = dlerror();
        char buf[BUFSIZ] = { 0 };
        snprintf(buf, BUFSIZ, "Can't Open SO: %s : Errno: %s", strDllName.c_str(), err);
        LOG_ERROR << buf;
        exit(0);
    }
    if(dp)
    {
        m_pfnCreateAndroid = (PFNCreateAndroid)dlsym(dp, FNAME_CREATE_ANDROID);
        if(!m_pfnCreateAndroid)
        {
            err = dlerror();
            char buf[BUFSIZ] = { 0 };
            snprintf(buf, BUFSIZ, "Can't Find Function: %s : errno: %s", FNAME_CREATE_ANDROID, err);
            LOG_ERROR << buf;
            exit(0);
            return;
        }
    }

//    LoadAndroidParam();

#if 0
    m_TimerCheckIn.start(3000, bind(&CAndroidUserManager::OnTimerCheckUserIn, this, std::placeholders::_1), NULL, -1, false);
#else
//    m_TimerLoopThread.getLoop()->runEvery(3.0, boost::bind(&CAndroidUserManager::OnTimerCheckUserIn, this));
#endif
}

bool CAndroidUserManager::LoadAndroidParam()
{
    //LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    uint32_t nCount = 0;
    bool bSuccess = false;
    try
    {
        //LOG_INFO << "Preload android Game Strategy";
        mongocxx::collection androidStrategy = MONGODBCLIENT["gameconfig"]["android_strategy"];
        bsoncxx::document::value query_value = document{} << "gameid" << (int32_t)m_pGameRoomInfo->gameId << "roomid" << (int32_t)m_pGameRoomInfo->roomId << finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> result_ = androidStrategy.find_one(query_value.view());
       
        LOG_WARN << "Query android_strategy:"<<bsoncxx::to_json(query_value);
        
        if( result_ )
        {
            bsoncxx::document::view view = result_->view();
            LOG_ERROR << "Query Android Strategy view:"<<bsoncxx::to_json(view);
            m_pAndroidStrategy.gameId = view["gameid"].get_int32();
            m_pAndroidStrategy.roomId = view["roomid"].get_int32();
            m_pAndroidStrategy.exitLowScore = view["exitLowScore"].get_int64();
            m_pAndroidStrategy.exitHighScore = view["exitHighScore"].get_int64();
            m_pAndroidStrategy.minScore = view["minScore"].get_int64();
            m_pAndroidStrategy.maxScore = view["maxScore"].get_int64();
            auto arr = view["areas"].get_array();
            for( auto &elem : arr.value)
            {
                AndroidStrategyArea area;
                area.weight = elem["weight"].get_int32();
                area.lowTimes = elem["lowTimes"].get_int32();
                area.highTimes = elem["highTimes"].get_int32();
                m_pAndroidStrategy.areas .push_back(area);
            }

            if( m_pAndroidStrategy.areas.size() == 0 ){
                LOG_ERROR << "=======Android Strategy areas.size is 0 ,please check again !!!=======";
                LOG_ERROR << "=======Android Strategy areas.size is 0 ,please check again !!!=======";
                LOG_ERROR << "=======Android Strategy areas.size is 0 ,please check again !!!=======";
            }
        }
        else{
            LOG_ERROR << "=======Android Strategy Query Error,please check again !!!=======";
            LOG_ERROR << "=======Android Strategy Query Error,please check again !!!=======";
            LOG_ERROR << "=======Android Strategy Query Error,please check again !!!=======";
        }

        //本次请求开始时间戳(微秒) 
        muduo::Timestamp timestart = muduo::Timestamp::now();

        uint32_t maxAndroidCount = m_maxAndroidCountPerTable * m_pGameRoomInfo->tableCount;
		LOG_INFO << __FUNCTION__ << " --- *** " << "Preload android user, maxCount = " << maxAndroidCount;
        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["android_user"];
//        auto query_value = document{} << finalize;
//        LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
        mongocxx::cursor cursor = coll.find({});
        for(auto &doc : cursor)
        {
//            LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(doc);
            shared_ptr<tagAndroidUserParameter> param(new tagAndroidUserParameter);
            param->userId       = doc["userId"].get_int64();
            param->account      = doc["account"].get_utf8().value.to_string();
            param->nickName     = doc["nickName"].get_utf8().value.to_string();
            param->headId       = doc["headId"].get_int32();

            param->enterTime    = doc["enterTime"].get_utf8().value.to_string();
            param->leaveTime    = doc["leaveTime"].get_utf8().value.to_string();
            param->takeMinScore = doc["minScore"].get_int64();
            param->takeMaxScore = doc["maxScore"].get_int64();
            param->location     = doc["location"].get_utf8().value.to_string();

            param->score        = GlobalFunc::RandomInt64(param->takeMaxScore, param->takeMaxScore);

            AddAndroid(param);

            if(++nCount >= maxAndroidCount)
                break;

         //   LOG_DEBUG << ">>> Preload Android Item, userId:" << param->userId << ", score:" << param->score;
        }

        muduo::Timestamp timenow = muduo::Timestamp::now();
        double timdiff = muduo::timeDifference(timenow, timestart);
        LOG_WARN << "---加载机器人耗时(s)=======>[" << timdiff<<"]<=======";

        bSuccess = true;
		LOG_WARN << __FUNCTION__ << " --- *** " << "Add count = " << nCount;
    }catch (exception &e)
    {
       // LOG_ERROR << "LoadAndroidParam MongoDB failed, message:" << e.what();
    }
    return bSuccess;
}

uint32_t CAndroidUserManager::GetMaxAndroidCountPerTable()
{
    return m_maxAndroidCountPerTable;
}

void CAndroidUserManager::ClearAndroidUserParam()
{
}

void CAndroidUserManager::AddAndroid(shared_ptr<tagAndroidUserParameter>& ptrAndroidParam)
{
    GSUserBaseInfo userInfo;
    userInfo.userId = ptrAndroidParam->userId;
    userInfo.account = ptrAndroidParam->account;
    userInfo.headId = ptrAndroidParam->headId;
    userInfo.nickName = ptrAndroidParam->headId;
    userInfo.userScore = ptrAndroidParam->score;
    userInfo.agentId = 0;
    userInfo.status = 0;
    userInfo.location = ptrAndroidParam->location;
    userInfo.takeMinScore = m_pAndroidStrategy.minScore; //ptrAndroidParam->takeMinScore;
    userInfo.takeMaxScore = m_pAndroidStrategy.maxScore;//ptrAndroidParam->takeMaxScore;

    shared_ptr<CAndroidUserItem> pAndroidUserItem(new CAndroidUserItem());
    shared_ptr<IAndroidUserItemSink> pSink = m_pfnCreateAndroid();

    pAndroidUserItem->SetAndroidUserItemSink(pSink);
    pAndroidUserItem->SetUserBaseInfo(userInfo);

    shared_ptr<IServerUserItem> pIServerUserItem = pAndroidUserItem;
    pSink->SetUserItem(pIServerUserItem);
    pSink->SetAndroidStrategy(&m_pAndroidStrategy);
    {
//        WRITE_LOCK(m_mutex);
        m_AllAndroidUserVec.push_back(pAndroidUserItem);
        m_AndroidUserItemFreeList.push_back(pAndroidUserItem);
    }
}

shared_ptr<CAndroidUserItem> CAndroidUserManager::GetFreeAndroidUserItem()
{
    shared_ptr<CAndroidUserItem> pUserItem;

//    WRITE_LOCK(m_mutex);
    if(m_AndroidUserItemFreeList.empty())
    {
        return pUserItem;
    }

    pUserItem = m_AndroidUserItemFreeList.front();
    m_AndroidUserItemUsedList.push_back(pUserItem);
    m_AndroidUserItemFreeList.pop_front();

    return pUserItem;
}

bool CAndroidUserManager::DeleteAndroidUserItem(int64_t userId)
{
   // LOG_DEBUG << __FILE__ << " " << __FUNCTION__ << " DeleteAndroidUserItem, userid:" << userId;

    bool bSuccess = false;

//    if(pIServerUserItem)
    {
//        WRITE_LOCK(m_mutex);
        for(auto it = m_AndroidUserItemUsedList.begin(); it != m_AndroidUserItemUsedList.end(); ++it)
        {
            shared_ptr<CAndroidUserItem> pTempUserItem(*it);
            if(pTempUserItem->GetUserId() == userId)
            {
                //把机器人的分数
//                int64_t userId = pIServerUserItem->GetUserId();

#if LANDY_DEBUF
                GS_UserBaseInfo& userinfo = pTempUserItem->GetUserBaseInfo();
                userinfo.nUserScore       = pTempUserItem->GetUserScore();

                userinfo.nUserScore       = userinfo.nDefaultScore;
                userinfo.nUserScore       = 200000;
                userinfo.nVipLevel        = rand() % 7;
                userinfo.nVipLevel2       = rand() % 7;
#endif

//                //到DB更新机器人状态
//                UpdateAndroidStatus(nUserId, 0);
                m_AndroidUserItemFreeList.push_back(pTempUserItem);
                m_AndroidUserItemUsedList.erase(it);
                shared_ptr<IAndroidUserItemSink> pSink = pTempUserItem->GetAndroidUserItemSink();
                if(pSink)
                    pSink->RepositionSink();
                return true;
            }
        }
    }
    return bSuccess;
}

void CAndroidUserManager::SetAndroidUserCount(uint32_t androidCount, uint32_t androidMaxUserCount)
{
    m_maxAndroidCountPerTable = androidCount;
    m_maxAndroidMaxUserCountPerTable = androidMaxUserCount;
}


void CAndroidUserManager::OnTimerCheckUserIn()
{
    //回收旧的定时器
    m_game_logic_thread->getLoop()->cancel(m_timerIdCheckUserIn);
    //定时器间隔时间
    float gap = m_pMatchInfo->androidEnterMinGap;
	if (m_pMatchInfo->androidEnterMaxGap > m_pMatchInfo->androidEnterMinGap)
		gap += rand() % (m_pMatchInfo->androidEnterMaxGap - m_pMatchInfo->androidEnterMinGap);
    //获取比赛房间列表
    gap=gap/1000.0f;
    list<shared_ptr<IMatchRoom>>& vecMatch = MatchRoomManager::get_mutable_instance().GetUsedMatch();
    LOG_DEBUG << "--- *** 比赛房间数量 = " << vecMatch.size() << " 检查加入间隔时间 = " << gap << "s";
    
    do {
        if (m_pGameRoomInfo->serverStatus == SERVER_STOPPED || !m_pMatchInfo->bCanJoinMatch)
        {
            LOG_DEBUG << "--- *** 比赛房间状态 = " << m_pGameRoomInfo->serverStatus
                << (m_pMatchInfo->bCanJoinMatch ? " 能比赛，加入比赛失败 !!!" : " 不能比赛，加入比赛失败 !!!");
            break;
        }

        if (m_AndroidUserItemFreeList.empty())
        {
            LOG_DEBUG << "--- *** 机器人没有库存了，加入比赛失败 !!!";
            break;
        }
        //遍历比赛房间 ///
        for (auto it : vecMatch)
        {
            shared_ptr<IMatchRoom> pMatchRoom = it;
            if (pMatchRoom
                && pMatchRoom->GetMatchStatus() == MATCH_WAIT_START
                && pMatchRoom->GetPlayerCount() >= 0)
            {
                LOG_DEBUG << "--- *** ["
                    << pMatchRoom->getMatchRoundId() << "] "
                    << pMatchRoom->GetMatchRoomInfo()->matchRoomId << " { "
                    << pMatchRoom->GetMatchRoomInfo()->matchName << " }"
                    << " 状态 = " << pMatchRoom->GetMatchStatus()
                    << " 人数 = " << pMatchRoom->GetPlayerCount() << " 等待开赛";

                uint32_t maxPlayerCount = m_pMatchInfo->maxPlayerNum;
                uint32_t curPlayerCount = pMatchRoom->GetAllPlyerCount();
                if (m_pGameRoomInfo->bEnableAndroid &&
                    curPlayerCount < maxPlayerCount /*&&
                    curAndroidPlayerCount < maxAndroidCount*/)
                {
                    uint32_t wNeedCount = 0;// maxAndroidCount - curAndroidPlayerCount;
                    uint32_t count = maxPlayerCount - curPlayerCount;
                    wNeedCount = count;//wNeedCount < count ? wNeedCount : count;
                    for (int i = 0; i < wNeedCount; ++i)
                    {
                        // check current table android player count is arrived the max.
                        shared_ptr<CServerUserItem> pUserItem = GetFreeAndroidUserItem();
                        if (!pUserItem)
                        {
                            LOG_DEBUG << "--- *** 没有空闲机器人，加入比赛失败 !!!";
                            continue;
                        }

                        int64_t minLine = 0;
                        int64_t highLine = 100000;
                        int64_t minScore = pUserItem->GetTakeMinScore();//(m_pGameRoomInfo->enterMinScore > 0 ? m_pGameRoomInfo->enterMinScore : pUserItem->GetTakeMinScore());
                        LOG_INFO << "--- *** 机器人 *** 1 --- " << minScore;
                        int randNum = rand() % 1000;
                        for (int j = 0; j < m_pAndroidStrategy.areas.size(); j++)
                        {
                            LOG_INFO << "------Android weight------" << j <<" randNum["<<randNum << "],weight["<<m_pAndroidStrategy.areas[j].weight<<"]";
                            if (randNum < m_pAndroidStrategy.areas[j].weight)
                            {
                                 //根据房间准入范围随机机器人积分
                                minLine = m_pAndroidStrategy.areas[j].lowTimes * minScore / 100;
                                highLine = m_pAndroidStrategy.areas[j].highTimes * minScore / 100;
                                break;
                            }
                        } 
                       
                        LOG_INFO << "--- *** 机器人 *** 2 ---minLine[" << minLine << "],highLine["<<highLine<<"]";
                        int64_t score = GlobalFunc::RandomInt64(minLine, highLine);
                        pUserItem->SetUserScore(score);
                        //加入比赛房间 ///
                        pMatchRoom->JoinMatch(pUserItem);
                        //让每次都只进一个机器人 ///
                        goto return_;
                    }
                }
                else {
                    LOG_DEBUG << "--- *** ["
                        << pMatchRoom->getMatchRoundId() << "] "
                        << "比赛房间人数 = " << curPlayerCount
                        << " 最大人数 = " << maxPlayerCount << " 加入比赛失败 !!!";
                }
            }
            else {
                if (pMatchRoom) {
                    LOG_DEBUG << "--- *** ["
                        << pMatchRoom->getMatchRoundId() << "] "
                        << pMatchRoom->GetMatchRoomInfo()->matchRoomId << " { "
                        << pMatchRoom->GetMatchRoomInfo()->matchName << " }"
                        << " 状态 = " << pMatchRoom->GetMatchStatus()
                        << " 人数 = " << pMatchRoom->GetPlayerCount() << " 轮询比赛失败 !!!";
                }
                else {
                    LOG_DEBUG << "--- *** pMatchRoom == NULL 轮询比赛失败 !!!";
                }
            }
        }
		//到这里说明没有一个比赛是可进入的，新开一个比赛
		shared_ptr<CServerUserItem> pUser = make_shared<CServerUserItem>();
        shared_ptr<IMatchRoom> pMatchRoom = MatchRoomManager::get_mutable_instance().FindNormalSuitMatch(pUser);
        if (pMatchRoom) {
			LOG_DEBUG << "--- *** ["
				<< pMatchRoom->getMatchRoundId() << "] "
				<< pMatchRoom->GetMatchRoomInfo()->matchRoomId << " { "
				<< pMatchRoom->GetMatchRoomInfo()->matchName << " }"
				<< " 状态 = " << pMatchRoom->GetMatchStatus()
				<< " 人数 = " << pMatchRoom->GetPlayerCount() << " 新开比赛房间成功 :) :) :)";
        } else {
            LOG_DEBUG << "--- *** 日哦，没有可用比赛房间，新开比赛房间失败 :( :( :(";
        }
    } while (0);
return_:
    //重新启动定时器 ///
    m_timerIdCheckUserIn = m_game_logic_thread->getLoop()->runAfter(gap, bind(&CAndroidUserManager::OnTimerCheckUserIn, this));
    LOG_DEBUG << "--- *** 重新启动定时器 @@@@@@@@@@@@@@@@";
}

void CAndroidUserManager::Hourtimer(shared_ptr<EventLoopThread>& eventLoopThread)
{
    time_t t=time(NULL);
    struct tm *local=localtime(&t);
    uint8_t hour=(int)local->tm_hour;
    float ra=((random()%10)+95)*0.01;//随机浮动一定比例0.9~1.1
    //本来是整点定时器，现在又要一个小时内机器人要有波动
    m_androidEnterPercentage=m_pGameRoomInfo->enterAndroidPercentage[hour]?
              m_pGameRoomInfo->enterAndroidPercentage[hour]*(ra):0.5*(ra);
    //nextHourTick-localTick

    eventLoopThread->getLoop()->runAfter(60,bind(&CAndroidUserManager::Hourtimer,&(CAndroidUserManager::get_mutable_instance()),eventLoopThread));
}


