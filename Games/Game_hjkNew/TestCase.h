#ifndef HJK_TESTCASE_H
#define HJK_TESTCASE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include "MSG_HJK.h"
#include "GameLogic.h"

#define CASE_FILE       "./conf/hjk-case.json"
#define ENABLED_TEST     true


using namespace std;
class CTestCase
{
public:
    CTestCase();
    ~CTestCase();

private:
    std::map<std::string, std::vector<uint8_t>> cases;
    int32_t curCaseIdx;

private:
    void loadConf(void);

public:
    int32_t InitCheck(uint8_t carddata[GAME_PLAYER*2][MAX_COUNT]);
    uint8_t CaseInsure(uint8_t carddata[GAME_PLAYER*2][MAX_COUNT]);
    uint8_t getCardLogicValue(uint8_t carddata);
    uint8_t genCard(uint8_t carddata);

};

#endif // HJK_TESTCASE_H
