/*
	how to use:
	CHookFileChanged hook;
	hook.StartHook(file,dir);
	hook.isChanged();
*/

#ifndef __HOOKFILECHANGED_HEADER__
#define __HOOKFILECHANGED_HEADER__

#ifdef  _WIN32
#include <windows.h>
#endif//_WIN32

#include <types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <logger/Logtemp.h>

class CHookFileChanged
{
public:
    CHookFileChanged()
    {
        m_nLastWriteTime=0;
        m_szFile[0]='\0';
    }

    virtual ~CHookFileChanged()
    {
    }

public://¼à¿ØÄ¿Â¼
    LONG StartHook(LPCTSTR lpszFile,LPCTSTR lpszDir=NULL)
    {
        LONG nStatus = -1;
        if (lpszDir)
        {
            strncpy(m_szFile,lpszDir,MAX_PATH);
        }   else
        {
			LPCTSTR lpsz = CPathMagic::Current();
            strncpy(m_szFile,lpsz,MAX_PATH);
        }

        // append the path file now.
        strcat(m_szFile,TEXT("/"));
        strcat(m_szFile,lpszFile);
    //Cleanup:
        m_nLastWriteTime = GetLastChangedTime();
        return S_OK;
    }

    // check changed.
    BOOL isChanged()
    {
        BOOL bChanged = FALSE;
        long ft = {0};
        do 
        {
            ft = GetLastChangedTime();
            if (ft != m_nLastWriteTime)
            {
                // update last changed.
                m_nLastWriteTime = ft;
                bChanged = TRUE;
            }
        } while (0);
    //Cleanup:
        return (bChanged);
    }

protected:
    long GetLastChangedTime()
    {
        long ft=0;
		FILE* fp = fopen(m_szFile,"r");
		if (fp) {
			int fd = fileno(fp);
			struct stat buf;
			fstat(fd,&buf);
			ft = buf.st_mtime;
			fclose(fp);
		}
    //Cleanup:
        return (ft);
    }

protected:
    TCHAR m_szFile[MAX_PATH];
    long m_nLastWriteTime;
};




#endif//__HOOKFILECHANGED_HEADER__
