#include "redblackalgorithm.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
uint8_t RedBlackAlgorithm::m_cbTableCard[MAX_CARDS] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
};
RedBlackAlgorithm::RedBlackAlgorithm()/*:timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(),"hhe"))*/
{
//    timelooper->startLoop();

//    timelooper->getLoop()->runEvery(1,bind(&RedBlackAlgorithm::CallBackHandle,this));
//    profit=25000;
//   zongjin=25000;
    m_lcurrentProfit=0;
	m_roomID = 0;
	m_tableID = 0;
}

void  RedBlackAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
{
    if(m_profitInterval==1)//上限以上就看概率,或者放分，或者随机
    {
        if(RandomInt(0,10000)<resultProbility)
        {
            m_killOrReword = 1;
        }
        else
        {
            m_killOrReword = 0;
        }
    }
    else if(m_profitInterval==-1)//目前盈利在下限以下，看概率，或者杀分，或者随机
    {
        if(RandomInt(0,10000)<resultProbility)
        {
            m_killOrReword = -1;
        }
        else
        {
            m_killOrReword = 0;
        }
    }
    else
    {
        m_killOrReword = 0;
    }
}
void RedBlackAlgorithm::CallBackHandle()
{
//    SetBetting(0 ,RandomInt(200,2000));
//    SetBetting(1 ,RandomInt(200,2000));
//    SetBetting(2 ,RandomInt(200,2000));
//    SetAlgorithemNum(50000,0, profit,0.45, 5000);
//    //测试算法的
//    uint8_t aaa[2][3];
//    GetOpenCard(aaa);
//    CalculationResults();
    //LOG_INFO << "win   --------------------"<<ifWinProfit;
}
void  RedBlackAlgorithm::openLog(const char *p, ...)
{

    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/honghei/HongHei_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_roomID, m_tableID);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}
void RedBlackAlgorithm::BlackGetMustKillOpenCard(uint8_t handcardarr[][3],uint8_t & wintag,uint8_t & wintype,float probirity[3],int64_t userJetton[3] )
{
    bool isxingYunYiJi=false;
    bool ishongOrHei=false;
    float pribiHong=0.0f;
    float pribiHei=0.0f;
    //计算幸运一击门是否换牌
    if(m_random.betweenFloat(0.0,1.0).randFloat_mt(true)<probirity[0])
    {
        isxingYunYiJi=true;
    }
    pribiHong=0.5*(1-probirity[2]);
    pribiHei=0.5*(1-probirity[1]);
    float kaihong=pribiHong/(pribiHong+pribiHei);
    float kaihei=pribiHei/(pribiHong+pribiHei);
    //出红是否换牌概率
    openLog("黑名单控制开黑的概率:%f    黑名单控制开红的概率:%f",kaihei,kaihong);
    if(m_random.betweenFloat(0.0,1.0).randFloat_mt(true)<kaihei)
    {
        ishongOrHei=true;//换牌那就是出嘿
        openLog("黑名单要求出黑哦");
    }
    //开黑
    if(userJetton[1]>userJetton[2])
    {
        isxingYunYiJi=false;
        ishongOrHei =false;//换牌那就是出嘿
    }
    else
    {
        isxingYunYiJi=false;
        ishongOrHei= true;//换牌那就是出嘿
    }
    uint8_t tmpcbTableCardArray[2][3];
    do
    {

        m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));//打乱扑克
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));//随机发6张扑克
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        memcpy(handcardarr, tmpcbTableCardArray, sizeof(handcardarr));
        uint8_t  heiType =   m_GameLogic.GetCardType(tmpcbTableCardArray[BalckGate-1], 3);
        uint8_t  hongType=   m_GameLogic.GetCardType(tmpcbTableCardArray[RedGate-1], 3);
        m_TableCardType[BalckGate-1] = heiType;
        m_TableCardType[RedGate-1] = hongType;
        uint8_t heiCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BalckGate-1], 3,heiType);
        uint8_t hongCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RedGate-1],3, hongType);

        if(m_GameLogic.CompareCard(tmpcbTableCardArray[BalckGate-1],tmpcbTableCardArray[RedGate-1],3))
        {
            m_winType=heiType;
            m_winFlag=BalckGate;
            m_loseFlag=RedGate;
            wintag=BalckGate;
            wintype=heiType;
            if(m_winType==1&&heiCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }
        else
        {
            m_winType=hongType;
            m_winFlag=RedGate;
            m_loseFlag= BalckGate;
            wintag=RedGate;
            wintype=hongType;
            if(m_winType==1&&hongCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }


        long winscore=0;

        winscore +=bettingQuantity[m_loseFlag]-bettingQuantity[m_winFlag]*m_muticleArr[0];
        if(m_winType>0)
        {
            winscore-=bettingQuantity[LuckyGate]*m_muticleArr[m_winType];
        }
        else
        {
            winscore+=bettingQuantity[LuckyGate];
        }

        m_lcurrentProfit=winscore;

        //算法要求出黑
        if(ishongOrHei)
        {
            if(m_GameLogic.CompareCard(tmpcbTableCardArray[BalckGate-1],tmpcbTableCardArray[RedGate-1],3))
            {
                if(m_winType==1&&heiCard<9)
                {
                    m_winType=CT_SINGLE;
                }
                if(isxingYunYiJi&&((m_winType==1&&heiCard>=9)||m_winType>1))
                {

                }
                else
                {
                     openLog("黑名单控制已经开了黑的牌");
                    break;
                }

            }

        }
        else
        {
            if(m_GameLogic.CompareCard(tmpcbTableCardArray[RedGate-1],tmpcbTableCardArray[BalckGate-1],3))
            {
                if(m_winType==1&&hongCard<9)
                {
                    m_winType=CT_SINGLE;
                }
                if(isxingYunYiJi&&((m_winType==1&&heiCard>=9)||m_winType>1))
                {

                }
                else
                {
                    openLog("黑名单控制已经开了红的牌");
                    break;
                }

            }
        }

    }while(true);
}
void RedBlackAlgorithm::BlackGetOpenCard(uint8_t handcardarr[][3],uint8_t & wintag,uint8_t & wintype,float probirity[3])
{

    bool isxingYunYiJi=false;
    bool ishongOrHei=false;
    float pribiHong=0.0f;
    float pribiHei=0.0f;
    //计算幸运一击门是否换牌
    if(m_random.betweenFloat(0.0,1.0).randFloat_mt(true)<probirity[0])
    {
        isxingYunYiJi=true;
    }
    pribiHong=0.5*(1-probirity[2]);
    pribiHei=0.5*(1-probirity[1]);
    float kaihong=pribiHong/(pribiHong+pribiHei);
    float kaihei=pribiHei/(pribiHong+pribiHei);
    //出红是否换牌概率
    openLog("黑名单控制开黑的概率:%f    黑名单控制开红的概率:%f",kaihei,kaihong);
    if(m_random.betweenFloat(0.0,1.0).randFloat_mt(true)<kaihei)
    {
        ishongOrHei=true;//换牌那就是出嘿
        openLog("黑名单要求出黑哦");
    }
    uint8_t tmpcbTableCardArray[2][3];
    do
    {
Nunber0:
        m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));//打乱扑克
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));//随机发6张扑克
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        memcpy(handcardarr, tmpcbTableCardArray, sizeof(handcardarr));
        uint8_t  heiType =   m_GameLogic.GetCardType(tmpcbTableCardArray[BalckGate-1], 3);
        uint8_t  hongType=   m_GameLogic.GetCardType(tmpcbTableCardArray[RedGate-1], 3);
        m_TableCardType[BalckGate-1] = heiType;
        m_TableCardType[RedGate-1] = hongType;
        uint8_t heiCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BalckGate-1], 3,heiType);
        uint8_t hongCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RedGate-1],3, hongType);

        if(m_GameLogic.CompareCard(tmpcbTableCardArray[BalckGate-1],tmpcbTableCardArray[RedGate-1],3))
        {
            m_winType=heiType;
            m_winFlag=BalckGate;
            m_loseFlag=RedGate;
            wintag=BalckGate;
            wintype=heiType;
            if(m_winType==1&&heiCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }
        else
        {
            m_winType=hongType;
            m_winFlag=RedGate;
            m_loseFlag= BalckGate;
            wintag=RedGate;
            wintype=hongType;
            if(m_winType==1&&hongCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }


        long winscore=0;

        winscore +=bettingQuantity[m_loseFlag]-bettingQuantity[m_winFlag]*m_muticleArr[0];
        if(m_winType>0)
        {
            winscore-=bettingQuantity[LuckyGate]*m_muticleArr[m_winType];
        }
        else
        {
            winscore+=bettingQuantity[LuckyGate];
        }

        m_lcurrentProfit=winscore;
        if(m_killOrReword==-1)
        {
            if(winscore<0)
            {
                goto Nunber0;//系统杀分时，本次赢分小于零则继续

            }
            openLog("系统要求杀分了，黑名单暂且放你一马");
            break;
        }
        if(m_killOrReword==1)
        {
            if(winscore>0)
            {
                goto Nunber0;//系统放分时，本次盈利大于零则继续
            }
            if(winscore<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
            {
                goto Nunber0;//系统放分时，超过红线则继续
            }
            openLog("系统要放分，黑名单我不管你");
            break;
        }

        //算法要求出黑
        if(ishongOrHei)
        {
            if(m_GameLogic.CompareCard(tmpcbTableCardArray[BalckGate-1],tmpcbTableCardArray[RedGate-1],3))
            {
                if(m_winType==1&&heiCard<9)
                {
                    m_winType=CT_SINGLE;
                }
                if(isxingYunYiJi&&((m_winType==1&&heiCard>=9)||m_winType>1))
                {

                }
                else
                {
                     openLog("黑名单控制已经开了黑的牌");
                    break;
                }

            }

        }
        else
        {
            if(m_GameLogic.CompareCard(tmpcbTableCardArray[RedGate-1],tmpcbTableCardArray[BalckGate-1],3))
            {
                if(m_winType==1&&hongCard<9)
                {
                    m_winType=CT_SINGLE;
                }
                if(isxingYunYiJi&&((m_winType==1&&heiCard>=9)||m_winType>1))
                {

                }
                else
                {
                    openLog("黑名单控制已经开了红的牌");
                    break;
                }

            }
        }

    }while(true);


}
void RedBlackAlgorithm::GetOpenCard(uint8_t handcardarr[][3],uint8_t & wintag,uint8_t & wintype)
{
    uint8_t tmpcbTableCardArray[2][3];
    bool isOk=false;
    do
    {
        isOk=false;
        m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));//打乱扑克
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));//随机发6张扑克
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        memcpy(handcardarr, tmpcbTableCardArray, sizeof(handcardarr));
        uint8_t  heiType =   m_GameLogic.GetCardType(tmpcbTableCardArray[BalckGate-1], 3);
        uint8_t  hongType=   m_GameLogic.GetCardType(tmpcbTableCardArray[RedGate-1], 3);
        m_TableCardType[BalckGate-1] = heiType;
        m_TableCardType[RedGate-1] = hongType;
        uint8_t heiCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[BalckGate-1], 3,heiType);
        uint8_t hongCard = m_GameLogic.GetCardLevelValue(tmpcbTableCardArray[RedGate-1],3, hongType);

        if(m_GameLogic.CompareCard(tmpcbTableCardArray[BalckGate-1],tmpcbTableCardArray[RedGate-1],3))
        {
            m_winType=heiType;
            m_winFlag=BalckGate;
            m_loseFlag=RedGate;
            wintag=BalckGate;
            wintype=heiType;
            if(m_winType==1&&heiCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }
        else
        {
            m_winType=hongType;
            m_winFlag=RedGate;
            m_loseFlag= BalckGate;
            wintag=RedGate;
            wintype=hongType;
            if(m_winType==1&&hongCard<9)
            {
                m_winType=CT_SINGLE;
            }
        }
        int64_t winscore=0;

        winscore +=bettingQuantity[m_loseFlag]-bettingQuantity[m_winFlag]*m_muticleArr[0];
        if(m_winType>0)
        {
            winscore-=bettingQuantity[LuckyGate]*m_muticleArr[m_winType];
        }
        else
        {
            winscore+=bettingQuantity[LuckyGate];
        }

        m_lcurrentProfit=winscore;
        if(m_killOrReword==-1)
        {
            if(winscore<0)
            {
                isOk=true;//系统杀分时，本次赢分小于零则继续
            }
        }
        else if(m_killOrReword==1)
        {
            if(winscore>0)
            {
                isOk=true;//系统放分时，本次盈利大于零则继续
            }
            if(winscore<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
            {
                isOk=true;//系统放分时，超过红线则继续
            }
        }
        else
        {
            if(winscore<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
            {
                isOk=true;//随机时，超过红线则继续
            }
        }
    }while(isOk);//系统要放分的时候，本次系统盈利要大于零
}
#ifdef Test_Algorithm
void RedBlackAlgorithm::CalculationResults()
{


    long thisprofit=0;
    zongjin+=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    thisprofit+=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    zongchu +=bettingQuantity[m_winFlag]*2;
    thisprofit-=bettingQuantity[m_winFlag]*2;
    if(m_winType>0)
    {
        zongchu+=bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
        thisprofit-=bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
    profit=zongjin-zongchu;
    LOG_INFO<<"-----------------------------------------------------------------";
    LOG_INFO<<"总进===="<<zongjin;
    LOG_INFO<<"总出===="<<zongchu;
    LOG_INFO<<"盈利===="<<profit;
    LOG_INFO<<"本次盈利是===="<<thisprofit;
    if(m_winFlag==BalckGate)
    {
        LOG_INFO<<"本次是----黑赢---押注是"<<bettingQuantity[BalckGate]<<"-----本利是"<<bettingQuantity[BalckGate]*2;
    }
    if(m_winFlag==RedGate)
    {
        LOG_INFO<<"本次是----红赢---押注是"<<bettingQuantity[RedGate]<<"-----本利是"<<bettingQuantity[RedGate]*2;
    }
    if(m_winType==0)
    {
        LOG_INFO<<"本次幸运一击的牌型是{单牌}-------押注是"<<bettingQuantity[LuckyGate];
    }
    else if(m_winType==1)
    {
        LOG_INFO<<"本次幸运一击的牌型是{对子}-------押注是"<<bettingQuantity[LuckyGate]<<"-----本利是"<<bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==2)
    {
        LOG_INFO<<"本次幸运一击的牌型是{顺子}-------押注是"<<bettingQuantity[LuckyGate]<<"-----本利是"<<bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==3)
    {
        LOG_INFO<<"本次幸运一击的牌型是{金花}-------押注是"<<bettingQuantity[LuckyGate]<<"-----本利是"<<bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==4)
    {
        LOG_INFO<<"本次幸运一击的牌型是{顺金}-------押注是"<<bettingQuantity[LuckyGate]<<"-----本利是"<<bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==5)
    {
        LOG_INFO<<"本次幸运一击的牌型是{豹子}-------押注是"<<bettingQuantity[LuckyGate]<<"-----本利是"<<bettingQuantity[LuckyGate]*(mutipleArr[m_winType]+1);
    }
}
#else

#endif
double RedBlackAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
{
    if(m_curProfit>m_highLimit)//超出了上限
    {
        m_profitInterval = 1;
        int64_t resh=m_curProfit-m_highLimit;
        int64_t resl=m_highLimit-m_lowLimit;
        double dpro=m_probability*2*(double)resh/(double)resl;
        return dpro<m_probability*1?dpro:m_probability*1;
    }
    else if(m_curProfit<m_lowLimit)//超出了下限
    {
        m_profitInterval = -1;
        int64_t resh=m_lowLimit-m_curProfit;
        int64_t resl=m_highLimit-m_lowLimit;
        double dpro=m_probability*2*(double)(resh)/(double)resl;
        return dpro<m_probability*1?dpro:m_probability*1;
    }
    else//在两者之间不干涉
    {
        m_profitInterval = 0;
        return 0.0f;//在两者之间不干涉
    }
}

