#include "SlwhAlgorithm.h"

#include "public/GlobalFunc.h"

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

SlwhAlgorithm::SlwhAlgorithm()
{
	for (int i = 0;i < MaxGate;i++)
	{
		m_muticleArr[i] = 5;
		sortprobirity[i] = 0.0f;
		animalID[i] = i;
	}
	
    miCurrentProfit=0;
	m_iwinTag_sd = 0;			//送灯时赠送的动物类型
	iResultId_tmp = 0;
	m_lMustKillLimitScore = 0;
}
void  SlwhAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t SlwhAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}

void  SlwhAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "log//slwh//slwh%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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
void SlwhAlgorithm::BlackGetOpenCard(int &winTag,float probirity[], int &typeTag, int tmpID[5])
{
	for (int i = 0;i < MaxGate + 1;i++)
	{
		sortprobirity[i] = probirity[i];
	}
	sortProbirityAndAnimal();
    int64_t betting=0;
    float wholeweight=0.0;
    winTag=m_random.betweenInt(0,MaxGate-4).randInt_mt(true);
    //公平百分之五十出随机结果
    for(int i=0;i<MaxGate;i++)
    {
		if (i < MaxGate-3)
		{
			betting += bettingQuantity[i];
		}
		openLog("黑名单的权重[%d]= %f", i, probirity[i]);
		openLog("黑名单每项下注是[%d]= %d", i, bettingQuantity[i]);
    }
    //权重之限于前十项的

    float weights[MaxGate+1]={10000};
    for(int i=0;i<MaxGate+1;i++)
    {
        wholeweight += sortprobirity[i];
        weights[i] = sortprobirity[i];

    }
    float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);

    float res=0.0;

	for (int i = 0;i < MaxGate + 1;i++)
	{
		res += weights[i];
		if (pribility - res <= 0)
		{
			if (i < 12)
			{
				winTag = animalID[i];
				typeTag = 0;
			}
			else
			{
				winTag = tmpID[i - 11];
				typeTag = i - 11;
			}
			break;
		}
	}

	/*if (typeTag == (int)OpenType::eNormal)
	{
		for (int i = 0;i < MaxGate - 3;i++)
		{
			res += weights[i];
			if (pribility - res <= 0)
			{
				winTag = animalID[i];
				openLog("开出的结果=:%d", animalID[i]);
				break;
			}
		}
	}
	else
	{
		if (typeTag == (int)OpenType::eSongDeng)
		{
			do
			{
				pribility = m_random.betweenFloat(0.0, wholeweight).randFloat_mt(true);
				res = 0.0;
				for (int i = 0;i < MaxGate - 3;i++)
				{
					res += weights[i];
					if (pribility - res <= 0)
					{
						winTag = animalID[i];
						break;
					}
				}
			} while (winTag == m_iwinTag_sd);
		}
	}*/
    
    float allweight = 0.0;
    for(int i=0;i<MaxGate+1;i++)
    {
         allweight += weights[i];
    }

	for (int i = 0;i < MaxGate + 1;i++)
	{
		openLog("第[%d]个项的开奖概率是=:%f", animalID[i], weights[i] / allweight);
	}
    /*openLog("第[0]个项红兔子的出牌概率是=:%f" ,weights[0]/allweight);
    openLog("第[1]个项红猴子的出牌概率是=:%f" ,weights[1]/allweight);
    openLog("第[2]个项红熊猫的出牌概率是=:%f" ,weights[2]/allweight);
    openLog("第[3]个项红狮子的出牌概率是=:%f" ,weights[3]/allweight);
    openLog("第[4]个项绿兔子的出牌概率是=:%f" ,weights[4]/allweight);
    openLog("第[5]个项绿候子的出牌概率是=:%f" ,weights[5]/allweight);
    openLog("第[6]个项绿熊猫的出牌概率是=:%f" ,weights[6]/allweight);
    openLog("第[7]个项绿狮子的出牌概率是=:%f" ,weights[7]/allweight);
    openLog("第[8]个项黄兔子的出牌概率是=:%f" ,weights[8]/allweight);
    openLog("第[9]个项黄猴子的出牌概率是=:%f" ,weights[9]/allweight);
	openLog("第[10]个项黄熊猫的出牌概率是=:%f", weights[10] / allweight);
	openLog("第[11]个项黄狮子的出牌概率是=:%f", weights[11] / allweight);
	openLog("第[12]个项霹雳闪电的出牌概率是=:%f", weights[12] / allweight);
	openLog("第[13]个项送灯的出牌概率是=:%f", weights[13] / allweight);
	openLog("第[14]个项大三元的出牌概率是=:%f", weights[14] / allweight);
	openLog("第[15]个项大四喜的出牌概率是=:%f", weights[15] / allweight);*/

    if(betting<=0)
    {
        return;
    }

    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    long winscore=0;
    for(int i = 0;i < MaxGate - 3;i++)
    {
        winscore+=bettingQuantity[i];
    }
	winscore -= CalculatWinScore(winTag, typeTag);
    miCurrentProfit=winscore;
   //做个是否有结果的验证
   if(m_killOrReword==1&&betting>0)
   {
        int countOpen=0;
        for(int i=0;i<MaxGate-3;i++)
        {
             long winscore1=0;
             for(int j=0;j<MaxGate-3;j++)
             {
                 winscore1+=bettingQuantity[j];
             }
			 winscore1 -= CalculatWinScore(winTag, typeTag);
             
             if(winscore1 < 0 && winscore1 >= -m_thresholdNum && winscore1 >= -(m_curProfit - m_lowLimit) / 2)
             {

                break;
             }
             else
             {
                 countOpen++;
             }
        }
        if(countOpen>=MaxGate-3)//没有结果,那只能随机听天由命了
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
           long winscore1=0;
           for(int i=0;i<MaxGate-3;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i=0;i<MaxGate-3;i++)
           {
               if(winTag==i)
               {				    
				   winscore1 -= CalculatWinScore(winTag, typeTag);
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
               float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);

               float res=0.0;
               for(int i=0;i<MaxGate+1;i++)
               {
                   res += weights[i];
                   if(pribility-res<=0)
                   {
					   if (i < 12)
					   {
						   winTag = animalID[i];
						   typeTag = 0;
					   }
					   else
					   {
						   winTag = tmpID[i - 11];
						   typeTag = i - 11;
					   }
                       break;
                   }
               }
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   openLog("黑名单经过限红判断后的结果是=%d  变了吗，没变没触洪线，变了那是触洪了",winTag);

}
void SlwhAlgorithm::GetOpenCard(int &winTag,float probirity[],int &typeTag, int tmpID[5])
{
	for (int i = 0;i < MaxGate + 1;i++)
	{
		sortprobirity[i] = probirity[i];
		animalID[i] = i;
	}
    sortProbirityAndAnimal();
    winTag=m_random.betweenInt(0,MaxGate-4).randInt_mt(true);
    int64_t betting=0;
	float wholeweight=0.0;
    //公平百分之五十出随机结果
    float weights[MaxGate+1]={0};
	//int weights[MaxGate + 1] = { 0 };
	string strWeight;
	string strID = "";
	for (int i = 0;i < 5;i++)
	{
		strID += to_string(i) + "_" + to_string(tmpID[i]) + "  ";
	}
    for(int i = 0;i < MaxGate - 3;i++)
    {
        betting+=bettingQuantity[i];
    }
    for(int i = 0;i < MaxGate + 1;i++)
    {
        wholeweight += sortprobirity[i];
        weights[i] = sortprobirity[i];
		strWeight += to_string(i) + "_" + to_string(weights[i]) + "  ";
    }

    float pribility=m_random.betweenFloat(0.0,wholeweight).randFloat_mt(true);
	float res = 0.0;
	/*int  pribility = m_random.betweenInt(0, wholeweight).randInt_mt(true);
	int  res = 0;*/
	for (int i = 0;i < MaxGate + 1;i++)
	{
		res += weights[i];
		if (pribility - res <= 0)
		{
			if (i<12)
			{
				winTag = tmpID[0];//animalID[i];
				typeTag = 0;
			} 
			else
			{
				winTag = tmpID[i-11];
				typeTag = i - 11;
			}
			openLog("算法动物:%s", strID.c_str());
			openLog("算法权重:%s", strWeight.c_str());
			openLog("算法结果 pribility:%f,wholeweight:%f，类型:%d,i=%d,动物:%d \n", pribility, wholeweight, typeTag, i, winTag);
			break;
		}
	}

	/*if (typeTag == (int)OpenType::eNormal)
	{
		for (int i = 0;i < MaxGate - 3;i++)
		{
			res += weights[i];
			if (pribility - res <= 0)
			{
				winTag = animalID[i];
				break;
			}
		}
	} 
	else
	{
		if (typeTag == (int)OpenType::eSongDeng)
		{
			do 
			{
				pribility = m_random.betweenFloat(0.0, wholeweight).randFloat_mt(true);
				res = 0.0;
				for (int i = 0;i < MaxGate - 3;i++)
				{
					res += weights[i];
					if (pribility - res <= 0)
					{
						winTag = animalID[i];
						break;
					}
				}
			} while (winTag == m_iwinTag_sd);
		}
	}*/
    
    if(betting<=0)
    {
        return;
    }

    //出红是否换牌概率
    //openLog("黑名单控制开顺门的概率:%f    黑名单控制开天门的概率:%f      黑名单控制开地门的概率:%f",proshun,protian,prodi);

    long winscore=0;
    for(int i = 0;i < MaxGate - 3;i++)
    {
        winscore+=bettingQuantity[i];
    }

    for(int i = 0;i < MaxGate - 3;i++)
    {
        if(winTag==i)
		{
			winscore -= CalculatWinScore(winTag, typeTag);			
            break;
        }
    }

    miCurrentProfit=winscore;
   //做个是否有结果的验证
   if(m_killOrReword==1&&betting>0)
   {
        int countOpen=0;
        for(int i=0;i<MaxGate-3;i++)
        {
             long winscore1=0;
             for(int j=0;j<MaxGate-3;j++)
             {
                 winscore1+=bettingQuantity[j];
             }
			 winscore1 -= CalculatWinScore(winTag, typeTag);	
             if(winscore1 < 0 && winscore1 >= -(m_curProfit - m_lowLimit) / 2)
             {
                break;
             }
             else
             {
                 countOpen++;
             }
        }
        if(countOpen>=12)//没有结果,那只能随机听天由命了
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
           for(int i = 0;i < MaxGate - 3;i++)
           {
               winscore1+=bettingQuantity[i];
           }
           if(winscore1==0)//没有人押分的时候纯随机
           {
               break;
           }
           for(int i = 0;i < MaxGate - 3;i++)
           {
               if(winTag==i)
               {
				   winscore1 -= CalculatWinScore(winTag, typeTag);
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
               for(int i = 0;i < MaxGate + 1;i++)
               {
                   res += weights[i];
                   if(pribility-res<=0)
                   {
					   if (i < 12)
					   {
						   winTag = animalID[i];
						   typeTag = 0;
					   }
					   else
					   {
						   winTag = tmpID[i - 11];
						   typeTag = i - 11;
					   }
                       break;
                   }
               }
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
   }
   openLog("不是黑名单情况下，正常出的结果:%d",winTag);
}
double SlwhAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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

//动物权重排序
void SlwhAlgorithm::sortProbirityAndAnimal()
{
	for (int i = 0;i < MaxGate + 1;i++)
	{
		animalID[i] = i;
	}
	float tmpWeight = 0.0f;
	int animID = 0;
	for (int  i = 0; i < MaxGate - 4; i++)
	{
		for (int j = i+1; j < MaxGate - 3; j++)
		{
			if (sortprobirity[i] < sortprobirity[j])
			{
				tmpWeight = sortprobirity[i];
				sortprobirity[i] = sortprobirity[j];
				sortprobirity[j] = tmpWeight;

				animID = animalID[i];
				animalID[i] = animalID[j];
				animalID[j] = animID;
			}
		}
	}
}

//计算出临时赢分
long SlwhAlgorithm::CalculatWinScore(int winTag, int typeTag)
{
	long winscore = 0;
	if (typeTag == (int)OpenType::eNormal)
	{
		winscore += bettingQuantity[winTag] * m_muticleArr[winTag];
	}
	else if (typeTag == (int)OpenType::ePiliShanDian)
	{
		winscore += bettingQuantity[winTag] * m_muticleArr[winTag] * 2;
	}
	else if (typeTag == (int)OpenType::eSongDeng)
	{
		winscore += bettingQuantity[winTag] * m_muticleArr[winTag];
		winscore += bettingQuantity[m_iwinTag_sd] * m_muticleArr[m_iwinTag_sd]; //送灯的赔分
	}
	else if (typeTag == (int)OpenType::eDaSanYuan)
	{
		int animID = winTag % 4;
		for (int i = 0;i < 3;++i)
		{
			winscore += bettingQuantity[animID + i * 4] * m_muticleArr[animID + i * 4];
		}
	}
	else if (typeTag == (int)OpenType::eDaSiXi)
	{
		int animID = winTag / 4;
		for (int i = 0;i < 4;++i)
		{
			winscore += bettingQuantity[animID * 4 + i] * m_muticleArr[animID * 4 + i];
		}
	}

	return winscore;
}


void SlwhAlgorithm::BlackGetMustkillOpenCard_HZX(int &wintag, float probirity[], int64_t jetton[15], bool isMust)
{
	wintag = eHe;
	int8_t ZhuangRes = 0;
	int8_t XianRes = 0;
	ZhuangRes = m_random.betweenInt(1, 13).randInt_mt(true);
	XianRes = m_random.betweenInt(1, 13).randInt_mt(true);
	if (ZhuangRes == XianRes)
	{
		wintag = eHe;//和
	}
	else if (ZhuangRes > XianRes)
	{
		wintag = eZhuang;//庄
	}
	else
	{
		wintag = eXian;//闲
	}
	if (isMust)
	{
		//公平百分之五十出随机结果
		int64_t betting = bettingQuantity[eHe] + bettingQuantity[eZhuang] + bettingQuantity[eXian];
		if (betting <= 0)
		{
			return;
		}
		float pribility = m_random.betweenFloat(0.0, 1.0).randFloat_mt(true);
		if (pribility > 0.9253)
		{
			wintag = eHe;
			openLog("黑名单要求开和哦");
		}
		else
		{
			if (jetton[eZhuang] > jetton[eXian])
			{
				wintag = eXian;
				openLog("庄下注大于闲下注，开闲");
			}
			else
			{
				wintag = eZhuang;
				openLog("庄下注小于闲下注，开庄");
			}
		}

		long winscore = 0;
		if (wintag == eHe)
		{
			winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
		}
		else if (wintag == eZhuang)
		{
			winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
		}
		else
		{
			winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
		}
		miCurrentProfit = winscore;
	}
	else
	{
		//公平百分之五十出随机结果
		int64_t betting = bettingQuantity[eHe] + bettingQuantity[eZhuang] + bettingQuantity[eXian];
		if (betting <= 0)
		{
			return;
		}
		float pribility = m_random.betweenFloat(0.0, 1.0).randFloat_mt(true);
		if (pribility > 0.9253)
		{
			wintag = eHe;
			openLog("黑名单要求开和哦");
		}
		else
		{
			if (jetton[eZhuang] > jetton[eXian])
			{
				wintag = eXian;
				openLog("庄下注大于闲下注，开闲");
			}
			else
			{
				wintag = eZhuang;
				openLog("庄下注小于闲下注，开庄");
			}
		}

		long winscore = 0;
		if (wintag == eHe)
		{
			winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
		}
		else if (wintag == eZhuang)
		{
			winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
		}
		else
		{
			winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
		}
		miCurrentProfit = winscore;

		if (miCurrentProfit < -1000000 && m_killOrReword == -1)
		{
			if (wintag == eXian) wintag = eZhuang;
			if (wintag == eZhuang) wintag = eXian;
		}
		else if (miCurrentProfit < -500000 && m_killOrReword == 0)
		{
			if (wintag == eXian || wintag == eZhuang)
				wintag = m_random.betweenInt(eZhuang, eXian).randInt_mt(true);
		}
		winscore = 0;
		if (wintag == eHe)
		{
			winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
		}
		else if (wintag == eZhuang)
		{
			winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
		}
		else
		{
			winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
		}
		miCurrentProfit = winscore;
	}

}
void SlwhAlgorithm::BlackGetOpenCard_HZX(int &wintag, float probirity[])
{
	int8_t ZhuangRes = 0;
	int8_t XianRes = 0;
	ZhuangRes = m_random.betweenInt(1, 13).randInt_mt(true);
	XianRes = m_random.betweenInt(1, 13).randInt_mt(true);
	if (ZhuangRes == XianRes)
	{
		wintag = eHe;//和
	}
	else if (ZhuangRes > XianRes)
	{
		wintag = eZhuang;//庄
	}
	else
	{
		wintag = eXian;//闲
	}

	//公平百分之五十出随机结果
	int64_t betting = bettingQuantity[eHe] + bettingQuantity[eZhuang] + bettingQuantity[eXian];
	if (betting <= 0)
	{
		return;
	}
	float pribizhuang = 0.0f;
	float pribixian = 0.0f;
	pribixian = 0.46265*(1 - probirity[2]);
	pribizhuang = 0.46265*(1 - probirity[1]);
	float kaizhuang = 0.9253*pribizhuang / (pribizhuang + pribixian);
	float kaixian = 0.9253*pribixian / (pribixian + pribizhuang);
	float kaihe = 0.0747;
	//出红是否换牌概率
	openLog("黑名单控制开庄的概率:%f    黑名单控制开闲的概率:%f", kaizhuang, kaixian);
	float pribility = m_random.betweenFloat(0.0, 1.0).randFloat_mt(true);
	if (pribility > 0.9253)
	{
		wintag = eHe;
		openLog("黑名单要求开和哦");
	}
	else
	{
		if (pribility < kaizhuang)
		{
			wintag = eZhuang;
			openLog("黑名单要求开庄哦");
		}
		else
		{
			wintag = eXian;
			openLog("黑名单要求开闲哦");
		}
	}

	long winscore = 0;
	if (wintag == eHe)
	{
		winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
	}
	else if (wintag == eZhuang)
	{
		winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
	}
	else
	{
		winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
	}
	miCurrentProfit = winscore;
    std::vector<int64_t> v_wintag;
	v_wintag.clear();
	int opentag = 0;
	if (m_killOrReword == 1)
	{
		for (int i = 12;i < 14;i++)
		{
			int opentag = i;
			long winscore = 0;
			if (opentag == eHe)
			{
				winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
			}
			else if (opentag == eZhuang)
			{
				winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
			}
			else
			{
				winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
			}
			if (winscore < 0)
			{
				v_wintag.push_back(winscore);
			}


		}
		int jishi = 0;
		for (int i = 0;i < v_wintag.size();i++)
		{
			if (v_wintag[i] >= -m_thresholdNum || v_wintag[i] >= -(m_curProfit - m_lowLimit) / 2)
			{

			}
			else
			{
				jishi++;
			}
		}
		if (v_wintag.size() <= 0 || jishi == v_wintag.size())//没有结果或者结果都是不合法
		{
			m_killOrReword = 0;
		}
	}
	bool isOk = false;

	//if(m_killOrReword==-1||m_killOrReword==1)
	{
		do
		{
			isOk = false;
			//公平百分之五十出随机结果
			if (betting <= 0)
			{
				break;
			}
			long winscore1 = 0;
			if (wintag == eHe)
			{
				winscore1 = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
			}
			else if (wintag == eZhuang)
			{
				winscore1 = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
			}
			else
			{
				winscore1 = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
			}
			miCurrentProfit = winscore1;
			if (m_killOrReword == -1)
			{
				if (winscore1 < 0)
				{
					isOk = true;//系统杀分时，本次系统赢分小于零则继续
				}
			}
			else if (m_killOrReword == 1)
			{
				if (winscore1 > 0)
				{
					isOk = true;//系统放分时，本次盈利大于零则继续
				}
				if (winscore1 < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
				{
					isOk = true;//系统放分时，超过红线则继续
				}
			}
			else
			{
				if (winscore1 < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
				{
					isOk = true;//系统放分时，超过红线则继续
				}

			}
			if (isOk)
			{
				int8_t ZhuangRes = 0;
				int8_t XianRes = 0;
				ZhuangRes = m_random.betweenInt(1, 13).randInt_mt(true);
				XianRes = m_random.betweenInt(1, 13).randInt_mt(true);
				if (ZhuangRes == XianRes)
				{
					wintag = eHe;//和
				}
				else if (ZhuangRes > XianRes)
				{
					wintag = eZhuang;//庄
				}
				else
				{
					wintag = eXian;//闲
				}
			}
			else
			{
				break;
			}

		} while (isOk);//系统要放分的时候，本次系统盈利要大于零
		openLog("黑名单庄闲算法最终开奖: %d ", wintag);
	}
}
void SlwhAlgorithm::GetOpenCard_HZX(int &wintag)
{
	bool isOk = false;
    std::vector<int64_t> v_wintag;
	v_wintag.clear();
	int opentag = 0;
	if (m_killOrReword == 1)
	{
		for (int i = 12;i < 14;i++)
		{
			int opentag = i;
			int64_t betting = bettingQuantity[eHe] + bettingQuantity[eZhuang] + bettingQuantity[eXian];
			long winscore = 0;
			if (opentag == eHe)
			{
				winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
			}
			else if (opentag == eZhuang)
			{
				winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
			}
			else
			{
				winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
			}
			if (winscore < 0)
			{
				v_wintag.push_back(winscore);
			}


		}
		int jishi = 0;
		for (int i = 0;i < v_wintag.size();i++)
		{
			if (v_wintag[i] >= -m_thresholdNum || v_wintag[i] >= -(m_curProfit - m_lowLimit) / 2)
			{

			}
			else
			{
				jishi++;
			}
		}
		if (v_wintag.size() <= 0 || jishi == v_wintag.size())//没有结果或者结果都是不合法
		{
			m_killOrReword = 0;
		}
	}

	do
	{
		isOk = false;
		wintag = eHe;
		int8_t ZhuangRes = 0;
		int8_t XianRes = 0;
		ZhuangRes = m_random.betweenInt(1, 13).randInt_mt(true);
		XianRes = m_random.betweenInt(1, 13).randInt_mt(true);
		if (ZhuangRes == XianRes)
		{
			wintag = eHe;//和
		}
		else if (ZhuangRes > XianRes)
		{
			wintag = eZhuang;//庄
		}
		else
		{
			wintag = eXian;//闲
		}
		//公平百分之五十出随机结果
		int64_t betting = bettingQuantity[eHe] + bettingQuantity[eZhuang] + bettingQuantity[eXian];
		if (betting <= 0)
		{
			break;
		}
		long winscore = 0;
		if (wintag == eHe)
		{
			winscore = -bettingQuantity[eHe] * (m_muticleArr[eHe] - 1);
		}
		else if (wintag == eZhuang)
		{
			winscore = -bettingQuantity[eZhuang] * (m_muticleArr[eZhuang] - 1) + bettingQuantity[eHe] + bettingQuantity[eXian];
		}
		else
		{
			winscore = -bettingQuantity[eXian] * (m_muticleArr[eXian] - 1) + bettingQuantity[eHe] + bettingQuantity[eZhuang];
		}
		miCurrentProfit = winscore;
		if (m_killOrReword == -1)
		{
			if (winscore < 0)
			{
				isOk = true;//系统杀分时，本次系统赢分小于零则继续
			}
		}
		else if (m_killOrReword == 1)
		{
			if (winscore > 0)
			{
				isOk = true;//系统放分时，本次盈利大于零则继续
			}
			if (winscore < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
			{
				isOk = true;//系统放分时，超过红线则继续
			}
		}
		else
		{
			if (winscore < -m_thresholdNum && winscore < -(m_curProfit - m_lowLimit) / 2)
			{
				isOk = true;//随机时，超过红线则继续
			}
		}
	} while (isOk);//系统要放分的时候，本次系统盈利要大于零
	openLog("庄闲算法最终开奖: %d ", wintag);
}

//必杀黑名单玩家特殊处理
bool SlwhAlgorithm::KillBlackGetOpenCard(int &winTag, int &typeTag, int64_t userBetScore[])
{
	// 结果数组
	vector<int32_t> tempWinRetIdx;
	vector<int32_t> tempLoseRetIdx;
	vector<int32_t> tempAllRetIdx;
    vector<int64_t> tempLoseScore;
	// 筛选出结果
	// 允许最大输分值
	int64_t maxScore = (m_curProfit - m_lowLimit) * m_stockCtrlRate;
	openLog("允许最大输分值 maxScore=%d, m_stockCtrlRate=%f, curStor=%d, lowStor=%d;", maxScore, m_stockCtrlRate, m_curProfit, m_lowLimit);
	// 统计动物总押分
    int64_t tempUserAllBet = 0;
    int64_t tempAllBet = 0;
	for (int index = 0; index < MaxGate - 3; index++)
	{
		tempAllBet += bettingQuantity[index];
		tempUserAllBet += userBetScore[index];
	}
	//库存正常才杀
	if (tempUserAllBet <= 0 || tempUserAllBet < m_lMustKillLimitScore || m_profitInterval != 0)
		return false;
	// 筛选出结果
	string winStr = "黑名单系统可赢选项:", loseStr = "黑名单系统输选项:";
	for (int index = 0; index < MaxGate-3; index++)
	{
		// 押分 *  倍数 < 允许最大输分值
        int64_t tempItemWin = bettingQuantity[index] * m_muticleArr[index];
        int64_t tempUserItemWin = userBetScore[index] * m_muticleArr[index];
		
		//黑名单玩家输分
		if (tempUserItemWin <= tempUserAllBet)
		{
			// 系统赢分
			if (tempItemWin <= tempAllBet)
			{
				tempWinRetIdx.push_back(index);
				winStr += to_string(index) + " ";
				tempAllRetIdx.push_back(index);
			}
			else
			{
				// 系统输分
				if ((tempItemWin - tempAllBet) < std::max(maxScore, (int64_t)m_thresholdNum * 100))
				{
					tempLoseRetIdx.push_back(index);
					loseStr += to_string(index) + " ";
					tempAllRetIdx.push_back(index);
					tempLoseScore.push_back(tempItemWin - tempAllBet);
				}
			}
		}		
	}
	openLog("%s;", winStr.c_str());
	openLog("%s;", loseStr.c_str());
	// 可选择结果
	int allSize = tempAllRetIdx.size();
	if (allSize <= 0)
	{
		return false;
	}
	else
	{
		int winSize = tempWinRetIdx.size();
		if (tempWinRetIdx.size() > 0)
		{
			sort(tempWinRetIdx.begin(), tempWinRetIdx.end());
			int tempResIdx = m_random.betweenInt(0, winSize - 1).randInt_mt(true);
			winTag = tempWinRetIdx[tempResIdx];
			typeTag = 0;
		} 
		else if (tempLoseRetIdx.size() > 0)  // 系统输分，选择输分最少那个
		{
			int size = tempLoseScore.size();
            int64_t loseMinScore = tempLoseScore[0];
            int64_t chooseIndex = tempLoseRetIdx[0];
			for (int i = 1;i < size;i++)
			{
				if (tempLoseScore[i] < loseMinScore)
				{
					loseMinScore = tempLoseScore[i];
					chooseIndex = tempLoseRetIdx[i];
				}
			}
			winTag = chooseIndex;
			typeTag = 0;
		}
		else
		{
			return false;
		}
	}	
	openLog("必杀黑名单结果 类型:%d,动物:%d \n", typeTag, winTag);
	return true;
}

