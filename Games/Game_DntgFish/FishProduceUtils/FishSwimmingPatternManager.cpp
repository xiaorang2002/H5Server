#include "FishSwimmingPatternManager.h"

InitilaizeSignletonInstance(FishSwimmingPatternManager, m_instance);

void FishSwimmingPatternManager::setFishSwimmingPattern(const FishSwimmingPattern& fishSwimmingPattern) {
	m_FishSwimmingPatternMap.insert(FishSwimmingPatternMap::value_type(getKeyWithFishType(fishSwimmingPattern.fishType), fishSwimmingPattern));
}

FishSwimmingPattern* FishSwimmingPatternManager::getFishSwimmingPatternWithFishType(const FishType& fishType) {
	FishSwimmingPatternMap::iterator iter = m_FishSwimmingPatternMap.find(getKeyWithFishType(fishType));
	if (iter != m_FishSwimmingPatternMap.end()) {
		return &iter->second;
	} else if (fktSameBomb ==fishType.king) {
		FishType _fishType = {fishType.type, fktNormal};
		iter = m_FishSwimmingPatternMap.find(getKeyWithFishType(_fishType));
		if (iter != m_FishSwimmingPatternMap.end()) {
			return &iter->second;
		}
	}
	return NULL;
}
