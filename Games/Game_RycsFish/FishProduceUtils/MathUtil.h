#pragma once

/*
** Common math constants
*/
#ifndef M_2PI
#define M_2PI   6.28318530717958647692f
#endif
#ifndef M_PI
#define M_PI	3.14159265358979323846f
#define M_PI_2	1.57079632679489661923f
#define M_PI_4	0.785398163397448309616f
#define M_1_PI	0.318309886183790671538f
#define M_2_PI	0.636619772367581343076f
#endif

#ifndef PI_UP_180
#define PI_UP_180    0.01745329251994329576f
#define PI_UNDER_180 57.2957795130823208768f
#endif//PI_UP_180

class MathUtility
{
public:
	static float getAngle(float x, float y)
	{
		return atan2f(x, y) * PI_UNDER_180;
	}
	static float getLength(float x, float y)
	{
		return sqrtf(x * x + y * y);
	}
	static float getDxWithAngle(float angle, float length)
	{
		float rad = angle * PI_UP_180;
		return sinf(rad) * length;
	}
	static float getDyWithAngle(float angle, float length)
	{
		float rad = angle * PI_UP_180;
		return cosf(rad) * length;
	}
};