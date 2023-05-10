#ifndef __AICSNEWUSERCHECK_HEADER__
#define __AICSNEWUSERCHECK_HEADER__

#include "NewUserCheck.h"

class CCAICSNewUserCheck
{
public:
	CCAICSNewUserCheck();
	virtual ~CCAICSNewUserCheck();
	
public:
	int OnUpdateGainLost(DWORD dwUserID, WORD wRoomID, double dGain, double dLost);
	int OnUserSitdown(DWORD dwUserID, WORD wRoomID, double dWinCoin, double dRecharge);
	int OnUserStandup(DWORD dwUserID, WORD wRoomID);
	
protected:
	NewUserCheck* _NewUserCheck;
};

#endif//__AICSNEWUSERCHECK_HEADER__