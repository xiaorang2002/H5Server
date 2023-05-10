#ifndef __FISH_ARRAY_1_WELCOME_H__
#define __FISH_ARRAY_1_WELCOME_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"

#define WELCOME_STANDLINE (140)
#define WELCOME_BIGFHLINE (220)

class FishArray1Welcome : public FishArrayBase {
public:
	FishArray1Welcome(FishArrayParam1& param) : m_param(&param) {
	}

public:
	virtual bool initWithXmlFile(CXmlParser& xml)
    {
		if (xml.Sel("/FishArray/welcome"))
		{
			m_duration = xml.GetLong("duration");
            if (xml.Sel("small"))
            {
				m_param->small_fish_count = xml.GetLong("count");
				m_param->small_fish_type = xml.GetLong("fish_type");
				m_param->move_speed_small = xml.GetLong("move_speed");
				xml.Sel("..");
			}

            if (xml.Sel("big"))
            {
				m_param->move_speed_big = xml.GetLong("move_speed");
				m_param->big_fish_time = xml.GetLong("duration");
				m_param->space = xml.GetLong("space");
				int i = 0;
				CTextParserUtility txtParser0, txtParser1;
				char key[][50] = { "fish_list_left", "fish_list_right" };
				while (i < 2)
				{
					txtParser0.Parse(xml.GetString(key[i]), ";");
					for (int j = 0; j < txtParser0.Count(); ++j)
					{
						FishType& fish_kind = m_param->big_fish[i][m_param->big_fish_kind_count[i]++];
						txtParser1.Parse(txtParser0[j], ",");
						fish_kind.type = atoi(txtParser1[0]);
						if (txtParser1.Count() > 1) {
							fish_kind.king = atoi(txtParser1[1]);
						}
					}
					++i;
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
		short dx, dy, sign = 1;
		FishPoint deltaPoint;
		FishType fishType;

		fishType.type = m_param->small_fish_type;
		fishType.king = fktNormal;
		for (i = 0; i < m_param->small_fish_count; ++i) {
			FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
			if (fishProduceData) {
 				//setFirstAndLastFishID(fishProduceData->getFishKind().id);
				fishProduceData->setShoalType(stSchool);
				FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
				fishProduceData->setFishPathData(fishPathData);
				deltaPoint.x = 0;
				dy = Random::getInstance()->RandomInt(0, 300);
				short length = dy + WELCOME_STANDLINE;
				float duration = length / (float)m_param->move_speed_small;
				if (i % 2) {
					fishPathData->setInitPosition(Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH), -dy);
					deltaPoint.y = length;
				} else {
					deltaPoint.y = -length;
					fishPathData->setInitPosition(Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH), GAME_RESOLUTION_HEIGHT + dy);
				}
				fishPathData->addFishPathParam(FishPathDataFactory::createLine(duration, deltaPoint));
				fishPathData->addFishPathParam(FishPathDataFactory::createDelayTime(m_param->big_fish_time * 0.001f));
				deltaPoint.x = 0;
				if (i % 2) {
					deltaPoint.y = GAME_RESOLUTION_HEIGHT;
				} else {
					deltaPoint.y = -GAME_RESOLUTION_HEIGHT;
				}
				fishPathData->addFishPathParam(FishPathDataFactory::createLine((float)abs(deltaPoint.y) / m_param->move_speed_small, deltaPoint));
			}
		}
		deltaPoint.y = 0;
		for (i = 0; i < 2; ++i) {
			for (int j = 0; j != m_param->big_fish_kind_count[i]; ++j) {
				FishType fishType = {m_param->big_fish[i][j].type, m_param->big_fish[i][j].king};
				FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
				if (fishProduceData) {
					fishProduceData->setShoalType(stSchool);
 					//setFirstAndLastFishID(fishProduceData->getFishKind().id);
					FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
					fishProduceData->setFishPathData(fishPathData);
					dx = m_param->space * (j + 1);
					deltaPoint.x = GAME_RESOLUTION_WIDTH + dx + m_param->space;
					if (i % 2) {
						fishPathData->setInitPosition(-dx, WELCOME_BIGFHLINE);
					} else {
						deltaPoint.x = -deltaPoint.x;
						fishPathData->setInitPosition(GAME_RESOLUTION_WIDTH + dx, GAME_RESOLUTION_HEIGHT - WELCOME_BIGFHLINE);
					}
					fishPathData->addFishPathParam(FishPathDataFactory::createLine((float)abs(deltaPoint.x) / m_param->move_speed_big, deltaPoint));
				}
			}
		}
 		//calcFishCount();

		return ret;
	}

protected:
	FishArrayParam1* m_param;
};

#endif//__FISH_ARRAY_1_WELCOME_H__
