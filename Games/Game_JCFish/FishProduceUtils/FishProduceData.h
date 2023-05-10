#ifndef __FishProduceData_h__
#define __FishProduceData_h__

#include "CMD_Fish.h"
#include "FishPathData.h"
#include <string.h>
#include <map>

class FishProduceData
{
public:
	FishProduceData();
	~FishProduceData();

public:
	inline void setFishKind(const FishKind& fishKind) {
		m_fishKind = fishKind;
	}
	inline const FishKind& getFishKind() const {
		return m_fishKind;
	}
	inline void setShoalType(ShoalType shoalType) {
		m_shoalType = shoalType;
	}
	inline ShoalType getShoalType() const {
		return m_shoalType;
	}
        inline void setShoalKind(ShoalType shoalType) {
                m_shoalKind = shoalType;
        }
        inline ShoalType getShoalKind() const {
                return m_shoalKind;
        }
	inline void setRandomSeed(unsigned int randomSeed) {
		m_randomSeed = randomSeed;
	}
	inline unsigned int getRandomSeed() const {
		return m_randomSeed;
	}
	inline void setLeaderFishID(FISH_ID fishID) {
		m_leaderFishID = fishID;
	}
	inline FISH_ID getLeaderFishID() const {
		return m_leaderFishID;
	}
        inline unsigned int getLeaderFishNum() const {
                return leaderFishNum;
        }
        inline unsigned int setLeaderFishNum(const unsigned int num)  {
                 leaderFishNum+=num;
        }
        inline void clearFishNum()  {
              leaderFishNum=0;
        }
	void recycle();
	bool setFishPathData(FishPathData* fishPathData);
	inline FishPathData* getFishPathData() const {
		return m_fishPathData;
	}
	void update(float delta);
	void makeLeaderFishProduceData(FishProduceData* leaderFishProduceData);
    inline bool isAlive() const
    {
        // return m_fishPathData ? !m_fishPathData->isDone() : false;
        bool bAlive = false;
        if ((m_fishPathData != NULL) &&
            (m_fishPathData->isDone() == FALSE)) {
            bAlive = true;
        }
    //Cleanup:
        return (bAlive);
	}

protected:
        int64_t m_randomSeed;
	FishKind m_fishKind;
	ShoalType m_shoalType;
	FISH_ID m_leaderFishID;
	FishPathData* m_fishPathData;
        unsigned int   leaderFishNum;
        ShoalType m_shoalKind;
};
typedef std::map<FISH_ID, FishProduceData*> FishProduceDataMap;

#endif//__FishProduceData_h__
