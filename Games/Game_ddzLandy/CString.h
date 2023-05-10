
#ifndef __CSTRING_HEADER__
#define __CSTRING_HEADER__

#include <string>
#include <stdarg.h>

#ifdef  _WIN32
#define vsnprintf _vsnprintf
#endif//_WIN32

// Empty()
// AppendFormat()
// AppendChar()

class CString
{
public:
	CString()	{}
	CString(const char* inval) {
		val = inval;
	}

	virtual ~CString() {}

public://Empty
	void Empty()
	{
		val = "";
	}

    bool IsEmpty()
    {
        return (val.length() == 0);
    }

    void Replace(const char* src, const char* dst)
    {
        int pos = 0;
        int src_len = lstrlen(src);
        int dst_len = lstrlen(dst);
        pos = val.find(src,pos);
        while (pos != std::string::npos)
        {
            val = val.replace(pos,src_len,dst);
            pos = val.find(src,(pos+dst_len));
        }
    }

    // get length.
    int GetLength()
    {
        return (val.length());
    }

    // get the at value.
    char GetAt(int idx)
    {
        return (val.at(idx));
    }

    // get the string now.
    const char* c_str()
    {
        return (val.c_str());
    }

    // format the special string now.
    void Format(const char* fmt,...)
    {
        va_list va;
        char buf[1024]={0};
        va_start(va,fmt);
        vsnprintf(buf,sizeof(buf),fmt,va);
        va_end(va);
        val = buf;
    }

	// append the special format content now.
	void AppendFormat(const char* fmt, ...)
	{
		va_list va;
		char buf[1024]={0};
		va_start(va,fmt);
		vsnprintf(buf,sizeof(buf),fmt,va);
		va_end(va);
		val += buf;
	}

	// append special char content.
	void AppendChar(char chValue)
	{
		char buf[2]={0};
		buf[0]=chValue;
		val += buf;
	}

	// try to get the value now.
	operator const char*()  {
		return (val.c_str());
	}

	// append the special data item value now.
	CString& operator=(const char* inval) {
		val  = inval;
		return *this;
	}

    // try to append the special text content.
    CString& operator+=(const char* inval) {
        val += inval;
        return *this;
    }

protected:
	std::string val;
};

#endif//__CSTRING_HEADER__
