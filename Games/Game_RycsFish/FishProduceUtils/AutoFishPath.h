#ifndef __AutoFishPath_h__
#define __AutoFishPath_h__

#include "CMD_Fish.h"
#include "FishPathData.h"

class AutoFishPath
{
    CreateSignleton(AutoFishPath, m_instance);

public:
	FishPathData* createFishPath(const FishBasicData* fishBasicData, const FishSwimmingPattern* pattern);
	FishPathData* createFishPathShoal(FishPathData* fishPathLeader, ShoalType shoalType, unsigned short index, const FishBasicData* fishBasicData);
	ShoalType getShoalType(unsigned short& shoalCount, const FishShoalInfo* shoalInfo);

protected:
	FishPoint makeOffsetPoint(const FishPoint& deltaPointOld, const FishPoint& deltaPointAdd);
};

#endif//__AutoFishPath_h__
