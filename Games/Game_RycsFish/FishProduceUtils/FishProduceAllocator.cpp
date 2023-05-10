#include "FishProduceAllocator.h"
//#include "../fish_server/stdafx.h"

InitilaizeSignletonInstance(FishProduceAllocator, m_instance);

FishProduceAllocator::FishProduceAllocator() {
	m_fishPathDataPool.reserve(500);
	m_fishProduceDataPool.reserve(500);
	m_fishPathParamPool.reserve(1000);
	m_produceFishPool.reserve(500);
	m_produceFishShoalPool.reserve(500);
}
FishPathData* FishProduceAllocator::allocFishPathData() {
	return m_fishPathDataPool.alloc();
}
void FishProduceAllocator::freeFishPathData(FishPathData* fishPathData) {
	fishPathData->recycle();
	m_fishPathDataPool.free(fishPathData);
}
FishPathParam* FishProduceAllocator::allocFishPathParam() {
	return m_fishPathParamPool.alloc();
}
void FishProduceAllocator::freeFishPathParam(FishPathParam* fishPathParam) {
	m_fishPathParamPool.free(fishPathParam);
}
FishProduceData* FishProduceAllocator::allocFishProduceData() {
	FishProduceData* p = m_fishProduceDataPool.alloc();
	//debug("FishProduceAllocator::allocFishProduceData.[%u]", p);
	return p;
}
void FishProduceAllocator::freeFishProduceData(FishProduceData* fishProduceData) {
	fishProduceData->recycle();
	//debug("FishProduceAllocator::freeFishProduceData.[%u]", fishProduceData);
	m_fishProduceDataPool.free(fishProduceData);
}
CMD_S_ProduceFish* FishProduceAllocator::allocCMD_S_ProduceFish() {
	return m_produceFishPool.alloc();
}
void FishProduceAllocator::freeCMD_S_ProduceFish(CMD_S_ProduceFish* produceFish) {
	memset(produceFish, 0, sizeof(CMD_S_ProduceFish));
	m_produceFishPool.free(produceFish);
}
CMD_S_ProduceFishShoal* FishProduceAllocator::allocCMD_S_ProduceFishShoal() {
	return m_produceFishShoalPool.alloc();
}
void FishProduceAllocator::freeCMD_S_ProduceFishShoal(CMD_S_ProduceFishShoal* produceFishShoal) {
	memset(produceFishShoal, 0, sizeof(CMD_S_ProduceFishShoal));
	m_produceFishShoalPool.free(produceFishShoal);
}
