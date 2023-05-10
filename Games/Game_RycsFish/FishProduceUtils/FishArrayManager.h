#ifndef __FishArrayManager_h__
#define __FishArrayManager_h__

#include <xml/XmlParser.h>

#include "array1welcome.h"
#include "array2leftout.h"
#include "array3circle.h"
#include "array4cirboom.h"
#include "array5circle2.h"
#include "array6crossout.h"
#include "array7centerout.h"
#include "array8fourcorners.h"
#include "IFishProduce.h"
#include <vector>

#include <muduo/base/Logging.h>

class FishArrayManager {
public:
	FishArrayManager() : m_fishArrayCount(0) {
		memset(m_fishArray, 0, sizeof(m_fishArray));
	}
	~FishArrayManager() {
		reset();
	}

public:
    bool InitWithXmlFile(CXmlParser& xml, FishArrayParam& param) {
		if (!initWithFishArrayParam(param)) {
			return false;
		}
		for (unsigned char i = 0; i < m_fishArrayCount; ++i) {
			if (m_fishArray[i]) {
				m_fishArray[i]->initWithXmlFile(xml);
			}
		}
		return true;		
	}

	bool initWithFishArrayParam(FishArrayParam& param) {
		reset();
		m_fishArray[m_fishArrayCount++] = new FishArray1Welcome(param.param1);//小鱼0
		m_fishArray[m_fishArrayCount++] = new FishArray2Leftout(param.param2);
		m_fishArray[m_fishArrayCount++] = new FishArray3Circle(param.param3);
		m_fishArray[m_fishArrayCount++] = NULL;//new FishArray4Cirboom(param.param4);
		m_fishArray[m_fishArrayCount++] = NULL;//new FishArray5Circle2(param.param5);
		m_fishArray[m_fishArrayCount++] = NULL;//new FishArray6Crossout(param.param6);
		m_fishArray[m_fishArrayCount++] = NULL;//new FishArray7Centerout(param.param7);
		m_fishArray[m_fishArrayCount++] = new FishArray8FourCorners(param.param8);
		for (unsigned char i = 0; i < m_fishArrayCount; ++i) {
			if (m_fishArray[i]) {
				m_fishArray[i]->setIFishProduce(m_iFishProduce);
                                m_fishArray[i]->setID(i /*+ 1*/);
				m_randomListOrg.push_back(i);
			}
		}
		return true;
	}
	void setIFishProduce(IFishProduce* iFishProduce) {
		m_iFishProduce = iFishProduce;
	}

	FishArrayBase* getNextFishArray() {
		//return m_fishArray[1];
		if (!m_fishArrayCount) {
			return NULL;
		}

		// init and reset random id list.
		if (0 == m_randomList.size()) {
			m_randomList = m_randomListOrg;
		}

		// try to get the special index of the random fish array item content now.
		int index = Random::getInstance()->RandomInt(0, m_randomList.size() - 1);
		// modify by James for test only, use fish array by sequeuce like 0,1,2,3,4,5,...
		// index = 0;
		// modify by James end.

		FishArrayBase* fishArray = m_fishArray[m_randomList.at(index)];
		// erase current used fish array item id for use.
		m_randomList.erase(m_randomList.begin() + index);
	//Cleanup:
		return fishArray;
	}

	FishArrayBase* getFishArrayWithID(unsigned char id) {
		for (unsigned char i = 0; i < m_fishArrayCount; ++i) {
			if (id == m_fishArray[i]->getID()) {
				return m_fishArray[i];
			}
		}
		return NULL;
	}

protected:
	void reset() {
		for (unsigned short i = 0; i != m_fishArrayCount; ++i) {
			FishArrayBase* p = m_fishArray[i];
			if (p) {
				delete p;
				p = NULL;
			}
		}
		m_fishArrayCount = 0;
	}
protected:
	FishArrayBase* m_fishArray[32];
	unsigned short m_fishArrayCount;
	std::vector<unsigned short> m_randomList;
	std::vector<unsigned short> m_randomListOrg;
	IFishProduce* m_iFishProduce;
};
#endif//__FishArrayManager_h__
