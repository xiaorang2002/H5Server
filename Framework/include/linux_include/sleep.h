#ifndef __MYSLEEP_HEADER__
#define __MYSLEEP_HEADER__

#include <unistd.h>

inline void sleep(int minillisec)
{
	usleep(minillisec * 1000);
}

#endif//__MYSLEEP_HEADER__

