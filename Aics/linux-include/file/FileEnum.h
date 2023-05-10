/*
    CFileEnum fs;
    fs.Find("C:\\");
    while (fs.Next(buf,size))
    {
        ...
    }

History:
	2016-12-28 12:17 修正了枚举不到第一个文件的问题.
*/

#ifndef __FILEENUM_HEADER__
#define __FILEENUM_HEADER__

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

//
// Enum File now.
class CFileEnum
{
public:
    CFileEnum()
    {
        // try to empty the special buffer now.
        memset(m_lpszDir,0,sizeof(m_lpszDir));
        /// initialized.
        m_hFind  = NULL;
    }

    ~CFileEnum()
    {
        Final();
    }

public:
    DWORD CountSubFiles(LPCTSTR lpszDir)
    {
        TCHAR szTemp[MAX_PATH];
        DWORD dwCount=0;
        if (Find(lpszDir))
        {
            do
            {
                dwCount++;
            }   while (Next(szTemp,MAX_PATH,FALSE));
        }
    //Cleanup:
        return (dwCount);
    }
    
	// try to find the special directory content now.
    BOOL Find(LPCTSTR lpszDir,LPCTSTR lpszFilter=0)
    {
        LONG nSize = 0;
		BOOL bSucc = FALSE;
        TCHAR szPath[1024]=TEXT("");
        // try to build the full.
        nSize = strlen(lpszDir);
		if (!nSize) {
            return (bSucc);
		}

        // try to get the content item now.
        strncpy(m_lpszDir,lpszDir,MAX_PATH);
        m_hFind = opendir(m_lpszDir);
        if (m_hFind)  
		{
            // Succeeded.
        	bSucc = TRUE;
		}
    //Cleanup:
        return (bSucc);
    }
    
	// try to get the next file name with toe special file now try to get the next file name with the file.
    LPCTSTR Next(LPTSTR lpszBuffer,LONG nSize,BOOL bFullPath=TRUE,BOOL* pbDir=NULL,dirent* pFindData=NULL)
    {
		LPCTSTR lpszFile=0;
		do
		{
			// try to empty the buffer.
			memset(lpszBuffer,0,nSize);
			//try to find the special next item value.
			dirent* spfile = readdir(m_hFind);
			if (!spfile) break; // checked.
			// loop to skip the "." and ".."
			while ((strcmp(spfile->d_name,".")==0) || 
				   (strcmp(spfile->d_name,"..")==0)){
				spfile = readdir(m_hFind);
				if (!spfile) break;
			}

			// check if find ok.
			if (!spfile) break;
			if (pbDir) 
			{
				*pbDir=FALSE;
				if (spfile->d_type == DT_DIR) {
					*pbDir=TRUE;            
				}
			}
		
			/// full path.
			if (bFullPath)
			{
				// try to build the full path now.
				strncpy(lpszBuffer,m_lpszDir,MAX_PATH);
				strcat(lpszBuffer,TEXT("\\"));
				strcat(lpszBuffer,spfile->d_name);
			}
			else
			{
				// try to return the file name content value now.
				strncpy(lpszBuffer,spfile->d_name,nSize);
			}
	
			// get buffer content.
			lpszFile = lpszBuffer;
			// find data now.
			if (pFindData) {
				memcpy(pFindData,spfile,sizeof(dirent));
			}
			
		} while (0);
	//Cleanup:
		return (lpszFile);
    }

    // Final.
    VOID Final()
    {
        if (m_hFind)
        {
            // try to close.
            closedir(m_hFind);
            m_hFind = NULL;
        }
    }

protected:
    TCHAR m_lpszDir[MAX_PATH];
    DIR*  m_hFind; // handle.
};


#endif//__FILEENUM_HEADER__
