#include "FishProduceAllocator.h"
#include "FishProduceData.h"

FishProduceData::FishProduceData() : m_randomSeed(0),leaderFishNum(0), m_fishPathData(NULL), m_shoalType(stSingle), m_leaderFishID(INVALID_FISH_ID) {
	memset(&m_fishKind, 0, sizeof(m_fishKind));
}
FishProduceData::~FishProduceData() {

}
void FishProduceData::recycle() {
	m_randomSeed = 0;
	memset(&m_fishKind, 0, sizeof(m_fishKind));
	m_shoalType = stSingle;
	m_leaderFishID = INVALID_FISH_ID;
	if (m_fishPathData) {
		FishProduceAllocator::getInstance()->freeFishPathData(m_fishPathData);
	}
	m_fishPathData = NULL;
    m_shoalKind=stSingle;
}

bool FishProduceData::setFishPathData(FishPathData* fishPathData)
{
	if (m_fishPathData) {
		FishProduceAllocator::getInstance()->freeFishPathData(m_fishPathData);
	}

	m_fishPathData = fishPathData;
    // this is for test only.
    if (fishPathData == NULL)
    {
        char buf[256]={0};
        // modify by James 181217.
        FILE* fp = fopen("fish_debug.log","a");
        snprintf(buf,sizeof(buf),"setFishPathData,this:%08x,fishPathData:%08x\n",this,fishPathData);
        fputs(buf,fp);
        fclose(fp);
    }

	return true;
}

void FishProduceData::update(float delta) {
	if (m_fishPathData) {
		m_fishPathData->update(delta);
	}
}
void FishProduceData::makeLeaderFishProduceData(FishProduceData* leaderFishProduceData) {
	if (leaderFishProduceData) {
		FishKind fishKind = m_fishKind;
		fishKind.id = m_leaderFishID;
		leaderFishProduceData->setRandomSeed(m_randomSeed);
		leaderFishProduceData->setFishKind(fishKind);
		leaderFishProduceData->setShoalType(stSingle);
	}
}
