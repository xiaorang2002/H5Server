#ifndef TASKSERVICE_H
#define TASKSERVICE_H

#include <stdint.h>
#include <vector>
#include <boost/serialization/singleton.hpp>
//#include <boost/circular_buffer.hpp>
//#include <boost/unordered_set.hpp>
//#include <boost/version.hpp>
//#include <boost/random.hpp>
//#include <boost/thread.hpp>

#include "muduo/base/Mutex.h"
#include "Globals.h"
#include "gameDefine.h"

#include "ThreadLocalSingletonMongoDBClient.h"

using namespace std;
using namespace muduo;

using boost::serialization::singleton;

enum eTaskType
{
    TASK_TOTAL_WIN_TIMES    = 1,//total win game times
    TASK_TOTAL_GAME_TIMES   = 2,//total play game times
    TASK_FIRST_WIN          = 3,//first win in game
    TASK_TOTAL_BET          = 4,//total bet in game
    TASK_PARTI_MTH          = 5,//participate in match
};

enum eTaskStatus
{
    TaskStatusStart     = 0,
    TaskStatusOngoing   = 1,
    TaskStatusFinished  = 2,
    TaskStatusAwarded   = 3,
};

struct  tagTaskConfig
{
    int32_t taskId;
    string taskName;
    int32_t taskType;
    int32_t taskStatus;//task is open
    int64_t taskAwardGold;
    int64_t taskAwardScore;
    int64_t taskQty;
    int32_t taskOrder;
    int32_t taskGameId;
    int32_t taskRoomId; 
    int32_t taskCycle;
	int32_t taskCurrency;
};

struct tagUserTaskInfo
{
    int32_t taskId;
    string taskName;
    int32_t taskType;
    int32_t taskStatus;//task current status: ongoing 0,finished 1,rewarded 2
    int64_t taskAwardGold;
    int64_t taskAwardScore;
    int64_t taskQty;
    int32_t taskOrder;
    int32_t taskGameId;
    int32_t taskRoomId;
	int64_t taskProgress;
	int32_t taskCurrency;
};

class TaskService : public singleton<TaskService>
{
public:
    TaskService();

    void loadTaskConfigs();
    void loadTaskStatus(int32_t currency = 0);
    // 无状态的玩家任务
    void getUserTasksByGameId(int64_t userId,map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t gameId, int32_t roomId, int32_t currency = 0);
    void getUserTasks(int64_t userId,map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t currency = 0, int32_t gameId = 0, int32_t roomId = 0);
    void updateTaskSystem(int64_t userId, int32_t agentId,int64_t addScore,int64_t rWinScore, uint32_t gameId, uint32_t roomId,bool isMatch = false);
    int updateTaskProgress(int64_t userId, int32_t taskId, int64_t progress);
 
    bool GetTaskInfo(int32_t task_id, int64_t userId, tagUserTaskInfo &taskInfo);
     
    void AddTaskRecord(int64_t userId, int32_t agentId ,string &linecode, string &account, string &roundId, int64_t beforeScore,int64_t before_intescore,tagUserTaskInfo & takInfo);
    bool isPermitShowTask(int32_t _agentId);
    void setUpdateTask(int32_t currency, bool isUpdate = true);

    bool createUserTasksByCycleIdx(int32_t cycleType , int64_t userId,mongocxx::collection & coll, map<int32_t, vector<tagUserTaskInfo>> &tasks, int32_t currency = 0);
  
    void getTasksListById(int32_t type , vector<int32_t> &tasksList,int32_t currency){
        if(type == 0 ){
            //tasksList = task_ids;
			tasksList = CurrencyTask_ids[currency];
        }
        else if(type == 1){
            //tasksList = weeklytasks;
			tasksList = CurrencyWeeklytasks[currency];
        }
        else{
            //tasksList = monthtasks;
			tasksList = CurrencyMonthtasks[currency];
        }
    }
private:
    std::map<int64_t, std::map<int32_t, vector<tagUserTaskInfo>>> m_tasksRecord;
    std::map<int32_t,tagTaskConfig>          task_configs;
    MutexLock m_mutex;
    MutexLock m_mutex_ids;
    MutexLock m_mutex_tasks;

public:
    int32_t status;
    int32_t task_amount;
    
	std::map<int32_t, int64_t> refresh_time_map;
    bool    m_isUpdate;
    vector<int32_t> task_ids;       //
    vector<int32_t> agent_ids;      //禁止显示的白名单

    int32_t month_amount;
    int32_t weekly_amount;
    vector<int32_t> weeklytasks;    //周任务
    vector<int32_t> monthtasks;     //月任务

	std::map<int32_t, bool>				  m_isUpdate_map;
	std::map<int32_t, vector<int32_t>>    CurrencyTask_ids;
	std::map<int32_t, vector<int32_t>>    CurrencyWeeklytasks;
	std::map<int32_t, vector<int32_t>>    CurrencyMonthtasks;
};

#endif // TASKSERVICE_H
