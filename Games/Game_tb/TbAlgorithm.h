#ifndef TBALGORITHM_H
#define TBALGORITHM_H

#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"
// 大， 0
// 小， 1
// 单， 2
// 双， 3
// 全骰(任意一个点数的围骰)，4
// 1点对子，5
// 2点对子，6
// 3点对子，7
// 4点对子，8
// 5点对子，9
// 6点对子，10
// 1点围骰，11
// 2点围骰，12
// 3点围骰，13
// 4点围骰，14
// 5点围骰，15
// 6点围骰，16
// 和4点，17
// 和5点，18
// 和6点，19
// 和7点，20
// 和8点，21
// 和9点，22
// 和10点，23
// 和11点，24
// 和12点，25
// 和13点，26
// 和14点，27
// 和15点，28
// 和16点，29
// 和17点，30
// 1点三军，31
// 2点三军，32
// 3点三军，33
// 4点三军，34
// 5点三军，35
// 6点三军，36

enum TouPoint
{
    eOnePonit=1,
    eTwoPonit,
    eThreePonit,
    eFourPonit,
    eFivePonit,
    eSixPonit,
};
enum bettinggate
{
    eDa=0,
    eXiao,
    eDan,
    eShuang,
    eQuanTou,
    eDuizi1,
    eDuizi2,
    eDuizi3,
    eDuizi4,
    eDuizi5,
    eDuizi6,
    eWeiTou1,
    eWeiTou2,
    eWeiTou3,
    eWeiTou4,
    eWeiTou5,
    eWeiTou6,
    eDian4,
    eDian5,
    eDian6,
    eDian7,
    eDian8,
    eDian9,
    eDian10,
    eDian11,
    eDian12,
    eDian13,
    eDian14,
    eDian15,
    eDian16,
    eDian17,
    eSanjun1,
    eSanjun2,
    eSanjun3,
    eSanjun4,
    eSanjun5,
    eSanjun6,
    eMax
};
const int32_t muticallist[14]={60,30,18,12,8,7,6,6,7,8,12,18,30,60};

struct strResultInfo
{
    strResultInfo()
    {
        for(int i=0;i<3;i++)
        {
            touList[i] = 0;
        }
        for(int i=0;i<eMax;i++)
        {
            winList[i] = false;
            muticlList[i] = 0;
        }
        iDianshu = 0;
        iDanshuang = 0;
        iDaxiao = 0;
    }
    void reset()
    {
        for(int i=0;i<3;i++)
        {
            touList[i] = 0;
        }
        for(int i=0;i<eMax;i++)
        {
            winList[i] = false;
            muticlList[i] = 0;
        }
        iDianshu = 0;
        iDanshuang = 0;
        iDaxiao = 0;
    }
    void setTouList(int32_t list[])
    {
        reset();
        for(int i=0;i<3;i++)
        {
            touList[i] = list[i];
        }
        SetWinMuticList(); //设置骰子的时候，顺便计算各项输赢情况
    }
    void   SetWinMuticList()
    {
        for(int i=0;i<3;i++)
        {
            if(touList[i]<1||touList[i]>6)
            {
                return;
            }
        }
        //计算点数
        iDianshu = touList[0]+touList[1]+touList[2];
        //三个骰子一样
        if(touList[0]==touList[1]&&touList[1]==touList[2])
        {
            int32_t index = touList[0];
            int32_t dian = touList[0]+touList[1]+touList[2];

            winList[index+eDuizi1-1] = true;    //eDuizi1  5  开始
            winList[index+eWeiTou1-1] = true;   //eWeiTou1 11 开始
            winList[index+eSanjun1-1] = true;   //eSanjun1 31 开始
            winList[eQuanTou] = true;

            muticlList[index+eDuizi1-1] = 8;
            muticlList[index+eWeiTou1-1] = 150;
            muticlList[index+eSanjun1-1] = 3;
            muticlList[eQuanTou] = 30;
            if(dian>3&&dian<18)
            {
                winList[dian-4+eDian4] = true; //eDian4 17 开始
                muticlList[dian-4+eDian4] = muticallist[dian-4];
            }
            iDanshuang = 2;
            iDaxiao = 2;

         }//两个骰子一样
        else if((touList[0]==touList[1]&&touList[1]!=touList[2])||(touList[0]==touList[2]&&touList[2]!=touList[1])||(touList[1]==touList[2]&&touList[2]!=touList[0]))
        {
            int32_t index =touList[0];
            int32_t index1=touList[2];
            int32_t dian = touList[0]+touList[1]+touList[2];
            if(touList[1]==touList[2]&&touList[2]!=touList[0])
            {
                index = touList[1];
                index1 = touList[0];
            }
            if(touList[0]==touList[2]&&touList[2]!=touList[1])
            {
                index1 = touList[1];
            }

            if(dian>3&&dian<18)
            {
                winList[dian-4+eDian4] = true; //eDian4 17 开始
                muticlList[dian-4+eDian4] = muticallist[dian-4];
            }
            if(dian%2==0)
            {
                winList[eShuang] = true;
                muticlList[eShuang] = 1;
                iDanshuang = 1;

            }
            else
            {
                winList[eDan] = true;
                muticlList[eDan] = 1;
                iDanshuang = 0;
            }
            if(dian<11)
            {
                winList[eXiao] = true;
                muticlList[eXiao] = 1;
                iDaxiao = 1;
            }
            else
            {
                winList[eDa] = true;
                muticlList[eDa] = 1;
                iDaxiao = 0;
            }
            winList[index+eDuizi1-1] = true;        //eDuizi1   5 开始
            winList[index+eSanjun1-1] = true;       //eSanjun1  31开始
            muticlList[index+eDuizi1-1] = 8;        //eDuizi1   5 开始
            muticlList[index+eSanjun1-1] = 2;       //eSanjun1  31开始

            winList[index1+eSanjun1-1] = true;       //eSanjun1  31开始 另外一个中三军也算是中
            muticlList[index1+eSanjun1-1] = 1;       //eSanjun1  31开始

        }
        else //三个骰子都不一样
        {
            int32_t dian = touList[0]+touList[1]+touList[2];
            if(dian>3&&dian<18)
            {
                winList[dian-4+eDian4] = true; //eDian4 17 开始
                muticlList[dian-4+eDian4] = muticallist[dian-4];
            }
            if(dian%2==0)
            {
                winList[eShuang] = true;
                muticlList[eShuang] = 1;
                iDanshuang = 1;
            }
            else
            {
                winList[eDan] = true;
                muticlList[eDan] = 1;
                iDanshuang = 0;
            }
            if(dian<11)
            {
                winList[eXiao] = true;
                muticlList[eXiao] = 1;
                iDaxiao = 1;
            }
            else
            {
                winList[eDa] = true;
                muticlList[eDa] = 1;
                iDaxiao = 0;
            }

            winList[touList[0]+eSanjun1-1] = true;       //eSanjun1  31开始 另外一个中三军也算是中
            muticlList[touList[0]+eSanjun1-1] = 1;       //eSanjun1  31开始
            winList[touList[1]+eSanjun1-1] = true;       //eSanjun1  31开始 另外一个中三军也算是中
            muticlList[touList[1]+eSanjun1-1] = 1;       //eSanjun1  31开始
            winList[touList[2]+eSanjun1-1] = true;       //eSanjun1  31开始 另外一个中三军也算是中
            muticlList[touList[2]+eSanjun1-1] = 1;       //eSanjun1  31开始
        }
    }
    int32_t touList[3];
    bool    winList[eMax];
    int32_t muticlList[eMax];
    int32_t iDianshu;
    int32_t iDanshuang;
    int32_t iDaxiao;
};

#define MAX_CARDS                  52
#define Denominator 10000
class TbAlgorithm
{
public:
    TbAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int64_t  m_thresholdNum;//阈值设定
     int  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)
     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     int  resultProbility;
     int64_t bettingQuantity[eMax];//门押注
     int8_t   m_muticleArr[eMax];
     STD::Random m_random;
     int64_t  miCurrentProfit;

public:
     inline void  SetBetting(int index ,long value)
     {
         if(index<0||index>=eMax)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,int64_t curr,float ptobali,int64_t threshold=5000)
     {
         m_thresholdNum =threshold;
         m_highLimit = high;
         m_lowLimit = low;
         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         miCurrentProfit=0;

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

     void GetOpenCard(strResultInfo &winTag);
     void TestOpenCard(strResultInfo &winTag,int32_t testArr[]);
     void BlackGetOpenCard(strResultInfo &winTag,float killpro,int64_t balckerbet[],int64_t systemlose);
     void CallBackHandle();

     void  openLog(const char *p, ...);
     int64_t CurrentProfit();//计算本局不算税的系统赢输


};

#endif // JSYSALGORITHM_H
