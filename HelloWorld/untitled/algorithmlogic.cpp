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
using namespace std::chrono;//冰糖葫芦，包子，桃子，如意，福袋，金鱼，财主，弥勒佛，财神,金锣,财神到
const int32_t m_nWeights1[11]={760,760,700,660,660,364,320,320,250,320,90};
const int32_t m_nWeights2[11]={660,660,580,520,520,364,300,300,250,300,80};
const int32_t m_nWeights3[11]={760,760,700,660,660,364,320,320,250,320,90};
const int32_t m_nWeights4[11]={100,100,120,164,460,460,800,800,800,260,170};
const int32_t m_nfruitMutical[16]={5,3,10,3,15,3,20,3,20,3,30,3,40,3,120,50};
const int32_t m_lUserAwardMutical[16]={20,20,30,30,40,40,50,50,60,60,80,80,100,100,200,200};
const int32_t m_nWeightsMarry[18]={4,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1};
//                                  冰糖葫芦   ，包子      ，桃子       ，如意      ，福袋        ，金鱼     ，财主     ，弥勒佛        ，财神  ，金锣
const double_t m_mutical[10][3]={{0.2,0.6,3},{0.2,0.6,3},{0.2,0.8,4},{0.2,1.2,4},{0.2,1.2,4},{0.6,2,8},{0.6,4,10},{0.6,4,10},{1,6,12},{0,0,0}};


AlgorithmLogic::AlgorithmLogic():
    timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "ThreadInitCallback"))
{
    historyWinScore = 0;
    RoundTimes = 0;
    timelooper->startLoop();
    InItAgorithm();
    totalNormalIn = 0;
    totalNormalWin = 0;
    freeCount = 0;
    freeRound = 0;
    totalMarryWin = 0;
    totalMarryIn = 0;
    roundNum = 0;
    srand(time(NULL));

    mongocxx::instance instance{}; // This should be done only once.
    m_mongoDBServerAddr = "mongodb://192.168.2.97:27017";
    ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);

    mongocxx::database db = MONGODBCLIENT["gamemain"];
}

AlgorithmLogic::~AlgorithmLogic()
{

}
bool AlgorithmLogic::InItAgorithm()
{
    timelooper->getLoop()->runEvery(0.1,bind(&AlgorithmLogic::ThreadInitCallback,this));
    return true;
}
void AlgorithmLogic::CountIsWin()
{

    resultArrVec.clear();
    //初始化结果数组
    for(int i=0;i<10;i++)
    {
        isWinFruit[i] =false;
        for(int j=0;j<5;j++)
        {
            isTrue[i][j] = false;
            fiveLieVec[i][j].clear();
        }
    }
    //应该是每种图标只能中将一次,那么就是每种图标判断一次就可以了
    for(int i=0;i<10;i++)
    {
        winFruit[i].clear();
        for(int k=0;k<3;k++)
        {
           for(int j=0;j<5;j++)
           {
               //财神到或者等于当前图标
                if(i!=9&&(t_table[k][j]==i||t_table[k][j]==10))
                {
                   isTrue[i][j] = true;
                   fruitWin fruit;
                   fruit.fruit = i;
                   fruit.heng = k;
                   fruit.lie = j;
                   fruit.index=j+k*5;
                   winFruit[i].push_back(fruit);
                }
                else if(i==9&&t_table[k][j]==i)
                {
                    isTrue[i][j] = true;
                    fruitWin fruit;
                    fruit.fruit = i;
                    fruit.heng = k;
                    fruit.lie = j;
                    fruit.index=j+k*5;
                    winFruit[i].push_back(fruit);
                }
           }
        }
    }

    int32_t count[10] = {0};
    for(int i=0;i<10;i++)
    {

        for(int j=0;j<5;j++)
        {
            if(isTrue[i][j])
            {
               count[i]++; //算出最多有多少列
            }
            else
            {
                break;
            }
        }
        //达不到中奖目标全部清空
        if(winFruit[i].size()<=2||count[i]<=2)
        {
            for(int j=0;j<5;j++)
            {
                isTrue[i][j] = false;
                fiveLieVec[i][j].clear();
            }
            isWinFruit[i] = false;
        }
        else
        {
            isWinFruit[i] = true;
            for(int k=0;k<count[i];k++)
            {
                for(int x=0;x<winFruit[i].size();x++)
                {
                    if(k==winFruit[i][x].lie)
                    {
                        fiveLieVec[i][k].push_back(winFruit[i][x]);
                    }
                }
            }
            //只有三种情况出现三连，四连，五连
            switch (count[i]) {
            case 3:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {

                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {

                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {
                                resultStr arr;
                                arr.count = 3;
                                arr.fruit = i;
                                arr.fruitVct.clear();
                                arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                if(i!=9)
                                {
                                   arr.isFree = false;
                                   arr.isWin =true;
                                }
                                else
                                {
                                    arr.isFree = true;
                                    arr.isWin =false;
                                }
                                arr.mutic =  m_mutical[i][0];
                                resultArrVec.push_back(arr);
                            }
                        }
                    }
                }
                break;
            case 4:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {

                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {

                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {

                                for(int b=0;b<fiveLieVec[i][3].size();b++)
                                {

                                    resultStr arr;
                                    arr.count = 4;
                                    arr.fruit = i;
                                    arr.fruitVct.clear();
                                    arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                    arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                    arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                    arr.fruitVct.push_back(fiveLieVec[i][3][b]);
                                    if(i!=9)
                                    {
                                       arr.isFree = false;
                                       arr.isWin =true;
                                    }
                                    else
                                    {
                                        arr.isFree = true;
                                        arr.isWin =false;
                                    }
                                    arr.mutic =  m_mutical[i][1];
                                    resultArrVec.push_back(arr);
                                }
                            }
                        }
                    }
                }
                break;
            case 5:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {
                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {
                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {
                                for(int b=0;b<fiveLieVec[i][3].size();b++)
                                {
                                    for(int n=0;n<fiveLieVec[i][4].size();n++)
                                    {

                                        resultStr arr;
                                        arr.count = 5;
                                        arr.fruit = i;
                                        arr.fruitVct.clear();
                                        arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                        arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                        arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                        arr.fruitVct.push_back(fiveLieVec[i][3][b]);
                                        arr.fruitVct.push_back(fiveLieVec[i][4][n]);
                                        if(i!=9)
                                        {
                                           arr.isFree = false;
                                           arr.isWin =true;
                                        }
                                        else
                                        {
                                            arr.isFree = true;
                                            arr.isWin =false;
                                        }
                                        arr.mutic =  m_mutical[i][2];
                                        resultArrVec.push_back(arr);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }

        }

    }


    //到这里只要长度不为零的容器对应的水果就是在中将了

}

void AlgorithmLogic::ThreadInitCallback()
{
    //ReadPlayDB();
    //AggregatePlayerBets();
    currentWinScore = 0;
    currentJettonScore= 0;

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
    calculateJettonScore=0;
    thisPushIn=0;
    PlayersBetting();

    if(freeCount>0)
    {
        for(int i=0;i<ALL_FRUIT;i++)
         m_nWeights[i]=m_nWeights4[i];
    }
    roundNum++;

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
    long jettom=RandomInt(100,100);
    currentJettonScore = jettom;
    calculateJettonScore = jettom;

}
void AlgorithmLogic::Result1()
{
//    int32_t randint=RandomInt(0,100);
//    if(randint>20)
//    {
//        //在这里出结果吧
//        int32_t tsum = 0L;
//        int resultIndex=0;

//        uint8_t MarryNum=0;
//        for (int i = 0; i < 18; i++)
//        {
//            tsum += m_nWeights[i];
//        }
//        int32_t randint=RandomInt(0,tsum);
//        int addsum=0;
//        for(int i = 0; i < 18; i++)
//        {
//            addsum+=m_nWeights[i];
//            if(addsum>randint)
//            {
//                resultIndex =i;
//                break;
//            }
//        }
//        //开出普通结果
//        if(resultIndex<16)
//        {
//            //计算游戏结果
//            LOG_INFO << "开出普通结果";
//            currentWinScore=m_nfruitMutical[resultIndex]*calculateJettonScore[resultIndex]-currentJettonScore;
//            totalGetOut +=m_nfruitMutical[resultIndex]*calculateJettonScore[resultIndex];

//            LOG_INFO << "本次盈利是： -----------------       : "<<currentWinScore;
////            LOG_INFO << "本次总倍数是 -----------------       : "<<currentWinScore/currentJettonScore;
//            LOG_INFO << "本次开出的水果下标-----------------   : "<<resultIndex;
//            LOG_INFO << "总进数量----------------------      : "<<totalPushIn;
//            LOG_INFO << "总出数量--------------------        : "<<totalGetOut;
//        }
//        else//开出游戏机游戏
//        {
//            //计算玛利结果
//            LOG_INFO << "开出玛利机";
//            if(resultIndex==16||resultIndex==17)//蓝色lucky
//            {
//                int32_t MarryNum = 0;
//                //随机马丽机次数
//                if(resultIndex==17)//黄色lucky
//                {
//                    //随机马丽机次数
//                     MarryNum = RandomInt(5,8);
//                }
//                else
//                {
//                    MarryNum = RandomInt(1,4);
//                }


//                int32_t marrycount = MarryNum;
//                ///////////////////
//                //////////////////
//                ///
//                int fruitNumCount[16] = {0};
//                do
//                {
//                    int32_t tsum = 0L;
//                    int32_t fruitIndex = 0;
//                    for (int32_t i = 0; i < 16; i++)
//                    {
//                        tsum += m_nWeights[i];
//                    }
//                    int32_t randint=RandomInt(0,tsum);
//                    int32_t addsum=0;
//                    for(int32_t i = 0; i < 16; i++)
//                    {
//                        addsum+=m_nWeights[i];
//                        if(addsum>randint)
//                        {
//                            fruitIndex = i;
//                            fruitNumCount[i]++;
//                            break;
//                        }

//                    }
//                    marrycount--;
//                    currentMarryWinSocre+=calculateJettonScore[fruitIndex]*m_nfruitMutical[fruitIndex];
//                    for(int i = 0; i < 15; i+=2)
//                    {
//                        //这里证明有游戏机出现
//                        if(fruitNumCount[i]+fruitNumCount[i+1]>=3)
//                        {
//                           MarryNum = MarryNum -  marrycount;
//                           marrycount = 0;
//                           currentMarryWinSocre = m_lUserAwardMutical[i]*currentJettonScore;
//                           break;
//                        }
//                    }



//                }while(marrycount>0);

//                totalGetOut +=currentMarryWinSocre;
//            }


//            currentWinScore=currentMarryWinSocre-currentJettonScore;
//            LOG_INFO << "本次盈利是： -----------------       : "<<currentWinScore;
//            //LOG_INFO << "本次总倍数是 -----------------       : "<<currentWinScore/currentJettonScore;
//            //OG_INFO << "本次开出的水果下标-----------------   : "<<resultIndex;
//            LOG_INFO << "总进数量----------------------      : "<<totalPushIn;
//            LOG_INFO << "总出数量--------------------        : "<<totalGetOut;
//        }


//         RoundTimes++;

//         historyWinScore+=currentWinScore;
//         if(historyWinScore>BigHitStoryScore)
//         {
//             BigHitStoryScore=historyWinScore;
//         }

//         LOG_INFO << "相对玩家，历史赢过最多的钱------------        : "<<BigHitStoryScore;
//         LOG_INFO << "相对玩家的历史盈利--------------------        : "<<historyWinScore;




//         LOG_INFO << "游戏总局数--------------------        : "<<RoundTimes;
//         LOG_INFO << "                             ";
//         LOG_INFO << "                             ";
//         LOG_INFO << "                             ";
//         LOG_INFO << "                             ";
//    }
//    else
//    {
//        do
//        {
//            //在这里出结果吧

//            currentMarryWinSocre=0;
//            int32_t tsum = 0L;
//            int resultIndex=0;
//            int crrentWin = 0;
//            uint8_t MarryNum=0;
//            for (int i = 0; i < 18; i++)
//            {
//                tsum += m_nWeights[i];
//            }
//            int32_t randint=RandomInt(0,tsum-1);
//            int addsum=0;
//            for(int i = 0; i < 18; i++)
//            {
//                addsum+=m_nWeights[i];
//                if(addsum>=randint)
//                {
//                    resultIndex =i;
//                    break;
//                }
//            }
//            if(resultIndex<16)
//            {
//                currentMarryWinSocre=m_nfruitMutical[resultIndex]*calculateJettonScore[resultIndex]-currentJettonScore;
//            }
//            else
//            {
//                int32_t MarryNum = 0;
//                //随机马丽机次数
//                if(resultIndex==17)//黄色lucky
//                {
//                    //随机马丽机次数
//                     MarryNum = RandomInt(5,8);
//                }
//                else
//                {
//                    MarryNum = RandomInt(1,4);
//                }


//                int32_t marrycount = MarryNum;
//                ///////////////////
//                //////////////////
//                ///
//                int fruitNumCount[16] = {0};
//                do
//                {
//                    int32_t tsum = 0L;
//                    int32_t fruitIndex = 0;
//                    for (int32_t i = 0; i < 16; i++)
//                    {
//                        tsum += m_nWeights[i];
//                    }
//                    int32_t randint=RandomInt(0,tsum);
//                    int32_t addsum=0;
//                    for(int32_t i = 0; i < 16; i++)
//                    {
//                        addsum+=m_nWeights[i];
//                        if(addsum>randint)
//                        {
//                            fruitIndex = i;
//                            fruitNumCount[i]++;
//                            break;
//                        }

//                    }
//                    marrycount--;
//                    currentMarryWinSocre+=calculateJettonScore[fruitIndex]*m_nfruitMutical[fruitIndex];
//                    for(int i = 0; i < 15; i+=2)
//                    {
//                        //这里证明有游戏机出现
//                        if(fruitNumCount[i]+fruitNumCount[i+1]>=3)
//                        {
//                           MarryNum = MarryNum -  marrycount;
//                           marrycount = 0;
//                           currentMarryWinSocre = m_lUserAwardMutical[i]*currentJettonScore;
//                           break;
//                        }
//                    }



//                }while(marrycount>0);


//            }
//            totalGetOut +=currentMarryWinSocre;
//            currentMarryWinSocre = currentMarryWinSocre- currentJettonScore;
//            if(currentMarryWinSocre<=0)
//            {
//                break;
//            }
//        }while(true);

//    }
}
void AlgorithmLogic::Result()
{
    currentWinScore=0;
    //在这里出结果吧
    int32_t tsum = 0L;
    int resultIndex=0;

    uint8_t MarryNum=0;
    for (int i = 0; i < 11; i++)
    {
        tsum += m_nWeights[i];
    }

    for(int k=0;k<15;k++)
    {
        int32_t randint=RandomInt(0,tsum);
        int addsum=0;
        for(int i = 0; i < 11; i++)
        {
            addsum+=m_nWeights[i];
            if(addsum>randint)
            {
                t_table[k/5][k%5] =i;
                a_table[k] = i;
                break;
            }
        }
    }

    LOG_INFO << t_table[0][0]<<"  "<<t_table[0][1]<<"  "<<t_table[0][2]<<"  "<<t_table[0][3]<<"  "<<t_table[0][4];
    LOG_INFO << t_table[1][0]<<"  "<<t_table[1][1]<<"  "<<t_table[1][2]<<"  "<<t_table[1][3]<<"  "<<t_table[1][4];
    LOG_INFO << t_table[2][0]<<"  "<<t_table[2][1]<<"  "<<t_table[2][2]<<"  "<<t_table[2][3]<<"  "<<t_table[2][4];
    CountIsWin();
    //开出普通结果
    //计算游戏结果
    bool IsFree =false;
    LOG_INFO << "开出普通结果";

    for(int i=0;i<resultArrVec.size();i++)
    {
        if(resultArrVec[i].isWin)
        {
            resultArrVec[i].winScore = resultArrVec[i].mutic*currentJettonScore;
        }
        currentWinScore +=resultArrVec[i].winScore;

        for(int j=0;j<resultArrVec[i].fruitVct.size();j++)
        {
            LOG_INFO << " 水果位置是:"<<  resultArrVec[i].fruitVct[j].index;
        }
        LOG_INFO << "中奖的水果是------------        : "<<  resultArrVec[i].fruit<< "     中奖个数:"<<resultArrVec[i].count<<"   中奖倍数是 :" <<resultArrVec[i].mutic;
        if(resultArrVec[i].isFree)
        {
            IsFree =true;
        }
    }


    if(freeCount>0)
    {
        freeCount -- ;
        currentWinScore=currentWinScore;
        totalGetOut +=currentWinScore;

        totalNormalIn +=0;
        totalNormalWin +=currentWinScore;
        thisPushIn+=0;
        totalPushIn+=0;
    }
    else
    {
        currentWinScore=currentWinScore-currentJettonScore;
        totalGetOut +=currentWinScore+currentJettonScore;

        totalNormalIn +=currentJettonScore;
        totalNormalWin +=currentWinScore-currentJettonScore;
        thisPushIn+=currentJettonScore;
        totalPushIn+=currentJettonScore;
    }

    if(IsFree)
    {
        freeCount = 10;
        freeRound ++;
    }

    LOG_INFO << "剩余免费次数------------        : "<< freeCount;
    LOG_INFO << "产生的盈利------------        : "<<totalPushIn-totalMarryWin;
    LOG_INFO << "普通奖产生的盈利------------        : "<<totalPushIn-totalNormalWin;
    LOG_INFO << "本次盈利是： -----------------       : "<<currentWinScore;
    //LOG_INFO << "本次总倍数是 -----------------       : "<<currentWinScore/currentJettonScore;
    LOG_INFO << "本次开出的水果下标-----------------   : "<<resultIndex;
    LOG_INFO << "总进数量----------------------      : "<<totalPushIn;
    LOG_INFO << "总出数量--------------------        : "<<totalGetOut;
    LOG_INFO << "出现免费拉霸的次数------------        : "<<freeRound;
    LOG_INFO << "运行局数---------------------        : "<<roundNum;

}
void AlgorithmLogic::ClearDate()
{

}
