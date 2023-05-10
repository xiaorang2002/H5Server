

#ifndef __CSECUREENGINE_HEADER__
#define __CSECUREENGINE_HEADER__

#include <types.h>

#ifndef IN
#define IN 
#define OUT
#define INOUT
#endif//IN.

#ifndef CSECURE_VERSION
#define CSECURE_VERSION (0x0200)
#endif//CSECURE_VERSION

enum CSecureEngineErrorCode
{
	CSERR_UNK=-1,
	CSERR_OK = 0,				// OK.
	CSERR_CHKSUM       = 200,	// Check file sum failed.
	CSERR_UPDATEHDD    = 201,   // update HDD info failed.
	CSERR_FILE_NOFOUND = 220,	// the special file not found.
	CSERR_UNSUPPORT_OS = 221,   // the OS current un support.
	CSERR_CHECKHDD     = 222,	// check hdd serial failed.
};


struct ISecureEngine
{
protected:
	BYTE temp[512];

public:
	virtual int32 Init()         = 0;	// initialize the secure engine.
	virtual int32 ExternalAuth() = 0;    // do the internal authentication.
	virtual int32 InternalAuth() = 0;    // do the internal authentication.
	virtual int32 CheckHDMagic() = 0;    // check the hardware bind now.
	virtual LPBYTE LoadEncryptFile(int32 nLoadType,WORD wVendorID,WORD wProductID,int32 nRoomID,int32* plSize,WORD* pCrc,int32* plStatus)=0;

protected:
	BYTE temp2[256];
};

extern ISecureEngine* WINAPI GetSecureEngine();
#endif//__CSECUREENGINE_HEADER__