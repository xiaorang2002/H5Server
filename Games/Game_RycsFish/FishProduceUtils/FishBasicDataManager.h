#ifndef __FishBasicDataManager_h__
#define __FishBasicDataManager_h__

#include "CMD_Fish.h"
#include <map>

typedef std::map<unsigned short, FishBasicData> FishBasicDataMap;
class FishBasicDataManager {
	CreateSignleton(FishBasicDataManager, m_instance);
public:
	void setFishBasicData(const FishBasicData& fishBasicData);
	FishBasicData* getFishBasicDataWithFishType(const FishType& fishType);
	const FishBasicDataMap& getAllFishBasicData() const {
		return m_fishBasicDataMap;
	}
	void removeAll() {
		m_fishBasicDataMap.clear();
	}
protected:
	FishBasicDataMap m_fishBasicDataMap;
};

#endif//__FishBasicDataManager_h__
