#ifndef __PAIPAILE_H__
#define __PAIPAILE_H__

#include "../utils/ActionTextBlow.h"
#include "../utils/ActionTextCountdown.h"

class IPplCallback
{
public:
	virtual void OnPaipaileTimeEvent(LONG nPlayerId, LONG nCounter) = 0;
	virtual void OnPaipaileDone(LONG nPlayerId) = 0;
};

class CPaipaile : public HgeGameObject
{
public:
	CPaipaile(IPplCallback* callback, LONG playerid, const GamePoint& pos_blow, const GamePoint& pos_countdown)
	: m_pCallback(callback)
	, m_nPlayerId(playerid)
	, m_bIsPlaying(false)
	, m_bPaused(false)
	, m_bAdded(false)
	, m_bIsDone(true)
	, m_inifinite(false)
	, m_fTime(0.0f)
	, m_fTimeLimit(0.0f)
	, m_nState(0)
	{
		m_actBlow.setPosition(pos_blow);
		m_actCountdown.setPosition(pos_countdown);
		m_pSnd = GameElementManager::Instance()->GetGameSound("ppl.mp3");
		m_pSnd = CLONE_SOUND(m_pSnd);
	}

	~CPaipaile()
	{
		SAFE_AUTORELEASE(m_pSnd);
	}

public:
	void StartTimer(float time)
	{
		if (time < 0.0f) {
			m_inifinite = true;
		} else {
			m_inifinite = false;
			m_actCountdown.start(time);
		}
		m_actBlow.start();
		m_bAdded = false;
		m_bIsPlaying = true;
		m_bIsDone = false;
		m_bPaused = false;
		m_fTimeLimit = 30.f;
		m_fTime = 0.0f;
		m_nState = 1;
	}

	void Pause()
	{
		m_bPaused = true;
	}
	
	void Resume()
	{
		m_bPaused = false;
	}

	bool AddAHit()
	{
		if (m_inifinite) {
			return false;
		}
		if (!m_bPaused && m_bIsPlaying && m_actCountdown.IsPlaying())
		{
			if (!m_bAdded)
			{
				m_pSnd->Play(true);
			}
			m_bAdded = true;
			m_actBlow.count();
			return true;
		}
		return false;
	}

	bool setHits(WORD number)
	{
		if (m_bIsPlaying)
		{
			return m_actBlow.setCounterNumber(number);
		}
		return false;
	}

	void end()
	{
		m_actBlow.resetCounter();
		m_bIsPlaying = false;
		m_bIsDone = true;
		m_bPaused = false;
		m_bAdded = false;
		m_nState = 0;
	}

	void Update(float dt)
	{
		if (m_bIsPlaying)
		{
			if (m_inifinite) {
				m_actBlow.update(dt);
			} else {
				if (1 == m_nState) {
					if (!m_bAdded) {
						m_fTimeLimit -= dt;
					}
					else if (!m_bPaused) {
						m_actCountdown.update(dt);
						if (m_fTime >= 1.0f) {
							if (m_pCallback) {
								m_pCallback->OnPaipaileTimeEvent(m_nPlayerId, m_actBlow.getCounter());
							}
							m_fTime = 0.0f;
						} else {
							m_fTime += dt;
						}
					}
					m_actBlow.update(dt);
					if (m_actCountdown.IsDone()) {
						m_pSnd->Stop();
						m_nState = 2;
						m_fTime = 1.5f;
						if (m_pCallback) {
							m_pCallback->OnPaipaileTimeEvent(m_nPlayerId, m_actBlow.getCounter());
						}
					}
					else if (!m_bAdded && m_fTimeLimit <= 0.0f) {
						m_bAdded = true;
						m_pSnd->Play(true);
					}
				} else if (2 == m_nState) {
					if (m_fTime > 0.0f) {
						m_actBlow.update(dt);
						m_fTime -= dt;
					} else {
						m_pCallback->OnPaipaileDone(m_nPlayerId);
						m_nState = 3;
					}
				}
			}
		}
	}
	
	void Render()
	{
		if (m_bIsPlaying)
		{
			if (m_bAdded)
			{
				m_actCountdown.render();
			}
			m_actBlow.render();
		}
	}

	inline bool IsPlaying() {return m_bIsPlaying;}
	inline bool IsDone() {return m_bIsDone;}

protected:
	bool m_bIsPlaying, m_bPaused, m_bAdded, m_bIsDone;
	float m_fTime, m_fTimeLimit;
	LONG m_nPlayerId, m_nState;
	bool m_inifinite;
	GameSound* m_pSnd;
	IPplCallback* m_pCallback;
	CActionTextBlow m_actBlow;
	CActionTextCountdown m_actCountdown;
};
#endif//__PAIPAILE_H__