#ifdef __GAMEPLAY_PAIPAILE__

#ifndef __EFFECT_PAIPAILE_H__
#define __EFFECT_PAIPAILE_H__

#include "../helps/IEffectimpl.h"
#include "../helps/GameElementManager.h"
#include "../helps/XmlPackLoad.h"
#include "../utils/ActionSpeed.h"
#include "Paipaile.h"

#define PPL_BLOW_OFFX 0.0f
#define PPL_BLOW_OFFY -64.0f
#define PPL_COUNTDOWN_OFFX 4.0f
#define PPL_COUNTDOWN_OFFY -80.0f

class CEffectPaipaile : public IEffectImplBossTrigger, public IPplCallback
{
public:
	CEffectPaipaile()
	: m_nState(0)
	, m_bKeyPressed(false)
	, m_bPaused(false)
	, m_bFadeout(false)
	, m_nTimes(0)
	, m_fScale(0.0f)
	, m_fScale0(0.0f)
	, m_dwColor(0xFFFFFFFF)
	, m_bIsBeating(false)
	, m_bThetaSet(false)
	, m_fRadius(100.0f)
	{
		m_vecList.reserve(PLAYERS_MAX);
		memset(m_pPaipaile, 0, sizeof(m_pPaipaile));
	}

	~CEffectPaipaile()
	{
		
	}

public:
	virtual void OnPaipaileDone(LONG nPlayerId)
	{
		assert(nPlayerId >= 0);
		m_pPlayer[nPlayerId]->OnEffectEvent(gptPpl, gctEnd, 0, &m_Param[nPlayerId]);
	}
	virtual void OnPaipaileTimeEvent(LONG nPlayerId, LONG nCounter)
	{
		assert(nPlayerId >= 0);
		assert(nCounter >= 0);
		m_Param[nPlayerId].m_multiple = nCounter;
		m_pPlayer[nPlayerId]->OnEffectEvent(gptPpl, gctTimeEvent, 0, &m_Param[nPlayerId]);
	}

public:
	virtual void OnGameResourceLoad()
	{
		IEffectImplBossTrigger::OnGameResourceLoad();
		if (XmlPackLoad::Instance()->Xml().Sel("/setting"))
		{
			m_fRadius = XmlPackLoad::Instance()->Xml().GetFloat("radius");
		}
		m_pAnimBg = GameElementManager::Instance()->GetGameAnimation("ppl_bg.png");
		assert(m_pAnimBg);
		m_pAnimBeat = GameElementManager::Instance()->GetGameAnimation("ppl_beat.png");
		assert(m_pAnimBeat);
		m_pAnimBeat->SetLoop(false);
		m_pAnimBeat->Stop();
	}
	
	virtual void OnGameInit(CMainFrame* pMainFrame, const CMD_S_GameConfig& gameConfig)
	{
		int i;
		IEffectImplBossTrigger::OnGameInit(pMainFrame, gameConfig);
		SetPosition(GamePoint(m_scrSize / 2));
		for (i = 0; i < m_nPlayers; ++i)
		{
			m_ptBlow[i].z = m_pPlayer[i]->m_Playerdata.playpos->scoreAngel1;
			m_ptBlow[i].x = m_pPlayer[i]->m_Playerdata.playpos->scoreXPos1 + GetfThetaDX(m_ptBlow[i].z, PPL_BLOW_OFFX, PPL_BLOW_OFFY);
			m_ptBlow[i].y = m_pPlayer[i]->m_Playerdata.playpos->scoreYPos1 + GetfThetaDY(m_ptBlow[i].z, PPL_BLOW_OFFX, PPL_BLOW_OFFY);
			m_ptCountdown[i].z = m_pPlayer[i]->m_Playerdata.playpos->scoreAngel1;
			m_ptCountdown[i].x = m_pPlayer[i]->m_Playerdata.playpos->TaiupXpos + GetfThetaDX(m_ptCountdown[i].z, PPL_COUNTDOWN_OFFX, PPL_COUNTDOWN_OFFY);
			m_ptCountdown[i].y = m_pPlayer[i]->m_Playerdata.playpos->TaiupYpos + GetfThetaDY(m_ptCountdown[i].z, PPL_COUNTDOWN_OFFX, PPL_COUNTDOWN_OFFY);
			m_pPaipaile[i] = new CPaipaile(this, i, m_ptBlow[i], m_ptCountdown[i]);
		}
		GameParameters param;
		param.m_gameplayCommandType = gctStartSync;
		param.m_fTime = -1.0f;
		for (i = 0; i < m_nPlayers; ++i) {
			if (gameConfig.pplData[i].isPlaying/* && !m_pPlayer[i]->GetOwner()*/) {
				this->Start(i, &param);
				param.m_gameplayCommandType = gctStepSync;
				param.m_multiple = gameConfig.pplData[i].hits;
				this->Start(i, &param);
			}
		}
	}
	
	virtual void Start(LONG nPlayerId, GameParameters* param)
	{
		assert(nPlayerId >= 0);
		if (param)
		{
			if (gctStart == param->m_gameplayCommandType || gctStartSync == param->m_gameplayCommandType)
			{
				m_Param[nPlayerId] = *param;
				m_vecList.push_back(nPlayerId);
				if (!m_bThetaSet && !m_pPaipaile[nPlayerId]->IsPlaying())
				{
					m_position.z = m_pPlayer[nPlayerId]->m_Playerdata.playpos->scoreAngel1;
					m_bThetaSet = true;
				}
				if (m_nState < 1)
				{
					m_bFadeout = false;
					m_dwColor = 0xFFFFFFFF;
					m_pAnimBg->SetColor(m_dwColor);
					m_actSpeed.startWithDistance(1.0f, 1.5f);
					m_nState = 1;
					m_fScale = 0.0f;
					m_fScale0 = 0.0f;
				}
			}
			else if (gctStep == param->m_gameplayCommandType)
			{
				if (m_nState > 1)
				{
					if (m_pPaipaile[nPlayerId]->AddAHit())
					{
						m_Param[nPlayerId].m_Rect = m_Rect;
						m_pPlayer[nPlayerId]->OnEffectEvent(gptPpl, gctStep, 0, &m_Param[nPlayerId]);
						if (!m_bIsBeating)
						{
							m_pAnimBeat->Play();
						}
					}
				}
			}
			else if (gctStepSync == param->m_gameplayCommandType)
			{
				if (m_pPaipaile[nPlayerId]->setHits(param->m_multiple))
				{
					m_Param[nPlayerId].m_Rect = m_Rect;
					m_pPlayer[nPlayerId]->OnEffectEvent(gptPpl, gctStepSync, 0, &m_Param[nPlayerId]);
					if (!m_bIsBeating)
					{
						m_pAnimBeat->Play();
					}
				}
			}
			else if (gctEndSync == param->m_gameplayCommandType)
			{
				m_Param[nPlayerId].m_pplScore = param->m_pplScore;
				m_Param[nPlayerId].m_position = m_ptBlow[nPlayerId];
				m_pPlayer[nPlayerId]->ShowBeatingBoard(false);
				m_pPlayer[nPlayerId]->EnableKeyInput(true, true);
				m_pPlayer[nPlayerId]->OnEffectEvent(gptPpl, gctEndSync, 0, &m_Param[nPlayerId]);
				if (m_pPaipaile[nPlayerId]->IsPlaying()) {
					m_pPaipaile[nPlayerId]->end();
				}
			}
		}
	}

	virtual void OnGameQuit()
	{
		for (int i = 0; i < m_nPlayers; ++i)
		{
			SAFE_DELETE(m_pPaipaile[i]);
		}
	}

	virtual void OnUpdate(float dt)
	{
		int i;

		if (m_bFadeout)
		{
			m_actFade.update(dt);
			m_dwColor = m_actFade.getColor();
			m_pAnimBg->SetColor(m_dwColor);
			if (m_actFade.isDone())
			{
				m_bFadeout = false;
			}
		}
		m_bIsBeating = m_pAnimBeat->IsPlaying();
		if (m_nState)
		{
			switch(m_nState)
			{
			case 1:
				m_actSpeed.update(dt);
				m_fScale = m_fScale0 + m_actSpeed.getShift();
				if (m_actSpeed.isDone())
				{
					++m_nState;
				}
				break;
			case 2:
				{
					vector<int>::iterator iter = m_vecList.begin();
					if (iter != m_vecList.end())
					{
						int idx;
						for (i = m_vecList.size() - 1; i >= 0; --i)
						{
							idx = *(iter + i);
							if (!m_pPaipaile[idx]->IsPlaying())
							{
								m_pPaipaile[idx]->StartTimer(m_Param[idx].m_fTime);
								m_pPlayer[idx]->OnEffectEvent(gptPpl, gctStart, 0, 0);
								m_pPlayer[idx]->ShowBeatingBoard(true);
								m_pPlayer[idx]->EnableKeyInput(false, false);
							}
						}
						m_vecList.clear();
					}
					LONG nCount = 0;
					for (i = 0; i < m_nPlayers; ++i)
					{
						m_pPaipaile[i]->Update(dt);
						if (m_pPaipaile[i]->IsDone())
						{
							++nCount;
						}
					}
					if (m_nPlayers == nCount)
					{
						m_nState = 0;
						m_actFade.setColor(0xFFFFFFFF);
						m_actFade.fadeOut(1.5f);
						m_bFadeout = true;
						m_bThetaSet = false;
					}
				}
				break;
			}
		}
	}
	
	virtual void OnRender(long level)
	{
		if (level == GAMELEVEL_HIGH)
		{
			if (m_bFadeout)
			{
				m_pAnimBg->Render(m_position, m_fScale, m_fScale);
			}
			if (m_nState)
			{
				if (m_bIsBeating)
				{
					m_pAnimBeat->Render(m_position, m_fScale, m_fScale);
				}
				else
				{
					m_pAnimBg->Render(m_position, m_fScale, m_fScale);
				}
// 				for (int i = 0; i < m_nPlayers; ++i)
// 				{
// 					m_pPaipaile[i]->Render();
// 				}
			}
		} else if (level == GAMELEVEL_HIGEST) {
			for (int i = 0; i < m_nPlayers; ++i)
			{
				m_pPaipaile[i]->Render();
			}
		}
	}

	virtual void OnGameStateChange(LONG nState, LONG nOldState)
	{
		if (GAMESTATE_WAVING == nState)
		{
			for (int i = 0; i < m_nPlayers; ++i)
			{
				m_pPaipaile[i]->Pause();
			}
		}
		else if (GAMESTATE_RUNNING == nState)
		{
			for (int i = 0; i < m_nPlayers; ++i)
			{
				m_pPaipaile[i]->Resume();
			}
		}
	}

	virtual void OnResetData(BOOL bReport, BOOL bZero)
	{
		
	}
	
	virtual const char* GetIconFileName()
	{
		return "ico_paipaile.png";
	}
	
protected:
	virtual const char* GetXmlPackName()
	{
		return "effpaipaile.pak";
	}

	void SetPosition(const GamePoint& point)
	{
		GamePoint p = GamePointMake(m_fRadius, m_fRadius, 0.0f);
		m_position = point;
		m_Rect.origin = point - p;
		m_Rect.size = p * 2.0f;
	}

protected:
	GamePoint m_position, m_ptBlow[PLAYERS_MAX], m_ptCountdown[PLAYERS_MAX];
	LONG m_nState;
	CActionFade m_actFade;
	CActionSpeed m_actSpeed;
	DWORD m_dwColor;
	bool m_bKeyPressed, m_bPaused, m_bFadeout, m_bIsBeating, m_bThetaSet;
	float m_fScale, m_fScale0, m_fRadius;
	unsigned long m_nTimes;
	GameAnimation* m_pAnimBg;
	GameAnimation* m_pAnimBeat;
	CPaipaile* m_pPaipaile[PLAYERS_MAX];
	GameParameters m_Param[PLAYERS_MAX];
	GameRect m_Rect;
	vector<int> m_vecList;
};
#endif//__EFFECT_PAIPAILE_H__
#endif
