/*
    新实现的Log类,根据当前进程名打印日志信息,日志文件存在就打印,不存在就不打印.
	修正BUG: 支持中文打印.

History:
	2017-05-31 10:47
		写日志线程偶尔会退出,修改为不使用缓冲机制,看能否解决此问题.

	2015-09-17 14:24 修正了new出LogAsyncItem多线程报错的BUG.

	CLogAsync::Instance().start();
	CLogAsync::Instance().LogInfo("test.txt","hello Logasync");
	CLogAsync::Instance().Term();
*/

#ifndef __LOGASYNC_HEADER__
#define __LOGASYNC_HEADER__

#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/CriticalSection.h>
// #include <File/FStreamCache.h>
#include <logger/Logtemp.h>
#include <unistd.h>
#include <pthread.h>

#ifdef _WIN32
#include <locale.h>
#include <tchar.h>
#include <Shlwapi.h>
#pragma warning(disable:4996)

// link shalwapi.
#pragma comment(lib,"shlwapi.lib")

// 
// #include <Console/ConsoleDump.h>
// #include <Console/ConsoleTimecheck.h>
#endif//_WIN32

// one line max log content now.
#define MAX_LOGONELINE	(1024)

typedef struct tagLogAsyncItem
{
	TCHAR lpszFile[MAX_PATH];	// the file name.
	TCHAR lpszInfo[MAX_LOGONELINE];	// the info value.
}   LogAsyncItem;

typedef std::deque<LogAsyncItem>  QUELOGASYNC;
typedef QUELOGASYNC::iterator QUELOGASYNCITER;

class  CLogAsync;
extern CLogAsync* g_obLogAsync;
class  CLogAsync
{
public:
	CLogAsync()
	{
		m_nSleep = 10;
		CCriticalSectionEx::Create(&ctxfile);
		m_nOneTimecnt = 4096;
		m_hHookThread = 0;
		m_bHookThread = FALSE;
		m_bThread     = FALSE;
		m_bStarted    = FALSE;
		m_hThread     = 0;
		start();
	}

	virtual ~CLogAsync()

	{
		Term();
	}

	static CLogAsync& Instance()
	{
		static CLogAsync* pInst=NULL;
		if (!pInst) {
			 pInst  = new CLogAsync();
		}
	//Cleanup:
		return (*pInst);
	}

public:
	long start(long nSleep=500,long nOneTimecnt=1000000)
	{
		LONG nStatus=-1;
		m_bThread = TRUE;
		m_nSleep = nSleep;
		m_nOneTimecnt = nOneTimecnt;
		if (m_bStarted) {
			return S_OK;
		}

		CCriticalSectionEx::Create(&ctx);
		int err = pthread_create(&m_hThread,0,ThreadLoggerWriter,this);
		if (0 != err) {
                	CLogtemp log(TEXT("LogAsync.log"));
                	log.log(TEXT("pthread_create ThreadLoggerWriter error:[%d]."),err);
		}

		// start hook thread.
		m_bHookThread = TRUE;
		int err2 = pthread_create(&m_hHookThread,0,ThreadHookThread,this);
		if (0 != err2) {
                	CLogtemp log(TEXT("LogAsync.log"));
                	log.log(TEXT("_dowrite thread exit."));
		}

                CLogtemp log(TEXT("LogAsync.log"));
                log.log(TEXT("CLogAsync.start, create ThreadLoggerWrite return:[%d],ThreadHookThread return:[%d]"),err,err2);
		if ((0 == err) && (0 == err2)) {
			m_bStarted = TRUE;
			nStatus = S_OK;
		}

	//Cleanup:
		return (nStatus);
	}


	void Term()
	{
		m_bStarted = FALSE;
		m_bHookThread = FALSE;
		m_bThread = FALSE;
		if (m_hThread)
		{
			pthread_join(m_hThread,NULL);
			pthread_join(m_hHookThread,NULL);
			// close the handle now.
			m_hThread=0;
		}
	}

public:
	static void LogInfo(LPCTSTR lpszFile,LPCTSTR lpszFormat,...)
	{
		static long lasttick = 0;
		do 
		{
			va_list va;
			time_t now;
			timeval tv;
			struct tm* st = 0;
			TCHAR szFormat[1024]=TEXT("");
			TCHAR szResult[4096]=TEXT("");
			// build the list data.
			va_start(va,lpszFormat);
			vsnprintf(szFormat,sizeof(szFormat),lpszFormat,va);
			va_end(va);
			time(&now);
			gettimeofday(&tv,NULL);
			st = localtime(&now);
			int millisecond = ((tv.tv_sec*1000+tv.tv_usec/1000)/100000000);
			// build the full time string value time string value content item value content data now.
			snprintf(szResult,sizeof(szResult),_T("%02d/%02d %02d:%02d:%02d.%03d,pid=%d,tid=%d\t%s\n"),
				st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
				st->tm_sec,millisecond,(int)getpid(),
				(int)pthread_self(),szFormat);

			//// try to trace the special information content.
			CLogAsync::Instance().rawLog(lpszFile,szResult);
			if ((tv.tv_sec - lasttick) > 60) {
				lasttick = tv.tv_sec;
				CLogtemp log(TEXT("/var/log/LogAsync.log"));
				log.log_force(TEXT("trace lpszFile:[%s]"),lpszFile);
			}

		}   while (0);
	//Cleanup:
		return;
	}

	static void Logbin(LPCTSTR lpszFile,LPCTSTR lpszInfo,BYTE* pcbData,LONG nSize)
	{
		TCHAR szLine[1024]=TEXT("");
		TCHAR szItem[32]=TEXT("");
		LPTSTR lpszBuffer=NULL;

		do 
		{
			// initialize the data value now.
			if ((!pcbData)|| (!nSize)) break;
			if ((nSize>16384)) nSize=16384;
			// dump the information.
			if (strlen(lpszInfo)) {
				LogInfo(lpszFile,lpszInfo);
			}

			// try to allocate the special buffer item.
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
					szLine[0]='\t';
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
			CLogAsync::Instance().rawLog(lpszFile,lpszBuffer);
			free(lpszBuffer);
			lpszBuffer=NULL;
		}
	}




public:
	void rawLog(LPCTSTR lpszFile,LPCTSTR lpszInfo)
	{
		do 
		{
			// try to check the special file now.
			if ((!lpszFile)||(!lpszInfo)) break;
			CCriticalSectionEx lock;
			lock.Lock(&ctx);

			// add log item now.
			LogAsyncItem item;
			strncpy(item.lpszFile,lpszFile,MAX_PATH);
			strncpy(item.lpszInfo,lpszInfo,MAX_LOGONELINE);
			/// lock to insert data.
			veclist.push_back(item);
			lock.Unlock();
		}       while (0);
	//Cleanup:
		return;
	}


protected:
	long _dowrite()
	{
		long tickcount = 0;
		long nStatus   =-1;
		while (m_bThread)
		{
doWrite:
			QUELOGASYNC vecdump;
			// try to wait the lock.
			CCriticalSectionEx lock;
			{
				lock.Lock(&ctx);
				std::swap(vecdump,veclist);
//printf("vecdump size:[%d],veclist size:[%d]\n",vecdump.size(),veclist.size());
				// unlock data.
				lock.Unlock();
			}

			// loop to write now.
			if (vecdump.size())
			{
				QUELOGASYNCITER iter;
				for (iter=vecdump.begin();iter!=vecdump.end();iter++)
				{
					/// get the log info item.
					LogAsyncItem& item = *iter;
					if (item.lpszFile)
					{
						// try to open the special file and write.
						FILE* fp = fopen(item.lpszFile,TEXT("a"));
						if (fp) 
						{
							// try to puts the content.
							fputs(item.lpszInfo,fp);
							fclose(fp);
						}
					}
				}

				// delete all now.
				vecdump.clear();
			}

			// wait for next time.
			usleep(m_nSleep*1000);
			if (++tickcount >= 120) {
				tickcount= 0;
				CLogtemp log(TEXT("/var/log/LogAsync.log"));
				log.log_force(TEXT("ThreadLoggerWrite is alive.\n"));
			}
		}

		CLogtemp log(TEXT("LogAsync.log"));
		log.log(TEXT("_dowrite thread exit."));
		// check if break value.
		CCriticalSectionEx lock;
		lock.Lock(&ctx);
		size_t size = veclist.size();
		lock.Unlock();
		if (size)
		{
			// do the left all item.
			m_nOneTimecnt=(DWORD)0x7FFFFFFF;
			goto doWrite;
		}
	//Cleanup:
		return (nStatus);
	}

	DWORD _dohook()
	{
		long nStatus = -1;
		unsigned int count = 1200; // 10 minute.
		while (m_bHookThread)
		{
			if (0 == pthread_tryjoin_np(m_hThread,NULL))
			{
				int err = pthread_create(&m_hThread,0,ThreadLoggerWriter,this);
                                CLogtemp log(TEXT("LogAsync.log"));
                                log.log(TEXT("ThreadLoggerWriter has been crashed, now restart it,bThread:[%d],err:[%d]!"),m_bThread,err);
				if (0 == err) {
					nStatus = S_OK;
				}
			}

			// delay for thread now.
			usleep(500 * 1000);
			if (++count > 1200) {
			      	count = 0;
                		CLogtemp log(TEXT("LogAsync.log"));
				log.log(TEXT("ThreadHookThread alived.pid:[%d],tid:[%d]"),getpid(),pthread_self());
			}
		}
	//Cleanup:
		CLogtemp log(TEXT("LogAsync.log"));
		log.log(TEXT("_dohook exited."));
		return 0;
	}


protected: // thread to write all the log content item now.
	static void* ThreadLoggerWriter(LPVOID lParam)
	{
		CLogAsync* sp = (CLogAsync*)lParam;
		if (sp)
		{
			// write china logger.
// 			setlocale(LC_ALL ,"");
			sp->_dowrite();
		}
		//Cleanup:
		return (0);
	}

	// thread to hook the log writer thread now.
	static void* ThreadHookThread(LPVOID lParam)
	{
		CLogAsync* sp = (CLogAsync*)lParam;
		if (sp) {
			sp->_dohook();
		}
	//Cleanup:
		return (0);
	}

protected:
// 	CFStreamCache fspool;
	CRITICAL_SECTION  ctxfile;		// declare the critical section.
	unsigned long m_nOneTimecnt;	// one time update log count.
	unsigned long  m_nSleep;		// delay time for one loop.
	CRITICAL_SECTION    ctx;		// the critical section.
	QUELOGASYNC	    veclist;		// the vec list of log.
	pthread_t	  m_hThread;		// the main log write thread.
	BOOL		  m_bThread;		// the log write thread flag.
	pthread_t m_hHookThread;		// the hook thread item.
	BOOL	  m_bHookThread;		// the hook thread flag.
	BOOL	  m_bStarted;			// check if thread started.
};

#endif//__LOGASYNC_HEADER__
