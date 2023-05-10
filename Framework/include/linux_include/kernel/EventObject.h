
#ifndef __MYEVENT_HEADER__
#define __MYEVENT_HEADER__

#include "../types.h"
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef INFINITE
#define INFINITE	(-1)
#endif//INFINITE

class CEventObject
{
public:
    CEventObject()
    {
		m_bMutexInited = FALSE;
		m_bCondInited  = FALSE;
	}
    
    ~CEventObject()
    {
        Term();
    }
    
    bool Init(bool bManualReset, bool bInitialStatus)
    {
        Term();        
        //m_hEvent = (void*)CreateEvent( NULL, bManualReset, bInitialState, NULL );
        //return (m_hEvent != NULL);
    	m_bManualReset = bManualReset;
		m_bEventStatus = bInitialStatus;
		if (!m_bMutexInited) 
		{
			if (0 == pthread_mutex_init(&m_mutex,NULL))
			{
				m_bMutexInited = TRUE;
			}
		}

		if (!m_bCondInited)
		{
			if (0 == pthread_cond_init(&m_cond,NULL))
			{
				m_bCondInited = TRUE;
			}
		}
	//Cleanup:
		return ((m_bMutexInited) && (m_bCondInited));
	}
    
    void Term()
    {
        if (m_bMutexInited)
        {
            //CloseHandle( (HANDLE)m_hEvent );
            //m_hEvent = NULL;
			pthread_mutex_destroy(&m_mutex);
			m_bCondInited = FALSE;
		}

		if (m_bCondInited)
		{
			pthread_cond_destroy(&m_cond);
			m_bCondInited = FALSE;
		}
    }
    
    void* GetPtr() const
    {
        return (void*)&m_mutex;
    }
    
    BOOL SetEvent()
    {
        //bSucc = ::SetEvent((HANDLE)m_hEvent);
        if (!EnsureInited()) {
			return FALSE;
		}

		// try to lock the special mutex now.
		pthread_mutex_lock(&m_mutex);
		m_bEventStatus = TRUE;
		pthread_cond_broadcast(&m_cond);
		pthread_mutex_unlock(&m_mutex);
    //Cleanup:
        return (TRUE);
    }
    
    BOOL ResetEvent()
    {
        //bSucc = ::ResetEvent((HANDLE)m_hEvent);
        if (!EnsureInited())
		{
			return FALSE;
		}

		// try to lock the mutex
		pthread_mutex_lock(&m_mutex);
		m_bEventStatus = FALSE;
		pthread_mutex_unlock(&m_mutex);
    //Cleanup:
        return (TRUE);
    }

	BOOL WaitEvent(int nMilliSeconds=INFINITE)
	{
		if (!EnsureInited()) {
			return FALSE;
		}

		int error = 0;
		// lock the special mutex now.
		pthread_mutex_lock(&m_mutex);

		if (nMilliSeconds != INFINITE)
		{
			struct timeval tv;
			gettimeofday(&tv,NULL);

			struct timespec ts;
			ts.tv_sec = tv.tv_sec + (nMilliSeconds / 1000);
			ts.tv_nsec = tv.tv_usec * 1000 + (nMilliSeconds / 1000) * 1000000;

			if (ts.tv_nsec >= 1000000000) {
				ts.tv_nsec -= 1000000000;
				ts.tv_sec++;
			}

			while ((!m_bEventStatus) && (error == 0))
			{
				error = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
			}
		}
		else
		{
			while ((!m_bEventStatus) && (error == 0))
			{
				error = pthread_cond_wait(&m_cond, &m_mutex);
			}
		}

		if (0 == error && !m_bManualReset)
		{
			m_bEventStatus = FALSE;
		}
	//Cleanup:
		pthread_mutex_unlock(&m_mutex);
		return (0 == error);
	}

protected:
	BOOL EnsureInited()
	{
		return (m_bMutexInited && m_bCondInited);
	}
    
private:
    pthread_mutex_t m_mutex;	// the mutex object.
	pthread_cond_t   m_cond;	// condition object.
	BOOL 	 m_bMutexInited;	// bool mutex inited.
	BOOL 	  m_bCondInited;	// bool cond inited.
	BOOL	 m_bEventStatus;	// if event has singal.
	BOOL	 m_bManualReset;	// manual reset singal.
};


#endif//__MYEVENT_HEADER__
