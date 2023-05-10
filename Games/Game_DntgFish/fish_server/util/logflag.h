
#ifndef __LOGFLAG_HEADER__
#define __LOGFLAG_HEADER__

#include <windows.h>

class CLogflag
{
public:
    CLogflag() {
        m_bThread = FALSE;
        m_hThread = NULL;
        m_bisOn = FALSE;
    }

    ~CLogflag() {
        term();
    }

public:
    static CLogflag& Singleton()
    {
        static CLogflag obj;
        return obj;
    }

public: //读取哪个文件,多少间隔(以秒为单位).
    long init(LPCTSTR lpszFile=TEXT("logflags.txt"),DWORD dwInterval=300)
    {
        long nStatus = -1;
        DWORD dwThreadID=0;
        do 
        {
            if (m_hThread) {
                m_bThread = FALSE;
                if (WAIT_OBJECT_0 != WaitForSingleObject(m_hThread,100)) {
                    TerminateThread(m_hThread,0);
                }

                // close the handle now.
                CloseHandle(m_hThread);
                m_hThread=NULL;
            }

            m_bThread = TRUE;
            // try to store special file name.
            lstrcpyn(m_szFile,lpszFile,MAX_PATH);
            // try to start the special thread item.
            m_dwInterval = ((dwInterval * 10) + 10);
            m_hThread = CreateThread(NULL,0,ThreadSyncFlag,this,0,&dwThreadID);
            if (WAIT_OBJECT_0 != WaitForSingleObject(m_hThread,100)) {
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
        if (WAIT_OBJECT_0 != WaitForSingleObject(m_hThread,100)) {
            TerminateThread(m_hThread,0);
        }

        // close the handle now.
        CloseHandle(m_hThread);
        m_hThread=NULL;
    }

    // flag on.
    BOOL isOn()
    {
        return (m_bisOn != 0);
    }

public:
    DWORD _doLoop()
    {
        DWORD dwTick = 0;
        while (m_bThread)
        {
            if ((++dwTick) >= m_dwInterval) {
                LoadLogFlags();
                dwTick = 0;
            }

            Sleep(100);
        }
    //Cleanup:
        return (0);
    }

protected:
    static DWORD WINAPI ThreadSyncFlag(LPVOID lp)
    {
        CLogflag* pThis = (CLogflag*)lp;
        if (pThis) {
            pThis->_doLoop();
        }
    //Cleanup:
        return (0);
    }

    // load log flags.
    long LoadLogFlags()
    {
        long nStatus = -1;
        do 
        {
            // to read file.
            char buf[32]={0};
            FILE* fp = NULL;
            _tfopen_s(&fp,m_szFile,TEXT("r"));
            if (fp) {
                fread(buf,1,1,fp);
                fclose(fp);
            }

            // initialized.
            long bisOn = 0;
            if (lstrcmpiA(buf,"1")==0) {
                bisOn = 1;
            }

            // exchange the special flag item.
            InterlockedExchange(&m_bisOn,bisOn);
        } while (0);
    //Cleanup:
        return (nStatus);
    }

protected:
    TCHAR  m_szFile[MAX_PATH];
    DWORD  m_dwInterval;
    LONG   m_bisOn;
    BOOL   m_bThread;
    HANDLE m_hThread;
};



#endif//__LOGFLAG_HEADER__