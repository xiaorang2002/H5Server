#include "BrnnAlgorithm.h"

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

BrnnAlgorithm::BrnnAlgorithm()
{
    for(int i=0;i<4;i++)
        m_muticleArr[i]=2;
    miCurrentProfit=0;

    m_isTenMuti=false;
}
//MAX_CARDS = 52 * 8
uint8_t BrnnAlgorithm::m_cbTableCard[52] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
};

void  BrnnAlgorithm::CalculatCotrol()   //计算出第一步是杀还是放还是随机
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
int64_t BrnnAlgorithm::CurrentProfit()//计算本局不算税的系统赢输
{
    return miCurrentProfit;
}
void  BrnnAlgorithm::openLog(const char *p, ...)
{
    {
        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "log//brnn//brnn_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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
void BrnnAlgorithm::BlackGetOpenCard(uint8_t m_cbCurrentRoundTableCards[5][5],int cardtype[],int mutical[],float probirity[4])
{
    //公平百分之五十出随机结果
    int64_t betting=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2]+bettingQuantity[3];
    //先随机生成一个吧,不然返回了啥都没有
    m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克
    int allMucl[4]={0};
    int index = 0;
    for(int i = 0; i < 5; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];        
        }
        cardtype[i]=m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i],5);
    }
    for(int i=0;i<4;i++)
    {
        allMucl[i]= m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i+1],m_cbCurrentRoundTableCards[0],5);
        mutical[i]=allMucl[i];
        if(allMucl[i]>0)
        {
            allMucl[i]=1;
        }
        else
        {
            allMucl[i]=0;
        }
    }
    if(betting<=0)
    {
        return;
    }
    int winTag[4]={0};
    float outPro[4]={0.0};
    for(int i=0;i<4;i++)
    {
       outPro[i]=0.5*(1-probirity[i]);
    }
    //出红是否换牌概率
    openLog("黑名单控制开天门的概率:%f    黑名单控制开地门的概率:%f      黑名单控制开玄门的概率:%f      黑名单控制开黄门的概率:%f",outPro[0],outPro[1],outPro[2],outPro[3]);

    for(int i=0;i<4;i++)
    {
        float pribility=m_random.betweenFloat(0.0,1.0).randFloat_mt(true);
        if(pribility<outPro[i])
        {
           winTag[i]=1;
           openLog("黑名单概率控制开出的结果:%d 门赢  ",i);
        }
        else
        {
           winTag[i]=0;
           openLog("黑名单概率控制开出的结果:%d 门输  ",i);
        }
    }
    do
    {
        m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克

        int index = 0;
        for(int i = 0; i < 5; ++i)
        {
            for(int j = 0; j < 5; ++j)
            {
                m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];               
            }
            cardtype[i]=m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i],5);
        }
        int allMucl[4]={0};
        for(int i=0;i<4;i++)
        {
            allMucl[i]= m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i+1],m_cbCurrentRoundTableCards[0],5);
            mutical[i]=allMucl[i];
            if(allMucl[i]>0)
            {
                allMucl[i]=1;
            }
            else
            {
                allMucl[i]=0;
            }
        }

        if(winTag[0]==allMucl[0]&&winTag[1]==allMucl[1]&&winTag[2]==allMucl[2]&&winTag[3]==allMucl[3])
        {
            break;
        }
    }while(true);

    //扑克牌出来了，算出扑克赔率，相对庄家的

    int64_t iTianWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[0];
    int64_t iDiWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[1];
    int64_t iXuanWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[2];
    int64_t iHuangWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[3];
    int64_t systemWin=-(iTianWin+iDiWin+iHuangWin+iXuanWin);

    miCurrentProfit=systemWin;

   bool isOk=false;
   {
       do
       {

           isOk=false;

           for(int i=0;i<4;i++)
           {
               allMucl[i]= m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i+1],m_cbCurrentRoundTableCards[0],5);
               mutical[i]=allMucl[i];
               if(allMucl[i]>0)
               {
                   allMucl[i]=1;
               }
               else
               {
                   allMucl[i]=0;
               }
           }
           for(int i = 0; i < 5; ++i)
           {
               cardtype[i]=m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i],5);
           }
           //公平百分之五十出随机结果
           int64_t iTianWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[0];
           int64_t iDiWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[1];
           int64_t iXuanWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[2];
           int64_t iHuangWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[3];
           int64_t systemWin1=-(iTianWin1+iDiWin1+iHuangWin1+iXuanWin1);

           miCurrentProfit=systemWin1;
           if(m_killOrReword==-1)
           {
               if(systemWin1<0)
               {
                   openLog("由于系统下线杀分，需要改边结果");
                   isOk=true;//系统杀分时，本次系统赢分小于零则继续
               }
           }
           if(m_killOrReword==1)
           {
               if(systemWin1>0)
               {
                   openLog("由于系统上线放分，需要改边黑名单控制结果");
                   isOk=true;//系统放分时，本次盈利大于零则继续
               }
               if(systemWin1<-m_thresholdNum&&systemWin1<-(m_curProfit-m_lowLimit)/2)
               {
                   openLog("由于系统上线放分，但是触发保险了,需要改边黑名单控制结果");
                   isOk=true;//系统放分时，超过红线则继续
               }
           }
           if(systemWin1<=-m_thresholdNum&&systemWin1<=-(m_curProfit-m_lowLimit)/2)
           {
               openLog("触发保险了,需要改边黑名单控制结果");
               isOk=true;//系统随机时，超过红线则继续
           }
           if(isOk)
           {
               m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克

               int index = 0;
               for(int i = 0; i < 5; ++i)
               {
                   for(int j = 0; j < 5; ++j)
                   {
                       m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];
                   }
               }
           }

       }while(isOk);//系统要放分的时候，本次系统盈利要大于零
       for(int i=0;i<4;i++)
       {
           if(winTag[i])
           {
              openLog("黑名单结果被上下限红修改以后的结果是:%d 门赢  ",i);
           }
           else
           {
              openLog("黑名单结果被上下限红修改以后的结果是:%d 门输  ",i);
           }
       }
   }


}
void BrnnAlgorithm::GetOpenCard(uint8_t m_cbCurrentRoundTableCards[5][5],int cardtype[],int mutical[])
{

   int8_t winTag[4] ={0};
   for(int i=0;i<4;i++)
        winTag[i]=0;//每门的赢输结果，-1是输，1是赢
   bool isOk=false;
   int pushin=bettingQuantity[0]+bettingQuantity[1]+bettingQuantity[2]+bettingQuantity[3];
    do
    {

        isOk=false;
        //随机生成赢输牌型

	Nunber0:
        m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克
        int index = 0;
        for(int i = 0; i < 5; ++i)
        {
            for(int j = 0; j < 5; ++j)
            {
                m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];

            }
            cardtype[i]=m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i],5);
        }
        int allMucl[4]={0};
        for(int i=0;i<4;i++)
        {
            allMucl[i]= m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i+1],m_cbCurrentRoundTableCards[0],5);
            mutical[i]=allMucl[i];
            if(allMucl[i]>0)
            {
                allMucl[i]=1;
                winTag[i]=1;
            }
            else
            {
                allMucl[i]=0;
                 winTag[i]=0;
            }
        }

        //公平百分之五十出随机结果
        int64_t iTianWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[0];
        int64_t iDiWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[1];
        int64_t iXuanWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[2];
        int64_t iHuangWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4],m_cbCurrentRoundTableCards[0],5)*bettingQuantity[3];
        int64_t systemWin1=-(iTianWin1+iDiWin1+iHuangWin1+iXuanWin1);

        miCurrentProfit=systemWin1;
        if(m_killOrReword==-1)
        {
            if(systemWin1<0)
            {
                isOk=true;//系统杀分时，本次系统赢分小于零则继续
            }
        }
        else if(m_killOrReword==1)
        {
            if(systemWin1>0)
            {

                isOk=true;//系统放分时，本次盈利大于零则继续
            }
        }
        else
        {

        }
        if(systemWin1<=-m_thresholdNum&&systemWin1<=-(m_curProfit-m_lowLimit)/2)
        {
            openLog("正常结果由于触红线，修改结果");
            openLog("正常结果由于触红线，修改结果");
            isOk=true;//随机时，超过红线则继续
        }
    }while(isOk);//系统要放分的时候，本次系统盈利要大于零
}
double BrnnAlgorithm::OverHighLowLimit() //库存高于上限或者库存低于下限时返回一个概率
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
uint8_t BrnnAlgorithm::GetMultiple(uint8_t cbCardData[])
{
	uint8_t  cbCardType = m_GameLogic.SortNiuNiuCard(cbCardData, MAX_COUNT);
	if (cbCardType <= 8)return NORMAL_MUL;
	else if (cbCardType <= 10)return NN_MUL;
	else if (cbCardType == 13)return BOMB_MUL;
	else return SPECIAL_MUL;

	return 0;
}
// 真实玩家坐庄
void BrnnAlgorithm::GetOpenCard_UserBancker(uint8_t m_cbCurrentRoundTableCards[5][5], int cardtype[], int mutical[])
{
	int8_t winTag[4] = { 0 };
	for (int i = 0;i < 4;i++)
		winTag[i] = 0;//每门的赢输结果，-1是输，1是赢
	bool isOk = false;
    int64_t pushin = bettingQuantity[0] + bettingQuantity[1] + bettingQuantity[2] + bettingQuantity[3];
	do
	{
		isOk = false;
		//随机生成赢输牌型
		m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克
		int index = 0;
		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 5; ++j)
			{
				m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];

			}
			cardtype[i] = m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i], 5);
		}
		int allMucl[4] = { 0 };
		for (int i = 0;i < 4;i++)
		{
			allMucl[i] = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i + 1], m_cbCurrentRoundTableCards[0], 5);
			mutical[i] = allMucl[i];
			if (allMucl[i] > 0)
			{
				allMucl[i] = 1;
				winTag[i] = 1;
			}
			else
			{
				allMucl[i] = 0;
				winTag[i] = 0;
			}
		}
		//公平百分之五十出随机结果
		int64_t iTianWin1 = mutical[0] * bettingQuantity[0];
		int64_t iDiWin1 = mutical[1] * bettingQuantity[1];
		int64_t iXuanWin1 = mutical[2] * bettingQuantity[2];
		int64_t iHuangWin1 = mutical[3] * bettingQuantity[3];
		int64_t systemWin1 =(iTianWin1 + iDiWin1 + iHuangWin1 + iXuanWin1);

		miCurrentProfit = systemWin1;
		if (m_killOrReword == -1)
		{
			if (systemWin1 < 0)
			{
				isOk = true;//系统杀分时，本次系统赢分小于零则继续
			}
		}
		else if (m_killOrReword == 1)
		{
			if (systemWin1 > 0)
			{
				isOk = true;//系统放分时，本次盈利大于零则继续
			}
		}
		else
		{
		}
		if (systemWin1 <= -m_thresholdNum && systemWin1 <= -(m_curProfit - m_lowLimit) / 2)
		{
			openLog("正常结果由于触红线，修改结果");
			openLog("正常结果由于触红线，修改结果");
			isOk = true;//随机时，超过红线则继续
		}
	} while (isOk);//系统要放分的时候，本次系统盈利要大于零
}
// 真实玩家坐庄，存在黑名单玩家
void BrnnAlgorithm::BlackGetOpenCard_UserBancker(uint8_t m_cbCurrentRoundTableCards[5][5], int cardtype[], int mutical[], float probirity[4])
{
	//公平百分之五十出随机结果
	int64_t betting = bettingQuantity[0] + bettingQuantity[1] + bettingQuantity[2] + bettingQuantity[3];
	//先随机生成一个吧,不然返回了啥都没有
	m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克
	int allMucl[4] = { 0 };
	int index = 0;
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 5; ++j)
		{
			m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];
		}
		cardtype[i] = m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i], 5);
	}
	for (int i = 0;i < 4;i++)
	{
		allMucl[i] = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i + 1], m_cbCurrentRoundTableCards[0], 5);
		mutical[i] = allMucl[i];
		if (allMucl[i] > 0)
		{
			allMucl[i] = 1;
		}
		else
		{
			allMucl[i] = 0;
		}
	}
	if (betting <= 0)
	{
		return;
	}
	int winTag[4] = { 0 };
	float outPro[4] = { 0.0 };
	for (int i = 0;i < 4;i++)
	{
		outPro[i] = 0.5*(1 - probirity[i]);
	}
	//出红是否换牌概率
	openLog("黑名单控制开天门的概率:%f    黑名单控制开地门的概率:%f      黑名单控制开玄门的概率:%f      黑名单控制开黄门的概率:%f", outPro[0], outPro[1], outPro[2], outPro[3]);

	for (int i = 0;i < 4;i++)
	{
		float pribility = m_random.betweenFloat(0.0, 1.0).randFloat_mt(true);
		if (pribility < outPro[i])
		{
			winTag[i] = 1;
			openLog("黑名单概率控制开出的结果:%d 门赢  ", i);
		}
		else
		{
			winTag[i] = 0;
			openLog("黑名单概率控制开出的结果:%d 门输  ", i);
		}
	}
	do
	{
		m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克

		int index = 0;
		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 5; ++j)
			{
				m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];
			}
			cardtype[i] = m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i], 5);
		}
		int allMucl[4] = { 0 };
		for (int i = 0;i < 4;i++)
		{
			allMucl[i] = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i + 1], m_cbCurrentRoundTableCards[0], 5);
			mutical[i] = allMucl[i];
			if (allMucl[i] > 0)
			{
				allMucl[i] = 1;
			}
			else
			{
				allMucl[i] = 0;
			}
		}

		if (winTag[0] == allMucl[0] && winTag[1] == allMucl[1] && winTag[2] == allMucl[2] && winTag[3] == allMucl[3])
		{
			break;
		}
	} while (true);

	//扑克牌出来了，算出扑克赔率，相对庄家的

	int64_t iTianWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[0];
	int64_t iDiWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[1];
	int64_t iXuanWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[2];
	int64_t iHuangWin = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[3];
	int64_t systemWin = (iTianWin + iDiWin + iHuangWin + iXuanWin);

	miCurrentProfit = systemWin;

	bool isOk = false;
	{
		do
		{

			isOk = false;

			for (int i = 0;i < 4;i++)
			{
				allMucl[i] = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[i + 1], m_cbCurrentRoundTableCards[0], 5);
				mutical[i] = allMucl[i];
				if (allMucl[i] > 0)
				{
					allMucl[i] = 1;
				}
				else
				{
					allMucl[i] = 0;
				}
			}
			for (int i = 0; i < 5; ++i)
			{
				cardtype[i] = m_GameLogic.SortNiuNiuCard(m_cbCurrentRoundTableCards[i], 5);
			}
			//公平百分之五十出随机结果
			int64_t iTianWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[1], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[0];
			int64_t iDiWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[2], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[1];
			int64_t iXuanWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[3], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[2];
			int64_t iHuangWin1 = m_GameLogic.CalculateMultiples(m_cbCurrentRoundTableCards[4], m_cbCurrentRoundTableCards[0], 5)*bettingQuantity[3];
			int64_t systemWin1 = (iTianWin1 + iDiWin1 + iHuangWin1 + iXuanWin1);

			miCurrentProfit = systemWin1;
			if (m_killOrReword == -1)
			{
				if (systemWin1 < 0)
				{
					openLog("由于系统下线杀分，需要改边结果");
					isOk = true;//系统杀分时，本次系统赢分小于零则继续
				}
			}
			if (m_killOrReword == 1)
			{
				if (systemWin1 > 0)
				{
					openLog("由于系统上线放分，需要改边黑名单控制结果");
					isOk = true;//系统放分时，本次盈利大于零则继续
				}
				if (systemWin1 < -m_thresholdNum && systemWin1 < -(m_curProfit - m_lowLimit) / 2)
				{
					openLog("由于系统上线放分，但是触发保险了,需要改边黑名单控制结果");
					isOk = true;//系统放分时，超过红线则继续
				}
			}
			if (systemWin1 <= -m_thresholdNum && systemWin1 <= -(m_curProfit - m_lowLimit) / 2)
			{
				openLog("触发保险了,需要改边黑名单控制结果");
				isOk = true;//系统随机时，超过红线则继续
			}
			if (isOk)
			{
				m_GameLogic.RandCardList(m_cbTableCard, 52);            //混乱扑克

				int index = 0;
				for (int i = 0; i < 5; ++i)
				{
					for (int j = 0; j < 5; ++j)
					{
						m_cbCurrentRoundTableCards[i][j] = m_cbTableCard[index++];
					}
				}
			}

		} while (isOk);//系统要放分的时候，本次系统盈利要大于零
		for (int i = 0;i < 4;i++)
		{
			if (winTag[i])
			{
				openLog("黑名单结果被上下限红修改以后的结果是:%d 门赢  ", i);
			}
			else
			{
				openLog("黑名单结果被上下限红修改以后的结果是:%d 门输  ", i);
			}
		}
	}


}

