#pragma once
#ifdef __GAMEPLAY_COLLECT__
#define IsCollectFish(collectIndex) ((collectIndex) >= 0)
#define IsCollectComplete(collectStats, collectCount) ((collectStats) == ((1 << (collectCount)) - 1))
#define GetCollectStats(collectStats, collectIndex) ((collectStats >> collectIndex) & 0x1)
#define SetCollectStats(collectStats, collectIndex) collectStats |= 1 << (collectIndex)
#define ResetCollectStats(collectStats, collectIndex) collectStats &= ~(1 << (collectIndex))

class CCollectUtility
{
public:
	CCollectUtility() : m_collectCount(0), m_isCollectOnlyOne(false)
	{
		memset(&m_collectData, 0, sizeof(m_collectData));
		for (BYTE i = 0; i < MAX_FISH_SPECIES; ++i) {
			m_collectIndexList[i] = -1;
		}
	}

public:
	void initialize(const CollectData& collectData)
	{
		BYTE fishId;
		m_isCollectOnlyOne = false;
		m_collectCount = collectData.collectCount;
		for (BYTE i = 0; i < COLLECT_COUNT; ++i) {
			fishId = collectData.collectFishId[i];
			if (fishId) {
				m_collectIndexList[fishId] = i;
			} else {
				m_isCollectOnlyOne = 1 == i;
				break;
			}
		}
	}
	short getCollectIndex(LONG fishType, LONG fishKing, BYTE currentCollectStats, short currentCollectIndex)
	{
		short collectIndex = -1;
		if (fishKing == fktNormal) {
			collectIndex = m_collectIndexList[fishType];
			if (IsCollectFish(collectIndex) && m_isCollectOnlyOne) {
				if (IsCollectFish(currentCollectIndex)) {
					collectIndex = currentCollectIndex + 1;
				} else {
					collectIndex = getCollectFishCount(currentCollectStats);
				}
				if (collectIndex >= m_collectCount) {
					collectIndex = m_collectCount - 1;
				}
			}
		}
		return collectIndex;
	}
	BYTE getCollectFishCount(BYTE collectStats)
	{
		BYTE fishCount = 0;
		for (int i = 0; i < m_collectCount; ++i) {
			if ((collectStats >> i) & 0x1) {
				++fishCount;
			}
		}
		return fishCount;
	}

private:
	BYTE m_collectCount;
	bool m_isCollectOnlyOne;
	CollectData m_collectData;
	short m_collectIndexList[MAX_FISH_SPECIES];
};
#endif
