#pragma once

#include "MsyqDef.h"
#include "../../utils/ActionSpeed.h"
#include "../../utils/ActionTimeInterval.h"

#define MSYQ_STANDBY_MOVE_SPEED_SLOW 100
#define MSYQ_STANDBY_MOVE_SPEED_FAST 200

class CActionMsyqStandby : public HgeGameObject
{
public:
	CActionMsyqStandby() : m_playedOnceMore(false), m_state(msyqssNone), 
		m_color(-1), m_animCurrent(NULL), m_movementCount(0), m_immovableCount(0), m_currentMovementIndex(0)
	{
		m_actionFade.setAlpha(0xFF);
		m_moveSpeed[0] = MSYQ_STANDBY_MOVE_SPEED_SLOW;
		m_moveSpeed[1] = MSYQ_STANDBY_MOVE_SPEED_FAST;
	}

	~CActionMsyqStandby()
	{

	}

public:
	void initialize(const GameRect& rectMovementRange, const MsyqRenderer& renderer)
	{
		int i;
		m_rectMovementRange = rectMovementRange;
		m_msyqRenderer = renderer;
		for (i = 0; i < MAX_MSYQ_STANDBY; ++i) {
			if (renderer.animStandbyStatsMovement[i]) {
				renderer.animStandbyStatsMovement[i]->SetLoop(true);
				++m_movementCount;
			}
			if (renderer.animStandbyStatsImmovable[i]) {
				if (0 == i) {
					renderer.animStandbyStatsImmovable[i]->SetLoop(true);
				} else {
					renderer.animStandbyStatsImmovable[i]->SetLoop(false);
				}
				++m_immovableCount;
			}
		}
	}

	void play()
	{
		if (m_movementCount) {
			if (!m_playedOnceMore) {
				m_state = msyqssMove;
				m_playedOnceMore = true;
				m_currentMovementIndex = Random.Range(0, m_movementCount - 1);
				m_animCurrent = m_msyqRenderer.animStandbyStatsMovement[m_currentMovementIndex];
				m_position = GamePointMake(-m_animCurrent->GetContentSize().height / 2, m_scrSize.height / 2, 0);
				float distance = fabs(m_position.x) + m_scrSize.width / 2;
				m_actionSpeed.startWithPoint(m_position, m_scrSize / 2, distance / m_moveSpeed[m_currentMovementIndex]);
			} else {
				m_state = msyqssFadeIn;
				m_actionFade.fadeIn(1.5f);
				m_animCurrent = m_msyqRenderer.animStandbyStatsImmovable[0];
				m_animCurrent->Play();
			}
		}
	}
	
	void stop()
	{
		if (msyqssFadeOut != m_state) {
			m_state = msyqssFadeOut;
			m_actionFade.fadeOut(1.5f);
		}
	}

	virtual void Update(float dt)
	{
		switch(m_state)
		{
		case msyqssNone:
			break;
		case msyqssMove:
			if (!m_actionSpeed.isDone()) {
				m_actionSpeed.update(dt);
			} else {
				actionSelect();
			}
			break;
		case msyqssStandby:
			if (!m_animCurrent->IsPlaying()) {
				m_animCurrent = m_msyqRenderer.animStandbyStatsImmovable[0];
				m_animCurrent->Play();
			}
			if (m_actionTimeInterval.isDone()) {
				actionSelect();
			} else {
				m_actionTimeInterval.update(dt);
			}
			break;
		case msyqssFadeIn:
			m_color = m_actionFade.getColor();
			if (m_actionFade.isDone()) {
				m_state = msyqssStandby;
				actionSelect();
			} else {
				m_actionFade.update(dt);
			}
			break;
		case msyqssFadeOut:
			m_color = m_actionFade.getColor();
			if (m_actionFade.isDone()) {
				m_state = msyqssNone;
				m_animCurrent = NULL;
			} else {
				m_actionFade.update(dt);
			}
			break;
		}
	}

	void Render()
	{
		if (m_animCurrent) {
			m_animCurrent->SetColor(m_color);
			m_animCurrent->Render(m_position);
		}
	}

private:
	void actionSelect()
	{
		int randNumber = Random.Range(0, 2);
		if (!randNumber) { // ´ý»ú<->ÒÆ¶¯×´Ì¬×ª»»
			if (msyqssMove == m_state) {
				if (m_immovableCount) {
					m_state = msyqssStandby;
					m_animCurrent = m_msyqRenderer.animStandbyStatsImmovable[0];
					m_animCurrent->Play();
					createNextStandby();
				} else {
					createNextMovement();
				}
			} else if (msyqssStandby == m_state) {
				if (m_movementCount) {
					m_state = msyqssMove;
					m_currentMovementIndex = Random.Range(0, m_movementCount - 1);
					m_animCurrent = m_msyqRenderer.animStandbyStatsMovement[m_currentMovementIndex];
					createNextMovement();
				} else {
					createNextStandby();
				}
			}
		} else {
			if (msyqssMove == m_state) {
				createNextMovement();
			} else if (msyqssStandby == m_state) {
				createNextStandby();
			}
		}
	}

	void createNextMovement()
	{
		GamePoint to(Random.Range(m_rectMovementRange.GetMinX(), m_rectMovementRange.GetMaxX()),
			Random.Range(m_rectMovementRange.GetMinY(), m_rectMovementRange.GetMaxY()), 0);
		float distance = to.Distance(m_position);
		m_actionSpeed.startWithPoint(m_position, to, distance / m_moveSpeed[m_currentMovementIndex]);
	}

	void createNextStandby()
	{
		int randNumber = Random.Range(0, 2);
		float time = Random.Range(1.0f, 3.0f);
		if (randNumber || 1 == m_immovableCount) {
			m_actionTimeInterval.startTimer(time);
		} else {
			m_animCurrent = m_msyqRenderer.animStandbyStatsImmovable[Random.Range(1, m_immovableCount - 1)];
			m_animCurrent->Play();
			time += m_animCurrent->GetFrames() / m_animCurrent->GetFPS();
			m_actionTimeInterval.startTimer(time);
		}
	}

protected:
	WORD m_movementCount, m_immovableCount;
	GameAnimation* m_animCurrent;
	MsyqRenderer m_msyqRenderer;
	GameRect m_rectMovementRange;
	GamePoint m_position;
	CActionSpeed m_actionSpeed;
	CActionFade m_actionFade;
	CActionTimeInterval m_actionTimeInterval;
	ActionMsyqStandbyState m_state;
	bool m_playedOnceMore;
	int m_currentMovementIndex;
	DWORD m_color;
	WORD m_moveSpeed[MAX_MSYQ_STANDBY];
};