#include "TestCase.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include "GameLogic.h"
#include "MSG_HJK.h"

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
    curCaseIdx = -1;
    if(boost::filesystem::exists(CASE_FILE))
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(CASE_FILE,pt);
        boost::property_tree::ptree::iterator it0;
        boost::property_tree::ptree element;

        int idx = 0;
        for(boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it)
        {
            string key = it->first;
//            LOG(ERROR) << key;
            if(key == "enablecase")
            {
                curCaseIdx = it->second.get_value<int32_t>();
//                LOG(ERROR) << "curCaseIdx:" << curCaseIdx;
            }else
            {
//                element = pt.get_child(it->first);

//                for(it0 = element.begin();it0 != element.end(); it0++)
//                {
//                    string str = it0->second.get_value<string>();
//                    LOG(ERROR) << strtol(str.c_str(),nullptr,16);
//                    idx++;
//                }
            }


//            cases.insert(pair<string,vector<uint8_t>>(it->first, cards));
        }
    }
}


int32_t CTestCase::InitCheck(uint8_t carddata[GAME_PLAYER*2][MAX_COUNT])
{
    if(curCaseIdx == -1)
    {
        return -1;
    }

    switch (curCaseIdx) {
    case 0:
        return CaseInsure(carddata);
        break;
    default:
        return -1;
        break;
    }
}

uint8_t CTestCase::CaseInsure(uint8_t carddata[GAME_PLAYER*2][MAX_COUNT])
{
    uint8_t card = genCard(1);
    carddata[5][0] = card;
    return 5;
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
