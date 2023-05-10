#include "BcbmAlgorithm.h"

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

BcbmAlgorithm::BcbmAlgorithm()
{
    for(int i=0;i<8;i++)
        m_muticleArr[i]=5;
    miCurrentProfit=0;
}
const int8_t resultOpen[8][3]={{1,1,1},{1,0,1},{1,1,0},{1,0,0},{0,1,1},{0,0,1},{0,1,0},{0,0,0}};
void  BcbmAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
{
    if(m_profitInterval==1)//上限以上就看概率,或者放分，或者随机
    {
        if(m_random.betweenInt(0,10000).randInt_mt(true)<resultProbility)
        {
            m_killOrReword = 1;
            openLog("库存上限获得放分概率应当获取放分结果");
        }
        else
        {
            m_killOrReword = 0;
            openLog("库存上限NO获得放分概率应当获取随机结果");
        }
    }
    else if(m_profitInterval==-1)//目前盈利在下限以下，看概率，或者杀分，或者随机
    {
        if(m_random.betweenInt(0,10000).randInt_mt(true)<resultProbility)
        {
            m_killOrReword = -1;
            openLog("库存下限获得杀分概率应当获取杀分结果");
        }
        else
        {
            m_killOrReword = 0;
            openLog("库存下限NO获得杀分概率应当获取随机结果");
        }
    }
    else
    {
        m_killOrReword = 0;
        openLog("正常随机区间应当获取随机结果");
    }
}
int64_t BcbmAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}

void  BcbmAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "log//benzbmw//bcbm%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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
void BcbmAlgorithm::BlackGetOpenCard(int &winTag,float probirity[])
{
    int64_t betting=0;
    float wholeweight=0.0;
    //公平百分之五十出随机结果
    for(int i=0;i<8;i++)
    {
        betting+=bettingQuantity[i];
        wholeweight+=probirity[i];
         openLog("黑名单的权重[%d]=:%f" ,i,probirity[i]);
         openLog("黑名单每项下注是[%d]=:%d" ,i,bettingQuantity[i]);
    }
    for(int i=0;i<8;i++)
    {
        openLog("每项 的概率是[%d]=:%f" ,i,probirity[i]/wholeweight);
    }
    float weights[8]={10000};
    for(int i=0;i<8;i++)
        weights[i]=probirity[i];

    float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);

    float res=0.0;
    for(int i=0;i<8;i++)
    {
        res += weights[i];
        if(pribility-res<=0)
        {
            winTag=i;
            openLog("开出的结果=:%d" ,i);
            break;
        }
    }
    if(betting<=0)
    {
        return;
    }


    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    int64_t winscore=0;
    for(int i=0;i<8;i++)
    {
        winscore+=bettingQuantity[i];
    }
    winscore-=bettingQuantity[winTag]*(m_muticleArr[winTag]);
    miCurrentProfit=winscore;
   //做个是否有结果的验证
   if(m_killOrReword==1&&betting>0)
   {

        int countOpen=0;
        int textwinTag[3]={0};
        for(int i=0;i<8;i++)
        {
             long winscore1=0;
             for(int j=0;j<8;j++)
             {
                 winscore1+=bettingQuantity[j];
             }
             if(winscore1-bettingQuantity[i]*(m_muticleArr[i])<0&&winscore1>=-(m_curProfit-m_lowLimit)/2)
             {
                break;
             }
             else
             {
                 countOpen++;
             }
         }
         if(countOpen>=8)//没有结果,那只能随机听天由命了
         {
             openLog("没有结果啊我日,只能杀一下玩家了");
             m_killOrReword=-1;
         }
    }
   bool isOk=false;
   {
       do
       {

           isOk=false;

           //公平百分之五十出随机结果
           int64_t winscore1=0;
           for(int i=0;i<8;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           winscore1-=bettingQuantity[winTag]*(m_muticleArr[winTag]);
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
               if(winscore1<-(m_curProfit-m_lowLimit)/2)
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
               float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);

               float res=0.0;
               for(int i=0;i<8;i++)
               {
                   res += weights[i];
                   if(pribility-res<=0)
                   {
                       winTag=i;
                       break;
                   }
               }
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   openLog("黑名单经过限红判断后的结果是=%d  变了吗，没变没触洪线，变了那是触洪了",winTag);

}
void BcbmAlgorithm::GetOpenCard(int &winTag,float probirity[])
{
    int64_t betting=0;
    float wholeweight=0.0;
    //公平百分之五十出随机结果
    float weights[8]={10000};
    for(int i=0;i<8;i++)
    {
        betting+=bettingQuantity[i];
        wholeweight+=probirity[i];      
        weights[i]=probirity[i];
    }

    float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);

    float res=0.0;
    for(int i=0;i<8;i++)
    {
        res += weights[i];
        if(pribility-res<=0)
        {
            winTag=i;
            break;
        }
    }
    if(betting<=0)
    {
        return;
    }



    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    long winscore=0;
    for(int i=0;i<8;i++)
    {
        winscore+=bettingQuantity[i];
    }
    for(int i=0;i<8;i++)
    {
        if(winTag==i)
        {
            winscore-=bettingQuantity[i]*(m_muticleArr[i]);
            break;
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
             long winscore1=0;
             for(int i=0;i<8;i++)
             {
                 winscore1+=bettingQuantity[i];
             }
             if(winscore1-bettingQuantity[i]*(m_muticleArr[i])<0&&winscore1>=-(m_curProfit-m_lowLimit)/2)
             {
                break;
             }
             else
             {
                 countOpen++;
             }
         }
         if(countOpen>=8)//没有结果,那只能随机听天由命了
         {
             openLog("正常出结果，属于上限以上，而且决定刚给玩家放水，但是放水的项又会让玩家赢太多触洪，没结果只能杀一下玩家了");
             m_killOrReword=-1;
         }
    }
   bool isOk=false;
   {
       do
       {

           isOk=false;

           //公平百分之五十出随机结果
           long winscore1=0;
           for(int i=0;i<8;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           winscore1-=bettingQuantity[winTag]*(m_muticleArr[winTag]);
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
               if(winscore1<-(m_curProfit-m_lowLimit)/2)
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
               float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);
               float res=0.0;
               for(int i=0;i<8;i++)
               {
                   res += weights[i];
                   if(pribility-res<=0)
                   {
                       winTag=i;
                       break;
                   }
               }
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   openLog("不是黑名单情况下，正常出的结果:%d",winTag);
}
double BcbmAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
{   
    if(m_curProfit>m_highLimit)//超出了上限
    {
        m_profitInterval = 1;
        int64_t resh=m_curProfit-m_highLimit;
        int64_t resl=m_highLimit-m_lowLimit;
        double dpro=m_probability*2*(double)resh/(double)resl;
        openLog("库存超出了上限,控制系数是:%f", m_probability);
        openLog("库存超出了上限,控制系数算出的概率是:%f", dpro);
        return dpro<m_probability*1?dpro:m_probability*1;

    }
    else if(m_curProfit<m_lowLimit)//超出了下限
    {
        m_profitInterval = -1;
        int64_t resh=m_lowLimit-m_curProfit;
        int64_t resl=m_highLimit-m_lowLimit;
        double dpro=m_probability*2*(double)(resh)/(double)resl;
        openLog("库存超出了下限,控制系数是:%f", m_probability);
        openLog("库存超出了下限,控制系数算出的概率是:%f", dpro);
        return dpro<m_probability*1?dpro:m_probability*1;
    }
    else//在两者之间不干涉
    {
        m_profitInterval = 0;
        openLog("库存在正常区间,无控制");
        return 0.0f;//在两者之间不干涉
    }
}

