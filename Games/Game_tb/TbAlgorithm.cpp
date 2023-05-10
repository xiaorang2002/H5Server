#include "TbAlgorithm.h"

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

TbAlgorithm::TbAlgorithm()
{
    for(int i=0;i<eMax;i++)
        m_muticleArr[i]=0;
    miCurrentProfit=0;
}
void  TbAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t TbAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}

void  TbAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "log//Toubao//jsys%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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
void TbAlgorithm::BlackGetOpenCard(strResultInfo &winTag ,float killpro,int64_t balckerbet[],int64_t systemlose)
{
    int64_t betting=0; 
    int64_t blackUserBet = 0;
    /////////////////////
    //随机产生三颗骰子
    int32_t point[3]={0,0,0};
    point[0] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
    point[1] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
    point[2] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
    winTag.setTouList(point);
    /////////////////////////
    //公平百分之五十出随机结果
    for(int i=0;i<eMax;i++)
    {
        betting+=bettingQuantity[i];
        blackUserBet+=balckerbet[i];
        openLog("黑名单每项下注是[%d]=:%d" ,i,bettingQuantity[i]);
    }
    if(betting<=0)
    {
        return;
    }


    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    long winscore=0;

    for(int i=0;i<eMax;i++)
    {
        winscore+=bettingQuantity[i];       
        if(winTag.winList[i])
        {
            winscore-=(winTag.muticlList[i]+1)*bettingQuantity[i];
        }
    }
    miCurrentProfit=winscore;
    bool isOk=false;
   //不出杀招的时候
   if(m_random.betweenFloat(0,1).randFloat_mt(true)>killpro)
   {
       do
       {

           isOk=false;

           //公平百分之五十出随机结果
           long winscore1=0;
           for(int i=0;i<eMax;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i=0;i<eMax;i++)
           {
               if(winTag.winList[i])
               {
                   winscore1-=(winTag.muticlList[i]+1)*bettingQuantity[i];
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
               point[0] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[1] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[2] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               winTag.setTouList(point);
           }


       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   else
   {
       int32_t count=0;
       do
       {

           isOk=false;
           //公平百分之五十出随机结果
           int64_t winscore1=0;
           int64_t userWinScore = 0;
           for(int i=0;i<eMax;i++)
           {
               winscore1+=bettingQuantity[i];
               userWinScore+=balckerbet[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i=0;i<eMax;i++)
           {
               winscore1-=(winTag.muticlList[i]+1)*bettingQuantity[i];
               if(winTag.winList[i])
               {
                   userWinScore-=(winTag.muticlList[i]+1)*balckerbet[i];
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
           if(m_killOrReword!=-1)
           {
               //玩家赢则重新开牌
               if(userWinScore<=0&&winscore1>-systemlose)
               {
                    isOk=true;
               }
           }
           count++;
           if(count>1000)
           {
                isOk = false;
                openLog("一千次没有结果，跳出循环");
           }
           if(isOk)
           {
               point[0] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[1] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[2] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               winTag.setTouList(point);
           }


       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }

}
void TbAlgorithm::TestOpenCard(strResultInfo &winTag,int32_t testArr[])
{
    int64_t betting=0;
    /////////////////////
    //随机产生三颗骰子
    int32_t point[3]={0,0,0};
    point[0] = testArr[0];
    point[1] = testArr[1];
    point[2] = testArr[2];
    openLog("测试用例出结果[0]=:%d" ,testArr[0]);
    openLog("测试用例出结果[1]=:%d" ,testArr[1]);
    openLog("测试用例出结果[2]=:%d" ,testArr[2]);
    winTag.setTouList(point);
    /////////////////////////
    //公平百分之五十出随机结果
    for(int i=0;i<eMax;i++)
    {
        betting+=bettingQuantity[i];

    }
    if(betting<=0)
    {
        return;
    }

    long winscore=0;
    for(int i=0;i<eMax;i++)
    {
        winscore+=bettingQuantity[i];
        if(winTag.winList[i])
        {
            winscore-=(winTag.muticlList[i]+1)*bettingQuantity[i];
        }
    }
    miCurrentProfit=winscore;

}
void TbAlgorithm::GetOpenCard(strResultInfo &winTag)
{
    int64_t betting=0;
    /////////////////////
    openLog("调用算法一次");
    //随机产生三颗骰子
    int32_t point[3]={0,0,0};
    point[0] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(false);
    point[1] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(false);
    point[2] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(false);
    winTag.setTouList(point);
    /////////////////////////
    //公平百分之五十出随机结果
    for(int i=0;i<eMax;i++)
    {
        betting+=bettingQuantity[i];
        openLog("正常出结果每项下注金额[%d]=:%d" ,i,bettingQuantity[i]);
    }
    if(betting<=0)
    {
        return;
    }


    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    long winscore=0;
    for(int i=0;i<eMax;i++)
    {
        winscore+=bettingQuantity[i];       
        if(winTag.winList[i])
        {
            winscore-=(winTag.muticlList[i]+1)*bettingQuantity[i];
        }
    }
    miCurrentProfit=winscore;
    int32_t count =0;
   bool isOk=false;
   {
       do
       {

           isOk=false;

           //公平百分之五十出随机结果
           long winscore1=0;
           for(int i=0;i<eMax;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i=0;i<eMax;i++)
           {
               if(winTag.winList[i])
               {
                   winscore1-=(winTag.muticlList[i]+1)*bettingQuantity[i];
               }

           }
           miCurrentProfit=winscore1;
           if(m_killOrReword==-1)
           {
               if(winscore1<0)
               {
                   isOk=true;//系统杀分时，本次系统赢分小于零则继续
               }
               openLog("正常出结果,但是库存在下限以下，所以要求出必杀结果");
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
               openLog("正常出结果,但是库存在上限以上，所以要求出必放结果");
           }
           if(winscore1<-m_thresholdNum&&winscore1<-(m_curProfit-m_lowLimit)/2)
           {
               isOk=true;//系统随机时，超过红线则继续
           }

           count++;
           if(count>1000)
           {
                isOk = false;
                openLog("一千次没有结果，跳出循环");
           }
           if(isOk)
           {
               point[0] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[1] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               point[2] = m_random.betweenInt(eOnePonit,eSixPonit).randInt_mt(true);
               winTag.setTouList(point);
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   openLog("黑名单经过限红判断后的结果是=%d  变了吗，没变没触洪线，变了那是触洪了",winTag);
}
double TbAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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
