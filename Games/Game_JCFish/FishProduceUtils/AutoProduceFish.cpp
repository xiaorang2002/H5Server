

#include "AutoProduceFish.h"
#include "AutoFishPath.h"
#include "FishBasicDataManager.h"
#include "FishSwimmingPatternManager.h"
#include "FishShoalInfoManager.h"

// create the special fish and init path data.
unsigned short AutoProduceFish::create(const FishType& fishType, unsigned int elapsed)
{
    // m_iFishProcude = FishProcudeDataManager.
	FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
	if (!fishProduceData) {
		return 0;
	}

	FishBasicData* fishBasicData = FishBasicDataManager::getInstance()->getFishBasicDataWithFishType(fishType);
	if (!fishBasicData) {
		return 0;
	}

	FishSwimmingPattern* pattern = FishSwimmingPatternManager::getInstance()->getFishSwimmingPatternWithFishType(fishType);
	if (!pattern || !pattern->paramDataCount) {
		return 0;
	}

	FishPathData* fishPathData = AutoFishPath::getInstance()->createFishPath(fishBasicData, pattern);
	if (!fishPathData) {
		m_iFishProduce->remove(fishProduceData->getFishKind().id);
		return 0;
	}

	fishProduceData->setFishPathData(fishPathData);
	fishPathData->setElapsed(fishPathData->getElapsed() - elapsed * 0.001f);
	unsigned short fishCount = 1;
	unsigned short shoalCount = 0;
	const FishShoalInfo* fishShoalInfo = FishShoalInfoManager::getInstance()->getFishShoalInfoWithFishType(fishType);
	ShoalType shoalType = AutoFishPath::getInstance()->getShoalType(shoalCount, fishShoalInfo);
	FishProduceData* fishProduceDataShoal;
	FishPathData* fishPathDataShoal;
	if (shoalType == stFollow || shoalType == stLump) {
		for (unsigned short i = 0; i < shoalCount; ++i) {
			fishProduceDataShoal = m_iFishProduce->create(fishType);
			if (fishProduceDataShoal) {
				fishProduceDataShoal->setLeaderFishID(fishProduceData->getFishKind().id);
				fishProduceDataShoal->setRandomSeed(fishProduceData->getRandomSeed());
				fishProduceDataShoal->setShoalType(shoalType);
				fishPathDataShoal = AutoFishPath::getInstance()->createFishPathShoal(fishPathData, shoalType, i, fishBasicData);
				fishProduceDataShoal->setFishPathData(fishPathDataShoal);
			}
		}
	}

	return fishCount + shoalCount;
}
