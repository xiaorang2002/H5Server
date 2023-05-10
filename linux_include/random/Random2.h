#ifndef __RANDOM2_HEADER__
#define __RANDOM2_HEADER__

#include <math.h>
#include <types.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#endif//_WIN32

class CRandom2
{
public:
	CRandom2()
	{
		timeval tv;
		gettimeofday(&tv,NULL);
		m_nSeed=(((tv.tv_sec*1000)+(tv.tv_usec/1000))/4294967294);
		m_nTick = m_nSeed;
        Random_Seed(m_nSeed);
    }

public:
	static CRandom2& Instance()
	{
		static CRandom2* sp =0;
		if (!sp) {
			 sp = new CRandom2();
		}
	//Cleanup:
		return (*sp);
	}

public:
    void Random_Seed(unsigned int seed=0)
    {
        if(!seed) 
		{
			timeval tv;
			gettimeofday(&tv,NULL);
			m_nSeed=(((tv.tv_sec*1000)+(tv.tv_usec/1000))/4294967294);
        }
		else
		{
        	m_nSeed=seed;
        }

		// reset last tick.
		m_nTick = m_nSeed;
    }
    
	// 根据tick重设随机因子(确保不跟上一个重复).
	void Random_Seed_newTick(DWORD* poutTick=NULL,DWORD* pnSkipped=NULL)
	{
		timeval tv;
		gettimeofday(&tv,NULL);		
		DWORD dwTick=(((tv.tv_sec*1000)+(tv.tv_usec/1000))/4294967294);
		if (m_nTick != dwTick) {
			m_nTick  = dwTick;
			// 优化随机数,丢弃前面几个随机数,这样更准确.
			DWORD n  = 16 + (dwTick &0x0F);
			Random_Seed(dwTick);
			for (DWORD i=0;i<n;i++) {
				Random_Float(0,1.0f);
			}

			/// return the special content.
			if (poutTick)  *poutTick=dwTick;
			if (pnSkipped) *pnSkipped=n;
		}
	}

	// include max.
	int Random_Int(int min, int max)
	{
        GetNextSeed();
        return min+(m_nSeed ^ m_nSeed>>15)%(max-min+1);
	}

	float Random_Float(float min, float max)
	{
        GetNextSeed();
        //return min+m_nSeed*(1.0f/4294967295.0f)*(max-min);
        return min+(m_nSeed>>16)*(1.0f/65535.0f)*(max-min);
	}

	int Range(int min, int max)
	{
		GetNextSeed();
		return min + (m_nSeed ^ m_nSeed >> 15) % (max - min + 1);
	}

	float Range(float min, float max)
	{
		GetNextSeed();
		return min + (m_nSeed >> 16) * (1.0f / 65535.0f) * (max - min);
	}

	unsigned int GetNextSeed()
	{
		return m_nSeed = 214013 * m_nSeed + 2531011;
	}

	unsigned int GetCurrentSeed() 
	{
		return m_nSeed;
	}

public:
	unsigned long m_nTick;	// 记录最后一处tickcount.

protected:
	unsigned int  m_nSeed;
};

#define Random2 (*CRandom2::Instance())
#endif//__RANDOM2_HEADER__
