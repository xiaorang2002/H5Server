#include "TestCase.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include <vector>

//#define BOOST_SPIRIT_THREADSAFE
using namespace std;
CTestCase::CTestCase()
{
    loadConf();
}

CTestCase::~CTestCase()
{

}

void CTestCase::loadConf()
{
    if(boost::filesystem::exists(CASE_FILE))
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(CASE_FILE,pt);
        boost::property_tree::ptree::iterator it0;
        boost::property_tree::ptree element;

        int idx = 0;
        for(boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it)
        {
            if(it->first != ENABLE_KEY&&it->first != ANDROID_KEY)
            {
                vector<uint8_t> cards(5);
                element = pt.get_child(it->first);
                idx = 0;
                for(it0 = element.begin();it0 != element.end(); it0++)
                {
                    string str = it0->second.get_value<string>();
                    cards[idx] = strtol(str.c_str(),nullptr,16);
                    idx++;
                }

                cases.insert(pair<string,vector<uint8_t>>(it->first, cards));
            }else{
                if(it->first == ENABLE_KEY)
                {
                    enabled = it->second.get_value<int>() == 1;
                }
                else if(it->first == ANDROID_KEY)
                {
                    android = it->second.get_value<int>() == 1;
                }

            }

        }
    }
}

void CTestCase::CheckAdr(string account, uint8_t carddata[])
{
    map<string, vector<uint8_t>>::iterator it = cases.find(account);
    if(it != cases.end())
    {
        vector<uint8_t> cards = it->second;
        int nums = 0;
        for(uint8_t i =0; i < 5 ; i++)
        {
            carddata[i] = cards[i];
            if( cards[i] != 0)
            {
                nums++;
            }
        }
    }
}
void CTestCase::Check(string account, uint8_t carddata[])
{
    map<string, vector<uint8_t>>::iterator it = cases.find(account);
    if(it != cases.end())
    {
        vector<uint8_t> cards = it->second;
        int nums = 0;
        for(uint8_t i =0; i < 5 ; i++)
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
            case 11:
                randBomb(carddata);
                break;
            case 10:
                randNN(carddata);
                break;
            default:
                randNNum(cards[0],carddata);
                break;
            }
        }else if(cards.size() == 5 && nums == 0)
        {
            randNNum(0,carddata);
        }
    }
}

void CTestCase::randBomb(uint8_t carddata[])
{
    uint8_t num = rand()%0x0d + 1;
    uint8_t num1 = rand()%0x0d + 1;
    while( num == num1)
    {
        num1 = rand()%0x0d + 1;
    }
    carddata[0] = num;
    carddata[1] = num + 16;
    carddata[2] = num + 32;
    carddata[3] = num + 48;
    carddata[4] = num1 + (16 * (rand()%3 +1));
}

void CTestCase::randNN(uint8_t carddata[])
{
    //10 j q
    uint8_t num0 = 0x0a + (16 * (rand()%3 +1));
    uint8_t num1 = 0x0b + (16 * (rand()%3 +1));
    uint8_t num2 = 0x0c + (16 * (rand()%3 +1));

    //2
    uint8_t num3 = rand()%9 + 1;
    uint8_t num4 = T_MUL - num3;
    num3 += (16 * (rand()%3 +1));
    num4 += (16 * (rand()%3 +1));

    carddata[0] = num0;
    carddata[1] = num1;
    carddata[2] = num2;
    carddata[3] = num3;
    carddata[4] = num4;
}

void CTestCase::randNNum(uint8_t num, uint8_t carddata[])
{
    if(num >= 10)
    {
        LOG(ERROR) << "Error number:" << num;
        return;
    }

    uint8_t num0,num1,num2,num3,num4;

    if(num != 0)
    {
        //cannot insure 3+2 have no special type
        num0 = rand()%0x0d + 1;
        num1 = rand()%0x0d + 1;
        num2 = genCard(T_MUL - (getCardLogicValue(num0)+getCardLogicValue(num1))%T_MUL);

        num3 = rand()%0x09 + 1;
        if(num < num3)
        {
            num4 = num + T_MUL - num3;
        }else
        {
            num4 = num - num3;
        }
        num0 += 16 * (rand()%3 +1);
        num1 += 16 * (rand()%3 +1);
        num3 += 16 * (rand()%3 +1);
        num4 += 16 * (rand()%3 +1);
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
