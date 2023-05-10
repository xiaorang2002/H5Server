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
using namespace std::chrono;//兔子、猴子、熊猫、狮子、老鹰、孔雀、鸽子、燕子 、金鲨银鲨
const int32_t m_nWeights1[9]={1625,1218,1218,812,812,1218,1218,1625,254};
const int32_t m_nWeights2[9]={1625,1218,1218,812,812,1218,1218,1625,254};
const int32_t m_nWeights3[9]={1625,1218,1218,812,812,1218,1218,1625,254};
//兔子、猴子、熊猫、狮子、老鹰、孔雀、鸽子、燕子 、金鲨、银鲨  32.5  50.15  38.4
const int32_t  all_mutical[10] = { 6,8,8,12,12,8,8,6,24,24 };//所有动物的倍数
const int32_t  feizou[2] = {2,2};  //飞禽走兽倍数
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

    //mongocxx::instance instance{}; // This should be done only once.
    //m_mongoDBServerAddr = "mongodb://192.168.2.97:27017";
    //ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
    roundCount = 0;
    goldSharkCount = 0;
    silverSharkCount = 0;
    for(int i=0;i<8;i++)
    {
        aniCount[i]=0;
    }

    //mongocxx::database db = MONGODBCLIENT["gamemain"];
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

    currentWinScore = 0;
    for(int i=0;i<9;i++)
    {
        currentJettonScore[i]= 0;
        if(i<2) currentFeiQinZouShouJetton[i] = 0;
    }


    for(int i=0;i<9;i++)
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
    //Result();
}

void AlgorithmLogic::PlayersBetting()
{
//    currentJettonScore[0] = 400;
//    calculateJettonScore += 400;
//    currentJettonScore[1] = 300;
//    calculateJettonScore += 300;
//    currentJettonScore[2] = 300;
//    calculateJettonScore += 300;
//    currentJettonScore[3] = 200;
//    calculateJettonScore += 200;
//    currentJettonScore[4] = 200;
//    calculateJettonScore += 200;
//    currentJettonScore[5] = 300;
//    calculateJettonScore += 300;
//    currentJettonScore[6] = 300;
//    calculateJettonScore += 300;
//    currentJettonScore[7] = 400;
//    calculateJettonScore += 400;

    //sha yu
    currentJettonScore[8] = 100;
    calculateJettonScore += 100;
//    currentFeiQinZouShouJetton[0] = RandomInt(100,100);
//    calculateJettonScore += currentFeiQinZouShouJetton[0];
//    currentFeiQinZouShouJetton[1] = RandomInt(100,100);
//    calculateJettonScore += currentFeiQinZouShouJetton[1];

}

void AlgorithmLogic::Result()
{
    currentWinScore=0;
    //在这里出结果吧
    int32_t tsum = 0L;
    int32_t resultIndex = 0;
    int32_t otherIndex = 0;


    for (int i = 0; i < 9; i++)
    {
        tsum += m_nWeights[i];
    }
    int32_t randint=RandomInt(0,tsum);
    int addsum=0;
    for(int i = 0; i < 9; i++)
    {
        addsum+=m_nWeights[i];
        if(addsum>randint)
        {
            resultIndex = i;
            LOG_INFO << "开出十种动物的一种 ----: "<<resultIndex;
            break;
        }
    }
    //走兽
    if(resultIndex<4)
    {
       currentWinScore += currentJettonScore[resultIndex]*all_mutical[resultIndex];
       currentWinScore += currentFeiQinZouShouJetton[0]*feizou[0];
       aniCount[resultIndex]++;
    }//飞禽
    else if(resultIndex>=4&&resultIndex<8)
    {
        currentWinScore += currentJettonScore[resultIndex]*all_mutical[resultIndex];
        currentWinScore += currentFeiQinZouShouJetton[1]*feizou[1];
        aniCount[resultIndex]++;
    }//金鲨银鲨
    else
    {
        int result = RandomInt(1,300); //均等射灯

        if(result<200)
        {
            resultIndex = 9;
        }
        else
        {
            resultIndex = 8;
        }
        //金鲨
        if(resultIndex==8)
        {
            goldSharkCount++;
            int32_t otherIndex=RandomInt(0,7); //均等射灯

            int32_t ran = RandomInt(1,100);
            int32_t mutic = 0;
            int32_t muticBouce = 0;
            int32_t ran1 = RandomInt(1,100);
            if(ran<60)
            {
                mutic = RandomInt(25,50);

            }
            else if(ran>=60&&ran<90)
            {
                mutic = RandomInt(51,75);

            }
            else
            {
                mutic = RandomInt(76,99);

            }
            if(ran<60)
            {

                muticBouce = RandomInt(6,25);
            }
            else if(ran>=60&&ran<90)
            {

                muticBouce = RandomInt(26,50);
            }
            else
            {

                muticBouce = RandomInt(51,99);
            }
            currentWinScore += currentJettonScore[8]*(muticBouce);
            currentWinScore += currentJettonScore[8]*all_mutical[otherIndex];
        }//银鲨
        else
        {
            silverSharkCount++;
            int32_t otherIndex=RandomInt(0,7);//均等射灯
            currentWinScore += currentJettonScore[8]*all_mutical[resultIndex];
            currentWinScore += currentJettonScore[8]*all_mutical[otherIndex];
        }
    }


    currentWinScore=currentWinScore-calculateJettonScore;
    totalGetOut +=currentWinScore+calculateJettonScore;

    totalNormalIn +=calculateJettonScore;
    totalNormalWin +=currentWinScore-calculateJettonScore;
    thisPushIn+=calculateJettonScore;
    totalPushIn+=calculateJettonScore;


    LOG_INFO << "产生的盈利------------        : "<<totalPushIn-totalMarryWin;
    LOG_INFO << "普通奖产生的盈利------------        : "<<totalPushIn-totalNormalWin;
    LOG_INFO << "本次盈利是： -----------------       : "<<currentWinScore;
    //LOG_INFO << "本次总倍数是 -----------------       : "<<currentWinScore/currentJettonScore;
    LOG_INFO << "本次开出的水果下标-----------------   : "<<resultIndex;
    LOG_INFO << "总进数量----------------------      : "<<totalPushIn;
    LOG_INFO << "总出数量----------------------      : "<<totalGetOut;
    //兔子、猴子、熊猫、狮子、老鹰、孔雀、鸽子、燕子 、金鲨、银鲨
    LOG_INFO << "兔子数量--------------------        : "<<aniCount[0]<<"  比率是:  "<<(double)aniCount[0]/(double)roundCount;
    LOG_INFO << "猴子数量--------------------        : "<<aniCount[1]<<"  比率是:  "<<(double)aniCount[1]/(double)roundCount;
    LOG_INFO << "熊猫数量--------------------        : "<<aniCount[2]<<"  比率是:  "<<(double)aniCount[2]/(double)roundCount;
    LOG_INFO << "狮子数量--------------------        : "<<aniCount[3]<<"  比率是:  "<<(double)aniCount[3]/(double)roundCount;
    LOG_INFO << "老鹰数量--------------------        : "<<aniCount[4]<<"  比率是:  "<<(double)aniCount[4]/(double)roundCount;
    LOG_INFO << "孔雀数量--------------------        : "<<aniCount[5]<<"  比率是:  "<<(double)aniCount[5]/(double)roundCount;
    LOG_INFO << "鸽子数量--------------------        : "<<aniCount[6]<<"  比率是:  "<<(double)aniCount[6]/(double)roundCount;
    LOG_INFO << "燕子数量--------------------        : "<<aniCount[7]<<"  比率是:  "<<(double)aniCount[7]/(double)roundCount;
    LOG_INFO << "出金鲨次数--------------------      : "<<goldSharkCount<<"  比率是:  "<<(double)goldSharkCount/(double)roundCount;
    LOG_INFO << "出银鲨次数--------------------      : "<<silverSharkCount<<"  比率是:  "<<(double)silverSharkCount/(double)roundCount;
    LOG_INFO << "********************************************************";
    LOG_INFO << "************************第"<<roundCount<<"局************************";
    LOG_INFO << "********************************************************";

}
void AlgorithmLogic::ClearDate()
{

}
