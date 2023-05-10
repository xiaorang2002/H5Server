#include "wxhhAlgorithm.h"
#include <vector>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
using namespace std;
wxhhAlgorithm::wxhhAlgorithm()
{
    for(int i=0;i<Max;i++)
        m_muticleArr[i]=1.0;
    miCurrentProfit=0;
}

void  wxhhAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t wxhhAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}

void  wxhhAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/wxhh/wxhhsf_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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

void wxhhAlgorithm::BlackGetOpenCard(int8_t &wintag,float probirity[5],bool hasWang)
{
    //公平百分之五十出随机结果
    int64_t betting=bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]
            +bettingQuantity[Fang]+bettingQuantity[Wang];
    if(betting<=0)
    {
        return;
    }
    float pribihei=0.0f;
    float pribihong=0.0f;
    float pribimei=0.0f;
    float pribifang=0.0f;
    float wangGailv=0.037;
    float huaGailv=0.963;
    if(hasWang)//扑克还有王牌
    {
        wangGailv=0.037;
        huaGailv=0.963;
    }
    else
    {
        wangGailv=0;
        huaGailv=1.0;
    }
    pribihei=10000*(1-probirity[Hei]);
    pribihong=10000*(1-probirity[Hong]);
    pribimei=10000*(1-probirity[Mei]);
    pribifang=10000*(1-probirity[Fang]);
    float kaiHei=huaGailv*pribihei/(pribihei+pribihong+pribimei+pribifang);
    float kaiHong=huaGailv*pribihong/(pribihei+pribihong+pribimei+pribifang);
    float kaiMei=huaGailv*pribimei/(pribihei+pribihong+pribimei+pribifang);
    float kaiFang=huaGailv*pribifang/(pribihei+pribihong+pribimei+pribifang);
    float kaiWang=wangGailv;
    //出红是否换牌概率
    openLog("黑名单控制开黑桃的概率:%f    黑名单控制开红桃的概率:%f     黑名单控制开梅花的概率:%f     黑名单控制开方块的概率:%f",kaiHei,kaiHong,kaiMei,kaiFang);
    float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility>huaGailv)
    {
        wintag=Wang;
        openLog("黑名单要求开王哦");
    }
    else
    {
        if(pribility<kaiHei)
        {
            wintag=Hei;
            openLog("黑名单要求开黑桃哦");
        }
        else if(pribility<kaiHei+kaiHong&&pribility>=kaiHei)
        {
            wintag=Hong;
            openLog("黑名单要求开红桃哦");
        }
        else if(pribility<kaiHei+kaiHong+kaiMei&&pribility>=kaiHei+kaiHong)
        {
            wintag=Mei;
            openLog("黑名单要求开梅花哦");
        }
        else
        {
             wintag=Fang;
             openLog("黑名单要求开方块哦");
        }
    }

    long winscore=0;
    if(wintag==Wang)
    {
        winscore =-bettingQuantity[Wang]*m_muticleArr[Wang];
    }
    else if(wintag==Hei)
    {
        winscore =-bettingQuantity[Hei]*m_muticleArr[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
    }
    else if(wintag==Hong)
    {
        winscore =-bettingQuantity[Hong]*m_muticleArr[Hong]+bettingQuantity[Hei]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
    }
    else if(wintag==Mei)
    {
        winscore =-bettingQuantity[Mei]*m_muticleArr[Mei]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Fang]+bettingQuantity[Wang];
    }
    else
    {
        winscore =-bettingQuantity[Fang]*m_muticleArr[Fang]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Wang];
    }
    miCurrentProfit=winscore;
    std::vector<int64_t> v_wintag;
    v_wintag.clear();
    if(m_killOrReword==1)
    {
        for(int i=0;i<5;i++)
        {
            int opentag=i;
            long winscore1=0;
            if(opentag==Wang)
            {
                winscore1 =-bettingQuantity[Wang]*m_muticleArr[Wang];
            }
            else if(opentag==Hei)
            {
                winscore1 =-bettingQuantity[Hei]*m_muticleArr[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(opentag==Hong)
            {
                winscore1 =-bettingQuantity[Hong]*m_muticleArr[Hong]+bettingQuantity[Hei]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(opentag==Mei)
            {
                winscore1 =-bettingQuantity[Mei]*m_muticleArr[Mei]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else
            {
                winscore1 =-bettingQuantity[Fang]*m_muticleArr[Fang]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Wang];
            }
            if(winscore1<0)
            {
                v_wintag.push_back(winscore1);
            }


        }
        int jishi =0;
        for(int i=0;i<v_wintag.size();i++)
        {
            if(v_wintag[i]>=-m_thresholdNum||v_wintag[i]>=-(m_curProfit-m_lowLimit)/2)
            {

            }
            else
            {
                jishi++;
            }
        }
        if(v_wintag.size()<=0||jishi==v_wintag.size())//没有结果或者结果都是不合法
        {
            m_killOrReword=0;
            openLog("黑名单受库存限制:库存要求放水，但是没有合适的结果");
        }
    }
    bool isOk=false;

    //if(m_killOrReword==-1||m_killOrReword==1)
    {
        do
        {
            isOk=false;
            //公平百分之五十出随机结果
            if(betting<=0)
            {
                break;
            }
            long winscore1=0;
            if(wintag==Wang)
            {
                winscore1 =-bettingQuantity[Wang]*m_muticleArr[Wang];
            }
            else if(wintag==Hei)
            {
                winscore1 =-bettingQuantity[Hei]*m_muticleArr[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(wintag==Hong)
            {
                winscore1 =-bettingQuantity[Hong]*m_muticleArr[Hong]+bettingQuantity[Hei]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(wintag==Mei)
            {
                winscore1 =-bettingQuantity[Mei]*m_muticleArr[Mei]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else
            {
                winscore1 =-bettingQuantity[Fang]*m_muticleArr[Fang]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Wang];
            }
            miCurrentProfit=winscore1;
            if(m_killOrReword==-1)
            {
                if(winscore1<0)
                {
                    isOk=true;//系统杀分时，本次系统赢分小于零则继续
                }
            }
            else if(m_killOrReword==1)
            {
                if(winscore1>0)
                {
                    isOk=true;//系统放分时，本次盈利大于零则继续
                }
                if(winscore1<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
                {
                    isOk=true;//系统放分时，超过红线则继续
                }
            }
            else
            {
                if(winscore1<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
                {
                    isOk=true;//系统放分时，超过红线则继续
                }

            }
            if(isOk)
            {
                int8_t card=0;
                if(hasWang)
                {
                    card=m_random.betweenInt(1,54).randInt_mt(true);
                    openLog("有王牌，走正常有王牌概率");
                }
                else
                {
                    card=m_random.betweenInt(1,52).randInt_mt(true);
                    openLog("没有王牌了");
                }
                if(card<=13)
                {
                    wintag=Hei;//和
                }
                else if(card>13&&card<=26)
                {
                    wintag=Hong;//龙
                }
                else if(card>26&&card<=39)
                {
                    wintag=Mei;//虎
                }
                else if(card>39&&card<=52)
                {
                    wintag=Fang;//虎
                }
                else
                {
                    wintag=Wang;//虎
                }
            }
            else
            {
                break;
            }

        }while(isOk);//系统要放分的时候，本次系统盈利要大于零
    }

}
void wxhhAlgorithm::GetOpenCard(int8_t &wintag,bool hasWang)
{

    bool isOk=false;
    std::vector<int64_t> v_wintag;
    v_wintag.clear();
    if(m_killOrReword==1)
    {
        for(int i=0;i<5;i++)
        {
            int opentag=i;
            int64_t betting=bettingQuantity[Hei]
                    +bettingQuantity[Hong]
                    +bettingQuantity[Mei]
                    +bettingQuantity[Fang]
                    +bettingQuantity[Wang];
            long winscore=0;
            if(opentag==Wang)
            {
                winscore =-bettingQuantity[Wang]*m_muticleArr[Wang];
            }
            else if(opentag==Hei)
            {
                winscore =-bettingQuantity[Hei]*m_muticleArr[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(opentag==Hong)
            {
                winscore =-bettingQuantity[Hong]*m_muticleArr[Hong]+bettingQuantity[Hei]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else if(opentag==Mei)
            {
                winscore =-bettingQuantity[Mei]*m_muticleArr[Mei]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Fang]+bettingQuantity[Wang];
            }
            else
            {
                winscore =-bettingQuantity[Fang]*m_muticleArr[Fang]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Wang];
            }
            if(winscore<0)
            {
                v_wintag.push_back(winscore);
            }


        }
        int jishi =0;
        for(int i=0;i<v_wintag.size();i++)
        {
            if(v_wintag[i]>=-m_thresholdNum||v_wintag[i]>=-(m_curProfit-m_lowLimit)/2)
            {

            }
            else
            {
                jishi++;
            }
        }
        if(v_wintag.size()<=0||jishi==v_wintag.size())//没有结果或者结果都是不合法
        {
            m_killOrReword=0;
            openLog("库存要求放水，但是没有合适的结果");
        }
    }

    do
    {
        isOk=false;
        wintag=m_random.betweenInt(0,3).randInt_mt(true);;
        int8_t card=0;
        if(hasWang)
        {
            card=m_random.betweenInt(1,54).randInt_mt(true);
            openLog("有王牌，走正常有王牌概率");
        }
        else
        {
            card=m_random.betweenInt(1,52).randInt_mt(true);
            openLog("没有王牌了");
        }
        if(card<=13)
        {
            wintag=Hei;//和
        }
        else if(card>13&&card<=26)
        {
            wintag=Hong;//龙
        }
        else if(card>26&&card<=39)
        {
            wintag=Mei;//虎
        }
        else if(card>39&&card<=52)
        {
            wintag=Fang;//虎
        }
        else
        {
            wintag=Wang;//虎
        }
        //公平百分之五十出随机结果
        int64_t betting=bettingQuantity[Hei]
                +bettingQuantity[Hong]
                +bettingQuantity[Mei]
                +bettingQuantity[Fang]
                +bettingQuantity[Wang];
        if(betting<=0)
        {
            break;
        }
        long winscore=0;
        if(wintag==Wang)
        {
            winscore =-bettingQuantity[Wang]*m_muticleArr[Wang];
        }
        else if(wintag==Hei)
        {
            winscore =-bettingQuantity[Hei]*m_muticleArr[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
        }
        else if(wintag==Hong)
        {
            winscore =-bettingQuantity[Hong]*m_muticleArr[Hong]+bettingQuantity[Hei]+bettingQuantity[Mei]+bettingQuantity[Fang]+bettingQuantity[Wang];
        }
        else if(wintag==Mei)
        {
            winscore =-bettingQuantity[Mei]*m_muticleArr[Mei]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Fang]+bettingQuantity[Wang];
        }
        else
        {
            winscore =-bettingQuantity[Fang]*m_muticleArr[Fang]+bettingQuantity[Hei]+bettingQuantity[Hong]+bettingQuantity[Mei]+bettingQuantity[Wang];
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

double wxhhAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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
        openLog("库存在正常区间,控制系数是");
        return 0.0f;//在两者之间不干涉
    }
}
