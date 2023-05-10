#include "Random.h"

InitilaizeSignletonInstance(Random, m_instance);

Random::Random() {
	RandomSeed();
}

void Random::RandomSeed(int64_t seed/* = 0*/)
{
	if(!seed) {
		m_nSeed = (unsigned int)time(0);
	} else {
		m_nSeed = seed;      
	}
    //LOG(WARNING)<<"m_nSeed==========="<<m_nSeed;
}

int Random::RandomInt(int min, int max) {
    if (min > max) {
        int temp = max;
        max = min;
        min = temp;
    }
//    GetNextSeed();
//    return min + (m_nSeed ^ m_nSeed >> 15) % (max - min + 1);
    //LOG(WARNING)<<"RandomIntm_nSeed==============="<<m_nSeed;
    GetNextSeed();
    return m_nSeed % (max - min + 1) + min;
}

float Random::RandomFloat(float min, float max) {
	if (min > max) {
		int temp = max;
		max = min;
		min = temp;
	}
	GetNextSeed();
	return min + (m_nSeed >> 16) * (1.0f / 65535.0f) * (max - min);
}
