#include "FishBasicDataManager.h"

InitilaizeSignletonInstance(FishBasicDataManager, m_instance);

void FishBasicDataManager::setFishBasicData(const FishBasicData& fishBasicData) {
	m_fishBasicDataMap.insert(FishBasicDataMap::value_type(getKeyWithFishType(fishBasicData.fishType), fishBasicData));
}

FishBasicData* FishBasicDataManager::getFishBasicDataWithFishType(const FishType& fishType) {
	FishBasicDataMap::iterator iter = m_fishBasicDataMap.find(getKeyWithFishType(fishType));
	if (iter != m_fishBasicDataMap.end()) {
		return &iter->second;
	} else if (fktSameBomb == fishType.king) {
		FishType _fishType = {fishType.type, fktNormal};
		iter = m_fishBasicDataMap.find(getKeyWithFishType(_fishType));
		if (iter != m_fishBasicDataMap.end()) {
			return &iter->second;
		}
	}
	return NULL;
}
