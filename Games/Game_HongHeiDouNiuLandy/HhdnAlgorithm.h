#ifndef HHDNALGORITHM_H
#define HHDNALGORITHM_H
#include "GameLogic.h"
#include <string.h>
#include "stdlib.h"
#include "public/StdRandom.h"

enum bettinggate
{
    HeGate = 0,
    BalckGate = 1,
    RedGate = 2,
	NiuOneGate = 3,
	NiuTwoGate = 4,
	NiuThreeGate = 5,
	NiuFourGate = 6,
	NiuFiveGate = 7,
	NiuSixGate = 8,
	NiuSevenGate = 9,
	NiuEightGate = 10,
	NiuNineGate = 11,
	NiuNiuGate = 12,
	NiuSpecialGate = 13,
    MaxGate = 14
};

//const int mutipleArr[MaxGate] = { 5 ,1, 1, 6, 6, 6, 6 ,6 ,6 ,6 ,6 ,6 ,6 ,500 };

#define MAX_CARDS                  52
#define Denominator                10000
//#define TESTECARD				   TRUE
#define RANDCARD_MAXCOUNT          200  //最大洗牌次数
class HhdnAlgorithm
{
public:
    HhdnAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int64_t  m_thresholdNum;//阈值设定

     int  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)

     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     CGameLogic  m_pLogic;

     int  resultProbility;

     int64_t bettingQuantity[MaxGate];//十四门押注


     uint8_t						 m_TableCardType[2]; //
     uint8_t						 m_winType;
     uint8_t						 m_winFlag;
     uint8_t						 m_loseFlag;
     uint8_t						 m_wincardValue;

	 double						     m_muticleArr[MaxGate];
     static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
     uint8_t                         m_cbTableCardArray[2*MAX_COUNT];                   //桌面扑克
     CGameLogic                      m_GameLogic;						//游戏逻辑

     int64_t                         m_lcurrentProfit;                  //当局游戏赢分

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
         if(index<0||index>=MaxGate)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,int64_t curr,float ptobali, double mularr[],int64_t threshold=5000)
     {
         m_thresholdNum =threshold;
         m_highLimit = high;
         m_lowLimit = low;
         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         m_lcurrentProfit =0 ;
         memcpy(m_muticleArr,mularr,sizeof(m_muticleArr));

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
     inline int64_t GetCurProfit()
     {
         return m_lcurrentProfit;
     }
     double OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个干涉概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(uint8_t handcardarr[][MAX_COUNT],uint8_t & wintag,uint8_t & wintype);

     void BlackGetOpenCard(uint8_t handcardarr[][MAX_COUNT],uint8_t & wintag,uint8_t & wintype,float probilityAll[3]);
     void CallBackHandle();
     void openLog(const char *p, ...);
	 int64_t ConcludeWinScore(uint8_t handcardarr[2][MAX_COUNT], uint8_t & wintag, uint8_t & wintype);
	 //牌型转押注区域
	 int getAreaIndexByType(uint8_t cardType);
	 //检验手牌牌值是否重复或者为0
	 bool CheckHanCardIsOK(uint8_t handcardarr[], uint8_t cardcount);
};

#endif // HHDNALGORITHM_H
