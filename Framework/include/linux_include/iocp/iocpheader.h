
#ifndef __IOCP_HEADER__
#define __IOCP_HEADER__

#include <types.h>

// #ifdef _WIN32
// #include <Windows.h>
// #pragma comment(lib,"IOCPLib.lib")
// #endif//_WIN32

#define IOCPLIB_SIGN	(0x50434F49)

// error code declare.
enum tagIOCPErrorCodes
{
	IOCPERR_UNK    =-1,
	IOCPERR_OK     = 0,
	IOCPERR_INIT   = 1,	// WSAStartup failed.
	IOCPERR_VER    = 2,	// Invalidate socket library version.
	IOCPERR_SOCK   = 3,	// Create socket error.
	IOCPERR_BIND   = 4,	// bind socket failed.
	IOCPERR_LISTEN = 5,	// listen socket failed.
	IOCPERR_CRPORT = 6,	// create IO port failed.
	IOCPERR_WORKER = 7, // create worker thread failed.
	IOCPERR_ACCEPT = 8, // create accept thread failed.
	IOCPERR_NOFIND = 9, // find socket has failed.
	IOCPERR_CLOSE  =10, // close socket error.
	IOCPERR_INVALID=11, // invalidate socket.
	IOCPERR_BUFNUL =12, // buffer is null.
	IOCPERR_ADDR   =13, // invalidate ip.
	IOCPERR_TIMEOUT=14, // communication timeout.

};

// IOCPPACK包头.
typedef struct tagIOCPPACKHEAD
{
	int32 signature;	// 包头标识.
	int32 nlength;	// 总数据长度.
	int32 errorcode;	// 错误号.
	int32 reserved;	// 保留.
} IOCPPACK_HEAD;


// IOCPServer函数.
struct IIOCPServer
{
public:
	// 初始化
	virtual int32 Initialize(const TCHAR* ip, WORD nPort, WORD nMaxClient=3000)=0;

	// 断开客户端
	virtual int32 Disconnect(WORD nClient)=0;

	// 发送数据
	virtual int32 SendData(WORD nClient, void* data, DWORD len)=0;

	// update for network now.
	virtual void update()=0;

};

//IOCPServer回调函数.
struct IIOCPServerEvents
{
public:
	// 事件通知
	virtual int32 OnAccept(WORD nClient,DWORD dwClientAddr) = 0;
	virtual int32 OnClose(WORD nClient) = 0;
	virtual int32 OnSend(WORD nClient, char* pData, DWORD dwDataLen) = 0;
	virtual int32 OnReceive(WORD nClient, char* pData, DWORD dwDataLen) = 0;
	virtual int32 OnError(WORD nClient, int iError) = 0;
};

// IOCPClient接口.
struct IIOCPClient
{
	// 接口函数.
public: // 注意必须UNICODE!
	virtual int32 Connect(const TCHAR* lpszAddr,WORD nPort) = 0;
	virtual int32 SendReceive(BYTE* lpszToSend,int32 dwBytesToSend,BYTE* lpszToRead,uint32* pdwBytesToRead) = 0;
	virtual int32 Close(BOOL bDestroy) = 0;
	virtual int32 Getsid() = 0;
};


/// the function to create the IOCP server for output user to hide the IOCP.
extern IIOCPServer* WINAPI CreateIOCPServer(IIOCPServerEvents* pEvents=0);
extern IIOCPClient* WINAPI CreateIOCPClient();

#endif//__IOCP_HEADER__
