
#ifndef __IACTIONANALYZERENGINE_HEADER__
#define __IACTIONANALYZERENGINE_HEADER__

#include <stdio.h>
#include <tchar.h>
#include <PshPack2.h>

// �������
enum enumAcAnErrorCode
{
	ACANERR_UNK		  = -1,	// unknown.
	ACANERR_OK		  =  0,	// S_OK.
	ACANERR_POOLISNUL =  1, // pool is null.
	ACANERR_OPENFAIL  =  2, // open file failed.
};

// ��������.
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
    ANALYZE_TYPE21= 21, //9999(����)
    ANALYZE_TYPE22= 22, //9999(����)
    ANALYZE_TYPE23= 23, //9999(����)
    ANALYZE_TPMAX = 24, //�������.
};

// ���������ݽṹ.
// typedef struct tagActionAnalyzerPack
// {
// 	unsigned int32 sign;		// ��Ч��.
// 	unsigned int32 roomid;	// ����id.
// 	unsigned int32 tableid;	// ��̨id.
// 	unsigned int32 charid;	// ����id.
// 	unsigned int32 userid;	// �û�id.
// 	unsigned int32 type;		// ����/�÷�.
// 
// }   ActionAnalyzerPack;

// �ṹ��:ϴ�����ܽ��ܳ�.
// size=1 + 3 + 8 + 8 + 4 = 24.
typedef struct tagXimalianItem
{
    unsigned char	   oddsType;	// ��ı�������.
    unsigned char		resv[3];	// �����ֶ�.
	unsigned long     firescore;	// ���ڷ�(������).
	unsigned long    catchscore;	// ����(������).
    unsigned int32		  resv2;    // �����ֶ�.
}   XimalianItem;


// �ṹ��: �����Ϊ�����ṹ��
// size = 4 + 4 + 2 * 4 + 4 + 4 + 4 + 20 + 8 + 8 + 24 * 24 = 640
typedef struct tagActionAnalyzeUserInfo
{
    unsigned int32		   sign;	// �ṹ��ʶ�ֶ�.
	unsigned int32          tick;	// ����ʱ����.
	unsigned short			vid;	// ����id.
	unsigned short			pid;	// ��Ʒid.
	unsigned short		tableid;	// ��̨id.
	unsigned short		 charid;	// ����id.
	unsigned int32		 roomid;	// ����id.
	unsigned int32		 userid;	// �û����.
    unsigned int32	   toubbili;	// Ͷ�ұ���.
	unsigned int32	    resv[5];	// �����ֶ�.
    unsigned long     firescore;	// �ܽ���(������).
    unsigned long    catchscore;	// �ܳ���(������).
    XimalianItem  item[ANALYZE_TPMAX];
}   ActionAnalyzeUserInfo;

#include <PopPack.h>

#ifndef ACANAPI
#define ACANAPI int32 __stdcall
#endif//ACANAPI

struct IActionAnalyzerEngine
{
public:
	virtual ACANAPI Init(int32 nVendorID,int32 nProductID,int32 nRoomID,int32 nToubbili) = 0;
	virtual ACANAPI Term() = 0;

public://��¼��ҿ���/�÷�����.
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

public://������ҿ��ڼ��÷�.
	virtual ACANAPI OnPlayerFired(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nBulletScore)
	{
		return (ACANERR_OK);
	}

	virtual ACANAPI OnFishCatched(int32 nTableID,int32 nCharID,int32 nUserId,int32 nOdds,int32 nFishScore) 
	{
		return (ACANERR_OK);
	}
};

// ��д��־.
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
// �����㷨����,����ʼ����Ҫ����ز����Ա���.
inline IActionAnalyzerEngine* __stdcall GetActionAnalyzeEngine()
{
	static IActionAnalyzerEngine* acan = NULL;
	do 
	{
		// pointer.
		if (!acan)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("ActionAnalyzerEngine.so"));
			if (!hInst) {
				LogInfoAcan("ActionAnalyzerEngine.so not exist.");
				break;
			}

			// try to get the special function to create the interface now create the now.
			PFNCREATEACANENGINE  pfn = (PFNCREATEACANENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				LogInfoAcan("get ActionAnalyzerEngine.so function fun0000 failed!");
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