#ifndef REDBLACKALGORITHM_H
#define REDBLACKALGORITHM_H
#include "GameLogic.h"
#include "stdlib.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Logging.h"

#define Test_Algorithm
using namespace std;
using namespace muduo;
using namespace muduo::net;
enum bettinggate
{
    LuckyGate=0,
    BalckGate=1,
    RedGate=2,
    MaxGate=3
};


#define danpaiMultiple  1
#define duiziMultiple   1
#define shunziMultiple  2
#define jinhuaMultiple  3
#define shunjinMultiple 5
#define baoziMultiple   10

const int mutipleArr[6]={1,1,2,3,5,10};

////扑克类型
//#define CT_SINGLE					0									//单牌类型
//#define CT_DOUBLE					1									//对子类型
//#define	CT_SHUN_ZI					2									//顺子类型
//#define CT_JIN_HUA					3									//金花类型
//#define	CT_SHUN_JIN					4									//顺金类型
//#define	CT_BAO_ZI					5									//豹子类型
//#define CT_SPECIAL					6									//特殊类型

#define MAX_CARDS                  52
#define Denominator 10000


struct NodeMongo
{
    NodeMongo(int64_t id, int64_t Selft ,int64_t fatherid)
    {
        TeamContribution = 0;
        SelftContribution = Selft;
        fatherId = fatherid;
        myId = id;
    }
    int64_t myId;
    int64_t TeamContribution;
    int64_t SelftContribution;
    int64_t fatherId;
};
class RedBlackAlgorithm
{
public:
    RedBlackAlgorithm();

public:

     int64_t  m_highLimit;//上限
     int64_t  m_lowLimit;//下限
     int64_t m_curProfit;//当前库存
     float m_probability;//干涉比率
     int64_t  m_thresholdNum;//阈值设定

     int64_t  m_profitInterval;//当前利润所属区间 (0--上限和下限之间不做干预的区间)(1--是利润超越上限的区间，属于放水区间)(-1是当前利润低于下限的区间，属于杀分的区间)

     int  m_killOrReword;//通过算法出来的概率确定系统是杀呢还是放呢还是随机呢(1属于放水,-1属于杀分,0随机)
     CGameLogic  m_pLogic;

     int  resultProbility;

     long bettingQuantity[MaxGate];//三门押注


     uint8_t   m_TableCardType[2]; //
     uint8_t   m_winType;
     uint8_t   m_winFlag;
     uint8_t   m_loseFlag;
     uint8_t   m_wincardValue;

     static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
     uint8_t                         m_cbTableCardArray[6];                   //桌面扑克
     CGameLogic                      m_GameLogic;						//游戏逻辑


     shared_ptr <EventLoopThread>     timelooper;

#ifdef Test_Algorithm
     int64_t zongjin;
     int64_t zongchu;
     int64_t profit;

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
         return min+rand()%(max-min)+1;
     }
     inline void  SetBetting(int index ,long value)
     {
         if(index<0||index>=MaxGate)
              return;
         bettingQuantity[index]=value;
     }
     inline void  SetAlgorithemNum(int64_t high,int64_t low,long curr,float ptobali,int64_t threshold=5000)
     {
         m_thresholdNum =threshold;
         m_highLimit = high;
         m_lowLimit = low;
         m_curProfit = curr;
         m_probability = ptobali;
         resultProbility = 0;
         resultProbility=OverHighLowLimit()*Denominator; //算出当前盈利在哪个区间,并且算出在这个区间的控制概率10000为分母
         CalculatCotrol();   //计算出第一步是杀还是放还是随机resultProbility等于1或者等于0或者等于-1,

     }

     float OverHighLowLimit(); //库存高于上限或者库存低于下限时返回一个概率
     void  CalculatCotrol();   //计算出第一步是杀还是放还是随机

     void GetOpenCard(uint8_t handcardarr[][3]);

     void CallBackHandle();


     void Caculet();
     vector<NodeMongo> MogoNodes;
     //
     void NodeRecursion(NodeMongo &node,vector<NodeMongo> vec)
     {
         int count =0 ;
         for(int i=0;i<vec.size();i++)
         {
            if(MogoNodes[i].fatherId==node.myId)
            {
                NodeRecursion(MogoNodes[i],vec);
                node.TeamContribution+=(MogoNodes[i].SelftContribution+MogoNodes[i].TeamContribution);
            }
            else
            {
                count++;
                continue;
            }
         }
         if(count==vec.size())
         {
            return;
         }
     }

};

#endif // REDBLACKALGORITHM_H
