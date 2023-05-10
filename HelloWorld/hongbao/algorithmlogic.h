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
    AlgorithmLogic();
    ~AlgorithmLogic();
    struct BettingGate
    {
        BettingGate()
        {
            probability=0.0;
        }
        float multiple;
        bool  isEffecive;
        int   index;
        long  jettom;
        double probability;
    };
    struct Player
    {
        Player() {}
        int userID;
        long userPushIn;
        long userGetOut;
        long jettons[8];
    };

    long totalPushIn;
    long totalGetOut;
    long thisPushIn;

    int  upperLimit;   // 上限
    int  lowerLimit;   // 下限

    int64_t totalNormalIn;
    int64_t totalNormalWin;
    int64_t totalMarryWin;
    int64_t totalMarryIn;



    int notUsebleGateIndex;
    int usebleGateIndex;

    int64_t calculateJettonScore[16];
    int openResultIndex;


    int64_t currentWinScore;
    int64_t currentJettonScore;

    int64_t historyWinScore;
    int64_t BigHitStoryScore;

    int64_t RoundTimes;
    int64_t currentMarryWinSocre;


    int32_t m_nWeights[18];

    int32_t marryNum;
    std::vector<BettingGate> vtBettingGate;
    std::vector<Player> vtPlayers;

    std::vector<BettingGate> vtValidItem; //you xiao xiang
    std::vector<BettingGate> vtNotValidItem; //wu xiao  xiang
    shared_ptr <EventLoopThread>     timelooper;
public:

    void ThreadInitCallback();
    bool InItAgorithm();
    void PlayersBetting();

    void GetResultArr(int32_t arr[],int32_t num,int64_t monye);
    void Result(int32_t playerNum,int32_t res[],int64_t monye,int32_t minMonye);
    void Result1();
    void ClearDate();
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
