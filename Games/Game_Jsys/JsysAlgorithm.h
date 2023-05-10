#ifndef JSYSALGORITHM_H
#define JSYSALGORITHM_H

#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"
/*
* 位置是15-16-17是兔子， 0
* 位置是19-20-21是燕子，1
* 位置是12-13是猴子，2
* 位置是23-24是鸽子，3
* 位置是9-10是熊猫，4
* 位置是26-27是孔雀，5
* 位置5-6-7是狮子，6
* 位置1-2-3是老鹰，7
* 位置0-8-14-22是银鲨，8
* 位置4-11-18-25是金鲨，9
*/
enum class AnimType
{
    eRabbit=0,
    eSwallow,
    eMonkey,
    ePigeon,
    ePanda,
    ePeacock,
    eLion,
    eEagle,
    eSilverSharp,
    eGoldSharp,
};

enum bettinggate
{
    eRabbit=0,    //兔子
    eSwallow,     //燕子
    eMonkey,      //猴子
    ePigeon,      //鸽子
    ePanda,       //熊猫
    ePeacock,     //孔雀
    eLion,        //狮子
    eEagle,       //老鹰
    eSilverSharp, //银鲨
    eGoldSharp,   //金鲨
    eFeiQin,
    eZouShou,
    MaxGate
};

#define MAX_CARDS                  52
#define Denominator 10000
class JsysAlgorithm
{
public:
    JsysAlgorithm();

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
         for(int i=0;i<12;i++)
          m_muticleArr[i]=mularr[i];

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
	 //设置赠送的动物[sdId]和开金鲨是的彩金倍数[mulcj]
	 inline void SetSendOdds(int sdId, int mulcj)
	 {
		 m_iwinTag_sd = sdId;
		 m_iwinTagMul_cj = mulcj;
	 }
	 double OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(int &winTag,float probirity[]);
     void BlackGetOpenCard(int &winTag,float probirity[]);
     void CallBackHandle();

     void  openLog(const char *p, ...);
     int64_t CurrentProfit();//计算本局不算税的系统赢输

	 int	 m_iwinTag_sd;			//若开金鲨或银鲨赠送的动物类型
	 int	 m_iwinTagMul_cj;		//开金鲨赠送的彩金倍数

};

#endif // JSYSALGORITHM_H
