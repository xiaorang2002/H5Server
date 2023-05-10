#include "FishPathData.h"
#include "FishProduceAllocator.h"

FishPathData::FishPathData() : m_duration(0.0f)
                             , m_elapsed(0.0f)
//                           , m_randomSeed(0)
                             , m_speed(0)
                             , m_started(false)
                             , m_startTick(0)
{
	memset(&m_initPosition, 0, sizeof(m_initPosition));
}

FishPathData::~FishPathData() {
	// clearPathData();
}

void FishPathData::update(float delta) {
	m_elapsed += delta;
}

void FishPathData::recycle() {
	clearPathData();
	m_elapsed = 0.0f;
	m_speed = 0;
	memset(&m_initPosition, 0, sizeof(m_initPosition));

        m_started = false;
	m_startTick = 0;
}

FishPathData& FishPathData::operator= (FishPathData& other) {
	const FishPathParam* p;
	unsigned short count = other.getPathCount();
	m_elapsed = other.getElapsed();
	m_speed = other.getSpeed();
	m_initPosition = other.getInitPosition();
	clearPathData();
	for (int i = 0; i != count; ++i) {
		p = other.getFishPathParam(i);
		if (p) {
			FishPathParam* pp = FishProduceAllocator::getInstance()->allocFishPathParam();
			*pp = *p;
			addFishPathParam(pp);
		}
	}
	return *this;
}
void FishPathData::setInitPosition(short x, short y) {
	m_initPosition.x = x;
	m_initPosition.y = y;
}
const FishPoint& FishPathData::getInitPosition() const {
	return m_initPosition;
}
bool FishPathData::addFishPathParam(FishPathParam* param) {
	if (!param || param->duration <= 0) {
		FishProduceAllocator::getInstance()->freeFishPathParam(param);
		return false;
	}
	m_pathDataList.push_back(param);
	m_duration += param->duration;
	return true;
}
bool FishPathData::insertPathData(unsigned short index, FishPathParam* param) {
	if (!param || param->duration <= 0) {
		FishProduceAllocator::getInstance()->freeFishPathParam(param);
		return false;
	}
	if (index < 0 || index >= m_pathDataList.size()) {
		return addFishPathParam(param);
	} else {
		m_pathDataList.insert(m_pathDataList.begin() + index, param);
		m_duration += param->duration;
		return true;
	}
	return false;
}
void FishPathData::clearPathData() {
	FishPathParam* p;
	for (int i = 0; i != m_pathDataList.size(); ++i) {
		p = m_pathDataList.at(i);
		if (p) {
			FishProduceAllocator::getInstance()->freeFishPathParam(p);
		}
	}
	m_pathDataList.clear();
	m_duration = 0.0f;
}
unsigned short FishPathData::getPathCount() const {
	return m_pathDataList.size();
}

FishPathParam* FishPathData::getFishPathParam(unsigned short index)
{
	if (index < m_pathDataList.size()) {
		return m_pathDataList.at(index);
	}
	return NULL;
}

void FishPathData::startTickAt(unsigned int tick) {
	m_started = true;
	m_startTick = tick;
}

void FishPathData::endTickAt(unsigned int tick) {
	if (m_started) {
		unsigned int tickDiff = getDiffTick(m_startTick, tick);
		m_elapsed += tickDiff * 0.001f;
		m_started = false;
	}
}

unsigned int FishPathData::getDiffTick(unsigned int startTick, unsigned int endTick) {
	if (startTick > endTick) {
		return 0xFFFFFFFF - startTick + endTick;
	} else {
		return endTick - startTick;
	}
}

