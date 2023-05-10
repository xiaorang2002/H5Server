#ifndef LONGHUALGORITHM_H
#define LONGHUALGORITHM_H
#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"
enum bettinggate
{
    He=0,
    Long=1,
    Hu=2,
    Max
};
#define MAX_CARDS                  52
#define Denominator 10000
class LongHuAlgorithm
{
public:
    LongHuAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int  m_thresholdNum;//阈值设定
     int  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)
     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     int64_t  resultProbility;
     long bettingQuantity[Max];//三门押注
     int8_t   m_muticleArr[Max];
     int64_t  miCurrentProfit;
     STD::Random m_random;
#ifdef Test_Algorithm
     long zongjin;
     long zongchu;
     long profit;

     void CalculationResults();
#else

#endif

public:
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
         return min+rand()%(max-min+1);
     }
     inline void  SetBetting(int index ,long value)
     {
         if(index<0||index>=Max)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,int64_t curr,float ptobali,int8_t mularr[],int threshold=5000)
     {
         if(high<low)
         {
             m_highLimit = low;
             m_lowLimit = high;
         }
         else {
             m_highLimit = high;
             m_lowLimit = low;
         }
         m_thresholdNum =threshold;

         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         miCurrentProfit = 0;
         memcpy(m_muticleArr,mularr,sizeof(m_muticleArr));

         resultProbility=OverHighLowLimit()*Denominator; //算出当前盈利在哪个区间,并且算出在这个区间的控制概率10000为分母
         CalculatCotrol();   //计算出第一步是杀还是放还是随机resultProbility等于1或者等于0或者等于-1,

     }

     double OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(int8_t & wintag);

     void CallBackHandle();

     int64_t CurrentProfit();//计算本局不算税的系统赢输

     void BlackGetOpenCard(int8_t &wintag,float probirity[3]);
     void  openLog(const char *p, ...);
};

#endif // REDBLACKALGORITHM_H
