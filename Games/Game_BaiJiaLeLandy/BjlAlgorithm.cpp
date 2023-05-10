#include "BjlAlgorithm.h"
#include <vector>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
using namespace std;
BjlAlgorithm::BjlAlgorithm()
{
    for(int i=0;i<Max;i++)
        m_muticleArr[i]=1;
    miCurrentProfit=0;
	m_roomID = 0;
	m_tableID = 0;
}

void  BjlAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t BjlAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}
void BjlAlgorithm::CallBackHandle()
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
void  BjlAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/bjl/bjlAlgorithm_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_roomID,m_tableID);
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
void BjlAlgorithm::BlackGetMustkillOpenCard(int8_t &wintag,float probirity[3],int64_t jetton[Max], int8_t cardPair[],bool isMust)
{
    if(isMust)
    {
        //公平百分之五十出随机结果
        int64_t betting = getAllBetScore();
        if(betting<=0)
        {
            return;
        }
        float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
        if(pribility>0.9253)
        {
            wintag = (int)WinTag::eHe;
            openLog("黑名单要求开和哦");
        }
        else
        {
            if(jetton[Zhuang]>jetton[Xian])
            {
                wintag = (int)WinTag::eXian;
                openLog("庄下注大于闲下注，开闲");
            }
            else
            {
                wintag = (int)WinTag::eZhuang;
                openLog("庄下注小于闲下注，开庄");
            }
        }

        long winscore = CalculatWinScore(wintag, cardPair);
        
        miCurrentProfit=winscore;
    }
    else
    {
        //公平百分之五十出随机结果
        int64_t betting = getAllBetScore();
        if(betting<=0)
        {
            return;
        }
        float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
        if(pribility>0.9253)
        {
            wintag = (int)WinTag::eHe;
            openLog("黑名单要求开和哦");
        }
        else
        {
            if(jetton[Zhuang]>jetton[Xian])
            {
                wintag = (int)WinTag::eXian;
                openLog("庄下注大于闲下注，开闲");
            }
            else
            {
                wintag = (int)WinTag::eZhuang;
                openLog("庄下注小于闲下注，开庄");
            }
        }

        long winscore = CalculatWinScore(wintag, cardPair);
        
        miCurrentProfit=winscore;

        if(miCurrentProfit<-1000000&&m_killOrReword==-1)
        {
            if(wintag == (int)WinTag::eXian) wintag = (int)WinTag::eZhuang;
            if(wintag == (int)WinTag::eZhuang) wintag = (int)WinTag::eXian;
        }
        else if(miCurrentProfit<-500000&&m_killOrReword==0)
        {
            if(wintag == (int)WinTag::eXian ||wintag == (int)WinTag::eZhuang)
              wintag = m_random.betweenInt((int)WinTag::eZhuang, (int)WinTag::eXian).randInt_mt(true);
        }
		winscore = CalculatWinScore(wintag, cardPair);
        miCurrentProfit=winscore;
    }

}
void BjlAlgorithm::BlackGetOpenCard(int8_t &wintag,float probirity[3], int8_t cardPair[])
{
    //公平百分之五十出随机结果
    int64_t betting = getAllBetScore();
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
    openLog("黑名单控制开庄的概率:%f    黑名单控制开闲的概率:%f",kailong,kaihu);
    float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
    if(pribility>0.9253)
    {
        wintag=0;
        openLog("黑名单要求开和哦");
    }
    else
    {
        if(pribility<kailong)
        {
            wintag=1;
			openLog("黑名单要求开庄哦");
        }
        else
        {
            wintag=2;
			openLog("黑名单要求开闲哦");
        }
    }

    long winscore = CalculatWinScore(wintag, cardPair);
    miCurrentProfit=winscore;
    std::vector<int> v_wintag;
    v_wintag.clear();
    int opentag=0;
    if(m_killOrReword==1)
    {
		for (int i = 0;i < 3;i++)
        {
            int opentag=i;
            long winscore = CalculatWinScore(opentag, cardPair);
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
	int loopCount = 0;
	int npair = 0;
	for (int i = 0;i < 3;i++)
	{
		if (cardPair[i] == 1)
			npair++;
	}
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
            long winscore1 = CalculatWinScore(wintag, cardPair);
            miCurrentProfit=winscore1;
            if(m_killOrReword==-1)
            {
                if(winscore1<0)
                {
                    isOk=true;//系统杀分时，本次系统赢分小于零则继续
					loopCount++;
					if (npair > 0 && loopCount >= 500)
					{
						loopCount = 0;
						npair--;
						for (int i = 0;i < 3;i++)
						{
							if (cardPair[i] == 1)
							{
								cardPair[i] = 0;
								break;
							}
						}
					}
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
					loopCount++;
					if (npair > 0 && loopCount >= 500)
					{
						loopCount = 0;
						npair--;
						for (int i = 0;i < 3;i++)
						{
							if (cardPair[i] == 1)
							{
								cardPair[i] = 0;
								break;
							}
						}
					}
                }
            }
            if(isOk)
            {
                int8_t ZhuangRes=0;
                int8_t XianRes=0;
                ZhuangRes=m_random.betweenInt(1,13).randInt_mt(true);
                XianRes=m_random.betweenInt(1,13).randInt_mt(true);
                if(ZhuangRes==XianRes)
                {
                    wintag=0;//和
                }
                else if(ZhuangRes>XianRes)
                {
                    wintag=1;//庄
                }
                else
                {
                    wintag=2;//闲
                }
            }
            else
            {
                break;
            }
			if (npair == 0 && loopCount >= 500)
				isOk = false;
        }while(isOk);//系统要放分的时候，本次系统盈利要大于零
    }

}
void BjlAlgorithm::GetOpenCard(int8_t &wintag, int8_t cardPair[])
{
    bool isOk=false;
    std::vector<int64_t> v_wintag;
    v_wintag.clear();
    int opentag=0;
    if(m_killOrReword==1)
    {
		for (int i = 0;i < 3;i++)
        {
            int opentag=i;
            int64_t betting = getAllBetScore();
            long winscore = CalculatWinScore(opentag, cardPair);
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
	int loopCount = 0;
	int npair = 0;
	for (int i = 0;i < 3;i++)
	{
		if (cardPair[i] == 1)
			npair++;
	}
    do
    {
        isOk=false;
        wintag=0;
        int8_t ZhuangRes=0;
        int8_t XianRes=0;
        ZhuangRes=m_random.betweenInt(1,13).randInt_mt(true);
        XianRes=m_random.betweenInt(1,13).randInt_mt(true);
        if(ZhuangRes==XianRes)
        {
            wintag=0;//和
        }
        else if(ZhuangRes>XianRes)
        {
            wintag=1;//庄
        }
        else
        {
            wintag=2;//闲
        }
        //公平百分之五十出随机结果
        int64_t betting = getAllBetScore();
        if(betting<=0)
        {
            break;
        }
        int64_t winscore = CalculatWinScore(wintag, cardPair);
        miCurrentProfit=winscore;
        if(m_killOrReword==-1)
        {
            if(winscore<0)
            {
                isOk=true;//系统杀分时，本次系统赢分小于零则继续
				loopCount++;
				if (npair > 0 && loopCount >= 500)
				{
					loopCount = 0;
					npair--;
					for (int i = 0;i < 3;i++)
					{
						if (cardPair[i] == 1)
						{ 
							cardPair[i] = 0;
							break;
						}
					}
				}
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
				loopCount++;
				if (npair > 0 && loopCount >= 500)
				{
					loopCount = 0;
					npair--;
					for (int i = 0;i < 3;i++)
					{
						if (cardPair[i] == 1)
						{
							cardPair[i] = 0;
							break;
						}
					}
				}
            }
        }
		if (npair == 0 && loopCount >= 500)
			isOk = false;
    }while(isOk);//系统要放分的时候，本次系统盈利要大于零
}

double BjlAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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
//计算出临时赢分
long BjlAlgorithm::CalculatWinScore(int winTag, int8_t cardPair[])
{
    int64_t winscore = 0;
	for (int i = 0;i < Max;i++)
	{
		if (i < 3)
		{
			if (i == winTag)
			{
				winscore += -bettingQuantity[i] * (m_muticleArr[i] - 1);
			}
			else
			{
				if (winTag == (int)WinTag::eHe) //开和
				{
					if (i == (int)WinTag::eZhuang || i == (int)WinTag::eXian)
						continue;
					winscore += bettingQuantity[i];
				}
				else
				{
					winscore += bettingQuantity[i];
				}
			}
		}
		else if (i >= 3) //牌对
		{
			if (cardPair[i - 3] == 1)
			{
				winscore -= bettingQuantity[i] * (m_muticleArr[i] - 1);
			}
			else
			{
				winscore += bettingQuantity[i];
			}
		}
	}

	return winscore;
}

long BjlAlgorithm::getAllBetScore()
{
    int64_t allScore = 0;
	for (int i = 0;i < Max;i++)
	{
        allScore += bettingQuantity[i];
	}

	return allScore;
}

long BjlAlgorithm::CalculatTestWinScore(int winTag, int8_t cardPair[])
{
    int64_t winscore = 0;
	for (int i = 0;i < Max;i++)
	{
		if (i < 3)
		{
			if (i == winTag)
			{
				winscore += bettingQuantity[i] * m_muticleArr[i];
			}
		}
		else if (i >= 3) //牌对
		{
			if (cardPair[i - 3] == 1)
			{
				winscore += bettingQuantity[i] * m_muticleArr[i];
			}
		}
	}

	return winscore;
}

