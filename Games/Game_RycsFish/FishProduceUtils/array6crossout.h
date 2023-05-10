#ifndef __FISH_ARRAY_6_CROSS_OUT_H__
#define __FISH_ARRAY_6_CROSS_OUT_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"

class FishArray6Crossout : public FishArrayBase {
public:
	FishArray6Crossout(FishArrayParam6& param) : m_param(&param) {

	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml) {
		if (xml.Sel("/FishArray/crossout")) {
			m_duration = xml.GetLong("duration");
			m_param->move_speed = xml.GetLong("move_speed");
			if (xml.Sel("left")) {
				if (xml.CountSubet("item")) {
					while(xml.MoveNext()) {
						CrossoutParam& para = m_param->param[0][m_param->param_count[0]++];
						para.count = xml.GetLong("count");
						para.radius = xml.GetLong("radius");
						para.fish_type = xml.GetLong("fish_type");
						if (para.radius > m_param->max_radius[0]) {
							m_param->max_radius[0] = para.radius;
						}
					}
				}
				xml.Sel("..");
			}
			if (xml.Sel("right")) {
				if (xml.CountSubet("item")) {
					while(xml.MoveNext()) {
						CrossoutParam& para = m_param->param[1][m_param->param_count[1]++];
						para.count = xml.GetLong("count");
						para.radius = xml.GetLong("radius");
						para.fish_type = xml.GetLong("fish_type");
						if (para.radius > m_param->max_radius[1]) {
							m_param->max_radius[1] = para.radius;
						}
					}
				}
				xml.Sel("..");
			}
		}
		return m_duration > 0;
	}

        virtual bool createFishArray() {
		if (!FishArrayBase::createFishArray()) {
			return false;
		}
		bool ret = true;
		const short OFFSET = 100;
		float radius_y, angle_avg;
		FishPoint centerPoint = {0, GAME_RESOLUTION_HEIGHT / 2};
		FishPoint deltaPoint = {0, 0};
		CrossoutParam* para;

		for (int i = 0; i < 2; ++i) {
			float divX = m_param->max_radius[i] + OFFSET;
			float dx = GAME_RESOLUTION_WIDTH + divX * 2;
			float duration = dx / m_param->move_speed;
			if (i % 2) {
				centerPoint.x = GAME_RESOLUTION_WIDTH + divX;
				deltaPoint.x = -dx;
			} else {
				centerPoint.x = -divX;
				deltaPoint.x = dx;
			}
			if (m_param->param_count[i]) {
				for (int j = 0; j != m_param->param_count[i]; ++j) {
					para = &m_param->param[i][j];
					radius_y = para->radius * 100 / 95.0f;
					if (para->count > 0) {
						angle_avg = M_2PI / para->count;
						FishType fishType = {para->fish_type, fktNormal};
						for (int k = 0; k < para->count; ++k) {
							FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
							if (fishProduceData) {
								fishProduceData->setShoalType(stSchool);
// 								setFirstAndLastFishID(fishProduceData->getFishKind().id);
								FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
								fishProduceData->setFishPathData(fishPathData);
								float theta = k * angle_avg;
								fishPathData->setInitPosition(centerPoint.x + sinf(theta) * para->radius, centerPoint.y + cosf(theta) * radius_y);
								fishPathData->addFishPathParam(FishPathDataFactory::createLine(duration, deltaPoint));
							}
						}
					}
				}
			}
		}
// 		calcFishCount();

		return ret;
	}

protected:
	FishArrayParam6* m_param;
};

#endif//__FISH_ARRAY_6_CROSS_OUT_H__
