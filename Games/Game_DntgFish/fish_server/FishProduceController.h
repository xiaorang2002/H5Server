#ifndef __FISH_PRODUCE_ITEM_H__
#define __FISH_PRODUCE_ITEM_H__

#include "../FishProduceUtils/CMD_Fish.h"
#include "../FishProduceUtils/AutoProduceFish.h"
#include <vector>

class FishProduceManager;

enum eAllowProduceStatus {
	eapsUnkown,
	eapsStart,
	eapsWork,
//	eapsSchedule,
};
class FishProduceController
{
public:
	FishProduceController();
	virtual ~FishProduceController();

public:
	void initialize(IFishProduce* iFishProduce);
	void setAllowSame(BOOL allow) { m_allowSame = allow; }
	void setInterval(unsigned int min, unsigned int max) {
		m_intervalMin = min;
		m_intervalMax = max;
		if (min || max) {
			m_allowSchedule = TRUE;
		} else {
			m_allowSchedule = FALSE;
		}
	}
// 	void setFishCount(unsigned short min, unsigned short max);
	void setDuration(unsigned int min, unsigned int max);
	void addFishType(const FishType& fishType);
	void updateTime(unsigned int deltaTime, bool produce);
	void reset();

	unsigned int getMinDuraton() {
		if (m_allowSchedule) {
			return m_intervalMin;
		} else {
			return m_durationMin;
		}
	}

	unsigned int getMaxDuraton() {
		if (m_allowSchedule) {
			return m_intervalMax;
		} else {
			return m_durationMax;
		}
	}

protected:
	void refreshStatus();
	void makeDuration();

protected:
	std::vector<FishType> m_fishTypesOrg, m_fishTypes;
	unsigned int m_durationMin, m_durationMax, m_duration, m_time;
	unsigned int m_intervalMin, m_intervalMax;
// 	unsigned short m_fishCountMin, m_fishCountMax, m_averageDuration;
// 	int m_fishProduceCount;
	int m_previousIndex;
	BOOL m_allowSame;
	BOOL m_allowSchedule;
	AutoProduceFish m_autoProduceFish;
	IFishProduce* m_iFishProduce;
	eAllowProduceStatus m_status;
};

#endif//__FISH_PRODUCE_ITEM_H__