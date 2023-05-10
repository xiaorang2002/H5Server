#ifndef __TTHREAD_HEADER__
#define __TTHREAD_HEADER__

#ifdef  _WIN32
#include <windows.h>

#define THREADAPI			DWORD WINAPI
#define THREAD_STAT_EXIT	(WAIT_OBJECT_0)
#define THREAD_STAT_RUNNING	(1)

#ifndef usleep
#define usleep(a)		Sleep(a/1000)
#endif//usleep

typedef HANDLE  TTHREAD_HANDLE;
typedef DWORD (WINAPI* THREADFUNC)(LPVOID lParam);

#else
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define THREADAPI			void*
#define THREAD_STAT_EXIT	(0)
#define THREAD_STAT_RUNNING (1)

typedef void* (*THREADFUNC)(void* args);
typedef pthread_t TTHREAD_HANDLE;


#endif//_WIN32


class CThread
{
public:
	CThread() {}
	virtual ~CThread() {}

public:
	static int CreateThreadT(TTHREAD_HANDLE* pHandle, THREADFUNC ThreadFunc, void* args = NULL)
	{
		int status = -1;
#ifdef  _WIN32
		DWORD dwThreadID = 0;
		*pHandle = CreateThread(NULL, 0, ThreadFunc, args, 0, &dwThreadID);
		if (WAIT_OBJECT_0 != WaitForSingleObject(&pHandle, 100)) {
			status = S_OK;
		}
#else
		status = pthread_create(pHandle, 0, ThreadFunc, args);
#endif//_WIN32
	//Cleanup:
		return (status);
	}

	// try to wait for the special single object content item for later user.
	static int WaitForSingleObjectT(TTHREAD_HANDLE& hThread,unsigned int nTimeout=100)
	{
		int nStatus = THREAD_STAT_RUNNING;
#ifdef  _WIN32
		if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, nTimeout)) {
			nStatus = THREAD_STAT_EXIT;
		}
#else
		if (0 == pthread_tryjoin_np(hThread, NULL)) {
			nStatus = THREAD_STAT_EXIT;
		}
#endif//_WIN32
	//Cleanup:
		return (nStatus);
	}

	static void TerminateThreadT(TTHREAD_HANDLE& hThread, int nExitCode=0)
	{
#ifdef  _WIN32
		TerminateThread(hThread, nExitCode);
#else
		pthread_cancel(hThread);
#endif//_WIN32
	}

	static int GetCurThreadId()
	{
		int threadid = 0;
#ifdef  _WIN32
		threadid = GetCurrentThreadId();
#else
		threadid = pthread_self();
#endif//_WIN32
	//Cleanup:
		return (threadid);
	}

};

#endif//__TTHREAD_HEADER__
