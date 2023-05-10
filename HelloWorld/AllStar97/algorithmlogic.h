#ifndef ALGORITHMLOGIC_H
#define ALGORITHMLOGIC_H
#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Logging.h"

#define ALL_FRUIT  11


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

    struct fruitWin
    {
        fruitWin(){}
        int32_t lie;    //在哪一列
        int32_t heng;   //在哪一横
        int32_t fruit;  //是什么水果,财神到或者是本尊
        int32_t index;  //在十五格的那个位置
    };


    struct resultStr
    {
        resultStr()
        {
            fruit = 0;
            mutic = 0;
            count = 0.0;
            isFree = false;
            winScore = 0;
            isWin = false;
            fruitVct.clear();
        }
        void Clear()
        {
            fruit = 0;
            mutic = 0.0;
            count = 0;
            isFree = false;
            winScore = 0;
            isWin =false;
            fruitVct.clear();
        }
        int32_t fruit; //开奖的水果
        double_t mutic; //开奖倍数
        int32_t count; //连续中水果的个数
        bool    isFree;//是否是免费开奖
        bool    isWin;//是否中普通奖
        int64_t winScore;//中奖金额
        vector<fruitWin> fruitVct;

    };
    vector<fruitWin> winFruit[10];

    vector<fruitWin> fiveLieVec[10][5];  //每种水果分成五列放
    vector<resultStr> resultArrVec;
    bool isTrue[10][5];  //记录是不是连续的
    bool isWinFruit[10]; //水果是否中奖


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

    int64_t calculateJettonScore;
    int openResultIndex;


    int64_t currentWinScore;
    int64_t currentJettonScore[10];    //十门动物注项
    int64_t currentFeiQinZouShouJetton[2];//飞禽走兽下注

    int64_t historyWinScore;
    int64_t BigHitStoryScore;

    int64_t RoundTimes;
    int64_t currentMarryWinSocre;


    int32_t m_nWeights[10];

    int32_t marryNum;
    std::vector<BettingGate> vtBettingGate;
    std::vector<Player> vtPlayers;

    std::vector<BettingGate> vtValidItem; //you xiao xiang
    std::vector<BettingGate> vtNotValidItem; //wu xiao  xiang
    shared_ptr <EventLoopThread>     timelooper;


    string m_mongoDBServerAddr;



    int32_t t_table[3][5];      //左面生成的随机图标
    int32_t a_table[15];

    int32_t roundCount;
    int32_t goldSharkCount;
    int32_t silverSharkCount;
    int32_t aniCount[8];
public:
    void ReadPlayDB();

    void CountIsWin();



    void AggregatePlayerBets(); //汇总当天玩家投注

    void ThreadInitCallback();
    bool InItAgorithm();
    void PlayersBetting();
    void Result();
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
