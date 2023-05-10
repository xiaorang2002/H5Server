#include "LongHuAlgorithm.h"
#include <vector>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
using namespace std;
LongHuAlgorithm::LongHuAlgorithm()
{
    for(int i=0;i<3;i++)
        m_muticleArr[i]=1;
    miCurrentProfit=0;
}

void  LongHuAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t LongHuAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}
void LongHuAlgorithm::CallBackHandle()
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
void  LongHuAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/longhu/longhu_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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

void LongHuAlgorithm::BlackGetOpenCard(int8_t &wintag,float probirity[3])
{
    //公平百分之五十出随机结果
    int64_t betting=bettingQuantity[He]+bettingQuantity[Long]+bettingQuantity[Hu];
    if(betting<=0)
    {
        return;
    }
    float pribilong=0.0f;
    float pribihu=0.0f;
    pribihu=0.46265*(1-probirity[2]);
    pribilong=0.46265*(1-probirity[1]);
    float kailong=0.9253*pribilong/(pribilong+pribihu);
    float kaihu=0.9253*pribihu/(pribihu+pribilong);
    float kaihe=0.0747;
    //出红是否换牌概率
    openLog("黑名单控制开龙的概率:%f    黑名单控制开虎的概率:%f",kailong,kaihu);
    float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility>0.9253)
    {
        wintag=1;
        openLog("黑名单要求开龙哦");
    }
    else
    {
        if(pribility<kailong)
        {
            wintag=2;
        }
        else
        {
            wintag=3;
        }
    }

    long winscore=0;
    if(wintag==1)
    {
        winscore =-bettingQuantity[He]*m_muticleArr[He];
    }
    else if(wintag==2)
    {
        winscore =-bettingQuantity[Long]*m_muticleArr[Long]+bettingQuantity[He]+bettingQuantity[Hu];
    }
    else
    {
        winscore =-bettingQuantity[Hu]*m_muticleArr[Hu]+bettingQuantity[He]+bettingQuantity[Long];
    }
    miCurrentProfit=winscore;
    std::vector<int> v_wintag;
    v_wintag.clear();
    int opentag=0;
    if(m_killOrReword==1)
    {
        for(int i=1;i<4;i++)
        {
            int opentag=i;
            long winscore=0;
            if(opentag==1)
            {
                winscore =-bettingQuantity[He]*m_muticleArr[He];
            }
            else if(opentag==2)
            {
                winscore =-bettingQuantity[Long]*m_muticleArr[Long]+bettingQuantity[He]+bettingQuantity[Hu];
            }
            else
            {
                winscore =-bettingQuantity[Hu]*m_muticleArr[Hu]+bettingQuantity[He]+bettingQuantity[Long];
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
            if(wintag==1)
            {
                winscore1 =-bettingQuantity[He]*m_muticleArr[He];
            }
            else if(wintag==2)
            {
                winscore1 =-bettingQuantity[Long]*m_muticleArr[Long]+bettingQuantity[He]+bettingQuantity[Hu];
            }
            else
            {
                winscore1 =-bettingQuantity[Hu]*m_muticleArr[Hu]+bettingQuantity[He]+bettingQuantity[Long];
            }
            miCurrentProfit=winscore1;
            if(m_killOrReword==-1)
            {
                if(winscore1<0)
                {
                    isOk=true;//系统杀分时，本次系统赢分小于零则继续
                }
            }
            else if(m_killOrReword==-1)
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
                int8_t LongRes=0;
                int8_t HeRes=0;
                LongRes=m_random.betweenInt(1,13).randInt_mt(true);
                HeRes=m_random.betweenInt(1,13).randInt_mt(true);
                if(LongRes==HeRes)
                {
                    wintag=1;//和
                }
                else if(LongRes>HeRes)
                {
                    wintag=2;//龙
                }
                else
                {
                    wintag=3;//虎
                }
            }
            else
            {
                break;
            }

        }while(isOk);//系统要放分的时候，本次系统盈利要大于零
    }

}
void LongHuAlgorithm::GetOpenCard(int8_t &wintag)
{

    bool isOk=false;
    std::vector<int> v_wintag;
    v_wintag.clear();
    int opentag=0;
    if(m_killOrReword==1)
    {
        for(int i=1;i<4;i++)
        {
            int opentag=i;
            int64_t betting=bettingQuantity[He]+bettingQuantity[Long]+bettingQuantity[Hu];
            long winscore=0;
            if(opentag==1)
            {
                winscore =-bettingQuantity[He]*m_muticleArr[He];
            }
            else if(opentag==2)
            {
                winscore =-bettingQuantity[Long]*m_muticleArr[Long]+bettingQuantity[He]+bettingQuantity[Hu];
            }
            else
            {
                winscore =-bettingQuantity[Hu]*m_muticleArr[Hu]+bettingQuantity[He]+bettingQuantity[Long];
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
        }
    }

    do
    {
        isOk=false;
        wintag=1;
        int8_t LongRes=0;
        int8_t HeRes=0;
        LongRes=m_random.betweenInt(1,13).randInt_mt(true);
        HeRes=m_random.betweenInt(1,13).randInt_mt(true);
        if(LongRes==HeRes)
        {
            wintag=1;//和
        }
        else if(LongRes>HeRes)
        {
            wintag=2;//龙
        }
        else
        {
            wintag=3;//虎
        }
        //公平百分之五十出随机结果
        int64_t betting=bettingQuantity[He]+bettingQuantity[Long]+bettingQuantity[Hu];
        if(betting<=0)
        {
            break;
        }
        long winscore=0;
        if(wintag==1)
        {
            winscore =-bettingQuantity[He]*m_muticleArr[He];
        }
        else if(wintag==2)
        {
            winscore =-bettingQuantity[Long]*m_muticleArr[Long]+bettingQuantity[He]+bettingQuantity[Hu];
        }
        else
        {
            winscore =-bettingQuantity[Hu]*m_muticleArr[Hu]+bettingQuantity[He]+bettingQuantity[Long];
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

double LongHuAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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
