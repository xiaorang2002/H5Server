#ifndef SLWHALGORITHM_H
#define SLWHALGORITHM_H

#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"

/*
* 动物圈顺时针:
* 位置0-5-12-17是狮子，3
* 位置4-8-16-20是熊猫，2
* 位置2-6-10-13-15-18-22是猴子，1
* 位置1-3-7-9-11-14-19-21-23是兔子，0
*/

enum class AnimType
{
	eRedRabbit = 0,		//红兔子0
	eRedMonkey,			//红猴子1
	eRedPanda,			//红熊猫2
	eRedLion,			//红狮子3
	eGreenRabbit,		//绿兔子4
	eGreenMonkey,		//绿猴子5
	eGreenPanda,		//绿熊猫6
	eGreenLion,			//绿狮子7
	eYellowRabbit,		//黄兔子8
	eYellowMonkey,		//黄猴子9
	eYellowPanda,		//黄熊猫10
	eYellowLion,		//黄狮子11
	eAreaHe,			//区域和12
	eAreaZhuang,		//区域庄13
	eAreaXian,			//区域闲14
};

enum class OpenType
{
	eNormal = 0,		//正常开奖0
	ePiliShanDian,		//霹雳闪电1
	eSongDeng,			//送灯2
	eDaSanYuan,			//大三元3
	eDaSiXi,			//大四喜4
};

enum bettinggate
{
	eHe = 12,
	eZhuang = 13,
	eXian = 14,
};

#define MaxGate			15
#define MAX_CARDS       52
#define Denominator		10000


class SlwhAlgorithm
{
public:
    SlwhAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int64_t  m_thresholdNum;//阈值设定
     int  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)
     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     int  resultProbility;
     int64_t bettingQuantity[MaxGate];//门押注
     int32_t   m_muticleArr[MaxGate];
     STD::Random m_random;
     int64_t  miCurrentProfit;
	 float	m_stockCtrlRate; //库存量百分比，库存1/2用于保险
     int64_t m_lMustKillLimitScore;	//黑名单必杀押注门槛

public:
     inline void  SetBetting(int index ,long value)
     {
         if(index<0||index>=MaxGate)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,int64_t curr,float ptobali,int32_t mularr[],int64_t threshold=5000, float stockCtrlRate = 0.5f,int64_t KillLimitScore = 10000)
     {
         m_thresholdNum =threshold;
         m_highLimit = high;
         m_lowLimit = low;
         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         miCurrentProfit=0;
         for(int i = 0;i < MaxGate;i++)
          m_muticleArr[i]=mularr[i];

         resultProbility=OverHighLowLimit()*Denominator; //算出当前盈利在哪个区间,并且算出在这个区间的控制概率10000为分母
         CalculatCotrol();   //计算出第一步是杀还是放还是随机resultProbility等于1或者等于0或者等于-1,
		 m_stockCtrlRate = stockCtrlRate;
		 m_lMustKillLimitScore = KillLimitScore;
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
	 inline void SetSendOdds(int sdId)
	 {
		 m_iwinTag_sd = sdId;
	 }
	 double OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(int &winTag,float probirity[], int &typeTag, int tmpID[5]);
     void BlackGetOpenCard(int &winTag,float probirity[], int &typeTag, int tmpID[5]);
     void CallBackHandle();

     void  openLog(const char *p, ...);
     int64_t CurrentProfit();//计算本局不算税的系统赢输

	 void sortProbirityAndAnimal();	 //动物权重排序
	 long CalculatWinScore(int winTag,int typeTag);		//计算出临时赢分
     void BlackGetMustkillOpenCard_HZX(int &wintag, float probirity[], int64_t jetton[15], bool isMust = true);
	 void BlackGetOpenCard_HZX(int &wintag, float probirity[]);
	 void GetOpenCard_HZX(int &wintag);

	 //必杀黑名单玩家特殊处理
     bool KillBlackGetOpenCard(int &winTag, int &typeTag, int64_t userBetScore[]);


	 int	 m_iwinTag_sd;			  //结果送灯赠送的动物类型
	 float	 sortprobirity[MaxGate+1];//动物权重排序[0-11:动物的权重 12:霹雳闪电 13:送灯 14:大三元 15:大四喜]
	 int	 animalID[MaxGate+1];	  //动物ID排序
	 int     iResultId_tmp;			  //特殊开奖类型时选择的开奖结果	

};

#endif // SLWHALGORITHM_H
