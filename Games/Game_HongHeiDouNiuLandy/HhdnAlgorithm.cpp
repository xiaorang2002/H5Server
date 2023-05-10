#include "HhdnAlgorithm.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
uint8_t HhdnAlgorithm::m_cbTableCard[MAX_CARDS] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
};
HhdnAlgorithm::HhdnAlgorithm()/*:timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(),"hhe"))*/
{
//    timelooper->startLoop();

//    timelooper->getLoop()->runEvery(1,bind(&HhdnAlgorithm::CallBackHandle,this));
//    profit=25000;
//   zongjin=25000;
    m_lcurrentProfit=0;
}

void  HhdnAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
{
    if(m_profitInterval==1)//上限以上就看概率,或者放分，或者随机
    {
		int randProfit = RandomInt(0, 10000);
		openLog("===库存上限以上,控制随机值[%d],控制值[%d];", randProfit, resultProbility);
        if(randProfit <resultProbility)
        {
            m_killOrReword = 1;
			openLog("===库存上限以上,控制开:放分");
        }
        else
        {
            m_killOrReword = 0;
			openLog("===库存上限以上,控制开:随机");
        }
    }
    else if(m_profitInterval==-1)//目前盈利在下限以下，看概率，或者杀分，或者随机
    {
		int randProfit = RandomInt(0, 10000);
		openLog("===库存下限以下,控制随机值[%d],控制值[%d];", randProfit, resultProbility);
        if(randProfit <resultProbility)
        {
            m_killOrReword = -1;
			openLog("===库存下限以下,控制开:杀分", resultProbility);
        }
        else
        {
            m_killOrReword = 0;
			openLog("===库存下限以下,控制开:随机");
        }
    }
    else
    {
        m_killOrReword = 0;
		openLog("===库存正常范围,控制开:随机");
    }
}
void HhdnAlgorithm::CallBackHandle()
{
//    SetBetting(0 ,RandomInt(200,2000));
//    SetBetting(1 ,RandomInt(200,2000));
//    SetBetting(2 ,RandomInt(200,2000));
//    SetAlgorithemNum(50000,0, profit,0.45, 5000);
//    //测试算法的
//    uint8_t aaa[2][MAX_COUNT];
//    GetOpenCard(aaa);
//    CalculationResults();
    //LOG_INFO << "win   --------------------"<<ifWinProfit;
}
void  HhdnAlgorithm::openLog(const char *p, ...)
{

    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/Hhdn/Hhdn_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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
void HhdnAlgorithm::BlackGetOpenCard(uint8_t handcardarr[][MAX_COUNT],uint8_t & wintag,uint8_t & wintype,float probirity[3])
{
    bool isHe=false;
    bool ishongOrHei=false;
    float pribiHong=0.0f;
    float pribiHei=0.0f;
    pribiHong= 0.4216*(1-probirity[2]);
    pribiHei= 0.4216*(1-probirity[1]);
    float kaihong= 0.8432*pribiHong/(pribiHong+pribiHei);
    float kaihei= 0.8432*pribiHei/(pribiHong+pribiHei);
	float kaihe = 0.1568;
	float pribility = m_random.betweenFloat(0.0, 1.0).randFloat_mt(true);
    //出红是否换牌概率
    openLog("===黑名单控制开黑的概率:%f    黑名单控制开红的概率:%f  获取到的概率 pribility:%f",kaihei,kaihong,pribility);
	if (pribility > 0.8432)
	{
		isHe = true;
		openLog("===黑名单要求开和");
	}
	else
	{
		if (pribility < kaihei)
		{
			 ishongOrHei=true;//换牌那就是出黑
			 openLog("===黑名单要求出黑哦");
		}
	}
	uint8_t tmpcbTableCardArray[2][MAX_COUNT] = { 0 };
	bool bControlBlackOK = false;
	int32_t randCardCount = 0;
	bool bControlKuCun = false;
    do
    {
Nunber0:
        m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));//打乱扑克
        memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));//随机发10张扑克
        memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
        memcpy(handcardarr, tmpcbTableCardArray, sizeof(tmpcbTableCardArray));
		int64_t winscore = ConcludeWinScore(tmpcbTableCardArray, wintag, wintype);
		bControlBlackOK = false; 
		randCardCount++;
		if (!CheckHanCardIsOK(m_cbTableCardArray, 2*MAX_COUNT))
		{
			openLog("===黑名单控制获取牌值错误,继续");
			goto Nunber0;//获取牌值错误,继续
		}
		//校验是否所需的结果
		if (!bControlKuCun && randCardCount < RANDCARD_MAXCOUNT / 2)
		{
			if (isHe)
			{
				if (m_winFlag == HeGate)
				{
					openLog("===黑名单控制已经开了和的牌");
					bControlBlackOK = true;
				}
			}
			else
			{
				//算法要求出黑
				if (ishongOrHei)
				{
					if (m_winFlag == BalckGate)
					{
						openLog("===黑名单控制已经开了黑的牌");
						bControlBlackOK = true;
					}
				}
				else
				{
					if (m_winFlag == RedGate)
					{
						openLog("===黑名单控制已经开了红的牌");
						bControlBlackOK = true;
					}
				}
			}
		}
		else
		{
			bControlBlackOK = true;
		}
		//重新发牌N次后，依然无有效结果则开系统赢（容错，例如库存区间特别狭窄的情况）
		if (randCardCount > RANDCARD_MAXCOUNT && winscore > 0)
		{
			openLog("===重新发牌%d次后依然无有效结果,开系统赢,winscore=%d;", randCardCount, winscore);
			bControlBlackOK = false;
			break;
		}
		if (bControlBlackOK)
		{
			openLog("===系统库存:m_killOrReword=%d;", m_killOrReword);
			if (m_killOrReword == -1)
			{
				if (winscore <= 0)
				{
					if (randCardCount >= RANDCARD_MAXCOUNT / 2)
					{
						bControlKuCun = true;
					}					
					goto Nunber0;//系统杀分时，本次赢分小于零则继续
				}
				openLog("===系统要求杀分了,黑名单暂且放你一马,winscore=%d;", winscore);
				break;
			}
			if (m_killOrReword == 1)
			{
				if (winscore >= 0)
				{
                    if(randCardCount >= RANDCARD_MAXCOUNT / 2)
					{
						bControlKuCun = true;
					}
					goto Nunber0;//系统放分时，本次盈利大于零则继续
				}
				if (winscore < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
				{
					if (randCardCount >= RANDCARD_MAXCOUNT / 2)
					{
						bControlKuCun = true;
					}
					goto Nunber0;//系统放分时，超过红线则继续
				}
				openLog("===系统要放分,黑名单我不管你,winscore=%d;", winscore);
				break;
			}
			if (m_killOrReword == 0)
			{
				if (winscore<0)
				{
					if (winscore < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
					{
						if (randCardCount >= RANDCARD_MAXCOUNT / 2)
						{
							bControlKuCun = true;
						}
						goto Nunber0;//系统正常时，超过红线则继续
					}
				}		
                openLog("===系统要随机开,保险值=%d,winscore=%d;",std::max(m_thresholdNum, (m_curProfit - m_lowLimit) / 2), winscore);
			}
			
			break;
		}
    }while(true);

}
void HhdnAlgorithm::GetOpenCard(uint8_t handcardarr[][MAX_COUNT],uint8_t & wintag,uint8_t & wintype)
{
#ifdef TESTECARD
	uint8_t tmpcbTableCardArray[2][MAX_COUNT] = { { 0x15,0x04,0x09,0x2a,0x08 },{ 0x01,0x3d,0x1c,0x14,0x38 } };
	bool bTestCard = true;
#else
	uint8_t tmpcbTableCardArray[2][MAX_COUNT] = { 0 };
	bool bTestCard = false;
#endif // 1   
    bool isOk=false;
	int32_t randCardCount = 0;
    do
    {
        isOk=false;
		randCardCount++;
		if (!bTestCard)
		{
			m_GameLogic.RandCardList(m_cbTableCard, sizeof(m_cbTableCard));//打乱扑克
			memcpy(m_cbTableCardArray, m_cbTableCard, sizeof(m_cbTableCardArray));//随机发10张扑克
			memcpy(tmpcbTableCardArray, m_cbTableCardArray, sizeof(tmpcbTableCardArray));
		}    
		else
		{
			memcpy(m_cbTableCardArray, tmpcbTableCardArray, sizeof(tmpcbTableCardArray));
		}
        memcpy(handcardarr, tmpcbTableCardArray, sizeof(tmpcbTableCardArray));
		int64_t winscore = ConcludeWinScore(tmpcbTableCardArray, wintag, wintype);
        if(m_killOrReword==-1)
        {
            if(winscore<0)
            {
                isOk=true;//系统杀分时，本次赢分小于零则继续
            }
			else
			{
				openLog("===库存控制,干涉率:%d,杀分:winscore=%d;", resultProbility, winscore);
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
			if (!isOk)
			{
				openLog("===库存控制,干涉率:%d,放分:winscore=%d;", resultProbility, winscore);
			}
        }
        else
        {
            if(winscore<-m_thresholdNum&&winscore<-(m_curProfit-m_lowLimit)/2)
            {
                isOk=true;//随机时，超过红线则继续
            }
			else
			{
                openLog("===库存控制,干涉率:%d,随机:保险值=%d,winscore=%d;", resultProbility, std::max(m_thresholdNum, (m_curProfit - m_lowLimit) / 2), winscore);
			}
        }
		if (!CheckHanCardIsOK(m_cbTableCardArray, 2 * MAX_COUNT))
		{
			openLog("===获取牌值错误,继续");
			isOk = true;
		}
		if (isOk)
		{
			//重新发牌N次后，依然无有效结果则开系统赢（容错，例如库存区间特别狭窄的情况）
			if (randCardCount > RANDCARD_MAXCOUNT && winscore > 0)
			{
				openLog("===重新发牌%d次后依然无有效结果,开系统赢,winscore=%d;", randCardCount, winscore);
				isOk = false;
			}
		}
    }while(isOk);//系统要放分的时候，本次系统盈利要大于零
}
#ifdef Test_Algorithm
void HhdnAlgorithm::CalculationResults()
{


    long thisprofit=0;
    zongjin+=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    thisprofit+=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2];
    zongchu +=bettingQuantity[m_winFlag]*2;
    thisprofit-=bettingQuantity[m_winFlag]*2;
    if(m_winType>0)
    {
        zongchu+=bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
        thisprofit-=bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
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
        LOG_INFO<<"本次幸运一击的牌型是{单牌}-------押注是"<<bettingQuantity[HeGate];
    }
    else if(m_winType==1)
    {
        LOG_INFO<<"本次幸运一击的牌型是{对子}-------押注是"<<bettingQuantity[HeGate]<<"-----本利是"<<bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==2)
    {
        LOG_INFO<<"本次幸运一击的牌型是{顺子}-------押注是"<<bettingQuantity[HeGate]<<"-----本利是"<<bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==3)
    {
        LOG_INFO<<"本次幸运一击的牌型是{金花}-------押注是"<<bettingQuantity[HeGate]<<"-----本利是"<<bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==4)
    {
        LOG_INFO<<"本次幸运一击的牌型是{顺金}-------押注是"<<bettingQuantity[HeGate]<<"-----本利是"<<bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
    }
    else if(m_winType==5)
    {
        LOG_INFO<<"本次幸运一击的牌型是{豹子}-------押注是"<<bettingQuantity[HeGate]<<"-----本利是"<<bettingQuantity[HeGate]*(mutipleArr[m_winType]+1);
    }
}
#else

#endif
double HhdnAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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


int64_t HhdnAlgorithm::ConcludeWinScore(uint8_t handcardarr[2][MAX_COUNT], uint8_t & wintag, uint8_t & wintype)
{
	int64_t winscore = 0;
	uint8_t winIndex = 0;
	uint8_t  heiType = m_GameLogic.GetCardType(handcardarr[BalckGate - 1], MAX_COUNT);
	uint8_t  hongType=   m_GameLogic.GetCardType(handcardarr[RedGate-1], MAX_COUNT);
	m_TableCardType[BalckGate-1] = heiType;
	m_TableCardType[RedGate-1] = hongType;
	bool bSpecialNiu = false;
	uint8_t CompareIndex = m_GameLogic.CompareCard(handcardarr[BalckGate - 1], handcardarr[RedGate - 1], MAX_COUNT);
	if (CompareIndex == 2) //和
	{
		m_winType = heiType;
		m_winFlag = HeGate;
		m_loseFlag = RedGate;
		wintag = HeGate;
		wintype = heiType;
	}
	else if (CompareIndex == 1) //黑
	{
		m_winType = heiType;
		m_winFlag = BalckGate;
		m_loseFlag = RedGate;
		wintag = BalckGate;
		wintype = heiType;
	}
	else if (CompareIndex == 0) //红
	{
		m_winType = hongType;
		m_winFlag = RedGate;
		m_loseFlag = BalckGate;
		wintag = RedGate;
		wintype = hongType;
	}	  
	
	if (m_winFlag==HeGate) //开和，红牛和黑牛的押注退还给玩家
	{
		winscore -= bettingQuantity[m_winFlag] * m_muticleArr[m_winFlag];
		if (heiType > 0)//有牛
		{
			winIndex = getAreaIndexByType(heiType);
			for (uint8_t i = NiuOneGate;i < MaxGate;++i)
			{
		  		if (i == winIndex) //开和并且有牛，则赔付一个牛
		  		{
		  			winscore -= bettingQuantity[i] * m_muticleArr[i];
		  		}
		  		else
		  		{
		  			winscore += bettingQuantity[i];
		  		}
			}
		}
		else
		{
			for (uint8_t i = NiuOneGate;i < MaxGate;++i)
			{
				winscore += bettingQuantity[i];
			}
		}
	} 
	else
	{
		winscore += bettingQuantity[m_loseFlag] + bettingQuantity[HeGate] - bettingQuantity[m_winFlag] * m_muticleArr[m_winFlag];
		if (heiType > 0)  //有牛，赔付对应倍数的金币
		{
			winIndex = getAreaIndexByType(heiType);
			winscore -= bettingQuantity[winIndex] * m_muticleArr[winIndex];
			if (winIndex == NiuSpecialGate)
			{
				bSpecialNiu = true;
			}
		}
		if (hongType > 0) //有牛，赔付对应倍数的金币
		{
			winIndex = getAreaIndexByType(hongType);
			if (winIndex != NiuSpecialGate || !bSpecialNiu) 
			{
				winscore -= bettingQuantity[winIndex] * m_muticleArr[winIndex];
			}
		}
		for (uint8_t i = NiuOneGate;i < MaxGate;++i)
		{
			uint8_t heiIndex = getAreaIndexByType(heiType);
			uint8_t hongIndex = getAreaIndexByType(hongType);
			if (i != heiIndex && i != hongIndex)
			{
		  		winscore += bettingQuantity[i];
			}
		}
	}
	m_lcurrentProfit = winscore;
	return winscore;
}

//牌型转押注区域
int HhdnAlgorithm::getAreaIndexByType(uint8_t cardType)
{
	uint8_t winIndex = 0;
	if (cardType > 0)
	{
		winIndex = cardType <= CT_NIUNIU ? (cardType + RedGate) : NiuSpecialGate;
	}
	return winIndex;
}

//检验手牌牌值是否重复或者为0
bool HhdnAlgorithm::CheckHanCardIsOK(uint8_t handcardarr[], uint8_t cardcount)
{
	for (int i = 0; i < cardcount; ++i)
	{
		if (handcardarr[i] == 0)
			return false;
		for (int j = i + 1; j < cardcount; j++)
		{
			if (handcardarr[i] == handcardarr[j])
			{
				return false;
			}
		}
	}

	return true;
}
