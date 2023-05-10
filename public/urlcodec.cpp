
#include "urlcodec.h"

namespace URL {

    std::map<char,bool> dontNeedEncoding;

	void initCharVec()
	{
		dontNeedEncoding.clear();
		int i;
		for (i = 'a'; i <= 'z'; i++)
		{
			dontNeedEncoding[i] = true;
		}
		for (i = 'A'; i <= 'Z'; i++)
		{
			dontNeedEncoding[i] = true;
		}
		for (i = '0'; i <= '9'; i++)
		{
			dontNeedEncoding[i] = true;
		}
		dontNeedEncoding['+'] = true; //'+';
		dontNeedEncoding['-'] = true; //'-';
		dontNeedEncoding['_'] = true; //'_';
		dontNeedEncoding['.'] = true; //'.';
		dontNeedEncoding['*'] = true; //'*';
	}

	//判断c是否是16进制的字符
	bool isDigit16Char(char c)
	{
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')|| (c >= 'a' && c <= 'f');
	}
	// 
	bool hasUrlEncoded(std::string str)
	{
		if (dontNeedEncoding.size() == 0)
		{
			 initCharVec();
			 printf("-----hasUrlEncoded-------[%d]------\n",dontNeedEncoding.size());
		}

		for (int i = 0; i < str.length(); i++)
		{
			char c = str[i];
			if (dontNeedEncoding[c])
			{
				continue;
			}

			if (c == '%' && (i + 2) < str.length())
			{
				// 判断是否符合urlEncode规范
				char c1 = str[++i];
				char c2 = str[++i];
				// 存在urlEncode
				if (isDigit16Char(c1) && isDigit16Char(c2))
				{
					return true;
				}
			}
		}

		return false;
	}

	static char CharToInt(char ch)
	{
		if (ch >= '0' && ch <= '9')
		{
			return (char)(ch - '0');
		}
		if (ch >= 'a' && ch <= 'f')
		{
			return (char)(ch - 'a' + 10);
		}
		if (ch >= 'A' && ch <= 'F')
		{
			return (char)(ch - 'A' + 10);
		}
		return -1;
	}

	static char StrToBin(char* pString)
	{
		char szBuffer[2];
		char ch;
		szBuffer[0] = CharToInt(pString[0]); //make the B to 11 -- 00001011 
		szBuffer[1] = CharToInt(pString[1]); //make the 0 to 0 -- 00000000 
		ch = (szBuffer[0] << 4) | szBuffer[1]; //to change the BO to 10110000 
		return ch;
	}

	std::string Encode(const std::string& str)
	{
		std::string strResult;
		size_t nLength = str.length();
		unsigned char* pBytes = (unsigned char*)str.c_str();
		char szAlnum[2];
		char szOther[4];
		for (size_t i = 0; i < nLength; i++)
		{
			if (isalnum((char)str[i]))
			{
				snprintf(szAlnum, sizeof(szAlnum), "%c", str[i]);
				strResult.append(szAlnum);
			}
			else if (isspace((char)str[i]))
			{
				strResult.append("+");
			}
			else
			{
				snprintf(szOther, sizeof(szOther), "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
				strResult.append(szOther);
			}
		}
		return strResult;
	}
	// 防止多重编码
	std::string MultipleDecode(const std::string& str)
	{
		// 判断是否存在url编码
		if( !URL::hasUrlEncoded(str) )
		{
			return str;
		}
		std::string strURL;
		if(URL::Decode(str,strURL) == 1)
		{
			std::string temp;
			do{
				temp = strURL;
			} while (URL::Decode(temp,strURL) == 1);
		}

		return strURL;
	}

	// 1-2重编码
	int32_t Decode(const std::string& str,std::string& strResult)
	{
		strResult.clear();
		int ret = 0;
		char szTemp[2];
		size_t i = 0;
		size_t nLength = str.length();
		while (i < nLength)
		{
			if (str[i] == '%')
			{
				szTemp[0] = str[i + 1];
				szTemp[1] = str[i + 2];
				// 做两重url解密
				char ch = StrToBin(szTemp);
				if ( ch == '%' ){ 
					szTemp[0] = str[i + 3];
					szTemp[1] = str[i + 4];
					ch = StrToBin(szTemp);

					strResult += ch;
					i = i + 5;

					// 还有多重编码
					if( ch == '%' ){
						ret = 1;
					}
				}
				else{
					strResult += StrToBin(szTemp);
					i = i + 3;
				}
			}
			else if (str[i] == '+')
			{
				strResult += ' ';
				i++;
			}
			else
			{
				strResult += str[i];
				i++;
			}
		}
 
		return ret;
	}

	std::string url_encode(const std::string &d)
	{
#define _INLINE_C_2_H(x) (unsigned char)(x > 9 ? x + 55 : x + 48)
		std::string e = "";
		std::size_t size = d.size();
		for (std::size_t i = 0; i < size; i++)
		{
			if (isalnum((unsigned char)d.at(i)) ||
				(d.at(i) == '-') ||
				(d.at(i) == '_') ||
				(d.at(i) == '.') ||
				(d.at(i) == '~'))
			{
				e.append(1, d.at(i));
			}
			else if (d.at(i) == ' ')
			{
				e.append(1, '+');
			}
			else
			{
				e.append(1, '%');
				e.append(1, _INLINE_C_2_H((unsigned char)d.at(i) >> 4));
				e.append(1, _INLINE_C_2_H((unsigned char)d.at(i) % 16));
			}
		}
		return e;
	}

	std::string url_decode(const std::string &e)
	{
#define __INLINE_H_2_C(x) ((x >= 'A' && x <= 'Z') ? (x - 'A' + 10) : ((x >= 'a' && x <= 'z') ? (x - 'a' + 10) : ((x >= '0' && x <= '9') ? (x - '0') : x)))
		std::string d = "";
		std::size_t size = e.size();
		for (std::size_t i = 0; i < size; i++)
		{
			if (e.at(i) == '+')
			{
				d.append(1, ' ');
			}
			else if (e.at(i) == '%')
			{
				if (i + 2 < size)
				{
					d.append(1, (unsigned char)(__INLINE_H_2_C((unsigned char)e.at(i + 1)) * 16 + __INLINE_H_2_C((unsigned char)e.at(i + 2))));
					i += 2;
				}
				else
				{
					for (; i < size; i++)
					{
						d.append(1, e.at(i));
					}
				}
			}
			else
			{
				d.append(1, e.at(i));
			}
		}
		return d;
	}
}