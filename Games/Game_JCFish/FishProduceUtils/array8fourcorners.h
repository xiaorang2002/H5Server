#ifndef __FISH_ARRAY_8_FOUR_CORNERS_H__
#define __FISH_ARRAY_8_FOUR_CORNERS_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"

class FishArray8FourCorners : public FishArrayBase {
public:
	FishArray8FourCorners(FishArrayParam8& param) : m_param(&param) {

	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml) {
		if (xml.Sel("/FishArray/fourcorners")) {
			m_duration = xml.GetLong("duration");
			if (xml.Sel("small")) {
				m_param->small_fish_count = xml.GetLong("count");
				m_param->small_fish_type = xml.GetLong("fish_type");
				m_param->move_speed_small = xml.GetLong("move_speed");
				m_param->small_fish_time_interval = xml.GetLong("interval") * 0.001f;
				xml.Sel("..");
			}
			if (xml.Sel("big")) {
				m_param->big_fish_time_interval = xml.GetLong("interval") * 0.001f;
				m_param->move_speed_big = xml.GetLong("move_speed");
				CTextParserUtility txtParser0, txtParser1;
				char key[][20] = {"topleft", "topright", "bottomleft", "bottomright"};
				for (int i = 0; i < 4; ++i) {
					txtParser0.Parse(xml.GetString(key[i]), ";");
					for (int j = 0; j < txtParser0.Count(); ++j) {
						FishType& fish_kind = m_param->big_fish[i][m_param->big_fish_kind_count[i]++];
						txtParser1.Parse(txtParser0[j], ",");
						fish_kind.type = atoi(txtParser1[0]);
						if (txtParser1.Count() > 1) {
							fish_kind.king = atoi(txtParser1[1]);
						}
					}
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
		float elapsed = 0.f;
		const int OFFSET = 200;
		int i;
		int x = 0, y = OFFSET, x2 = GAME_RESOLUTION_WIDTH - OFFSET, y2 = GAME_RESOLUTION_HEIGHT;
		float initAngle = MathUtility::getAngle(static_cast<float>(x2 - x), static_cast<float>(y2 - y)), angle = 0.0f;
		float length = GAME_RESOLUTION_WIDTH + 800;

		for (i = 0; i < 4; ++i) {
			switch(i) {
				case 0:
					angle = initAngle; x = 0; y = OFFSET;
					break;
				case 1:
					angle = -initAngle; x = GAME_RESOLUTION_WIDTH - OFFSET; y = 0;
					break;
				case 2:
					angle = initAngle - 180; x = GAME_RESOLUTION_WIDTH; y = GAME_RESOLUTION_HEIGHT - OFFSET;
					break;
				case 3:
					angle = 180 - initAngle; x = OFFSET; y = GAME_RESOLUTION_HEIGHT;
					break;
			}
			x -= static_cast<int>(MathUtility::getDxWithAngle(angle, 500));
			y -= static_cast<int>(MathUtility::getDyWithAngle(angle, 500));
			for (int j = 0; j != m_param->big_fish_kind_count[i]; ++j) {
				FishProduceData* fishProduceData = m_iFishProduce->create(m_param->big_fish[i][j]);
				if (fishProduceData) {
					fishProduceData->setShoalType(stSchool);
// 					setFirstAndLastFishID(fishProduceData->getFishKind().id);
					FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
					fishProduceData->setFishPathData(fishPathData);
					fishPathData->setInitPosition(x, y);
					fishPathData->setElapsed(-(m_param->big_fish_time_interval * j));
					fishPathData->addFishPathParam(FishPathDataFactory::createLine(angle, length, m_param->move_speed_big));
				}
			}
		}

		short dy = 100;
		FishPoint deltaPoint;
		FishType fishType = {m_param->small_fish_type, fktNormal};
		for (i = 0; i < m_param->small_fish_count; ++i) {
			FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
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
	FishArrayParam8* m_param;
};

#endif//__FISH_ARRAY_8_FOUR_CORNERS_H__
