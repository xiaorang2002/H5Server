
#ifndef __ILOGDBENGINE_HEADER__
#define __ILOGDBENGINE_HEADER__

#include <logger/TraceInfo.h>

enum enumLogdbErrorCode
{
	LOGDBERR_UNK		= -1,			// 未知错误.
	LOGDBERR_OK			=  0,			// 没有错误.
	LOGDBERR_POOLISNUL	= 0x8000,	// 输入的Pool无效.
};


#ifndef LOGDBAPI
#define LOGDBAPI int32 __stdcall
#endif//LOGDBAPI

struct ILogdbEngine
{
public:
	virtual LOGDBAPI Init() = 0;
	virtual LOGDBAPI Term() = 0;

public://写日志.
	virtual LOGDBAPI WriteGainLost(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,__int64 nGainScoreT,__int64 nLostScoreT,int32 nGainCoin,int32 nLostCoin) = 0;
	virtual LOGDBAPI WriteAddScore(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,LPCTSTR lpszName,BOOL bisAdd,int32 nScore,int32 nCoin) = 0;
};

class CLogdbEngineNUL : public ILogdbEngine
{
public:
	virtual LOGDBAPI Init()
	{
		CTraceInfo::Trace(_T("Load LogdbEngine.so failed!"));
		return (LOGDBERR_OK);
	}
	
	virtual LOGDBAPI Term()
	{
		return (LOGDBERR_OK);
	}

	virtual LOGDBAPI WriteGainLost(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,__int64 nGainScoreT,__int64 nLostScoreT,int32 nGainCoin,int32 nLostCoin)
	{
		return (-1);
	}

	virtual LOGDBAPI WriteAddScore(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,LPCTSTR lpszName,BOOL bisAdd,int32 nScore,int32 nCoin)
	{
		return (-1);
	}
};

typedef ILogdbEngine* (WINAPI* PFNCREATELOGDBENGINE)();
//// 加载算法引擎,并初始化需要的相关参数以备用.
inline ILogdbEngine* __stdcall GetLogdbEngine()
{
	static ILogdbEngine* pLogdb = NULL;
	do 
	{
		/// pointer.
		if (!pLogdb)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("LogdbEngine.so"));
			if (!hInst) {
				CTraceInfo::Trace(_T("Load LogdbEngine.so failed,create CLogdbEngineNUL stub!"));
				break;
			}

			// try to get the special function to create the interface now create the item now.
			PFNCREATELOGDBENGINE  pfn = (PFNCREATELOGDBENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				CTraceInfo::Trace(_T("get LogdbEngine.so function fun0000 failed!"));
				FreeLibrary(hInst);
				break;
			}

			// interface.
			pLogdb = pfn();
		}
	}   while (0);

	// check now.
	if (!pLogdb) {
		 pLogdb = new CLogdbEngineNUL();
		 CTraceInfo::Trace(_T("Load LogdbEngine.so failed,create CLogdbEngineNUL stub!"));
	}
//Cleanup:
	return (pLogdb);
}

#endif//__ILOGDBENGINE_HEADER__