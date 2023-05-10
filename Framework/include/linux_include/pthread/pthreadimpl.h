/*
    Cross platform pthread thread and locker implement.
    1.create thread:
    void* thr_fn(void *arg);

    2.
    CPThreadThread::CreateThread()
    CPThreadMutex mutex;
    CPThreadMutexLock lock(&mutex);
    
*/


#ifndef __PTHREADIMPL_HEADER__
#define __PTHREADIMPL_HEADER__

#ifdef WIN32
#include <windows.h>
#else
#include <lsm/types.h>
#include <signal.h>
#endif

#include <pthread.h>

// pThread¿â.
class CPThreadThread
{
public:
	CPThreadThread()
	{
	}

public:
	static void Init()
	{
#ifdef  WIN32
		pthread_init_static();
#endif//WIN32
	}

public:
	// create the special thread item value data content now.
	static pthread_t CreateThread(void *(*start) (void *))
	{
		pthread_t tid = {0};
		int err = pthread_create(&tid,NULL,start,NULL);
		return tid;
	}

	static long TreminateThread(pthread_t tid,int nErrCode=0)
	{
		return (pthread_kill(tid,nErrCode));
	}
};

// Mutex Object item.
class CPThreadMutex
{
public:
	CPThreadMutex()
	{
		Init();
	}

public:
	long Init()
	{
		return (pthread_mutex_init(&mtx,NULL));
	}

	long Lock()
	{
		return (pthread_mutex_lock(&mtx));
	}

	long Unlock()
	{
		return (pthread_mutex_unlock(&mtx));
	}
protected:
	pthread_mutex_t mtx;
};


class CPThreadMutexLock
{
public:
	CPThreadMutexLock(CPThreadMutex* mutex,BOOL bLock=TRUE)
	{
		m_pThreadMutex = mutex;
		if (bLock) {
			m_bOwner=1;
			Lock();
		}
	}

	// Lock now.
	void Lock()
	{
		// check owner.
		if (!m_bOwner)
		{
			m_bOwner=1;
			if (m_pThreadMutex) {
				m_pThreadMutex->Lock();
			}
		}
	}

	// Unlock now.
	void Unlock()
	{
		// unlock item. 
		if (m_bOwner)
		{
			m_bOwner=0;
			if (m_pThreadMutex) {
				m_pThreadMutex->Unlock();
			}
		}
	}

protected:
	CPThreadMutex* m_pThreadMutex;
	long           m_bOwner;
};

#endif//__PTHREADIMPL_HEADER__
