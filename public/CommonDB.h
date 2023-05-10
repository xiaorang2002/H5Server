#ifndef COMMONDB_H
#define COMMONDB_H

#include <stdint.h>
#include <vector>

#include "muduo/base/Mutex.h"
#include "Globals.h"
#include "gameDefine.h"

#include "ThreadLocalSingletonMongoDBClient.h"

using namespace std;
using namespace muduo;

//using boost::serialization::singleton;

enum class eCommonStatus
{
    TaskStatusStart     = 0,
    TaskStatusOngoing   = 1,
    TaskStatusFinished  = 2,
    TaskStatusAwarded   = 3,
};

struct tagDBConfig
{
    int32_t taskId;
    string taskName;
};

class CommonDB //: public singleton<CommonDB>
{
public:
    CommonDB();
    CommonDB(string str);
    ~CommonDB();

    void loadTaskConfigs();
    void loadTaskStatus();
    // bool JiFenChangeRecordToDB(int64_t userId, string account, int32_t agentId, int64_t incrintegral, int32_t before_integral, eJiFenChangeType changetype, string reason);
private:
    // std::map<int64_t, std::map<int32_t, vector<tagUserTaskInfo>>> m_tasksRecord;
    // std::map<int32_t, tagTaskConfig> task_configs;
    MutexLock m_mutex;
    MutexLock m_mutex_ids;
    MutexLock m_mutex_tasks;

public:
    int32_t status;
    int32_t task_amount;

    int64_t refresh_time;
    bool m_isUpdate;
    vector<int32_t> task_ids;  //
    vector<int32_t> agent_ids; //禁止显示的白名单

    int32_t month_amount;
    int32_t weekly_amount;
    vector<int32_t> weeklytasks; //周任务
};
 
#endif // COMMONDB_H
