/*
	针对玩法等不太好打日志的地方,单独出来的打日志类.

	how to use:
		CLogtemp log;
		log.log("hello");

*/

#ifndef __LOGTEMP_HEADER__
#define __LOGTEMP_HEADER__

#include <stdio.h>
#include <unistd.h>
#include <PathMagic.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <logger/Logflags.h>

// log temp now.
class CLogtemp
{
public:
	CLogtemp(LPCTSTR lpszFile=NULL,HINSTANCE hInst=NULL)
	{
		if (lpszFile) 
		{
			strncpy(szFile,lpszFile,sizeof(szFile));
		}   
		else
		{
			TCHAR szTemp[MAX_PATH]=TEXT("");
			CPathMagic::GetModuleFileName(hInst,szTemp,MAX_PATH);
			CPathMagic::PathRemoveExtension(szTemp);
			strcat(szTemp,TEXT(".log"));
			strncpy(szFile,szTemp,sizeof(szFile));
		}
	}

	// add CLogflags support for log.
	void log(LPCTSTR lpszFormat,...)
	{
		if (!CLogflags::Singleton().isOn()) return;

		va_list va;
		time_t now;
		timeval tv;
		struct tm* st = 0;
		TCHAR sz[1024]=TEXT("");
		TCHAR szTime[1024]=TEXT("");
		TCHAR szTitle[1024]=TEXT("");

		FILE* fp = fopen(szFile,TEXT("a"));
		if (!fp) return;
		
		va_start(va,lpszFormat);
		vsnprintf(sz,sizeof(sz),lpszFormat,va);
		va_end(va);
		
		time(&now);
		gettimeofday(&tv,NULL);
		st = localtime(&now);
		int millisecond = tv.tv_usec/1000;
		// build the full time string value time string now.
		snprintf(szTime,sizeof(szTime),_T("%02d/%02d %02d:%02d:%02d.%03d"),
                 st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
                 st->tm_sec,millisecond);
		
		// puts string now.
		fputs(szTime,fp);
		
		// dump the content of current process item content value data now.
		snprintf(szTitle,sizeof(szTitle),_T(",tick=%d,pid=%d,tid=%d, "),
			tv.tv_usec,(int)getpid(),
			(int)pthread_self());

		// write the file now.
		fputs(szTitle,fp);
		fputs(sz,fp);
		fputs(TEXT("\n"),fp);
		fclose(fp);
	}

        // add CLogflags support for log.
        void log_force(LPCTSTR lpszFormat,...)
        {
                va_list va;
                time_t now;
                timeval tv;
                struct tm* st = 0;
                TCHAR sz[1024]=TEXT("");
                TCHAR szTime[1024]=TEXT("");
                TCHAR szTitle[1024]=TEXT("");

                FILE* fp = fopen(szFile,TEXT("a"));
                if (!fp) return;

                va_start(va,lpszFormat);
                vsnprintf(sz,sizeof(sz),lpszFormat,va);
                va_end(va);

                time(&now);
                gettimeofday(&tv,NULL);
                st = localtime(&now);
                int millisecond = tv.tv_usec/1000;
                // build the full time string value time string now.
                snprintf(szTime,sizeof(szTime),_T("%02d/%02d %02d:%02d:%02d.%03d"),
                 st->tm_mday,st->tm_mon+1,st->tm_hour,st->tm_min,
                 st->tm_sec,millisecond);

                // puts string now.
                fputs(szTime,fp);

                // dump the content of current process item content value data now.
                snprintf(szTitle,sizeof(szTitle),_T(",tick=%d,pid=%d,tid=%d, "),
                        tv.tv_usec,(int)getpid(),
                        (int)pthread_self());

                // write the file now.
                fputs(szTitle,fp);
                fputs(sz,fp);
                fputs(TEXT("\n"),fp);
                fclose(fp);
        }



	// 打印二进制数据.
	void logbin(void* lpszInput,LONG nBufSize,LPCTSTR lpszTitle=NULL)
	{
		int i = 0;
		static TCHAR szTemp[8192]={0};
		TCHAR szBuffer[1024]={0};
		FILE* fp = fopen(szFile,TEXT("a"));
		if  (!fp) return;
		// convert the data type item value.
		LPBYTE lpszData = (LPBYTE)lpszInput;
		// try to dump the special title information.
		if (strlen(lpszTitle)) fputs(lpszTitle,fp);
		// try to dump the content buffer address and the data size content item value data now.
		snprintf(szTemp,sizeof(szTemp),TEXT("buffer=[0x%08x],size=[%d]\n"),lpszData,nBufSize);
		fputs(szTemp,fp);
		fputs(TEXT("===============================================\n"),fp);
		if (nBufSize>sizeof(szTemp)) nBufSize=sizeof(szTemp);
		// dump the content now.
		for (i=0;i<nBufSize;i++)
		{
			snprintf(szTemp,sizeof(szTemp),TEXT("%02X "),lpszData[i]);
			strcat(szBuffer,szTemp);
			if ((i%16)==15)
			{
				fputs(szBuffer,fp);
				fputs(TEXT("\n"),fp);
				szBuffer[0]=0;
			}
		}

		//// last line data now.
		if (strlen(szBuffer)) {
			fputs(szBuffer,fp);
		}

		// close the file now.
		fputs(TEXT("\n"),fp);
		fclose(fp);
	}

protected:
	TCHAR szFile[MAX_PATH];
};

#endif//__LOGTEMP_HEADER__
