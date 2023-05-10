/*
    这个接口主要完成对配置数据的读取.
*/

#ifndef __ISECUREHOST_HEADER__
#define __ISECUREHOST_HEADER__

#include <types.h>
#include <socket/IpAddress.h>
#include <PathMagic.h>
// #include <file/FileHelp.h>
#include <random/Random.h>
#include <openssl/rc4.h>

#ifdef  _WIN32
#include <tchar.h>
#pragma comment(lib,"libeay32.lib")
#endif//_WIN32

#define SECUREHOST_SIGN	(0xED43C631)
#define SECUREHOST_V1	(0x10000000)

#ifdef  _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32

// 定义一组服务器的端口地址.
// size = 16;
typedef struct tagSecureHostItem
{
	uint32 sign;			// 包头标示.
	uint32 host;			// 主机地址.
	uint32 port;			// 主机端口.
	uint32 resv;			// 保留字节.
}   SecureHostItem;

// 定义所有完整的配置信息列表.
typedef struct tagSecureHostInfo
{
	uint32  sign;		// 配置信息列表.
	uint32  version;		// 版本标识.
	uint32  vid;			// 厂商编号.
	uint32  pid;			// 产品编号.
	uint32  count;		// 主机数量.
	SecureHostItem item[32];	// 主机列表.
	uint32  resv[890];	// 保留数据.
	uint32  crc;			// 校验字节.
}   SecureHostInfo;

// 1.1:定义一个保存远程服务器地址端口的结构体.
typedef struct tagRemoteHostInfo
{
	TCHAR szIp[32];
	unsigned short port;
}   RemoteHostInfo;


// 配置读取接口标识.
class ISecureHostReader
{
public:
	virtual int32 LoadHostInfo(SecureHostInfo* pInfo,LPCTSTR lpszFile=NULL) = 0;
};

// 读取配置数据.
class CSecureHostReader : public ISecureHostReader
{
public:
	CSecureHostReader()
	{
		idx = 0;
		memset(&info,0,sizeof(info));
		info.sign = SECUREHOST_SIGN;
		info.version =SECUREHOST_V1;
		info.vid	 = 1087;
		info.pid	 = 0001;
		info.count   = 1;	// default host.
		info.item[0].sign = SECUREHOST_SIGN;
		info.item[0].host = 0x7f000001;
		info.item[0].port = 8000;
	}

	virtual ~CSecureHostReader()
	{

	}

public://读取服务器配置.
	virtual int32 LoadHostInfo(SecureHostInfo* pInfo,LPCTSTR lpszFile=NULL)
	{

		int32 nStatus=-1;
		do 
		{
			// get path name.
			if (!lpszFile) {
				 lpszFile = CPathMagic::Build(CPathMagic::Current(),TEXT("AicsEngine.dat"));
			}

			// default it.
			if (pInfo)  {
				memcpy(pInfo,&info,sizeof(SecureHostInfo));
			}

			// try to load the special file item now.
			FILE* fp = _tfopen(lpszFile, TEXT("rb"));
			fseek(fp, 0, SEEK_END);
			int32 nSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			if (nSize > 8192) nSize = 8192;
			// try to read the content.
			BYTE lpszData[8192] = { 0 };
			fread(lpszData, 1, nSize, fp);
			fclose(fp);
			
			SecureHostInfo* pTmp=(SecureHostInfo*)lpszData;
			// generic key.
			CRandom random;
			BYTE bufkey[32]={0};
			random.Random_Seed(SECUREHOST_SIGN);
			for (int i=0;i<32;i++) {
				bufkey[i]=random.Random_Int(0,255);
			}

			// decrypt buffer.
			RC4_KEY key = {0};
			RC4_set_key(&key,32,bufkey);
			RC4(&key,nSize,lpszData,lpszData);   
			// Computer the special crc value.
			uint32 crc=SECUREHOST_SIGN;
			int32 crcSize = nSize-sizeof(int32);
			for (int i=0;i<crcSize;i++) {
				crc ^= lpszData[i];
			}
	
			// try to check crc result.
			if ((crc == pTmp->crc) && 
				(SECUREHOST_SIGN == pTmp->sign)) {
					memcpy(pInfo,pTmp,sizeof(SecureHostInfo));
					nStatus = S_OK;
			}   else
			{
				// crc failed.
				nStatus = -5;
			}

		} while (0);
	//Cleanup:
		return (nStatus);
	}

protected:
	SecureHostInfo info;	// 服务器信息列表.
	uint32   idx;	// 当前服务器的id.
};

// 获取读取配置信息工具类的接口.
inline ISecureHostReader* GetHostReader()
{
	static CSecureHostReader obj;
	return &obj;
}

#ifdef  _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32


#endif//__ISECUREHOST_HEADER__
