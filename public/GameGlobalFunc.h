// Copyright (c) 2019, Landy
// All rights reserved.


#ifndef GAME_GLOBALFUNC_H
#define GAME_GLOBALFUNC_H


#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <random>

#include "Globals.h"

// #include <muduo/base/ThreadLocalSingleton.h>
// #include "RedisClient/RedisClient.h"






class GameGlobalFunc
{
public:
    GameGlobalFunc() = default;


public:

//    static bool RedisLoginInfoScoreBankScoreChange(int32_t userId, int64_t nAddscore, int64_t nAddBankScore, int64_t &newScore, int64_t &newBankScore);
////    static void RedisSetLoginInfoScore(int32_t userId, double score);

//    static bool InsertScoreChangeRecord(dword userid, int64_t lUserScore, int64_t lBankScore,
//                                                    int64_t lAddScore, int64_t lAddBankScore,
//                                                    int64_t lTargetUserScore, int64_t lTargetBankScore,
//                                                    int type, int configId = 0);

//    static bool UserRechargeScore(shared_ptr<sql::Connection> &conn, int32_t userid, int64_t lAddGem, int64_t lAddScore, int32_t *vipLevel, int64_t *allCharge);
//    static bool UserExchangeFailScore(shared_ptr<sql::Connection> &conn, int32_t userid, int64_t lAddGem, int64_t lAddScore, int64_t *allExchange);
//    static bool PromoterGetMoney(shared_ptr<sql::Connection> &conn, int32_t userId, int64_t lAddScore);

//    static bool sendCodeSMS(string strPhoneValue, string strCode);
};

#endif // GameGlobalFunc
