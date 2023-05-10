#pragma once
#ifndef BJ_ERRORCODE_H
#define BJ_ERRORCODE_H

#define     ERR_CODE_DEFAULT            0//默认的错误码,未知错误
#define     ERR_CODE_SCORE_SEAT         1//下注位置错误
#define     ERR_CODE_LACK_MONEY         2//余额不足
#define     ERR_CODE_SEAT_CANT_BET      3//此位置不能下注
#define     ERR_CODE_BETSELF            4//先给自己的位置下注
#define     ERR_CODE_PARAM              5//参数错误
#define     ERR_CODE_BETED              6//重复下注
#define     ERR_CODE_INSURE_SEAT        7//下注位置错误
#define     ERR_CODE_INSURED            8//重复保险
#define     ERR_CODE_WRONG_OPER         9//操作错误
#define     ERR_CODE_P_STATUS           10//玩家状态错误
#define     ERR_CODE_DATA               11//数据错误
#define     ERR_CODE_G_STATUS           12//游戏状态错误
#define     ERR_CODE_TURN               13//不是当前操作玩家
#define     ERR_CODE_OPER               14//操作错误
#define     ERR_CODE_NO_BETS            15//当前还没有下注
#define     ERR_CODE_ALREADY_END_BET    16//已经停止下注了
#define     ERR_CODE_INSRE_LACK_MONEY   17//余额不足
#define     ERR_CODE_OVER_BET           18//
#endif // ERRORCODE_H
