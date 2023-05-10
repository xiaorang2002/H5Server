/*
    异步日志开关类,可以通过配置动态打开关闭日志项.
    CLogflags::Singleton().init(TEXT("logflags.txt"),10);
    if (CLogflags::Singleton().isOn()) {
        printf("log is on!\n");
    }
*/

#ifndef __LOGFLAGS_HEADER__
#define __LOGFLAGS_HEADER__

#include "../types.h"
#include <pthread.h>
#include "../PathMagic.h"
#include <unistd.h>

#ifdef  _WIN32
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#endif//_WIN32

class CLogflags
{
public:
    CLogflags() {
        m_bThread = FALSE;
        m_hThread = 0;
        m_bisOn = FALSE;
    }

    ~CLogflags() {
        term();
    }

public:
    static CLogflags& Singleton()
    {
        static CLogflags obj;
        return obj;
    }

public:
    int32 init()
    {
	TCHAR szPath[MAX_PATH]=TEXT("");
        TCHAR szFile[MAX_PATH]=TEXT("");
        CPathMagic::GetModuleFileName(NULL,szFile,MAX_PATH);
        LPTSTR lpszFile = (LPTSTR)CPathMagic::PathFindFileName(szFile);
        if (strlen(lpszFile))
        {
            CPathMagic::PathRemoveExtension(lpszFile);
            strcat(lpszFile,TEXT(".txt"));
            strncpy(m_szFile,lpszFile,MAX_PATH);
        }   else
        {
            // try to get the default file name of flag file.
            strncpy(m_szFile,TEXT("logflags.txt"),MAX_PATH);
        }

	printf("CLogflags default file name:[%s]\n",m_szFile);
        // initialize the operation.
        return (init(m_szFile,300));
    }

public: //读取哪个文件,多少间隔(以秒为单位).
    int32 init(LPCTSTR lpszFile,DWORD dwInterval=300)
    {
        int32 nStatus = -1;
	TCHAR szPath[MAX_PATH]=TEXT("");
	CPathMagic::GetModuleFileName(NULL,szPath,MAX_PATH);
	CPathMagic::PathRemoveFileSpec(szPath);
        DWORD dwThreadID=0;
        do 
        {
            if (m_hThread) {
                m_bThread = FALSE;
		pthread_join(m_hThread,NULL);
                // close the handle now.
                m_hThread=0;
            }

            m_bThread = TRUE;
	    lstrcat(szPath,lpszFile);
	    printf("CLogflags init file name:[%s]\n",szPath);
            // try to store special file name.
            lstrcpyn(m_szFile,szPath,MAX_PATH);
            // try to start the special thread item.
            m_dwInterval = ((dwInterval * 10) + 10);
            int err = pthread_create(&m_hThread,0,ThreadSyncFlag,this);
            if (0 == err) {
		LoadLogFlags();
                nStatus = S_OK;
            }
        } while (0);
    //Cleanup:
        return (nStatus);
    }

    //terminate.
    void term()
    {
        m_bThread = FALSE;
	if (m_hThread)
	{
		// wait for thread to exit now.
		pthread_join(m_hThread,NULL);
        	// close the handle now.
        	m_hThread=0;
	}
    }

    // flag on.
    BOOL isOn()
    {
        return (m_bisOn != 0);
    }

public:
    DWORD _doLoop()
    {
        DWORD dwTick = 100000000;   // modify by James 180119-修正首次读取.
        while (m_bThread)
        {
            if ((++dwTick) >= m_dwInterval) {
                LoadLogFlags();
                dwTick = 0;
            }

            usleep(100000);
        }
    //Cleanup:
        return (0);
    }

protected:
    static void* ThreadSyncFlag(LPVOID lp)
    {
        CLogflags* pThis = (CLogflags*)lp;
        if (pThis) {
            pThis->_doLoop();
        }
    //Cleanup:
        return (0);
    }

    // load log flags.
    int32 LoadLogFlags()
    {
        int32 nStatus = -1;
	static bool bDumped = false;
        do 
        {
            // to read file.
            char buf[32]={0};
            FILE* fp=fopen(m_szFile,TEXT("r"));
            if (fp) {
                fread(buf,1,1,fp);
                fclose(fp);
            }

            // initialize item value.
            if (strcmp(buf,"1")==0) {
		if (!bDumped) {
		    printf("CLogflags is on!\n");
		    bDumped = true;
		}

               m_bisOn = 1;
            }

        } while (0);
    //Cleanup:
        return (nStatus);
    }

protected:
    TCHAR  	m_szFile[MAX_PATH];
    DWORD  	m_dwInterval;
    LONG   	m_bisOn;
    BOOL   	m_bThread;
    pthread_t	m_hThread;
};



#endif//__LOGFLAGS_HEADER__
