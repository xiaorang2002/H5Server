#pragma once

#include "MsyqDef.h"
#include "../../utils/ActionFade.h"

#define MSYQ_RUNNING_ROTATION_SPEED M_PI_4
#define MSYQ_RUNNING_RADIUS 190

class CActionMsyqRunning : public HgeGameObject
{
public:
	CActionMsyqRunning() : m_position(NULL), m_animCurrent(NULL), m_color(-1), m_state(0), m_strike(false)
	, m_isPlaying(false), m_paused(NULL), m_thetaSum(0.0f)
	{
		memset(&m_msyqRenderer, 0, sizeof(m_msyqRenderer));
		m_centerPosition = m_scrSize / 2;
		m_initPosition.SetPoint(0, -MSYQ_RUNNING_RADIUS, 0);
		m_initPosition.Rotate(m_initPosition.z);
		m_initPosition += m_centerPosition;
	}

	~CActionMsyqRunning()
	{
	}

public:
	void initialize(const MsyqRenderer& renderer, const GamePoint& offsetStrike)
	{
		m_msyqRenderer = renderer;
		m_msyqRenderer.animRunningStatsKick->SetLoop(false);
		m_msyqRenderer.animRunningStatsStrike->SetLoop(false);
		m_msyqRunningSize = m_msyqRenderer.animRunningStatsMovement->GetContentSize();
		m_msyqOffsetStrike = offsetStrike;
	}

	void bindPaused(const bool& pause)
	{
		m_paused = &pause;
	}

	void bindPosition(GamePoint& position)
	{
		m_position = &position;
	}

	void play()
	{
		m_state = 1;
		if (m_msyqRenderer.soundKick && !m_msyqRenderer.soundKick->IsPlaying()) {
			m_msyqRenderer.soundKick->Play(false);
		}
		m_animCurrent = m_msyqRenderer.animRunningStatsKick;
		m_animCurrent->Play();
		m_msyqRenderer.animRunningStatsStrike->SetColor(-1);
		m_msyqRenderer.animRunningStatsMovement->SetColor(-1);
		m_isPlaying = true;
		*m_position = m_initPosition;
		m_position->z += M_PI_2;
		m_thetaSum = m_initPosition.z - M_PI_2;
		m_strike = false;
	}

	void stop()
	{
		m_state = 3;
		m_actionFade.setAlpha(0xFF);
		m_actionFade.fadeOut(1.0f);
		m_isPlaying = false;
		m_strike = false;
	}

	void strike()
	{
		if (2 == m_state && !m_strike) {
			m_strike = true;
			m_msyqRenderer.animRunningStatsStrike->Play();
			if (m_msyqRenderer.soundStrike && (!m_msyqRenderer.soundStrike->IsPlaying() || m_msyqRenderer.soundStrike->GetPosition() > 0.5f)) {
				m_msyqRenderer.soundStrike->Play(false);
			}
		}
	}

	void Update(float dt)
	{
		if (m_state) {
			if (1 == m_state) {
				if (!m_animCurrent->IsPlaying()) {
 					m_state = 2;
					m_animCurrent = m_msyqRenderer.animRunningStatsMovement;
				}
			} else if (2 == m_state) {
				if (!*m_paused) {
					m_thetaSum += MSYQ_RUNNING_ROTATION_SPEED * dt;
					m_position->x = m_centerPosition.x + MSYQ_RUNNING_RADIUS * cosf(m_thetaSum);
					m_position->y = m_centerPosition.y + MSYQ_RUNNING_RADIUS * sinf(m_thetaSum);
					m_position->z = m_thetaSum + M_PI;
				}
				m_animCurrent = m_msyqRenderer.animRunningStatsMovement;
				m_strike = m_msyqRenderer.animRunningStatsStrike->IsPlaying();
			} else if (3 == m_state) {
				m_animCurrent->SetColor(m_actionFade.getColor());
				if (m_strike) {
					m_msyqRenderer.animRunningStatsStrike->SetColor(m_actionFade.getColor());
				}
				if (m_actionFade.isDone()) {
					m_state = 0;
					m_animCurrent = NULL;
				} else {
					m_actionFade.update(dt);
				}
			}
			if (m_strike) {
				m_msyqStrikePosition.SetPoint(m_msyqOffsetStrike.x, m_msyqOffsetStrike.y, m_position->z);
				m_msyqStrikePosition.Rotate(m_position->z);
				m_msyqStrikePosition += *m_position;
			}
		}
	}

	void Render()
	{
		if (m_state) {
			if (m_animCurrent) {
				m_animCurrent->Render(*m_position);
			}
			if (m_strike)
			{
				m_msyqRenderer.animRunningStatsStrike->Render(m_msyqStrikePosition);
			}
		}
	}
	
	const GameSize& getContentSize() {return m_msyqRunningSize;}

	inline bool isPlaying() {return m_isPlaying;}

private:
	GamePoint* m_position;
	GamePoint m_msyqOffsetStrike, m_msyqStrikePosition;
	GamePoint m_initPosition, m_centerPosition;
	GameSize m_msyqRunningSize;
	float m_thetaSum;
	MsyqRenderer m_msyqRenderer;
	GameAnimation* m_animCurrent;
	CActionFade m_actionFade;
	DWORD m_color;
	short m_state;
	bool m_strike, m_isPlaying;
	const bool* m_paused;
};