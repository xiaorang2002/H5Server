#ifndef __IFishProduce_h__
#define __IFishProduce_h__

#include "FishProduceData.h"
#include "CMD_Fish.h"

class IFishProduce {
public:
	virtual FishProduceData* create(const FishType& fishType) = 0;
	virtual bool remove(FISH_ID fishID) = 0;
	virtual FishProduceData* get(FISH_ID fishID) = 0;
	virtual FISH_ID getCurrentFishID() const = 0;
	virtual void setCurrentFishID(FISH_ID id) = 0;
	virtual unsigned short getAliveFishCount() const = 0;
	virtual short getAliveFishCountWithFishType(const FishType& fishType) = 0;
	virtual bool getbProduceYWDJ() = 0;
	virtual bool getbProduceSpecialFire() = 0;
};
#endif//__IFishProduce_h__