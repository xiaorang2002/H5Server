#include "algorithmlogic.h"
#include <time.h>//110 40 //120 50
const int32_t m_nWeights1[18]={2000,3333,1000,3333,667,3333,500,3333,500,3333,333,3333,250,3333,65,150,10,5};
const int32_t m_nWeights2[18]={2000,2833,1000,2833,667,2833,500,2833,500,2833,333,2833,250,2833,80,180,720,300};
const int32_t m_nWeights3[18]={2000,2933,1000,2933,667,2933,500,2933,500,2933,333,2933,250,2933,80,180,250,50};
const int32_t m_nfruitMutical[16]={5,3,10,3,15,3,20,3,20,3,30,3,40,3,120,50};
const int32_t m_lUserAwardMutical[16]={20,20,30,30,40,40,50,50,60,60,80,80,100,100,200,200};
const int32_t m_nWeightsMarry[18]={4,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1};

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
    srand(time_t());
}
AlgorithmLogic::~AlgorithmLogic()
{

}


void AlgorithmLogic::ThreadInitCallback()
{
//    vtValidItem.clear();
//    vtNotValidItem.clear();

    currentWinScore = 0;
    currentJettonScore= 0;

    for(int i=0;i<18;i++)
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


    for(int i=0 ;i<16;i++)
    {
        calculateJettonScore[i]=0;
    }
    thisPushIn=0;
    PlayersBetting();
//    int32_t res[5];
//    int32_t res1[5];
//    Result(5,res,50,1);
//    Result(5,res1,50,1);

//    int resultArr[10]={0};
//    for(int i=0;i<5;i++)
//    {
//        resultArr[i]=res[i];
//        resultArr[i+5]=res1[i];
//    }
//    for(int k=0;k<10;k++)
//    {
//        for(int j=k+1;j<10;j++)
//        {
//            if(resultArr[k]>=resultArr[j])
//            {
//               continue;
//            }
//            else
//            {
//               int32_t res= resultArr[k];
//                resultArr[k]=resultArr[j];
//                resultArr[j]=res;
//            }
//        }
//    }
    int Arrsize = 10;
    int resultArr[10]={0};
    int weight[10]={0};
    int allWeight=0;
    int leftNum = 0;
    for(int i=0;i<Arrsize;i++)
    {
        weight[i]=RandomInt(1,100);
        allWeight+=weight[i];
    }
    int ranInt = RandomInt(1,allWeight);

    for(int i=0;i<Arrsize;i++)
    {
        if(i==Arrsize-1)
        {
            resultArr[i] = 10000-leftNum;
        }
        else
        {
            resultArr[i]=(weight[i]*10000)/allWeight+1;
            leftNum+=resultArr[i];
        }
    }

    for(int k=0;k<Arrsize;k++)
    {
        for(int j=k+1;j<Arrsize;j++)
        {
            if(resultArr[k]>=resultArr[j])
            {
               continue;
            }
            else
            {
               int32_t res= resultArr[k];
                resultArr[k]=resultArr[j];
                resultArr[j]=res;
            }
        }
    }
    //GetResultArr(resultArr,Arrsize,10000);
    for(int i=0;i<Arrsize;i++)
    LOG_INFO << "产生红包("<<i+1<<")------------        : "<<resultArr[i];

    int64_t moneyAll= 0;
    for(int i=0;i<Arrsize;i++) moneyAll+=resultArr[i];
    LOG_INFO << "红包总和是------------        : "<<moneyAll;

    LOG_INFO << "********************************************* ";
    LOG_INFO << "********************************************* ";
    LOG_INFO << "********************************************* ";
    LOG_INFO << "********************************************* ";
    LOG_INFO << "********************************************* ";
    LOG_INFO << "********************************************* ";
    //Result();
}
bool AlgorithmLogic::InItAgorithm()
{
    timelooper->getLoop()->runEvery(0.01,bind(&AlgorithmLogic::ThreadInitCallback,this));
    return true;
}
void AlgorithmLogic::PlayersBetting()
{

}
void AlgorithmLogic::GetResultArr(int32_t arr[],int32_t num,int64_t money)
{
    if(num==1)
    {
        arr[0]=money;
        return;
    }
    int32_t sizeNum1=0;
    int32_t sizeNum2=0;
    int64_t money1 = 0;
    int64_t money2 = 0;
    if(num%2==0)
    {
        sizeNum1 = num/2;
        sizeNum2 = num/2;
    }
    else
    {
        sizeNum1 = num/2;
        sizeNum2 = num/2+1;
    }
    if(money%2==0)
    {
        money1 = money/2;
        money2 = money/2;
    }
    else
    {
        money1 = money/2;
        money2 = money/2+1;
    }
    int32_t res[sizeNum1];
    int32_t res1[sizeNum2];
    Result(sizeNum1,res,money1,1);
    Result(sizeNum2,res1,money2,1);


    for(int i=0;i<sizeNum1;i++)
    {
        arr[i]=res[i];
    }
    for(int i=0;i<sizeNum2;i++)
    {
        arr[i+sizeNum1]=res1[i];
    }
    for(int k=0;k<num;k++)
    {
        for(int j=k+1;j<num;j++)
        {
            if(arr[k]>=arr[j])
            {
               continue;
            }
            else
            {
               int32_t res= arr[k];
                arr[k]=arr[j];
                arr[j]=res;
            }
        }
    }
}
void AlgorithmLogic::Result(int32_t playerNum,int32_t res[],int64_t monye,int32_t minMonye)
{
    int32_t hongbao[playerNum];
    int32_t allMoney = monye;
    int32_t minMoney = minMonye;
    int32_t maxPalyer = playerNum;
    int32_t leftMoney = monye;
    bool isOk = false;
    do
    {
        isOk = false;
        leftMoney = monye;
        for(int i=0;i<maxPalyer;i++)
        {
            hongbao[i] =0;
        }
        for(int i=0;i<maxPalyer;i++)
        {
            if(i==maxPalyer-1)
            {
                hongbao[i] = leftMoney;
            }
            else
            {   int32_t maxNum = (leftMoney-minMoney*(maxPalyer-1-i))/2;
                hongbao[i] = RandomInt(minMoney,maxNum);
                leftMoney -= hongbao[i];
            }
        }
//        for(int k=0;k<maxPalyer;k++)
//        {
//            for(int j=k+1;j<maxPalyer;j++)
//            {
//                if(hongbao[k]>=hongbao[j])
//                {
//                   continue;
//                }
//                else
//                {
//                   int32_t res= hongbao[k];
//                    hongbao[k]=hongbao[j];
//                    hongbao[j]=res;
//                }
//            }
//            if(isOk) break;
//        }
//        for(int k=0;k<10;k++)
//        {
//            for(int j=k+1;j<10;j++)
//            {
//                if(hongbao[k]==hongbao[j])
//                {
//                    isOk=true;
//                    break;
//                }
//            }
//            if(isOk) break;
//        }

    }while(isOk);
    for(int i=0;i<playerNum;i++)
    {
        res[i]=hongbao[i];
    }
}
void AlgorithmLogic::ClearDate()
{

}
