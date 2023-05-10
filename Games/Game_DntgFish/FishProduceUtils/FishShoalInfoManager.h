#ifndef __FishShoalInfoManager_h__
#define __FishShoalInfoManager_h__

#include "CMD_Fish.h"
#include <map>

typedef std::map<unsigned short, FishShoalInfo> FishShoalInfoMap;
class FishShoalInfoManager {
	CreateSignleton(FishShoalInfoManager, m_instance);
public:
	void setFishShoalInfo(const FishShoalInfo& fishShoalInfo);
	FishShoalInfo* getFishShoalInfoWithFishType(const FishType& fishType);
	const FishShoalInfoMap& getAllFishShoalInfo() {
		return m_fishShoalInfoMap;
	}
	void removeAll() {
		m_fishShoalInfoMap.clear();
	}
protected:
	FishShoalInfoMap m_fishShoalInfoMap;
};

#endif//__FishShoalInfoManager_h__
