
#include "../fish_server/StdAfx.h"
#include "AicsNewUserCheck.h"

CCAICSNewUserCheck::CCAICSNewUserCheck()
{
    _NewUserCheck = NewUserCheck::Instance();
}

CCAICSNewUserCheck::~CCAICSNewUserCheck()
{
}

int CCAICSNewUserCheck::OnUserSitdown(DWORD dwUserID, WORD wRoomID, double dWinCoin, double dRecharge)
{
	if (!_NewUserCheck)
	{
		return S_FALSE;
	}

	CCriticalSectionEx lock;
	lock.Lock(&_NewUserCheck->m_csThis);
	tagUserData* userData = _NewUserCheck->getUserData(dwUserID, wRoomID);
	if (!userData)
	{
		userData = _NewUserCheck->allocUserData();
		if (userData)
		{
			userData->userID = dwUserID;
			userData->roomID = wRoomID;
			userData->wci.dWinCoin = dWinCoin;
			userData->wci.dRecharge = dRecharge;
			if (!_NewUserCheck->addUserData(userData))
			{
				_NewUserCheck->freeUserData(userData);
			}
		}
	}
	else
	{
		userData->wci.dWinCoin = dWinCoin;
		userData->wci.dRecharge = dRecharge;
	}
	return S_OK;
}

int CCAICSNewUserCheck::OnUserStandup(DWORD dwUserID, WORD wRoomID)
{
	CCriticalSectionEx lock;
	lock.Lock(&_NewUserCheck->m_csThis);
	_NewUserCheck->removeUserData(dwUserID, wRoomID, true);
	return S_OK;
}

int CCAICSNewUserCheck::OnUpdateGainLost(DWORD dwUserID, WORD wRoomID, double dGain, double dLost)
{
	CCriticalSectionEx lock;
	lock.Lock(&_NewUserCheck->m_csThis);
	return _NewUserCheck->updateGainLost(dwUserID, wRoomID, dGain, dLost);
}
