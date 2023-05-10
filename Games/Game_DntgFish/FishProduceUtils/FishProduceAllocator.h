#ifndef __FishProduceAllocator_h__
#define __FishProduceAllocator_h__

#include "CMD_Fish.h"
#include "FishProduceData.h"
#include "MemAlloc.h"

class FishProduceAllocator {
	CreateSignleton(FishProduceAllocator, m_instance);
public:
	FishProduceAllocator();

public:
	FishPathData* allocFishPathData();
	void freeFishPathData(FishPathData* fishPathData);
	FishPathParam* allocFishPathParam();
	void freeFishPathParam(FishPathParam* fishPathParam);
	FishProduceData* allocFishProduceData();
	void freeFishProduceData(FishProduceData* fishProduceData);
	CMD_S_ProduceFish* allocCMD_S_ProduceFish();
	void freeCMD_S_ProduceFish(CMD_S_ProduceFish* fishProduceData);
	CMD_S_ProduceFishShoal* allocCMD_S_ProduceFishShoal();
	void freeCMD_S_ProduceFishShoal(CMD_S_ProduceFishShoal* fishProduceData);

protected:
	MemAlloc<FishPathData> m_fishPathDataPool;
	MemAlloc<FishProduceData> m_fishProduceDataPool;
	MemAlloc<FishPathParam> m_fishPathParamPool;
	MemAlloc<CMD_S_ProduceFish> m_produceFishPool;
	MemAlloc<CMD_S_ProduceFishShoal> m_produceFishShoalPool;
};
#endif//__FishProduceAllocator_h__