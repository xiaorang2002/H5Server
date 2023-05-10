#ifndef __FISH_ARRAY_5_CIRCLE2_H__
#define __FISH_ARRAY_5_CIRCLE2_H__

#include <math.h>
#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"

class FishArray5Circle2 : public FishArrayBase {
public:
	FishArray5Circle2(FishArrayParam5& param) : m_param(&param) {

	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml)
        {
		if (xml.Sel("/FishArray/circle2")) {
			m_duration = xml.GetLong("duration");
			m_param->rot_speed = xml.GetLong("rot_speed");
			m_param->rot_angles = xml.GetLong("rot_angles");
			m_param->time_interval = xml.GetLong("interval") * 0.001f;
			if (xml.Sel("left")) {
				if (xml.CountSubet("item")) {
					while(xml.MoveNext()) {
						Circle2Param& para = m_param->param[0][m_param->param_count[0]++];
						para.count = xml.GetLong("count");
						para.radius = xml.GetLong("radius");
						para.fish_type = xml.GetLong("fish_type");
						para.move_speed = xml.GetLong("move_speed");
						para.order = xml.GetLong("order");
					}
				}
				xml.Sel("..");
			}
			if (xml.Sel("right")) {
				if (xml.CountSubet("item")) {
					while(xml.MoveNext()) {
						Circle2Param& para = m_param->param[1][m_param->param_count[1]++];
						para.count = xml.GetLong("count");
						para.radius = xml.GetLong("radius");
						para.fish_type = xml.GetLong("fish_type");
						para.move_speed = xml.GetLong("move_speed");
						para.order = xml.GetLong("order");
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
		float delay_avg = 0.0f, angle_avg, off_angle, start_angle = 90;
		FishPoint deltaPoint = {0, 0};
		short startX = GAME_RESOLUTION_WIDTH / 4, y = GAME_RESOLUTION_HEIGHT / 2;
		short divX = GAME_RESOLUTION_WIDTH / 2;
		short length = GAME_RESOLUTION_WIDTH + 500;
		const Circle2Param* para;

		for (int i = 0; i < 2; ++i) {
			if (m_param->param_count[i]) {
				for (int j = m_param->param_count[i] - 1; j >= 0; --j) {
					para = &m_param->param[i][j];
					if (para->count > 0) {
						angle_avg = 360.0f / para->count;
						delay_avg = fabs(angle_avg / m_param->rot_speed);
						if (para->count > 1) {
							off_angle = -45;
						} else {
							off_angle = 0;
						}
						float duration = fabs((float)m_param->rot_angles / m_param->rot_speed);
						deltaPoint.x = -para->radius;
						FishType fishType = {para->fish_type, fktNormal};
						for (int k = 0; k < para->count; ++k) {
							FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
							if (fishProduceData) {
								fishProduceData->setShoalType(stSchool);
// 								setFirstAndLastFishID(fishProduceData->getFishKind().id);
								FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
								fishProduceData->setFishPathData(fishPathData);
								float delays = delay_avg * k;
								fishPathData->setInitPosition(startX + divX * i + para->radius, y);
								fishPathData->setElapsed(-delays);
								float dur = duration - delays + m_param->time_interval * para->order;
								if (para->radius == 0) {
									fishPathData->addFishPathParam(FishPathDataFactory::createRotation(dur, start_angle, m_param->rot_speed));
									float angle = start_angle + dur * m_param->rot_speed + off_angle;
									fishPathData->addFishPathParam(FishPathDataFactory::createLine(angle, length, para->move_speed));
								} else {
									fishPathData->addFishPathParam(FishPathDataFactory::createCircle(dur, deltaPoint, m_param->rot_speed));
									float angle = start_angle + dur * m_param->rot_speed + 90 + off_angle;
									fishPathData->addFishPathParam(FishPathDataFactory::createLine(angle, length, para->move_speed));
								}
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
	FishArrayParam5* m_param;
};

#endif//__FISH_ARRAY_5_CIRCLE2_H__
