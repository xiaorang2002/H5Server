#include "CTableMgr.h"
#include <string>
#include <iostream>
#include <algorithm>

CTableMgr* CTableMgr::s_instance = nullptr;
const int32_t keys[] = { 2, 3, 5, 6, 8, 9, 11, 12, 14 };
const int32_t qiduiKeys[] = { 2, 4, 6, 8, 10, 12, 14 };

CTableMgr::CTableMgr()
{

	for (auto key : keys)
	{
		m_tables[key] = shared_ptr<CTable>(new CTable(TableType_Wan));
		m_ziTables[key] = shared_ptr<CTable>(new CTable(TableType_Zi));
	}
	for (auto key : qiduiKeys)
	{
		m_qiduiTables[key] = shared_ptr<CTable>(new CTable(TableType_Wan));
		m_ziQiduiTables[key] = shared_ptr<CTable>(new CTable(TableType_Zi));
	}
}


CTableMgr::~CTableMgr()
{

}

CTableMgr* CTableMgr::Instance()
{
	if (s_instance == nullptr)
	{
		s_instance = new CTableMgr();
	}
	return s_instance;
}

void CTableMgr::GenTable()
{
	//vector<int32_t> cards(9, 0);
	//vector<int32_t> ziCards(7, 0);
	//cout << "GenTable" << endl;
	//AddItemAndAllEyeToTable(cards);
	//GenTableItem(cards, 1);
	//cout << "GenZiTable" << endl;
	//AddItemAndAllEyeToTable(ziCards);
	//GenZiTableItem(ziCards, 1);
	//cout << "GenQiDuiTable" << endl;
	//GenQiDuiTableItem(cards, 1);
	//GenQiDuiTableItem(ziCards, 1);
	DumpTable();
}

void CTableMgr::DumpTable()
{
	cout << "DumpTable" << endl;
	for (auto item : m_tables)
	{
		string filename = "table_" + to_string(item.first);
		item.second->dump(filename);
	}
	cout << "DumpZiTable" << endl;
	for (auto item : m_ziTables)
	{
		string filename = "table_zi_" + to_string(item.first);
		item.second->dump(filename);
	}
	cout << "DumpQDuiTable" << endl;
	for (auto item : m_qiduiTables)
	{
		string filename = "table_qidui_" + to_string(item.first);
		item.second->dump(filename);
	}
	for (auto item : m_ziQiduiTables)
	{
		string filename = "table_qidui_zi_" + to_string(item.first);
		item.second->dump(filename);
	}
}

void CTableMgr::LoadTable()
{
	for (auto item : m_tables)
	{
		string filename = "table_" + to_string(item.first);
		item.second->load(filename);
	}
	for (auto item : m_ziTables)
	{
		string filename = "table_zi_" + to_string(item.first);
		item.second->load(filename);
	}
	for (auto item : m_qiduiTables)
	{
		string filename = "table_qidui_" + to_string(item.first);
		item.second->load(filename);
	}
	for (auto item : m_ziQiduiTables)
	{
		string filename = "table_qidui_zi_" + to_string(item.first);
		item.second->load(filename);
	}
}

shared_ptr<CTable> CTableMgr::GetTable(int32_t sum, bool zi)
{
	const int32_t* pKey = find(keys, keys + 9, sum);
	if (pKey  == keys + 9)
	{
		return nullptr;
	}
	return zi ? m_ziTables[sum] : m_tables[sum];
}

shared_ptr<CTable> CTableMgr::GetQiDuiTable(int32_t sum, bool zi)
{
	const int32_t* pKey = find(qiduiKeys, qiduiKeys + 7, sum);
	if (pKey == qiduiKeys + 7)
	{
		return nullptr;
	}
	return zi ? m_ziQiduiTables[sum] : m_qiduiTables[sum];
}

void CTableMgr::DumpProbability(string folderName)
{
#ifdef _WIN32
	for (auto item : m_tables)
	{
		string filename = "table_" + to_string(item.first);
		item.second->dumpPrInfos(folderName, filename);
	}
	for (auto item : m_ziTables)
	{
		string filename = "table_zi_" + to_string(item.first);
		item.second->dumpPrInfos(folderName, filename);
	}
	for (auto item : m_qiduiTables)
	{
		string filename = "table_qidui_" + to_string(item.first);
		item.second->dumpPrInfos(folderName, filename);
	}
	for (auto item : m_ziQiduiTables)
	{
		string filename = "table_qidui_zi_" + to_string(item.first);
		item.second->dumpPrInfos(folderName, filename);
	}
#endif
}

void CTableMgr::ClearProbability()
{
	for (auto item : m_tables)
	{
		item.second->clearPrInfos();
	}
	for (auto item : m_ziTables)
	{
		item.second->clearPrInfos();
	}
	for (auto item : m_qiduiTables)
	{
		item.second->clearPrInfos();
	}
	for (auto item : m_ziQiduiTables)
	{
		item.second->clearPrInfos();
	}
}

void CTableMgr::AddItemToTable(vector<int32_t>& cards)
{
	int32_t value = 0;
	int32_t sum = 0;
	for (int32_t i=0; i<cards.size(); ++i)
	{
		value += cards[i];
		if (i != cards.size()-1)
		{
			value *= 10;
		}
		sum += cards[i];
	}
	if (sum != 0)
	{
		if (cards.size() == 7)
		{
			m_ziTables[sum]->add(value);
		}
		else
		{
			m_tables[sum]->add(value);
		}
	}
}

void CTableMgr::AddItemAndAllEyeToTable(vector<int32_t>& cards)
{
	AddItemToTable(cards);
	for (uint32_t i=0; i<cards.size(); ++i)
	{
		if (cards[i] < 3)
		{
			cards[i] += 2;
			AddItemToTable(cards);
			cards[i] -= 2;
		}
	}
}

void CTableMgr::GenTableItem(vector<int32_t>& cards, int32_t level)
{
	for (int i=0; i<16; ++i)
	{
		if (i < cards.size())
		{
			if (cards[i] >= 2)
			{
				continue;
			}
			cards[i] += 3;
		}
		else
		{
			int32_t idx = i - cards.size();
			if (cards[idx] >= 4 || cards[idx+1] >= 4 || cards[idx+2] >= 4)
			{
				continue;
			}
			cards[idx] += 1;
			cards[idx+1] += 1;
			cards[idx+2] += 1;
		}
		AddItemAndAllEyeToTable(cards);
		if (level < 4)
		{
			GenTableItem(cards, level + 1);
		}
		if (i < 9)
		{
			cards[i] -= 3;
		} 
		else
		{
			int32_t idx = i - 9;
			cards[idx] -= 1;
			cards[idx + 1] -= 1;
			cards[idx + 2] -= 1;
		}
	}
}

void CTableMgr::GenZiTableItem(vector<int32_t>& cards, int32_t level)
{
	for (int i = 0; i < cards.size(); ++i)
	{
		if (cards[i] >= 2)
		{
			continue;
		}
		cards[i] += 3;
		AddItemAndAllEyeToTable(cards);
		if (level < 4)
		{
			GenZiTableItem(cards, level + 1);
		}
		cards[i] -= 3;
	}
}

void CTableMgr::GenQiDuiTableItem(vector<int32_t>& cards, int32_t level)
{
	for (int i = 0; i < cards.size(); ++i)
	{
		if (cards[i] >= 3)
		{
			continue;
		}
		cards[i] += 2;
		int32_t value = 0;
		int32_t sum = 0;
		for (int32_t i = 0; i < cards.size(); ++i)
		{
			value += cards[i];
			if (i != cards.size() - 1)
			{
				value *= 10;
			}
			sum += cards[i];
		}
		if (cards.size() == 7)
		{
			m_ziQiduiTables[sum]->add(value);
		}
		else
		{
			m_qiduiTables[sum]->add(value);
		}
		if (level < 7)
		{
			GenQiDuiTableItem(cards, level + 1);
		}
		cards[i] -= 2;
	}
}
