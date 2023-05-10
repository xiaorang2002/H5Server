#ifndef __FishSwimmingPatternManager_h__
#define __FishSwimmingPatternManager_h__

#include "CMD_Fish.h"
#include <map>

typedef std::map<unsigned short, FishSwimmingPattern> FishSwimmingPatternMap;
class FishSwimmingPatternManager {
	CreateSignleton(FishSwimmingPatternManager, m_instance);
public:
	void setFishSwimmingPattern(const FishSwimmingPattern& fishSwimmingPattern);
	FishSwimmingPattern* getFishSwimmingPatternWithFishType(const FishType& fishType);
	const FishSwimmingPatternMap& getAllFishSwimmingPattern() const {
		return m_FishSwimmingPatternMap;
	}
	void removeAll() {
		m_FishSwimmingPatternMap.clear();
	}
protected:
	FishSwimmingPatternMap m_FishSwimmingPatternMap;
};

#endif//__FishSwimmingPatternManager_h__
