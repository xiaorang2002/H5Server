#ifndef __AutoProduceFish_h__
#define __AutoProduceFish_h__

#include "CMD_Fish.h"
#include "IFishProduce.h"

class AutoProduceFish {
public:
	AutoProduceFish() : m_iFishProduce(NULL) {}
	~AutoProduceFish() {}
public:
	void setIFishProduce(IFishProduce* iFishProduce) {
		m_iFishProduce = iFishProduce;
	}
	unsigned short create(const FishType& fishType, unsigned int elapsed);
	bool getbProduceYWDJ() { 
		if (m_iFishProduce != NULL)
		{
			bool bProdece = m_iFishProduce->getbProduceYWDJ();
			return bProdece;
		}
		return true;
	}
	bool getbProduceSpecialFire() {
		if (m_iFishProduce != NULL)
		{
			bool bProdece = m_iFishProduce->getbProduceSpecialFire();
			return bProdece;
		}
		return true;
	}

	void setHLWdirection(int direction) {
		if (direction == fpscLeft)
		{
			m_bHaiLongWangDirection = fpscRight;
		} 
		else
		{
			m_bHaiLongWangDirection = fpscLeft;
		}
		
	}
	int getHLWdirection() {
		return m_bHaiLongWangDirection;
	}

protected:
	IFishProduce* m_iFishProduce;
	int m_bHaiLongWangDirection;
};

#endif//__AutoProduceFish_h__
