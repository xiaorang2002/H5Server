#include "StdAfx.h"
#ifdef __GAMEPLAY_MSYQ__
#include "../../helps/EffectContainer.h"
#include "FishItemMsyq.h"

CFishItemMsyq::CFishItemMsyq() : m_actionMsyqRunning(NULL), m_actionMsyqStandby(NULL)
{
	m_bCatchable = false;
	m_nObjectType = 0;
	m_nObjectId = FISH_ID_MSYQ;
	m_bIsKing = TRUE;
	m_nKingType = FISHKING_MSYQ;
	m_privilege = 0;
}

CFishItemMsyq::~CFishItemMsyq()
{
	
}

void CFishItemMsyq::setPermit(LONG playerId, bool allow)
{
	if (allow) {
		m_privilege |= 1 << playerId;
		if (!IsSwimming()) {
			m_actionMsyqStandby->stop();
			m_actionMsyqRunning->play();
			m_bSwimIn = TRUE;
			SetSwiming();
		}
	} else {
		m_privilege &= ~(1 << playerId);
		if (0 == m_privilege) {
			m_actionMsyqStandby->play();
			m_actionMsyqRunning->stop();
			m_bSwimIn = FALSE;
			m_eState = FISHSTATE_READY;
			m_bBoompause = false;
			m_fPauseBoomTime = 0.0f;
		}
	}
}

LONG CFishItemMsyq::Initialize(LONG nType)
{
	m_actionMsyqRunning = GetEffectObject(CEffectMsyq, gptMsyq)->getActionMsyqRunning();
	m_actionMsyqStandby = GetEffectObject(CEffectMsyq, gptMsyq)->getActionMsyqStandby();
//	m_actionMsyqStandby->play();
	m_actionMsyqRunning->bindPosition(m_Objpst);
	m_actionMsyqRunning->bindPaused(m_bBoompause);

	//Cleanup:
    return (S_OK);
}

BOOL CFishItemMsyq::IsObjCanHit(CFishItemBase* pObject, LONG nPlayerId)
{
	return ((m_privilege >> nPlayerId) & 0x1) && CFishItem::IsObjCanHit(pObject, nPlayerId);
}

bool CFishItemMsyq::IsObjCanLock(LONG nPlayerId)
{
	return ((m_privilege >> nPlayerId) & 0x1);
}

///  CFishItem::Update(float fDelta)
VOID CFishItemMsyq::Update(float dt)
{	
    if (IsSwimming()) {
// 		m_actionMsyqRunning->Update(dt);
		// pause by boom.
		if (m_bBoompause) {
			if (m_fPauseBoomTimeOut >= 0.0f) {
				m_fPauseBoomTime += dt;
				if (m_fPauseBoomTime >= m_fPauseBoomTimeOut) {
					m_bBoompause = false;
					m_fPauseBoomTime = 0.0f;
				}
			}
		} else {
			m_fCollTime += dt;
			if (m_fCollTime > 0.3f) {
				// try to update circle.
				UpdateCollisionCircle();
				m_fCollTime = 0.0f;
			}
		}
	} else {
// 		m_actionMsyqStandby->Update(dt);
	}
}

/// try to render the object.
void CFishItemMsyq::Render()
{
	if (m_privilege) {
		CFishItemBase::Render();
	}
	m_actionMsyqStandby->Render();
	m_actionMsyqRunning->Render();
}

// try to check if can render now.
bool CFishItemMsyq::IfCanRender(BOOL bTop)
{
	if (bTop) {
		if (m_privilege) {
			return true;
		}
	} else if (!m_privilege) {
		return true;
	}
	return false;
}

#endif//__GAMEPLAY_MSYQ__