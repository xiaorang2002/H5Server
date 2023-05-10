#pragma once

#include "../../helps/GameElementManager.h"

#define MAX_MSYQ_STANDBY 5

typedef struct msyqRenderer
{
	GameAnimation* animStandbyStatsImmovable[MAX_MSYQ_STANDBY]; // ������������ֹ����Ϣ��Ťͷ�����ȣ�
	GameAnimation* animStandbyStatsMovement[MAX_MSYQ_STANDBY]; // �����������ƶ����ߣ��ܣ�
	GameAnimation* animRunningStatsKick; // ������������ȶ���
	GameAnimation* animRunningStatsMovement; // ����������ܵĶ���
	GameAnimation* animRunningStatsStrike; // ������ʱ���ӵĶ�������Ԫ����
	GameAnimation* animRunningStatsPrompt; // �淨��ʾ����
	GameSound* soundKick; // ������ʱ����Ч
	GameSound* soundStrike; // ������ʱ����Ч
	GameSound* soundCheer; // ������ɺ�Ļ�����
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
