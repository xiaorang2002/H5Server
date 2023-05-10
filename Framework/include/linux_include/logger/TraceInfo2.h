/*
    新实现的Log类,根据当前进程名打印日志信息,日志文件存在就打印,不存在就不打印.
	修正BUG: 支持中文打印.

	定义 DISABLE_TRACE 屏蔽日志.
*/

#ifndef  __TRACEINFOLOG2_HEADER__
#define  __TRACEINFOLOG2_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../PathMagic.h"
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include "Logflags.h"

#ifdef  DISABLE_TRACE
#define TRACE_LOG_BEGIN
#define TRACE_LOG
#define TRACE_HEX
#define TRACE_LOG_END

#else

#ifndef TRACE_LOG_BEGIN
    #define TRACE_LOG_BEGIN	CTraceInfo2::Instance()
    #define TRACE_LOG		CTraceInfo2::Instance()->Trace
    #define TRACE_HEX		CTraceInfo2::Instance()->TraceBin
    #define TRACE_LOG_END	CTraceInfo2::Instance()->Term()
    #endif//TRACE_LOG_BEGIN
#endif//

#include <stdio.h>
#include "../kernel/CriticalSection.h"
#include <deque>

typedef std::deque<TCHAR*> DEQLOG;
typedef DEQLOG::iterator DEQLOGITER;

class  CTraceInfo2
{
public:
	CTraceInfo2(const char* lpszFile=NULL)
	{
        m_szLogFile[0]='\0';
        CCriticalSectionEx::Create(&ctxrd);
        CCriticalSectionEx::Create(&ctxwr);
		m_hDeamonThread = 0;
		m_bDeamonThread = FALSE;
		m_nOneTimecnt =4096;
		m_bExited = FALSE;
		m_bThread = FALSE;
		m_hThread = 0;
		m_nSleep  = 10;
		start(lpszFile);
	}

	virtual ~CTraceInfo2()
	{
		Term();
	}

	static CTraceInfo2* Instance(const char* lpszFile=NULL)
	{
        static CTraceInfo2* pInst=NULL;
		if (!pInst) {
			 pInst  = new CTraceInfo2(lpszFile);
		}
	//Cleanup:
		return (pInst);
	}

public:
	long start(LPCTSTR lpszFile=NULL,long nSleep=100,long nOneTimecnt=100000)
	{
		LONG nStatus=-1;
		m_bExited = 0;
		m_bThread = TRUE;
		m_nSleep = nSleep;
		m_nOneTimecnt = nOneTimecnt;
		CPathMagic::GetModuleFileName(NULL,m_szLogFile,MAX_PATH);
		CPathMagic::PathRenameExtension(m_szLogFile,_T(".log"));
		if (lpszFile) strncpy(m_szLogFile,lpszFile,MAX_PATH);
		int ret = pthread_create(&m_hThread,0,ThreadLoggerWriter,this);
		if (0 == ret) {
			nStatus = S_OK;
		}

		// deamon thread start.
		m_bDeamonThread = TRUE;
		ret = pthread_create(&m_hDeamonThread,0,ThreadLoggerDemaon,this);
		if (0 == ret) {
			nStatus = S_OK;
		}
	//Cleanup:
		return (nStatus);
	}

	void Term()
	{
		/// initialize the data.
		m_bDeamonThread = FALSE;
		if (m_hDeamonThread)
		{
			// to wait thread terminate now.
			pthread_join(m_hDeamonThread,0);
			m_hDeamonThread = 0;
		}

		// initialize item.
		m_bThread = FALSE;
		if (m_hThread)
		{
			// wait for thread value.
			pthread_join(m_hThread,0);
			// close the handle now.
			m_hThread = 0;
		}

        //// try to destroy the object item.
        CCriticalSectionEx::Destroy(&ctxrd);
        CCriticalSectionEx::Destroy(&ctxwr);
	}

public:
	static void Trace(LPCTSTR lpszFormat,...)
	{
		do 
		{
			if (!CLogflags::Singleton().isOn()) return;

			va_list va;
			time_t now;
			timeval tv;
			struct tm *st = 0;
			TCHAR szFormat[1024]=TEXT("");
			TCHAR szResult[4096]=TEXT("");
			// build the list data.
			va_start(va,lpszFormat);
			vsnprintf(szFormat,sizeof(szFormat),lpszFormat,va);
			va_end(va);

			time(&now);
			gettimeofday(&tv,NULL);
			st = localtime(&now);
			int millisecond = (((tv.tv_sec*1000)+(tv.tv_usec/1000))/100000000);
			// build the full time string value time string value content item value content data now.
			snprintf(szResult,sizeof(szResult),_T("%02d/%02d %02d:%02d:%02d.%03d,pid=%d,tid=%d\t%s\n"),
				st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
				st->tm_sec,millisecond,(int)getpid(),
				(int)pthread_self(),
				szFormat);

			// try to trace the special information now.
			CTraceInfo2::Instance()->mytrace(szResult);
		}   while (0);
	//Cleanup:
		return;
	}

	static void TraceBin(BYTE* pcbData,LONG nSize)
	{
		TCHAR szLine[1024]=TEXT("");
		TCHAR szItem[32]=TEXT("");
		LPTSTR lpszBuffer=NULL;

		do 
		{
			// initialize the data value now.
			if ((!pcbData)|| (!nSize)) break;
			if ((nSize>16384)) nSize=16384;


			LONG nBufSize = ((nSize<<2) * sizeof(TCHAR));
			lpszBuffer = (LPTSTR)malloc(nBufSize);
			if (!lpszBuffer) break;
			// initialize now.
			lpszBuffer[0]='\0';
			// loop to dump all content.
			for (int i=0;i<nSize;i++) {
				snprintf(szItem,sizeof(szItem),TEXT("%02X "),pcbData[i]);
				strcat(szLine,szItem);
				if ((i%16)==15) {
					strcat(lpszBuffer,szLine);
					strcat(lpszBuffer,TEXT("\n"));
					szLine[0]='\0';
				}
			}

			/// dump the last item.
			if (strlen(szLine)) {
				strcat(lpszBuffer,szLine);
				strcat(lpszBuffer,TEXT("\n"));
			}
		} while (0);
	//Cleanup:
		if (lpszBuffer) {
			CTraceInfo2::Instance()->mytrace(lpszBuffer);
			free(lpszBuffer);
			lpszBuffer=NULL;
		}
	}

public:
	void mytrace(LPCTSTR lpszInfo)
	{
		do 
		{
			/// lock to insert data.
			CCriticalSectionEx lock;
			lock.Lock(&ctxrd);
			/// try to dump the string value.
			LPTSTR lpsz = strdup(lpszInfo);
			deqlist.push_back(lpsz);
			lock.Unlock();
		}   while (0);
	//Cleanup:
		return;
	}


protected:
	long _dowrite()
	{
		long nStatus =-1;
		// write china logger.
		while (m_bThread)
		{
doWrite:
			// lock the special now.
			CCriticalSectionEx lock;
			if (lock.TryLock(&ctxrd))
            {
				// loop to get the from now 
                DEQLOG deqdump = deqlist;
                deqlist.clear();
				// unlock data.
				lock.Unlock();
                lock.Lock(&ctxwr);
			    /// loop to write now.
			    while (deqdump.size())
			    {
				    // open the special file to write item.
				    FILE* fp = fopen(m_szLogFile,_T("a"));
				    if   (fp)
				    {
					    DEQLOGITER iter;
					    for (iter=deqdump.begin();iter!=deqdump.end();iter++)
					    {
						    TCHAR* ptr = *iter;
						    if (ptr)
						    {
							    // puts strings.
							    fputs(ptr,fp);
							    free(ptr);
						    }
					    }

					    //// delete all.
					    deqdump.clear();
					    fflush(fp);
					    fclose(fp);
				    }  else
				    {
					    /// wait now.
					    usleep(1000);
				    }
			    } // end while size.
                lock.Unlock();
			} // endif.

			//wait for next timer.
			usleep(m_nSleep*1000);
		}

		// check if break value.
		CCriticalSectionEx lock;
		lock.Lock(&ctxrd);
		size_t size = deqlist.size();
		lock.Unlock();
		if (size>0)
		{
			// do the left all item for time.
			m_nOneTimecnt=(DWORD)0x7FFFFFFF;
			goto doWrite;
		}
	//Cleanup:
		m_bExited=1;
		return (nStatus);
	}

	// wait for thread.
	void _dodeamon()
	{
		while (m_bDeamonThread)
		{
			int ret = pthread_tryjoin_np(m_hThread,NULL);
			if ((0 == ret) && (m_bThread)) {
				pthread_create(&m_hThread,0,ThreadLoggerWriter,this);
			}

			usleep(500000);
		}
	}

protected:
	static void* ThreadLoggerDemaon(void* args)
	{
		CTraceInfo2* sp = (CTraceInfo2*)args;
		if (sp) {
			sp->_dodeamon();
		}
	//Cleanup:
		return (0);
	}

protected:
	static void* ThreadLoggerWriter(void* args)
	{
		CTraceInfo2* sp = (CTraceInfo2*)args;
		if (sp) {
			sp->_dowrite();
		}
	//Cleanup:
		return (0);
	}

protected:
	static LPCTSTR GetLogFileName(LPTSTR buf)
	{
		CPathMagic::GetModuleFileName(NULL,buf,MAX_PATH);
		// try to get the log file content now.
		CPathMagic::PathRenameExtension(buf,_T(".log"));
	//Cleanup:
		return (buf);
	}

protected:
	TCHAR m_szLogFile[MAX_PATH];
	unsigned long m_nOneTimecnt;
	unsigned long m_nSleep;		// delay time for one loop.
	pthread_t m_hDeamonThread;	// thread handle.
	BOOL   m_bDeamonThread;		// is deamon now.
	CRITICAL_SECTION ctxrd; 	// read ctx.
    CRITICAL_SECTION ctxwr; 	// write ctx.
	DEQLOG    	   deqlist;		// the vec list of log.
	LONG      	 m_bExited;		// the flag for exited.
	pthread_t 	 m_hThread;		// the thread handle.
	BOOL     	 m_bThread;		// thread has exited.
};


#endif// __TRACEINFOLOG2_HEADER__
