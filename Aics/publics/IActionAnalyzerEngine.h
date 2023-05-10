
#ifndef __IACTIONANALYZERENGINE_HEADER__
#define __IACTIONANALYZERENGINE_HEADER__

#include <stdio.h>
#include <tchar.h>
#include <PshPack2.h>

// 错误代码
enum enumAcAnErrorCode
{
	ACANERR_UNK		  = -1,	// unknown.
	ACANERR_OK		  =  0,	// S_OK.
	ACANERR_POOLISNUL =  1, // pool is null.
	ACANERR_OPENFAIL  =  2, // open file failed.
};

// 倍率种类.
enum enumAnalyzeOddsType
{
    ANALYZE_TYPE0 = 0,  //unknown.
    ANALYZE_TYPE1 = 1,  //2 
    ANALYZE_TYPE2 = 2,  //3
    ANALYZE_TYPE3 = 3,  //4 
    ANALYZE_TYPE4 = 4,  //5 
    ANALYZE_TYPE5 = 5,  //6 
    ANALYZE_TYPE6 = 6,  //7 
    ANALYZE_TYPE7 = 7,  //8
    ANALYZE_TYPE8 = 8,  //9
    ANALYZE_TYPE9 = 9,  //10
    ANALYZE_TYPE10= 10, //12
    ANALYZE_TYPE11= 11, //15
    ANALYZE_TYPE12= 12, //18
    ANALYZE_TYPE13= 13, //20
    ANALYZE_TYPE14= 14, //25
    ANALYZE_TYPE15= 15, //30
    ANALYZE_TYPE16= 16, //40
    ANALYZE_TYPE17= 17, //41-100
    ANALYZE_TYPE18= 18, //101-300
    ANALYZE_TYPE19= 19, //301-500
    ANALYZE_TYPE20= 20, //501-999
    ANALYZE_TYPE21= 21, //9999(保留)
    ANALYZE_TYPE22= 22, //9999(保留)
    ANALYZE_TYPE23= 23, //9999(保留)
    ANALYZE_TPMAX = 24, //最大种类.
};

// 交互的数据结构.
// typedef struct tagActionAnalyzerPack
// {
// 	uint32 sign;		// 有效性.
// 	uint32 roomid;	// 房间id.
// 	uint32 tableid;	// 桌台id.
// 	uint32 charid;	// 椅子id.
// 	uint32 userid;	// 用户id.
// 	uint32 type;		// 开炮/得分.
// 
// }   ActionAnalyzerPack;

// 结构体:洗码量总进总出.
// size=1 + 3 + 8 + 8 + 4 = 24.
typedef struct tagXimalianItem
{
    unsigned char	   oddsType;	// 鱼的倍率种类.
    unsigned char		resv[3];	// 保留字段.
	unsigned __int64  firescore;	// 开炮分(倍率鱼).
	unsigned __int64 catchscore;	// 渔获分(倍率鱼).
    uint32		  resv2;    // 保留字段.
}   XimalianItem;


// 结构体: 玩家行为分析结构体
// size = 4 + 4 + 2 * 4 + 4 + 4 + 4 + 20 + 8 + 8 + 24 * 24 = 640
typedef struct tagActionAnalyzeUserInfo
{
    uint32		   sign;	// 结构标识字段.
	uint32          tick;	// 发送时间间隔.
	unsigned short			vid;	// 厂商id.
	unsigned short			pid;	// 产品id.
	unsigned short		tableid;	// 桌台id.
	unsigned short		 charid;	// 椅子id.
	uint32		 roomid;	// 房间id.
	uint32		 userid;	// 用户编号.
    uint32	   toubbili;	// 投币比例.
	uint32	    resv[5];	// 保留字段.
    unsigned __int64  firescore;	// 总进分(所有鱼).
    unsigned __int64 catchscore;	// 总出分(所有鱼).
    XimalianItem  item[ANALYZE_TPMAX];
}   ActionAnalyzeUserInfo;

#include <PopPack.h>

#ifndef ACANAPI
#define ACANAPI int __stdcall
#endif//ACANAPI

struct IActionAnalyzerEngine
{
public:
	virtual ACANAPI Init(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nToubbili) = 0;
	virtual ACANAPI Term() = 0;

public://记录玩家开炮/得分数据.
	virtual ACANAPI OnPlayerFired(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nBulletScore) = 0;
	virtual ACANAPI OnFishCatched(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nFishScore) = 0;
};

class CActionAnalyzerEngineNUL : public IActionAnalyzerEngine
{
public:
	virtual ACANAPI Init(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nToubbili)
	{
		return (ACANERR_OK);
	}
	
	virtual ACANAPI Term()
	{
		return (ACANERR_OK);
	}

public://跟踪玩家开炮及得分.
	virtual ACANAPI OnPlayerFired(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nBulletScore)
	{
		return (ACANERR_OK);
	}

	virtual ACANAPI OnFishCatched(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nFishScore) 
	{
		return (ACANERR_OK);
	}
};

// 简单写日志.
inline void LogInfoAcan(const char* info)
{
	TCHAR szFile[MAX_PATH];
	GetModuleFileName(NULL,szFile,MAX_PATH);
	lstrcat(szFile,TEXT(".log"));
	FILE* fp = _tfopen(szFile,_T("a"));
	fputs(info,fp);
	fclose(fp);
}

typedef IActionAnalyzerEngine* (WINAPI* PFNCREATEACANENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IActionAnalyzerEngine* __stdcall GetActionAnalyzeEngine()
{
	static IActionAnalyzerEngine* acan = NULL;
	do 
	{
		// pointer.
		if (!acan)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("ActionAnalyzerEngine.dll"));
			if (!hInst) {
				LogInfoAcan("ActionAnalyzerEngine.dll not exist.");
				break;
			}

			// try to get the special function to create the interface now create the now.
			PFNCREATEACANENGINE  pfn = (PFNCREATEACANENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				LogInfoAcan("get AicsEngine.dll function fun0000 failed!");
				FreeLibrary(hInst);
				break;
			}

			// interface.
			acan = pfn();
		}
	}   while (0);

	// check now.
	if (!acan) {
		 acan = new CActionAnalyzerEngineNUL();
		 LogInfoAcan("Load AicsEngine.dll failed,create CAicsEngineNUL stub!");
	}
//Cleanup:
	return (acan);
}

#endif//__IAICSENGINE_HEADER__