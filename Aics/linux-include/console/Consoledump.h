/*
    declare macro: 
        CONSOLE_DEBUG : enable log dump.

		CConsoledump::Instance()->init(hInst,bForce);
		CConsoledump::Instance()->dump/dumpWarn/dumpError.

        这种写法要小心,在生成Release版会被编译器优化掉!!!
    static* Instance() {
        static obj;
        return obj;
    }

    history:


*/

#ifndef __CONSOLEDUMP_HEADER__
#define __CONSOLEDUMP_HEADER__

#include <stdio.h>
#include <assert.h>
#include <file/FileExist.h>
#include <PathMagic.h>
#include <sys/time.h>
#include <time.h>

#ifdef  _WIN32
#include <tchar.h>
#else
#include <pthread.h>
#define _tprintf	printf
#define _fputts		fputs

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"

#endif//_WIN32

#define logInfo     CConsoledump::Instance()->dump
#define logWarn     CConsoledump::Instance()->dumpWarn
#define logError    CConsoledump::Instance()->dumpError

enum eTextColor
{
    eColor_default = 0,
    eColor_Yellow  = 1,
    eColor_Red     = 2,
};

#pragma warning(disable:4996)

class CConsoleColor
{
public:
    CConsoleColor(eTextColor color=eColor_default)
    {
        switch (color)
        {
        case eColor_Red:
#ifdef  _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_RED|FOREGROUND_INTENSITY));
#else
			printf(LIGHT_RED);
#endif//_WIN32
            break;
        case eColor_Yellow:
#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (6|FOREGROUND_INTENSITY));
#else
			printf(YELLOW);
#endif//_WIN32
            break;
        default:
#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
			printf(NONE);
#endif//_WIN32
            break;
        }
    }

    virtual ~CConsoleColor()
    {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
		printf(NONE);
#endif//_WIN32
    }

};


class CConsoledump
{
public:
	CConsoledump()
	{
		m_bConsole = FALSE;
		m_szFile[0]= '\0';
	}

	virtual ~CConsoledump()
	{
		if (m_bConsole) {
#ifdef  _WIN32
			FreeConsole();	
#endif//_WIN32
		}
	}

// #pragma optimize("", off)
    // try to instance the data now.
	static CConsoledump* Instance()
	{
		static CConsoledump* dump = 0;
        if (!dump) {
             dump = new CConsoledump();
        }
    //Cleanup:
		return (dump);
	}
// #pragma optimize("", on)
	
public:// enable dump. bForceShowConsole = force show console.
	void init(HINSTANCE hInst=NULL,BOOL bForceConsole=FALSE)
	{
        BOOL bExist=FALSE;
		CPathMagic::GetModuleFileName(hInst,m_szFile,MAX_PATH);
		lstrcat(m_szFile,TEXT(".debug")); // file.
		printf("console dump file name:[%s]\n",m_szFile);

#ifdef _WIN32
        DWORD dwAttrib = GetFileAttributes(m_szFile);
        if (INVALID_FILE_ATTRIBUTES!=dwAttrib) {
            bExist=TRUE;
        }
#else
		// check file exist.
		struct stat st={0};
		int err = stat(m_szFile,&st);
		if (0 == err) {
			bExist=TRUE;
		}
#endif//_WIN32
        // check if the console has created.
        if ((bExist) || (bForceConsole)) {
            m_bConsole  = TRUE;
        }

        // create console.
		if (m_bConsole) 
		{
#ifdef  _WIN32
			AllocConsole();
			freopen("CONOUT$","w+t",stdout);
			freopen("CONIN$","r+t",stdin);  
#endif//_WIN32
		}
	}

	// try to dump the special content value.
	void static dump(LPCTSTR lpszFormat,...)
	{
		va_list va;
		TCHAR buf[32]=TEXT("");
		if (!Instance()->m_bConsole) return;
		static TCHAR szInfo[4096]=TEXT("");
		va_start(va,lpszFormat);
		vsnprintf(szInfo,sizeof(szInfo),lpszFormat,va);
		va_end(va);
	//Cleanup:
		TCHAR szTime[256]=TEXT("");
#ifdef _WIN32
		SYSTEMTIME st;
		GetLocalTime(&st);
		wsprintf(szTime,TEXT("%02d%02d%02d.%02d%02d%02d.%03d(pid:%d,tid:%d)\t: "),
			st.wYear%100,st.wMonth,st.wDay,st.wHour,
			st.wMinute,st.wSecond,st.wMilliseconds,
			GetCurrentProcessId(),GetCurrentThreadId());
#else
		struct tm* st = 0;
		time_t now;
		timeval tv;
		time(&now);
		gettimeofday(&tv,NULL);
		st = localtime(&now);
		int millisecond = tv.tv_usec/1000;
		// build the full time string value time string now.
		snprintf(szTime,sizeof(szTime),_T("%02d/%02d %02d:%02d:%02d.%03d(pid:%d,tid:%d)\t: "),
                 st->tm_mon+1,st->tm_mday,st->tm_hour,st->tm_min,
                 st->tm_sec,millisecond,getpid(),
				 (int)pthread_self());
#endif//_WIN32
		_tprintf(TEXT("%s\n"),szInfo);
		FILE* fp = _tfopen(Instance()->m_szFile,TEXT("a"));
		_fputts(szTime,fp);
		_fputts(szInfo,fp);
		_fputts(TEXT("\n"),fp);
		fclose(fp);
	}

    // dump the special warning information now.
    void static dumpWarn(LPCTSTR lpszFormat,...)
    {
        va_list va;
        if (!Instance()->m_bConsole) return;
        static TCHAR szInfo[4096]=TEXT("");
        va_start(va,lpszFormat);
        vsnprintf(szInfo,sizeof(szInfo),lpszFormat,va);
        va_end(va);
    //Cleanup:

		TCHAR szTime[256]=TEXT("");
		CConsoleColor color(eColor_Yellow);
#ifdef  _WIN32
		SYSTEMTIME st;
		GetLocalTime(&st);
		wsprintf(szTime,TEXT("%02d%02d%02d.%02d%02d%02d.%03d(pid:%d,tid:%d)\t: "),
			st.wYear%100,st.wMonth,st.wDay,st.wHour,
			st.wMinute,st.wSecond,st.wMilliseconds,
			GetCurrentProcessId(),GetCurrentThreadId());
#else
        struct tm* st = 0;
        time_t now;
        timeval tv;
        time(&now);
        gettimeofday(&tv,NULL);
        st = localtime(&now);
        int millisecond = tv.tv_usec/1000;
        // build the full time string value time string now.
        snprintf(szTime,sizeof(szTime),_T("%02d/%02d %02d:%02d:%02d.%03d(pid:%d,tid:%d)\t: "),
                 st->tm_mon+1,st->tm_mday,st->tm_hour,st->tm_min,
                 st->tm_sec,millisecond,getpid(),
                 (int)pthread_self());
#endif//_WIN32
		// dump the content value now.
		_tprintf(TEXT("%s\n"),szInfo);
        FILE* fp = _tfopen(Instance()->m_szFile,TEXT("a"));
		_fputts(szTime,fp);
        _fputts(szInfo,fp);
        _fputts(TEXT("\n"),fp);
        fclose(fp);
        return;
    }

    // dump the special error information data now.
    void static dumpError(LPCTSTR lpszFormat,...)
    {
        va_list va;
        if (!Instance()->m_bConsole) return;
        static TCHAR szInfo[4096]=TEXT("");
        va_start(va,lpszFormat);
        vsnprintf(szInfo,sizeof(szInfo),lpszFormat,va);
        va_end(va);
    //Cleanup:
		TCHAR szTime[256]=TEXT("");
		CConsoleColor color(eColor_Red);

#ifdef _WIN32
		GetLocalTime(&st);
		wsprintf(szTime,TEXT("%02d%02d%02d.%02d%02d%02d.%03d(pid:%d,tid:%d)\t: "),
			st.wYear%100,st.wMonth,st.wDay,st.wHour,
			st.wMinute,st.wSecond,st.wMilliseconds,
			GetCurrentProcessId(),GetCurrentThreadId());
#else
        struct tm* st = 0;
        time_t now;
        timeval tv;
        time(&now);
        gettimeofday(&tv,NULL);
        st = localtime(&now);
        int millisecond = tv.tv_usec/1000;
        // build the full time string value time string now.
        snprintf(szTime,sizeof(szTime),_T("%02d/%02d %02d:%02d:%02d.%03d(pid:%d,tid:%d)\t: "),
                 st->tm_mon+1,st->tm_mday,st->tm_hour,st->tm_min,
                 st->tm_sec,millisecond,getpid(),
                 (int)pthread_self());
#endif//_WIN32
		_tprintf(TEXT("%s\n"),szInfo);
        FILE* fp = _tfopen(Instance()->m_szFile,TEXT("a"));
		_fputts(szTime,fp);
        _fputts(szInfo,fp);
        _fputts(TEXT("\n"),fp);
        fclose(fp);
        return;
    }

    // try to break the special content value item for later user.
    void static assertbreak(BOOL bCondition,LPCTSTR format=NULL,...)
    {
        static TCHAR szInfo[1024]=TEXT("");
        do 
        {
            if (format)
            {
                va_list va;
                va_start(va,format);
                vsnprintf(szInfo,sizeof(szInfo),format,va);
                // try to dump information.
                Instance()->dump(szInfo);
                va_end(va);
            }
            
            // try to check if the console item now.
            BOOL bConsole = Instance()->m_bConsole;
            if ((bConsole) && (!bCondition))
            {
//                 if (IDYES != ::MessageBox(GetDesktopWindow(),szInfo,TEXT("是否中断?"),MB_YESNO|MB_DEFBUTTON2))
//                 {
//                     // break now.
//                     __asm int 3;
//                 }
            }
        } while (0);
    //Cleanup:
        return;
    }

public:
	TCHAR m_szFile[MAX_PATH];
	BOOL  m_bConsole;
};

// get current time string.
inline LPCTSTR getTime()
{
    static TCHAR szTime[32];
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    _sntprintf(szTime,32,TEXT("%02d:%03d.%03d"),st.wMinute,st.wSecond,st.wMilliseconds);
#else
        struct tm* st = 0;
        time_t now;
        timeval tv;
        time(&now);
        gettimeofday(&tv,NULL);
        st = localtime(&now);
        int millisecond = tv.tv_usec/1000;
        // build the full time string value time string now.
        snprintf(szTime,sizeof(szTime),_T("%02d:%02d.%03d"),st->tm_min,st->tm_sec,millisecond);
#endif//_WIN32
//Cleanup:
    return (szTime);
}


#endif//__CONSOLEDUMP_HEADER__
