/*
Description:

how to use:
    ofs.Load("test.txt",&user,sizeof(user));
    ofs.Openwrite("test.txt");
    ofs.Write(&user,sizeof(user));
*/


#ifndef __FASTPERSIST_HEADER__
#define __FASTPERSIST_HEADER__

#include <stdio.h>
#include <types.h>

#ifdef  _WIN32
#include <windows.h>
#endif//_WIN32

// declare the file begin now.
#define FILE_BEGIN	SEEK_SET

class CFastpersist
{
public:
    CFastpersist()
    {
        bOver = FALSE;
        hFile = NULL;
    }

    ~CFastpersist()
    {
        Close();
    }

    LONG Openappend(LPCTSTR filename)
    {
        hFile = fopen(filename,TEXT("ab"));
        if (hFile) {
            DWORD dwSize = GetFileSize(hFile,NULL);
            SetFilePointer(hFile,dwSize,NULL,FILE_BEGIN);
            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

    LONG Openwrite(LPCTSTR filename)
    {
        hFile = fopen(filename,"wb");
        if (hFile) {
            bOver = TRUE;
            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

	// is file open.
	BOOL IsOpened()
	{
		return (hFile != NULL);
	}

    // L ReadOffset(LONG nOffset,LPVOID lpszData,DWORD nSize)
    BOOL ReadOffset(LONG nOffset,LPVOID lpszData,DWORD nSize)
    {
        BOOL bSucc = FALSE;
        if (!hFile) {
            return (FALSE);
        }
        
        // RD dwOff = SetFilePointer(hFile,nOffset,NULL,FILE_BEGIN);
        DWORD dwOff = SetFilePointer(hFile,nOffset,NULL,FILE_BEGIN);
        if (dwOff != (DWORD)nOffset) {
            return (FALSE);
        }

        // cc = Read(lpszData,nSize);
        bSucc = Read(lpszData,nSize);

    //Cleanup:
        return (bSucc);
    }

    // L WriteOffset(LONG nOffset,LPVOID lpszData,DWORD nSize)
    BOOL WriteOffset(LONG nOffset,LPVOID lpszData,DWORD nSize)
    {
        BOOL bSucc=FALSE;
        if (!hFile) {
            return (FALSE);
        }

        // RD dwOff = SetFilePointer(hFile,nOffset,NULL,FILE_BEGIN);
        DWORD dwOff = SetFilePointer(hFile,nOffset,NULL,FILE_BEGIN);
        if (dwOff != (DWORD)nOffset) {
            return (FALSE);
        }

        // cc = Write(lpszData,nSize);
        bSucc = Write(lpszData,nSize);

    //Cleanup:
        return (bSucc);
    }

    BOOL Write(LPVOID lpszData,DWORD nSize)
    {
        DWORD dwWritten =0;
        BOOL bSucc = FALSE;
        if (!hFile) {
            return (FALSE);
        }

        // try to write the special content value.
        dwWritten = fwrite(lpszData,1,nSize,hFile);
        if (dwWritten == nSize)
        {
            // set alpha.
            if (bOver) {
                SetFilePointer(hFile,0,NULL,FILE_BEGIN);
            }

            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

	// try to open read for file now.
    BOOL Openread(LPCTSTR filename)
    {
        hFile = fopen(filename,"rb");
        if (hFile) {
            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

    BOOL Read(LPVOID lpszData,DWORD nSize,BOOL bSkipSizeCheck=FALSE)
    {
        DWORD dwReaded = 0;
        BOOL bSucc = FALSE;
        if (!hFile) {
            return (FALSE);
        }

        /// try to write the special file content.
        dwReaded = fread(lpszData,1,nSize,hFile);
        if ((dwReaded == nSize)||(bSkipSizeCheck))
        {
            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

    BOOL Load(LPCTSTR lpszFile,LPVOID lpszData,LONG nSize)
    {
        BOOL bSucc = FALSE;
        DWORD dwReaded = 0;
        if (!Openread(lpszFile)) {
            return (FALSE);
        }

        // try to read the file item.
        bSucc = Read(lpszData,nSize);
    //Cleanup:
        return (bSucc);
    }

    ///  DoUpdate()
    VOID DoUpdate()
    {
        // NO_IMPL.
        return;
    }
	
	void Flush()
	{
		if (hFile) {
			fflush(hFile);
        }
	}
	
    ///  Close()
    VOID Close()
    {
        bOver = FALSE;
        if (hFile) 
		{
			fclose(hFile);
            hFile=0;
        }
    }

protected:
    static DWORD WINAPI ThreadUpdate(LPVOID lParam)
    {
        CFastpersist* pThis = (CFastpersist*)lParam;
        if (pThis) {
            pThis->DoUpdate();
        }
    //Cleanup:
        return (0);
    }

protected:
	DWORD SetFilePointer(FILE* fp,LONG offset,LONG* plhighoffset,DWORD movemethod)
	{
		DWORD dwpos = 0;
		if (fp) {
			fseek(fp,offset,movemethod);
			dwpos = ftell(fp);
		}
	//Cleanup:
		return (dwpos);
	}

	DWORD GetFileSize(FILE* fp,LONG* plhigh=0)
	{
		DWORD pos = ftell(fp);
		fseek(fp,0,SEEK_END);
		DWORD size = ftell(fp);
		fseek(fp,pos,SEEK_SET);
	//Cleanup:
		return (size);
	}


protected:
    FILE*  hFile;
    BOOL   bOver;
};

#endif//__FASTPERSIST_HEADER__
