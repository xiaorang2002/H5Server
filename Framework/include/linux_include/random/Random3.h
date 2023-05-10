#ifndef __RANDOM3_HEADER__
#define __RANDOM3_HEADER__

/*
	使用rand_s生成高质量的随机数.

history:
    2016-01-20  修复新系统第一次没有Container的问题,并加入极端情况下使用rand()生成随机数.
*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef UINT_MAX
#define UINT_MAX (0xFFFFFFFF)
#endif//UINT_MAX

#ifdef  _WIN32
#pragma warning(disable:4005)
#endif//_WIN32

class CRandom3
{
public:
	CRandom3()
	{
    }

    virtual ~CRandom3()
    {
    }

public:
	static CRandom3& Instance()
	{
		static CRandom3* sp = 0;
		if (!sp) {
			 sp = new CRandom3();
		}
	//Cleanup:
		return (*sp);
	}

public:// no use for random seed.
	void Random_Seed(int n) {}

public:
	// include max number. max must <= INT_MAX.
	int Random_Int(int min, int max)
	{
        unsigned int u = GetCryptRandom(); 
        return (min+(u % ((max - min)+1))); 
	}

	float Random_Float(float min, float max)
	{
        unsigned u = GetCryptRandom(); 
        return min+(u)*(1.0f/(float)UINT_MAX)*(max-min);
	}

	int Range(int min, int max)
	{
        unsigned int u = GetCryptRandom(); 
        return (min+(u % ((max - min)+1))); 
	}

	float Range(float min, float max)
	{
		unsigned u = GetCryptRandom(); 
        return min+(u)*(1.0f/(float)UINT_MAX)*(max-min);
	}


protected: // get random.
    UINT GetCryptRandom()
    {
        UINT u=0;
		int fd = open("/dev/random",O_RDONLY|O_NONBLOCK);
		if (fd) 
		{
			int len = read(fd,&u,sizeof(u));
			if (len<=0) 
			{
				int fd2 = open("/dev/urandom", O_RDONLY|O_NONBLOCK);
				if (fd2) 
				{
					len = read(fd2,&u,sizeof(u));					
					if (len<=0) 
					{
						timeval tv;
						gettimeofday(&tv,NULL);
						u=(((tv.tv_sec*1000)+(tv.tv_usec/1000)));
					}
					
					/// closed.
					close(fd2);
				}
			}

			// closed.
			close(fd);
		}
    //Cleanup:
        return u;
    }
};

#define Random3 (*CRandom3::Instance())
#endif//__RANDOM3_HEADER__
