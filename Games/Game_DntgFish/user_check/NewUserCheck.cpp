
#include "../fish_server/StdAfx.h"
#include "NewUserCheck.h"
//#include "TraceInfo.h"
//#include <atlstr.h>
//#include "WinInetCaller.h"
#include <ini/INIHelp.h>
#include <TextParserUtility.h>
#include <PathMagic.h>
#include <glog/logging.h>
#include <pthread.h>

// CTraceInfo _TraceLog;
#define UPDATE_INTERVAL 60

NewUserCheck::NewUserCheck()
    : m_pBlackListData(NULL)
    , m_thread(NULL)
{
	memset(&m_blackListInfo, 0, sizeof(m_blackListInfo));
	CCriticalSectionEx::Create(&m_csThis);
	CCriticalSectionEx::Create(&m_csAlloc);
	CCriticalSectionEx::Create(&m_csUserData);

	m_exitThread = false;
	TCHAR filePath[MAX_PATH];
    CPathMagic::GetModuleFileName(NULL, filePath, MAX_PATH);

	m_strExeFilePath = filePath;
    int pos = m_strExeFilePath.find_last_of(_T("/"));
    if (pos > 0)
	{
		m_strExeFilePath = m_strExeFilePath.substr(0, pos + 1);
	}

    // try to initialize the path.
    string temp = m_strExeFilePath;
	temp.append(_T("AICSNewUserCheck.log"));
    LOG(WARNING) << "服务[AICSNewUserCheck]开始加载黑名单设置。";
	if (loadBlackListInfo())
	{
        int err = pthread_create(&m_thread, 0, AgentThread, this);
	}
//Cleanup:
    LOG(WARNING) << "服务[AICSNewUserCheck]启动完成。";
}

NewUserCheck::~NewUserCheck()
{
	m_exitThread = true;
    // WaitForSingleObject(m_thread, INFINITE);
    // CloseHandle(m_thread);
    pthread_join(m_thread,NULL);
	CCriticalSectionEx::Destroy(&m_csAlloc);
	CCriticalSectionEx::Destroy(&m_csUserData);
	CCriticalSectionEx::Destroy(&m_csThis);
	if (m_pBlackListData)
	{
		delete m_pBlackListData;
		m_pBlackListData = NULL;
	}

    LOG(WARNING) << "服务[AICSNewUserCheck]停止。";
}

KEY_OF_USER_DATA NewUserCheck::getKeyWithUserIDAndRoomID(DWORD userID, WORD roomID)
{
	KEY_OF_USER_DATA key = roomID;
	key <<= 32;
	key |= userID;
	return key;
}

bool NewUserCheck::loadBlackListInfo()
{
	CINIHelp iniHelp;
    string temp = m_strExeFilePath;
	temp.append(INI_FILENAME);
	iniHelp.Open(temp.c_str());

    // try to check the special interval content item from the content item buffer content.
	m_blackListInfo.dwCheckInterval = iniHelp.GetLong(INI_SECTION_CONFIG, INI_KEY_INTERVAL);
	if (!m_blackListInfo.dwCheckInterval)
	{
		m_blackListInfo.dwCheckInterval = 300;
	}

	m_blackListInfo.dwWriteStartNumber = iniHelp.GetLong(INI_SECTION_CONFIG, INI_KEY_START_NUMBER);
	if (!m_blackListInfo.dwWriteStartNumber)
	{
		m_blackListInfo.dwWriteStartNumber = 100;
	}

    int i;
    LOG(WARNING) << "起始编号:" << m_blackListInfo.dwWriteStartNumber << ",检查频率(秒):" << m_blackListInfo.dwCheckInterval;
    LPCTSTR cond = iniHelp.GetString(INI_SECTION_CONFIG, INI_KEY_CONDITION);
	CTextParserUtility txtParser, txtParser1;
    txtParser.Parse(cond, ";");

    //  loop to parser the special content.
	for (i = 0; i < txtParser.Count(); ++i)
	{
		txtParser1.Parse(txtParser.getStringAt(i), ",");
		if (txtParser1.Count() > 1)
		{
			int nWinCoin, nRecharge;
			nRecharge = atoi(txtParser1.getStringAt(0));
			nWinCoin = atoi(txtParser1.getStringAt(1));
			if (nRecharge > 0 && nWinCoin > 0)
			{
				WinCoinInfo &info = m_blackListInfo.wci[m_blackListInfo.dwConditionCount++];
				info.dWinCoin = nWinCoin;
				info.dRecharge = nRecharge;
                LOG(WARNING) << "拉黑条件:" << m_blackListInfo.dwConditionCount << "：充值数:" << info.dRecharge << "赢利数:" << info.dWinCoin;
			}
		}
	}

	m_pBlackListData = new CBlackListData(m_blackListInfo.dwWriteStartNumber);
	return m_blackListInfo.dwConditionCount > 0;
}

// try to get the special agent thread content.
LPVOID NewUserCheck::AgentThread(LPVOID lparam)
{
	NewUserCheck* sp = static_cast<NewUserCheck*>(lparam);
	if (sp)
	{
		DWORD dwCounter = 0;
		CCriticalSectionEx lock;
		while (true)
		{
            ++dwCounter;
            usleep(1000 * 1000);
			if (dwCounter >= sp->m_blackListInfo.dwCheckInterval || sp->m_exitThread)
			{
				dwCounter = 0;
				if (sp->m_userDataMap.size())
				{
					if (lock.TryLock(&sp->m_csUserData))
					{
						UserDataMap userData = sp->m_userDataMap;
						lock.Unlock();
						BlackListInfo& blackListInfo = sp->m_blackListInfo;
                        LOG(WARNING) << " >>> 拉黑检测开始.";
						for (UserDataMap::iterator iter = userData.begin(); iter != userData.end(); ++iter)
						{
							tagUserData* p = iter->second;
							WinCoinInfo& info = p->wci;
							bool bSetBlack = false;
							for (DWORD i = 0; i < blackListInfo.dwConditionCount; ++i)
							{
                                if ((info.dRecharge <= blackListInfo.wci[i].dRecharge) &&
                                    (info.dWinCoin  >= blackListInfo.wci[i].dWinCoin))
								{
                                    LOG(WARNING) << "拉黑用户:" << p->userID     << "充值数:" << info.dRecharge
                                                 << "赢利数:"   << info.dWinCoin << "满足拉黑条件."
                                                 << "充值数:"   << blackListInfo.wci[i].dRecharge
                                                 << "赢利数:"   << blackListInfo.wci[i].dWinCoin;

									bSetBlack = true;
									break;
								}
							}
							if (bSetBlack)
							{
								sp->m_pBlackListData->AddUser(p->userID);
							}
							else
							{
								sp->m_pBlackListData->RemoveUser(p->userID);
							}
						}

						sp->m_pBlackListData->StartWork();
                        LOG(WARNING) << " <<< 拉黑检查结束.";
					}
				}
			}
			if (sp->m_exitThread)
			{
				break;
			}
		}
	}
	return 0;
}

tagUserData* NewUserCheck::allocUserData()
{
	CCriticalSectionEx lock;
	lock.Lock(&m_csAlloc);
	tagUserData* userData = m_userDataAllocator.alloc();
	if (userData) {
		memset(userData, 0, sizeof(tagUserData));
	}
	return userData;
}

void NewUserCheck::freeUserData(tagUserData* userData)
{
	CCriticalSectionEx lock;
	lock.Lock(&m_csAlloc);
	m_userDataAllocator.free(userData);
}

tagUserData* NewUserCheck::getUserData(KEY_OF_USER_DATA key)
{
	CCriticalSectionEx lock;
	lock.Lock(&m_csUserData);
	UserDataMap::iterator iter = m_userDataMap.find(key);
	if (iter != m_userDataMap.end())
	{
		return iter->second;
	}
	return NULL;
}

tagUserData* NewUserCheck::getUserData(DWORD userID, WORD roomID)
{
	return getUserData(getKeyWithUserIDAndRoomID(userID, roomID));
}

bool NewUserCheck::removeUserData(DWORD userID, WORD roomID, bool free)
{
	CCriticalSectionEx lock;
	KEY_OF_USER_DATA key = getKeyWithUserIDAndRoomID(userID, roomID);
	tagUserData* userData = getUserData(key);
	lock.Lock(&m_csUserData);
	m_userDataMap.erase(key);
	if (free)
	{
		freeUserData(userData);
	}
	return true;
}

bool NewUserCheck::addUserData(tagUserData* userData)
{
	if (!userData) return false;
	KEY_OF_USER_DATA key = getKeyWithUserIDAndRoomID(userData->userID, userData->roomID);
	if (!getUserData(key))
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csUserData);
		m_userDataMap[key] = userData;
		return true;
	}
	return false;
}

int NewUserCheck::updateGainLost(DWORD userID, WORD roomID, double gain, double lost)
{
	tagUserData* p = getUserData(userID, roomID);
	p->wci.dWinCoin += lost - gain;
	return S_OK;
}
