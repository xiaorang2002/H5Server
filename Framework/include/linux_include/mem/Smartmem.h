/*
	临时内存管理类,

History:
	2016-12-29 17:21
		加入只传一个大小,动态分配一块内存参数.

	BUG
		如果读取的是一个字符串,需要多申请一点内存,以补充最后的\0.
*/


#ifndef __SMARTMEM_HEADER__
#define __SMARTMEM_HEADER__

#include <types.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rc4.h>

#ifdef _WIN32
#include <windows.h>
// pragma comment the special lib.
#pragma comment(lib,"libeay32.lib")
#endif//_WIN32


class CSmartmem
{
public:
	CSmartmem() : m_lpMem(0), m_nSize(0)
	{
	}

    // allocate the special memory block item now.
    CSmartmem(long size) : m_lpMem(0), m_nSize(0)
    {
        Alloc(size);
    }

	CSmartmem(void* lpszData,long nBufSize) :  m_lpMem(0), m_nSize(0)
	{
        Alloc(nBufSize);
		Copy(lpszData,nBufSize);
	}

	virtual ~CSmartmem()
	{
		Destroy();
	}

public:
	// copy the data to my buffer,nToOffset is my buffer offset.
	long Copy(void* lpszData,LONG nBufSize,LONG nToOffset=0)
	{
		LONG nStatus=-1;
		do 
		{
			// check if the input data valid.
			if ((!lpszData)||(!nBufSize)) {
				break;
			}

			// reset the copy size now.
			LONG nCopySize = nBufSize;
			if ((nBufSize+nToOffset)>m_nSize) {
				nCopySize = (m_nSize-nToOffset);
			}

			// try to get the special data item value now.
			memcpy(m_lpMem+nToOffset,lpszData,nCopySize);
			nStatus = S_OK;
		}   while (0);
	//Cleanup:
		return (nStatus);
	}

	// try to clone the special memory block.
	long Clone(void* lpszData,LONG nBufSize)
	{
		LONG nStatus=-1;
		do 
		{
			// check if the input data valid.
			if ((!lpszData)||(!nBufSize)) {
				break;
			}
			
			Destroy();
			m_nSize = nBufSize;
			// try to allocate the new buffer content.
			m_lpMem = new unsigned char[nBufSize+32];
			if (!m_lpMem) {
				nStatus=-2;
				break;
			}

			// empty the buffer item value.
			memset(m_lpMem,0,nBufSize+32);
			// try to get the special data now.
			memcpy(m_lpMem,lpszData,nBufSize);
			nStatus = S_OK;

		}   while (0);
	//Cleanup:
		return (nStatus);
	}

    // allocate the buffer.
    long Alloc(long size)
    {
        long nStatus = -1;
        do 
        {
            // delete.
            Destroy();
            m_nSize = size;
            // try to allocate the new buffer now.
            m_lpMem = new unsigned char[size+32];
            if (!m_lpMem) {
                nStatus=-2;
                break;
            }

            /// empty the buffer item.
            memset(m_lpMem,0,size+32);
            nStatus = S_OK;
        }   while (0);
    //Cleanup:
        return (nStatus);
    }

	// detach the data item.
	unsigned char* Detach()
	{
		LPBYTE lpsz = m_lpMem;
		m_lpMem = NULL;
	//Cleanup:
		return (lpsz);
	}

	////// Destroy
	void Destroy()
	{
		if (m_lpMem)
		{
			delete[] m_lpMem;
			m_lpMem = NULL;
		}
	}

	//// encrypt all data item value.
	long encrypt(unsigned long tick)
	{
		long nStatus=-1;
		unsigned char k[32]={0};
		do 
		{
			srand(tick);
			for (int i=0;i<32;i++) {
				k[i]=(rand()&0xFF);
			}

			// encrypt buffer.
			RC4_KEY key = {0};
			RC4_set_key(&key,32,k);
			RC4(&key,m_nSize,m_lpMem,m_lpMem);   
			nStatus = S_OK;

		} while (0);
	//Cleanup:
		return (nStatus);
	}

public:
	unsigned char* GetPtr()
	{
		return m_lpMem;
	}

    unsigned char* c_ptr()
    {
        return m_lpMem;
    }

	void zeromem()
	{
		if ((m_lpMem) && (m_nSize)) {
			memset(m_lpMem,0,m_nSize);
		}
	}

	unsigned char* data()
	{
		return m_lpMem;
	}

	operator unsigned char*()
	{
		return (m_lpMem);
	}

	long size()
	{
		return (m_nSize);
	}

protected:
	unsigned char* m_lpMem;
	long		   m_nSize;
};



#endif//__SMARTMEM_HEADER__
