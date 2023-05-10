#ifndef __FISH_ARRAY_2_LEFT_OUT_H__
#define __FISH_ARRAY_2_LEFT_OUT_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"

class FishArray2Leftout : public FishArrayBase {
public:
	FishArray2Leftout(FishArrayParam2& param) : m_param(&param) {
		
	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml)
        {
                if (xml.Sel("/FishArray/leftout"))
                {
			m_duration = xml.GetLong("duration");
                        if (xml.Sel("small"))
                        {
				m_param->small_fish_count = xml.GetLong("count");
				m_param->small_fish_type = xml.GetLong("fish_type");
				m_param->move_speed_small = xml.GetLong("move_speed");
				m_param->small_fish_time_interval = xml.GetLong("interval");
				xml.Sel("..");
			}

                        if (xml.Sel("big"))
                        {
				m_param->move_speed_big = xml.GetLong("move_speed");
				m_param->big_fish_count = xml.GetLong("count");
				m_param->big_fish_time_interval = xml.GetLong("interval");
				CTextParserUtility txtParser0, txtParser1;
				txtParser0.Parse(xml.GetString("fish_list"), ";");
				for (int j = 0; j < txtParser0.Count(); ++j) {
					FishType& fish_type = m_param->big_fish[m_param->big_fish_kind_count++];
					txtParser1.Parse(txtParser0[j], ",");
					fish_type.type = atoi(txtParser1[0]);
					if (txtParser1.Count() > 1) {
						fish_type.king = atoi(txtParser1[1]);
					}
				}
			}
		}
        //Cleanup:
		return m_duration > 0;
	}

        virtual bool createFishArray() {
		if (!FishArrayBase::createFishArray()) {
			return false;
		}
		bool ret = true;
		int i;
		FishPoint initPoint = { -500, GAME_RESOLUTION_HEIGHT / 2 }, deltaPoint;
		FishType fish_type;
		
		if (m_param->big_fish_kind_count) {
			FishPoint pointTo;
			pointTo.x = GAME_RESOLUTION_WIDTH;
			for (i = 0; i < m_param->big_fish_count; ++i) {
				fish_type = m_param->big_fish[Random::getInstance()->RandomInt(0, m_param->big_fish_kind_count - 1)];
				FishProduceData* fishProduceData = m_iFishProduce->create(fish_type);
				if (fishProduceData) {
					fishProduceData->setShoalType(stSchool);
// 					setFirstAndLastFishID(fishProduceData->getFishKind().id);
					FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
					fishProduceData->setFishPathData(fishPathData);
					fishPathData->setInitPosition(initPoint.x, initPoint.y);
					pointTo.y = Random::getInstance()->RandomInt(0, GAME_RESOLUTION_HEIGHT);
					fishPathData->setElapsed(-(m_param->big_fish_time_interval * i * 0.001f));
					fishPathData->addFishPathParam(FishPathDataFactory::createLine(initPoint, pointTo, 500, m_param->move_speed_big));
				}
			}
		}

		short dy = 100;
		fish_type.type = m_param->small_fish_type;
		for (i = 0; i < m_param->small_fish_count; ++i) {
			FishProduceData* fishProduceData = m_iFishProduce->create(fish_type);
			if (fishProduceData) {
				fishProduceData->setShoalType(stSchool);
// 				setFirstAndLastFishID(fishProduceData->getFishKind().id);
				FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
				fishProduceData->setFishPathData(fishPathData);
				short length = dy + dy + GAME_RESOLUTION_HEIGHT;
				float duration = length / (float)m_param->move_speed_small;
				deltaPoint.x = 0;
				if (i % 2) {
					fishPathData->setInitPosition(Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH), -dy);
					deltaPoint.y = length;
				}
				else {
					deltaPoint.y = -length;
					fishPathData->setInitPosition(Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH), GAME_RESOLUTION_HEIGHT + dy);
				}
				fishPathData->setElapsed(-(m_param->small_fish_time_interval * i * 0.001f));
				fishPathData->addFishPathParam(FishPathDataFactory::createLine(duration, deltaPoint));
			}
		}
// 		calcFishCount();

		return ret;
	}

protected:
	FishArrayParam2* m_param;
};


#endif//__FISH_ARRAY_2_LEFT_OUT_H__
