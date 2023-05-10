#ifndef BCBMALGORITHM_H
#define BCBMALGORITHM_H


#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"
enum bettinggate
{
    benchi=0,
    baoma=1,
    dazhong=2,
    leikesasi=3,
    baoshijie=4,
    mashaladi=5,
    lanbojini=6,
    falali=7,
    MaxGate=8
};

#define MAX_CARDS                  52
#define Denominator 10000
class BcbmAlgorithm
{
public:
    BcbmAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int64_t  m_thresholdNum;//阈值设定
     int  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)
     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     int  resultProbility;
     long bettingQuantity[MaxGate];//门押注
     int8_t   m_muticleArr[MaxGate];
     STD::Random m_random;
     int64_t  miCurrentProfit;

public:
     inline void  SetBetting(int index ,long value)
     {
         if(index<0||index>=MaxGate)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,int64_t curr,float ptobali,int32_t mularr[],int64_t threshold=5000)
     {
         m_thresholdNum =threshold;
         m_highLimit = high;
         m_lowLimit = low;
         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         miCurrentProfit=0;
         for(int i=0;i<MaxGate;i++)
         {
             m_muticleArr[i]=mularr[i];
         }


         resultProbility=OverHighLowLimit()*Denominator; //算出当前盈利在哪个区间,并且算出在这个区间的控制概率10000为分母
         CalculatCotrol();   //计算出第一步是杀还是放还是随机resultProbility等于1或者等于0或者等于-1,

     }
     //-1是必杀，0是随机 ，1 是放分
     inline void SetMustKill(int isKillOrRewar)
     {
         if(isKillOrRewar>1)
         {
             isKillOrRewar = 1;
         }
         else if(isKillOrRewar<-1)
         {
             isKillOrRewar = -1;
         }
         m_killOrReword = isKillOrRewar;
     }
     double OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(int &winTag,float probirity[]);
     void BlackGetOpenCard(int &winTag,float probirity[]);
     void CallBackHandle();

     void  openLog(const char *p, ...);
     int64_t CurrentProfit();//计算本局不算税的系统赢输

};

#endif // BCBMALGORITHM_H
