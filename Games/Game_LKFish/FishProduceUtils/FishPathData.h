#ifndef __FishPathData_h__
#define __FishPathData_h__

#include "CMD_Fish.h"
#include <vector>
#include <glog/logging.h>
class FishPathData {
public:
	FishPathData();
	~FishPathData();

public:
	void update(float delta);
	void recycle();
	FishPathData& operator= (FishPathData& other);
	void setInitPosition(short x, short y);
	const FishPoint& getInitPosition() const;
	bool addFishPathParam(FishPathParam* param);
	bool insertPathData(unsigned short index, FishPathParam* param);
	void clearPathData();
	unsigned short getPathCount() const;
	FishPathParam* getFishPathParam(unsigned short index);

        void startTickAt(unsigned int tick);
	void endTickAt(unsigned int tick);

	inline void setElapsed(float elapsed) {
		m_elapsed = elapsed;
	}
	inline float getElapsed() const {
		return m_elapsed;
	}
	inline void setSpeed(short speed) {
		m_speed = speed;
	}
	inline short getSpeed() const {
		return m_speed;
	}
	inline float getDuration() const {
		return m_duration;
	}
    inline bool isDone() const
    {
        if(m_elapsed >= m_duration)
        {
            //LOG(WARNING)<<"delete this fish id ===   "<<m_duration<<"speed===      "<<m_speed<<"   escapsed=== "<<m_elapsed;
        }
        return m_elapsed >= m_duration+5.0;
	}

protected:
	unsigned int getDiffTick(unsigned int startTick, unsigned int endTick);

protected:
	std::vector<FishPathParam*> m_pathDataList;
	FishPoint m_initPosition;
	float m_duration;
	float m_elapsed;
	short m_speed;
	bool  m_started;
	unsigned int m_startTick;
};

// typedef the fish path param content value item now.
typedef std::vector<FishPathParam*> FishPathDataVector;
#endif//__FishPathData_h__
