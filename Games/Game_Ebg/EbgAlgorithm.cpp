#include "EbgAlgorithm.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

EbgAlgorithm::EbgAlgorithm()
{
    for(int i=0;i<3;i++)
        m_muticleArr[i]=2;
    miCurrentProfit=0;
	m_roomID = 0;
	m_tableID = 0;
}
const int8_t resultOpen[8][3]={{1,1,1},{1,0,1},{1,1,0},{1,0,0},{0,1,1},{0,0,1},{0,1,0},{0,0,0}};
void  EbgAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
{
    if(m_profitInterval==1)//上限以上就看概率,或者放分，或者随机
    {
        if(m_random.betweenInt(0,10000).randInt_mt(true)<resultProbility)
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
        if(m_random.betweenInt(0,10000).randInt_mt(true)<resultProbility)
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
int64_t EbgAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}
void EbgAlgorithm::CallBackHandle()
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
void  EbgAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "log//erbagang//erbagang%d%d%d_%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_roomID, m_tableID);
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
void EbgAlgorithm::BlackGetOpenCard(int8_t winTag[],int Length,float probirity[3])
{
    //公平百分之五十出随机结果
    int64_t betting=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    if(betting<=0)
    {
        return;
    }
    float proshun=0.0;
    float protian=0.0;
    float prodi  =0.0;
    proshun=0.5*(1-probirity[0]);
    protian=0.5*(1-probirity[1]);
    prodi  =0.5*(1-probirity[2]);


    //出红是否换牌概率
    openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);
    float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility<proshun)
    {
       winTag[0]=1;
    }
    else
    {
       winTag[0]=0;
    }
    pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility<protian)
    {
       winTag[1]=1;
    }
    else
    {
       winTag[1]=0;
    }
    pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility<prodi)
    {
       winTag[2]=1;
    }
    else
    {
       winTag[2]=0;
    }
    int64_t winscore=0;
    winscore =bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    for(int i=0;i<3;i++)
    {
        if(winTag[i])
        {
            winscore-=bettingQuantity[i]*m_muticleArr[i];
        }
    }
    miCurrentProfit=winscore;
   //做个是否有结果的验证
   if(m_killOrReword==1&&betting>0)
   {

        int countOpen=0;
        int textwinTag[3]={0};
        for(int i=0;i<8;i++)
        {
             for(int k=0;k<3;k++)
                 textwinTag[k]=resultOpen[i][k];
             long winscore1=0;
             winscore1 =bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
             for(int j=0;j<Length;j++)
             {
                 if(textwinTag[j])
                 {
                     winscore1-=bettingQuantity[j]*m_muticleArr[j];
                 }
             }
             if(winscore1<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2||winscore1>0)
             {
                countOpen++;
             }
         }
         if(countOpen>=8)//没有结果,那只能随机听天由命了
         {
             m_killOrReword=0;
         }
    }
   bool isOk=false;
   //if(m_killOrReword==1||m_killOrReword==-1)
   {
       do
       {

           isOk=false;

           //公平百分之五十出随机结果
           int64_t winscore1=0;
           winscore1 =bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i=0;i<Length;i++)
           {
               if(winTag[i])
               {
                   winscore1-=bettingQuantity[i]*m_muticleArr[i];
               }
           }
           miCurrentProfit=winscore1;
           if(m_killOrReword==-1)
           {
               if(winscore1<0)
               {
                   isOk=true;//系统杀分时，本次系统赢分小于零则继续
               }
           }
           if(m_killOrReword==1)
           {
               if(winscore1>0)
               {
                   isOk=true;//系统放分时，本次盈利大于零则继续
               }
               if(winscore1<-m_thresholdNum&&winscore1<-(m_curProfit-m_lowLimit)/2)
               {
                   isOk=true;//系统放分时，超过红线则继续
               }
           }
           if(winscore1<-m_thresholdNum&&winscore1<-(m_curProfit-m_lowLimit)/2)
           {
               isOk=true;//系统随机时，超过红线则继续
           }
           if(isOk)
           {
               for(int i=0;i<Length;i++)
                   winTag[i]=m_random.betweenInt(0, 1).randInt_mt(true);
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }


}
void EbgAlgorithm::GetOpenCard(int8_t winTag[],int Length)
{
    for(int i=0;i<Length;i++)
      winTag[i]=0;//每门的赢输结果，-1是输，1是赢
    bool isOk=false;



   int64_t pushin=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
   //做个是否有结果的验证
   if(m_killOrReword==1&&pushin>0)
   {

        int countOpen=0;
        for(int i=0;i<8;i++)
        {
             for(int k=0;k<3;k++)
                 winTag[k]=resultOpen[i][k];
             int64_t winscore=0;
             winscore =bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
             for(int j=0;j<Length;j++)
             {
                 if(winTag[j])
                 {
                     winscore-=bettingQuantity[j]*m_muticleArr[j];
                 }
             }
             if(winscore<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2||winscore>0)
             {
                countOpen++;
             }
         }
         if(countOpen>=8)//没有结果,那只能随机听天由命了
         {
             m_killOrReword=0;
         }
    }
    do
    {

        isOk=false;
        for(int i=0;i<Length;i++)
            winTag[i]=m_random.betweenInt(0, 1).randInt_mt(true);
        //公平百分之五十出随机结果
        int64_t winscore=0;
        winscore =bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
        if(winscore==0)//没有人押分的时候纯随机
        {
            break;
        }
        for(int i=0;i<Length;i++)
        {
            if(winTag[i])
            {
                winscore-=bettingQuantity[i]*m_muticleArr[i];
            }
        }
        miCurrentProfit=winscore;
        if(m_killOrReword==-1)
        {
            if(winscore<0)
            {
                isOk=true;//系统杀分时，本次系统赢分小于零则继续
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
void EbgAlgorithm::CalculationResults()
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
double EbgAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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

