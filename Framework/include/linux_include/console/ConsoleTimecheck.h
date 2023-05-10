/*
	how to use:
		CConsoleTimecheck::Instance().start();

		// check.
		CConsoleTimecheck::Instance().check("test");

*/

#ifndef __CONSOLETIMECHECK_HEADER__
#define __CONSOLETIMECHECK_HEADER__

#include <types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

class CConsoleTimecheck
{
public:
	CConsoleTimecheck()
	{
		start();
	}

	static CConsoleTimecheck& Instance()
	{
		static CConsoleTimecheck* sp=0;
		if (!sp) {
			 sp = new CConsoleTimecheck();
		}
	//Cleanup:
		return (*sp);
	}

public:
	long start(long miniSecond=50)
	{
		dwMini = miniSecond;
		dwLast = GetTickCount();

printf("dwLast:[%d]\n",dwLast);

		return (0);
	}

	void check(LPCSTR pszInfo)
	{
		DWORD dwSub = GetTickCount() - dwLast;
		if ((GetTickCount() - dwLast)> dwMini)
		{
			time_t now;
			struct tm* st = 0;
			CHAR szResult[4096]=TEXT("");
			
			// get time.
			time(&now);
			st = localtime(&now);
			int millisecond = GetTickCount();
			// build the full time string value time string value content item value content for later user content item now.
			snprintf(szResult,sizeof(szResult),("%02d/%02d %02d:%02d:%02d.%03d,pid=%d,tid=%d\t%s timeout, minSecond:[%d]\n"),
				st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
				st->tm_sec,millisecond,
				(int)getpid(),(int)pthread_self(),
				pszInfo,dwSub);
			
			// dump info now.
			printf(szResult);
		}
	//Cleanup:
		// get the tick count now.
		dwLast = GetTickCount();
	}

	DWORD GetTickCount()
	{
		timeval tv;
		gettimeofday(&tv,NULL);
		int64_t tmp = ((tv.tv_sec*1000)+(tv.tv_usec/1000));
		DWORD dw = (DWORD)(tmp%100000000);
	//Cleanup:
		return (dw);
	}


protected:
	DWORD dwLast;	// tick time.
	DWORD dwMini;	// mini second sub.
};




#endif//__CONSOLETIMECHECK_HEADER__
