
#ifndef __FILEEXIST_HEADER__
#define __FILEEXIST_HEADER__
#include <PathMagic.h>

class CFileExist
{
public:
	static BOOL IsFile(LPCTSTR lpszFile)
	{
        BOOL bExist = FALSE;
#ifdef _WIN32
       DWORD dwAttrib = GetFileAttributes(lpszFile);
        if (INVALID_FILE_ATTRIBUTES != dwAttrib) {
            bExist = TRUE;
        }
#else
		struct stat st={0};
		int err = stat(lpszFile,&st);
		if (0 == err) {
			bExist = TRUE;
		}
#endif//_WIN32
    //Cleanup:
		return (bExist);
	}

    static BOOL IsFileA(LPCSTR lpszFile)
    {
        BOOL bExist = FALSE;
#ifdef _WIN32
        DWORD dwAttrib = GetFileAttributesA(lpszFile);
        if (INVALID_FILE_ATTRIBUTES != dwAttrib) {
            bExist = TRUE;
        }
#else
		struct stat st={0};
		int err = stat(lpszFile,&st);
		if (0 == err) {
			bExist = TRUE;
		}
#endif//_WIN32
    //Cleanup:
        return (bExist);
    }
    
    static BOOL IsFileW(LPCWSTR lpszFile)
    {
        BOOL b = FALSE;
#ifdef _WIN32
        HANDLE hf=INVALID_HANDLE_VALUE;
        hf=CreateFileW(lpszFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
        // check the handle type.
        if (INVALID_HANDLE_VALUE!=hf)
        {
            // succeeded.
            b = TRUE;
        }
        
        //Cleanup:
        if ( INVALID_HANDLE_VALUE!=hf) {
            CloseHandle(hf);
            hf=INVALID_HANDLE_VALUE;
        }
#else
		// check file exist.
		struct stat st={0};
		int err = stat(lpszFile,&st);
		if (0 == err) {
			b = TRUE;
		}
#endif//_WIN32
	//Cleanup:
        return (b);
    }

    /// check the special file is special extension name.
    static BOOL IsSpecialFileA(LPCSTR lpszFile,LPCSTR lpszExtension)
    {
        BOOL bSuccess = FALSE;
		CHAR sz[MAX_PATH]={0};
#ifdef _WIN32
		LPCSTR lpsz = CPathMagic::PathExtensionA(lpszFile);
        // try to build the extension filename.
        lstrcatA( sz,lpsz);
        // try to compare the extension.
        if ( lstrcmpiA(sz,lpszExtension)==0)
             bSuccess = TRUE;
		}
#else
		LPCSTR lpsz = CPathMagic::PathExtension(lpszFile);
		// try to build the extension filename.
		lstrcat(sz,lpsz);
		// try to compare the extension.
		if (lstrcmpi(sz,lpszExtension)==0)
		{
			bSuccess = TRUE;
		}
#endif//_WIN32
    //Cleanup:
        return (bSuccess);
    }

    static BOOL IsSpecialFileW(LPCWSTR lpszFile, LPCWSTR lpszExtension)
    {
        BOOL bSuccess = FALSE;
#ifdef _WIN32
        WCHAR sz[MAX_PATH] = {0};
        LPCWSTR lpsz = CPathMagic::PathExtensionW(lpszFile);
        // try to build the extension filename.
        lstrcatW( sz, lpsz);
        // try to compare the extension.
        if ( lstrcmpiW(sz,lpszExtension)==0) {
            bSuccess = TRUE;
		}
#else
		CHAR sz[MAX_PATH]={0};
		LPCSTR lpsz = CPathMagic::PathExtension(lpszFile);
		// try to build the special file name.
		lstrcat(sz,lpsz);
		// try to compare the extension.
		if (lstrcmpi(sz,lpszExtension)==0) {
			bSuccess = TRUE;
		}
#endif//_WIN32
    //Cleanup:
        return (bSuccess);
    }

};


#endif//__FILEEXIST_HEADER__
