#ifndef __FISH_ARRAY_7_CENTER_OUT_H__
#define __FISH_ARRAY_7_CENTER_OUT_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"

class FishArray7Centerout : public FishArrayBase {
public:
	FishArray7Centerout(FishArrayParam7& param) : m_param(&param) {

	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml)
        {
                if (xml.Sel("/FishArray/centerout"))
                {
			m_duration = xml.GetLong("duration");
			m_param->fish_time_delay = xml.GetLong("delay") * 0.001f;
			if (xml.CountSubet("item")) {
				while(xml.MoveNext()) {
					CenterOutParam& para = m_param->param[m_param->param_count++];
					para.count = xml.GetLong("count");
					para.dirs = xml.GetLong("dirs");
					para.fish_type = xml.GetLong("fish_type");
					para.interval = xml.GetLong("interval") * 0.001f;
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
		float elapsed = 0.f;
		float angle_avg, angle_min, angle_max;
		int initX = GAME_RESOLUTION_WIDTH / 2, initY = GAME_RESOLUTION_HEIGHT / 2;
		short length = GAME_RESOLUTION_WIDTH / 2 + 300;
		const CenterOutParam* para;

		if (m_param->param_count) {
			for (i = 0; i != m_param->param_count; ++i) {
				para = &m_param->param[i];
				angle_avg = 360.0f / para->dirs;
				FishType fishType = {para->fish_type, fktNormal};
				for (int k = 0; k < para->count; ++k) {
					for (int j = 0; j < para->dirs; ++j) {
						if (para->count == 1) {
							angle_min = angle_max = angle_avg * j;
						} else {
							angle_min = angle_avg * j + angle_avg / 4;
							angle_max = angle_min + angle_avg / 2;
						}
						FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
						if (fishProduceData) {
							fishProduceData->setShoalType(stSchool);
// 							setFirstAndLastFishID(fishProduceData->getFishKind().id);
							FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
							fishProduceData->setFishPathData(fishPathData);
							fishPathData->setInitPosition(initX, initY);
							fishPathData->setElapsed(elapsed - para->interval * k);
							fishPathData->addFishPathParam(FishPathDataFactory::createLine(Random::getInstance()->RandomFloat(angle_min, angle_max), length, para->move_speed));
						}
					}
				}
				elapsed -= para->count * para->interval + m_param->fish_time_delay;
			}
		}
// 		calcFishCount();

		return ret;
	}

protected:
	FishArrayParam7* m_param;
};

#endif//__FISH_ARRAY_7_CENTER_OUT_H__
