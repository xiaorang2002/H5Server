#ifndef CMD_QZNN_HEAD_FILE
#define CMD_QZNN_HEAD_FILE

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "public/Globals.h"
#include "public/gameDefine.h"


//////////////////////////////////////////////////////////////////////////////////
//服务定义

////游戏属性
#define GAME_NAME                               "抢庄牛牛"                           //游戏名字
#define KIND_ID                                 890									//游戏 I D

#define GAME_PLAYER                             4									//游戏人数
#define MIN_GAME_PLAYER							2									//最小游戏人数
#define MAXCOUNT                                5									//扑克数目


// call math round.
#define Double_Round(x,y,z)                     round(x)


//#define GAME_PLAYER								5								//游戏人数

//数值定义
#define MAX_COUNT								5								//最大手牌张数
#define SHOW_COUNT                              4
#define MAX_JETTON_MUL							4								//下注倍数个数
#define MAX_BANKER_MUL							3								//叫庄倍数个数
#define MAX_BANKER_CALL							4+1								//叫庄最大倍数

//////////////////////////////////////////////////////////////////////////
//游戏状态
#define GAME_STATUS_INIT                        0                              //INIT status
#define GAME_STATUS_READY						99								//空闲状态
#define GAME_STATUS_CALL						102								//叫庄状态
#define GAME_STATUS_SCORE						103								//下注状态
#define GAME_STATUS_OPEN						104								//开牌状态

#endif
