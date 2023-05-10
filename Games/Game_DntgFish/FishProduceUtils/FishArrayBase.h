#ifndef __FishArrayBase_h__
#define __FishArrayBase_h__
#include "CMD_Fish.h"
#include "IFishProduce.h"
#include "FishPathDataFactory.h"
#include "Random.h"

#include <xml/XmlParser.h>
#include <TextParserUtility.h>

class FishArrayBase {
public:
	FishArrayBase() : m_duration(0), m_id(0), m_firstFishID(INVALID_FISH_ID), m_lastFishID(INVALID_FISH_ID), /*m_fishCount(0), */m_iFishProduce(NULL), m_randomSeed(0) {}
	virtual ~FishArrayBase() {}

	void setIFishProduce(IFishProduce* iFishProduce) {
		m_iFishProduce = iFishProduce;
	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml) = 0;

public:
	virtual bool createFishArray() {
		m_firstFishID = m_iFishProduce->getCurrentFishID();
		m_randomSeed = Random::getInstance()->GetCurrentSeed();
		return true;
	}
// 	void calcFishCount() {
// 		if (m_lastFishID < m_firstFishID) {
// 			m_fishCount = MAX_FISH_ID - m_firstFishID + 1 + (m_lastFishID - MIN_FISH_ID + 1);
// 		} else {
// 			m_fishCount = m_lastFishID - m_firstFishID + 1;
// 		}
// 	}
	inline void setID(unsigned char id) {
		m_id = id;
	}
	inline unsigned char getID() {
		return m_id;
	}
	inline unsigned int getDuration() {
		return m_duration;
	}
	inline FISH_ID getFirstFishID() {
		return m_firstFishID;
	}
	inline FISH_ID getLastFishID() {
		return m_firstFishID;
	}
// 	inline unsigned short getFishCount() {
// 		return m_fishCount;
// 	}
	inline unsigned int getRandomSeed() {
		return m_randomSeed;
	}
// 	void setFirstAndLastFishID(FISH_ID fishID) {
// 		if (!isValidFishID(m_firstFishID)) {
// 			m_firstFishID = fishID;
// 		}
// 		m_lastFishID = fishID;
// 	}

protected:
	FISH_ID m_firstFishID;
	FISH_ID m_lastFishID;
// 	unsigned short m_fishCount;
	unsigned int m_duration;
	unsigned int m_randomSeed;
	unsigned char m_id;
	IFishProduce* m_iFishProduce;
};
#endif//__FishArrayBase_h__
