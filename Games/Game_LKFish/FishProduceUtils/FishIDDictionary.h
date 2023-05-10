#ifndef __FishIDDictionary_h__
#define __FishIDDictionary_h__

#include "CMD_Fish.h"

class FishIDDictionary {
public:
	FishIDDictionary() {
		memset(m_aliveList, 0, sizeof(m_aliveList));
	}

	inline void setFishID(FISH_ID fishID) {
		m_aliveList[fishID] = true;
	}
	inline bool exist(FISH_ID fishID) const {
		return m_aliveList[fishID];
	}
protected:
	DECLARE_FISH_ARRAY(bool, m_aliveList);
};
#endif//__FishIDDictionary_h__
