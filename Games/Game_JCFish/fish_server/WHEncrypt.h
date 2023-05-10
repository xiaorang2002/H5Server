#pragma once

// #include "ServiceCoreHead.h"

//////////////////////////////////////////////////////////////////////////////////

//宏定义
#define XOR_TIMES					8									//加密倍数
#define MAX_SOURCE_LEN				64									//最大长度
#define MAX_ENCRYPT_LEN				(MAX_SOURCE_LEN*XOR_TIMES)			//最大长度

//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#else

#define LEN_MD5		(33)

#endif//_WIN32


//加密组件
class /*SERVICE_CORE_CLASS*/ CWHEncryptClone
{
	//函数定义
public:
	//构造函数
	CWHEncryptClone();
	//析构函数
	virtual ~CWHEncryptClone();

	//加密函数
public:
	//生成密文
	static bool MD5Encrypt(LPCTSTR pszSourceData, TCHAR szMD5Result[LEN_MD5]);

	//加密函数
public:
	//生成密文
	static bool XorEncrypt(LPCTSTR pszSourceData, LPTSTR pszEncrypData, WORD wMaxCount);
	//解开密文
	static bool XorCrevasse(LPCTSTR pszEncrypData, LPTSTR pszSourceData, WORD wMaxCount);

	//加密函数
public:
	//生成密文
	static bool MapEncrypt(LPCTSTR pszSourceData, LPTSTR pszEncrypData, WORD wMaxCount);
	//解开密文
	static bool MapCrevasse(LPCTSTR pszEncrypData, LPTSTR pszSourceData, WORD wMaxCount);

	//加密函数
public:
	//生成密文
	static bool DesEncrypt(LPCTSTR pszSourceData, LPTSTR pszEncrypData, WORD wMaxCount);
	//解开密文
	static bool DesCrevasse(LPCTSTR pszEncrypData, LPTSTR pszSourceData, WORD wMaxCount);
};

//////////////////////////////////////////////////////////////////////////////////
