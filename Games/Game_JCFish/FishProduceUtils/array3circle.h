#ifndef __FISH_ARRAY_3_CIRCLE_H__
#define __FISH_ARRAY_3_CIRCLE_H__

#include "FishArrayBase.h"
#include "FishProduceAllocator.h"
#include "array4cirboom.h"

class FishArray3Circle : public FishArrayBase {
public:
	FishArray3Circle(FishArrayParam3& param) : m_param(&param) {
		
	}

public:
        virtual bool initWithXmlFile(CXmlParser& xml)
        {
                if (xml.Sel("/FishArray/circle"))
                {
			m_duration = xml.GetLong("duration");
			m_param->rot_speed = xml.GetLong("rot_speed");
			m_param->rot_angles = xml.GetLong("rot_angles");
			m_param->time_interval = xml.GetLong("interval") * 0.001f;
                        if (xml.CountSubet("item"))
                        {
                                while(xml.MoveNext())
                                {
					CirboomParam& para = m_param->param[m_param->param_count++];
					para.count = xml.GetLong("count");
					para.radius = xml.GetLong("radius");
					para.fish_type = xml.GetLong("fish_type");
					para.move_speed = xml.GetLong("move_speed");
				}
			}
		}
        //Cleanup:
		return m_duration > 0;
	}

        virtual bool createFishArray()
        {
            // try to create the special fish array now.
            if (!FishArrayBase::createFishArray())
            {
                    return false;
            }

            bool ret = true;
            float delay_avg = 0.0f, angle_avg, start_angle = 90;
            float off_angle;
            FishPoint pointCenter = { GAME_RESOLUTION_WIDTH / 2, GAME_RESOLUTION_HEIGHT / 2 };
            FishPoint deltaPoint = {0, 0};
            short length = GAME_RESOLUTION_WIDTH + 500;
            const CirboomParam* para;

            if (m_param->param_count)
            {
                    for (int j = m_param->param_count - 1; j >= 0; --j)
                    {
                            para = &m_param->param[j];
                            if (para->count > 0)
                            {
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
                                    for (int k = 0; k < para->count; ++k)
                                    {
                                            FishProduceData* fishProduceData = m_iFishProduce->create(fishType);
                                            if (fishProduceData)
                                            {
                                                    fishProduceData->setShoalType(stSchool);
// 							setFirstAndLastFishID(fishProduceData->getFishKind().id);
                                                    FishPathData* fishPathData = FishProduceAllocator::getInstance()->allocFishPathData();
                                                    fishProduceData->setFishPathData(fishPathData);
                                                    float delays = delay_avg * k;
                                                    fishPathData->setInitPosition(pointCenter.x + para->radius, pointCenter.y);
                                                    fishPathData->setElapsed(-delays);
                                                    float dur = duration - delays + m_param->time_interval * j;
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

//          calcFishCount();
            return ret;
	}

protected:
	FishArrayParam3* m_param;
};

#endif//__FISH_ARRAY_3_CIRCLE_H__
