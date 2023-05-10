#pragma once

#ifdef _WIN32
#else
#include <pthread.h>
#define StrToInt(a)    strtod(a,NULL)
#endif//_WIN32

#include <ini/INIHelp.h>
#include <kernel/CriticalSection.h>
#include <kernel/EventObject.h>
#include <glog/logging.h>
#include <map>

#define INI_FILENAME            _T("StaticBlackList.ini")
#define INI_SECTION_RUNNING     _T("Running")
#define INI_SECTION_CONFIG      _T("Config")
#define INI_SECTION_BLACK_LIST  _T("BlackList")
#define INI_KEY_CONDITION       _T("Condition")
#define INI_KEY_INTERVAL        _T("Interval")
#define INI_KEY_CURRENT_NUMBER  _T("CurrentNumber")
#define INI_KEY_START_NUMBER    _T("StartNumber")

using namespace std;
class CBlackListData
{
public:
        CBlackListData(DWORD dwStartNumber)
            : m_dwStartNumber(dwStartNumber)
            , m_dwEndNumber(0)
//          , m_hEvent(NULL)
//          , m_hThread(NULL)
            , m_bExitThread(false)
	{
		CCriticalSectionEx::Create(&m_csMap);
		LoadBlackListData();

                m_hEvent.Init(FALSE, FALSE);
                // m_hEvent  = CreateEvent(NULL, FALSE, FALSE, NULL);
                // m_hThread = CreateThread(NULL, 0, AgentThread, this, 0, NULL);
                int err = pthread_create(&m_hThread,0,AgentThread,this);
	}

	~CBlackListData()
	{
		m_bExitThread = true;
                // SetEvent(m_hEvent);
                // WaitForSingleObject(m_hThread, INFINITE);
                // CloseHandle(m_hThread);
                // CloseHandle(m_hEvent);
                m_hEvent.SetEvent();
                pthread_join(m_hThread,NULL);
        //Cleanup:
		CCriticalSectionEx::Destroy(&m_csMap);
	}

public:
	void AddUser(DWORD dwUserID)
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csMap);
		m_mapUsed[dwUserID] = 1;
		if (m_mapUsed.size() > m_dwEndNumber - m_dwStartNumber)
		{
			++m_dwEndNumber;
		}
	}

	void RemoveUser(DWORD dwUserID)
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csMap);
		m_mapUsed.erase(dwUserID);
	}

	void StartWork()
	{
            // SetEvent(m_hEvent);
            m_hEvent.SetEvent();
	}

protected:
	void LoadBlackListData()
	{
		CINIHelp iniHelp;
		iniHelp.Open(INI_FILENAME);
		LPCTSTR s = iniHelp.GetString(INI_SECTION_RUNNING, INI_KEY_CURRENT_NUMBER);
		m_dwEndNumber = StrToInt(s);
		if (!m_dwEndNumber)
		{
			m_dwEndNumber = m_dwStartNumber;
		}
		TCHAR szNumber[50];
		LONG nUserID;
		DWORD i;
		for (i = 1; i < m_dwEndNumber; ++i)
		{
                        snprintf(szNumber,sizeof(szNumber),_T("%d"), i);
			nUserID = iniHelp.GetLong(INI_SECTION_BLACK_LIST, szNumber, -1);
			if (nUserID == -1)
			{
				iniHelp.SetLong(INI_SECTION_BLACK_LIST, szNumber, 0);
			}
		}
		for (i = m_dwStartNumber; i < m_dwEndNumber; ++i)
		{
                        snprintf(szNumber,sizeof(szNumber),_T("%d"), i);
			nUserID = iniHelp.GetLong(INI_SECTION_BLACK_LIST, szNumber, 0);
			if (nUserID > 0)
			{
				m_mapUsed[nUserID] = 1;
			}
		}
	}

	void WriteBlackListData(DWORD dwStartNum, DWORD dwEndNum, const map<int,int>& mapUsed)
	{
		CINIHelp iniHelp;
		iniHelp.Open(INI_FILENAME);
		iniHelp.SetLong(INI_SECTION_RUNNING, INI_KEY_CURRENT_NUMBER, m_dwEndNumber);
		TCHAR szNumber[50];
		LONG nUserID;
		DWORD dwCurrentNum = dwStartNum;
		DWORD i;
		map<int,int>::const_iterator iter = mapUsed.begin();
		while(iter != mapUsed.end())
		{
            snprintf(szNumber,sizeof(szNumber),_T("%d"), dwCurrentNum);
            nUserID = iniHelp.SetLong(INI_SECTION_BLACK_LIST, szNumber, iter->first);
            ++dwCurrentNum;
            ++iter;
		}

        // try to initialize the content item now.
        for (i = dwCurrentNum; i < dwEndNum; ++i)
        {
            snprintf(szNumber,sizeof(szNumber),_T("%d"), i);
            nUserID = iniHelp.SetLong(INI_SECTION_BLACK_LIST, szNumber, 0);
		}
	}

protected:
        static void* AgentThread(LPVOID lparam)
	{
		CBlackListData* sp = static_cast<CBlackListData*>(lparam);
                if (sp != NULL)
		{
			CCriticalSectionEx lock;
			while(!sp->m_bExitThread)
			{
                            sp->m_hEvent.WaitEvent(INFINITE);
                            // WaitForSingleObject(sp->m_hEvent, INFINITE);
                            if (lock.TryLock(&sp->m_csMap))
                            {
                                // get the special map for used content now.
                                map<int, int> userData = sp->m_mapUsed;
                                lock.Unlock();

                                std::string strUserList = "";
                                map<int,int>::iterator iter;
                                for (iter=userData.begin();iter!=userData.end();iter++)
                                {
                                    // try to build the special string list now.
                                    strUserList += to_string(iter->first) + ",";
                                }

                                // try to write the special black list content item data value content now.
                                sp->WriteBlackListData(sp->m_dwStartNumber, sp->m_dwEndNumber, userData);
                                LOG(WARNING) << "执行拉黑, dwStartNumber:" << sp->m_dwStartNumber
                                             << ", dwEndNumber:" << sp->m_dwEndNumber
                                             << ", 拉黑玩家列表:"  << strUserList;
                            }
			}
		}
		return 0;
	}

protected:
        DWORD m_dwStartNumber;
        DWORD m_dwEndNumber;
	map<int,int> m_mapUsed;
	CRITICAL_SECTION m_csMap;
        // HANDLE m_hThread, m_hEvent;
        pthread_t    m_hThread;
        CEventObject m_hEvent;
	bool m_bExitThread;
};
