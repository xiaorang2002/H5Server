#ifndef __TICKH_HEADER__
#define __TICKH_HEADER__

#include <sys/time.h>

#define tick() GetTickCount()
// GetTickCount for linux implemnt.
inline unsigned int GetTickCount()
{
	timeval tv;
	gettimeofday(&tv,NULL);
	unsigned int tick = (unsigned int)(((tv.tv_sec*1000)+(tv.tv_usec/1000)));
	return (tick);
}

#endif//__TICKH_HEADER__
