#ifndef __TextParserUtility_h__
#define __TextParserUtility_h__

#include <vector>
using namespace std;

#define MAX_TEXT_LENGTH 255

class CTextParserUtility
{
public:
	CTextParserUtility() : m_nCount(0) {}
	~CTextParserUtility() { Empty(); }

	void Parse(const char* pText, const char* pSearch)
	{
		char szText[255] = { 0 };
		char szTemp[MAX_PATH] = { 0 };

		strcpy(szText, pText);
		m_nCount = 0;
		char* token = strstr(szText, pSearch);
		while (token)
		{
			memset(szTemp, 0, sizeof(szTemp));
			strncpy(szTemp, szText, token - szText);
			AddToList(m_nCount, szTemp);
			strncpy(szText, token+1, sizeof(szText));
			++m_nCount;
			token = strstr(szText, pSearch);
		}
		AddToList(m_nCount, szText);
		++m_nCount;
	}
	int Count() { return m_nCount; }
	char* operator [](int i) const { return m_vecText[i]; }
	char* getStringAt(int i) const { return m_vecText[i]; }

private:
    vector<char*> m_vecText;
    int m_nCount;
	void Empty()
	{
		for (vector<char*>::iterator iter = m_vecText.begin();
			iter != m_vecText.end(); ++iter) {
			delete[] * iter;
		}
	}
	void AddToList(int nPos, char* pValue)
	{
		char* p = pValue;
		if (!p)
		{
			p = "";
		}
		if (nPos < (int)m_vecText.size())
		{
			strcpy(m_vecText[nPos], p);
		}
		else
		{
			char* pTok = new char[MAX_TEXT_LENGTH];
			memset(pTok, 0, MAX_TEXT_LENGTH);
			strncpy(pTok, p, MAX_TEXT_LENGTH);
			m_vecText.push_back(pTok);
		}
	}
};
#endif//__TextParserUtility_h__
