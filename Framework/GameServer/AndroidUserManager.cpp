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
//#include "Random.h"

#include "AndroidUserManager.h"
#include "AndroidUserItem.h"
#include "GameTableManager.h"
#include "TableFrame.h"

#include "public/ThreadLocalSingletonMongoDBClient.h"


#include "proto/GameServer.Message.pb.h"


//using namespace Landy;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

CAndroidUserManager::CAndroidUserManager() :
    m_pGameInfo(NULL)
    , m_pGameRoomInfo(NULL)
    , m_pILogicThread(NULL)
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
	m_androidEnterPercentageVec.clear();

    m_pGameInfo = NULL;
    m_pGameRoomInfo = NULL;
    m_pILogicThread = NULL;
}
void CAndroidUserManager::SetGameRoomInfo(tagGameInfo* gameInfo ,tagGameRoomInfo* roomInfo)
{
    m_pGameInfo  = gameInfo;
    m_pGameRoomInfo  = roomInfo;
}
void CAndroidUserManager::Init(tagGameInfo* pGameInfo, tagGameRoomInfo * pGameRoomInfo, ILogicThread* pILogicThread)
{
    m_pGameInfo  = pGameInfo;
    m_pGameRoomInfo  = pGameRoomInfo;
    m_pILogicThread  = pILogicThread;

    //开始加载机器人模块
    string strDllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
    strDllPath.append("/");
    std::string strDllName = GlobalFunc::clearDllPrefix(pGameInfo->gameServiceName);
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

bool CAndroidUserManager::LoadAndroidParam(int32_t agentid,int32_t currency)
{
    //LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    uint32_t nCount = 0;
    bool bSuccess = false;
    try
    {
        //这里还要加上分代理判断
        //LOG_INFO << "Preload android Game Strategy";
        mongocxx::collection androidStrategy = MONGODBCLIENT["gameconfig"]["android_strategy"];
        bsoncxx::document::value query_monye = document{} << "gameid" << (int32_t)m_pGameRoomInfo->gameId << "roomid" << (int32_t)m_pGameRoomInfo->roomId <<"currency"<<currency<< finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> queryresult = androidStrategy.find_one(query_monye.view());
        if(currency!=0)
        {
            if(queryresult)
            {
                bsoncxx::document::view view = queryresult->view();
                LOG_DEBUG << "Query Android Strategy:"<<bsoncxx::to_json(view);
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

                uint32_t maxAndroidCount = m_maxAndroidCountPerTable * m_pGameRoomInfo->tableCount;
                LOG_INFO << __FUNCTION__ << " --- *** " << "Preload android user, maxCount = " << maxAndroidCount;
                mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["android_user"];
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

                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            bsoncxx::document::value query_value = document{} << "gameid" << (int32_t)m_pGameRoomInfo->gameId << "roomid" << (int32_t)m_pGameRoomInfo->roomId <<"agentid"<<agentid<< finalize;
            bsoncxx::stdx::optional<bsoncxx::document::value> result = androidStrategy.find_one(query_value.view());
            if(result)
            {
                bsoncxx::document::view view = result->view();
                LOG_DEBUG << "Query Android Strategy:"<<bsoncxx::to_json(view);
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
            }
            else
            {
                bsoncxx::document::value query = document{} << "gameid" << (int32_t)m_pGameRoomInfo->gameId << "roomid" << (int32_t)m_pGameRoomInfo->roomId <<"agentid"<<0<< finalize;
                bsoncxx::stdx::optional<bsoncxx::document::value> resultr = androidStrategy.find_one(query.view());


                bsoncxx::document::view view = resultr->view();
                LOG_ERROR << "读不到配置用零号配置";
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
            }
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

                //LOG_DEBUG << ">>> Preload Android Item, userId:" << param->userId << ", score:" << param->score;
            }
        }

        bSuccess = true;
		LOG_INFO << __FUNCTION__ << " --- *** " << "Add count = " << nCount;
    }catch (exception &e)
    {
        LOG_ERROR << "LoadAndroidParam MongoDB failed, message:" << e.what();
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
    //LOG_DEBUG << __FILE__ << " " << __FUNCTION__ << " DeleteAndroidUserItem, userid:" << userId;

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
//    LOG_INFO << __FILE__ << __FUNCTION__;

    if(m_pGameRoomInfo->serverStatus == SERVER_STOPPED)
    {
        LOG_WARN << "SERVER IS STOPPING...";
        return;
    }

    //LOG_ERROR << " Used Android Count:" << m_AndroidUserItemUsedList.size() << " Free Android Count:"<<m_AndroidUserItemFreeList.size();

    if(m_AndroidUserItemFreeList.empty())
    {
        LOG_DEBUG << "+++++++++机器人没有库存了+++++++";
        return;
    }

//    muduo::MutexLockGuard lock(m_AddAndroidUsermutex);

//    int maxAndroidCount = m_maxAndroidCount;		    //Max Android Count
    list<shared_ptr<ITableFrame>> &vecTableFrame = CGameTableManager::get_mutable_instance().GetUsedTableFrame();
	//随机选百人桌子号
	int nTableCount = vecTableFrame.size();
	vector<int32_t> vWeigt;
	uint32_t chooseTableID = 0;
	if (m_pGameInfo->gameType == GameType_BaiRen && nTableCount > 1)
	{
		/*int32_t allWeight = 0;
		int32_t cerWeight = 0;
		for (int i = 0;i < nTableCount;i++)
		{
            cerWeight = 10000 + (nTableCount - 1 - i) * 1000;
			allWeight += cerWeight;
			vWeigt.push_back(cerWeight);
		}
		int32_t pribility = m_random.betweenInt(0, allWeight).randInt_mt(true);
		int32_t resWeight = 0;
		for (int j = 0;j < nTableCount;j++)
		{
			resWeight += vWeigt[j];
			if (pribility - resWeight <= 0)
			{
				chooseTableID = j;
				break;
			}
		}*/
		chooseTableID = m_random.betweenInt(0, nTableCount - 1).randInt_mt(true);
	}
    //牛牛游戏需要让长时间不入桌子的机器人离开桌子
//    uint32_t dwNow = (uint32_t)time(NULL);
    for (auto it : vecTableFrame)
    {
        shared_ptr<CTableFrame> pTableFrame = dynamic_pointer_cast<CTableFrame>(it);
        if(pTableFrame)// && (m_pRoomKindInfo->bisDynamicJoin || (pTableFrame->GetGameStatus() == GAME_STATUS_FREE)))
        {			
            uint32_t maxPlayerCount = m_pGameRoomInfo->maxPlayerNum;
            int32_t maxAndroidCount = m_pGameRoomInfo->androidMaxUserCount;
            uint32_t curRealPlayerCount = 0, curAndroidPlayerCount = 0;
            pTableFrame->GetPlayerCount(curRealPlayerCount, curAndroidPlayerCount);
            uint32_t curPlayerCount = curRealPlayerCount + curAndroidPlayerCount;

            uint32_t tableID = pTableFrame->GetTableId();
            if (m_pGameInfo->gameType == GameType_BaiRen)
            {
				// 相同桌子才坐下
				if (chooseTableID != tableID)
				{
					continue;
				}
                maxAndroidCount*=(m_androidEnterPercentage*m_androidEnterPercentageVec[tableID]);

                //百人场机器人进入房间逻辑
                auto pUser=make_shared<CServerUserItem>();
                if(!pTableFrame->CanJoinTable(pUser))
                    continue;

                // +++++++++百人场机器人进入房间逻辑 2+++++++ 1 100 0 0 - AndroidUserManager.cpp:335
                //LOG_DEBUG << "+++++++++百人场机器人进入房间逻辑 2+++++++ " << curPlayerCount <<" "<< maxPlayerCount <<" " << curAndroidPlayerCount <<" " << maxAndroidCount;

                if(m_pGameRoomInfo->bEnableAndroid &&
                        curPlayerCount < maxPlayerCount &&
                        curAndroidPlayerCount < maxAndroidCount)
                {
                    //几个真实玩家兑换一个机器人
                    if(m_pGameRoomInfo->realChangeAndroid>0)
                        maxAndroidCount-=(int)curRealPlayerCount/m_pGameRoomInfo->realChangeAndroid;

                    int32_t wNeedCount = 0;// maxAndroidCount - curAndroidPlayerCount;
                    int32_t count =maxAndroidCount-curAndroidPlayerCount;
                    if(curAndroidPlayerCount>=maxAndroidCount)//达到机器人数，不再进入
                        count=0;

                   // LOG_DEBUG << "+++++++++百人场机器人进入房间逻辑 3+++++++";

                    //一次最多进五个
                    wNeedCount = count>5?5:count;//wNeedCount < count ? wNeedCount : count;
                    for (int i = 0; i < wNeedCount; ++i)
                    {
                        shared_ptr<CServerUserItem> pUserItem = GetFreeAndroidUserItem();
                        if(!pUserItem)
                        {
                            continue;
                        }
						if (pUserItem->isPlaying())
						{
							continue;
						}
                        // 水果机百人场机器人数量限制（仅限水果机使用,1810目前写死，后面优化）
                        if(m_pGameRoomInfo->gameId == (int32_t)eGameKindId::sgj){
                            if(!pTableFrame->CanJoinTable(pUserItem)){
                                // LOG_DEBUG << "+++++++++水果机百人场机器人不能进入房间+++++++";
                                break;
                            }
                        }

                        int64_t minScore = pUserItem->GetTakeMinScore();

                        int randNum = rand()%1000;
                        int j  = 0;
                        for(; j < m_pAndroidStrategy.areas.size(); j++)
                        {
                            if( randNum < m_pAndroidStrategy.areas[j].weight)
                            {
                                break;
                            }
                        }
                        int64_t minLine = m_pAndroidStrategy.areas[j].lowTimes*minScore/100;
                        int64_t highLine = m_pAndroidStrategy.areas[j].highTimes*minScore/100;
                        int64_t score = GlobalFunc::RandomInt64(minLine,highLine);
                        pUserItem->SetUserScore(score);

                       // LOG_DEBUG << "+++++++++百人场机器人进入房间逻辑 4+++++++";


    //                    GSUserScoreInfo scoreInfo;
    //                    scoreInfo.userId    = pUserItem->GetUserId();
    //                    scoreInfo.enterScore  = pUserItem->GetUserScore();
    //                    pUserItem->SetUserScoreInfo(scoreInfo);

                        shared_ptr<IAndroidUserItemSink> pSink = pUserItem->GetAndroidUserItemSink();
                        if(pSink)
                        {
                            pSink->SetTableFrame(it);
                        }

    //                     LOG_DEBUG << ">>> InsertAndroidUserItem for Table:" << pTableFrame->GetTableId()
    //                               << ", userid:" << pUserItem->GetUserId() << ", score:" << pUserItem->GetUserScore()
    //                               << ", android count:" << pTableFrame->GetAndroidPlayerCount()
    //                               << ", m_AndroidUserItemFreeList size:" << m_AndroidUserItemFreeList.size()
    //                               << ", m_AndroidUserItemSitList size:" << m_AndroidUserItemUsedList.size();

    //                    //到DB更新机器人状态
    //                    UpdateAndroidStatus(pUserItem->GetUserID(), 1);
                        // room sit chair item data value for later.
                        pTableFrame->RoomSitChair(pUserItem, NULL, false);

                        //LOG_DEBUG << "+++++++++百人场机器人进入房间逻辑 5+++++++";

                        //让每次都只进一个机器人
                        //break;
                    }
					break;
                }
            }else
            {
                if(curRealPlayerCount == 0 || pTableFrame->GetGameStatus() == GAME_STATUS_START) //duizhan
                    continue;
                //非百人游戏匹配前3.6秒都必须等待玩家加入桌子，禁入机器人，定时器到时空缺的机器人一次性填补
                //如果定时器触发前，真实玩家都齐了，秒开
                //处理逻辑在各个子游戏模块 CanJoinTable 中处理
                auto pUser=make_shared<CServerUserItem>();
                if(!pTableFrame->CanJoinTable(pUser))
                    continue;

    //            if(curRealPlayerCount >= m_pGameRoomInfo->androidMaxUserCount)
    //            {
    //                LOG_INFO << "curRealPlayerCount >= androidMaxUserCount" << curRealPlayerCount << ":" <<androidMaxUserCount<<" return";
    //                continue;
    //            }

    //            if(curPlayerCount >= maxPlayerCount)
    //            {
    //                LOG_INFO << "curPlayerCount >= maxPlayerCount" << curPlayerCount << ":" <<maxPlayerCount<<" return";
    //                continue;
    //            }

                if(m_pGameRoomInfo->bEnableAndroid &&
                        curPlayerCount < maxPlayerCount /*&&
                        curAndroidPlayerCount < maxAndroidCount*/)
                {
                    uint32_t wNeedCount = 0;// maxAndroidCount - curAndroidPlayerCount;
                    uint32_t count = maxPlayerCount - curPlayerCount;
                    wNeedCount = count;//wNeedCount < count ? wNeedCount : count;
                    for (int i = 0; i < wNeedCount; ++i)
                    {
                        // check current table android player count is arrived the max.
                        //if(pTableFrame->GetAndroidPlayerCount() >= maxAndroidCount)
                        //{
                        //    break;
                        //}

                        shared_ptr<CServerUserItem> pUserItem = GetFreeAndroidUserItem();
                        if(!pUserItem)
                        {
                            continue;
                        }

    //                    int64_t score = GlobalFunc::RandomInt64(pUserItem->GetTakeMinScore(), pUserItem->GetTakeMaxScore());
                        int64_t minScore = pUserItem->GetTakeMinScore();//(m_pGameRoomInfo->enterMinScore > 0 ? m_pGameRoomInfo->enterMinScore : pUserItem->GetTakeMinScore());
    //                    int64_t maxScore =  (m_pGameRoomInfo->enterMaxScore > 0 ? m_pGameRoomInfo->enterMaxScore : pUserItem->GetTakeMaxScore());
    //                    int64_t score = GlobalFunc::RandomInt64(minScore, maxScore);
                        int randNum = rand()%1000;
                        int j  = 0;
                        for(; j < m_pAndroidStrategy.areas.size(); j++)
                        {
                            if( randNum < m_pAndroidStrategy.areas[j].weight)
                            {
                                break;
                            }
                        }
                        int64_t minLine = m_pAndroidStrategy.areas[j].lowTimes*minScore/100;
                        int64_t highLine = m_pAndroidStrategy.areas[j].highTimes*minScore/100;

                        if(minLine==0&&highLine==0)
                        {
                            LOG_ERROR<<" 机器人配置表配置不对，或者没有配置,请检查配置 android_strategy ！！！！！！！！！！！！！！！！！！";
                            minLine = 500000;
                            highLine = 50000000;
                        }
                        int64_t score = GlobalFunc::RandomInt64(minLine,highLine);
                        pUserItem->SetUserScore(score);

    //                    GSUserScoreInfo scoreInfo;
    //                    scoreInfo.userId    = pUserItem->GetUserId();
    //                    scoreInfo.enterScore  = pUserItem->GetUserScore();
    //                    pUserItem->SetUserScoreInfo(scoreInfo);

                        shared_ptr<IAndroidUserItemSink> pSink = pUserItem->GetAndroidUserItemSink();
                        if(pSink)
                        {
                            pSink->SetTableFrame(it);
                        }

    //                     LOG_DEBUG << ">>> InsertAndroidUserItem for Table:" << pTableFrame->GetTableId()
    //                               << ", userid:" << pUserItem->GetUserId() << ", score:" << pUserItem->GetUserScore()
    //                               << ", android count:" << pTableFrame->GetAndroidPlayerCount()
    //                               << ", m_AndroidUserItemFreeList size:" << m_AndroidUserItemFreeList.size()
    //                               << ", m_AndroidUserItemSitList size:" << m_AndroidUserItemUsedList.size();

    //                    //到DB更新机器人状态
    //                    UpdateAndroidStatus(pUserItem->GetUserID(), 1);
                        // room sit chair item data value for later.
                        pTableFrame->RoomSitChair(pUserItem, NULL, false);
                        //让每次都只进一个机器人
                        break;
                    }
                }
            }
        }
    }

}

void CAndroidUserManager::Hourtimer(shared_ptr<EventLoopThread>& eventLoopThread)
{
    time_t t=time(NULL);
    struct tm *local=localtime(&t);
    uint8_t hour=(int)local->tm_hour;
    //float ra=((random()%10)+95)*0.01;//随机浮动一定比例0.9~1.1		
	int32_t pribility = m_random.betweenInt(0, 9).randInt_mt(true);
	float ra = (pribility + 95)*0.01;//随机浮动一定比例0.9~1.1

    //本来是整点定时器，现在又要一个小时内机器人要有波动

//    struct tm nextHour;
//    memcpy(&nextHour,local,sizeof(struct tm));
//    nextHour.tm_hour=(hour+1)%24;
//    nextHour.tm_min =0;
//    nextHour.tm_sec= 0; /* Seconds.	[0-60] (1 leap second) */
//    time_t localTick = mktime(local);
//    time_t nextHourTick = mktime(&nextHour);
//    LOG_DEBUG <<"---------localTick:"<<localTick<<"nextHourTick"<<nextHourTick<<"interval:"<<nextHourTick-localTick;
    m_androidEnterPercentage=m_pGameRoomInfo->enterAndroidPercentage[hour]?
              m_pGameRoomInfo->enterAndroidPercentage[hour]*(ra):0.5*(ra);
    //nextHourTick-localTick

    eventLoopThread->getLoop()->runAfter(60,bind(&CAndroidUserManager::Hourtimer,&(CAndroidUserManager::get_mutable_instance()),eventLoopThread));
	if (m_pGameInfo->gameType == GameType_BaiRen && m_pGameRoomInfo->tableCount >= 1)
	{
		m_androidEnterPercentageVec.clear();
		float randFloat = 1.0f;
		if (m_pGameRoomInfo->tableCount == 1)
			m_androidEnterPercentageVec.push_back(randFloat);
		else
		{
			for (int i = 0;i < m_pGameRoomInfo->tableCount;i++)
			{
                randFloat = m_random.betweenFloat(0.25, 1.0).randFloat_mt(true);
				m_androidEnterPercentageVec.push_back(randFloat);
			}
		}		
	}
}

//// 默认返回平台的最大玩家数,如果有专用时间段配置,使用专用配置,否则使用默认配置.
//uint32_t CAndroidUserManager::GetAndroidMaxUserCount(bool bMaxonly)
//{
//    uint32_t dwMaxUserCount = m_maxAndroidMaxUserCountPerTable;
//    if(bMaxonly)
//    {
//        return (m_maxAndroidMaxUserCount);
//    }

//	time_t tt;
//	time(&tt);
//	int min_count = 0, max_count = 0;
//    struct tm* ptm  = localtime(&tt);
//    char szDate[32] = {0};
//    if(ptm)
//    {
//        snprintf(szDate,sizeof(szDate),("%02d:%02d:%02d"),ptm->tm_hour, ptm->tm_min,ptm->tm_sec);
//    }

//    // try to check if the special date time value has been arrived the max.
//    if(CAndroidTimedLimit::Singleton().isExist(szDate,min_count,max_count))
//	{
//		CRandom random;
//		dwMaxUserCount = random.Random_Int(min_count, max_count);
//        if(!dwMaxUserCount)
//        {
//             dwMaxUserCount = m_maxAndroidMaxUserCount;
//		}
//	}
////Cleanup:
//	return (dwMaxUserCount);
//}

//void CAndroidUserManager::UpdateAndroidStatus(int64_t userId, uint8_t cbStatus)
//{
//    LOG_ERROR << "CAndroidUserManager::UpdateAndroidStatus NO IMPL yet!";

//    /*
//    MSG_G2DB_Android_Status androidStatus;
//    androidStatus.dwAndroidUserID = nUserId;
//    androidStatus.cbStatus = cbStatus;
//    CNetModule::getSingleton().Send(pNetDB->GetSocket(), MSG_GDB_MAIN, SUB_G2DB_ANDROID_STATUE, &androidStatus, sizeof(androidStatus));
//    */

//    //更新在线人数
//    UpdateOnlineCount();
//}

//// try to get the specail android user manager base information content now.
//GS_UserBaseInfo* CAndroidUserManager::GetAndroidUserBaseInfo(dword userid)
//{
//    auto it  = m_AndroidUserBaseInfoMap.find(userid);
//    if  (it != m_AndroidUserBaseInfoMap.end())
//    {
//        return &it->second;
//    }
////Cleanup:
//    return NULL;
//}


//void CAndroidUserManager::OnTimeTick()
//{
//    MutexLockGuard lock(m_mutexLock);

//    for (auto it : m_lstAndroidUserSit)
//    {
//        it->OnTimerTick();
//    }
//}

//void CAndroidUserManager::CreateAllAndroid()
//{
//    while(!m_listAndroidParamStore.empty())
//    {
//        // check if no store android param.
//        CServerUserItem *pUserItem = NULL;

//        std::shared_ptr<tagAndroidUserParameter>& ptrAndroidParam = m_listAndroidParamStore.front();
//        //CreateAndroidUserItem(&pUserItem, ptrAndroidParam->nUserId);

//        try
//        {
//            CAndroidUserItem* pAndroidUserItem = new CAndroidUserItem();
//            pUserItem = pAndroidUserItem;
//            IAndroidUserItemSink* pSink = m_pfnCreateAndroid();
//            pAndroidUserItem->SetAndroidUserItemSink(pSink);
//        }
//        catch(...)
//        {
//            assert(false);
//            return;
//        }

//        // initialize the special content value item data.
//        m_lstAndroidUserFree.push_back(pUserItem);
//        m_mapAndroidUser[ptrAndroidParam->nUserId] = pUserItem;

//        if(pUserItem == NULL)
//        {
//            continue;
//        }

//        //int nTableAndroidCount = pTableFrame->GetAndroidPlayerCount();
//        //LOG_DEBUG << ">>> create android item, userid:" << userid << ", table android count:" << nTableAndroidCount << ",wMaxAndroidCount:" << wMaxAndroidCount;

//        auto itBaseInfo = m_mapAndroidUserInfo.find(ptrAndroidParam->nUserId);
//        if (itBaseInfo != m_mapAndroidUserInfo.end())
//        {
//            //检测分数是否符合标准
//            if (itBaseInfo->second.nUserScore < m_pRoomKindInfo->nEnterMinScore)
//            {
//                itBaseInfo->second.nUserScore += m_pRoomKindInfo->nEnterMinScore;
//            }
//            if (itBaseInfo->second.nUserScore > m_pRoomKindInfo->nEnterMaxScore)
//            {
//                int64_t maxScore = (int64_t)m_pRoomKindInfo->nEnterMaxScore;
//                if (maxScore != 0)
//                {
//                    itBaseInfo->second.nUserScore = m_pRoomKindInfo->nEnterMinScore + (rand() % maxScore);
//                }
//                else
//                {
//                    // if (m_pRoomKindInfo->wGameID == GAME_BR)
//                    if(m_pRoomKindInfo->bisDynamicJoin)
//                    {
//                        int nMaxScore = (ptrAndroidParam->nTakeMaxScore - ptrAndroidParam->nTakeMinScore);
//                        if (!nMaxScore) nMaxScore = 1;  // try to check if the number is not zero.
//                        double lTakeScore = ptrAndroidParam->nTakeMinScore + (rand() % nMaxScore);
//                        itBaseInfo->second.nUserScore = lTakeScore;
//                    }
//                }

//                //SetAndroidUserScore(ptrAndroidParam->nUserId, itBaseInfo->second.nUserScore);
//            }
//            //SetAndroidUserScore(ptrAndroidParam->nUserId, itBaseInfo->second.nUserScore);
//            //itBaseInfo->second.cbHeadIndex = ptrAndroidParam->cbHeadId;
//            pUserItem->SetUserBaseInfo(itBaseInfo->second);
//        }
//        else
//        {
//            GS_UserBaseInfo userinfo = {0};
//            userinfo.nUserId    = ptrAndroidParam->nUserId;
//            userinfo.nGem       = ptrAndroidParam->nGem;
//            userinfo.nGender    = ptrAndroidParam->nGender;
//            userinfo.nHeadId    = ptrAndroidParam->nHeadId;
//            userinfo.nHeadBoxId = ptrAndroidParam->nHeadBoxId;
//            userinfo.nVipLevel2 = ptrAndroidParam->nVip2;
//            strncpy(userinfo.szNickName, ptrAndroidParam->szNickName, sizeof(userinfo.szNickName));
//            strncpy(userinfo.szLocation, ptrAndroidParam->szLocation, sizeof(userinfo.szLocation));
//            // try to check if the android user score is in the rangle of validate score value.
//            int nMaxScore = (ptrAndroidParam->nTakeMaxScore - ptrAndroidParam->nTakeMinScore);
//            if (!nMaxScore)
//            {
//                nMaxScore = 1;
//            }

//            userinfo.nUserScore = ptrAndroidParam->nTakeMinScore + (rand() % nMaxScore);
//            // try to update the special android user item score value for user.
//            SetAndroidUserScore(ptrAndroidParam->nUserId, userinfo.nUserScore);
//            pUserItem->SetUserBaseInfo(userinfo);

//            m_mapAndroidUserInfo.insert(std::make_pair(userinfo.nUserId, userinfo));
//        }

//        //GS_UserBaseData& baseData = pUserItem->GetUserBaseData();
//        GS_UserScoreInfo scoreinfo = {0};
//        scoreinfo.nUserId    = ptrAndroidParam->nUserId;
//        scoreinfo.nAddScore  = 0;
//        scoreinfo.nRevenue   = 0;
//        scoreinfo.nWinCount  = 0;
//        scoreinfo.nLostCount = 0;
//        scoreinfo.nDrawCount = 0;
//        scoreinfo.nFleeCount = 0;
//        scoreinfo.nPlayTime  = 0;
//        scoreinfo.nEnterTime = (dword)time(NULL);
//        pUserItem->SetUserScoreInfo(scoreinfo);
//        // pUserItem->SetClientNetAddr(0);
//        pUserItem->ResetClientNetAddr();
//        pUserItem->ResetUserConnect();
//        // try to get the special sink item data content value for test.

//        /*
//        IAndroidUserItemSink* pSink = pUserItem->GetAndroidUserItemSink();
//        if (pSink) {
//            pSink->Initialization(pTableFrame, pUserItem);
//        }
//        */


//        m_listAndroidParamStore.pop_front();
//        // room sit chair item data value for later.
//        //pTableFrame->RoomSitChair(pUserItem,false);
//    }
//}


//int CAndroidUserManager::GetRandomHeadBoxIndex(int vip)
//{
//    int headBoxIndex = 1;
//    switch(vip)
//    {
//        case 0:
////            headBoxIndex = rand() % 2;
//            headBoxIndex = GlobalFunc::RandomInt64(0, 1);
//            break;
//        case 1:
//            headBoxIndex = 2;
//            break;
//        case 2:
//            headBoxIndex = 3;
//            break;
//        case 3:
//            headBoxIndex = 4;
//            break;
//        case 4:
//            headBoxIndex = 5;
//            break;
//        case 5:
////            headBoxIndex = 6 + rand() % 3;
//            headBoxIndex = GlobalFunc::RandomInt64(6, 8);
//            break;
//        case 6:
////            headBoxIndex = 9 + rand() % 3;
//            headBoxIndex = GlobalFunc::RandomInt64(9, 11);
//            break;
//        default:
//            break;
//    }
//    return headBoxIndex;
//}

//int CAndroidUserManager::GetRandomNickNameForAndroid(vector<string> &nickNameVec, int num)
//{
//    nickNameVec.clear();

//    try
//    {
//        auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_ACCOUNT), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        int max = 0;
//        pstmt.reset(conn->prepareStatement("SELECT COUNT(*) FROM android_random_nickname"));
//        res.reset(pstmt->executeQuery());
//        if(res->next())
//        {
//            max = res->getUInt(1);
//        }
//        if(max < 1)
//            return 0;
//        else
//            max--;
//        pstmt->close();
//        res->close();
////        int index = rand() % max;
//        int index = GlobalFunc::RandomInt64(0, max);
//        pstmt.reset(conn->prepareStatement("SELECT nickname FROM android_random_nickname LIMIT ?,?"));
//        pstmt->setInt(1, index);
//        pstmt->setInt(2, num);
//        res.reset(pstmt->executeQuery());
//        while(res->next())
//        {
//            nickNameVec.push_back(res->getString(1));
//        }
//    }catch(sql::SQLException &e)
//    {
//        LOG_ERROR << "GetRandomNickNameForAndroid database failed,message:" << e.what();
//    }catch(...)
//    {
//        LOG_ERROR << "GetRandomNickNameForAndroid database unknown error failed.";
//    }
//    return nickNameVec.size();
//}

//int CAndroidUserManager::GetRandomLocationForAndroid(vector<string> &locationVec, int num)
//{
//    locationVec.clear();

//    try
//    {
//        auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_GAME), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        int max = 0;
//        pstmt.reset(conn->prepareStatement("SELECT COUNT(*) FROM android_address"));
//        res.reset(pstmt->executeQuery());
//        if(res->next())
//        {
//            max = res->getUInt(1);
//        }
//        if(max < 1)
//            return 0;
//        else
//            max--;
//        pstmt->close();
//        res->close();
////        int index = rand() % max;
//        int index = GlobalFunc::RandomInt64(0, max);
//        pstmt.reset(conn->prepareStatement("SELECT address FROM android_address LIMIT ?,?"));
//        pstmt->setInt(1, index);
//        pstmt->setInt(2, num);
//        res.reset(pstmt->executeQuery());
//        while(res->next())
//        {
//            locationVec.push_back(res->getString(1));
//        }
//    }catch(sql::SQLException &e)
//    {
//        LOG_ERROR << "GetRandomLocationForAndroid database failed,message:" << e.what();
//    }catch(...)
//    {
//        LOG_ERROR << "GetRandomLocationForAndroid database unknown error failed.";
//    }

//    return locationVec.size();
//}

//string CAndroidUserManager::GetRandomLocationForAndroid()
//{
//    string strAddress;
//    try
//    {
//        auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_GAME), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        int max = 0;
//        pstmt.reset(conn->prepareStatement("SELECT COUNT(*) FROM android_address"));
//        res.reset(pstmt->executeQuery());
//        if(res->next())
//        {
//            max = res->getUInt(1);
//        }
//        if(max < 1)
//            return 0;
//        else
//            max--;
//        pstmt->close();
//        res->close();
////        int index = rand() % max;
//        int index = GlobalFunc::RandomInt64(0, max);
//        LOG_DEBUG << "++++++++++++++++INDEX : " << index << "++++++++++++++++";
//        pstmt.reset(conn->prepareStatement("SELECT address FROM android_address LIMIT ?,1"));
//        pstmt->setInt(1, index);
//        res.reset(pstmt->executeQuery());
//        if(res->next())
//        {
//            strAddress = res->getString(1);
//        }
//    }catch (sql::SQLException &e)
//    {
//        LOG_ERROR << "GetRandomLocationForAndroid database failed,message:" << e.what();
//    }catch (...)
//    {
//        LOG_ERROR << "GetRandomLocationForAndroid database unknown error failed.";
//    }
//    return strAddress;
//}

//shared_ptr<CServerUserItem> CAndroidUserManager::FindAndroidUserItem(dword userid)
//{
//    shared_ptr<CServerUserItem> userItem;
//    READ_LOCK(m_mutex);
//    auto it = m_AndroidUserIdToUserItemMap.find(userid);
//    if  (it != m_AndroidUserIdToUserItemMap.end())
//    {
//        return it->second;
//    }
////Cleanup:
//    return userItem;
//}

//void  CAndroidUserManager::UpdateOnlineCount()
//{
////    LOG_ERROR << "CAndroidUserManager::UpdateOnlineCount NO IMPL yet!";

////    CServerUserManager::get_mutable_instance().UpdateUserCount(0, 3);
//}

//void CAndroidUserManager::SetAndroidUserScore(int64_t userId, int64_t userScore)
//{
//    LOG_ERROR << "CAndroidUserManager::SetAndroidUserScore write to DB NO IMPL yet!";
//    /*
//    MSG_UpdateUser_Score updateScoreToDB;
//    updateScoreToDB.nUserId = nUserId;
//    updateScoreToDB.llAddScore = lUserScore;
//    CNetModule::getSingleton().Send(pNetDB->GetSocket(), MSG_GDB_MAIN, SUB_G2DB_SET_USER_SCORE, &updateScoreToDB, sizeof(updateScoreToDB));
//    */
//}
