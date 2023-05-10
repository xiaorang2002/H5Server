/*
Description:
    implement to lock object operation.

Info:
    create Mutex return return handle,
            
        don't save to m_hMutex object.

    Attach operation like CMutexObject(hMutex);

    Samples:

        HANDLE hLock = lock.Create(TEXT("test"));

        lock.Attach(hLock);

        LONG l = lock.Close(hLock);

History:
	2017-07-14 08:48 修改lock返回值为bool,增加trylock函数.

    2011-08-25 created by James.
*/



#ifndef __LOCKOBJECT_HEADER__
#define __LOCKOBJECT_HEADER__

#include <types.h>
#include <pthread.h>

#ifndef INFINITE
#define INFINITE	(-1)
#endif//INFINITE

class CMutexObject
{
public:
    CMutexObject()
    {
        m_hMutex = NULL;
    }

    CMutexObject(pthread_mutex_t* hMutex)
    {
        m_hMutex = hMutex;
    }

    virtual ~CMutexObject()
    {
        // unlock automatic.
        Unlock();
    }

public:
    /*
        bInitialOwner : 是不是马上锁定Mutex,效果跟WaitForSingleObject相似。
        这块千万要注意,实验发现,如果bInitialOwner为TRUE.
        在第二次创建Mutex对象,错误代码ERROR_FILE_EXIST。虽然可以成功打开Mutex对象,
        但是对象不会被锁定!!! [建议bInitialOwner永远为FALSE,通过手动调用来Lock锁定]
    */

    // try to create the Mutex object content value item now.
    static pthread_mutex_t* Create(pthread_mutex_t* phMutex)
    {
		pthread_mutexattr_t attr;
		pthread_mutex_t* sp = NULL;
		do
		{
			/// check the mutex.
			if (!phMutex) break;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(phMutex,&attr);
			sp = phMutex;
		} while (0);
	//Cleanup:
		return (sp);
    }
	
    /// try to close the Mutex handle content item.
    static LONG Destroy(pthread_mutex_t* phMutex)
    {
        if (phMutex)
        {
			//CloseHandle(hMutex);
			//hMutex = NULL;
			pthread_mutex_destroy(phMutex);
			phMutex=NULL;
        }
    //Cleanup:
        return (S_OK);
    }

    // close the special mutex handle item now.
    static LONG Close(pthread_mutex_t* phMutex)
    {
        return (Destroy(phMutex));
    }

public:
    /// try to lock out the mutex now out the mutex contnet value now.
    BOOL Lock(pthread_mutex_t* phMutex=NULL,DWORD dwTimeout=INFINITE)
    {
		BOOL bSucc = FALSE;
        if (phMutex)
		{
            // Update mutex now.
            m_hMutex = phMutex;
			//if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex,dwTimeout))
			if (0 == pthread_mutex_lock(phMutex))
			{
				bSucc = TRUE;
			}
        }
    //Cleanup:
        return (bSucc);
    }

	// try to lock the special mutex item value.
	BOOL tryLock(pthread_mutex_t* phMutex=NULL)
	{
		BOOL bSucc = FALSE;
		if (phMutex)
		{
			m_hMutex = phMutex;
			if (0 == pthread_mutex_trylock(m_hMutex))
			{
				return TRUE;
			}
		}
	//Cleanup:
		return (FALSE);
	}


    // unlock now.
    void Unlock()
    {
        // check object.
        if (m_hMutex)
        {
            //ReleaseMutex(m_hMutex);
			pthread_mutex_unlock(m_hMutex);
			m_hMutex = NULL;
        }
    }

protected:
    pthread_mutex_t* m_hMutex;
};




#endif//__LOCKOBJECT_HEADER__
