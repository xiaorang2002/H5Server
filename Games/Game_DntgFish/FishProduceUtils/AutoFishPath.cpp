#include "AutoFishPath.h"
#include "Random.h"
#include "MathUtil.h"
#include "FishProduceAllocator.h"
#include "FishPathDataFactory.h"
#include <glog/logging.h>
extern void log_fishes(const char* fmt,...);

InitilaizeSignletonInstance(AutoFishPath, m_instance);

FishPathData* AutoFishPath::createFishPath(const FishBasicData* fishBasicData, const FishSwimmingPattern* pattern)
{
	if (!fishBasicData || !pattern || !pattern->paramDataCount) {
		return NULL;
	}

	FishPathData* p = FishProduceAllocator::getInstance()->allocFishPathData();
	if (p) {
		const FishSwimmingParam& swimmingParam = pattern->swimmingParam[Random::getInstance()->RandomInt(0, pattern->paramDataCount - 1)];
		FishProduceSideConstant side = static_cast<FishProduceSideConstant>(Random::getInstance()->RandomInt(0, fpscMax - 1));
		unsigned short radius = fishBasicData->boundingHeight >> 1;
		short speed = swimmingParam.speed;
		FishPoint initPosition = FishPathDataFactory::getInitPosition(side, radius);
		p->setSpeed(speed);
		p->setInitPosition(initPosition.x, initPosition.y);
		switch(swimmingParam.pathType)
		{
		case ptLine:
			{
				FishPoint to = {0};
				switch(side)
				{
				case fpscLeft: // left
				case fpscRight: // right
					if (fpscLeft == side) {
						to.x = GAME_RESOLUTION_WIDTH;
					} else {
						to.x = 0;
					}
                    if (initPosition.y < radius)
                    {
						to.y = Random::getInstance()->RandomInt(radius, GAME_RESOLUTION_HEIGHT);
                    } else if (GAME_RESOLUTION_HEIGHT - initPosition.y < radius)
                    {
						to.y = Random::getInstance()->RandomInt(0, GAME_RESOLUTION_HEIGHT - radius);
                    } else
                    {
						to.y = Random::getInstance()->RandomInt(0, GAME_RESOLUTION_HEIGHT);
					}
                    //LOG(WARNING)<<"swimmingParam.pathType fpscRight --->randomint()";
					break;
				case fpscTop: // top
				case fpscBottom: // bottom
					if (fpscTop == side) {
						to.y = 0;
					} else {
						to.y = GAME_RESOLUTION_HEIGHT;
					}
					if (initPosition.x < radius) {
						to.x = Random::getInstance()->RandomInt(radius, GAME_RESOLUTION_WIDTH);
					} else if (GAME_RESOLUTION_WIDTH - initPosition.x < radius) {
						to.x = Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH - radius);
					} else {
						to.x = Random::getInstance()->RandomInt(0, GAME_RESOLUTION_WIDTH);
					}
					break;
				}

				p->addFishPathParam(FishPathDataFactory::createLine(initPosition, to, radius, speed));

                // log_fishes("createFishPath ptLine, from:(%d,%d) to:(%d,%d)",initPosition.x,initPosition.y,to.x,to.y);
			}
			break;
		case ptEaseOut:
			{
				float angle = MathUtility::getAngle(GAME_RESOLUTION_WIDTH / 2 - initPosition.x, GAME_RESOLUTION_HEIGHT / 2 - initPosition.y);
				angle += Random::getInstance()->RandomInt(-10, 10);
				FishPoint deltaPoint, currentPosition = initPosition;
                short length = Random::getInstance()->RandomInt(swimmingParam.minLength, swimmingParam.maxLength);
				float duartion = (float)length / speed;
				do {
					deltaPoint.x = MathUtility::getDxWithAngle(angle, length);
					deltaPoint.y = MathUtility::getDyWithAngle(angle, length);
					p->addFishPathParam(FishPathDataFactory::createEaseOut(duartion, deltaPoint));
					currentPosition.x += deltaPoint.x;
					currentPosition.y += deltaPoint.y;
					angle += Random::getInstance()->RandomInt(swimmingParam.minRotation, swimmingParam.maxRotation);
				} while(currentPosition.x >= -radius && currentPosition.x <= GAME_RESOLUTION_WIDTH + radius && currentPosition.y >= -radius && currentPosition.y <= GAME_RESOLUTION_HEIGHT + radius);

                // log_fishes("createFishPath ptEaseOut, from:(%d,%d) to:(%d,%d)",initPosition.x,initPosition.y,currentPosition.x,currentPosition.y);

            }
			break;
		case ptCurve:
			{
                FishPathParam* param = FishPathDataFactory::createCurve(initPosition, swimmingParam, radius);
                p->addFishPathParam(param);

                std::string strValue = "";
                PathCurveInfo& info = param->data.pathCurve;
                for (int i=0;i<info.pointCount;i++) {
                    FishPoint& point = info.point[i];

                    char buf[1024]={0};
                    snprintf(buf,sizeof(buf),"\n\tidx:[%d],point:[%d,%d]\n",i,point.x,point.y);
                    strValue += buf;
                }

                // log_fishes("createFishPath ptCurve from:(%d,%d), point list: (%s)",initPosition.x,initPosition.y,strValue.c_str());

            }
			break;
		default:
			FishProduceAllocator::getInstance()->freeFishPathData(p);
			return NULL;
		}
		return p;
	}
	return NULL;
}

// modify by James 180831-create fish shoal.
FishPathData* AutoFishPath::createFishPathShoal(FishPathData* fishPathLeader, ShoalType shoalType, unsigned short index, const FishBasicData* fishBasicData) {
	if (shoalType == stLump) {
		short areaWidth = (fishBasicData->boundingWidth * 4) >> 1;
		FishPathParam* pathParam = fishPathLeader->getFishPathParam(0);
		if (pathParam) {
			if (pathParam->pathType == ptCurve || pathParam->pathType == ptLine || pathParam->pathType == ptCircle || pathParam->pathType == ptEaseOut) {
				FishPathData* p = FishProduceAllocator::getInstance()->allocFishPathData();
				*p = *fishPathLeader;
				FishPoint& point = p->getFishPathParam(0)->data.pathCurve.point[0];
				FishPoint deltaPointAdd = {0};
				deltaPointAdd.x = Random::getInstance()->RandomInt(-areaWidth, areaWidth);
				FishPoint offset = makeOffsetPoint(point, deltaPointAdd);
				point.x += offset.x;
				point.y += offset.y;
				float delayTime = Random::getInstance()->RandomFloat(0.1f, 1.65f);
				p->insertPathData(0, FishPathDataFactory::createDelayTime(delayTime));
				return p;
			}
		}
	} else if (shoalType == stFollow) {
//        float delayTime = (float)(fishBasicData->boundingHeight * 1.5f) / fishPathLeader->getSpeed();   // modify by James to add the delay time.
        float delayTime = (float)(fishBasicData->boundingHeight * 1.5f) / fishPathLeader->getSpeed();   // modify by James to add the delay time.
        FishPathData* p = FishProduceAllocator::getInstance()->allocFishPathData();
		*p = *fishPathLeader;
		p->setElapsed(p->getElapsed() - delayTime * (index + 1));
		return p;
	}
	return NULL;
}

ShoalType AutoFishPath::getShoalType(unsigned short& shoalCount, const FishShoalInfo* shoalInfo) {
	if (shoalInfo && (shoalInfo->allowFollow || shoalInfo->allowLump)) {
		int randNumber = Random::getInstance()->RandomInt(0, 99);
        if (randNumber < shoalInfo->genRatio)
        {
			shoalCount = Random::getInstance()->RandomInt(shoalInfo->amountMin, shoalInfo->amountMax);

            //LOG(WARNING)<<"shoalcount= "<<shoalCount   <<"randomseed= "<<Random::getInstance()->GetCurrentSeed();
			char follow = shoalInfo->allowFollow, lump = shoalInfo->allowLump;
            if (follow && lump)
            {
                if (randNumber % 2)
                {
					lump = 0;
                } else
                {
					follow = 0;
				}
			}
            if (lump)
            {
				return stLump;
            } else
            {
				return stFollow;
			}
		}
	}
	shoalCount = 0;
	return stSingle;
}

FishPoint AutoFishPath::makeOffsetPoint(const FishPoint& deltaPointOld, const FishPoint& deltaPointAdd) {
	FishPoint offsetPoint = {0};
	float angle = MathUtility::getAngle(deltaPointOld.x, deltaPointOld.y);
	float newAngle = 180 - (MathUtility::getAngle(deltaPointAdd.x, deltaPointAdd.y) - angle);
	float length = MathUtility::getLength(deltaPointAdd.x, deltaPointAdd.y);
	offsetPoint.x = MathUtility::getDxWithAngle(newAngle, length);
	offsetPoint.y = MathUtility::getDyWithAngle(newAngle, length);
	return offsetPoint;
}
