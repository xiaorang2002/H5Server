/*
	通信公共结构体定义.

	指令集:
	1.获取随机数:
		发送 head + rc4 encrypted vendorInfo.	// 头信息+厂商信息
		返回 head + random.						// 服务器查询有没有对应的配置信息,如果有，再返回头信息+随机数.

	2.外部认证:
		用 厂商信息+产品ID 做为随机因子,解密随机数数组(AES256).
		用 随机数头四个字节做随机因子,再次加密随机数组(RC4).

		发送 head + encrypted random. 到服务器.


	3.获取BIN二进制数据.
		发送head + vendorInfo.		// 发送头信息与厂商信息.
		返回 head + bin data.		// 返回加密后的bin数据(RSA).

	4.获取Cfg配置数据.
		发送head + vendorInfo.		// 发送头信息与厂商信息.
		返回 head + cfg data.		// 返回加密后的cfg数据(RSA).
*/

#ifndef __ARITHMSTRUCT_HEADER__
#define __ARITHMSTRUCT_HEADER__

#ifdef  _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32


#define ARITHMSTRUCT_KEY    {0xF5, 0x51, 0x6C, 0xA0, 0xAC, 0xED, 0xE0, 0xA4, 0x62, 0x74, 0x9E, 0x77, 0x57, 0x07, 0xDE, 0xB5};
#define ARCMD_SIGN			(0x48495241)
#define AUTH_FLAG_OK		(0x9000)
#define MD5_BINLEN			(16)

// INI配置文件Section.
#define SECTION_SERVICE	TEXT("service")
#define SECTION_ARITHM  TEXT("arithm")
#define SECTION_ROOM	TEXT("room")
#define SECTION_ROMLIST TEXT("roomlist")
#define SECTION_TABLE   TEXT("table")
#define SECTION_AUTHEN	TEXT("authentic")
#define SECTION_PLUSHRD TEXT("plushard")
#define PLUSHARD_NAME	TEXT("hardcode")

#define ARCFG_INIFILE	TEXT("ArithmEngine.ini")

#ifndef ARBUFMAX	// buffer max size.
#define ARBUFMAX	(32768)
#endif//ARBUFMAX

#define ARCMD_MAIN	(0x4152)

enum tagArithmCommand
{
	ARCMD_GETRND = 0x8001,	// get random.
	ARCMD_EXTAUTH= 0x8002,  // extension authentic.
	ARCMD_GETBIN = 0x8003,	// get the aics.bin.
	ARCMD_ENUMROOM=0x8004,	// enum vendor rooms.
	ARCMD_GETCFG = 0x8005,	// get room aics.xml.
	ARCMD_GETMENU= 0x8006,	// get menu.xml config.
	ARCMD_PLUSHARD=0x8007,	// query if any plus hard.
	ARCMD_ADDSUB = 0x8008,	// player add or sub score.
	ARCMD_GAINLOST=0x8009,	// full table gain lost.
	ARCMD_CHKEXPIR=0x8010,	// check if expire now.
	ARCMD_GETCFGMD5=0x8020,	// get room aics.xml md5.
	ARCMD_PUSHWIN = 0x8021, // push table win data.
};

enum tagArithmTransError
{
	ARERR_OK		 = 0,		// command OK.
	ARERR_UNK		 = 0xE000,	// Unknown error.
	ARERR_INVALIDATA = 0xE001,	// invalidate data.
	ARERR_AUTHFAIL	 = 0xE002,	// Authentic failed.
	ARERR_VENDORFAIL = 0xE003,  // Vendor decrypt failed.
	ARERR_NOVENDOR   = 0xE004,	// Vendor config not exist.
	ARERR_AUTHFIRST  = 0xE005,	// Must Authentic first(no authentic).
	ARERR_CHECKSUM   = 0xE006,	// check sum of data failed.
	ARERR_NOMEM		 = 0xE007,	// none memory value.
	ARERR_NOPLUSHARD = 0xE008,	// no plus hard code.
	ARERR_NOCFG      = 0xE009,	// no room config exist.
	ARERR_BUILDPATH  = 0xE010,	// build path been failed.
	ARERR_INVALIDFILE= 0xE011,	// no file exist or open failed.
	ARERR_OPENFILE   = 0xE012,	// open file has been failed.
	ARERR_EXPIRED    = 0xE013,	// the aics has been expired.
	ARERR_INVALIDSEC = 0xE014,  // the secure engine invalidate.
	// ARERR_;
};


typedef union tagArithmRndBuf
{
	DWORD    dw;	// the dword type value.
	BYTE buf[4];	// the byte array value.
}  ARITHMRNDBUF,*PARITHMRNDBUF;

// size = 4 + 2 * 6 = 16
typedef struct tagARTransHead
{
	unsigned int   sign;	// the sign of package.
	unsigned short hr;		// the hr code value.
	unsigned short maincmd;	// the command id.
	unsigned short subcmd;	// the sub command.
	unsigned short length;	// data+head length.
	unsigned short crc;		// the crc of data.
	unsigned short resv;	// the reserved data.
}   ARTRANSHEAD,*PARTRANSHEAD;

/// Extension Authentic value now.
typedef struct tagARTransExtAuth
{
	unsigned char encrypt[16];
} ARTRANSEXTAUTH,*PARTRANSEXTAUTH;

// size = 4 + 2 + 2 + 6 * 4 = 32
/// Get bin and config data item now.
typedef struct tagARTransVendorInfo
{
	unsigned int   sign;		// 头标识.
	unsigned short wVendorID;	// 厂商ID.
	unsigned short wProductID;	// 产品ID.
	unsigned int   nRoomID;		// 版本号.
	unsigned int   nTableID;	// 桌面ID.
	unsigned int   reserved[4]; // 保留字.
} ARTRANSVENDORINFO,*PARTRANSVENDORINFO;

/// size = 4 * 2 + 4 * 6 = 32(byte)
/// Enum the rooms list item now.
typedef struct tagAREnumRoomList
{
	unsigned int sign;			// 头部标识.
	unsigned int roomid;		// 房间编号.
	unsigned int reserved[6];   // 保留空间.
}   ARENUMROOMLIST,*PARENUMROOMLIST;

// check if AICS has expired now.
typedef struct tagARCheckExpire
{
	unsigned int   sign;		// 头部标识.
	unsigned int roomid;		// 房间号码.
	unsigned int result;		// 校验结量.
	unsigned int   resv;		// 保留数据.
}   ARCHECKEXPIRE, *PARCHECKEXPIRE;

// 桌台盈亏数据.
#define MAX_PUSHWINCNT	(100)
struct tagWinData
{
	unsigned short tabid   : 11; // 桌子号(最大2047)
	unsigned short bNegative :1; // 负数标志位. 
	unsigned short winhigh :  4; // 高五位总赢币.
	unsigned int   winlower;	 // 低32位总赢币(最大1000亿).
};

// 加载的数据文件类型
enum tagLoadEncFileType
{
	LOADFILE_BIN = 1,	// bin文件.
	LOADFILE_CFG = 2,	// cfg配置.
};


#ifdef  _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32

#endif//__ARITHMSTRUCT_HEADER__