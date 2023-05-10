/*
Description:
    XML UTIL powered by tinyXML.

how to use:

  1. parse XML from memory.
  const char *szXml = "<?xml version=\"1.0\"?><ROOT><BOOK><BOOKNAME>XML</BOOKNAME></BOOK></ROOT>";
  xml.Parse(szXml);
  
  2. load XML as file.
  xml.Load("test.xml");
  xml.Sel("/background");
  int32 nCount = xml.CountSubet("item");
  while (xml.MoveNext())
  {
    int id = xml.GetLong("id");
    lpsz = xml.GetString("sound");
  }

   3. attach to special node.
   CXmlParser temp;
   temp.Attach(xml.pNode);
   temp.Sel("group");
   nCount = temp.CountSubet("item");
   nCount = temp.GetLong("id");
*/


#ifndef __XMLPARSER_HEADER__
#define __XMLPARSER_HEADER__

#include <types.h>
#include <string.h>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
// #pragma comment(lib,"tinyxml.lib")
#include <atlbase.h>
#include <atlconv.h>
#else
#define USES_CONVERSION
#define T2CA
#define A2CT
#define CA2T
#endif//_WIN32

#include "xml/tinyxml.h"


#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif//ARRAYSIZE

namespace System  {
    namespace Xml {

class CXmlParser
{
public:
    CXmlParser()
    {
        pCur = NULL;
        bMov =FALSE;
        pXml = NULL;
        pNode= NULL;
        memset(szNodeName,0,sizeof(szNodeName));
		m_bIsOpened = FALSE;
    }

    ~CXmlParser()
    {
        Close();
    }

public:
    BOOL Attach(TiXmlNode* pNode)
    {
        if (!pNode) return (FALSE);
        pCur = pNode->ToElement();
		m_bIsOpened = TRUE;
    //Cleanup:
        return (TRUE);
    }

    BOOL Parse(const char* lpszXml)
    {
        // try to allocate the obj.
        pXml = new TiXmlDocument();
		if (!pXml) {
			// GameGetEngine()->LogInfo("new TiXmlDocument failed.");
			return (FALSE);
		}

		// parse the xml file.
        pXml->Parse(lpszXml);
        if (false == pXml->Error())
        {
			m_bIsOpened = TRUE;
            return (TRUE);
        }
    //Cleanup:
        return (FALSE);
    }

    BOOL LoadA(const char* lpszFile)
    {
        USES_CONVERSION;
        return Load(CA2T(lpszFile));
    }


    BOOL Load(LPCTSTR lpszFile)
    {
		USES_CONVERSION;
        pXml = new TiXmlDocument(T2CA(lpszFile));
        if (pXml) 
        {
            if (pXml->LoadFile())
            {
				m_bIsOpened = TRUE;
                return (Sel(_T("/")));
                // return (TRUE);
            }
        }
    //Cleanup:
        return (FALSE);
    }

    //// Close()
    VOID Close()
    {
		m_bIsOpened = FALSE;
        if (pNode){
            pNode=NULL;            
        }
        if (pCur) {
            pCur = NULL;
        }
        if (pXml) {
			// bug:if delete pXml,will crash!.
//             delete pXml;
            pXml = NULL;
        }
    }

	// L IsOpened()
	BOOL IsOpened()
	{
		return (m_bIsOpened);
	}


public:
    BOOL Sel(LPCTSTR lpszPath)
    {
        BOOL bSucc = FALSE;
        LPCTSTR lpsz = NULL;
        TCHAR szTemp[1024]={0};

        do 
        {
            // check path item.
            if ((!lpszPath)) {
                bSucc=TRUE;   
                break;
            }
            // check the split item now.
            if (('/'  == lpszPath[0])||
                ('\\' == lpszPath[0]))
            {
                if (!pXml) break;
                pCur = pXml->RootElement();
                lpszPath++;
            }

            // check if select root item.
            if (!strlen(lpszPath)) {
                bSucc = TRUE;   
                break;
            }
            // try to get the string value.
            strncpy(szTemp,lpszPath,1024);
            // try to split the input value.
            lpsz = strtok(szTemp,_T("//"));
            while (lpsz)
            {
                /// try to check if item now.
                if (strcmp(lpsz,_T(".."))==0)
                {
                    if (!pCur) break;
                    TiXmlNode* pNode = pCur->Parent();
                    if (pNode)
                    {
                        pCur = pNode->ToElement();
                    }
                }
                else
                {
                    if (!pCur) break;
					USES_CONVERSION;
                    // try to select the first child node item value content.
                    TiXmlElement* pEle = pCur->FirstChildElement(T2CA(lpsz));
                    if (!pEle)
                    {
                        bSucc = FALSE;
                        break;
                    }   else
                    {
                        pCur  = pEle;
                        bSucc = TRUE;
                    }
                }

                // move to next element it now.
                lpsz = strtok(NULL,_T("//"));
            }

        } while (0);

    //Cleanup:
        pNode  = pCur;
        return (bSucc);
    }

	BOOL SelA(const char* lpszPath)
	{
		USES_CONVERSION;
		return (Sel(A2CT(lpszPath)));
	}

    BOOL Reset(BOOL bRoot=TRUE)
    {
        bMov = FALSE;
        if (!pXml) 
            return TRUE;

        // reset the current to root.
        pCur = pXml->RootElement();
    //Cleanup:
        return (TRUE);
    }

    /// try to count the sub item value.
    int32 CountSubet(LPCTSTR lpszName)
    {
        int32 nCount = 0;
		USES_CONVERSION;
        for (pNode = pCur->FirstChildElement(T2CA(lpszName));
             pNode;pNode= pNode->NextSibling(T2CA(lpszName)))
             {
                nCount++;
             }
    //Cleanup:
        bMov = FALSE;
        strncpy(szNodeName,lpszName,1024);
        return (nCount);
    }

public:
    int32 GetLong(LPCTSTR lpszName)
    {
        int32 nValue  = 0;
        LPCTSTR lpsz = GetString(lpszName);
        if (lpsz)
        {
            nValue = atoi(lpsz);
        }
    //Cleanup:
        return (nValue);
    }

	int32 GetLongA(const char* lpszName)
	{
		USES_CONVERSION;
		return (GetLong(A2CT(lpszName)));
	}

    /// 默认值改成 : -1 
    /// 因为有可能Long值就是0进来的.
    int32 GetLongEx(LPCTSTR lpszName)
    {
        int32 nValue  = -1;
        LPCTSTR lpsz = GetString(lpszName);
        if (strlen(lpsz))
        {
            nValue = atoi(lpsz);
        }
        //Cleanup:
        return (nValue);
    }

	// 自己指定默认值.
    int32 GetLongEx2(LPCTSTR name,int32 defVal)
    {
        int32 nValue  = defVal;
        LPCTSTR lpsz = GetString(name);
        if (strlen(lpsz)) {
            nValue=atoi(lpsz);
        }
    //Cleanup:
        return (nValue);
    }

    // if NULL : get current node value now.
    LPCTSTR GetString(LPCTSTR lpszName)
    {
        const char* lpsz = NULL;
        TiXmlElement* pTemp = NULL;
		static TCHAR szTemp[1024]={0};
		USES_CONVERSION;
        if (pNode)
		{
			if (!lpszName)
			{
				lpsz = pNode->ToElement()->GetText();
			}
			else
			{
				pTemp = pNode->FirstChildElement(T2CA(lpszName));
				if (pTemp)
				{
					lpsz = pTemp->ToElement()->GetText();
				}
			}
		}
	//Cleanup:
		if (!lpsz) lpsz="";
		strncpy(szTemp,A2CT(lpsz),ARRAYSIZE(szTemp));
        return (szTemp);
    }

	LPCTSTR GetStringA(const char* lpszName)
	{
		USES_CONVERSION;
		return (GetString(A2CT(lpszName)));
	}

    float GetFloat(LPCTSTR lpszName)
    {
        float fTemp  = 0;
        LPCTSTR lpsz = GetString(lpszName);
        if (lpsz)
		{
			// try to get the value now content.
			fTemp = (float)strtod(lpsz,NULL);
        }
    //Cleanup:
        return (fTemp);
    }

	float GetFloatA(const char* lpszName)
	{
		USES_CONVERSION;
		return (GetFloat(A2CT(lpszName)));
	}


    float GetFloatEx2(LPCTSTR lpszName,float defVal)
    {
        float fTemp = defVal;
        LPCTSTR lpsz = NULL;
        lpsz = GetString(lpszName);
        if ((lpsz) && (strlen(lpsz))) {
            fTemp=(float)strtod(lpsz,NULL);
        }
	//Cleanup:
        return (fTemp);
    }

    //// MoveNext()
    BOOL MoveNext()
    {
		USES_CONVERSION;
        if (!bMov){
            pNode = pCur->FirstChildElement(T2CA(szNodeName));
            bMov  = TRUE;
        }   else
        {
            pNode = pNode->NextSibling(T2CA(szNodeName));
        }

        if (pNode) {
            return TRUE;
        }

    //Cleanup:
        return (FALSE);
    }

public:
//protected:
    TiXmlElement*     pCur; 
    TiXmlDocument*    pXml;
    TiXmlNode*       pNode;
    BOOL              bMov;
    TCHAR szNodeName[1024];
	BOOL	   m_bIsOpened;
};

}
}

using namespace System::Xml;
#endif//__XMLPARSER_HEADER__
