#ifndef CMD_BBQZNN_HEAD_FILE
#define CMD_BBQZNN_HEAD_FILE

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


#define GAME_NAME                               "百变抢庄牛牛"                           //游戏名字
#define KIND_ID                                 880									//游戏 I D

#define GAME_PLAYER                             4									//游戏人数
#define MIN_GAME_PLAYER							2									//最小游戏人数
#define MAXCOUNT                                5									//扑克数目

//不同的玩法类型
enum PlayGameType
{
	PlayGameType_None,
	PlayGameType_Normal,	//1 经典厅-5倍     【roomid:8801-8809】
	PlayGameType_Huangjin,	//2 黄金厅-六张5倍	【roomid:8811-8819】
	PlayGameType_Jinyu,		//3 金玉厅-六张3倍	【roomid:8821-8829】
	PlayGameType_Huangjia,	//4 皇家厅-百变	【roomid:8831-8839】
	PlayGameType_HaoHua,	//5 豪华厅-3倍		【roomid:8841-8849】
	PlayGameType_Chuanqi,	//6 传奇厅-六张百变【roomid:8851-8859】
	PlayGameType_Fengsao,	//7 风骚厅-六张10倍【roomid:8861-8869】
	PlayGameType_Rongyao,	//8 荣耀厅-10倍    【roomid:8871-8879】
};

// call math round.
#define Double_Round(x,y,z)                     round(x)


//#define GAME_PLAYER								5								//游戏人数

//数值定义
#define MAX_COUNT_SIX							6								//最大手牌张数6选5
#define MAX_COUNT								5								//最大手牌张数
#define MAX_JETTON_MUL							5								//下注倍数个数
#define MAX_BANKER_MUL							3								//叫庄倍数个数
#define MAX_BANKER_CALL							4+1								//叫庄最大倍数

//////////////////////////////////////////////////////////////////////////
//游戏状态
#define GAME_STATUS_INIT                        0                              //INIT status
#define GAME_STATUS_READY						99								//空闲状态
#define GAME_STATUS_LOCK_PLAYING                100                             //Playing
#define GAME_STATUS_CALL						102								//叫庄状态
#define GAME_STATUS_SCORE						103								//下注状态
#define GAME_STATUS_OPEN						104								//开牌状态
//#define GAME_STATUS_END                         105								//游戏结束

#endif
