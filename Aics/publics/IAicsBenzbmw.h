

#ifndef __IAICSBENZBMW_HEADER__
#define __IAICSBENZBMW_HEADER__

#include <types.h>
#include <logger/LogWarn.h>

#ifdef  _WIN32
#include <windows.h>
#else
#include <unistd.h>	
#include <dlfcn.h>
#include <string.h>
#endif//_WIN32

#ifdef _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32


#define sOk         (0)
#define sDllError   (-1001)
#define AICSBENZ_SIGN (0xDA33E374)

// server sign for remote plus hard.
#define AICS_SERVER_SIGN (0xA293F6DB)

#ifdef  _WIN32
#define AICSAPI int32 __stdcall
#define STDCALL	__stdcall
#else
#define AICSAPI int32
#define STDCALL
#endif//_WIN32

// 远程加难数据结构.
#define BENZ_MAXTABLE (2000)	// 一次性获取加难的最大桌台数量(因为通信缓冲区只有这么大).
typedef struct tagBenzRemotePlusHard
{
	int32 sign;		// 数据有效性.
	int32 tabid;	// 加难桌台号.
	int32 hard;		// 是否有加难.
	int32 resv;		// 保留数据值. 
} BenzRemotePlusHard,*PBenzRemotePlusHard;

// 错误代码.
enum eBenzBmwErrCodes
{
    AICSBENZERR_UNK	   = -1,    // 未知错误.
    AICSBENZERR_OK	   =  0,    // 没有错误.
    AICSBENZERR_NOKILL =  0,    // 算法默认没打死鱼状态.
    AICSBENZERR_KILLED =  0x90,	// 算法打死鱼返回值.
};

#define BENZGATE_MIN      (0)   //起始门.
// 八门押注. 
enum eBenzGateCode
{
    BENZGATE_PORSCHE     = 0,   //保时捷
    BENZGATE_BMW         = 1,   //宝马
    BENZGATE_BENZ        = 2,   //奔驰.         
    BENZGATE_VW          = 3,   //大众.

    BENZGATE_MIN_PORSCHE = 4,   //保时捷(小).
    BENZGATE_MIN_BMW     = 5,   //宝马(小).
    BENZGATE_MIN_BENZ    = 6,   //奔驰(小).
    BENZGATE_MIN_VW      = 7,   //大众(小).
	
    BENZGATE_MAX,               //三门押注最大值.
};

// 每一门参数配置.
// size = 4 + 8 = 12
struct tagBenzBmwGateIn
{
    long   sign;		         // 数据有效性(in).
    double lscorePlaced;         // 玩家下了多少分(in).
};

// 输出返回值列表.
// size = 4 + 8 + 1 + 1 + 1 + 1 = 16;
struct tagBenzBmwGateOut
{
    long   sign;                 // 数据有效性(in).
    double lscoreWinned;         // 玩家总嬴分(out).
    uint8   pocket;               // 牌类型(6种中的一种)
    double multi;                // 当前牌型倍数[赔率].
    uint8   iswin;	             // 是否中这一门(out).
    uint8   resv;
};

// 所有输入信息汇总.
// size = 12 + 16 + 4 + 32 = 64
struct tagBenzBmwGateInfo
{
    tagBenzBmwGateIn  inf[BENZGATE_MAX]; //输入值.
    tagBenzBmwGateOut out[BENZGATE_MAX]; //输出值.
    uint32                   outwingate; //中奖类型(红/黑/特殊牌).
    uint32                      resv[8]; //保留值.
};

// ox游戏内部参数配置.
struct tagBenzBmwSettings
{
    uint32       nToubbili;         //转换比例(1:1).           
    uint32       nMaxPlayer;        //原指桌面的玩家数量,现在改成天地玄黄四门数量.
    uint32		 nMaxTable;	        //当前房间的桌子数量.
};

// OX引擎接口.
struct IAicsBenzBmwEngine
{
public: // 初始化
    virtual AICSAPI Init(tagBenzBmwSettings* pGs) = 0;
    virtual AICSAPI Term() = 0;

public://选择房间.
    virtual AICSAPI SetRoomID(int32 nRoomID) = 0;

public: // 出牌计算.
    virtual AICSAPI benzBmwOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagBenzBmwGateInfo* pInfo) = 0;

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plAllLost, double* plTaxedLost, double* plPresent, double* plWincoin) = 0;
	virtual AICSAPI GetTexPresent(uint32 tabid, double* plPresent,BOOL bSubit=FALSE) = 0;
    virtual AICSAPI OnGivenScore(uint32 uid, uint32 tabid, uint32 chairid, double nGivescore) = 0;

    // 用户状态
    virtual AICSAPI OnUserSitdown(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserStandup(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserUpgrade(uint32 uid, uint32 tabid, uint32 chairid, uint32 lstStat, uint32 newStat, bool bromfree) = 0;
};

// 简单接口类,如果dll不存在,那么返回这个接口.
class CAicsBenzBmwEngineNUL : public IAicsBenzBmwEngine
{
public: // 初始化
    virtual AICSAPI Init(tagBenzBmwSettings* pGs)
    {
        return (sDllError);
    }

    virtual AICSAPI Term()
    {
        return (sDllError);
    }

public://选择房间.
    virtual AICSAPI SetRoomID(int32 roomid)
    {
        return (sDllError);
    }

public:
	virtual AICSAPI benzBmwOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagBenzBmwGateInfo* pInfo)
	{
		return (sDllError);
	}

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plALost, double* plTaxedLost, double* plPresent, double* plWincoin)
    {
        return (sDllError);
    }

	virtual AICSAPI GetTexPresent(uint32 tabid, double* plPresent,BOOL bSubit=FALSE)
	{
		return (sDllError);
	}

    virtual AICSAPI OnGivenScore(uint32 uid, uint32 tabid, uint32 chairid, double nGivescore)
    {
        return (sDllError);
    }

    // 用户状态
    virtual AICSAPI OnUserSitdown(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree)
    {
        return (sDllError);
    }

    virtual AICSAPI OnUserStandup(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree)
    {
        return (sDllError);
    }

    virtual AICSAPI OnUserUpgrade(uint32 uid, uint32 tabid, uint32 chairid, uint32 lstStat, uint32 newStat, bool bromfree)
    {
        return (sDllError);
    }
};

typedef IAicsBenzBmwEngine* (WINAPI* PFNCREATEAICSREDXENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsBenzBmwEngine* WINAPI GetAicsBenzBmwEngine()
{
    char buf[300]={0};
    static IAicsBenzBmwEngine* aics = 0;
    do 
    {
        // pointer.
        if (!aics)
        {
#ifdef _WIN32
            lstrcpyA(buf,"libAicsBenzbmwEngine.so");
            HINSTANCE hInst=LoadLibraryA(buf);
#else
            getcwd(buf,sizeof(buf));
            strcat(buf,TEXT("/libAicsBenzbmwEngine.so"));
            HINSTANCE hInst=LoadLibrary(buf);
#endif//_WIN32

            if (!hInst) {
				logError(TEXT("load library libAicsBenzbmwEngine.so failed!"));
                break;
            }

            // try to get the special function to create the interface now create the function item.
            PFNCREATEAICSREDXENGINE  pfn = (PFNCREATEAICSREDXENGINE)GetProcAddress(hInst,"fun0004");
            if (!pfn) {
				logError(TEXT("get function of fun0004 failed"));
                FreeLibrary(hInst);
                break;
            }

            // interface.
            aics = pfn();
        }
    }   while (0);

    // check now.
    if (!aics) {
         aics = new CAicsBenzBmwEngineNUL();
    }
//Cleanup:
    return (aics);
}



#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32



#endif//__IAICSBENZBMW_HEADER__
