#pragma once

#include <string>
#include <types.h>
#include <kernel/CriticalSection.h>

#ifdef _WIN32
	#ifdef  UNICODE
	#define tstring std::wstring
	#else
	#define tstring std::string
	#endif//UNICODE
#else
#define tstring std::string
#endif//_WIN32



struct tagUserPrefs
{
	DWORD userID;
	WORD tableID;
	WORD chairID;
	WORD configID;
	WORD ticketRatio;
	double exchangeRatio;
        tstring nickName;
	BOOL isOldUser;
	double sumGain;
	double sumLost;
	tstring beginTime;
	double srcGold;
	double dWinCoin;
	double dRecharge;
};

#include <object/ObjectAllocator.h>

#ifdef  _WIN32
#include "../AICSNewUserCheck/AICSNewUserCheck.h"
#else
// #include "PlayerLogMDB.h"
#include "../../../public/ITableFrame.h"
#include "../../../public/IServerUserItem.h"
#include "../user_check/AicsNewUserCheck.h"
#endif//_WIN32

class UserPrefs
{
	typedef std::map<int64_t, tagUserPrefs*> UserPrefsMap;
public:
	UserPrefs(WORD roomID);
	~UserPrefs();

public:
    static UserPrefs& Instance()
    {
        // allocate item buffer now.
        static UserPrefs* sp = 0;
        if (!sp) {
             sp = new UserPrefs(0);
        }
    //Cleanup:
        return (*sp);
    }

public:
    void setConfigID(int nRoomID) {}
    void onUserSitdown(shared_ptr<CServerUserItem> serverUserItem, WORD roomID, DWORD ticketRatio, double exchangeRatio,
		DWORD maxGain, DWORD maxWinCoin, DWORD beginUserID);
    void onUserStandup(shared_ptr<CServerUserItem> serverUserItem, WORD roomID);
	void onUserFired(DWORD userID, WORD roomID, int fireScore);
	void onFishCatched(DWORD userID, WORD roomID, LONG score);
        tagUserPrefs* getUserPrefs(DWORD userID, WORD roomID);

protected:
	void updateGainLost(DWORD userID, WORD roomID, int fireScore, DWORD score);

	tagUserPrefs* allocUserPrefs();
	void freeUserPrefs(tagUserPrefs* userPrefs);
	int64_t getKeyOfUserPrefs(DWORD userID, WORD roomID);
	void addUserPrefs(tagUserPrefs* userPrefs);
	void removeUserPrefs(tagUserPrefs* userPrefs);
	tstring getCurrentTime();

protected:

#ifdef __MULTI_THREAD__
	CRITICAL_SECTION m_cs;
#endif//__MULTI_THREAD__

#ifdef  _WIN32
	ICAICSNewUserCheck* spNewuid;
#else
//	PlayerLogMDB  m_playerLogMDB;
    CCAICSNewUserCheck* spNewuid;
#endif//_WIN32

	ObjectAllocator<tagUserPrefs> m_storageUserPrefs;
	UserPrefsMap m_allUserPrefs;
};




