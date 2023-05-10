#ifndef TBALGORITHM_H
#define TBALGORITHM_H

#include <string.h>
#include <vector>
#include "stdlib.h"
#include "stdint.h"
#include "public/StdRandom.h"
#include "GameLogic.h"

const float hhmfMulti[4]={4,4,3.8,3.8};
using namespace std;
// 黑，0
// 红，1
// 梅，2
// 方，3
// 王，4
// 大，5
// 小，6
// 单，7
// 双，8
// 红色，9
// 黑色，10
// seven，11

enum bettinggate
{
    eHei=0,
    eHong,
    eMei,
    eFang,
    eWang,
    eDa,
    eXiao,
    eDan,
    eShuang,
    eHongse,
    eHeise,
    eSeven,
    eMax
};
struct strResultInfo
{
    strResultInfo()
    {
        icard =0;
        for(int i=0;i<eMax;i++)
        {
            winList[i] = false;
            muticlList[i] = 0;
             bReturnMoney[i] =false;
        }
        iHonghei = 0;
        iDanshuang = 0;
        iDaxiao = 0;
        iHhmf =0;
    }
    void reset()
    {
        icard =0;
        for(int i=0;i<eMax;i++)
        {
            winList[i] = false;
            muticlList[i] = 0;
            bReturnMoney[i] =false;
        }
        iHonghei = 0;
        iDanshuang = 0;
        iDaxiao = 0;
        iHhmf =0;
    }
    void setTouList(uint8_t card)
    {
        reset();
        icard = card;
        SetWinMuticList(); //设置骰子的时候，顺便计算各项输赢情况
    }
    void   SetWinMuticList()
    {
        //方块和红桃时红色 红色 0 黑色 1  王  2
        int32_t colorIndex = logic.GetCardColor(icard);
        if(colorIndex==0||colorIndex==2)
        {
            iHonghei = 0;
            iHhmf = logic.GetCardColor(icard);
            winList[eHongse] =true;
            muticlList[eHongse] =1;

            winList[3-colorIndex] =true;
            muticlList[3-colorIndex] =hhmfMulti[colorIndex]-1;
        }//梅花和黑桃是黑色
        else if(colorIndex==1||colorIndex==3)
        {
            iHonghei = 1;
            winList[eHeise] =true;
            muticlList[eHeise] =1;
            iHhmf = logic.GetCardColor(icard);
            winList[3-colorIndex] =true;
            muticlList[3-colorIndex] =hhmfMulti[colorIndex]-1;
        }
        else //王
        {
            iHonghei = 2;
            iHhmf = colorIndex;
            winList[colorIndex] =true;
            muticlList[colorIndex] =19;


        }

        //单双 单 0  双  1  seven  2   王  3
        if(logic.GetCardLogicValue(icard)>13)//王牌
        {
            iDanshuang =3;
            iDaxiao =3;
            bReturnMoney[eHei] = true;
            bReturnMoney[eHong] = true;
            bReturnMoney[eMei] = true;
            bReturnMoney[eFang] = true;

        }
        else if(logic.GetCardLogicValue(icard)==7)
        {
            iDanshuang =2;
            iDaxiao =2;
            winList[eSeven] =true;
            muticlList[eSeven] =12;

            bReturnMoney[eDa] = true;
            bReturnMoney[eXiao] = true;
            bReturnMoney[eDan] = true;
            bReturnMoney[eShuang] = true;
        }
        else
        {
            if(logic.GetCardLogicValue(icard)%2==0) //双
            {
                iDanshuang =1;
                winList[eShuang] =true;
                muticlList[eShuang] =1;
            }
            else
            {
                iDanshuang =0;
                winList[eDan] =true;
                muticlList[eDan] =1;
            }
            if(logic.GetCardLogicValue(icard)>7) //双
            {
                iDaxiao =0;
                winList[eDa] =true;
                muticlList[eDa] =1;
            }
            else
            {
                iDaxiao =1;
                winList[eXiao] =true;
                muticlList[eXiao] =1;
            }
        }
    }
    CGameLogic  logic;
    uint8_t icard;
    bool    winList[eMax];
    float muticlList[eMax];
    bool    bReturnMoney[eMax];
    int32_t iHonghei;
    int32_t iDanshuang;
    int32_t iDaxiao;
    int32_t iHhmf;
};

#define MAX_CARDS                  54*8
#define Denominator 10000
class HhmfAlgorithm
{
public:
    HhmfAlgorithm();

    void SetGameRoomName(string name)
    {
        gameRoomName=name;
    }

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

     void GetOpenCard(strResultInfo &winTag,vector<uint8_t> &m_CardVec);
     void TestOpenCard(strResultInfo &winTag,int32_t testArr[]);
     void BlackGetOpenCard(strResultInfo &winTag,float killpro,int64_t balckerbet[],int64_t systemlose,vector<uint8_t> &m_CardVec);
     void CallBackHandle();

     void  openLog(const char *p, ...);
     int64_t CurrentProfit();//计算本局不算税的系统赢输

     string gameRoomName;

};

#endif //
