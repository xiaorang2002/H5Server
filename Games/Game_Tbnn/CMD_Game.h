#ifndef CMD_TBNN_HEAD_FILE
#define CMD_TBNN_HEAD_FILE

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Globals.h"
#include "gameDefine.h"


//////////////////////////////////////////////////////////////////////////////////
//服务定义



#define GAME_NAME                               "通比牛牛"                           //游戏名字
#define KIND_ID                                 870									//游戏 I D

#define GAME_PLAYER                             6									//游戏人数
#define MIN_GAME_PLAYER							2									//最小游戏人数
#define MAXCOUNT                                5									//扑克数目


// call math round.
#define Double_Round(x,y,z)                     round(x)


//#define GAME_PLAYER								5								//游戏人数

//数值定义
#define MAX_COUNT								5								//最大手牌张数
#define MAX_JETTON_MUL							5								//下注倍数个数

//////////////////////////////////////////////////////////////////////////
//游戏状态
#define GAME_STATUS_INIT                        0                              //INIT status
#define GAME_STATUS_READY						99								//空闲状态
#define GAME_STATUS_LOCK_PLAYING                100                             //Playing
#define GAME_STATUS_SCORE						103								//下注状态
#define GAME_STATUS_OPEN						104								//开牌状态
//#define GAME_STATUS_END                         105								//游戏结束

#endif
