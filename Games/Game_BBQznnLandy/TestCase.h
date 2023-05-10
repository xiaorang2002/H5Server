#ifndef BBQZNN_TESTCASE_H
#define BBQZNN_TESTCASE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include "public/StdRandom.h"

#define ENABLE_TEST  true
#define ENABLE_KEY "enabled"
#define ANDROID_KEY "enabledandroid"

#define CASE_FILE       "./conf/bbqznn_case.conf"
#define CASE_FILE_SIX   "./conf/bbqznn_casesix.conf"

using namespace std;
class CTestCase
{
public:
    CTestCase();
    ~CTestCase();

private:
    std::map<std::string, std::vector<uint8_t>> cases;
    static const uint8_t   m_byCardDataArray[52];  //扑克数据
private:
    void loadConf(uint8_t cardCount);

public:
	void CheckAdr(string account, uint8_t carddata[], uint8_t cardcout);
    bool Check(string account, uint8_t carddata[],uint8_t cardcout,uint8_t cardType=10);
    bool randTongHuaShun(uint8_t carddata[],uint8_t cardcout);
    bool randFiveSmallNiu(uint8_t carddata[],uint8_t cardcout);
    bool randBomb(uint8_t carddata[],uint8_t cardcout);
    bool randHuluNiu(uint8_t carddata[],uint8_t cardcout);
    bool randTongHuaNiu(uint8_t carddata[],uint8_t cardcout);
    bool randWuHuaNiu(uint8_t carddata[],uint8_t cardcout);
    bool randShunziNiu(uint8_t carddata[],uint8_t cardcout);
    bool randNN(uint8_t carddata[],uint8_t cardcout);
    bool randNNum(uint8_t num,uint8_t carddata[],uint8_t cardcout);
    uint8_t getCardLogicValue(uint8_t carddata);
    uint8_t genCard(uint8_t carddata);
    void setCardCount(uint8_t count);
    void setIsHaveking(bool bHave) {m_bhaveKing=bHave;};
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&0x0F; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&0xF0)>>4; }
    void clearHaveCard();
    bool isHaveSameCard(uint8_t cbCardData[],uint8_t cardcount);
    void addHaveChooseCard(uint8_t cbCardData[],uint8_t cardcount);
    void resetLestCard();
	//检验手牌牌值是否重复
	bool CheckHanCardIsOK(uint8_t cbHandCardData[], uint8_t cardcount);
public:
    const uint8_t T_MUL = 10;
    const uint8_t CHOOSE_COUNT = 200;
    uint8_t m_cardCount;
    bool m_bhaveKing;
    std::vector<uint8_t> haveUseCard;
    STD::Random m_random;
    uint8_t m_randCount;
    std::map<uint8_t, std::vector<uint8_t>> lestCard; //剩余未选到的牌
    uint8_t m_cardIndex[13];         //各牌值剩余个数
    uint8_t m_colorIndex[4][13];     //各花色牌值剩余个数

	bool enabled = false;
	bool android = false;
};

#endif // BBQZNN_TESTCASE_H
