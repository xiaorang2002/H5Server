/*
	How to use:
	1. main call
		Lock(ctx)		: lock the mutex to give data.

	2. thread call:
		LockWait(&ctx,&cond)	: lock and wait signal.
		LockWaitTime(&ctx,&cond,100);	// millisecond.

	3. main set signal:
		Signal(&cond);		: set signal to thread. 
		Unlock();
*/


#ifndef __CRITICALSECTION_HEADER__
#define __CRITICALSECTION_HEADER__

#include "../types.h"
#include <pthread.h>
#include <sys/time.h>

// declare the special type object item.
typedef pthread_mutex_t	CRITICAL_SECTION;
typedef pthread_cond_t  CRITICAL_COND;

//
//    CCriticalSection
class CCriticalSectionEx
{
public:
	CCriticalSectionEx()
	{
		m_bUnlocked = FALSE;
		m_hCriticalSection = NULL;
    }

    CCriticalSectionEx(CRITICAL_SECTION* lpCriticalSection)
    {
        m_bUnlocked = FALSE;
        m_hCriticalSection = lpCriticalSection;
    }

    virtual ~CCriticalSectionEx()
    {
        Unlock();
    }
    
public://0=success.
    static int Create(CRITICAL_SECTION* lpCriticalSection,CRITICAL_COND* pCond=0)
    {
        //InitializeCriticalSection(lpCriticalSection);
	/*
    	pthread_mutexattr_t attr={0};
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	return (pthread_mutex_init(lpCriticalSection,&attr));
	*/

	if (pCond) pthread_cond_init(pCond,NULL);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	return (pthread_mutex_init(lpCriticalSection,&attr));
    }

    static void Destroy(CRITICAL_SECTION* lpCriticalSection,CRITICAL_COND* pCond=0)
    {
        //DeleteCriticalSection(lpCriticalSection);
	pthread_mutex_destroy(lpCriticalSection);
    }

    void Lock(CRITICAL_SECTION* lpCriticalSection=NULL)
    {
        //g_log.Dump(LOG_INFO,TEXT("Locked."));
        // try to check if the object.
        if (lpCriticalSection)
            m_hCriticalSection = lpCriticalSection;

		// try to update the flag.
		m_bUnlocked = FALSE;
		//EnterCriticalSection(m_hCriticalSection);
		pthread_mutex_lock(m_hCriticalSection);
    }

    // lock the special critical section and wait the signal now.
    void LockWait(CRITICAL_SECTION* pCtx,CRITICAL_COND* pCond)
    {
		Lock(pCtx);
		pthread_cond_wait(pCond,pCtx);
    }

	// lock the special critical section and wait the signal with timeout content.
	bool LockWaitTime(CRITICAL_SECTION* pCtx,CRITICAL_COND* pCond,int millisec)
	{
		Lock(pCtx);
		struct timeval tv;
		struct timespec outtime;
		gettimeofday(&tv,NULL);
		int nsec = (millisec % 1000);
		outtime.tv_sec  =  tv.tv_sec + millisec/1000;
		outtime.tv_nsec =  tv.tv_usec * 1000 + (nsec * 1000);
		// try to wait the special cond with timeout value item.
		int err = pthread_cond_timedwait(pCond,pCtx,&outtime);
		if (err == 0) {
			return true;
		}
	//Cleanup:
		return false;
	}

	// unlock now.
    void Unlock()
    {
        //g_log.Dump(LOG_INFO,TEXT("Unlock."));
        // try to check if the object handle now.
        if ((m_hCriticalSection)&&(!m_bUnlocked))
        {
		m_bUnlocked = TRUE;
		//LeaveCriticalSection(m_hCriticalSection);
		pthread_mutex_unlock(m_hCriticalSection);
	}
    }

    // given an signal to other thread item.
    static void Signal(CRITICAL_COND* pCond)
    {
		if (pCond) {
			pthread_cond_signal(pCond);
		}
    }

    bool TryLock(CRITICAL_SECTION* lpCriticalSection=NULL)
    {
		if (lpCriticalSection)
		{
			int status = pthread_mutex_trylock(lpCriticalSection);
			if (0 == status)
			{
				m_hCriticalSection = lpCriticalSection;
				// try to update flag.
				m_bUnlocked = FALSE;
			}

		return TRUE;
		}

	//Cleanup:
		return FALSE;
	}

protected:
    CRITICAL_SECTION* m_hCriticalSection;
    BOOL m_bUnlocked;
};




#endif//__CRITICALSECTION_HEADER__
