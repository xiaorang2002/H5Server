/*
	�ͻ�����ӿ��ļ�.

History:
	2016-05-27 17:00 
	����OnUserSitdown/OnUserStandup,�������û�.
*/

#ifndef __IAICSENGINE_HEADER__
#define __IAICSENGINE_HEADER__

#include <windows.h>
#define AICS_DATA_SIGN (0xA293F6DB)

enum enumAicsErrorCode
{
	AICSERR_UNK		= -1,		// δ֪����.
	AICSERR_OK		=  0,		// û�д���.
	AICSERR_NOKILL  = 0x8000,	// �㷨Ĭ��û������״̬.
	AICSERR_KILLED	= 0x9000,	// �㷨�����㷵��ֵ.
};

#include <PshPack2.h>
#define  ONBKMAX  (8)	// һ����ɢ�������㲻����8��.
// �ӵ�������ʱ,�������������㷨��.
typedef struct tagOnBulletKillInfo
{
	long sign;		// ������Ч��.
	long nId;		// �����Ψһ��ʾ. 
	long nOdds;		// ��ǰ��ı���.
	long nResult;	// ����û����,TRUE/FALSE.
}   ONEBKILLINFO,*PONEBKINFO;

// Զ�̼������ݽṹ.
#define MAX_ROOMTABLE (2000)	// һ���Ի�ȡ���ѵ������̨����(��Ϊͨ�Ż�����ֻ����ô��).
typedef struct tagRemotePlusHard
{
	long sign;		// ������Ч��.
	long tableid;	// ������̨��.
	long hard;		// �Ƿ��м���.
	long resv;		// ��������ֵ. 
} RemotePlusHard,*PRemotePlusHard;

// �û����·ּ�¼.
typedef struct tagPlayerAddsubScore
{
	long	    sign;	// ������Ч��.
	long	   isadd;	// �Ƿ����Ϸ�(1:�Ϸ�,0:�·�)
	long	   seqid;	// ������ˮ��(�������ٶ���).
	long	   score;	// �û�������.
	long	    coin;	// �û����·ֱ�ֵ.
	long	 tableid;	// �û�������.
	long	  charid;	// �û����κ�.
	char	name[32];	// �û����û���.
	long	 resv[2];	// �������ݳ���.
} PlayerAddsubScore;

// ϴ����ͬ�����ݽṹ.
typedef struct tagTableGainLostInfo
{
	long			 sign;	// ������Ч��.
	long			seqid;	// ������ˮ��(�������ٶ���).
	long		  tableid;	// ��ǰ��̨��.
	long		 gaincoin;	// �ܽ�����.
	long		 lostcoin;	// �ܳ�����.
	long		  resv[4];	// �����ֽ�(16).
	__int64		gainscore;	// �ܽ�����.
	__int64		lostscore;	// �ܳ�����.
}   TableGainLostInfo;

// �ⲿ��Ϸ���ò�������.
typedef struct tagGameSettings
{
	LONG			cbSize;	// ��ǰ���ݽṹ�Ĵ�С(�汾��Ϣ).
	LONG    nTotalAddScore; //��ǰ���������ڻ������Ϸ�[��].
	LONG    nTotalSubScore; //��ǰ���������ڻ������·�[��].
	LONG    nTotallsProfit; //������������·��������.
	LONG    nPreviouProfit; //�ϴ��������Ǵ���ʱ������[��].
	LONG    nCurrentProfit; //��ǰ���������ڻ�����Ӯ��.
	LONG    nTotalToubiCnt; //��ǰ������Ͷ������.
	LONG    nTotalTuibiCnt; //��ǰ�������˱�����.
	LONG    nTotalTuicpCnt; //��ǰ�����˲�Ʊ����.
	LONG    nTotalRunnTime; //��ǰ����������ʱ��.
	LONG    nCurrToubiBiLi; //��ǰ������Ͷ�ұ���.
	LONG    nCurrCaiPoBiLi; //��ǰ�����Ĳ�Ʊ����.
	LONG    nbIsTuiCaiPiao; //�������˱һ��ǲ�Ʊ.[1:��Ʊ]
	LONG       nDifficulty; //��ǰ��Ӧ����Ϸ�Ѷ�.
	LONG         nAreaType; //��ǰ��Ӧ�ĳ�������.
	LONG       nJiapaoFudu; //��ǰ���ڵķ�����ֵ.
	LONG           nPaoMin; //��ǰ�ڵ���С����ֵ.
	LONG           nPaoMax; //��ǰ�ڵ��������ֵ.
	LONG       nDamaTiansu; //��ǰ���õĴ�������.
	LONG           nLineId; //��ǰ�������ߺ���Ϣ.
	LONG            nTaiId; //��ǰ������̨����Ϣ.
	LONG   nAddScoreMultip; //��ǰ���·ֵı���ֵ.
	LONG        nMaxPlayer; //��ǰ������������.
	LONG		 nMaxTable;	//��ǰ�������������.
}   GameSettings;

#include <PopPack.h>

#ifndef AICSAPI
#define AICSAPI long __stdcall
#endif//AICSAPI

struct IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet) = 0;
	virtual AICSAPI Term() = 0;

public://���÷���ID,������ROOMID,�㷨�Ż����xml.
	virtual AICSAPI SetRoomID(LONG nRoomID) = 0;

public://�û����ڸ��÷ּ�¼.
	virtual AICSAPI OnPlayerFired(LONG nTableID,LONG nCharID,LONG nBulletScore) = 0;
	virtual AICSAPI OnFishCatched(LONG nTableID,LONG nCharID,LONG nScore) = 0;
	virtual AICSAPI OnSyncData(BOOL bForceSync) = 0;

public://�û����·ּ�¼.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore) = 0;
	virtual AICSAPI OnSubScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore) = 0;

public: // �жϵ�ǰ�㵽��������.
	virtual AICSAPI OneBulletKill(LONG nTableID,LONG nCharID,LONG nUserID,LONG nBulletScore,PONEBKINFO pList,LONG nCount) = 0;

public: // ��ȡ�ܽ��ܳ�.
	virtual AICSAPI GetGainLostEx(LONG nTableId,LONG* plGain,LONG* plLost,LONG* plPresent,LONG* plWincoin) {return 0;}

public: // �����û�����/�뿪.
	virtual AICSAPI OnUserSitdown(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserStandup(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserUpgrade(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOldState,BOOL bNewState,BOOL bFreeroom) {return 0;}
};

class CAicsEngineNUL : public IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet)
	{
		return (AICSERR_OK);
	}
	
	virtual AICSAPI Term()
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI SetRoomID(LONG nRoomID)
	{
		return (0);
	}

public://�û����ڸ��÷ּ�¼.
	virtual AICSAPI OnPlayerFired(LONG nTableID,LONG nCharID,LONG nBulletScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnFishCatched(LONG nTableID,LONG nCharID,LONG nScore) 
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSyncData(BOOL bForceAsync)
	{
		return (AICSERR_OK);
	}

public://�û����·ּ�¼.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSubScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore)
	{
		return (AICSERR_OK);
	}

public: // �жϵ�ǰ�㵽��������.
	virtual AICSAPI OneBulletKill(LONG nTableID,LONG nCharID,LONG nUserID,LONG nBulletScore,PONEBKINFO pList,LONG nCount)
	{
		return (AICSERR_OK);
	}

public:	// �����û�����/�뿪.
	virtual AICSAPI OnUserSitdown(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserStandup(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserUpgrade(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIs2OldUser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}
};

typedef IAicsEngine* (WINAPI* PFNCREATEAICSENGINE)();
// �����㷨����,����ʼ����Ҫ����ز����Ա���.
inline IAicsEngine* __stdcall GetAicsEngine()
{
	static IAicsEngine* aics = NULL;
	do 
	{
		// pointer.
		if (!aics)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("AicsEngine.dll"));
			if (!hInst) {
				break;
			}

			// try to get the special function to create the interface now create the now.
			PFNCREATEAICSENGINE  pfn = (PFNCREATEAICSENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				FreeLibrary(hInst);
				break;
			}

			// interface.
			aics = pfn();
		}
	}   while (0);

	// check now.
	if (!aics) {
		 aics = new CAicsEngineNUL();
	}
//Cleanup:
	return (aics);
}

#endif//__IAICSENGINE_HEADER__