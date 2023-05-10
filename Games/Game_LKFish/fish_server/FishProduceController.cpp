
#include "StdAfx.h"
#include "FishProduceController.h"
#include "../FishProduceUtils/Random.h"

FishProduceController::FishProduceController() : m_durationMin(0), m_durationMax(0), m_duration(0)
, m_time(0)/*, m_fishCountMin(0), m_fishCountMax(0), m_fishProduceCount(0), m_averageDuration(0)*/
, m_previousIndex(-1), m_allowSame(FALSE), m_iFishProduce(NULL), m_intervalMin(0), m_status(eapsStart)
, m_allowSchedule(FALSE)
{

}
FishProduceController::~FishProduceController()
{

}
void FishProduceController::initialize(IFishProduce* iFishProduce) {
	m_iFishProduce = iFishProduce;
	m_autoProduceFish.setIFishProduce(iFishProduce);
}
// void FishProduceController::setFishCount(unsigned short min, unsigned short max)
// {
// 	m_fishCountMin = min;
// 	m_fishCountMax = max;
// }
void FishProduceController::setDuration(unsigned int min, unsigned int max)
{
	m_durationMin = min;
	m_durationMax = max;
}
void FishProduceController::addFishType(const FishType& fishType)
{
	m_fishTypesOrg.push_back(fishType);
}
void FishProduceController::updateTime(unsigned int deltaTime, bool produce)
{
	switch (m_status)
	{
	case eapsStart:
		{
			bool bAddTime = true;
			FishType fishType = m_fishTypesOrg.at(0);
			if (fishType.king == fktYWDJ)
			{
				bool bProduceYWDJ = m_iFishProduce->getbProduceYWDJ();
				if (bProduceYWDJ)
				{
					m_time += deltaTime;
				}
				bAddTime = false;
                //LOG(WARNING) << "===== AAA  FishProduceController::updateTime-->m_autoProduceFish.create fishType:" << (int32_t)fishType.type << " king: " << (int32_t)fishType.king << " m_time: " << (int32_t)m_time;
			}
			if (fishType.type == 54 || fishType.type == 55)
			{
				bool bProduce = m_iFishProduce->getbProduceSpecialFire();
				if (bProduce)
				{
					m_time += deltaTime;
				}
				bAddTime = false;
				//LOG(WARNING) << "===== AAA  FishProduceController::updateTime-->m_autoProduceFish.create fishType:" << (int32_t)fishType.type << " king: " << (int32_t)fishType.king << " m_time: " << (int32_t)m_time;
			}
			if (bAddTime)
			{
				m_time += deltaTime;
			}
			//DWORD averageDuration = m_fishProduceCount < 2 ? 0 : m_duration / m_fishProduceCount;
			if (produce && m_time >= m_duration && m_fishTypesOrg.size())
			{
				unsigned short count = 0;
				int index, maxIndex;
				if (0 == m_fishTypes.size())
				{
					m_fishTypes = m_fishTypesOrg;
				}
				maxIndex = m_fishTypes.size() - 1;
				index = Random::getInstance()->RandomInt(0, maxIndex);
				//LOG(WARNING)<<"maxIndex = m_fishTypes.size() - 1;  --->randomint()";
				FishType fishType = m_fishTypes.at(index);
				if (m_allowSame) {
					count = m_autoProduceFish.create(fishType, 0/*averageDuration * m_fishProduceCount*/);
				}
				else {
					if (index == m_previousIndex && m_fishTypes.size() == m_fishTypesOrg.size() && maxIndex > 0) {
						if (index < maxIndex) {
							++index;
						}
						else {
							--index;
						}
					}
					count = m_autoProduceFish.create(fishType, 0/*averageDuration * m_fishProduceCount*/);
					if (count) {
						m_fishTypes.at(index) = m_fishTypes.back();
						m_fishTypes.pop_back();
						m_previousIndex = index;
					}
				}
				if (!count) {
					TRACE_INFO(_T("FishProduceController::updateTime-->m_autoProduceFish.create failed. fish_id=%d"), m_iFishProduce->getCurrentFishID());
					break;
				}
				else {
					if (m_allowSchedule) {
						m_status = eapsWork;
					}
					LOG(WARNING) << "===== updateTime createFish type:" << (int32_t)fishType.type << " king: " << (int32_t)fishType.king << " Onlytype: " << (int32_t)fishType.Onlytype;
				}
				//m_fishProduceCount -= count;
				m_time = 0;
				makeDuration();
			}
			break;
		}
 		//case eapsSchedule:
 		//	m_time += deltaTime;
 		//	if (m_time >= m_scheduleInterval) {
 		//		m_status = eapsStart;
 		//		m_time = 0;
 		//	}
 		//	break;
	case eapsWork:
		{
			refreshStatus();
			break;
		}
	}
}
void FishProduceController::reset()
{
	m_time = 0;
	makeDuration();
	m_status = eapsStart;
 	//m_fishProduceCount = Random::getInstance()->RandomInt(m_fishCountMin, m_fishCountMax);
 	//m_averageDuration = m_fishProduceCount < 2 ? 0 : m_duration / m_fishProduceCount;
}
void FishProduceController::refreshStatus() {
	if (!m_allowSchedule) {
		m_status = eapsStart;
		return;
	}
	if (eapsWork == m_status) {
		unsigned short fishCount = 0;
		for (unsigned short i = 0; i < m_fishTypesOrg.size(); ++i) {
			if (fishCount = m_iFishProduce->getAliveFishCountWithFishType(m_fishTypesOrg.at(i))) {
				break;
			}
		}
		if (!fishCount) {
			m_status = eapsStart;
			m_time = 0;
		}
	}
}
void FishProduceController::makeDuration() {
	if (m_allowSchedule) {
        //LOG(WARNING)<<"makeDuration()   --->randomint()";
		m_duration = Random::getInstance()->RandomInt(m_intervalMin, m_intervalMax);
	} else {
        //LOG(WARNING)<<"makeDuration()   --->randomint()";
		m_duration = Random::getInstance()->RandomInt(m_durationMin, m_durationMax);
	}
}
