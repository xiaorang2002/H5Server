#ifndef KPQZNN_TESTCASE_H
#define KPQZNN_TESTCASE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>

#define ENABLE_KEY "enabled"
#define CASE_FILE       "./conf/kpqznn-case.conf"
#define ANDROID_KEY "enabledandroid"

using namespace std;
class CTestCase
{
public:
    CTestCase();
    ~CTestCase();

private:
    std::map<std::string, std::vector<uint8_t>> cases;

private:
    void loadConf(void);

public:
    void CheckAdr(string account, uint8_t carddata[]);
    void Check(string account, uint8_t carddata[]);
    void randBomb(uint8_t carddata[]);
    void randNN(uint8_t carddata[]);
    void randNNum(uint8_t num,uint8_t carddata[]);
    uint8_t getCardLogicValue(uint8_t carddata);
    uint8_t genCard(uint8_t carddata);

public:
    const uint8_t T_MUL = 10;
    bool enabled = false;
    bool android = false;
};

#endif // KPQZNN_TESTCASE_H
