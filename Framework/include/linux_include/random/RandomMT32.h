
#ifndef __RANDOMMT32_HEADER__
#define __RANDOMMT32_HEADER__

#include <stdlib.h>

#define MT32_SH0 1
#define MT32_SH1 10
#define MT32_SH8 8
#define MT32_MASK static_cast<unsigned int>(0x7fffffff)
#define MIN_LOOP 8
#define PRE_LOOP 8

struct MT32_T {
	unsigned int status[4];
	unsigned int mat1;
	unsigned int mat2;
	unsigned int tmat;
};

// random mt data.
class CRandomMT32
{
public:
	CRandomMT32()
	{
		m_nSeed = GetTick();
		Random_Seed(m_nSeed);
	}

public:
	static CRandomMT32& Instance()
	{
		static CRandomMT32 random;
		return random;
	}

public:
	float Random_Float(float min=0, float max=1.0f)
	{
		float fr = tinymt32_generate_float01(&m_mt32);
		return (fr);
	}

	// include the max value now.
	int Random_Int(int min, int max)
	{
		unsigned int val = tinymt32_generate_uint32(&m_mt32);
		return min + (val) % (max - min + 1);
	}

public: // random set the seed now.
	void Random_Seed(int _seed=0)
	{
		// update seed.
		m_nSeed = _seed;
		if (!_seed) {
			m_nSeed = GetTick();
		}

		// random value.
		srand(m_nSeed); 
		int count = 256 + (m_nSeed & 0xFF);
		for (int i=0;i<count;i++) {
			rand();
		}

		// set the special content item now.
		memset(&m_mt32, 0, sizeof(m_mt32));
		m_mt32.mat1  = rand();
		m_mt32.mat2  = rand();
		m_mt32.tmat  = rand();
		int mseed    = rand();
		mt32_init(&m_mt32, mseed);
		for (int i = 0; i < count; i++) {
			Random_Float(0,1.0f);
		}

		/* for check random right. 
		srand(_seed);
		memset(&m_mt32, 0, sizeof(m_mt32));
		m_mt32.mat1 = rand();
		m_mt32.mat2 = rand();
		m_mt32.tmat = rand();
		int mseed   = rand();

		mt32_init(&m_mt32, mseed);
		*/
	}

	// get the special seed value.
	unsigned int GetCurrentSeed()
	{
		return m_nSeed;
	}


protected:
	int GetTick()
	{
		int tick = 0;
#ifdef  _WIN32
		tick = GetTickCount();
#else
		timeval tv = { 0 };
		gettimeofday(&tv, NULL);
		tick = int((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#endif//_WIN32
		return tick;
	}

public:
//protected:
	void mt32_init(MT32_T * random, unsigned int seed) {
		random->status[0] = seed;
		random->status[1] = random->mat1;
		random->status[2] = random->mat2;
		random->status[3] = random->tmat;
		for (int i = 1; i < MIN_LOOP; i++) {
			random->status[i & 3] ^= i + static_cast<unsigned int>(1812433253)
				* (random->status[(i - 1) & 3]
					^ (random->status[(i - 1) & 3] >> 30));
		}

		period_cert(random);
		for (int i = 0; i < PRE_LOOP; i++) {
			mt32_next_state(random);
		}
	}

	// period cert function value item.
	void period_cert(MT32_T * random) {
		if ((random->status[0] & MT32_MASK) == 0 &&
			random->status[1] == 0 &&
			random->status[2] == 0 &&
			random->status[3] == 0) {
			random->status[0] = 'R';
			random->status[1] = 'A';
			random->status[2] = 'N';
			random->status[3] = 'D';
		}
	}

	void mt32_next_state(MT32_T* random) {
		unsigned int x;
		unsigned int y;

		y = random->status[3];
		x = (random->status[0] & MT32_MASK)
			^ random->status[1]
			^ random->status[2];
		x ^= (x << MT32_SH0);
		y ^= (y >> MT32_SH0) ^ x;
		random->status[0] = random->status[1];
		random->status[1] = random->status[2];
		random->status[2] = x ^ (y << MT32_SH1);
		random->status[3] = y;
		random->status[1] ^= -((int)(y & 1)) & random->mat1;
		random->status[2] ^= -((int)(y & 1)) & random->mat2;
	}

	unsigned int tinymt32_temper(MT32_T * random) {
		unsigned int t0, t1;
		t0 = random->status[3];
		t1 = random->status[0]
			+ (random->status[2] >> MT32_SH8);

		t0 ^= t1;
		t0 ^= -((int)(t1 & 1)) & random->tmat;
		return t0;
	}

	float mt32_temper_conv(MT32_T * random) {
		unsigned int t0, t1;
		union {
			unsigned int u;
			float f;
		} conv;

		t0 = random->status[3];
		t1 = random->status[0]
			+ (random->status[2] >> MT32_SH8);

		t0 ^= t1;
		conv.u = ((t0 ^ (-((int)(t1 & 1)) & random->tmat)) >> 9)
			| static_cast<unsigned int>(0x3f800000);
		return conv.f;
	}

protected:
	float tinymt32_generate_float01(MT32_T* random) {
		mt32_next_state(random);
		return mt32_temper_conv(random) - 1.0f;
	}

	unsigned int tinymt32_generate_uint32(MT32_T * random) {
		mt32_next_state(random);
		return tinymt32_temper(random);
	}

protected:
	unsigned int m_nSeed;
	MT32_T m_mt32;
};






#endif//__RANDOMMT32_HEADER__
