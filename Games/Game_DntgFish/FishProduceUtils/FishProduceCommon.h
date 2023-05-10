#ifndef __FishProduceCommon_h__
#define __FishProduceCommon_h__

#include "FishBasicDataManager.h"
#include "FishShoalInfoManager.h"
#include "FishSwimmingPatternManager.h"
#include "FishProduceAllocator.h"
#include "Random.h"
#include "AutoFishPath.h"

class FishProduceCommon {
public:
	static void initializeInstance() {
		Random::getInstance();
		FishProduceAllocator::getInstance();
		FishBasicDataManager::getInstance();
		FishShoalInfoManager::getInstance();
		FishSwimmingPatternManager::getInstance();
		AutoFishPath::getInstance();
	}
	static void destroyInstance() {
		AutoFishPath::destroyInstance();
		FishBasicDataManager::destroyInstance();
		FishShoalInfoManager::destroyInstance();
		FishSwimmingPatternManager::destroyInstance();
		FishProduceAllocator::destroyInstance();
		Random::destroyInstance();
	}
};
#endif//__FishProduceCommon_h__
