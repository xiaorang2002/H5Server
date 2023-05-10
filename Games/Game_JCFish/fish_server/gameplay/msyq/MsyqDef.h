#pragma once

#include "../../helps/GameElementManager.h"

#define MAX_MSYQ_STANDBY 5

typedef struct msyqRenderer
{
	GameAnimation* animStandbyStatsImmovable[MAX_MSYQ_STANDBY]; // 待机动作：静止（休息、扭头、蹬腿）
	GameAnimation* animStandbyStatsMovement[MAX_MSYQ_STANDBY]; // 待机动作：移动（走，跑）
	GameAnimation* animRunningStatsKick; // 带宝箱的马踢腿动作
	GameAnimation* animRunningStatsMovement; // 带宝箱的马奔跑的动作
	GameAnimation* animRunningStatsStrike; // 马被击中时叠加的动作（撒元宝）
	GameAnimation* animRunningStatsPrompt; // 玩法提示动画
	GameSound* soundKick; // 马踢腿时的音效
	GameSound* soundStrike; // 马被击中时的音效
	GameSound* soundCheer; // 任务完成后的欢呼声
}MsyqRenderer;

typedef enum actionMsyqStandbyState
{
	msyqssNone,
	msyqssMove,
	msyqssStandby,
	msyqssFadeIn,
	msyqssFadeOut,
}ActionMsyqStandbyState;

class IMsyqRunBackEvent
{
public:
	virtual void onMsyqRunBackDone(LONG playerId, LONG score) = 0;
};
