/*
    fast string pool with initialized buffer.
    push 1000000 ~= 100ms.
*/

#ifndef __STRINGBUF_HEADER__
#define __STRINGBUF_HEADER__

typedef char   tchar;

class cstringbuf
{
public://1M.
    cstringbuf()
    {
        buf   = 0;
        size  = 0;
        repet = 0;
        rpos  = 0;
        wpos  = 0;
        cnt   = 0;
    }

    cstringbuf(int bufsize)
    {
        size = bufsize;
        buf = new tchar[size+3];
        memset(buf,0,size+3);
        repet = 0;
        rpos  = 0;
        wpos  = 0;
        cnt   = 0;
    }

    virtual ~cstringbuf()
    {
        if (buf) {
            delete[] buf;
            buf=0;
        };
    }

public:
    static cstringbuf& Singleton()
    {
        static cstringbuf* sp=0;
        if (!sp) {
             sp = new cstringbuf();
        }
    //Cleanup:
        return (*sp);
    }

public://8M.
    int init(long bufsize=(1<<23))
    {
        size = bufsize;
        buf = new tchar[size+3];
        memset(buf,0,size+3);
        return (0);
    }

    void term()
    {
        if (buf) {
            delete[] buf;
            buf=0;
        }
    }

public:
    int push_back(const tchar* ptr)
    {
        if ((!ptr)||(!buf)) return 0;
        int len = lstrlen(ptr);
        if ((wpos + len) >= size) {
             repet = 1;
             wpos = 0;
        }

        // get the data content.
        tchar* dst = &buf[wpos];
        lstrcpyn(dst,ptr,len+1);
        if (!repet) cnt += 1;
        wpos += len+1;
        return (wpos);
    }

    void clear()
    {
        memset(buf,0,size+3);
        repet = 0;
        wpos  = 0;
        rpos  = 0;
        cnt   = 0;
    }

    void movefirst()
    {
        rpos = 0;
    }

    // try to move now.
    tchar* movenext()
    {
        tchar* p = &buf[rpos];
        int len = lstrlen(p);
        if (0 == len) {
            return 0;
        }

        /// move next.
        rpos += len+1;
    //Cleanup:
        return (p);
    }

public:
    int count()
    {
        return cnt;
    }

    int length()
    {
        return (wpos);
    }

public:
    CStringbuf& operator=(CStringbuf& inbuf)
    {
        cnt   = inbuf.cnt;
        rpos  = inbuf.rpos;
        wpos  = inbuf.wpos;
        repet = inbuf.repet;
        // try to check the size and content.
        if ((size) && (size == inbuf.size))
        {
            memset(buf,0,size+3);
        }  else
        {
            term();
            init(inbuf.size);
        }
    //Cleanup:
        int len = wpos;
        if (repet) len=size;
        memcpy(buf,inbuf.buf,len);
        return (*this);
    }


protected:
    int   size;  // size.
    tchar* buf;  // buffer.
    long  rpos;  // read pos.
    long  wpos;  // write pos.
    int  repet;  // overflow.
    long   cnt;  // count.
};

#endif//__STRINGBUF_HEADER__
