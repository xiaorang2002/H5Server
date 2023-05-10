#include "FishProduceAllocator.h"
#include "FishProduceDataManager.h"
#include "FishBasicDataManager.h"
#include "NetworkDataUtility.h"
#include "Random.h"

/*
void log_fishes(const char* fmt,...)
{
    va_list va;
    char buf[1024]={0};
    char sztime[256]={0};
    // build the buffer.
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

    timeval tv;
    time_t tt;
    time(&tt);
    struct tm* st = localtime(&tt);
    gettimeofday(&tv,NULL);
    int milliSecond = int((tv.tv_sec*1000)+(tv.tv_usec/1000));
    snprintf(sztime,sizeof(sztime),"%02d/%02d %02d:%02d:%02d : ",
             st->tm_mon+1,st->tm_mday,st->tm_hour,
             st->tm_min,  st->tm_sec,milliSecond);
    // try to build the full buffer.
    std::string strValue = sztime;
    strValue += buf;
    strValue += "\n";
    // try to open the special fish item now.
    FILE* fp = fopen("alive_fishes.log","a");
    fputs(strValue.c_str(),fp);
    fclose(fp);
} */

FishProduceDataManager::FishProduceDataManager()
    : m_currentFishID(MIN_FISH_ID)
    , m_pause(false)
	, m_bProduceYWDJ(true)
	, m_nHaveYWDJCount(0)
	, m_bProduceSpecialFire(true)
{
#ifdef __MULTI_THREAD__
    CCriticalSectionEx::Create(&m_cs);
#endif
}

FishProduceDataManager::~FishProduceDataManager()
{
#ifdef __MULTI_THREAD__
    CCriticalSectionEx::Destroy(&m_cs);
    // reset();
#endif
}

FISH_ID FishProduceDataManager::moveToNextFishID() {
	if (m_currentFishID < MIN_FISH_ID) {
		m_currentFishID = MIN_FISH_ID;
    }   else if (++m_currentFishID > MAX_FISH_ID)
    {
		m_currentFishID = MIN_FISH_ID;
	}

	return m_currentFishID;
}


FishProduceDataMap::iterator FishProduceDataManager::removeWithIter(FishProduceDataMap::iterator& iter)
{
	FishType fishType = iter->second->getFishKind().fishType;
	unsigned short key = getKeyWithFishType(fishType);
	FishProduceCountWithFishTypeMap::iterator iterCount = m_aliveFishCountWithFishType.find(key);
	if (iterCount != m_aliveFishCountWithFishType.end())
    {
		--iterCount->second;
	}
    else
    {
		m_aliveFishCountWithFishType.insert(FishProduceCountWithFishTypeMap::value_type(key, 1));
	}
	if (fishType.king == fktYWDJ)
	{
		if (m_nHaveYWDJCount > 0)
		{
			m_nHaveYWDJCount--;
			if (m_nHaveYWDJCount == 0)
				m_bProduceYWDJ = true;
		}
	}
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

//Cleanup:		
    FishProduceAllocator::getInstance()->freeFishProduceData(iter->second);
    FishProduceDataMap::iterator iterret = m_aliveFishes.erase(iter);
	return iterret;
}

FishProduceData* FishProduceDataManager::create(const FishType& fishType)
{
    // can't create the fish alive now.
	if (isAlive(m_currentFishID)) {
		return NULL;
	}
	if (!m_bProduceYWDJ && fishType.king == fktYWDJ)
		return NULL;
	if (!m_bProduceSpecialFire  && (fishType.type == 54 || fishType.type == 55))
	{
		LOG(WARNING) << "FishProduceDataManager::create, m_bProduceSpecialFire == false,fishType.type = " << (int32_t)fishType.type;
		return NULL;
	}
	const FishBasicData* fishBasicData = FishBasicDataManager::getInstance()->getFishBasicDataWithFishType(fishType);
	if (fishBasicData) {
		FishProduceData* fishProduceData = FishProduceAllocator::getInstance()->allocFishProduceData();
        if (fishProduceData) {

        // add by James for test only.
        // Random::getInstance()->RandomSeed(1);
        // add by James end.

        fishProduceData->setRandomSeed(Random::getInstance()->GetCurrentSeed());
        //LOG(WARNING)<<"fasong----------------------seed=="<<Random::getInstance()->GetCurrentSeed();
        FishKind fishKind;
        fishKind.fishType = fishType;
        fishKind.id = m_currentFishID;
        moveToNextFishID();
        fishKind.life = Random::getInstance()->RandomInt(fishBasicData->lifeMin, fishBasicData->lifeMax);
        //LOG(WARNING)<<"get fishKind.life from  fishBasicData->lifeMin, fishBasicData->lifeMax --->randomint()";
#ifdef __USE_LIFE2__
        fishKind.life2 = fishKind.life * (100 - Random::getInstance()->RandomInt(fishBasicData->life2Min, fishBasicData->life2Max)) / 100;
#endif
            fishProduceData->setFishKind(fishKind);
            setProduceData(fishProduceData);
			if (fishType.king == fktYWDJ)
			{
				m_nHaveYWDJCount++;
				if (m_nHaveYWDJCount >= 5)
					m_bProduceYWDJ = false;
			}
            // initialize value.
            char buf[1024]={0};
            snprintf(buf,sizeof(buf),"FishProduceDataManager::create fish_id=%d fish_type=(%d,%d) m_nHaveYWDJCount=%d randomseed=%u",
                    fishKind.id, fishType.type, fishType.king, m_nHaveYWDJCount, fishProduceData->getRandomSeed());
            LOG(INFO) << buf;

        }
		
        return fishProduceData;
	}

	return NULL;
}

void FishProduceDataManager::pause(unsigned int endTick)
{
    if (m_pause) return;

#ifdef __MULTI_THREAD__
    CCriticalSectionEx autoLock;
    autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

    //TRACE_INFO(_T("FishProduceDataManager::pause begin"));
    //debugFishID();

    FishProduceData* p;
    FishProduceDataMap::iterator iter = m_aliveFishes.begin();
    while(iter != m_aliveFishes.end()) {
        p = iter->second;
        p->getFishPathData()->endTickAt(endTick);
        if (!p->isAlive()) {
            iter = removeWithIter(iter);
            continue;
        }
        ++iter;
    }

    //debugFishID();
    //TRACE_INFO(_T("FishProduceDataManager::pause end"));
    m_pause = true;
}

void FishProduceDataManager::resume(unsigned int startTick)
{
    if (!m_pause) return;

#ifdef __MULTI_THREAD__
    CCriticalSectionEx autoLock;
    autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

    //TRACE_INFO(_T("FishProduceDataManager::resume begin"));
    //debugFishID();

    FishProduceData* p;
    FishProduceDataMap::iterator iter = m_aliveFishes.begin();
    while(iter != m_aliveFishes.end()) {
        p = iter->second;
        p->getFishPathData()->startTickAt(startTick);
        ++iter;
    }

    //debugFishID();
    //TRACE_INFO(_T("FishProduceDataManager::pause end"));
    m_pause = false;
}

void FishProduceDataManager::update(float delta)
{
    if (m_pause) return;

#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	//TRACE_INFO(_T("FishProduceDataManager::update begin"));
	//debugFishID();
	FishProduceDataMap::iterator iter = m_aliveFishes.begin();
	FishProduceData* p;
	while (iter != m_aliveFishes.end()) {
		p = iter->second;
		p->update(delta);
		if (!p->isAlive()) {
			iter = removeWithIter(iter);
			continue;
		}
		++iter;
	}

	//debugFishID();
	//TRACE_INFO(_T("FishProduceDataManager::update end"));
}

bool FishProduceDataManager::setProduceData(FishProduceData* fishProduceData)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__
    // try go get the special content item data now.
	pair<FishProduceDataMap::iterator, bool> _result;
	FishProduceDataMap::value_type vt(fishProduceData->getFishKind().id, fishProduceData);
	//TRACE_INFO(_T("FishProduceDataManager::set begin"));
    // try to insert the data item now.
    _result = m_aliveFishes.insert(vt);
	if (!_result.second)
    {
		FishProduceAllocator::getInstance()->freeFishProduceData(fishProduceData);
		return false;
	}

    // try to insert the sending.
	m_fishesForSending.insert(vt);
	unsigned short key = getKeyWithFishType(fishProduceData->getFishKind().fishType);
	FishProduceCountWithFishTypeMap::iterator iter = m_aliveFishCountWithFishType.find(key);
	if (iter != m_aliveFishCountWithFishType.end())
    {
		++iter->second;
	}
    else
    {
		m_aliveFishCountWithFishType.insert(FishProduceCountWithFishTypeMap::value_type(key, 1));
	}

	//debugFishID();
	//TRACE_INFO(_T("FishProduceDataManager::set end"));
	return true;
}

bool FishProduceDataManager::remove(FISH_ID fishID)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	FishProduceDataMap::iterator iter = m_aliveFishes.find(fishID);
	if (iter != m_aliveFishes.end()) {
		removeWithIter(iter);
		//TRACE_INFO(_T("FishProduceDataManager::remove"));
		//debugFishID();
		return true;
	}
	return false;
}

FishProduceData* FishProduceDataManager::get(FISH_ID fishID)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	FishProduceDataMap::iterator iter = m_aliveFishes.find(fishID);
	if (iter != m_aliveFishes.end()) {
		return iter->second;
	}

	return NULL;
}

bool FishProduceDataManager::isAlive(FISH_ID fishID) {
    bool bExist = false;
    if (getFishKindWithFishID(fishID)) {
        bExist = true;
    }
//Cleanup:
    return  bExist;
}

const FishKind* FishProduceDataManager::getFishKindWithFishID(FISH_ID fishID)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	FishProduceDataMap::const_iterator iter = m_aliveFishes.find(fishID);
	if (iter != m_aliveFishes.end()) {
		return &iter->second->getFishKind();
	}

    return NULL;
}

unsigned short FishProduceDataManager::popFishesForSending(unsigned char* buffer, unsigned short bufferSize, unsigned int startTick)
{
	unsigned short dataSize = 0;
	if (buffer && m_fishesForSending.size()) {
		dataSize = NetworkUtility::encodeProduceFishes(buffer, bufferSize, m_fishesForSending);
	}

#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

    std::string strValue = "";
    FishProduceDataMap::iterator iter;
    for(iter = m_fishesForSending.begin();iter!=m_fishesForSending.end();iter++)
    {
        // check the special map item data value now.
        FishProduceData* procudeData = iter->second;
        if (procudeData != NULL)
        {

            //LOG(WARNING)<<"---fish live ------"<<procudeData->getFishPathData()->getDuration();
            FishPathData* pathData = procudeData->getFishPathData();
            if (pathData != NULL) {
                pathData->startTickAt(startTick);
            }
        }
	}

    /*
    // check if validate data.
    if (strValue.length()) {
        log_fishes("popFishesForSending produce fish id list:%s",strValue.c_str());
    }
    */

	m_fishesForSending.clear();
	return dataSize;
}


void FishProduceDataManager::updateWithEndTick(unsigned int endTick)
{
    if (m_pause) return;

#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	//TRACE_INFO(_T("FishProduceDataManager::updateWithEndTick begin"));
	//debugFishID();
	FishProduceDataMap::iterator iter = m_aliveFishes.begin();
	FishProduceData* p;
    while (iter != m_aliveFishes.end())
    {
		p = iter->second;
        FishPathData* fishPathData = p->getFishPathData();
        if (fishPathData != NULL)
        {
            // p->getFishPathData()->endTickAt(endTick);
            fishPathData->endTickAt(endTick);
        }

        // check is alive.
        if (!p->isAlive())
        {
			iter = removeWithIter(iter);
			continue;
        }
        else
        {
            // check fish path.
            if (fishPathData)
            {
                // p->getFishPathData()->startTickAt(endTick);
                fishPathData->startTickAt(endTick);
            }
		}
		++iter;
	}

	//debugFishID();
	//TRACE_INFO(_T("FishProduceDataManager::updateWithEndTick end"));
}

bool FishProduceDataManager::contains(ShoalType shoalType)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

    FishProduceData* p;
    FishProduceDataMap::iterator iter = m_aliveFishes.begin();
	while (iter != m_aliveFishes.end())
    {
		p = iter->second;
		if (shoalType == p->getShoalType())
        {
			return true;
		}

		++iter;
	}

	return false;
}


void FishProduceDataManager::removeAllExcludeFromDictionary(const FishIDDictionary& dict)
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

    FishProduceData* p;
    FishProduceDataMap::iterator iter = m_aliveFishes.begin();
	//TRACE_INFO(_T("FishProduceDataManager::removeAllExcludeFromDictionary begin"));
	//debugFishID();
	while (iter != m_aliveFishes.end()) {
		p = iter->second;
		if (!dict.exist(p->getFishKind().id)) {
			iter = removeWithIter(iter);
			continue;
		}
		++iter;
	}
	//debugFishID();
	//TRACE_INFO(_T("FishProduceDataManager::removeAllExcludeFromDictionary end"));
}

void FishProduceDataManager::removeAll()
{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif __MULTI_THREAD__

	FishProduceDataMap::iterator iter = m_aliveFishes.begin();
    while (iter != m_aliveFishes.end())
    {
		FishType fishType = iter->second->getFishKind().fishType;
		if (fishType.king == fktYWDJ)
		{
			if (m_nHaveYWDJCount > 0)
			{
				m_nHaveYWDJCount--;
				if (m_nHaveYWDJCount == 0)
					m_bProduceYWDJ = true;
			}
		}
		FishProduceAllocator::getInstance()->freeFishProduceData(iter->second);
		++iter;
	}

    // log_fishes(" <<<<<<< remove all alive fish.");

	m_aliveFishes.clear();
	m_fishesForSending.clear();
	m_aliveFishCountWithFishType.clear();
}

void FishProduceDataManager::reset() {
	m_pause = false;
	removeAll();
}
