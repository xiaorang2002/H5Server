#ifndef __FishProduceDataManager_h__
#define __FishProduceDataManager_h__

#include "CMD_Fish.h"
#include "FishProduceData.h"
#include "FishIDDictionary.h"
#include "IFishProduce.h"
#include <map>
#include <vector>
#include "../fish_server/StdAfx.h"
#include "Random.h"

class FishProduceDataManager : public IFishProduce
{
	typedef std::map<unsigned short, short> FishProduceCountWithFishTypeMap;
public:
	FishProduceDataManager();
	~FishProduceDataManager();

public:
	virtual FishProduceData* create(const FishType& fishType);
	virtual bool remove(FISH_ID fishID);
	virtual FishProduceData* get(FISH_ID fishID);
	virtual void setCurrentFishID(FISH_ID id) {
		m_currentFishID = id;
	}
	virtual FISH_ID getCurrentFishID() const {
		return m_currentFishID;
	}
	virtual unsigned short getAliveFishCount() const {
		return m_aliveFishes.size();
	}
	virtual short getAliveFishCountWithFishType(const FishType& fishType) {
		FishProduceCountWithFishTypeMap::iterator iter = m_aliveFishCountWithFishType.find(getKeyWithFishType(fishType));
		if (iter != m_aliveFishCountWithFishType.end()) {
			return iter->second;
		} else {
			return 0;
		}
	}
	virtual bool getbProduceYWDJ() { 
		return m_bProduceYWDJ; 
	}
	void setbProduceSpecialFire(bool bProduce) {
		m_bProduceSpecialFire = bProduce;
	}
	bool getbProduceSpecialFire() {
		return m_bProduceSpecialFire;
	}
	void setHaiLongWangState(int index) {
		m_nHLWState = index;
		short lift = 300;
		switch (m_nHLWState)
		{
		case 0: // ����
			{
				lift = 300 + rand() % (500 - 300);
				break;
			}
		case 1: // �м�
			{
				lift = 500 + rand() % (800 - 500);
				break;
			}
		case 2: // �߼�
			{
				int pro[3] = { 85,10,5 };
				int all = pro[0] + pro[1] + pro[2];
				int randNum = rand() % 100;
				if (randNum < 85)
				{
					lift = 800 + rand() % (1000 - 800);
				}
				else if (randNum < 95)
				{
					lift = 1000 + rand() % (1200 - 1000);
				}
				else
				{
					lift = 1200 + rand() % (1500 - 1200);
				}
				break;
			}
		}

		m_nHLWLife = lift;
	}
	short getHaiLongWangLife() {		
		return m_nHLWLife;
	}

public:
	void update(float delta);
	void removeAll();
	const FishKind* getFishKindWithFishID(FISH_ID fishID);
	bool isAlive(FISH_ID fishID);

	void updateWithEndTick(unsigned int endTick);
	unsigned short popFishesForSending(unsigned char* buffer, unsigned short bufferSize, unsigned int startTick, const CMD_S_HaiLongWangInfo& haiLongWangInfo);

	bool contains(ShoalType shoalType);
	void removeAllExcludeFromDictionary(const FishIDDictionary& dict);
	void pause(unsigned int endTick);
	void resume(unsigned int startTick);
	void reset();
	inline bool getPause() { return m_pause; }
	const FishProduceDataMap& getAliveFishes() const {
		return m_aliveFishes;
	}

private:
    bool setProduceData(FishProduceData* fishProduceData);
	FISH_ID moveToNextFishID();
	FishProduceDataMap::iterator removeWithIter(FishProduceDataMap::iterator& iter);
//	void debugFishID(FishProduceDataMap data) {
//		FishProduceDataMap::iterator iter = data.begin();
//		while (iter != data.end()) {
//			FishProduceData* p = iter->second;
//			FISH_ID id = p->getFishKind().id;
// 			TRACE_INFO(_T("FishProduceDataManager:debugFishID. address=[%u]fishID=[%d]randomSeed=%u"), p, id, p->getRandomSeed());
//// 			if (id == 0) {
//// 				int i = 0;
//// 			}
//			++iter;
//		}
//	}

protected:
	bool m_pause;
	FISH_ID m_currentFishID;
	FishProduceDataMap m_aliveFishes;
	FishProduceDataMap m_fishesForSending;
	FishProduceCountWithFishTypeMap m_aliveFishCountWithFishType;
	bool m_bProduceYWDJ;
	int  m_nHaveYWDJCount;
	bool m_bProduceSpecialFire; //��?��??��2������?��1a?��?����������?��	
	int m_nHLWState;
	int m_nHLWLife;
	int m_bHLWdirection;

#ifdef __MULTI_THREAD__
	CRITICAL_SECTION m_cs;
#endif//____MULTI_THREAD__
};

#endif//__FishProduceDataManager_h__
