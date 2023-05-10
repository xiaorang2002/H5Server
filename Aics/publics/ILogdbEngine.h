
#ifndef __ILOGDBENGINE_HEADER__
#define __ILOGDBENGINE_HEADER__

#include <types.h>
#include <logger/Logtemp.h>
#ifdef _WIN32
#else
#include <dlfcn.h>
#endif//_WIN32

enum enumLogdbErrorCode
{
	LOGDBERR_UNK		= -1,			// 未知错误.
	LOGDBERR_OK			=  0,			// 没有错误.
	LOGDBERR_POOLISNUL	= 0x8000,	// 输入的Pool无效.
};

#ifdef _WIN32
#define LOGDBAPI int __stdcall
#else
#define LOGDBAPI int
#endif//_WIN32

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
		CLogtemp log(TEXT("LogdbEngine.log"));
		log.log(_T("Load LogdbEngine.dll failed,Use CLogdbEngineNUL."));
		return (LOGDBERR_OK);
	}
	
	virtual LOGDBAPI Term()
	{
		return (LOGDBERR_OK);
	}

	virtual LOGDBAPI WriteGainLost(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,__int64 nGainScoreT,__int64 nLostScoreT,int32 nGainCoin,int32 nLostCoin)
	{
		return (LOGDBERR_OK);
	}

	virtual LOGDBAPI WriteAddScore(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nTableId,LPCTSTR lpszName,BOOL bisAdd,int32 nScore,int32 nCoin)
	{
		return (LOGDBERR_OK);
	}
};

typedef ILogdbEngine* (WINAPI* PFNCREATELOGDBENGINE)();
//// 加载算法引擎,并初始化需要的相关参数以备用.
inline ILogdbEngine* WINAPI GetLogdbEngine()
{
	static ILogdbEngine* pLogdb = NULL;
	do 
	{
		/// pointer.
		if (!pLogdb)
		{
#ifdef  _WIN32
			HINSTANCE hInst=LoadLibrary(TEXT("libLogdbEngine.dll"));
#else
			HINSTANCE hInst=LoadLibrary(TEXT("libLogdbEngine.so"));
#endif//_WIN32
			if (!hInst) {
				CLogtemp log(TEXT("LogdbEngine.log"));
				log.log(_T("Load LogdbEngine.dll failed,create CLogdbEngineNUL stub!"));
				break;
			}

			// try to get the special function to create the interface now create the item now.
			PFNCREATELOGDBENGINE  pfn = (PFNCREATELOGDBENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				CLogtemp log(TEXT("LogdbEngine.log"));
				log.log(_T("get LogdbEngine.dll function fun0000 failed!"));
				FreeLibrary(hInst);
				break;
			}

			// interface now.
			pLogdb = pfn();
		}
	}   while (0);

	// check now.
	if (!pLogdb) {
		 pLogdb = new CLogdbEngineNUL();
		 CLogtemp log(TEXT("LogdbEngine.log"));
		 log.log(_T("Load LogdbEngine.dll failed,create CLogdbEngineNUL stub!"));
	}
//Cleanup:
	return (pLogdb);
}

#endif//__ILOGDBENGINE_HEADER__