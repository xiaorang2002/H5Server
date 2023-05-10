#ifndef __FISH_ARRAY_4_CIR_BOOM_H__
#define __FISH_ARRAY_4_CIR_BOOM_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"

class FishArray4Cirboom : public FishArrayBase
{
public:
	FishArray4Cirboom(FishArrayParam4& param) : m_param(&param) {
		
	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml) {
		if (xml.Sel("/FishArray/cirboom")) {
			m_duration = xml.GetLong("duration");
			if (xml.CountSubet("item")) {
				while(xml.MoveNext()) {
					CirboomParam& para = m_param->param[m_param->param_count++];
					para.count = xml.GetLong("count");
					para.radius = xml.GetLong("radius");
					para.fish_type = xml.GetLong("fish_type");
					para.delay = xml.GetLong("delay") * 0.001f;
					para.move_speed = xml.GetLong("move_speed");
				}
			}
		}
		return m_duration > 0;
	}

        virtual bool createFishArray() {
		if (!FishArrayBase::createFishArray()) {
			return false;
		}
		bool ret = true;
		int i;
		float elapsed = 0.f, initAngle, angle_avg;
		short initX = GAME_RESOLUTION_WIDTH / 2, initY = GAME_RESOLUTION_HEIGHT / 2;
		float length = sqrtf(initX * initX + initY * initY) + 500;
		const CirboomParam* para;

		if (m_param->param_count) {
			for (i = 0; i != m_param->param_count; ++i) {
				para = &m_param->param[i];
				if (para->count > 0) {
					elapsed += para->delay;
					angle_avg = 360.0f / para->count;
					if (i % 2) {
						initAngle = angle_avg / 2;
					} else {
						initAngle = 0;
					}
					FishType fishType = {para->fish_type, fktNormal};
					for (int k = 0; k < para->count; ++k) {
						FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
						if (fishProduceData) {
							fishProduceData->setShoalType(stSchool);
// 							setFirstAndLastFishID(fishProduceData->getFishKind().id);
							FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
							fishProduceData->setFishPathData(fishPathData);
							float angle = initAngle + k * angle_avg;
							short dx = MathUtility::getDxWithAngle(angle, para->radius), dy = MathUtility::getDyWithAngle(angle, para->radius);
							fishPathData->setInitPosition(initX + dx, initY + dy);
							fishPathData->setElapsed(-elapsed);
							fishPathData->addFishPathParam(FishPathDataFactory::createLine(angle, length - para->radius, para->move_speed));
						}
					}
				}
			}
		}
// 		calcFishCount();

		return ret;
	}

protected:
	FishArrayParam4* m_param;
};
#endif//__FISH_ARRAY_4_CIR_BOOM_H__
