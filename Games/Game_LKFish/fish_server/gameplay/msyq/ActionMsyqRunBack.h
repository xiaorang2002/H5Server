#pragma once

#include "MsyqDef.h"
#include "../../utils/ActionSpeed.h"

#define MSYQ_RUN_BACK_SPEED 400
#define MSYQ_RUN_BACK_SCALE 0.5f

class CActionMsyqRunBack
{
public:
	CActionMsyqRunBack() : m_animRunBack(NULL), m_isPlaying(false), m_isDone(false)
	, m_score(0), m_event(NULL), m_playerId(0), m_positionFrom(NULL)
	{

	}
	~CActionMsyqRunBack()
	{

	}

public:
	void initialize(LONG playerId, GameAnimation* animRunBack, const GamePoint&from, const GamePoint& to, IMsyqRunBackEvent* evt)
	{
		m_playerId = playerId;
		m_animRunBack = animRunBack;
		m_positionFrom = &from;
		m_positionTo = to;
		m_event = evt;
	}
	
	void play(LONG score)
	{
		m_score = score;
		m_isPlaying = true;
		m_isDone = false;
		m_position = *m_positionFrom;
		m_actionSpeed.startWithPoint(m_position, m_positionTo, m_positionTo.Distance(m_position) / MSYQ_RUN_BACK_SPEED);
	}

	void update(float dt)
	{
		if (m_isPlaying) {
			if (m_actionSpeed.isDone()) {
				m_isDone = true;
				m_isPlaying = false;
				if (m_event) {
					m_event->onMsyqRunBackDone(m_playerId, m_score);
				}
				m_score = 0;
			} else {
				m_actionSpeed.update(dt);
			}
		}
	}

	void render()
	{
		if (m_animRunBack && m_isPlaying) {
			m_animRunBack->Render(m_position, MSYQ_RUN_BACK_SCALE, MSYQ_RUN_BACK_SCALE);
		}
	}
	
	inline bool isPlaying() {return m_isPlaying;}
	inline bool isDone() {return m_isDone;}

private:
	GameAnimation* m_animRunBack;
	IMsyqRunBackEvent* m_event;
	GamePoint m_position;
	const GamePoint* m_positionFrom;
	GamePoint m_positionTo;
	LONG m_score, m_playerId;
	CActionSpeed m_actionSpeed;
	bool m_isPlaying, m_isDone;
};