
#include "StdAfx.h"
#include "FishProduceManager.h"
#include "../FishProduceUtils/FishBasicDataManager.h"
#include "../FishProduceUtils/FishShoalInfoManager.h"
#include "../FishProduceUtils/FishSwimmingPatternManager.h"

#include <console/Consoledump.h>

FishProduceManager::FishProduceManager() : m_fishProduceDataManager(NULL), m_produceCountMin(0)
, m_produceCountMax(0), m_allowProduceCount(0), m_startIndex(0) {
	
}

FishProduceManager::~FishProduceManager() {
	deleteAllObject();
}

bool FishProduceManager::initFromXml(CXmlParser& xml)
{
	FishType fishType;
	CTextParserUtility txtParser;
	if (xml.Sel("/Fish")) {
		if (xml.Sel("Basic")) {
			if (xml.CountSubet("Item") > 0) {
				while(xml.MoveNext()) {
					fishType.type = xml.GetLong("ID");
                    fishType.Onlytype = xml.GetLong("Only");
					fishType.king = fktNormal;
					createFishProperty(fishType, xml);
				}
			}
			xml.Sel("..");
		}
// 		if (xml.Sel("Tlzd")) {
// 			txtParser.Parse(xml.GetString("FishIDs"), ",");
// 			int count = txtParser.Count();
// 			for (int i = 0; i < count; ++i) {
// 				fishType.type = atoi(txtParser.getStringAt(i));
// 				fishType.king = fktNormal;
// 				FishProperty* fishProp = getFishPropertyWithFishType(fishType);
// 				if (fishProp) {
// 					fishProp = fishProp->clone();
// 					if (fishProp) {
// 						fishType.king = fktSameBomb;
// 						fishProp->setFishType(fishType);
// 						fishProp->setFishName(_T(""));
// 						// push data.
// 						addFishProperty(fishProp);
// 					}
// 				}
// 			}
// 			xml.Sel("..");
// 		}
		if (xml.Sel("Shoal")) {
			if (xml.CountSubet("Item") > 0) {
				while (xml.MoveNext()) {
					FishShoalInfo fishShoalInfo = {0};
					fishShoalInfo.fishType.type = xml.GetLong("FishID");
                    fishShoalInfo.fishType.Onlytype = xml.GetLong("Only");
					fishShoalInfo.fishType.king = fktNormal;
					fishShoalInfo.genRatio = xml.GetLong("GenRatio");
					txtParser.Parse(xml.GetString("Amount"), "~");
					fishShoalInfo.amountMin = fishShoalInfo.amountMax = atoi(txtParser.getStringAt(0));
					if (txtParser.Count() > 1) {
						fishShoalInfo.amountMax = atoi(txtParser.getStringAt(1));
					}
					fishShoalInfo.allowFollow = xml.GetLong("allowFollow");
					fishShoalInfo.allowLump = xml.GetLong("allowLump");
					FishShoalInfoManager::getInstance()->setFishShoalInfo(fishShoalInfo);
				}
			}
			xml.Sel("..");
		}

		if (xml.Sel("Boom")) {
			fishType.type = 0;
            fishType.Onlytype = xml.GetLong("Only");
			if (xml.Sel("Pause")) {
				fishType.king = fktPauseBomb;
				createFishProperty(fishType, xml);
				xml.Sel("..");
			}
			if (xml.Sel("Area")) {
				fishType.king = fktAreaBomb;
				createFishProperty(fishType, xml);
				xml.Sel("..");
			}
			if (xml.Sel("All")) {
				fishType.king = fktGlobalBomb;
				createFishProperty(fishType, xml);
				xml.Sel("..");
			}
			xml.Sel("..");
		}
		if (xml.Sel("Zuhe")) {
			if (xml.CountSubet("Item") > 0) {
				while(xml.MoveNext()) {
                    fishType.Onlytype = xml.GetLong("Only");
					fishType.type = xml.GetLong("ID");
					fishType.king = fktCombination;
					createFishProperty(fishType, xml);
				}
			}
			xml.Sel("..");
		}
		//一网打尽
		if (xml.Sel("YWDJ")) {
			if (xml.CountSubet("Item") > 0) {
				while (xml.MoveNext()) {
					fishType.Onlytype = xml.GetLong("Only");
					fishType.type = xml.GetLong("ID");
					fishType.king = fktYWDJ;
					createFishProperty(fishType, xml);
				}
			}
			xml.Sel("..");
		}
		xml.Sel("..");
	} else {
		return false;
	}
	if (xml.Sel("/Produce")) {
		CTextParserUtility txtParser;
		txtParser.Parse(xml.GetString("NumberOfFish"), "~");
		m_produceCountMin = m_produceCountMax = atoi(txtParser.getStringAt(0));
		if (txtParser.Count() > 1) {
			m_produceCountMax = atoi(txtParser.getStringAt(1));
		}
		if (xml.CountSubet("Item") > 0) {
			while (xml.MoveNext()) {
				addController(xml);
			}
		}
	}
	resetProduceCount();

	return true;
}

void FishProduceManager::addController(CXmlParser& xml) {
	CTextParserUtility txtParser, txtParserFishType;
	FishProduceController* controller = new FishProduceController;
	FishType fishType = {0};

	controller->initialize(m_fishProduceDataManager);
	txtParser.Parse(xml.GetString("FishTypes"), ";");
    for (int i = 0; i < txtParser.Count(); ++i)
    {
		txtParserFishType.Parse(txtParser.getStringAt(i), ",");
        fishType.type = atoi(txtParserFishType.getStringAt(0));
        if(fishType.type<=18)
        {
            fishType.Onlytype =fishType.type;
        }
        else
        {
            fishType.Onlytype =fishType.type-1;
        }
        if (txtParserFishType.Count() > 1)
        {
			fishType.king = atoi(txtParserFishType.getStringAt(1));
        } else
        {
			fishType.king = fktNormal;
		}
		controller->addFishType(fishType);
	}
	txtParser.Parse(xml.GetString("Duration"), "~");
	if (txtParser.Count() > 1) {
		controller->setDuration(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(1)));
	}
	else {
		controller->setDuration(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(0)));
	}
// 	txtParser.Parse(xml.GetString("Number"), "~");
// 	if (txtParser.Count() > 1) {
// 		controller->setFishCount(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(1)));
// 	}
// 	else {
// 		controller->setFishCount(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(0)));
// 	}
	controller->setAllowSame(xml.GetLong("AllowSame"));
	txtParser.Parse(xml.GetString("Interval"), "~");
	if (txtParser.Count() > 1) {
		controller->setInterval(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(1)));
	} else {
		controller->setInterval(atoi(txtParser.getStringAt(0)), atoi(txtParser.getStringAt(0)));
	}
	controller->reset();
	m_fishProduceControllers.insert(FishProduceControllerMap::value_type(controller->getMinDuraton() + m_fishProduceControllers.size(), controller));
}

const TCHAR* FishProduceManager::getFishName(const FishType& fishType) {
	const FishBasicData* fishBasicData = FishBasicDataManager::getInstance()->getFishBasicDataWithFishType(fishType);
	if (!fishBasicData) {
		if (fktSameBomb == fishType.king) {
			return _T("同类炸弹");
		} else {
			return _T("");
		}
	}
	return fishBasicData->fishName;
}

void FishProduceManager::createFishProperty(const FishType& fishType, CXmlParser& xml)
{
	const char* node;
	CTextParserUtility txtParser;
	FishBasicData fishBasicData = {0};
	fishBasicData.fishType = fishType;
	txtParser.Parse(xml.GetString("Life"), "~");
	if (txtParser.Count() > 1) {
		fishBasicData.lifeMin = atoi(txtParser.getStringAt(0));
		fishBasicData.lifeMax = atoi(txtParser.getStringAt(1));
	} else {
		fishBasicData.lifeMin = fishBasicData.lifeMax = atoi(txtParser.getStringAt(0));
	}
#ifdef __USE_LIFE2__
	node = xml.GetString("Life2");
	if (strlen(node)) {
		txtParser.Parse(xml.GetString("Life2"), "~");
		if (txtParser.Count() > 1) {
			fishBasicData.life2Min = atoi(txtParser.getStringAt(0));
			fishBasicData.life2Max = atoi(txtParser.getStringAt(1));
		} else {
			fishBasicData.life2Min = fishBasicData.life2Max = atoi(txtParser.getStringAt(0));
		}
	} else {
		fishBasicData.life2Min = fishBasicData.life2Max = 100;
	}
#endif
	node = xml.GetString("Name");
	if (node && strlen(node))
    {
#ifdef  UNICODE
		MultiByteToWideChar(CP_UTF8, 0, node, -1, fishBasicData.fishName, 200);
#else
        lstrcpyn(fishBasicData.fishName, node, CountArray(fishBasicData.fishName));
#endif//UNICODE
	}
	txtParser.Parse(xml.GetString("BoundingBox"), ",");
	fishBasicData.boundingWidth = atoi(txtParser.getStringAt(0));
	fishBasicData.boundingHeight = atoi(txtParser.getStringAt(1));
	FishBasicDataManager::getInstance()->setFishBasicData(fishBasicData);
	
	FishSwimmingPattern fishSwimmingPattern = {0};
	fishSwimmingPattern.fishType = fishType;
	CXmlParser _xml;
	_xml.Attach(xml.pNode);
	if (_xml.Sel("SwimmingPattern")) {
		if (_xml.CountSubet("Item") > 0) {
			unsigned char index = 0;
			while (_xml.MoveNext()) {
				FishSwimmingParam& param = fishSwimmingPattern.swimmingParam[index++];
				param.pathType = (PathType)_xml.GetLong("PathType");
				param.speed = _xml.GetLong("Speed");
				txtParser.Parse(_xml.GetString("DeltaLength"), "~");
				if (txtParser.Count() > 1) {
					param.minLength = atoi(txtParser.getStringAt(0));
					param.maxLength = atoi(txtParser.getStringAt(1));
				} else {
					param.minLength = param.maxLength = atoi(txtParser.getStringAt(0));
				}
				txtParser.Parse(_xml.GetString("DeltaRotation"), "~");
				if (txtParser.Count() > 1) {
					param.minRotation = atoi(txtParser.getStringAt(0));
					param.maxRotation = atoi(txtParser.getStringAt(1));
				} else {
					param.minRotation = param.maxRotation = atoi(txtParser.getStringAt(0));
				}
			}
			fishSwimmingPattern.paramDataCount = index;
			FishSwimmingPatternManager::getInstance()->setFishSwimmingPattern(fishSwimmingPattern);
		}
	}
}

bool FishProduceManager::produceFish(DWORD deltaTime) {
	unsigned short index = 0;
	if (m_startIndex >= m_fishProduceControllers.size()) {
		m_startIndex = 0;
	}
	bool setFlag = false;
	FishProduceControllerMap::reverse_iterator iter = m_fishProduceControllers.rbegin();
	while (iter != m_fishProduceControllers.rend()) {
		if (index >= m_startIndex) {
			if (m_fishProduceDataManager->getAliveFishCount() >= m_allowProduceCount) {
				if (!setFlag) {
					m_startIndex = index;
					setFlag = true;
				}
				iter->second->updateTime(deltaTime, false);
			} else {
				iter->second->updateTime(deltaTime, true);
			}
		} else {
			iter->second->updateTime(deltaTime, false);
		}
		++iter;
		++index;
	}
	if (!setFlag) {
		m_startIndex = 0;
	}
	return true;
}

void FishProduceManager::reset() {
	m_startIndex = 0;
	resetProduceCount();
	for (FishProduceControllerMap::iterator iter = m_fishProduceControllers.begin();
		iter != m_fishProduceControllers.end(); ++iter) {
		iter->second->reset();
	}
}
