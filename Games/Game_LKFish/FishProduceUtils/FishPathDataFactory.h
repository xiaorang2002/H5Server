#pragma once

#include "CMD_Fish.h"
#include "FishProduceAllocator.h"
#include "MathUtil.h"
#include "Random.h"

class FishPathDataFactory
{
public:
	static FishPathParam* createLine(float duration, const FishPoint& deltaPoint)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptLine;
		pathData->duration = duration;
		pathData->data.pathLine.deltaPoint = deltaPoint;
		return pathData;
	}
	static FishPathParam* createLine(const FishPoint& from, const FishPoint& to, short additionalLength, unsigned short speed)
	{
		float dx = to.x - from.x, dy = to.y - from.y;
		float angle = MathUtility::getAngle(dx, dy);
		float length = MathUtility::getLength(dx, dy) + additionalLength;
		return createLine(angle, length, speed);
	}
	static FishPathParam* createLine(float angle, float length, unsigned short speed)
	{
		float duration = length / speed;
		FishPoint deltaPoint;
		deltaPoint.x = MathUtility::getDxWithAngle(angle, length);
		deltaPoint.y = MathUtility::getDyWithAngle(angle, length);
		return createLine(duration, deltaPoint);
	}
	static FishPathParam* createDelayTime(float duration)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptDelayTime;
		pathData->duration = duration;
		return pathData;
	}
	static FishPathParam* createDelayTimeWithHidden(float duration)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptDelayTimeWithHidden;
		pathData->duration = duration;
		return pathData;
	}
	static FishPathParam* createCircle(float duration, const FishPoint& deltaPoint, short rotateSpeed)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptCircle;
		pathData->duration = duration;
		pathData->data.pathCircle.deltaPoint = deltaPoint;
		pathData->data.pathCircle.rotateSpeed = rotateSpeed;
		return pathData;
	}
	static FishPathParam* createRotation(float duration, short initAngle, short rotateSpeed)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptRotation;
		pathData->duration = duration;
		pathData->data.pathRotation.initAngle = initAngle;
		pathData->data.pathRotation.rotateSpeed = rotateSpeed;
		return pathData;
	}
	static FishPathParam* createCurve(const FishPoint& initPosition, const FishSwimmingParam& swimmingParam, short radius)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		float angle = MathUtility::getAngle(GAME_RESOLUTION_WIDTH / 2 - initPosition.x, GAME_RESOLUTION_HEIGHT / 2 - initPosition.y);
		angle += Random::getInstance()->RandomInt(-10, 10);
		FishPoint currentPosition = initPosition;
		short length, totalLength = 0;
 		unsigned char pointCount = 0;
		char sign = 1;
		short speed = swimmingParam.speed;
		FishPoint* point = pathData->data.pathCurve.point;
		for (int i = 0; i < MAX_CURVE_POINTS; ++i) {
			length = Random::getInstance()->RandomInt(swimmingParam.minLength, swimmingParam.maxLength);
			point[i].x = MathUtility::getDxWithAngle(angle, length);
			point[i].y = MathUtility::getDyWithAngle(angle, length);
			currentPosition.x += point[i].x;
			currentPosition.y += point[i].y;
			if (!Random::getInstance()->RandomInt(0, 2)) {
				sign = -sign;
			}
			angle += sign * Random::getInstance()->RandomInt(swimmingParam.minRotation, swimmingParam.maxRotation);
			totalLength += length;
			if (currentPosition.x < -radius || currentPosition.x > GAME_RESOLUTION_WIDTH + radius ||
				currentPosition.y < -radius || currentPosition.y > GAME_RESOLUTION_HEIGHT + radius) {
				pointCount = i + 1;
				break;
			}
		}
		pathData->pathType = ptCurve;
		pathData->data.pathCurve.pointCount = pointCount;
		pathData->duration = (float)totalLength / speed;
		return pathData;
	}
	static FishPathParam* createEaseOut(float duration, const FishPoint& deltaPoint)
	{
		FishPathParam* pathData = FishProduceAllocator::getInstance()->allocFishPathParam();
		pathData->pathType = ptEaseOut;
		pathData->duration = duration;
		pathData->data.pathEaseOut.deltaPoint = deltaPoint;
		return pathData;
	}

public:
	static FishPoint getInitPosition(FishProduceSideConstant side, short radius)
	{
		FishPoint position = {0};
		switch(side)
		{
		case fpscLeft: // left
			position.x = -radius;
			position.y = Random::getInstance()->RandomInt(10, GAME_RESOLUTION_HEIGHT - 10);
			break;
		case fpscBottom: // bottom
			position.x = Random::getInstance()->RandomInt(10, GAME_RESOLUTION_WIDTH - 10);
			position.y = -radius;
			break;
		case fpscRight: // right
			position.x = GAME_RESOLUTION_WIDTH + radius;
			position.y = Random::getInstance()->RandomInt(10, GAME_RESOLUTION_HEIGHT - 10);
			break;
		case fpscTop: // top
			position.x = Random::getInstance()->RandomInt(10, GAME_RESOLUTION_WIDTH - 10);
			position.y = GAME_RESOLUTION_HEIGHT + radius;
			break;
		}
		return position;
	}
	static FishPoint getFishPointWithAngle(float duration, float angle, unsigned short speed)
	{
		FishPoint position = {0};
		unsigned short distance = duration * speed;
		position.x = MathUtility::getDxWithAngle(angle, distance);
		position.y = MathUtility::getDyWithAngle(angle, distance);
		return position;
	}
};
