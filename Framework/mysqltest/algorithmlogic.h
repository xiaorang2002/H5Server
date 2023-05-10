#ifndef ALGORITHMLOGIC_H
#define ALGORITHMLOGIC_H
#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Logging.h"




#include <vector>
using namespace std;
using namespace muduo;
using namespace muduo::net;
class AlgorithmLogic
{
public:
    AlgorithmLogic(int32_t count,int32_t timedis);
    ~AlgorithmLogic();

    shared_ptr <EventLoopThread>     timelooper;


    string m_mongoDBServerAddr;
    int32_t numRount;
    int32_t timeDis;
public:
    void ReadPlayDB();




    void AggregatePlayerBets(); //汇总当天玩家投注

    void ThreadInitCallback();
    bool InItAgorithm();
    void PlayersBetting();
    void Result();
    void Result1();
    void ClearDate();
    void WtiteUserPlayRecordMysqlx(string mysqlStr);
    bool startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize);
    inline int RandomInt(int min,int max)
    {
        if(min>max)
        {
            int res=min;
            min=max;
            max=res;
        }
        if(min==max)
        {
            return min;
        }
        return min+rand()%(max-min)+1;
    }
    inline int RandomFloat(float min,float max)
    {
        if(min>max)
        {
            float res=min;
            min=max;
            max=res;
        }
       return min+rand()/RAND_MAX+(max-min);
    }
};

#endif // ALGORITHMLOGIC_H
