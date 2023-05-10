/*
Descrption:
	File operation help class.
	
History:
	2014-05-15 modified by James
		修改了Load如果文件为空,返回FALSE的BUG,应该返回TRUE.
*/

#ifndef __FILEHELPER_HEADER__
#define __FILEHELPER_HEADER__

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <PathMagic.h>

class CFileHelp
{
public:
    CFileHelp() 
    {
        m_dwSize   = 0;
        m_lpszData = NULL;
        m_bPart    = FALSE;
        m_bLoaded  = FALSE;
        m_hf       = NULL;
    }

    virtual ~CFileHelp() {
        Close();
    }

public:
    BOOL Open(LPCTSTR lpsz, BOOL bCreate = FALSE, BOOL bReadOnly = FALSE, BOOL bLoad=TRUE)
    {
	BOOL bSuccess = FALSE;
	const char* oper = "r+";	
	do
	{
		if (bReadOnly) oper = "r";
		if (bCreate)   oper = "a";
		m_bPart  = FALSE;
		m_dwSize = 0;
		// try to open the file.
		m_hf = fopen(lpsz,oper);
		if (m_hf) {
			bSuccess = TRUE;
			if (bLoad) {
			    bSuccess = Load();
			}
		}

	} while (0);
    //Cleanup:
	return (bSuccess);
    }

    // try get file size.
    DWORD GetFileSize()
    {	
	DWORD size = 0;
	do
	{
		if (!m_hf) break;
		// try to get the old file position.
		int oldpos = ftell(m_hf);
		fseek(m_hf,0,SEEK_END);
		size = ftell(m_hf);
		// re seek the file position.
		fseek(m_hf,oldpos,SEEK_SET);
	} while (0);
    //Cleanup:
	return (size);
    }

    /// get file size.
    DWORD GetSize()
    {
        DWORD dw = m_dwSize;
        if (!m_bPart) {
             dw = GetFileSize();
	}
    //Cleanup:
        return (dw);
    }

    /// read the data content from file content.
    BOOL Read(LPBYTE lpszData, DWORD dwSize)
    {
        DWORD dw = 0;
        BOOL  bSucc = FALSE;
	do
	{
		if (!m_hf) break;
		size_t dw = fread(lpszData,1,dwSize,m_hf);
		if (dw == dwSize) {
			bSucc=TRUE;
		}

	} while (0);
    //Cleanup:
	return (bSucc);
    }

    /// write the content to the special file content.
    BOOL Write(const void* lpszData, DWORD dwSize)
    {
        BOOL  bSucc = FALSE;
        // try to write the special file content now.
        size_t dw = fwrite(lpszData,dwSize,1,m_hf);
        if ( dw == dwSize ) {
            bSucc = TRUE;
	}
    //Cleanup:
        return (bSucc);
    }

    //// try to get the file buffer.
    LPBYTE GetBuffer()
    {
        return (m_lpszData);
    }

    /// close the file.
    void Close()
    {
        m_bLoaded = FALSE;
        if (m_lpszData) 
        {
            // free the buffer.
            free(m_lpszData);
            m_lpszData = NULL;
        }

        // check if the handle.
        if (NULL != m_hf ) {
            CloseHandle();
            m_hf = NULL;
        }

    }

	// close handle now.
	void CloseHandle()
	{
		// close spcial handle.
		if (NULL !=m_hf) {
			fclose(m_hf);
			m_hf=NULL;
		}
	}

    //// try to write the special file to hard disk content.
    BOOL Save(LPCTSTR lpszFile, LPBYTE lpszData, LONG nSize)
    {
        DWORD dw = 0;
	BOOL bSucc = FALSE;
	FILE* fp = fopen(lpszFile,"wb");
	if (fp) {
		size_t dw = fwrite(lpszData,nSize,1,fp);
		fclose(fp);
		if (dw == nSize) {
			bSucc=TRUE;
		}
	}
    //Cleanup:
        return (bSucc);
    }

    // try to delete current file.
    BOOL Delete(LPCTSTR lpszFile)
    {
	BOOL bSucc=FALSE;
	do
	{
		// try to remove the special file.
		int ret = remove(lpszFile);
		if (0 != ret)
		{
			char tmp[MAX_PATH];
			snprintf(tmp,sizeof(tmp),"/tmp/%s",lpszFile);	
			rename(lpszFile,tmp);
			bSucc=TRUE;
		}

	} while (0);
    //Cleanup:
	return (bSucc);
    }

protected:
    /// try to load all the file content.
    BOOL Load(DWORD dwSize=-1)
    {
        BOOL bSucc = FALSE;
        do 
        {
            /// if loaded.
            if (m_bLoaded) {
                break;
            }
        
            // check the buffer.
            if ( m_lpszData ) {
                free(m_lpszData);
                m_lpszData = NULL;
            }
        
            // check size now.
            if (-1 == dwSize)
            {
                // get the file size.
                dwSize = GetSize();
                if (!dwSize) {
                    return TRUE;
                }
            }
        
			// update the size.
			m_dwSize = dwSize;
            // read the file content from disk item.
            m_lpszData = (LPBYTE)malloc(dwSize+1024);
            if ( !m_lpszData )
                return FALSE;
        
            // Zero the memory.
            memset(m_lpszData,0,dwSize+1024);
            bSucc = Read(m_lpszData,dwSize);
            // try to update the load flag
            m_bLoaded = TRUE; 
        }   while (0);
        
    //Cleanup:
        return (bSucc);
    }

protected:
    FILE*  m_hf;
    BOOL   m_bLoaded;
    LPBYTE m_lpszData;
    DWORD  m_dwSize;
    BOOL   m_bPart; // only read special handle bytes.
};


#endif//__FILEHELPER_HEADER__
