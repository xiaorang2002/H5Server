
#include "stdafx.h"
#include "Log.h"
#include <time.h>

//////////////////////////////////////////////////////////////////////////

//构造函数
CLog::CLog()
{
	//设置变量
	m_bLoging = false;
	return;
}

//析构函数
CLog::~CLog()
{
	CloseLog();
}

//追踪信息
bool CLog::TraceString(LPCTSTR pszString)
{
	//输出日志
	if( m_bLoging && m_LogFile.is_open() )
	{
		//获取时间
        time_t tt;
        time(&tt);
        CHAR szTimeBuffer[64];
        struct tm* tm = localtime(&tt);
        snprintf(szTimeBuffer,sizeof(szTimeBuffer),"【 %04d-%02d-%02d %02d:%02d:%02d 】",
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

        m_LogFile << szTimeBuffer << pszString << "\n";
		m_LogFile.flush();
	
	}

	return true;
}

//创建日志
bool CLog::OpenLog(LPCTSTR lpszFile)
{
	//效验
	ASSERT(!m_bLoging);
	if( m_bLoging ) return false;

	ASSERT( !m_LogFile.is_open() );
	if( m_LogFile.is_open() )
		m_LogFile.close();

	m_bLoging = true;

	m_LogFile.open( (LPCSTR)CT2A(lpszFile),std::ios::out|std::ios::app );

	return m_bLoging;
}

//关闭日志
bool CLog::CloseLog()
{
	if( m_LogFile.is_open() )
		m_LogFile.close();

	m_bLoging = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
