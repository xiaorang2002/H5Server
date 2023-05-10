#ifndef __I_TIMER_EVENT_H__
#define __I_TIMER_EVENT_H__

#include <kernel/CriticalSection.h>

#ifdef _WIN32
#else
#include <unistd.h>
#include <pthread.h>
#endif//_WIN32

struct TimerItem
{
	WORD   dwTimerID;
	LONG   dwTimeInterval;
	LONG   dwTimeElapsed;
	DWORD  dwRepeatTimes;
	WPARAM dwBindParameter;
};

#define MAX_TIMER_ITEMS 10
#define TIMER_SPACE 50

class ITimerEvent
{
public:
	ITimerEvent() : m_timerItemCount(0), m_exitThread(false)
	{
		CCriticalSectionEx::Create(&m_CriticalSection);
#ifdef _WIN32
		m_thread = CreateThread(NULL, 0, TimeThread, this, 0, NULL);
#else
		int err = pthread_create(&m_thread,0,TimeThread, this);
#endif//_WIN32
	}
	virtual ~ITimerEvent()
	{
		m_exitThread = true;
#ifdef _WIN32
		WaitForSingleObject(m_thread, INFINITE);
		CloseHandle(m_thread);
#else
		pthread_join(m_thread,NULL);		
#endif//_WIN32
		CCriticalSectionEx::Destroy(&m_CriticalSection);
	}

	void SetGameTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter, bool noSync = false)
	{
		CCriticalSectionEx lock;
		if (!noSync) lock.Lock(&m_CriticalSection);
		TimerItem* p = GetTimerItemWithTimerID(dwTimerID);
		if (!p) {
			p = &m_timerItems[m_timerItemCount++];
		}
		p->dwTimerID = dwTimerID;
		p->dwTimeInterval = dwElapse;
		p->dwRepeatTimes = dwRepeat;
		p->dwBindParameter = dwBindParameter;
		p->dwTimeElapsed = 0;
	}
	void KillGameTimer(DWORD dwTimerID, bool noSync = false)
	{
		CCriticalSectionEx lock;
		if (!noSync) lock.Lock(&m_CriticalSection);

		short index = GetTimerItemIndexWithTimerID(dwTimerID);
		if (index >= 0) {
			m_timerItems[index] = m_timerItems[m_timerItemCount--];
		}
	}
	void KillAllGameTimer(bool noSync = false)
	{
		CCriticalSectionEx lock;
		if (!noSync) lock.Lock(&m_CriticalSection);
		m_timerItemCount = 0;
	}

protected:
	virtual void OnTimerEvent(DWORD dwTimerID, WPARAM dwBindParameter) = 0;

protected:
	TimerItem* GetTimerItemWithTimerID(DWORD dwTimerID)
	{
		for (short i = 0; i < m_timerItemCount; ++i) {
			if (m_timerItems[i].dwTimerID == dwTimerID) {
				return &m_timerItems[i];
			}
		}
		return NULL;
	}
	short GetTimerItemIndexWithTimerID(DWORD dwTimerID)
	{
		for (short i = 0; i < m_timerItemCount; ++i) {
			if (m_timerItems[i].dwTimerID == dwTimerID) {
				return i;
			}
		}
		return -1;
	}
#ifdef _WIN32
	static DWORD WINAPI TimeThread(LPVOID lparam)
#else
	static void* TimeThread(void* lparam)
#endif//_WIN32

	{
		ITimerEvent* sp = static_cast<ITimerEvent*>(lparam);
		if (sp) {
			DWORD currentTick = GetTickCount();
			DWORD  previousTick = currentTick;
			DWORD diffTick;
			TimerItem item[MAX_TIMER_ITEMS];
			while (!sp->m_exitThread) 
			{
#ifdef _WIN32
			Sleep(TIMER_SPACE);
#else
			usleep(TIMER_SPACE * 1000);
#endif//_WIN32
				currentTick = GetTickCount();
				if (previousTick > currentTick) {
					diffTick = 0xFFFFFFFF - previousTick + currentTick;
				} else {
					diffTick = currentTick - previousTick;
				}
				sp->TimeProcedure(item, diffTick);
				previousTick = currentTick;
			}
		}
		return 0;
	}

	void TimeProcedure(TimerItem* pTimerItems, DWORD deltaTime)
	{
		unsigned short itemCount = 0;
		short i;
		CCriticalSectionEx lock;
		lock.Lock(&m_CriticalSection);

		for (i = 0; i < m_timerItemCount; ++i) {
			m_timerItems[i].dwTimeElapsed += deltaTime;
			if (m_timerItems[i].dwTimeElapsed >= m_timerItems[i].dwTimeInterval) {
				pTimerItems[itemCount++] = m_timerItems[i];
				if (--m_timerItems[i].dwRepeatTimes == 0) {
					// delete item
					m_timerItems[i] = m_timerItems[m_timerItemCount--];
					continue;
				} else {
					m_timerItems[i].dwTimeElapsed -= m_timerItems[i].dwTimeInterval;
				}
			}
		}
		lock.Unlock();
		for (i = 0; i < itemCount; ++i) {
			OnTimerEvent(pTimerItems[i].dwTimerID, pTimerItems[i].dwBindParameter);
		}
	}

protected:
	CRITICAL_SECTION m_CriticalSection;
#ifdef _WIN32
	HANDLE m_thread;
#else
	pthread_t m_thread;
#endif//_WIN32
	bool m_exitThread;
	TimerItem m_timerItems[MAX_TIMER_ITEMS];
	unsigned short m_timerItemCount;
};

#endif
