
#ifndef __LOGWARN_HEADER__
#define __LOGWARN_HEADER__

#include <types.h>
#include <stdio.h>
#include <string.h>

#ifdef  WIN32
#include <windows.h>
#define snprintf _snprintf_s
#else
#include <stdarg.h>

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

#endif//WIN32



// define the function call.
#define logInfo  logTextinfo
#define logWarn  logWarninfo
#define logError logErrinfo


// color define now.
enum enumlogcolor
{
    ecolor_def = 0,
    ecolor_yellow,
    ecolor_red
};

class mlogcolor
{
public:
    mlogcolor(enumlogcolor color=ecolor_def)
    {
        switch (color)
        {
        case ecolor_red:
#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_RED|FOREGROUND_INTENSITY));
#else
            printf(LIGHT_RED);
#endif//_WIN32
            break;
        case ecolor_yellow:
#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (6|FOREGROUND_INTENSITY));
#else
            printf(YELLOW);
#endif//WIN32
            break;
        default:
#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
            printf(NONE);
#endif//WIN32
            break;
        }
    }

    ~mlogcolor()
    {
#ifdef  _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
        printf(NONE);
#endif//_WIN32
    }
};

inline void logTextinfo(const char* fmt,...)
{
    va_list va;
    char buf[4096]={0};
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
	lstrcat(buf,TEXT("\n"));
    va_end(va);
    printf(buf);
}

inline void logWarninfo(const char* fmt,...)
{
    va_list va;
    char buf[4096]={0};
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
	lstrcat(buf,TEXT("\n")); 
   va_end(va);
//Cleanup:
    mlogcolor color(ecolor_yellow);
    printf(buf);
}

inline void logErrinfo(const char* fmt,...)
{
    va_list va;
    char buf[4096]={0};
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
	lstrcat(buf,TEXT("\n"));
    va_end(va);
//Cleanup:
    mlogcolor color(ecolor_red);
    printf(buf);
}

#endif//__LOGWARN_HEADER__
