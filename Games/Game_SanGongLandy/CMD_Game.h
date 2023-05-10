#ifndef CMD_HK_FIVE_CARD_HEAD_FILE
#define CMD_HK_FIVE_CARD_HEAD_FILE

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "public/Globals.h"
#include "public/gameDefine.h"


//////////////////////////////////////////////////////////////////////////////////
//服务定义

////游戏属性
//#define GS_TK_FREE                              1                                   //等待开始
//#define GS_TK_CALL                              GAME_STATUS_START                   //叫庄状态
//#define GS_TK_CALLEND                           GAME_STATUS_START+1                 //叫庄状态
//#define GS_TK_SCORE                             GAME_STATUS_START+2                 //下注状态
//#define GS_TK_PLAYING                           GAME_STATUS_START+3				    //游戏进行


#define GAME_NAME                               "三公"                               //游戏名字
#define KIND_ID                                 860									//游戏 I D

#define GAME_PLAYER                             5									//游戏人数
#define MIN_GAME_PLAYER							2									//最小游戏人数
#define GAME_JETTON_COUNT                       5                                   //最多下注个数


// call math round.
#define Double_Round(x,y,z)                     round(x)


//#define GAME_PLAYER								5								//游戏人数

//数值定义
#define MAX_COUNT								3								//最大手牌张数

#define MAX_USER_SASIC                          2                               //闲家的基数
#define MAX_JETTON_MAX                          35                              //最大下注倍数为35
#define MAX_JETTON_BASE                         7                               //最大下注基数为7

//////////////////////////////////////////////////////////////////////////
//游戏状态
#define SG_GAME_STATUS_DEFAULT                  0
#define SG_GAME_STATUS_INIT                     98                              //INIT status
#define SG_GAME_STATUS_READY					99								//空闲状态
#define SG_GAME_STATUS_LOCK_PLAYING             100                             //Playing
#define SG_GAME_STATUS_CALL                     102								//叫庄状态
#define SG_GAME_STATUS_SCORE					103								//下注状态
#define SG_GAME_STATUS_OPEN                     104								//开牌状态
#define SG_GAME_STATUS_END                      105								//游戏结束

#endif
