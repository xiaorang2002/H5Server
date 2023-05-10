#include "TaskService.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include "muduo/base/Logging.h"
#include "muduo/base/TimeZone.h"
//#include "RedisClient/RedisClient.h"


#include "GlobalFunc.h"
#include "GameGlobalFunc.h"
#include "ThreadLocalSingletonMongoDBClient.h"

#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"



using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

using namespace std;

TaskService::TaskService()
{
    status = 0;
    task_amount = 0;
    m_isUpdate = true;
    task_ids.clear();
    weeklytasks.clear();
    monthtasks.clear();
    agent_ids.clear();
    m_tasksRecord.clear();
	CurrencyTask_ids.clear();
	CurrencyWeeklytasks.clear();
	CurrencyMonthtasks.clear();
	for (int32_t i = 0;i <= 5;i++)
	{
		m_isUpdate_map[i] = true;
		refresh_time_map[i] = 0;
	}
	
}

void TaskService::loadTaskStatus(int32_t currency)
{
    if ( !m_isUpdate_map[currency]){
        return;
    }
    // 清理缓存
    m_tasksRecord.clear();
    // 更新状态
	m_isUpdate_map[currency] = false;

    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_task_info"];
        bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(bsoncxx::builder::stream::document{} << "currency" << currency << finalize);
        if(result)
        {
            // LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(result->view());
            bsoncxx::document::view view = result->view();
            int64_t t_refresh_time = view["refresh_time"].get_int64();
            if( t_refresh_time == refresh_time_map[currency] || t_refresh_time - refresh_time_map[currency] < 10)
            {
                LOG_ERROR << "refresh too frequent.";
                return;
            }
			refresh_time_map[currency] = t_refresh_time;
            // 状态
            status = view["status"].get_int32();
            // 总数
            task_amount = view["daily_amount"].get_int32();
            // 日任务
            auto t_task_ids = view["tasks"].get_array();
            // 周任务
            auto t_week_task_ids = view["weeklytasks"].get_array();
            // 月任务
            auto t_month_task_ids = view["monthtasks"].get_array();
            LOG_WARN << "-------shield get_array-------1-" << "currency" << currency;
            auto t_agent_ids = view["shield"].get_array();
            LOG_WARN << "-------shield get_array-------2-" << "currency" << currency;
            {
                MutexLockGuard lock(m_mutex_ids);
                //task_ids.clear();
				if (CurrencyTask_ids.count(currency))
				{
					CurrencyTask_ids[currency].clear();
				}
                for (auto &task_id : t_task_ids.value)
                {
                    task_ids.push_back(task_id.get_int32());
					CurrencyTask_ids[currency].push_back(task_id.get_int32());
                }
                // 增加代理ID
                agent_ids.clear();
                for (auto &agent_id : t_agent_ids.value)
                {
                    agent_ids.push_back(agent_id.get_int32());
                    LOG_DEBUG << "agent_ids:" << agent_id.get_int32();
                }

                // 周任务
                //weeklytasks.clear();
				if (CurrencyWeeklytasks.count(currency))
				{
					CurrencyWeeklytasks[currency].clear();
				}
                for (auto &weektask_id : t_week_task_ids.value)
                {
                    weeklytasks.push_back(weektask_id.get_int32());
					CurrencyWeeklytasks[currency].push_back(weektask_id.get_int32());
                }

                // 月任务
                //monthtasks.clear();
				if (CurrencyMonthtasks.count(currency))
				{
					CurrencyMonthtasks[currency].clear();
				}
                for (auto &monthtask_id : t_month_task_ids.value)
                {
                    monthtasks.push_back(monthtask_id.get_int32());
					CurrencyMonthtasks[currency].push_back(monthtask_id.get_int32());
                }
            }
        } 
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << e.what();
    }

    // 加载所有任务配置表
    loadTaskConfigs();
}

// 设置
void TaskService::setUpdateTask(int32_t currency, bool isUpdate)
{
    m_isUpdate = isUpdate;
	m_isUpdate_map[currency] = isUpdate;
}

// 判断该代理是否允许开放
// 返回true为允许开放
bool TaskService::isPermitShowTask(int32_t _agentId)
{
    MutexLockGuard lock(m_mutex_ids);
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

// 游戏日常任务静态配置表
void TaskService::loadTaskConfigs()
{
    try
    {
        int32_t taskCount = 0;
        MutexLockGuard lock(m_mutex);
        task_configs.clear();
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_task"];
        mongocxx::cursor cursor = coll.find({});
        for (auto &doc : cursor)
        {
            tagTaskConfig taskConfig;
            taskConfig.taskId = doc["task_id"].get_int32();
            taskConfig.taskName = doc["task_name"].get_utf8().value.to_string();
            taskConfig.taskType = doc["task_type"].get_int32();
            taskConfig.taskQty = doc["task_quantity"].get_int64();
            taskConfig.taskOrder = doc["task_order"].get_int32();
            taskConfig.taskGameId = doc["task_game_id"].get_int32();
            taskConfig.taskRoomId = doc["task_room_id"].get_int32();
            taskConfig.taskAwardGold = doc["task_award_gold"].get_int64();
            taskConfig.taskAwardScore = doc["task_award_score"].get_int64();

            if( doc["task_cycle"] && doc["task_cycle"].type() == bsoncxx::type::k_int32 )
                taskConfig.taskCycle = doc["task_cycle"].get_int32();

			if (doc["currency"])
			{
				taskConfig.taskCurrency = doc["currency"].get_int32();
			}
            //LOG_INFO << "---[" << (++taskCount) << "]taskGameId[" << taskConfig.taskGameId << "],taskRoomId[" << taskConfig.taskRoomId << "],taskId["
            //            << taskConfig.taskId << "],taskCycle[" << taskConfig.taskCycle << "],taskName[" << taskConfig.taskName << "]";

            task_configs[taskConfig.taskId] = taskConfig;
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << e.what();
    }
}

//根据游戏ID获取用户任务(所有玩家任务一样)
void TaskService::getUserTasksByGameId(int64_t userId,map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t gameId, int32_t roomId, int32_t currency)
{
    int64_t findkey = (gameId << 32) | roomId;
    // 查找临时值
    std::map<int64_t, std::map<int32_t, vector<tagUserTaskInfo>>>::const_iterator it = m_tasksRecord.find(findkey);
	if (it != m_tasksRecord.end() ){
        tasks = it->second;
        LOG_WARN << "---"<<__FUNCTION__<< " 找到缓存 gameId[" << gameId << "],roomId[" << roomId << "]";
        return;
	}

    MutexLockGuard lock(m_mutex_ids);
    map<int32_t, vector<tagUserTaskInfo>> tasksTmp;
    // 给玩家增加3条记录
    for (int32_t taskCycleIdx = 0; taskCycleIdx < 3; taskCycleIdx++)
    {
        vector<int32_t> _task_ids;
        getTasksListById(taskCycleIdx, _task_ids, currency);

        tagUserTaskInfo taskInfo;
        vector<tagUserTaskInfo> taskslist;
        for (int i = 0; i < _task_ids.size(); i++)
        {
            tagTaskConfig task_config = task_configs[_task_ids[i]];

            taskInfo.taskGameId = task_config.taskGameId;
            taskInfo.taskRoomId = task_config.taskRoomId;

            // LOG_WARN << "--- "<<__FUNCTION__<< " "<< taskCycleIdx<< " gameId[" << gameId << "],roomId[" << roomId << "] taskGameId[" << task_config.taskGameId <<"] taskRoomId["<< task_config.taskRoomId<<"]";

            if (gameId != 0 && roomId != 0)
            {
                if (task_config.taskGameId != gameId) //非本游戏不处理
                    continue;

                if (task_config.taskRoomId != 0 && task_config.taskRoomId != roomId) //非本房间不处理
                    continue;

                LOG_WARN << "---"<<__FUNCTION__<< " gameId[" << gameId << "],roomId[" << roomId << "],taskId["<<task_config.taskId<<"]";
            }

            taskInfo.taskId = task_config.taskId;
            taskInfo.taskName = task_config.taskName;
            taskInfo.taskType = task_config.taskType;
            taskInfo.taskStatus = TaskStatusOngoing;
            taskInfo.taskQty = task_config.taskQty;
            taskInfo.taskOrder = task_config.taskOrder;
            taskInfo.taskProgress = 0;
            taskInfo.taskAwardGold = task_config.taskAwardGold;
            taskInfo.taskAwardScore = task_config.taskAwardScore;
			taskInfo.taskCurrency = task_config.taskCurrency;
            taskslist.push_back(taskInfo);
        }

        //循环读取
        tasksTmp[taskCycleIdx] = taskslist;
    }

    // 赋值
    tasks = tasksTmp;

    // 临时记录
    m_tasksRecord[findkey] = tasksTmp;
    LOG_WARN << "---"<<__FUNCTION__<< " 新增记录 gameId[" << gameId << "],roomId[" << roomId << "],count["<< m_tasksRecord.size() <<"]" ;
}
 
// create User Tasks By CycleIdx
bool TaskService::createUserTasksByCycleIdx(int32_t cycleType, int64_t userId, mongocxx::collection & coll, map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t currency)
{
    bsoncxx::builder::stream::document builder{};
    builder << "userId" << userId;
    builder << "cycle" << cycleType;
    auto insert_value = builder << "tasks" << open_array;
    {
        MutexLockGuard lock(m_mutex_ids);

        vector<int32_t> _task_ids;
        getTasksListById(cycleType, _task_ids, currency);

        // 遍历 game_task_info 表
        for (int i = 0; i < _task_ids.size(); i++)
        {
            // 从 game_task 表中查找 task_id,如果没有找到则这条任务不被添加到 game_user_task 个人表中
            if (task_configs.find(_task_ids[i]) != task_configs.end())
            {
                // 组织文档
                tagTaskConfig task_config = task_configs[_task_ids[i]];
                auto insert_value_1 = insert_value << bsoncxx::builder::stream::open_document
                                                   << "task_id" << task_config.taskId
                                                   << "task_game_id" << task_config.taskGameId
                                                   << "task_name" << task_config.taskName
                                                   << "task_type" << task_config.taskType
                                                   << "status" << TaskStatusOngoing
                                                   << "progress" << (int64_t)0
                                                   << "task_quantity" << task_config.taskQty
                                                   << "task_order" << task_config.taskOrder
                                                   << "task_room_id" << task_config.taskRoomId
                                                   << "task_award_score" << task_config.taskAwardScore
                                                   << "task_award_gold" << task_config.taskAwardGold
												   << "currency" << task_config.taskCurrency
                                                   << close_document;
            }
            else
            {
                LOG_ERROR << "Cannot find taskid: [" << (int)_task_ids[i] << "] for user:" << (int)userId;
            }
        }
    }
    insert_value << close_array;
    bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(builder.view());
    if (_result)
    {
        MutexLockGuard lock(m_mutex_ids);

        vector<int32_t> _task_ids;
        getTasksListById(cycleType, _task_ids, currency);

        //填充返回给玩家
        tagUserTaskInfo taskInfo;
        vector<tagUserTaskInfo> taskslist;
        for (int i = 0; i < _task_ids.size(); i++)
        {
            tagTaskConfig task_config = task_configs[_task_ids[i]];
            taskInfo.taskId = task_config.taskId;
            taskInfo.taskGameId = task_config.taskGameId;
            taskInfo.taskName = task_config.taskName;
            taskInfo.taskType = task_config.taskType;
            taskInfo.taskStatus = TaskStatusOngoing;
            taskInfo.taskQty = task_config.taskQty;
            taskInfo.taskOrder = task_config.taskOrder;
            taskInfo.taskRoomId = task_config.taskRoomId;
            taskInfo.taskProgress = 0;
            taskInfo.taskAwardGold = task_config.taskAwardGold;
            taskInfo.taskAwardScore = task_config.taskAwardScore;
			taskInfo.taskCurrency = task_config.taskCurrency;
            taskslist.push_back(taskInfo);
        }

        //循环读取
        tasks[cycleType] = taskslist;
    }

    return true;
}

//获取用户任务
void TaskService::getUserTasks(int64_t userId,map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t currency, int32_t gameId, int32_t roomId)
{
    //特殊处理新水果机和经典水果公用数据问题
    if (gameId == (int32_t)eGameKindId::xsgj){
        gameId = (int32_t)eGameKindId::sgj;
    }
    if (roomId == XSGJ_ROOM_ID){
        roomId = SGJ_ROOM_ID;
    }
	
    // 加载任务列表
    loadTaskStatus(currency);
    if(status == 0 || task_amount == 0)
    {
        LOG_ERROR << "---getUserTasks() status == 0 or task_amount == 0 userId[" << userId << "],status["<<status<<"],task_amount["<<task_amount<<"]";
        return;
    }

    try
    {
        MutexLockGuard lock(m_mutex_tasks);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user_task"];
        auto query_value = bsoncxx::builder::stream::document{} << "userId" << userId << finalize; //"cycle" << 0 <<

        //初始化
        map<int32_t, bool> mapCycle{{0, false}, {1, false}, {2, false}};

        mongocxx::cursor cursor = coll.find(query_value.view());
        for (auto &taskdoc : cursor)
        {
            if (!taskdoc["cycle"] || taskdoc["cycle"].type() != bsoncxx::type::k_int32)
            {
                LOG_ERROR << "game_user_task cycle is nullptr or type is error ! userId[" << userId << "]";
                continue;
            }

            int32_t cycleType = taskdoc["cycle"].get_int32();
            auto arr = taskdoc["tasks"].get_array();

            std::map<int32_t, bool>::iterator itor = mapCycle.find(cycleType);
            if (itor != mapCycle.end())
            {
                itor->second = true;
            }

            //arr.value.length();//The length of the array, in bytes. To compute the number of elements, use std::distance.
            int32_t taskCounts = std::distance(arr.value.begin(), arr.value.end());

            LOG_INFO << "---cycleType[" << cycleType << "],taskCounts[" << taskCounts << "],gameId[" << gameId << "],roomId[" << roomId << "]" << " currency=" << currency;

            tagUserTaskInfo taskInfo;
            vector<tagUserTaskInfo> taskslist;

            for (auto &doc : arr.value)
            {
                taskInfo.taskGameId = doc["task_game_id"].get_int32();
                taskInfo.taskRoomId = doc["task_room_id"].get_int32();

                if (gameId != 0 && roomId != 0)
                {
                    if (taskInfo.taskGameId != gameId) //非本游戏不处理
                        continue;

                    if (taskInfo.taskRoomId != 0 && taskInfo.taskRoomId != roomId) //非本房间不处理
                        continue;

                    // LOG_INFO << "gameId[" << gameId << "],roomId[" << roomId << "]";
                }
				if (doc["currency"])
				{
					taskInfo.taskCurrency = doc["currency"].get_int32();
					if (taskInfo.taskCurrency != currency)
					{
						LOG_WARN << "game_user_task 当前任务币种不是玩家自己的 ! userId[" << userId << "]" << " currency=" << currency << " taskInfo.taskCurrency=" << taskInfo.taskCurrency;
						continue;
					}
				}
                taskInfo.taskId = doc["task_id"].get_int32();
                taskInfo.taskName = doc["task_name"].get_utf8().value.to_string();
                taskInfo.taskType = doc["task_type"].get_int32();
                taskInfo.taskStatus = doc["status"].get_int32();
                taskInfo.taskQty = doc["task_quantity"].get_int64();
                taskInfo.taskOrder = doc["task_order"].get_int32();
                taskInfo.taskProgress = doc["progress"].get_int64();
                taskInfo.taskAwardGold = doc["task_award_gold"].get_int64();
                taskInfo.taskAwardScore = doc["task_award_score"].get_int64();				
                //
                taskslist.push_back(taskInfo);
            }

            //循环读取
            tasks[cycleType] = taskslist;
        }

        // 检查任务是否存在
        for (std::map<int32_t, bool>::const_iterator it = mapCycle.begin(); it != mapCycle.end(); ++it)
        {
            if (it->second == false)
            {
                createUserTasksByCycleIdx(it->first, userId, coll, tasks, currency);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << e.what();
    }
}

// 更新任务
int TaskService::updateTaskProgress(int64_t userId, int32_t taskId, int64_t progress)
{
    try
    {
        mongocxx::collection user_task_coll = MONGODBCLIENT["gamemain"]["game_user_task"];
        bsoncxx::document::value query_value = bsoncxx::builder::stream::document{} << "userId" << userId
                                                                                    << "tasks.task_id" << taskId << finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> _result = user_task_coll.find_one(query_value.view(), mongocxx::options::find{}.projection(make_document(kvp("tasks.$", 1))));
        if (_result)
        {
            bsoncxx::document::view view = _result->view();
            // LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(_result->view());
            auto tasks = view["tasks"].get_array();
            for (auto &taskDoc : tasks.value)
            {
                if (taskDoc["task_id"].get_int32() != taskId)
                    continue;

                int32_t tstatus = taskDoc["status"].get_int32();
                if (tstatus == TaskStatusOngoing) //progressing
                {
                    int64_t beforeProgress = taskDoc["progress"].get_int64();
                    int64_t total = taskDoc["task_quantity"].get_int64();
                    int64_t after_progress = beforeProgress + progress;
                    if (after_progress >= total)
                    {
                        after_progress = total;
                        user_task_coll.update_one(bsoncxx::builder::stream::document{} << "userId" << userId << "tasks.task_id" << taskId << finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << open_document << "tasks.$.status" << TaskStatusFinished
                                                                                       << "tasks.$.progress" << after_progress << close_document << finalize);
                    }
                    else
                    {
                        user_task_coll.update_one(bsoncxx::builder::stream::document{} << "userId" << userId << "tasks.task_id" << taskId << finalize,
                                                  bsoncxx::builder::stream::document{} << "$inc" << open_document << "tasks.$.progress" << progress << close_document << finalize);
                    }
                }
            }
        }
        else
            LOG_ERROR << "can't fount task [" << (int)taskId << "] for user: " << (int)userId;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << e.what();
    }

    return 0;
}

// 领取任务记录(帐变记录)
void TaskService::AddTaskRecord(int64_t userId, int32_t agentId ,string &linecode,string &account, string &roundId,int64_t beforegold,int64_t beforescore, tagUserTaskInfo & taskInfo)
{
    try
    { 
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                << "gameinfoid" << roundId
                << "userid" << userId
                << "account" << account
                << "agentid" << agentId
                << "linecode" << linecode
                << "task_type" << taskInfo.taskType
                << "task_id" << taskInfo.taskId
                << "task_name" << taskInfo.taskName
                << "task_quantity" << taskInfo.taskQty
                << "gameid" << taskInfo.taskGameId
                << "roomid" << (taskInfo.taskRoomId != 0 ? taskInfo.taskRoomId : taskInfo.taskGameId*10+1)
                << "finish_time" << bsoncxx::types::b_date(chrono::system_clock::now())
                << "task_award_score" << taskInfo.taskAwardScore
                << "task_award_gold" << taskInfo.taskAwardGold
			    << "currency" << taskInfo.taskCurrency
                << "changerecord" << open_array;
        if(taskInfo.taskAwardGold != 0)
        {
            auto insert_value_1 = insert_value << open_document
                                            << "beforevalue" << beforegold
                                            <<  "aftervalue" << (taskInfo.taskAwardGold+beforegold)
                                            << close_document;
        }
        if(taskInfo.taskAwardScore != 0)
        {
            auto insert_value_1 = insert_value << open_document
                                            << "beforevalue" << beforescore
                                            << "aftervalue" << (taskInfo.taskAwardScore*50000+beforescore)//yes, i am fucked
                                            << close_document;
        }
        auto after_value = insert_value << close_array;
        auto doc = after_value << finalize;
        mongocxx::collection user_task_record = MONGODBCLIENT["gamemain"]["play_record_task"];
        user_task_record.insert_one(doc.view());

        if(taskInfo.taskAwardGold != 0)
        {
            mongocxx::collection user_score_coll = MONGODBCLIENT["gamemain"]["user_score_record"];
            auto insert_record_value = bsoncxx::builder::stream::document{} << "userid" << userId << "account" << account << "agentid" << agentId
                                                << "changetype" << TASK_SCORE_CHANGE_TYPE << "changemoney" << taskInfo.taskAwardGold
                                                << "beforechange" << beforegold << "afterchange" << taskInfo.taskAwardGold+beforegold
                                                << "linecode" << linecode
                                                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now()) << finalize;
            user_score_coll.insert_one(insert_record_value.view());
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "================"<< __FUNCTION__<< " " << e.what();
    }
}

// 任务系统更新内容
void TaskService::updateTaskSystem(int64_t userId, int32_t agentId,int64_t addScore,int64_t rWinScore, uint32_t gameId, uint32_t roomId,bool isMatch)
{
    LOG_INFO << "---updateTaskSystem[" << userId << "],agentId["<<agentId<<"],addScore["<<addScore<<"],rWinScore["<<rWinScore<<"]";

    //特殊处理新水果机和经典水果公用数据问题
    if (gameId == (int32_t)eGameKindId::xsgj){
        gameId = (int32_t)eGameKindId::sgj;
    }
    if(gameId ==(int32_t)eGameKindId::lkpy )
    {
        gameId = (int32_t)eGameKindId::jcfish;
    }
    if(gameId ==(int32_t)eGameKindId::rycs )
    {
        gameId = (int32_t)eGameKindId::jcfish;
    }
    if(gameId ==(int32_t)eGameKindId::dntg )
    {
        gameId = (int32_t)eGameKindId::jcfish;
    }
    if (roomId == XSGJ_ROOM_ID){
        roomId = SGJ_ROOM_ID;
    }
    /////////
    if (roomId == LKPY_ROOM1_ID){
        roomId = JCBY_ROOM1_ID;
    }
    if (roomId == DNTG_ROOM1_ID){
        roomId = JCBY_ROOM1_ID;
    }
    if (roomId == RYCS_ROOM1_ID){
        roomId = JCBY_ROOM1_ID;
    }
    ////////////
    if (roomId == LKPY_ROOM2_ID){
        roomId = JCBY_ROOM2_ID;
    }
    if (roomId == DNTG_ROOM2_ID){
        roomId = JCBY_ROOM2_ID;
    }
    if (roomId == RYCS_ROOM2_ID){
        roomId = JCBY_ROOM2_ID;
    }
    //////////
    if (roomId == LKPY_ROOM3_ID){
        roomId = JCBY_ROOM3_ID;
    }
    if (roomId == DNTG_ROOM3_ID){
        roomId = JCBY_ROOM3_ID;
    }
    if (roomId == RYCS_ROOM3_ID){
        roomId = JCBY_ROOM3_ID;
    }
    ////
    if (roomId == LKPY_ROOM4_ID){
        roomId = JCBY_ROOM4_ID;
    }
    if (roomId == DNTG_ROOM4_ID){
        roomId = JCBY_ROOM4_ID;
    }
    if (roomId == RYCS_ROOM4_ID){
        roomId = JCBY_ROOM4_ID;
    }

	int32_t currency = 0;
	mongocxx::collection user_coll = MONGODBCLIENT["gamemain"]["game_user"];
	bsoncxx::stdx::optional<bsoncxx::document::value> user_result = user_coll.find_one(bsoncxx::builder::stream::document{} << "userid" << userId << finalize);
	if (user_result)
	{
		auto view = user_result->view();
		//增加多币数支持
		if (view["currency"])
			currency = view["currency"].get_int32();
	}
    // 初始化加载
    loadTaskStatus(currency);

    // 判断应该代理是否允许开放
    if (isPermitShowTask(agentId) == false){
        return ;
    }

    // 取出玩家任务
    std::map<int32_t, vector<tagUserTaskInfo>> ttasks;
    getUserTasksByGameId(userId,ttasks,gameId, roomId, currency);

    // 根据任务更新数据库
    for (map<int32_t, vector<tagUserTaskInfo>>::const_iterator it = ttasks.begin(); it != ttasks.end(); ++it)
    {
        uint32_t _taskType = it->first;
        LOG_INFO << "---cycle Type[" << _taskType << "],isMatch[" << isMatch <<"],gameId[" << gameId<<"],roomId["<<roomId<<"]";

        for (auto task : it->second)
        {
            switch (task.taskType)
            {
            case TASK_FIRST_WIN:
                if ( !isMatch && addScore > 0 ) updateTaskProgress(userId, task.taskId, 1);
                if ( !isMatch && addScore > 0 ) LOG_INFO << "---TASK_FIRST_WIN[" << userId << "],taskId[" << task.taskId <<"],taskName[" << task.taskName<<"]";
                break;
            case TASK_PARTI_MTH:
                if( isMatch ) updateTaskProgress(userId, task.taskId, 1); 
                if ( isMatch ) LOG_INFO << "---TASK_PARTI_MTH[" << userId << "],taskId[" << task.taskId <<"],taskName[" << task.taskName<<"]";
                break;
            case TASK_TOTAL_GAME_TIMES:
                if( !isMatch ) updateTaskProgress(userId, task.taskId, 1);
                if ( !isMatch ) LOG_INFO << "---TASK_TOTAL_GAME_TIMES[" << userId << "],taskId[" << task.taskId <<"],taskName[" << task.taskName<<"]";
                break;
            case TASK_TOTAL_BET:
                if ( !isMatch && rWinScore > 0 ) updateTaskProgress(userId, task.taskId, rWinScore);
                if ( !isMatch && rWinScore > 0 ) LOG_INFO << "---TASK_TOTAL_BET[" << userId << "],taskId[" << task.taskId <<"],rWinScore["<<rWinScore<<"],taskName[" << task.taskName<<"]";
                break;
            case TASK_TOTAL_WIN_TIMES:
                if ( !isMatch && addScore > 0 ) updateTaskProgress(userId, task.taskId, 1);
                if ( !isMatch && addScore > 0 ) LOG_INFO << "---TASK_TOTAL_WIN_TIMES[" << userId << "],taskId[" << task.taskId <<"],addScore["<<addScore<<"],taskName[" << task.taskName<<"]";
                break;
            default:
                LOG_ERROR << "unknown task_type : " << (int)task.taskType << ", task_name:" << task.taskName;
                break;
            }
        }
    }
}
