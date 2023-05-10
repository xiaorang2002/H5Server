#ifndef		___GAME_DEFINE_H_DEF___
#define		___GAME_DEFINE_H_DEF___

#include "public/Globals.h"
#include "public/gameDefine.h"
#include <string.h>

#pragma pack(1)

#define PLAYER_NUM		(2)     //游戏人数
#define MATCH_TIME      (5)     //匹配时长
#define GAME_INVALID_CHAIR  (-1)    //无效座位
//结束原因
#define GER_NO_PLAYER				(1)	   //没有玩家
#define GER_NORMAL_HU               (2)    //正常胡
#define GER_NO_MONEY                (3)    //金钱不足
#define GER_NORMAL_NO_HU            (4)    //流局
//下一个座位
#define NEXT_CHAIR(chair) (((chair)+1) % PLAYER_NUM)
#define WARNLOG(str) LOG_WARN  << __FUNCTION__ <<" " << (str)
#define ERRLOG(str) LOG_ERROR  << __FUNCTION__ <<" " << (str)

#define INI_FILENAME   "./conf/erqs/Erqs_config.ini"
#define TEST_INI_FILENAME   "./conf/erqs/Erqs_test.ini"
#define ROBOT_INI_FILENAME   "./conf/erqs/Erqs_robot.ini"

enum ControlType
{
	Ctrl_None,
	Ctrl_Normal,	//正常
	Ctrl_SystemWin,	//系统赢
	Ctrl_SystemLose	//系统输
};


#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif

#endif		//___GAME_DEFINE_H_DEF___

