
#ifndef __SPLITSTRING_HEADER__
#define __SPLITSTRING_HEADER__

#include <types.h>
#include <mem/Memhelp.h>
#include <vector>

#ifdef  _WIN32
#include <tchar.h>
#pragma warning(disable:4018)
#pragma warning(disable:4996)
#endif//_WIN32

class CSplitString
{
public:
    CSplitString()
    {
		index = 0;
    }

    virtual ~CSplitString()
    {
		clean_vector();
    }

public:
    /// try to split the string value now try to split.
    BOOL SetSplit(LPCTSTR din,LPCTSTR split=TEXT(";"))
    {
		BOOL   bSucc = false;
		long   insize= strlen(din);
		TCHAR* szTmp = NULL;
		do 
		{
			clean_vector();
			szTmp = strdup(din);
			if ((!din)||(!split)||(!szTmp)) break;
			strncpy(szTmp,din,insize+1);
			TCHAR* token = strtok(szTmp,split);
			while (token)
			{
				vec.push_back(strdup(token));
				token = strtok(NULL,split);
			}
			bSucc=TRUE;
			index=0;
		}   while (0);
	//Cleanup:
		if (szTmp) free(szTmp);
		return (bSucc);
    }

	///  count()
	LONG count()
	{
		return ((LONG)vec.size());
	}

	// try to get the special value now.
	const TCHAR* operator[](long  index)
	{
		return (GetIndex(index));
	}

	// try to get the string value.
    LPCTSTR GetIndex(LONG nIndex)
    {
		LPCTSTR lpsz = NULL;
		long size=(LONG)vec.size();
		do 
		{
			// try to initialize the special index.
			if ((nIndex<0)||(nIndex>=size)) break;
			lpsz = vec[nIndex];
		}   while (0);
	//Cleanup:
		return (lpsz);
    }

    /// next value.
    LPCTSTR Next()
    {
        BOOL bSucc = FALSE;
        LPCTSTR lpsz = NULL;
		do 
		{
			// get the value item now.
			if (index<vec.size()) {
				lpsz = vec[index];
				index++;
			}
		} while (0);
	//Cleanup:
		return (lpsz);
    }

protected:
	void clean_vector()
	{
		std::vector<TCHAR*>::iterator iter;
		for (iter=vec.begin();iter!=vec.end();iter++) {
			TCHAR* sp = *iter;
			if  (sp) free(sp);
		}
	//Cleanup:
		vec.clear();
	}

protected:
	std::vector<TCHAR*> vec;
	long index;
};

#endif//__SPLITSTRING_HEADER__
