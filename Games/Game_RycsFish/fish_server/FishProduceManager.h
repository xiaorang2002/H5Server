#ifndef __FISH_PRODUCE_MANAGER_H__
#define __FISH_PRODUCE_MANAGER_H__

#include "../FishProduceUtils/CMD_Fish.h"
#include "../FishProduceUtils/Random.h"
#include <TextParserUtility.h>
#include "FishProduceController.h"
#include "../FishProduceUtils/FishProduceDataManager.h"
#include <xml/XmlParser.h>

class FishProduceManager
{
typedef std::map<unsigned short, FishProduceController*> FishProduceControllerMap;
public:
	FishProduceManager();
	~FishProduceManager();

public:
	void setFishProduceDataManager(FishProduceDataManager* fishProduceDataManager) {
		m_fishProduceDataManager = fishProduceDataManager;
	}
	bool initFromXml(CXmlParser& xml);
	bool produceFish(DWORD deltaTime);
	const TCHAR* getFishName(const FishType& fishType);
	void reset();

private:
	void createFishProperty(const FishType& fishType, CXmlParser& xml);
	void deleteAllObject() {
		for (FishProduceControllerMap::iterator iter = m_fishProduceControllers.begin();
			iter != m_fishProduceControllers.end(); ++iter) {
			delete iter->second;
		}
		m_fishProduceControllers.clear();
	}
	void addController(CXmlParser& xml);
	void resetProduceCount() {
		m_allowProduceCount = Random::getInstance()->RandomInt(m_produceCountMin, m_produceCountMax);
        //LOG(WARNING)<<"resetProduceCount  --->randomint()";
    }

protected:
	FishProduceDataManager* m_fishProduceDataManager;
	FishProduceControllerMap m_fishProduceControllers;
	unsigned short m_produceCountMin, m_produceCountMax, m_allowProduceCount;
	unsigned short m_startIndex;
};

#endif//__FISH_PRODUCE_MANAGER_H__
