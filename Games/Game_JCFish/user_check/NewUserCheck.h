// NewUserCheck.h : Declaration of the NewUserCheck

#ifndef __NEW_USER_CHECK_H__
#define __NEW_USER_CHECK_H__

#include <pthread.h>
#include <object/ObjectAllocator.h>
#include <kernel/CriticalSection.h>
#include <xml/XmlParser.h>
#include "BlackListData.h"
#include <map>

typedef struct tagWinCoinInfo {
	double dWinCoin;
	double dRecharge;
} WinCoinInfo;

#define MAX_BLACK_LIST_CONDITION 20
typedef struct tagBlackListInfo {
	DWORD dwCheckInterval;		// 黑名单检查频率
	DWORD dwWriteStartNumber;	// 黑名单检查起始编号
	DWORD dwConditionCount;		// 黑名单条件个数
	tagWinCoinInfo wci[MAX_BLACK_LIST_CONDITION]; // 黑名单条件列表
} BlackListInfo;

struct tagUserGainLost
{
	double gain; // 总进(渔币)
	double lost; // 总进(渔币)
};

struct tagUserData
{
	DWORD userID;
	WORD roomID;
	tagUserGainLost ugl;
	tagWinCoinInfo wci;
};

struct UserNameInfo
{
	typedef std::vector<DWORD> USER_ID_VECTOR;
public:
	UserNameInfo& operator= (const UserNameInfo& other)
	{
		vec = other.vec;
		return *this;
	}
	void reset()
	{
		vec.clear();
	}
	USER_ID_VECTOR vec;
};

typedef __int64 KEY_OF_USER_DATA;
typedef std::map<KEY_OF_USER_DATA, tagUserData*> UserDataMap;
typedef std::map<KEY_OF_USER_DATA, tagUserData> UserDataMap2;
/////////////////////////////////////////////////////////////////////////////
// NewUserCheck
class NewUserCheck
{
public:
	NewUserCheck();
	~NewUserCheck();

public:
    static NewUserCheck* Instance()
    {
        static NewUserCheck* sp = 0;
        if (!sp) {
             sp = new NewUserCheck();
        }
    //Cleanup:
        return (sp);
    }

public:
	bool addUserData(tagUserData* userData);
	bool removeUserData(DWORD userID, WORD roomID, bool free);
	tagUserData* allocUserData();
	void freeUserData(tagUserData* userData);
	tagUserData* getUserData(KEY_OF_USER_DATA key);
	tagUserData* getUserData(DWORD userID, WORD roomID);
    int updateGainLost(DWORD userID, WORD roomID, double gain, double lost);

public:
	CRITICAL_SECTION m_csThis;

protected:
	bool loadBlackListInfo();
    static LPVOID AgentThread(LPVOID lparam);
	KEY_OF_USER_DATA getKeyWithUserIDAndRoomID(DWORD userID, WORD roomID);

protected:
    ObjectAllocator<tagUserData> m_userDataAllocator;
	CRITICAL_SECTION m_csAlloc, m_csUserData;
    UserDataMap m_userDataMap;
    // HANDLE m_thread;
    pthread_t m_thread;
    bool  m_exitThread;
	BlackListInfo m_blackListInfo;
	CBlackListData* m_pBlackListData;
    string m_strExeFilePath;
};

#endif //__NEW_USER_CHECK_H__
