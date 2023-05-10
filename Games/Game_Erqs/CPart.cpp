#include "CPart.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include "MjAlgorithm.h"

CPart* CPart::s_instance = nullptr;

CPart::CPart()
{
}


CPart::~CPart()
{
}

CPart* CPart::Instance()
{
	if (s_instance == nullptr)
	{
		s_instance = new CPart;
	}
	return s_instance;
}

void CPart::Init()
{
	InitParts();
	InitQiDuiParts();
}

void CPart::InitParts()
{
	m_partsMap.clear();
	m_partIds.clear();
	vector<int8_t> composes;
	int8_t key = 0;
	composes.push_back(2);
	key = 2;
	vector<vector<int8_t>> parts;
	vector<int8_t> onePart(2, 0);
	InitOnePart(0, composes, parts, onePart);
	m_partsMap[key] = parts;
	for (int32_t i = 1; i <= 4; ++i)
	{
		composes.push_back(3);
		key += 3;
		vector<vector<int8_t>> parts;
		InitOnePart(0, composes, parts, onePart);
		m_partsMap[key] = parts;
	}
}

void CPart::InitOnePart(uint32_t idx, vector<int8_t>& composes, vector<vector<int8_t>>& parts, vector<int8_t>& onePart)
{
	for (uint32_t i=0; i<onePart.size(); ++i)
	{
		onePart[i] = onePart[i] + composes[idx];
		if (idx < composes.size()-1)
		{
			InitOnePart(idx + 1, composes, parts, onePart);
		}
		else
		{
			int32_t id = 0;
			for(uint32_t k=0; k<onePart.size(); ++k)
			{
				int32_t value = ((int32_t)onePart[k]) << (k * 4);
				id += value;
			}
			auto itr = m_partIds.find(id);
			if (itr == m_partIds.end())
			{
				m_partIds.insert(id);
				parts.push_back(onePart);
			}
		}
		onePart[i] = onePart[i] - composes[idx];
	}
}

void CPart::InitQiDuiParts()
{
	m_qiduiParts.clear();
	m_qiduiPartIds.clear();
	vector<int8_t> composes(7, 2);
	vector<int8_t> oneQiDuiPart(2, 0);
	InitOneQiDuiPart(0, composes, oneQiDuiPart);
}

void CPart::InitOneQiDuiPart(uint32_t idx, vector<int8_t>& composes, vector<int8_t>& oneQiDuiPart)
{
	for (uint32_t i=0; i<oneQiDuiPart.size(); ++i)
	{
		oneQiDuiPart[i] = oneQiDuiPart[i] + composes[idx];
		if (idx < composes.size()-1)
		{
			InitOneQiDuiPart(idx + 1, composes, oneQiDuiPart);
		}
		else
		{
			int32_t id = 0;
			for (uint32_t k = 0; k < oneQiDuiPart.size(); ++k)
			{
				int32_t value = ((int32_t)oneQiDuiPart[k]) << (k * 4);
				id += value;
			}
			auto itr = m_qiduiPartIds.find(id);
			if (itr == m_qiduiPartIds.end())
			{
				m_qiduiPartIds.insert(id);
				m_qiduiParts.push_back(oneQiDuiPart);
			}
		}
		oneQiDuiPart[i] = oneQiDuiPart[i] - composes[idx];
	}
}

vector<vector<int8_t>>& CPart::GetParts(int32_t sum)
{
	if (!isValidHandSum(sum))
	{
		//手牌总数不对，则有异常
		return m_partsMap[2];
	}
	return m_partsMap[sum];
}

vector<int8_t>& CPart::RandPart(int32_t sum)
{
	if (!isValidHandSum(sum))
	{
		sum = 14;
	}
	srand(time(nullptr));
	int32_t idx = rand() % m_partsMap[sum].size();
	return m_partsMap[sum][idx];
}

vector<int8_t>& CPart::RandQiDuiPart()
{
	srand(time(nullptr));
	int32_t idx = rand() % m_qiduiParts.size();
	return m_qiduiParts[idx];
}
