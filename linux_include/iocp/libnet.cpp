
#include <map>
#include "libnet.h"

#ifdef _WIN32
#else
#include <unistd.h>
#endif//_WIN32

// socket ip address content.
#include <socket/IpAddress.h>

// declare the special map to hold all the content value.
typedef std::map<unsigned int,IIOCPClient*> MAPSESSION;
class CSessionpool
{
public:
    CSessionpool() {}

public:
    static CSessionpool& singleton()
    {
        static CSessionpool obj;
        return obj;
    }

    // add socket session pool.
    int add(IIOCPClient* socki)
    {
        int status = -1;
        do 
        {
            if (!socki) break;
            unsigned int sid = socki->Getsid();
            // find the special content item value content.
            MAPSESSION::iterator iter = mapsession.find(sid);
            if (iter != mapsession.end()) {
                iter->second = socki;
            }   else
            {
                // insert the special content item value content now.
                mapsession.insert(MAPSESSION::value_type(sid,socki));
            }

            status = 0;
        } while (0);
        //Cleanup:
        return (status);
    }

	/// try to get the special client and erase it or not.
	IIOCPClient* get(unsigned int sid,BOOL berase=false)
	{
		IIOCPClient* sp = 0;
		do 
		{
			//// find the special content item value content.
			MAPSESSION::iterator iter = mapsession.find(sid);
			if (iter != mapsession.end()) {
				sp = iter->second;
				if (berase) {
					mapsession.erase(iter);
				}
			}
		} while (0);
	//Cleanup:
		return (sp);
	}

    // delete the special object.
    int del(IIOCPClient* pSock)
    {
        int status = -1;
        do 
        {
            if (!pSock) break;
            unsigned int sid = pSock->Getsid();
            MAPSESSION::iterator iter = mapsession.find(sid);
            if (iter != mapsession.end()) {
                IIOCPClient* sp = iter->second;
                mapsession.erase(iter);
            }

        } while (0);
        //Cleanup:
        return (status);
    }

	// client count.
	int32 size()
	{
		mapsession.size();
	}


protected:
    MAPSESSION mapsession;
};

// declare the server socket item data item now.
class CIOCPServer : public aio_accept_callback,
                    public IIOCPServer
{
public:
    CIOCPServer(IIOCPServerEvents* sinki)
    {
        sid  = 1;
        sink = sinki;
    }

public:
    bool accept_callback(aio_socket_stream* stream)
    {
        CIOCPServerSocket* sock = new CIOCPServerSocket(sink);
      	if (sock)
        {
            stream->set_buf_max(0);
            sock->setstream(stream,sid);
            stream->add_read_callback(sock);
            stream->add_open_callback(sock);
            stream->add_close_callback(sock);
            stream->add_timeout_callback(sock);
            CSessionpool::singleton().add(sock);
            stream->read(sizeof(IOCPPACK_HEAD));
            if (sink) {
				const char* ip = stream->get_peer();
                uint32 addr = CIpAddress::StringIpToInt(ip);
				int status = sink->OnAccept(sid,addr);
				if (S_OK == status) {
					// try to call callback.
            		sock->open_callback();
				}
            }
        
			// try to get current client pool size item.
			int size = CSessionpool::singleton().size();
			printf("client pool size:[%d]\n",size);
            // add sid now.
            sid = sid + 1;
        }
    //Cleanup:
        return true;
    }

public:
    virtual int32 Initialize(const TCHAR* ip, WORD port, WORD nMaxClient=3000)
    {
        int status = -1;
        do 
        {
            char addr[32]={0};
            wsprintfA(addr,"%s:%d",ip,port);
            aio_handle& handle = CNetEngine::singleton().gethandle();
            aio_listen_stream* sstream = new aio_listen_stream(&handle);
            if (sstream->open(addr) == false) {
                CNetEngine::singleton().update();
                status = -2;
                break;
            }

            // add the accept callback item now.
            sstream->add_accept_callback(this);
            status = 0;
        }   while (0);
    //Cleanup:
        return (status);
    }

	// 断开客户端
	virtual int32 Disconnect(WORD nClient)
	{
		int32 status = -1;
		do 
		{
			// try to get the special client item from pool and delete it.
			IIOCPClient* sp = CSessionpool::singleton().get(nClient,true);
			if (sp) {
				sp->Close(false);
				status = 0;
			}
		} while (0);
	//Cleanup:
		return (status);
	}

	// 发送数据
	virtual int32 SendData(WORD nClient, void* data, DWORD len)
	{
		int sended = 0;
		do 
		{
			// try to get the special client and send data for it now.
			IIOCPClient* sp = CSessionpool::singleton().get(nClient);
			if (sp) {
				sended = sp->SendReceive((BYTE*)data,len,0,0);
			}
		} while (0);
	//Cleanup:
		return (sended);
	}

	// update function now.
	virtual void update()
	{
		CNetEngine::singleton().update();
	}

protected:
    unsigned int        sid;   // session id.
    IIOCPServerEvents* sink;   // socket sink.
};

/// destroy the special socket value item now.
int CIOCPServerSocket::Close(BOOL bDestroy)
{
	// stream item.
	if (stream) {
		stream->close();
		stream=0;
	}

	// destroy item.
	if (bDestroy) {
		delete this;
	}
//Cleanup:
	return 0;
}

// closed callback for server socket now.
void CIOCPServerSocket::close_callback()
{
	// callback.
    if (sink) {
        sink->OnClose((WORD)sid);
    }

	// delete item from the session pool.
	CSessionpool::singleton().del(this);
    // destroy item.
    this->Close(1);
}


//// create the special client content.
IIOCPClient* WINAPI CreateIOCPClient()
{
    CIOCPClient* sock = new CIOCPClient();
//Cleanup:
    return (sock);
}

// create the special iocp server data item value server item now.
IIOCPServer* WINAPI CreateIOCPServer(IIOCPServerEvents* pEvents)
{
    CIOCPServer* pSocket = new CIOCPServer(pEvents);
    return pSocket;
}
