#pragma once

#ifdef _WIN32

#pragma warning(disable:4018)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4244)

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// 从 Windows 头中排除极少使用的资料
#endif

// 如果您必须使用下列所指定的平台之前的平台，则修改下面的定义。
// 有关不同平台的相应值的最新信息，请参考 MSDN。
#ifndef WINVER				// 允许使用特定于 Windows 95 和 Windows NT 4 或更高版本的功能。
#define WINVER 0x0400		// 将此更改为针对于 Windows 98 和 Windows 2000 或更高版本的合适的值。
#endif

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows NT 4 或更高版本的功能。
#define _WIN32_WINNT 0x0400	// 将此更改为针对于 Windows 2000 或更高版本的合适的值。
#endif						

#ifndef _WIN32_WINDOWS		// 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0410 // 将此更改为针对于 Windows Me 或更高版本的合适的值。
#endif

#ifndef _WIN32_IE			// 允许使用特定于 IE 4.0 或更高版本的功能。
#define _WIN32_IE 0x0400	// 将此更改为针对于 IE 5.0 或更高版本的合适的值。
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 某些 CString 构造函数将为显式的

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 类
#include <afxodlgs.h>       // MFC OLE 对话框类
#include <afxdisp.h>        // MFC 自动化类
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC 数据库类
#endif // _AFX_NO_DB_SUPPORT

#include <afxdtctl.h>		// MFC 对 Internet Explorer 4 公共控件的支持
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "../../../SDKCondition.h"

//////////////////////////////////////////////////////////////////////////////////
#ifdef SDK_CONDITION

//////////////////////////////////////////////////////////////////////////////////
//开发环境

#include "../../../开发库/Include/GameServiceHead.h"
#ifndef _DEBUG
#ifndef _UNICODE
#pragma comment (lib,"../../../开发库/Lib/Ansi/KernelEngine.lib")
#else
#pragma comment (lib,"../../../开发库/Lib/Unicode/KernelEngine.lib")
#endif
#else
#ifndef _UNICODE
#pragma comment (lib,"../../../开发库/Lib/Ansi/KernelEngineD.lib")
#else
#pragma comment (lib,"../../../开发库/Lib/Unicode/KernelEngineD.lib")
#endif
#endif


//////////////////////////////////////////////////////////////////////////////////

#else

//////////////////////////////////////////////////////////////////////////////////
//系统环境


//平台环境
#include "..\..\..\..\系统模块\服务器组件\游戏服务\GameServiceHead.h"

#ifndef _DEBUG
#ifndef _UNICODE
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/KernelEngine.lib")
#else
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/KernelEngine.lib")
#endif
#else
#ifndef _UNICODE
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/KernelEngineD.lib")
#else
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/KernelEngineD.lib")
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////

#endif

#endif//_WIN32


#include "../inc/Feature.h"
#include "../FishProduceUtils/CMD_Fish.h"
#include <logger/TraceInfo2.h>
#include <logger/TraceInfo.h>


#ifdef NO_TRACE

#define TRACE_INFO_BEGIN
#define TRACE_INFO
#define TRACE_INFO_BIN
#define TRACE_INFO_END

#define TRACE_LOG_BEGIN             CTraceInfo::Instance()
#define TRACE_LOG                   CTraceInfo::Instance()->Trace
#define TRACE_LOG_BIN               CTraceInfo::Instance()->TraceBin
#define TRACE_LOG_END               CTraceInfo::Instance()->Term()

#else

#define TRACE_INFO_BEGIN(filename)  CTraceInfo2::Instance(filename);
#define TRACE_INFO                  CTraceInfo2::Instance()->Trace
#define TRACE_INFO_BIN              CTraceInfo2::Instance()->TraceBin
#define TRACE_INFO_END              CTraceInfo2::Instance()->Term()

#define TRACE_LOG_BEGIN             CTraceInfo::Instance();
#define TRACE_LOG                   CTraceInfo::Instance().Trace
#define TRACE_LOG_BIN               CTraceInfo::Instance().TraceBin
#define TRACE_LOG_END               CTraceInfo::Instance().Term()
#endif//
