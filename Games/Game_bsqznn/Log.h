#ifndef LOG_HEAD_FILE
#define LOG_HEAD_FILE

#pragma once

#include <fstream>

//////////////////////////////////////////////////////////////////////////

//输出日志
class CLog
{
	//服务参数
public:
	bool							m_bLoging;							//启动标志
	std::ofstream					m_LogFile;							//输出日志

	//函数定义
public:
	//构造函数
	CLog();
	//析构函数
	virtual ~CLog();

	//信息接口
public:
	//追踪信息
    virtual bool TraceString(std::string pszString);

	//功能函数
public:
	//创建日志
    bool OpenLog(std::string lpszFile);
	//关闭日志
	bool CloseLog();
	//查询状态
	bool IsLoging() { return m_bLoging; }
};

//////////////////////////////////////////////////////////////////////////

#endif
