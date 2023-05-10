
#ifndef __ANDROIDTIMEDLIMIT_HEADER__
#define __ANDROIDTIMEDLIMIT_HEADER__

#include <string>
#include <vector>
#include <strings.h>


struct tagAndroidTimedLimit
{
	int min_android_count;
	int max_android_count;
	std::string time_start;
	std::string time_end;
};

typedef std::vector<tagAndroidTimedLimit> VectorAndroidLimit;
class CAndroidTimedLimit
{
public:
	CAndroidTimedLimit()
	{
	}

	static CAndroidTimedLimit& Singleton()
	{
		static CAndroidTimedLimit* sp = 0;
		if (!sp) {
	 		 sp = new CAndroidTimedLimit();
		}
	//Cleanup:
		return (*sp);
	}

public:
	int Add(int min_count, int max_count, std::string time_start, std::string time_end)
	{
		int nStatus = -1;
		do 
		{
			// try to init the special content item.
            tagAndroidTimedLimit androidLimit;
			androidLimit.min_android_count = min_count;
			androidLimit.max_android_count = max_count;
			androidLimit.time_start = time_start;
			androidLimit.time_end   = time_end;
			m_vecAndroidLimit.push_back(androidLimit);
			nStatus = 0;
		}   while (0);
	//Cleanup:
		return (nStatus);
	}

	// check if exist item now content value for data item content now.
	bool isExist(std::string nowStr, int& min_count, int& max_count)
	{
		bool bExist = false;
		min_count = 0;
		max_count = 0;
		VectorAndroidLimit::iterator iter;
		for (iter = m_vecAndroidLimit.begin();iter!=m_vecAndroidLimit.end();iter++)
		{
			tagAndroidTimedLimit& limit = *iter;
			// check if time has cross day, if yes, check (time > start || time < end).
			if (strcasecmp(limit.time_end.c_str(),limit.time_start.c_str()) >= 0)
			{
				if ((strcasecmp(nowStr.c_str(),limit.time_start.c_str()) > 0) &&
				    (strcasecmp(nowStr.c_str(),limit.time_end.c_str())  <= 0))
				{
					// update the min and the max value.
					min_count = limit.min_android_count;
					max_count = limit.max_android_count;
					bExist = true;
					break;
				}
			}
			else
			{
				// if cross day. check (time > start || time < end).
				if ((strcasecmp(nowStr.c_str(),limit.time_start.c_str()) > 0)||
				    (strcasecmp(nowStr.c_str(),limit.time_end.c_str())  <= 0))
				{
					// update the min and the max value.
					min_count = limit.min_android_count;
					max_count = limit.max_android_count;
					bExist = true;
					break;
				}
			}

		}
	//Cleanup:
		return (bExist);
	}

protected:
	VectorAndroidLimit  m_vecAndroidLimit;
};


#endif//__ANDROIDTIMEDLIMIT_HEADER__



