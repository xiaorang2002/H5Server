
#ifndef __DLOPEN_HEADER__
#define __DLOPEN_HEADER__

#ifdef WIN32
#include <windows.h>
#else
#include <types.h>
#include <unistd.h>
#include <dlfcn.h>
#endif//WIN32

// void *dlopen(const char *filename, int flag);
// char *dlerror(void);
// void *dlsym(void *handle, const char *symbol);
// int dlclose(void *handle);


class Cdlopen
{
public:
	Cdlopen()
	{
		handle = NULL;
	}

	/// try to open the library.
	void* open(LPCTSTR lpszFile)
	{	
#ifdef  WIN32
		handle = (void*)LoadLibrary(lpszFile);
#else
		handle = dlopen(lpszFile,RTLD_LAZY);
#endif//WIN32
	//Cleanup:
		return (handle);
	}

	// get the function name now
	void* sym(LPCTSTR lpszFunc)
	{
		void* func = NULL;
#ifdef  WIN32
		func = (void*)GetProcAddress((HINSTANCE)handle,lpszFunc);
#else
		func = dlsym(handle,lpszFunc);
#endif//WIN32
	//Cleanup:
		return (func);
	}

	// close now.
	int close()
	{
		int err = -1;
#ifdef  WIN32
		if (FreeLibrary((HMODULE)handle)) {
			err = 0;
		}
#else
		err = dlclose(handle);		
#endif//WIN32
	}

protected:
	void* handle;
};

#endif//__DLOPEN_HEADER__
