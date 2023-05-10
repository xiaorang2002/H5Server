#ifndef __TMUTEX_HEADER__
#define __TMUTEX_HEADER__

#include <kernel/CriticalSection.h>

// mutex class.
class CMutex
{
public:
	CMutex() {
#ifdef  _WIN32
		InitializeCriticalSection(&lock);
#else
		pthread_mutex_init(&lock, NULL);
#endif//_WIN32
	}

	virtual ~CMutex() {
#ifdef  _WIN32
		DeleteCriticalSection(&lock);
#else
		pthread_mutex_destroy(&lock);
#endif//_WIN32
	}
public:
#ifdef  _WIN32
	CRITICAL_SECTION lock;
#else
	pthread_mutex_t  lock;
#endif//_WIN32
};


// mutex auto lock.
class CAutoLock
{
public:
	CAutoLock(CMutex& mutex) : mLock(mutex) {
#ifdef  _WIN32
		EnterCriticalSection(&mLock.lock);
#else
		pthread_mutex_lock(&mLock.lock);
#endif//_WIN32
	}

	CAutoLock(CMutex* mutex) : mLock(*mutex) {
#ifdef  _WIN32
		EnterCriticalSection(&mLock.lock);
#else
		pthread_mutex_lock(&mLock.lock);
#endif//_WIN32
	}

	virtual ~CAutoLock() {
#ifdef  _WIN32
		LeaveCriticalSection(&mLock.lock);
#else
		pthread_mutex_unlock(&mLock.lock);
#endif//_WIN32
	}

private:
	CMutex& mLock;
};


#endif//__TMUTEX_HEADER__
