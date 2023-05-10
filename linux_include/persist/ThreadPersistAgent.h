/*
	代理写数据线程.

history:
	2016-07-01 14:41
		修复去除重复项时,有可能不释放内存的BUG.

	2016-06-03 15:07
		优化,插入数据时去除列表中重复的数据项.
*/

#ifndef __THREADPERSISTAGENT_HEADER__
#define __THREADPERSISTAGENT_HEADER__

#include <kernel/CriticalSection.h>
#include <persist/FastPersist.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <string>
#include <vector>
//#include <deque>
#include <map>

#define THREADFILE_SIGN	(0x0547487B)

#ifdef _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32


class ThreadPersistBlock
{
public:
	ThreadPersistBlock()
	{
		vec.clear();
		file[0]= '\0';
		// data   = NULL;
		offset = 0;
		seqid  = 0;
		sign   = 0;
		// size   = 0;

	}

	virtual ~ThreadPersistBlock()
	{
		/*
		if (data) {
			delete[] data;
			data=NULL;
		}
		*/
	}

	void push_back(unsigned char* data,int size)
	{
		vec.clear();
		for (int i=0;i<size;i++) {
			vec.push_back(data[i]);
		}
	}

	unsigned char* data()
	{
		if (vec.size()) {
			return (&vec[0]);
		}

		return NULL;
	}

	long size()
	{
		return (vec.size());
	}

public:
	TCHAR  file[MAX_PATH];	// the file path name.
	unsigned long    sign;	// the sign of data now.
	// unsigned long    size;	// the size of the file.
	unsigned long  offset;
	// unsigned char*   data;
	std::vector<unsigned char> vec;
	unsigned long 	seqid;
};

//typedef std::deque<ThreadPersistBlock> VECTHFILEBLOCK;
//typedef VECTHFILEBLOCK::iterator VECTHFILEITER;

typedef std::map<std::string,ThreadPersistBlock> MAPTHFILEBLOCK;
typedef MAPTHFILEBLOCK::iterator MAPTHFILEITER;

class  CThreadPersistAgent;
extern CThreadPersistAgent* m_pThreadPersistAgent;
class  CThreadPersistAgent
{
public:
	CThreadPersistAgent()
	{
		m_nSleep = 30;
		m_nSeqid =  1;
		CCriticalSectionEx::Create(&ctx,&cond);
		m_nOneTimecnt = 256;
		m_bThread = FALSE;
		m_hThread = 0;
	}

	virtual ~CThreadPersistAgent()
	{
		// Term();
		// try to destroy the mutex item.
		CCriticalSectionEx::Destroy(&ctx,&cond);
		printf("~CThreadPersistAgent ok!\n");
	}

public:
	static CThreadPersistAgent& Instance()
	{
		if (!m_pThreadPersistAgent) {
			 m_pThreadPersistAgent = new CThreadPersistAgent();
		}
	//Cleanup:
		return (*m_pThreadPersistAgent);
	}

public:
	long start(LONG nSleep=5000,LONG nOneTimeWrite=10000)
	{
		int err = 0;
		DWORD dwThreadId;
		LONG nStatus=-1;
		m_bThread = TRUE;
		m_nSleep = nSleep;
		m_nOneTimecnt = nOneTimeWrite;
		err = pthread_create(&m_hThread,0,ThreadFileWriter,this);
		if (0 == err) {
			nStatus = S_OK;
		}
	//Cleanup:
		return (nStatus);
	}

	void Term()
	{
		CCriticalSectionEx::Signal(&cond);
		m_bThread = FALSE;
		if (m_hThread)
		{
			pthread_join(m_hThread,NULL);
			// close the handle now.
			m_hThread=0;
		}
	//Cleanup:
		delete this;
	}

public:// write file content now.
	long write(LPCTSTR file,unsigned char* data,unsigned long size,unsigned long offset=0)
	{
		long nStatus=-1;
		do 
		{
			// exchange the data now.
			CCriticalSectionEx lock;
			lock.Lock(&ctx);
			// clear the dump.
			// DropPrevSameItem(file);
			ThreadPersistBlock sp;
			sp.sign = THREADFILE_SIGN;
			sp.seqid = m_nSeqid++;
			strncpy(sp.file,file,MAX_PATH);
			sp.push_back(data,size);
			sp.offset = offset;
			// veclist.push_back(sp);
			veclist.insert(MAPTHFILEBLOCK::value_type(file,sp));

			lock.Signal(&cond);
			lock.Unlock();
			nStatus = S_OK;

		} while (0);
	//Cleanup:
		return (nStatus);
	}

	// try to force write the special content file item data for later user content item content.
	long force_write(LPCTSTR file,unsigned char* data,unsigned long size,unsigned long offset=0)
	{
		long status = -1;
		do 
		{
			// try to check the input parameters.
			if ((!file)||(!data)||(!size)) break;
			// write the file.
			CFastpersist  ofs;
			if (ofs.Openwrite(file)) {
				ofs.WriteOffset(offset,data,size);
				ofs.Close();
			}

		} while (0);
	//Cleanup:
		return (status);
	}

	/*
	// clear dump file from the list now.
	long DropPrevSameItem(LPCTSTR lpszFile)
	{
		VECTHFILEITER iter;
		//CCriticalSectionEx lock;
		//lock.Lock(&ctx);
		iter = veclist.begin();
		while (iter != veclist.end())
		{
			ThreadPersistBlock& sp = *iter;
			if ((&sp) && (sp.sign == THREADFILE_SIGN) &&  
			    (strcasecmp(lpszFile,sp.file)==0))
			{
				iter = veclist.erase(iter);
				// delete sp; // delete item.
			}
			else
			{
				iter++;
			}
		}
	//Cleanup:
		return (S_OK);
	} */


public:
	long _dowrite()
	{
		long nStatus =-1;
		while (m_bThread)
		{
doWrite:
			MAPTHFILEBLOCK vecdump;
			// VECTHFILEBLOCK vecdump;
			// try to get the special block data item now.
			CCriticalSectionEx lock;
			lock.LockWait(&ctx,&cond);
			vecdump.swap(veclist);
			lock.Unlock();			
			// loop to write now.
			if (vecdump.size())
			{
				// VECTHFILEITER iter;
				MAPTHFILEITER iter;
				for (iter=vecdump.begin();iter!=vecdump.end();iter++) {
					ThreadPersistBlock& sp = iter->second;
					if (&sp)
					{
						// check the file sign and data the file sign data.
						if ((sp.data()) && (THREADFILE_SIGN == sp.sign))
						{
							// write the file.
							CFastpersist ofs;
							if (ofs.Openwrite(sp.file)) {
							    ofs.WriteOffset(sp.offset,sp.data(),sp.size());
							    ofs.Close();
							}
						}
						else
						{
//printf("check error,sp:[%x],sp->sign:[%d]\n",&sp,sp.sign);
						}
						// delete.
						// delete sp;
					}
				}
				// delete all now.
				vecdump.clear();
			}

			usleep(m_nSleep*1000);
		};

		// check if break value.
		CCriticalSectionEx lock;
		lock.Lock(&ctx);
		long size = veclist.size();
		lock.Unlock();
		if (size)
		{
			// do the left all item.
			m_nOneTimecnt=(DWORD)0x7FFFFFFF;
			goto doWrite;
		}
	//Cleanup:
		return (nStatus);
	}


protected:
	static void* ThreadFileWriter(void* args)
	{
		CThreadPersistAgent* sp = (CThreadPersistAgent*)args;
		if (sp) {
		    sp->_dowrite();
		}
	//Cleanup:
		return (0);
	}

protected:
	MAPTHFILEBLOCK   veclist; 	// the map list of blocks.
	unsigned long m_nOneTimecnt;	// one time write content.
	unsigned long m_nSleep;		// delay time for one loop.
	unsigned long m_nSeqid;
	CRITICAL_SECTION   ctx;
	CRITICAL_COND	  cond;
	pthread_t    m_hThread;
	BOOL         m_bThread;
};


#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32


#endif//__THREADPERSISTAGENT_HEADER__
