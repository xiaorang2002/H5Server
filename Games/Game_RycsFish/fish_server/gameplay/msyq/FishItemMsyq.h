#ifdef __GAMEPLAY_MSYQ__
#pragma once

#include "../../fish/FishItem.h"
#include "../../helps/GameElementManager.h"
#include "ActionMsyqRunning.h"

class CFishItemMsyq : public CFishItem
{
public:
    CFishItemMsyq();
    ~CFishItemMsyq();
	
public:
	void setPermit(LONG playerId, bool allow);

public:
    virtual VOID Render();
	virtual void OnCatched() {}
	virtual void OnCatching() {}
	virtual VOID InitializeCollision() {SetCollision(g_CollisionKing[FISHKING_MSYQ]);}
	/// try to initialize fish.
    virtual LONG Initialize(LONG nType);
	virtual BOOL IsObjCanHit(CFishItemBase* pObject, LONG nPlayerId);
	virtual void Update(float dt);
	virtual bool IfCanRender(BOOL bTop);
	virtual bool IsObjCanLock(LONG nPlayerId);
	
protected:
	CActionMsyqRunning* m_actionMsyqRunning;
	CActionMsyqStandby* m_actionMsyqStandby;
	BYTE m_privilege;
	GamePoint m_initPosition;
};

#endif//__GAMEPLAY_MSYQ__