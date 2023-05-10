#ifdef __GAMEPLAY_MSYQ__
#pragma once

#include "../../helps/IEffectimpl.h"
#include "../../helps/GameElementManager.h"
#include "../../helps/XmlPackLoad.h"
#include "MsyqDef.h"
#include "ActionMsyqStandby.h"
#include "ActionMsyqRunBack.h"
#include "ActionMsyqRunning.h"
#include "FishItemMsyq.h"
#include "../../utils/ObjectAllocator.h"

class CEffectMsyq : public IEffectImplBase, public HgeGameObject, public IMsyqRunBackEvent
{
public:
	CEffectMsyq() : m_animRunback(NULL), m_fishMsyq(NULL)
	{
		memset(&m_msyqRenderer, 0, sizeof(m_msyqRenderer));
		memset(m_msyqTotalScore, 0, sizeof(m_msyqTotalScore));
		memset(m_showPrompt, 0, sizeof(m_showPrompt));
		m_rectStandbyMovementRange.SetRect(0.0f, 0.0f, m_scrSize.width, m_scrSize.height);
	}

	~CEffectMsyq()
	{
		
	}

protected:
	virtual const char* GetXmlPackName() {return "effmsyq.pak";}
	virtual void OnXmlPackLoaded(CXmlParserA &xml)
	{
		const char* node;
		GamePoint anchorPoint(0.5f, 0.5f, 0.0f);
		int standbyImmovableCount = 0;
		int standbyMovementCount = 0;
		if (xml.Sel("/ui/running")) {
			node = xml.GetString("movement");
			if (node && strlen(node)) {
				m_msyqRenderer.animRunningStatsMovement = GameElementManager::Instance()->GetGameAnimation(node);
				if (m_msyqRenderer.animRunningStatsMovement) {
					anchorPoint.x = g_CollisionKing[FISHKING_MSYQ].center.x / m_msyqRenderer.animRunningStatsMovement->GetContentSize().width;
					anchorPoint.y = g_CollisionKing[FISHKING_MSYQ].center.y / m_msyqRenderer.animRunningStatsMovement->GetContentSize().height;
					m_msyqRenderer.animRunningStatsMovement->SetAnchorPoint(anchorPoint);
				}
			}
			node = xml.GetString("kick");
			if (node && strlen(node)) {
				m_msyqRenderer.animRunningStatsKick = GameElementManager::Instance()->GetGameAnimation(node);
				if (m_msyqRenderer.animRunningStatsKick) {
					m_msyqRenderer.animRunningStatsKick->SetAnchorPoint(anchorPoint);
				}
			}
			node = xml.GetString("strike");
			if (node && strlen(node)) {
				m_msyqRenderer.animRunningStatsStrike = GameElementManager::Instance()->GetGameAnimation(node);
				if (m_msyqRenderer.animRunningStatsStrike) {
					m_msyqRenderer.animRunningStatsStrike->SetAnchorPoint(GamePointMake(0, 0.5f, 0));
					m_msyqRenderer.animRunningStatsStrike->Stop();
				}
			}
			node = xml.GetString("prompt");
			if (node && strlen(node)) {
				m_msyqRenderer.animRunningStatsPrompt = GameElementManager::Instance()->GetGameAnimation(node);
			}
		}
		if (xml.Sel("/ui/offset")) {
			CTextParserUtility txtParser;
			GameSize size;
			node = xml.GetString("standby_movement_range_margin");
			if (node && strlen(node)) {
				txtParser.Parse(node, ",");
				size.width = atoi(txtParser[0]);
				if (txtParser.Count() > 1) size.height = atoi(txtParser[1]);
				m_rectStandbyMovementRange.origin += size;
				m_rectStandbyMovementRange.size -= size + size;
			}
			node = xml.GetString("strike");
			if (node && strlen(node)) {
				txtParser.Parse(node, ",");
				m_msyqOffsetStrike.x = atoi(txtParser[0]);
				if (txtParser.Count() > 1) m_msyqOffsetStrike.y = atoi(txtParser[1]);
			}
			node = xml.GetString("prompt");
			if (node && strlen(node)) {
				txtParser.Parse(node, ",");
				m_msyqOffsetPrompt.x = atoi(txtParser[0]);
				if (txtParser.Count() > 1) m_msyqOffsetPrompt.y = atoi(txtParser[1]);
			}
			node = xml.GetString("runback");
			if (node && strlen(node)) {
				txtParser.Parse(node, ",");
				m_msyqOffsetRunback.x = atoi(txtParser[0]);
				if (txtParser.Count() > 1) m_msyqOffsetRunback.y = atoi(txtParser[1]);
			}
		}
		if (xml.Sel("/ui/standby")) {
			if (xml.Sel("immovable")) {
				if (xml.CountSubet("item") > 0) {
					while(xml.MoveNext() && standbyImmovableCount < MAX_MSYQ_STANDBY) {
						node = xml.GetString(NULL);
						if (node && strlen(node)) {
							m_msyqRenderer.animStandbyStatsImmovable[standbyImmovableCount] = GameElementManager::Instance()->GetGameAnimation(node);
							if (m_msyqRenderer.animStandbyStatsImmovable[standbyImmovableCount]) {
								m_msyqRenderer.animStandbyStatsImmovable[standbyImmovableCount]->SetAnchorPoint(anchorPoint);
								++standbyImmovableCount;
							}
						}
					}
				}
				xml.Sel("..");
			}
			if (xml.Sel("movement")) {
				if (xml.CountSubet("item") > 0) {
					while(xml.MoveNext() && standbyMovementCount < MAX_MSYQ_STANDBY) {
						node = xml.GetString(NULL);
						if (node && strlen(node)) {
							m_msyqRenderer.animStandbyStatsMovement[standbyMovementCount] = GameElementManager::Instance()->GetGameAnimation(node);
							if (m_msyqRenderer.animStandbyStatsMovement[standbyMovementCount]) {
								m_msyqRenderer.animStandbyStatsMovement[standbyMovementCount]->SetAnchorPoint(anchorPoint);
								++standbyMovementCount;
							}
						}
					}
				}
				xml.Sel("..");
			}
		}
		if (xml.Sel("/ui/sound")) {
			node = xml.GetString("kick");
			if (node && strlen(node)) {
				m_msyqRenderer.soundKick = GameElementManager::Instance()->GetGameSound(node);
			}
			node = xml.GetString("strike");
			if (node && strlen(node)) {
				m_msyqRenderer.soundStrike = GameElementManager::Instance()->GetGameSound(node);
			}
			node = xml.GetString("cheer");
			if (node && strlen(node)) {
				m_msyqRenderer.soundCheer = GameElementManager::Instance()->GetGameSound(node);
			}
		}
	}

public:
	virtual void onMsyqRunBackDone(LONG playerId, LONG score)
	{
		GameParameters param;
		param.m_msyqScore = score;
		param.m_position = m_msyqRunback[playerId];
		m_pPlayer[playerId]->OnEffectEvent(gptMsyq, gctStep, 0, &param);
	}
	
public:
	CActionMsyqRunning* getActionMsyqRunning()
	{
		return &m_actionRunning;
	}

	CActionMsyqStandby* getActionMsyqStandby()
	{
		return &m_actionStandby;
	}

public:
	virtual void OnGameInit(CMainFrame* pMainFrame, const CMD_S_GameConfig& gameConfig)
	{
		int i;
		IEffectImplBase::OnGameInit(pMainFrame, gameConfig);
		m_fishMsyq = new CFishItemMsyq();
		m_animRunback = CLONE_ANIMATION(m_msyqRenderer.animRunningStatsMovement);
		m_animRunback->SetLoop(true);
		m_animRunback->Play();
		m_actionStandby.initialize(m_rectStandbyMovementRange, m_msyqRenderer);
		m_actionRunning.initialize(m_msyqRenderer, m_msyqOffsetStrike);
		for (i = 0; i < m_nPlayers; ++i) {
			GamePoint point(m_msyqOffsetRunback);
			m_msyqRunback[i].SetPoint(m_msyqOffsetRunback.x, m_msyqOffsetRunback.y, m_pPlayer[i]->m_Playerdata.playpos->scoreAngel);
			m_msyqRunback[i].Rotate(m_msyqRunback[i].z);
			m_msyqRunback[i].x += m_pPlayer[i]->m_Playerdata.playpos->TaiXPos;
			m_msyqRunback[i].y += m_pPlayer[i]->m_Playerdata.playpos->TaiYPos;
			m_msyqPrompt[i].SetPoint(m_msyqOffsetPrompt.x, m_msyqOffsetPrompt.y, m_pPlayer[i]->m_Playerdata.playpos->scoreAngel);
			m_msyqPrompt[i].Rotate(m_msyqPrompt[i].z);
			m_msyqPrompt[i].x += m_pPlayer[i]->m_Playerdata.playpos->TaiXPos;
			m_msyqPrompt[i].y += m_pPlayer[i]->m_Playerdata.playpos->TaiYPos;
			for (WORD j = 0; j < m_actionRunBack[i].MaxSize(); ++j) {
				m_actionRunBack[i].AllAt(j).initialize(i, m_animRunback, m_fishMsyq->m_Objpst,
					GamePointMake(m_pPlayer[i]->m_Playerdata.playpos->TaiXPos, m_pPlayer[i]->m_Playerdata.playpos->TaiYPos, 0),
					this);
			}
		}
		pMainFrame->GetLaucher()->m_alivelist.PushBack(m_fishMsyq);
		GameSize size = m_msyqRenderer.animRunningStatsMovement->GetContentSize();
		m_fishMsyq->Initialize(0);
		m_fishMsyq->SetTxRadius(size.width, size.height);
		m_fishMsyq->SetCollision(g_CollisionKing[FISHKING_MSYQ]);
		m_actionStandby.play();
		for (i = 0; i < m_nPlayers; ++i) {
			m_pPlayer[i]->SetProgressValue(gameConfig.msyqData.msyqPercent[i]);
			if (gameConfig.msyqData.msyqPercent[i] >= 100) {
				m_fishMsyq->setPermit(i, true);
				m_showPrompt[i] = true;
			}
		}
	}
	
	virtual void Start(LONG nPlayerId, GameParameters* param)
	{
		if (param) {
			switch(param->m_gameplayCommandType)
			{
			case gctStart:
				m_fishMsyq->setPermit(nPlayerId, true);
				m_showPrompt[nPlayerId] = true;
				break;
			case gctPlaying:
				m_actionRunning.strike();
				m_msyqTotalScore[nPlayerId] += param->m_msyqScore;
				if (CActionMsyqRunBack* p = m_actionRunBack[nPlayerId].Alloc()) {
					p->play(param->m_msyqScore);
				}
				break;
			case gctEnd:
				if (m_msyqRenderer.soundCheer) {
					m_msyqRenderer.soundCheer->Play(false);
				}
				m_fishMsyq->setPermit(nPlayerId, false);
				m_pPlayer[nPlayerId]->OnEffectEvent(gptMsyq, gctEnd, 0, &m_msyqTotalScore[nPlayerId]);
				m_msyqTotalScore[nPlayerId] = 0;
				m_showPrompt[nPlayerId] = false;
				break;
			}
		}
	}

	virtual void OnGameQuit()
	{
		SAFE_AUTORELEASE(m_animRunback);
		SAFE_DELETE(m_fishMsyq);
	}

	virtual void OnUpdate(float dt)
	{
		m_actionStandby.Update(dt);
		m_actionRunning.Update(dt);
		for (int i = 0; i < m_nPlayers; ++i) {
			WORD j = 0;
			while(j < m_actionRunBack[i].Size()) {
				if (m_actionRunBack[i].At(j).isDone()) {
					m_actionRunBack[i].QuickFree(j);
				} else {
					m_actionRunBack[i].At(j).update(dt);
					++j;
				}
			}
		}
	}
	
	virtual void OnRender(long level)
	{
		if (level == GAMELEVEL_HIGH) {
			for (int i = 0; i < m_nPlayers; ++i) {
				for (WORD j = 0; j < m_actionRunBack[i].Size(); ++j) {
					m_actionRunBack[i].At(j).render();
				}
				if (m_msyqRenderer.animRunningStatsPrompt && m_showPrompt[i]) {
					m_msyqRenderer.animRunningStatsPrompt->Render(m_msyqPrompt[i]);
				}
			}
		}
	}

	virtual void OnGameStateChange(LONG nState, LONG nOldState)
	{
		
	}

protected:
	MsyqRenderer m_msyqRenderer;
	GameAnimation* m_animRunback;
	CFishItemMsyq* m_fishMsyq;
	CActionMsyqStandby m_actionStandby;
	CActionMsyqRunning m_actionRunning;
	bool m_showPrompt[PLAYERS_MAX];
	ObjectAllocator<CActionMsyqRunBack, 100> m_actionRunBack[PLAYERS_MAX];
	GameRect m_rectStandbyMovementRange;
	LONG m_msyqTotalScore[PLAYERS_MAX];
	GamePoint m_msyqRunback[PLAYERS_MAX];
	GamePoint m_msyqOffsetStrike, m_msyqOffsetPrompt, m_msyqPrompt[PLAYERS_MAX], m_msyqOffsetRunback;
};
#endif//__GAMEPLAY_MSYQ__