#ifndef QZNN_TESTCASE_H
#define QZNN_TESTCASE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>

#define ENABLE_TEST  false

#define CASE_FILE       "./conf/qznn-case.conf"

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
    void Check(string account, uint8_t carddata[]);
    void randBomb(uint8_t carddata[]);
    void randNN(uint8_t carddata[]);
    void randNNum(uint8_t num,uint8_t carddata[]);
    uint8_t getCardLogicValue(uint8_t carddata);
    uint8_t genCard(uint8_t carddata);

public:
    const uint8_t T_MUL = 10;
};

#endif // QZNN_TESTCASE_H
