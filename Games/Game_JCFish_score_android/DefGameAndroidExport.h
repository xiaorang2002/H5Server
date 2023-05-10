#ifndef		__DEF_H_GAME_ANDROID_EXPORT_H__
#define		__DEF_H_GAME_ANDROID_EXPORT_H__

#ifdef _WIN32
#if	defined (__GAME_ANDROID__)
#define		_GAME_ANDROID_EXPORT_	__declspec( dllexport )  
#else
#define		_GAME_ANDROID_EXPORT_	__declspec( dllimport )
#endif
#else
#define  _GAME_ANDROID_EXPORT_
#endif

#include <types.h>

class IAndroidUserItemSink;

//得到桌子实例
extern "C" _GAME_ANDROID_EXPORT_ IAndroidUserItemSink* GetAndroidInstance();
//删除桌子实例
extern "C" _GAME_ANDROID_EXPORT_ void DelAndroidInstance(IAndroidUserItemSink* pITableFrameSink);


#endif	//__DEF_H_GAME_ANDROID_EXPORT_H__








