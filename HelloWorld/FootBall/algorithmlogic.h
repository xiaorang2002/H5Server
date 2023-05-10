#ifndef ALGORITHMLOGIC_H
#define ALGORITHMLOGIC_H
#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Logging.h"

#define ALL_FRUIT  12

//行
#define Table_Row_Num	3
//列
#define	Table_Col_Num	5

#define  IMAGE_10               0		//10
#define  IMAGE_J                1		//J
#define  IMAGE_Q                2		//Q
#define  IMAGE_K                3		//K
#define  IMAGE_A                4		//A
#define  IMAGE_Jersey           5		//球衣
#define  IMAGE_Gloves           6		//手套
#define  IMAGE_Stopwatch        7		//秒表
#define  IMAGE_GoldenBoots		8		//金靴
#define  IMAGE_GoldenBall		9		//金球
#define  IMAGE_GoldenCup		10      //金杯
#define	 IMAGE_FootBall         11		//可变足球

#define	 FRUIT_MAX              12

#define	 LINE_MAX	25

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
    vector<fruitWin> winFruit[ALL_FRUIT];

    vector<fruitWin> fiveLieVec[10][5];  //每种水果分成五列放
    vector<resultStr> resultArrVec;
    bool isTrue[10][5];  //记录是不是连续的
    bool isWinFruit[10]; //水果是否中奖


    double totalPushIn;
    double totalGetOut;
    double thisPushIn;

    int  upperLimit;   // 上限
    int  lowerLimit;   // 下限

    double totalNormalIn;
    double totalNormalWin;
    double totalMarryWin;
    double totalMarryIn;



    int notUsebleGateIndex;
    int usebleGateIndex;

    int64_t calculateJettonScore;
    int openResultIndex;


    double currentWinScore;
    int64_t currentJettonScore[1];    //十门动物注项
    int64_t currentFeiQinZouShouJetton[2];//飞禽走兽下注

    int64_t historyWinScore;
    int64_t BigHitStoryScore;

    int64_t RoundTimes;
    int64_t currentMarryWinSocre;


    int32_t m_nWeights[ALL_FRUIT];

    int32_t marryNum;
    std::vector<BettingGate> vtBettingGate;
    std::vector<Player> vtPlayers;

    std::vector<BettingGate> vtValidItem; //you xiao xiang
    std::vector<BettingGate> vtNotValidItem; //wu xiao  xiang
    shared_ptr <EventLoopThread>     timelooper;


    string m_mongoDBServerAddr;


    int32_t bigBounsNum ;
    int32_t t_table[3][5];      //左面生成的随机图标
    int32_t a_table[15];
    int32_t last_table[3][5];
    int32_t allFootballNum;

    int32_t roundCount;
    int32_t goldSharkCount;
    int32_t silverSharkCount;
    int32_t aniCount[8];
    bool    isFree;
    int32_t freeRound;
    int32_t biggerNum;
public:
    int	 IsSame(int First, int Second);
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
