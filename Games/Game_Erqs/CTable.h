#pragma once

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <cstdint>
#include <array>
#include "MjAlgorithm.h"

using namespace std;

enum TableType
{
	TableType_None,
	TableType_Wan,
	TableType_Tong,
	TableType_Tiao,
	TableType_Zi,
	TableType_Max
};

class CTable
{
public:
	CTable(TableType type);
	virtual ~CTable();
	//增加
	void add(int32_t item);
	//存储
	void dump(string filename);
	//加载
	void load(string filename);
	//获取
	inline const unordered_set<int32_t>& GetItems() { return m_items; }
	inline const vector<array<int32_t, 9>>& GetTables() { return m_tables; }
	vector<int32_t> RandTableItem();
	//清空概率信息
	void clearPrInfos();
	//增加概率信息
	void addPrInfo(TablePrInfo& info);
	//获取所有概率
	inline vector<TablePrInfo>& GetPrInfos() { return m_prInfos; }
	//获取最快胡的概率
	inline vector<TablePrInfo>& GetValidFastestHuPrInfos() { return m_validFastHuPrInfos; }
	//获取最快胡的概率
	inline vector<TablePrInfo>& GetFastestHuPrInfos() { return m_fastHuPrInfos; }
	//计算最快胡牌概率
	int64_t calculateFastestHuPr(array<int32_t, 9> leftTable);
	//存储概率
	void dumpPrInfos(string folderName, string filename);
	//组合概率
	inline void SetPartProbality(int64_t probality) { m_partProbality = probality; };
	//组合最小需求
	inline void SetPartMinNeed(int32_t minNeed) { m_partMinNeed = minNeed; }
	//最优概率
	inline void SetBest(bool best) { m_isBest = best; };
	inline TableType GetTableType() { return m_type; }
	inline int32_t GetStartIdx() { return m_startIdx; }

private:
	unordered_set<int32_t>	m_items;
	vector<array<int32_t, 9>> m_tables;
	vector<TablePrInfo> m_prInfos;
	vector<TablePrInfo> m_fastHuPrInfos;
	vector<TablePrInfo> m_validFastHuPrInfos;
	TableType m_type;
	int64_t m_partProbality;
	int32_t m_partMinNeed;
	bool m_isBest;
	int32_t m_startIdx;
};

