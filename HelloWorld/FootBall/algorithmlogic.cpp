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

#include "public/ThreadLocalSingletonMongoDBClient.h"

#define MONGODBCLIENT ThreadLocalSingletonMongoDBClient::instance()

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

#include "algorithmlogic.h"
#include <time.h>//110 40 //120 50

using namespace std ;
using namespace std::chrono;        // 10    ,J    ,Q    ,K    ,A   ,球衣  ,手套 ,秒表  ,金靴 ,金球  ,金杯 ,可变足球
const int32_t m_nWeights1[ALL_FRUIT]={500  ,500  ,480  ,480  ,450  ,250  ,250  ,240  ,170  ,170 ,100  ,160};
const int32_t m_nWeights2[ALL_FRUIT]={500  ,500  ,480  ,480  ,450  ,250  ,250  ,240  ,170  ,170 ,100  ,160};
const int32_t m_nWeights3[ALL_FRUIT]={500  ,500  ,480  ,480  ,450  ,250  ,250  ,240  ,170  ,170 ,100  ,110};
//9线位置
const int32_t __Fruit_Line[25][5] =
{
    { 5,	6,	7,	8,	9 },
    { 0,	1,	2,	3,	4 },
    { 10,	11,	12,	13,	14 },
    { 0,	6,	12, 8,	4 },
    { 10,	6,	2,	8,	14 },
    { 0,	6,	2,	8,	4 },
    { 10,	6,	12,	8,	14 },
    { 5,	1,	7,	3,	9 },
    { 5,	11,	7,	13,	9 },
    { 0,	6,	7,	8,	4 },
    { 10,	6,	7,	8,	14 },
    { 5,	1,	2,	3,	9 },
    { 5,	11,	12,	13,	9 },
    { 5,	6,	2,	8,	9 },
    { 5,	6,	12,	8,	9 },
    { 0,	1,	7,	3,	4 },
    { 10,	11,	7,	13,	14 },
    { 0,	11,	12,	13,	4 },
    { 10,	1,	2,	3,	14 },
    { 10,	11,	2,	13,	14 },
    { 0,	1,	12,	3,	4 },
    { 5,	1,	12,	3,	9 },
    { 5,	11,	2,	13,	9 },
    { 0,	11,	7,	13,	4 },
    { 10,	1,	7,	3,	14 }
};
const double __Fruit_Multi[12][5] =
{
    { 0,	0,		12.5,	25,		50 },		//0  IMAGE_10
    { 0,	0,		12.5,	25,		50 },		//1  IMAGE_J
    { 0,	0,		12.5,	25,		62.5 },		//2  IMAGE_Q
    { 0,	0,		12.5,	25,		62.5 },		//3  IMAGE_K
    { 0,	0,		12.5,	25,		75 },		//4  IMAGE_A
    { 0,	0,		25,		50,		75 },		//5  IMAGE_yifu
    { 0,	0,		25,		62.5,	150 },		//6  IMAGE_shoutao
    { 0,	0,		25,		75,		200 },		//7  IMAGE_miaobiao
    { 0,	0,		37.5,	150,	250 },		//8  IMAGE_jinxue
    { 0,	0,		37.5,	150,	250 },		//9 IMAGE_jinqiu
    { 0,	0,		62.5,	250,	937.5 },	//10 IMAGE_jinbei
    { 0,	0,		0,		0,		0 }        //11 IMAGE_zuqiu = wild
};
AlgorithmLogic::AlgorithmLogic():
    timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "ThreadInitCallback"))
{
    historyWinScore = 0;
    RoundTimes = 0;
    timelooper->startLoop();
    InItAgorithm();
    totalNormalIn = 0;
    totalNormalWin = 0;
    totalMarryWin = 0;
    totalMarryIn = 0;
    srand(time(NULL));

    roundCount = 0;
    goldSharkCount = 0;
    silverSharkCount = 0;
    for(int i=0;i<8;i++)
    {
        aniCount[i]=0;
    }
    isFree = false;
    memset(&last_table,0,sizeof(last_table));
    bigBounsNum = 0;
    freeRound = 0;
    biggerNum = 0;
}

AlgorithmLogic::~AlgorithmLogic()
{

}
bool AlgorithmLogic::InItAgorithm()
{
    timelooper->getLoop()->runEvery(0.1,bind(&AlgorithmLogic::ThreadInitCallback,this));
    return true;
}
void AlgorithmLogic::ThreadInitCallback()
{



    for(int i=0;i<ALL_FRUIT;i++)
    {
        if(historyWinScore<-500000)
        {
            m_nWeights[i]=m_nWeights2[i];
        }
        else if(historyWinScore>=-500000&&historyWinScore<=0)
        {
            m_nWeights[i]=m_nWeights2[i];
        }
        else
        {
            m_nWeights[i]=m_nWeights2[i];
        }
    }


    if(isFree)
    {
        for(int i=0;i<ALL_FRUIT;i++)
            m_nWeights[i]=m_nWeights3[i];
    }
    currentWinScore = 0;
    currentJettonScore[0]= 0;
    calculateJettonScore=0;
    thisPushIn=0;
    PlayersBetting();
    roundCount++;


    if(historyWinScore>0)
    {
       Result();
    }
    else
    {
       Result();
    }
}

void AlgorithmLogic::PlayersBetting()
{
    long jettom=RandomInt(100,100);
    currentJettonScore[0] = jettom;
    calculateJettonScore += jettom;

}
int	AlgorithmLogic::IsSame(int First, int Second)
{
    assert(First < FRUIT_MAX && First >= 0);
    assert(Second < FRUIT_MAX  && Second >= 0);
    if (First == Second)
    {
        return First;
    }
    if (First == IMAGE_FootBall)
    {
        return Second;
    }
    if (Second == IMAGE_FootBall )
    {
        return First;
    }
    return -1;
}
void AlgorithmLogic::Result()
{
    currentWinScore=0;
    //在这里出结果吧
    int32_t tsum = 0L;
    memset(&a_table,-1,sizeof(a_table));

    int32_t thisFootballCount = 0;

    for (int i = 0; i < ALL_FRUIT; i++)
    {
        tsum += m_nWeights[i];
    }
    for(int j=0;j<15;j++)
    {
        int32_t randint=RandomInt(0,tsum);
        int addsum=0;
        for(int i = 0; i < ALL_FRUIT; i++)
        {
            addsum+=m_nWeights[i];
            if(addsum>=randint)
            {
                a_table[j] = i;
                LOG_INFO << "a_table["<<j<<"]"<<" = "<<i;
                break;
            }
        }
    }
    //假如是免费拉霸状态，就要把上次的football 向左移动
    if(isFree)
    {
        for(int i=0;i<3;i++)
        {
            for(int j=0;j<4;j++)
            {
                if(j==0&&last_table[i][j]==IMAGE_FootBall)
                {
                    thisFootballCount++;
                }
                last_table[i][j]=last_table[i][j+1];

            }
            last_table[i][4] = 0;
        }
        allFootballNum+=thisFootballCount;
        if(allFootballNum>biggerNum)
        {
           biggerNum = allFootballNum;
        }
    }
    if(isFree)
    {
        for(int i=0;i<15;i++)
        {
            if(last_table[i/5][i%5]==IMAGE_FootBall)
            {
                a_table[i] =last_table[i/5][i%5];
            }
        }
    }

    double mutical = 0;

    for(int i=0;i<25;i++)
    {
        int32_t result[5] = {-1};

        int32_t fruit=-1;
        int32_t fruitNum =0;

        int SameNum = 0;	//相同的水果个数
        int SameFruit = 0;
        //每条线进行判断
        for(int j=0;j<5;j++)
        {
            result[j]=a_table[__Fruit_Line[i][j]];
        }

        if (result[0] == result[1] &&
            result[1] == result[2] &&
            result[2] == result[3] &&
            result[3] == result[4])
        {
            if (result[0] == IMAGE_FootBall)
            {
                fruit = IMAGE_FootBall;
                fruitNum = 4;
                mutical += __Fruit_Multi[IMAGE_FootBall][4];
                break;
            }
        }
        for (int j = 0; j < Table_Col_Num - 1; j++)
        {
            int CurrentFruit = a_table[__Fruit_Line[i][j]];
            int Next = a_table[__Fruit_Line[i][j + 1]];
            int FruitCode = IsSame(CurrentFruit, Next);
            //相等
            if (FruitCode != -1)
            {
                if (SameFruit == -1)
                {
                    if (FruitCode != IMAGE_FootBall)
                    {
                        SameFruit = FruitCode;
                    }
                    SameNum++;
                }
                else {
                    if (IsSame(FruitCode, SameFruit) != -1)
                    {
                        SameNum++;
                    }
                    else {
                        break;
                    }
                }
            }
            else {
                break;
            }
        }
        if (SameFruit < 0 || SameFruit >= FRUIT_MAX)
        {
            break;
        }
        if (SameNum > 5 || SameNum < 0)
        {
            break;
        }
        mutical += __Fruit_Multi[SameFruit][SameNum];
    }
    LOG_INFO << "本次mutical       : "<<mutical;
    currentWinScore = mutical * (calculateJettonScore /25);

    int32_t thisTurnNum = 0;
    for(int i=0;i<15;i++)
    {
        if(a_table[i] ==IMAGE_FootBall)
        {
            thisTurnNum++;
        }
    }

    if(isFree&&allFootballNum>=15&&thisTurnNum==0)
    {
        currentWinScore+=calculateJettonScore*1000;
        bigBounsNum++;

    }
    if(isFree)
    {
        currentWinScore=currentWinScore;
        totalGetOut +=currentWinScore;
        totalNormalWin +=currentWinScore;
    }
    else
    {
        currentWinScore=currentWinScore-calculateJettonScore;
        totalGetOut +=currentWinScore+calculateJettonScore;
        totalNormalWin +=currentWinScore-calculateJettonScore;
        totalNormalIn +=calculateJettonScore;
        thisPushIn+=calculateJettonScore;
        totalPushIn+=calculateJettonScore;
    }




    LOG_INFO << "产生的盈利------------        : "<<totalPushIn-totalMarryWin;
    LOG_INFO << "普通奖产生的盈利------------        : "<<totalPushIn-totalNormalWin;
    LOG_INFO << "本次盈利是： -----------------       : "<<currentWinScore;
    //LOG_INFO << "本次总倍数是 -----------------       : "<<currentWinScore/currentJettonScore;

    LOG_INFO << "总进数量----------------------      : "<<totalPushIn;
    LOG_INFO << "总出数量----------------------      : "<<totalGetOut;

    LOG_INFO << "出现足球累加最大是----------------------      : "<<biggerNum;
    LOG_INFO << "出现免费次数是----------------------      : "<<freeRound;
    if(allFootballNum>=15&&thisTurnNum==0)
    {
        LOG_INFO << "出现大奖了      : "<<calculateJettonScore*1000;
    }
    LOG_INFO << "出现大奖次数是      : "<<bigBounsNum;
    LOG_INFO << "出现大奖比例是      : "<<(double)bigBounsNum/(double)roundCount;
    LOG_INFO << "********************************************************";
    LOG_INFO << "************************第"<<roundCount<<"局************************";
    LOG_INFO << "********************************************************";
    //这里预判的结果都是下一局了
    if(!isFree)
    {
        int count = 0;
        for(int i=0;i<15;i++)
        {
            if(a_table[i]==IMAGE_FootBall)
            {
                count++;
            }
        }
        if(count>=3)
        {
            isFree = true;
            freeRound++;
            for(int i=0;i<3;i++)
            {
                if(last_table[i][0]==IMAGE_FootBall)
                {
                    allFootballNum++;

                }
            }
            if(allFootballNum>biggerNum)
            {
               biggerNum = allFootballNum;
            }
        }
    }

    if(isFree)
    {
        int count = 0;
        for(int i=0;i<15;i++)
        {
            if(a_table[i]==IMAGE_FootBall)
            {
                count++;
                last_table[i/5][i%5]=a_table[i];
            }
        }
        if(count<=0)
        {
            isFree = false;
            allFootballNum = 0;
            memset(&last_table,0,sizeof(last_table));
        }
    }


}
void AlgorithmLogic::ClearDate()
{

}
