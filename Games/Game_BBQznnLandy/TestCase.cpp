#include "TestCase.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <glog/logging.h>
#include "Log.h"
#include <string>
#include <iostream>
#include <vector>

//#define BOOST_SPIRIT_THREADSAFE
using namespace std;

//扑克数据
const uint8_t CTestCase::m_byCardDataArray[52]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,   //方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,   //梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,   //红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D    //黑桃 A - K
};

CTestCase::CTestCase()
{
    m_cardCount = 5;
    m_bhaveKing = false;
    haveUseCard.clear();
    m_randCount = 0;
    resetLestCard();
    
}

CTestCase::~CTestCase()
{

}

void CTestCase::clearHaveCard()
{
    haveUseCard.clear();
    resetLestCard();
}
void CTestCase::resetLestCard()
{
    lestCard.clear();
    std::vector<uint8_t> v;
    for (uint8_t i = 0; i < 4; ++i)
    {
        v.clear();
        for (uint8_t j = 1; j <= 13; ++j)
        {
            v.push_back(j+i*16);
        }
        // lestCard.insert(pair<uint8_t,vector<uint8_t>>(i,v));
        lestCard[i] = v;
    } 
    for (uint8_t i = 0; i < 13; ++i)
    {
        m_cardIndex[i] = 4;
    }
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 13; ++j)
        {
            m_colorIndex[i][j]=1;
        }
    }
}
bool CTestCase::isHaveSameCard(uint8_t cbCardData[],uint8_t cardcount)
{
    bool bSame = false;
	bool bHaveInvalid = false; //有无效的牌值
	if (!CheckHanCardIsOK(cbCardData, cardcount))
	{
		return true;
	}
    for (int i = 0; i < cardcount; ++i)
    {
        if (0 == cbCardData[i])
		{
			bHaveInvalid = true;
			bSame = true;
			break;
		}
        for(auto data : haveUseCard)
        {
            if (data==cbCardData[i])
            {
                bSame = true;
            }
            if(bSame) break;
        }
        if(bSame) break;
    }
    if (!bSame)
    { 
        addHaveChooseCard(cbCardData,cardcount);      
    }
    return bSame;
}

void CTestCase::addHaveChooseCard(uint8_t cbCardData[],uint8_t cardcount)
{
    uint8_t color = 0;
    uint8_t value = 0;
    for (int i = 0; i < cardcount; ++i)
    {
        haveUseCard.push_back(cbCardData[i]);
        color = GetCardColor(cbCardData[i]);
        value = GetCardValue(cbCardData[i]);
        if (color<4)
        {
            if (m_cardIndex[value-1]>0)
                m_cardIndex[value-1]--;
            m_colorIndex[color][value-1] = 0;
        }
    } 
}

void CTestCase::setCardCount(uint8_t count)
{
    m_cardCount = count;
	enabled = false;
	android = false;
    if(ENABLE_TEST)
        loadConf(m_cardCount);
}
void CTestCase::loadConf(uint8_t cardCount)
{
	cases.clear();
	string strfile = cardCount == 5 ? CASE_FILE : CASE_FILE_SIX;

	//if (cardCount == 5)
	//{
        if(boost::filesystem::exists(strfile))
        {
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(strfile,pt);
            boost::property_tree::ptree::iterator it0;
            boost::property_tree::ptree element;

            int idx = 0;
            for(boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it)
            {
				if (it->first != ENABLE_KEY && it->first != ANDROID_KEY)
				{
					vector<uint8_t> cards(cardCount);
					element = pt.get_child(it->first);
					idx = 0;
					for (it0 = element.begin();it0 != element.end(); it0++)
					{
						string str = it0->second.get_value<string>();
						cards[idx] = strtol(str.c_str(), nullptr, 16);
						idx++;
					}
					cases.insert(pair<string, vector<uint8_t>>(it->first, cards));
				}
				else
				{
					if (it->first == ENABLE_KEY)
					{
						enabled = it->second.get_value<int>() == 1;
					}
					else if (it->first == ANDROID_KEY)
					{
						android = it->second.get_value<int>() == 1;
					}
				}
            }
        }
    //}
    /*else
    {
        if(boost::filesystem::exists(CASE_FILE_SIX))
        {
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(CASE_FILE_SIX,pt);
            boost::property_tree::ptree::iterator it0;
            boost::property_tree::ptree element;

            int idx = 0;
            for(boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it)
            {
				if (it->first != ENABLE_KEY && it->first != ANDROID_KEY)
				{
					vector<uint8_t> cards(6);
					element = pt.get_child(it->first);
					idx = 0;
					for (it0 = element.begin();it0 != element.end(); it0++)
					{
						string str = it0->second.get_value<string>();
						cards[idx] = strtol(str.c_str(), nullptr, 16);
						idx++;
					}
					cases.insert(pair<string, vector<uint8_t>>(it->first, cards));
				}
				else
				{
					if (it->first == ENABLE_KEY)
					{
						enabled = it->second.get_value<int>() == 1;
					}
					else if (it->first == ANDROID_KEY)
					{
						android = it->second.get_value<int>() == 1;
					}
				}
            }
        }
    }*/    
}

void CTestCase::CheckAdr(string account, uint8_t carddata[], uint8_t cardcout)
{
	map<string, vector<uint8_t>>::iterator it = cases.find(account);
	if (it != cases.end())
	{
		vector<uint8_t> cards = it->second;
		int nums = 0;
		for (uint8_t i = 0; i < cardcout; i++)
		{
			carddata[i] = cards[i];
			if (cards[i] != 0)
			{
				nums++;
			}
		}
	}
}
bool CTestCase::Check(string account, uint8_t carddata[],uint8_t cardcout,uint8_t cardType)
{
    map<string, vector<uint8_t>>::iterator it = cases.find(account);
    bool bChooseSuccess = false;
    if(it != cases.end())
    {
        vector<uint8_t> cards = it->second;
        int nums = 0;
        for(uint8_t i =0; i < cardcout ; i++)
        {
            carddata[i] = cards[i];
            if( cards[i] != 0)
            {
                nums++;
            }
        }

        if(nums == 1)
        {
            switch (cards[0]) {
            case 17: //同花顺牛
                bChooseSuccess = randTongHuaShun(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 16: //五小牛
                bChooseSuccess = randFiveSmallNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 15: //炸弹牛
                bChooseSuccess = randBomb(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randTongHuaNiu(carddata,cardcout);
                }
                break;
            case 14: //葫芦牛
                bChooseSuccess = randHuluNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randTongHuaNiu(carddata,cardcout);
                }
                break;
            case 13: //同花牛
                bChooseSuccess = randTongHuaNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            case 12: //五花牛
                bChooseSuccess = randWuHuaNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            case 11: //顺子牛
                bChooseSuccess = randShunziNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            case 10: //普通牛
                bChooseSuccess = randNN(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            default:
                bChooseSuccess = randNNum(cards[0],carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            }
        }else if(cards.size() == cardcout && nums == 0)
        {
            bChooseSuccess = randNNum(0,carddata,cardcout);
        }
		else
		{
			bChooseSuccess = true;
		}
    }
    else
    {
        switch (cardType)
        {
            case 17: //同花顺牛
                bChooseSuccess = randTongHuaShun(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 16: //五小牛
                bChooseSuccess = randFiveSmallNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 15: //炸弹牛
                bChooseSuccess = randBomb(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 14: //葫芦牛
                bChooseSuccess = randHuluNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randTongHuaNiu(carddata,cardcout);
                }
                break;
            case 13: //同花牛
                bChooseSuccess = randTongHuaNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 12: //五花牛
                bChooseSuccess = randWuHuaNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            case 11: //顺子牛
                bChooseSuccess = randShunziNiu(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                break;
            case 10: //普通牛
                bChooseSuccess = randNN(carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
            default:
                bChooseSuccess = randNNum(cardType,carddata,cardcout);
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randHuluNiu(carddata,cardcout);
                }
                if (!bChooseSuccess)
                {
                    bChooseSuccess = randShunziNiu(carddata,cardcout);
                }
                break;
        }
    }
    return bChooseSuccess;
}
bool CTestCase::randTongHuaShun(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        memcpy(TmpcardIndex,m_cardIndex,sizeof(TmpcardIndex));
        memcpy(TmpcolorIndex,m_colorIndex,sizeof(TmpcolorIndex));
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        m_randCount++;
        uint8_t startPos = 0;
        uint8_t endPos = 0;
        uint8_t lengPos = 0;
        bool bHave = false;
        int index[4] = {0,1,2,3};
        std::shuffle(&index[0], &index[4], STD::Generator::instance().get_mt());
        for (uint8_t i = 0; i < 4; ++i) //花色
        {
            for (uint8_t j = 0; j < 9; ++j) //牌值
            {
                if (TmpcolorIndex[index[i]][j]>0)
                {
                    lengPos = 0;
                    startPos = j;
                    for (uint8_t k = startPos; k < 13; ++k)
                    {
                        if (TmpcolorIndex[index[i]][k]>0)
                            lengPos++;
                        else
                            break;
                    }
                    if (lengPos>=5) //有顺子
                    {
                        bHave = true;
                        uint8_t count = 0;
                        uint8_t startIndex = m_random.betweenInt(0,lengPos-5).randInt_mt(true) + startPos;
                        for (uint8_t m = startIndex; m < startIndex+5; ++m)
                        {
							if (TmpcardIndex[m] > 0)
							{
								carddata[count++] = m + 16 * index[i] + 0x01;
								TmpcolorIndex[index[i]][m] = 0;
								TmpcardIndex[m]--;
							}
                        }
                    }
                    if (bHave) break;
                }
            }
            if (bHave) break;
        }        
        // if (!bHave)
        // {
        //     return bChooseSuccess;
        // }
        
        if (cardcout==6)
        {
            bool bHaveSame = false;
            do
            {
                bHaveSame = false;
                m_randCount++;
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + (16 * rand()%4);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + (16 * rand()%4); 
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame && m_randCount<CHOOSE_COUNT/2);
            if (m_randCount>=CHOOSE_COUNT/2)
            {
                bool bChoose = false;
                int index[4] = {0,1,2,3};
                std::shuffle(&index[0], &index[4], STD::Generator::instance().get_mt());
                for (uint8_t i = 0; i < 4; ++i)
                {
                    for (uint8_t j = 0; j < 13; ++j)
                    {
                        if (TmpcolorIndex[index[i]][j]>0 && TmpcardIndex[j]>0)
                        {
                            carddata[5] = j + 16*index[i] + 0x01;
                            TmpcolorIndex[index[i]][j] = 0;
                            TmpcardIndex[j]--;
                            bChoose = true;
                            break;
                        }
                    }
                    if (bChoose) break;
                }
            }
        }
    } while (isHaveSameCard(carddata,cardcout) && m_randCount<CHOOSE_COUNT);  
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}
bool CTestCase::randFiveSmallNiu(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    m_randCount = 0;
    do
    {
        uint8_t color = rand()%4;
        uint8_t num = rand()%0x04 + 1;
        bool bHaveSame = false;
        uint8_t allValue = 0;
        m_randCount++;
        do
        {
            bHaveSame = false;
            allValue = 0;
            m_randCount++;
            for (int i = 0; i < 5; ++i)
            {
				/*do 
				{
					carddata[i] = m_random.betweenInt(0, 3).randInt_mt(true) + 0x01 + 16 * (rand() % 4);
				} while (carddata[i] == (uint8_t)0 && m_randCount < CHOOSE_COUNT);*/
				carddata[i] = m_random.betweenInt(0, 3).randInt_mt(true) + 0x01 + 16 * (rand() % 4);
                allValue += GetCardValue(carddata[i]);
            }
            for (int i = 0; i < 5; ++i)
            {
                for (int j = i+1; j < 5; ++j)
                {
                    if (carddata[i]==carddata[j])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
                if (bHaveSame) break;
            }
        } while ((bHaveSame || allValue>10)&&m_randCount<CHOOSE_COUNT);   
        if (cardcout==6)
        {
            bool bSame = false;
            do
            {
                bSame = false; 
                m_randCount++;               
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + (16 * rand()%4);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + (16 * rand()%4); 
                } 

                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5] == carddata[i])
                    {
                        bSame = true;
                    }
                }
            } while (bSame&&m_randCount<CHOOSE_COUNT/2);      
        }
    } while (isHaveSameCard(carddata,cardcout) && m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
	for (int i = 0; i < 5; ++i)
	{
		if (carddata[i]==0)
		{
			bChooseSuccess = false;
			break;
		}
	}
    return bChooseSuccess;
}
bool CTestCase::randHuluNiu(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        memcpy(TmpcardIndex,m_cardIndex,sizeof(TmpcardIndex));
        memcpy(TmpcolorIndex,m_colorIndex,sizeof(TmpcolorIndex));
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        m_randCount++;
        uint8_t lengPos = 0;
        bool bHave = false;
        int colorid[4] = {0,1,2,3};
        std::shuffle(&colorid[0], &colorid[4], STD::Generator::instance().get_mt());
        uint8_t valuaid[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
        std::shuffle(&valuaid[0], &valuaid[13], STD::Generator::instance().get_mt());
        uint8_t num = 0;
        uint8_t num1 = 0;
        uint8_t num2 = 0;
        for (int i = 0; i < 13; ++i)
        {
            if (TmpcardIndex[valuaid[i]]>=3)
            {
                num = valuaid[i];
                TmpcardIndex[valuaid[i]] -= 3;
                break;
            }
        }
        for (int i = 0; i < 13; ++i)
        {
            if (TmpcardIndex[valuaid[i]]>=2)
            {
                num1 = valuaid[i];
                TmpcardIndex[valuaid[i]] -= 2;
                bHave = true;
                break;
            }
        }
        if (bHave)
        {
            uint8_t count =0;
            for (int i = 0; i < 4; ++i)
            {
                if (TmpcolorIndex[colorid[i]][num]>0 && count<3)
                {
                    carddata[count++] = num + 16*colorid[i] + 0x01;
                    TmpcolorIndex[colorid[i]][num] = 0;
                }
            }
            for (int i = 0; i < 4; ++i)
            {
                if (TmpcolorIndex[colorid[i]][num1]>0 && count<5)
                {
                    carddata[count++] = num1 + 16*colorid[i] + 0x01;
                    TmpcolorIndex[colorid[i]][num1] = 0;
                }
            }
        }
        
        if (cardcout==6)
        {
            do
            {
                num2 = rand()%0x0d + 1;
            } while( num == num2 || num1 == num2);
            bool bHaveSame = false;
            do
            {
                bHaveSame = false;
                m_randCount++;
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = num2 + (16 * rand()%4);
                    }
                }
                else
                {
                    carddata[5] = num2 + (16 * rand()%4); 
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2);    
            if (m_randCount>=CHOOSE_COUNT/2)
            {
                bool bChoose = false;
                for (int i = 0; i < 4; ++i)
                {
                    for (int j = 0; j < 13; ++j)
                    {
                        if (TmpcolorIndex[i][j]>0 && TmpcardIndex[j]>0)
                        {               
                            num2 = valuaid[j];         
                            carddata[5] = num2 + 16*i + 0x01;
                            TmpcolorIndex[i][j] = 0;
                            TmpcardIndex[j] -= 1;
                            bChoose = true;
                            break;
                        }
                    } 
                    if (bChoose) break;              
                }
            }       
        }
    } while (isHaveSameCard(carddata,cardcout) && m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}
bool CTestCase::randTongHuaNiu(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        memcpy(TmpcardIndex,m_cardIndex,sizeof(TmpcardIndex));
        memcpy(TmpcolorIndex,m_colorIndex,sizeof(TmpcolorIndex));
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }        
        m_randCount++;
        uint8_t color = rand()%4; 
        uint8_t startPos = 0;
        uint8_t endPos = 0;
        uint8_t lengPos = 0;
        bool bHave = false;
        int colorid[4] = {0,1,2,3};
        std::shuffle(&colorid[0], &colorid[4], STD::Generator::instance().get_mt());
        uint8_t valuaid[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
        std::shuffle(&valuaid[0], &valuaid[13], STD::Generator::instance().get_mt());
        for (uint8_t i = 0; i < 4; ++i) //花色
        {
            lengPos = 0;
            for (uint8_t j = 0; j < 13; ++j) //牌值
            {
                if (TmpcolorIndex[colorid[i]][valuaid[j]]>0)
                {
                    lengPos++;             
                }
            }                               
            if (lengPos>=5) //有同花
            {
                bHave = true;
                color = colorid[i]; 
                uint8_t count = 0;
                for (uint8_t j = 0; j < 13; ++j) //牌值
                {
                    if (TmpcolorIndex[colorid[i]][valuaid[j]]>0 && count<5)
                    {
                        carddata[count++] = valuaid[j] + 16*colorid[i] + 0x01;  
                        if (count>=5)
                        {                            
                            break;
                        }                  
                    }
                    
                }
            }
            if (bHave) break;
        }        
        
        bool bHaveSame = false;
        if (cardcout==6)
        {
            do
            {
                bHaveSame = false;
                m_randCount++;
                int otherclore = rand()%4; 
                while(otherclore==color)
                {
                    otherclore = rand()%4;
                }           
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + (16 * otherclore);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + (16 * otherclore);
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2);       
        }
    } while (isHaveSameCard(carddata,cardcout) && m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}
bool CTestCase::randWuHuaNiu(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        m_randCount++;
        uint8_t startPos = 0;
        uint8_t endPos = 0;
        uint8_t lengPos = 0;
        bool bHave = false;
        int index[4] = {0,1,2,3};
        std::shuffle(&index[0], &index[4], STD::Generator::instance().get_mt());
        std::vector<uint8_t> CardsV;
        uint8_t data = 0;
        uint8_t rangPos = 0;
        for (uint8_t i = 0; i < 4; ++i)
        {
            for (uint8_t j = 9; j < 13; ++j) //牌值
            {
                if (m_colorIndex[index[i]][j]>0)
                {
                    lengPos++; 
                    data = j + 16*index[i] + 0x01;
                    rangPos = rand()%lengPos;
                    if (rand()%100<30)
                    {
                        CardsV.push_back(data);
                    }
                    else
                    {
                        CardsV.insert(CardsV.begin()+rangPos,data);
                    }
                }
            }
        }        
        if (lengPos>=5) //有五花牛
        {
            bHave = true; 
            uint8_t color[4] ={0};  
            uint8_t colorIndex = 0;
            uint8_t count = 0;
            for (auto it : CardsV)
            {
                carddata[count++] = it;
                if (count>=5)
                {
                    break;
                }
            }        
        }
        else
        {
            return bChooseSuccess;
        }
        
        if (cardcout==6)
        {
            bool bHaveSame = false;
            do
            {
                bHaveSame = false;
                m_randCount++;
                int otherclore = rand()%4;            
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + (16 * otherclore);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + (16 * otherclore);
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2);      
        }
    } while (isHaveSameCard(carddata,cardcout) && m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}
bool CTestCase::randShunziNiu(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        memcpy(TmpcardIndex,m_cardIndex,sizeof(TmpcardIndex));
        memcpy(TmpcolorIndex,m_colorIndex,sizeof(TmpcolorIndex));
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        m_randCount++;
        uint8_t startPos = 0;
        uint8_t endPos = 0;
        uint8_t lengPos = 0;
        bool bHave = false;
        int index[4] = {0,1,2,3};
        std::shuffle(&index[0], &index[4], STD::Generator::instance().get_mt());
        uint8_t colorCount = 0;
        for (uint8_t j = 0; j < 9; ++j) //牌值
        {
            colorCount = TmpcolorIndex[0][j]+TmpcolorIndex[1][j]+TmpcolorIndex[2][j]+TmpcolorIndex[3][j];
            if (TmpcardIndex[j]>0 && colorCount>0)
            {
                lengPos = 0;
                startPos = j;
                for (uint8_t k = startPos; k < 13; ++k)
                {
                    colorCount = TmpcolorIndex[0][k]+TmpcolorIndex[1][k]+TmpcolorIndex[2][k]+TmpcolorIndex[3][k];
                    if (TmpcardIndex[k]>0 && colorCount>0)
                        lengPos++;
                    else
                        break;
                }
                if (lengPos>=5) //有顺子
                {
                    bHave = true;
                    uint8_t count = 0;
                    uint8_t startIndex = m_random.betweenInt(0,lengPos-5).randInt_mt(true) + startPos;
                    uint8_t samecolorCount = 0;  
                    uint8_t color[4] ={0};  
                    uint8_t colorIndex = 0;               
                    for (uint8_t m = startIndex; m < startIndex+5; ++m)
                    {
                        samecolorCount = 0;
                        for (int i = 0; i < 4; ++i)
                        {
                            if (TmpcolorIndex[i][m]>0)
                            {
                                color[samecolorCount++] = i;
                            }
                        }
                        if (samecolorCount>0 && TmpcardIndex[m]>0)
                        {
                            colorIndex = color[m_random.betweenInt(0,samecolorCount-1).randInt_mt(true)];
                            carddata[count++] = m + 16*colorIndex + 0x01;
                            TmpcardIndex[m]--;
                            TmpcolorIndex[colorIndex][m] = 0;
                        } 
                        else
                        {
                            bHave=false;
                        }                       
                    }
                }
                if (bHave) break;
            }
        }
        
        if (cardcout==6 && bHave)
        {
            bool bHaveSame = false;
            do
            {
                bHaveSame = false;
                m_randCount++;
                int otherclore = rand()%4;
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = rand()%10<5 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + (16 * otherclore);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + (16 * otherclore); 
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2);      
        }
    } while (isHaveSameCard(carddata,cardcout)&&m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}
bool CTestCase::randBomb(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    uint8_t TmpcardIndex[13];         //各牌值剩余个数
    uint8_t TmpcolorIndex[4][13];     //各花色牌值剩余个数
    memset(TmpcardIndex, 0, sizeof(TmpcardIndex));
    memset(TmpcolorIndex, 0, sizeof(TmpcolorIndex));
    m_randCount = 0;
    do
    {
        memcpy(TmpcardIndex,m_cardIndex,sizeof(TmpcardIndex));
        memcpy(TmpcolorIndex,m_colorIndex,sizeof(TmpcolorIndex));
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        m_randCount++;
        uint8_t lengPos = 0;
        bool bHave = false;
        int colorid[4] = {0,1,2,3};
        std::shuffle(&colorid[0], &colorid[4], STD::Generator::instance().get_mt());
        uint8_t valuaid[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
        std::shuffle(&valuaid[0], &valuaid[13], STD::Generator::instance().get_mt());
        uint8_t num = 0;
        uint8_t num1 = 0;
        uint8_t num2 = 0;
        bool bChoose = false;
        for (uint8_t i = 0; i < 13; ++i)
        {
            if (TmpcardIndex[valuaid[i]]==4)
            {
                num = valuaid[i];
                TmpcardIndex[valuaid[i]] = 0;
                bHave = true;
                break;
            }
        }
        uint8_t count =0;
        if (bHave)
        {
            count =0;
            bChoose = false;
            for (int i = 0; i < 4; ++i)
            {
                if (TmpcolorIndex[colorid[i]][num]>0 && count<4)
                {
                    carddata[count++] = num + 16*colorid[i] + 0x01;
                    TmpcolorIndex[colorid[i]][num] = 0;
                }
            }
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 13; ++j)
                {
                    if (TmpcardIndex[j]<=0 || valuaid[j]==num) continue;
                    if (TmpcolorIndex[colorid[i]][valuaid[j]]>0 && count<5)
                    {
                        num1 = valuaid[j];
                        carddata[count++] = num1 + 16*colorid[i] + 0x01;
                        TmpcolorIndex[colorid[i]][num1] = 0;
                        bChoose = true;
                        if (count>=5) break;
                    }
                }  
                if (bChoose)
                {
                    break;
                }              
            }
        }
        else
        {
            return bChooseSuccess;
        }
        if (cardcout==6)
        {
            bChoose = false;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 13; ++j)
                {
                    if (TmpcardIndex[j]<=0 || valuaid[j]==num || valuaid[j]==num1) continue;
                    if (TmpcolorIndex[colorid[i]][valuaid[j]]>0 && count<6)
                    {               
                        num2 = valuaid[j];         
                        carddata[count++] = num2 + 16*colorid[i] + 0x01;
                        TmpcolorIndex[colorid[i]][num2] = 0;
                        bChoose = true;
                        break;
                    }
                } 
                if (bChoose) break;              
            }
            if (!bChoose)
            {
                bool bHaveSame = false;
                do
                {
                    bHaveSame = false;
                    if (m_bhaveKing)
                    {
                        int randn = rand()%100;
                        if (randn<5)
                        {
                            carddata[5] = randn<6 ? 0x4e : 0x4f;
                        }
                        else 
                        {
                            carddata[5] = num2 + 16 * (rand()%4);
                        }
                    }
                    else
                    {
                        carddata[5] = num2 + 16 * (rand()%4);
                    }
                    for (int i = 0; i < 5; ++i)
                    {
                        if (carddata[5]==carddata[i])
                        {
                            bHaveSame = true;
                            break;
                        }
                    }
                }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2); 
            }
        }
    } while (isHaveSameCard(carddata,cardcout)&&m_randCount<CHOOSE_COUNT);  
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;  
}

bool CTestCase::randNN(uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    m_randCount = 0;
    do
    {
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        do
        {
            m_randCount++;
            int randCount = 0;
            bool bSame = false;
            uint8_t num0 = m_random.betweenInt(0,12).randInt_mt(true) + 0x01;
            uint8_t num1 = m_random.betweenInt(0,12).randInt_mt(true) + 0x01;
            uint8_t num2 = 0;
            while(num1==num0)
            {
                num1 = m_random.betweenInt(0,12).randInt_mt(true) + 0x01;
            }
            do
            {
                randCount++;
                num2 = genCard(T_MUL - (getCardLogicValue(num0)+getCardLogicValue(num1))%T_MUL);
            } while ((num2==num0 || num2==num1)&&randCount<100);
            if (num2==0)
            {
                randCount=0;
                do
                {
                    randCount++;
                    num2 = m_random.betweenInt(0,3).randInt_mt(true) + 0x0a;
                }while((num2==num0 || num2==num1)&&randCount<100);
            }

            //2
            uint8_t num3 = m_random.betweenInt(0,8).randInt_mt(true) + 0x01;
            randCount=0;
            do
            {
                randCount++;
                num3 = m_random.betweenInt(0,8).randInt_mt(true) + 0x01;
            } while ((num3==num0 || num3==num1 || num3==num2)&&randCount<100);
            uint8_t num4 = T_MUL - num3;
            
            num3 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));            
            num0 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
            num1 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
            
            if (GetCardValue(num4)==GetCardValue(num0) || GetCardValue(num4)==GetCardValue(num1) || GetCardValue(num4)==GetCardValue(num2))
            {
                randCount=0;
                do
                {
                    randCount++;
                    num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
                }while((GetCardColor(num4)==GetCardColor(num0) || GetCardColor(num4)==GetCardColor(num1) || GetCardColor(num4)==GetCardColor(num2))&&randCount<100);
            }
            else
            {
                num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
            }
            carddata[0] = num0;
            carddata[1] = num1;
            carddata[2] = num2;
            carddata[3] = num3;
            carddata[4] = num4;
        }while (isHaveSameCard(carddata,5)&&m_randCount<CHOOSE_COUNT);
        m_randCount=0;
        if (cardcout==6)
        {
            bool bHaveSame = false;
            do
            {
                bHaveSame = false;
                m_randCount++;
                int otherclore = rand()%4;
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = randn<6 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + 16 * (rand()%3 +1);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + 16 * (rand()%3 +1);
                } 
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2); 
        }
    } while (isHaveSameCard(carddata,cardcout)&&m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}

bool CTestCase::randNNum(uint8_t num, uint8_t carddata[],uint8_t cardcout)
{
    bool bChooseSuccess = false;
    m_randCount = 0;
    do
    {
        m_randCount++;
        if(num >= 10)
        {
            LOG(ERROR) << "Error number:" << num;
            return false;
        }
        for (int i = 0; i < cardcout; ++i)
        {
            carddata[i] = 0;
        }
        uint8_t num0,num1,num2,num3,num4;
        uint8_t randCount = 0;
        if(num != 0)
        {
            //cannot insure 3+2 have no special type
            num0 = rand()%0x0d + 1;
            num1 = rand()%0x0d + 1;
            while(num1==num0)
            {
                num1 = rand()%0x0d + 1;
            }
            do
            {
                randCount++;
                num2 = genCard(T_MUL - (getCardLogicValue(num0)+getCardLogicValue(num1))%T_MUL);
            } while ((num2==num0 || num2==num1)&&randCount<100);
            if (num2==0)
            {
                randCount=0;
                do
                {
                    randCount++;
                    num2 = rand()%0x04 + 10;
                }while((num2==num0 || num2==num1)&&randCount<100);
            }
            randCount=0;
            do
            {
                randCount++;
                num3 = rand()%0x09 + 1;
            } while ((num3==num0 || num3==num1 || num3==num2)&&randCount<100);

            if(num < num3)
            {
                num4 = num + T_MUL - num3;
            }else
            {
                num4 = num - num3;
            }            
            num0 += 16 * (m_random.betweenInt(0,3).randInt_mt(true));
            num1 += 16 * (m_random.betweenInt(0,3).randInt_mt(true));
            num3 += 16 * (m_random.betweenInt(0,3).randInt_mt(true));
            // num4 += 16 * (m_random.betweenInt(0,3).randInt_mt(true));
            if (num4==0)
            {
                randCount=0;
                do
                {
                    randCount++;
                    num4 = rand()%0x04 + 10;
                }while((num4==num0 || num4==num1 || num4==num2 || num4==num3)&&randCount<100);
                if (GetCardValue(num4)==GetCardValue(num0) || GetCardValue(num4)==GetCardValue(num1) || GetCardValue(num4)==GetCardValue(num2))
                {
                    randCount=0;
                    do
                    {
                        randCount++;
                        num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
                    }while((GetCardColor(num4)==GetCardColor(num0) || GetCardColor(num4)==GetCardColor(num1) || GetCardColor(num4)==GetCardColor(num2))&&randCount<100);
                }
                else
                {
                    num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
                }
            }
            else 
            {
                randCount=0;
                if (GetCardValue(num4)==GetCardValue(num0) || GetCardValue(num4)==GetCardValue(num1) || GetCardValue(num4)==GetCardValue(num2))
                {
                    randCount=0;
                    do
                    {
                        randCount++;
                        num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
                    }while((GetCardColor(num4)==GetCardColor(num0) || GetCardColor(num4)==GetCardColor(num1) || GetCardColor(num4)==GetCardColor(num2))&&randCount<100);
                }
                else
                {
                    num4 += (16 * (m_random.betweenInt(0,3).randInt_mt(true)));
                }
            }
        }else
        {
            uint8_t t0,t1,t2,t3,t4;

            map<uint8_t,bool> excepts;
            num0 = rand()%0x0d+1;
            t0 = getCardLogicValue(num0);
            num1 = rand()%0x0d+1;
            t1 = getCardLogicValue(num1);
            excepts.insert(std::make_pair( T_MUL - (t0+t1)%T_MUL,true));

            num2 = rand()%0x0d+1;
            t2 = getCardLogicValue(num2);
            while(excepts.find(t2) != excepts.end())
            {
                num2 = rand()%0x0d+1;
                t2 = getCardLogicValue(num2);
            }

            excepts.insert(std::make_pair(T_MUL - (t0+t2)%T_MUL, true));
            excepts.insert(std::make_pair(T_MUL - (t1+t2)%T_MUL, true));

            num3 = rand()%0x0d+1;
            t3 = getCardLogicValue(num3);
            while(excepts.find(t3) != excepts.end())
            {
                num3 = rand()%0x0d+1;
                t3 = getCardLogicValue(num3);
            }

            excepts.insert(std::make_pair(T_MUL - (t0+t3)%T_MUL, true));
            excepts.insert(std::make_pair(T_MUL - (t1+t3)%T_MUL, true));
            excepts.insert(std::make_pair(T_MUL - (t2+t3)%T_MUL, true));

            num4 = rand()%0x0d+1;
            t4 = getCardLogicValue(num4);
            while(excepts.find(t4) != excepts.end())
            {
                num4 = rand()%0x0d+1;
                t4 = getCardLogicValue(num4);
            }

            num0 += 16 * (rand()%3 +1);
            num1 += 16 * (rand()%3 +1);
            num2 += 16 * (rand()%3 +1);
            num3 += 16 * (rand()%3 +1);
            num4 += 16 * (rand()%3 +1);
        }
        carddata[0] = num0;
        carddata[1] = num1;
        carddata[2] = num2;
        carddata[3] = num3;
        carddata[4] = num4;
        if (cardcout==6)
        {             
            bool bHaveSame = false;
            m_randCount=0;
            do
            {
                bHaveSame = false;
                m_randCount++;
                int otherclore = rand()%4;
                if (m_bhaveKing)
                {
                    int randn = rand()%100;
                    if (randn<5)
                    {
                        carddata[5] = randn<6 ? 0x4e : 0x4f;
                    }
                    else 
                    {
                        carddata[5] = rand()%0x0d + 1 + 16 * (rand()%3 +1);
                    }
                }
                else
                {
                    carddata[5] = rand()%0x0d + 1 + 16 * (rand()%3 +1);
                }
                for (int i = 0; i < 5; ++i)
                {
                    if (carddata[5]==carddata[i])
                    {
                        bHaveSame = true;
                        break;
                    }
                }
            }while (bHaveSame&&m_randCount<CHOOSE_COUNT/2);

        }
    } while (isHaveSameCard(carddata,cardcout)&&m_randCount<CHOOSE_COUNT);
    if (m_randCount<CHOOSE_COUNT)
    {
        bChooseSuccess = true;
    }
    return bChooseSuccess;
}

uint8_t CTestCase::genCard(uint8_t carddata)
{
    carddata = carddata%16;
    if(carddata == 0)
    {
        LOG(ERROR) << "Illegal carddata:" << carddata;
        return 0;
    }
    if(carddata != 10)
    {
        return carddata + 16 * (rand()%3 +1);
    }else
    {
        return 10 + rand()%4 + 16 * (rand()%3 +1);
    }
}

uint8_t CTestCase::getCardLogicValue(uint8_t carddata)
{
    uint8_t rValue = carddata%16;
    return rValue < 10 ? rValue : 10;
}
//检验手牌牌值是否重复
bool CTestCase::CheckHanCardIsOK(uint8_t cbHandCardData[], uint8_t cardcount)
{
	for (int i = 0; i < cardcount; ++i)
	{
		for (int j = i+1; j < cardcount; j++)
		{
			if (cbHandCardData[i] == cbHandCardData[j])
			{
				return false;
			}
		}		
	}

	return true;
}
