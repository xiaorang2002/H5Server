#ifndef MY_HTMLCODE
#define MY_HTMLCODE

#include <algorithm>
#include <iostream>
#include <string>
#include "AsciiEntity.hpp"

#define array_length(a) (sizeof(a) / sizeof(a)[0])

namespace HTML
{
using namespace std;

static struct replace_t
{
	std::string src;
	std::string dst;
} codes[] = {
	{"&", "&amp;"},
	{"<", "&lt;"},
	{">", "&gt;"},
	{"\"", "&quot;"},
};

static size_t const codes_size = array_length(codes);

//return value.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;").replace("\"", "&quot;");
std::string Encode(const std::string &s)
{
	std::string rs(s);
	for (size_t i = 0; i < codes_size; i++)
	{

		std::string const &src = codes[i].src;
		std::string const &dst = codes[i].dst;
		std::string::size_type start = rs.find_first_of(src);

		while (start != std::string::npos)
		{
			rs.replace(start, src.size(), dst);

			start = rs.find_first_of(src, start + dst.size());
		}
	}
	return rs;
}
//value.replace("&gt;", ">").replace("&lt;", "<").replace("&quot;", '"').replace("&amp;", "&");
std::string Decode(const std::string &s)
{
	std::string rs(s);
	for (size_t i = 0; i < codes_size; i++)
	{

		std::string const &src = codes[i].dst;
		std::string const &dst = codes[i].src;
		std::string::size_type start = rs.find_first_of(src);

		while (start != std::string::npos)
		{
			rs.replace(start, src.size(), dst);

			start = rs.find_first_of(src, start + dst.size());
		}
	}
	return rs;
}

string EncodeHtml(const wstring &html)
{
	ostringstream result;
	wstring::const_iterator i;
	for (i = html.begin(); i != html.end(); ++i)
	{
		if (*i >= 128)
			result << "&#" << static_cast<int>(*i) << ";";
		else if (*i == '<')
			result << "&lt;";
		else if (*i == '>')
			result << "&gt;";
		else if (*i == '&')
			result << "&amp;";
		else if (*i == '"')
			result << "&quot;";
		else
			result << static_cast<char>(*i); // Copy untranslated
	}
	return result.str();
}

/**********************************************************************************************/
/**********************************************************************************************/
/**********************************************************************************************/

/**********************************************************************************************/
/**************************************** C#逻辑HtmlEncode、HtmlDecode ************************/
/**********************************************************************************************/

int32_t IndexOfHtmlEncodingChars(string s, int32_t startPos)
{
	int32_t num = s.length() - startPos;
	if (const char *ptr = s.data()) //fixed
	{
		const char *ptr2 = ptr + startPos;
		while (num > 0)
		{
			char c = *ptr2;
			if (c <= '>')
			{
				switch (c)
				{
				case '"':
				case '&':
				case '<':
				case '>':
					return s.length() - num;
				}
			}
			else if (c >= '\u00a0' && c < 'Ā')
			{
				return s.length() - num;
			}
			ptr2++;
			num--;
		}
	}
	return -1;
}

char Lookup(string entity)
{
	if (hashtable.size() == 0)
	{
		for (auto &text : entitiesList)
		{
			hashtable[text.substr(2)] = text[0];
		}
	}

	char obj = hashtable[entity];
	if (obj != 0) //nullptr)
	{
		return (char)obj;
	}

	return '\0';
}

// 解码
void HtmlDecode(string s, stringstream &output)
{
	if (s.empty())
	{
		return;
	}

	// 清空
	output.clear();

	string::size_type spos = s.find_first_of('&');
	if (spos == std::string::npos)
	{
		output << s;
		return;
	}

	int32_t length = s.length();
	for (int32_t i = 0; i < length; i++)
	{
		char c = s[i];
		if (c == '&')
		{
			// 当找到&后往下找;
			// int32_t num = s.IndexOfAny(s_entityEndingChars, i + 1);
			string::size_type spos1 = s.find_first_of(';', i + 1);
			string::size_type spos2 = s.find_first_of('&', i + 1);

			int32_t num = 0;
			if (spos1 != std::string::npos)
			{
				num = spos1;
				if (spos2 != std::string::npos)
				{
					num = (spos1 < spos2) ? spos1 : spos2;
				}
			}
			else if (spos2 != std::string::npos)
			{
				num = spos2;
			}

			// 取出值
			if (num > 0 && s[num] == ';')
			{
				// 取出值"&amp;","&#39;"
				string text = s.substr(i + 1, num - i - 1);
				if (text.length() > 1 && text[0] == '#')
				{
					try
					{
						c = ((text[1] != 'x' && text[1] != 'X') ? ((char)atoi(text.substr(1).c_str())) : ((char)atoi(text.substr(2).c_str())));
						i = num;
					}
					catch (exception e)
					{
						i++;
					}
				}
				else
				{
					i = num;
					char c2 = Lookup(text); //'\0';
					if (c2 == '\0')
					{
						// 如果没有找到就还原回去
						output << '&';  // output.Write('&');
						output << text; //output.Write(text);
						output << ';';  //output.Write(';');
						continue;
					}
					c = c2;
				}
			}
		}
		output << c; //output.Write(c);
	}
}

// 解码(C#逻辑)
std::string HtmlDecode(string s)
{
	if (s.empty())
	{
		return s;
	}

	string::size_type spos = s.find_first_of('&');
	if (spos == std::string::npos)
	{
		return s;
	}

	stringstream output;
	HtmlDecode(s, output);
	return output.str();
}

// 编码(C#逻辑)
std::string HtmlEncode(string &s)
{
	if (s.empty())
	{
		return s;
	}
	int32_t num = IndexOfHtmlEncodingChars(s, 0);
	if (num == -1)
	{
		return s;
	}
	// StringBuilder stringBuilder = new StringBuilder(s.length() + 5);
	stringstream sstream;
	int32_t length = s.length();
	int32_t num2 = 0;
	while (true)
	{
		if (num > num2)
		{
			// stringBuilder.Append(s, num2, num - num2);// Append(CharSequence s, int start, int end)
			string Appendstr = s.substr(num2, num - num2);
			sstream << Appendstr;
		}
		char c = s[num];
		if (c <= '>')
		{
			switch (c)
			{
			case '<':
				sstream << "&lt;"; // stringBuilder.Append("&lt;");
				break;
			case '>':
				sstream << "&gt;"; // stringBuilder.Append("&gt;");
				break;
			case '"':
				sstream << "&quot;"; // stringBuilder.Append("&quot;");
				break;
			case '&':
				sstream << "&amp;"; // stringBuilder.Append("&amp;");
				break;
			}
		}
		else
		{
			sstream << "&#"; // stringBuilder.Append("&#");
			// int32_t num3 = c;
			// stringBuilder.Append(num3.ToString(NumberFormatInfo.InvariantInfo));
			sstream << to_string((int32_t)c);
			sstream << ';'; // stringBuilder.Append(';');
		}
		num2 = num + 1;
		if (num2 >= length)
		{
			break;
		}
		num = IndexOfHtmlEncodingChars(s, num2);
		if (num == -1)
		{
			string Appendstr = s.substr(num2, length - num2); // stringBuilder.Append(s, num2, length - num2);
			sstream << Appendstr;
			break;
		}
	}
	return sstream.str(); //stringBuilder.ToString();
}

} // namespace HTML


#endif

/*	
	// 
	void HtmlEncode(string s, TextWriter output)
	{
		if (s != null)
		{
			int32_t num = IndexOfHtmlEncodingChars(s, 0);
			if (num == -1)
			{
				output.Write(s);
				return;
			}
			int32_t num2 = s.length() - num;
			fixed(char *ptr = s)
			{
				char *ptr2 = ptr;
				while (num-- > 0)
				{
					char *intPtr = ptr2;
					ptr2 = intPtr + 1;
					output.Write(*intPtr);
				}
				while (num2-- > 0)
				{
					char *intPtr2 = ptr2;
					ptr2 = intPtr2 + 1;
					char c = *intPtr2;
					if (c <= '>')
					{
						switch (c)
						{
						case '<':
							output.Write("&lt;");
							break;
						case '>':
							output.Write("&gt;");
							break;
						case '"':
							output.Write("&quot;");
							break;
						case '&':
							output.Write("&amp;");
							break;
						default:
							output.Write(c);
							break;
						}
					}
					else if (c >= '\u00a0' && c < 'Ā')
					{
						output.Write("&#");
						int32_t num3 = c;
						output.Write(num3.ToString(NumberFormatInfo.InvariantInfo));
						output.Write(';');
					}
					else
					{
						output.Write(c);
					}
				}
			}
		}
	}
		*/