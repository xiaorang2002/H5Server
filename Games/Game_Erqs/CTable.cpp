#include "CTable.h"
#include <fstream>
#ifdef _WIN32
#include<direct.h> 
#endif
#include <algorithm>
#include "MjAlgorithm.h"
#include <cmath>
#include <ctime>

CTable::CTable(TableType type)
{
	m_type = type;
	int32_t startIdxs[] = { 0,1,10,19,28 };
	m_startIdx = startIdxs[m_type];
	clearPrInfos();
}

CTable::~CTable()
{
}


void CTable::add(int32_t item)
{
	m_items.insert(item);
}

void CTable::dump(string filename)
{
	string path = "table/" + filename;
	ofstream file;
	file.open(path, ios::out | ios::trunc);
	for (auto item : m_items)
	{
		file << item << endl;
	}
	file.close();
}

void CTable::load(string filename)
{
	string path = "./conf/erqs/table/" + filename;
	ifstream file;
	file.open(path, ios::in);
	m_items.clear();
	while (!file.eof())
	{
		int32_t item;
		file >> item;
		if (item < 0)
		{
			continue;
		}
		m_items.insert(item);
	}
	file.close();
	m_tables.clear();
	int32_t offset = m_type == TableType_Zi ? 2 : 0;
	for (auto item : m_items)
	{
		array<int32_t, 9> table = { 0 };
		for (int32_t i = 0; i < 9; ++i)
		{
			int32_t value = item / ((int32_t)pow(10, i));
			if (value == 0)
			{
				break;
			}
			//字长度=7
			int32_t idx = 8 - i - offset;
			table[idx] = value % 10;
		}
		m_tables.push_back(table);
	}
}

vector<int32_t> CTable::RandTableItem()
{
	vector<int32_t> randCards;
	srand(time(nullptr));
	int32_t idx = rand() % m_tables.size();
	int32_t len = m_type == TableType_Zi ? 7 : 9;
	for (int32_t i = 0; i < len; ++i)
	{
		for (int32_t k = 0; k < m_tables[idx][i]; ++k)
		{
			int32_t card = getCardByIdx(m_startIdx + i);
			randCards.push_back(card);
		}
	}
	return randCards;
}

void CTable::clearPrInfos()
{
	m_prInfos.clear();
	m_fastHuPrInfos.clear();
	m_validFastHuPrInfos.clear();
	m_partProbality = 0.0;
	m_isBest = false;
	m_partMinNeed = 0;
}

void CTable::addPrInfo(TablePrInfo & info)
{
	m_prInfos.push_back(info);
}

int64_t CTable::calculateFastestHuPr(array<int32_t, 9> leftTable)
{
	if (m_prInfos.empty())
	{
		return 0.0;
	}
	sort(m_prInfos.begin(), m_prInfos.end(), [&](TablePrInfo a, TablePrInfo b)
		{
			return a.need < b.need;
		});
	int32_t minNeed = m_prInfos[0].need;
	//最快胡的概率
	m_fastHuPrInfos.clear();
	for (auto probabilityInfo : m_prInfos)
	{
		if (probabilityInfo.need > minNeed)
		{
			break;
		}
		m_fastHuPrInfos.push_back(probabilityInfo);
	}
	sort(m_fastHuPrInfos.begin(), m_fastHuPrInfos.end(), [&](TablePrInfo a, TablePrInfo b)
		{
			return a.probability > b.probability;
		});
	//有效的概率
	m_validFastHuPrInfos.clear();
	for (auto probabilityInfo : m_fastHuPrInfos)
	{
		bool enough = true;
		for (uint32_t i = 0; i < 9; ++i)
		{
			if (leftTable[i] < probabilityInfo.needTable[i])
			{
				enough = false;
				break;
			}
		}
		if (!enough)
		{
			continue;
		}
		for (uint32_t i = 0; i < 9; ++i)
		{
			leftTable[i] -= probabilityInfo.needTable[i];
		}
		m_validFastHuPrInfos.push_back(probabilityInfo);
	}
	//统计概率
	double probability = 0;
	for (auto probabilityInfo : m_validFastHuPrInfos)
	{
		probability += probabilityInfo.probability;
	}
	return probability;
}

void CTable::dumpPrInfos(string folderName, string filename)
{
#ifdef _WIN32
	if (m_prInfos.empty())
	{
		return;
	}
	if (!m_isBest)
	{
		return;
	}
	string foldPath = "probability/" + folderName;
	_mkdir(foldPath.c_str());
	string path = "probability/" + folderName + "/" + filename;
	ofstream file;
	file.open(path, ios::out | ios::trunc);

	if (m_isBest)
	{
		file << u8"**********************************最优概率*****************************************" << endl;
	}

	if (m_partProbality > 0)
	{
		file << u8"**********************************组合概率*****************************************" << endl;
		file << u8"组合概率 m_partProbality=" << m_partProbality << "		minNeed=" << m_partMinNeed << endl;
	}

	file << u8"**********************************最快胡牌有效概率*****************************************" << endl;
	double probability = 0.0;
	for (auto item : m_validFastHuPrInfos)
	{
		string keyStr;
		string needTableStr;
		for (int32_t i = 0; i < item.huTable.size(); ++i)
		{
			for (int32_t n = 0; n < item.huTable[i]; ++n)
			{
				keyStr += getCardStr(getCardByIdx(m_startIdx + i));
			}
			for (int32_t n = 0; n < item.needTable[i]; ++n)
			{
				needTableStr += getCardStr(getCardByIdx(m_startIdx + i));
			}
		}
		file << "key=" << keyStr << "		need=" << item.need << "		probability=" << item.probability << "		needTable=" << needTableStr << endl;
		probability += item.probability;
	}
	file << u8"**********************************局部概率*****************************************" << endl;
	file << u8"局部概率 probability=" << probability << endl;
	file << u8"**********************************最快胡牌概率*****************************************" << endl;
	for (auto item : m_fastHuPrInfos)
	{
		string keyStr;
		string needTableStr;
		for (int32_t i = 0; i < item.huTable.size(); ++i)
		{
			for (int32_t n = 0; n < item.huTable[i]; ++n)
			{
				keyStr += getCardStr(getCardByIdx(m_startIdx + i));
			}
			for (int32_t n = 0; n < item.needTable[i]; ++n)
			{
				needTableStr += getCardStr(getCardByIdx(m_startIdx + i));
			}
		}
		file << "key=" << keyStr << "		need=" << item.need << "		probability=" << item.probability << "		needTable=" << needTableStr << endl;
	}

	//sort(m_prInfos.begin(), m_prInfos.end(), [&](TablePrInfo a, TablePrInfo b)
	//	{
	//		return a.need < b.need;
	//	});
	//file << u8"**********************************所有概率*****************************************" << endl;
	//for (auto item : m_prInfos)
	//{
	//	string keyStr;
	//	string needTableStr;
	//	for (int32_t i = 0; i < item.huTable.size(); ++i)
	//	{
	//		for (int32_t n = 0; n < item.huTable[i]; ++n)
	//		{
	//			keyStr += getCardStr(getCardByIdx(startIdx + i));
	//		}
	//		for (int32_t n = 0; n < item.needTable[i]; ++n)
	//		{
	//			needTableStr += getCardStr(getCardByIdx(startIdx + i));
	//		}
	//	}
	//	file << "key=" << keyStr << "		need=" << item.need << "		probability=" << item.probability << "		needTable=" << needTableStr << endl;
	//}
	file.close();
#endif
}
