#ifndef __Random_h__
#define __Random_h__

#include <math.h>
#include <time.h>
#include "CMD_Fish.h"
#include <glog/logging.h>
class Random
{
	CreateSignleton(Random, m_instance);
public:
	Random();
public:
    void RandomSeed(int64_t seed = 0);
	int RandomInt(int min, int max);
	float RandomFloat(float min, float max);
    inline int64_t GetNextSeed()
    {
       // m_nSeed = 214013 * m_nSeed + 2531011;

        m_nSeed = (7 * m_nSeed % 2147483647);
        //LOG(WARNING)<<"random seed========="<<m_nSeed;
        return m_nSeed;

    }

    inline int64_t GetCurrentSeed() {
        return m_nSeed;
	}

protected:
    int64_t m_nSeed;
};

#endif//__Random_h__
