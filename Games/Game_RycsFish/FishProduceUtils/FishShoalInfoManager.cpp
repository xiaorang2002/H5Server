#include "FishShoalInfoManager.h"

InitilaizeSignletonInstance(FishShoalInfoManager, m_instance);

void FishShoalInfoManager::setFishShoalInfo(const FishShoalInfo& fishShoalInfo) {
	m_fishShoalInfoMap.insert(FishShoalInfoMap::value_type(getKeyWithFishType(fishShoalInfo.fishType), fishShoalInfo));
}

FishShoalInfo* FishShoalInfoManager::getFishShoalInfoWithFishType(const FishType& fishType) {
	if (fishType.king == fktYWDJ)
	{
		return NULL;
	}
	FishShoalInfoMap::iterator iter = m_fishShoalInfoMap.find(getKeyWithFishType(fishType));
	if (iter != m_fishShoalInfoMap.end()) {
		return &iter->second;
	}
	return NULL;
}
