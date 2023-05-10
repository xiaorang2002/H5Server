

#ifndef __LIBNET_HEADER__
#define __LIBNET_HEADER__

#include <types.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "iocpheader.h"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_istream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
//#include <logger/LogWarn.h>

using namespace acl;
// update net engine.
class CNetEngine
{
public:
	CNetEngine()
	{
		acl_cpp_init();
		phandle = new aio_handle(ENGINE_KERNEL);
		phandle->set_delay_usec(10000);
		phandle->set_delay_sec(0);
	}

	static CNetEngine& singleton()
	{
		static CNetEngine obj;
		return obj;
	}

	static CNetEngine* singletonp()
	{
		CNetEngine& obj = singleton();
		return &obj;
	}

public:
	aio_handle& gethandle()
	{
		return *phandle;
	}

	// check now.
	bool update()
	{
		bool bSuccess = false;
		if (phandle) {
			bSuccess = phandle->check();
		}
		//Cleanup:
		return false;
	}

protected:
	aio_handle *phandle;
};

/// 阻塞式socket客户端.
class CIOCPClient :  public IIOCPClient
{
public:
    CIOCPClient()
    {
		m_hSocket = 0;
    }

	~CIOCPClient()
	{
		if (m_hSocket) {
			close(m_hSocket);
			m_hSocket=0;
		}
	}

    // 功能函数.
public:
    int Connect(const TCHAR* ip,WORD port)
    {
		int status = -1;
		struct sockaddr_in addr={0};
		do
		{
			/// try to connect to the remote host now.
			m_hSocket = socket(AF_INET,SOCK_STREAM,0);
			if (m_hSocket <= 0) {
				status = -2;
				break;
			}
	
			// set no block mode.
			unsigned long ul=1;
			ioctl(m_hSocket,FIONBIO,&ul);
			// initialize the address.
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_aton(ip,&addr.sin_addr);
			int ret = connect(m_hSocket,(struct sockaddr*)&addr,sizeof(addr));
			if (ret == -1)
			{
				fd_set set;
				timeval tv={15,0}; // 15 second timeout.
				FD_ZERO(&set);
				FD_SET(m_hSocket,&set);
				socklen_t len = sizeof(int);
				if (select(m_hSocket+1,NULL,&set,NULL,&tv) > 0)
				{
					// try to get the special socket error code value item now.
					getsockopt(m_hSocket,SOL_SOCKET,SO_ERROR,&ret,(socklen_t*)&len);
				}
			}

			ul = 0;
			ioctl(m_hSocket,FIONBIO,&ul);
			if (ret != 0) {
				close(m_hSocket);
				m_hSocket=0;
			}
			else
			{
				// connect ok.
				status = 0;
			}
	
		} while (0);
	//Cleanup:
        return (status);
    }

    /// send the special content value and receive the remote host data with ensure len.
    int SendReceive(BYTE* lpszSendBuf,int32 nSendlen,BYTE* lpszToRead,uint32* nByteToRead)
    {
		int32 nErrorCode = IOCPERR_UNK;

		IOCPPACK_HEAD* pHead = (IOCPPACK_HEAD*)bufSend;

		pHead->signature = IOCPLIB_SIGN;
		pHead->nlength   = sizeof(IOCPPACK_HEAD) + nSendlen;
		nSendlen += sizeof(IOCPPACK_HEAD);
		pHead++;

		memmove(pHead,lpszSendBuf,nSendlen);

		do 
		{
			int32 nByteToSend = nSendlen;
			BYTE* lpszToSend  = bufSend;
			int32 nByteSent   = 0;

			while (nByteSent < nSendlen)
			{
				nErrorCode = send(m_hSocket, (const char*)lpszToSend, nByteToSend, 0);
				if (nErrorCode < 0)
				{
					nErrorCode = WSAGetLastError();
					Close();
					return nErrorCode;
				}

				// closed by server.
				if (nErrorCode == 0)
				{
					break;
				}

				// update the data pointer.
				nByteSent   += nErrorCode;
				lpszToSend  += nErrorCode;
				nByteToSend -= nErrorCode;
			}

			//数据未发全
			if (nByteSent != nSendlen)
			{
				return IOCPERR_TIMEOUT;
			}

			BOOL bfirst = TRUE;			//第一次接收到数据
			int32 nLength = 0;			//应接收的数据长度

			int32 nRealBufLen = *nByteToRead;	//接收缓冲区大小

			int32 nReceiveDataLen = sizeof(IOCPPACK_HEAD);	//接收缓冲区大小

			*nByteToRead = 0;
			//函数Receive在接收完数据之前不会返回，可以确保接收到所有的数据.
			while(TRUE)		
			{
				int nRec = recv(m_hSocket,
						(char*)lpszToRead + *nByteToRead, 
						nReceiveDataLen   - *nByteToRead,
						0);

				// check if socket error or has closed now.
				if ((nRec == SOCKET_ERROR)||(nRec == 0))
				{
					nErrorCode = WSAGetLastError();
					Close();
					return nErrorCode;
				}

				*nByteToRead += nRec;
				// check if head has been received content.
				if (*nByteToRead >= sizeof(IOCPPACK_HEAD))
				{
					// real length.
					if (bfirst)
					{
						IOCPPACK_HEAD* ptr = (IOCPPACK_HEAD*)lpszToRead;
						nReceiveDataLen = ptr->nlength;
						nLength = ptr->nlength;
						bfirst = FALSE;
					}

					//判断数据接收是否完毕.
					if (*nByteToRead == nLength && *nByteToRead != 0)
					{
						int32 headlen = sizeof(IOCPPACK_HEAD);
						// 丢弃掉头信息,并修正数据大小.
						if (nLength>=headlen)
						{
							nLength -= headlen;
							memmove(lpszToRead,lpszToRead+headlen,nLength);
							*nByteToRead = nLength;
						}

						nErrorCode = S_OK;
						break;
					}
				}
				else
				{
					continue;
				}

				usleep(5000);
			}

		} while (0);

	//Cleanup:
		return (nErrorCode);
    }

    //// close the socket now.
    int Close(BOOL bDestroy=FALSE)
	{
		if (m_hSocket) {
			close(m_hSocket);
			m_hSocket=0;
		}

		// delete the pointer now.
		if (bDestroy) delete this;
		return 0;
	}

    // get sid is no IMPL.
    virtual int Getsid()
    {
        return 0;
    }

public:
	DWORD GetTickCount()
	{
		DWORD dwTick = 0;
#ifdef _WIN32
		dwTick = GetTickCount();
#else
		timeval tv;
		gettimeofday(&tv,NULL);
		dwTick = (((tv.tv_sec*1000)+(tv.tv_usec/1000))/200000000);
#endif//_WIN32
	}

protected:
	BYTE bufSend[16384];	// the send buffer now.
	SOCKET    m_hSocket;	// the socket handler.
};



/// client socket data item content value item now.
class CIOCPServerSocket : public aio_open_callback,
	public IIOCPClient

{
public:
	CIOCPServerSocket(IIOCPServerEvents* sinki=0)
	{
		bheader = 1;
		sink    = sinki;
		stream  = 0;
	}

public:// set the special stream content item value.
	int setstream(void* istream,unsigned int nsid)
	{
		sid  = nsid;
		stream = (aio_socket_stream*)istream;
		return 0;
	}

	/// set the special sink content now.
	int setsink(IIOCPServerEvents* sinki)
	{
		sink = sinki;
		return 0;
	}

	// 功能函数.
public:
	virtual int32 Connect(const TCHAR* lpszAddr,WORD nPort)
	{
		// no connect for server socket.
		return 0;
	}

	/// send the special content value and receive the remote host data with ensure len now.
	int SendReceive(BYTE* lpszSendBuf,int32 nSendlen,BYTE* lpszToRead,uint32* nByteToRead)
	{
		int32 nErrorCode = IOCPERR_UNK;

		if (stream) {

			IOCPPACK_HEAD header;
			header.signature = IOCPLIB_SIGN;
			header.nlength   = sizeof(IOCPPACK_HEAD) + nSendlen;
			stream->write(&header,sizeof(header));

			// try to send the remaind data value now.
			stream->write(lpszSendBuf,nSendlen);

			nErrorCode = IOCPERR_OK;
		}
	//Cleanup:
		return (nErrorCode);
	}

	///// close the socket now.
	int Close(BOOL bDestroy);
	// get the special id.
	virtual int Getsid()
	{
		return sid;
	}

	// update the socket.
	virtual void update()
	{
		// no update for server socket.
		return;
	}

public:	// callbacks.
	virtual bool open_callback()
	{
		/*
		// sink now.
		if (sink) {
			// Use as OnConnected.
			sink->OnAccept(0,0);
		}
		*/
	//Cleanup:
		return true;
	}

protected:
	// read callback function content value content.
	virtual bool read_callback(char* data, int len)
	{
		int headsize = sizeof(IOCPPACK_HEAD);
		do
		{
			//logWarn("bheader:[%d],data:[%x],len:[%d]\n",bheader,data,len);
			if (bheader)
			{
				// try to get the special header content item.
				IOCPPACK_HEAD* pHeader = (IOCPPACK_HEAD*)data;
				if (!pHeader) {
					break;
				}

				// try to update special data length content. 
				int datalen = (pHeader->nlength - headsize);
				if (datalen<0) {
					datalen=0;
				}

				// read data.
				bheader = 0;
				/// read data content.
				stream->read(datalen);
			}
			else
			{
				// callback.
				if (sink) {
					sink->OnReceive((WORD)sid,data,len);
				}

				//read header.
				bheader = 1;
				if (stream) {
					stream->read(sizeof(IOCPPACK_HEAD));
				}
			}

		} while (0);

	//Cleanup:
		return true;
	}

	/// closed callback function.
	virtual void close_callback();

protected:
	aio_socket_stream* stream;  // socket stream item.
	IIOCPServerEvents*   sink;  // socket sink   item.
	unsigned int          sid;  // socket session  id.
	unsigned int	  bheader;  // is reading  header.
};



#endif//__LIBNET_HEADER__
