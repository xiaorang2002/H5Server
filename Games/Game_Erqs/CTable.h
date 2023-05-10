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
	//����
	void add(int32_t item);
	//�洢
	void dump(string filename);
	//����
	void load(string filename);
	//��ȡ
	inline const unordered_set<int32_t>& GetItems() { return m_items; }
	inline const vector<array<int32_t, 9>>& GetTables() { return m_tables; }
	vector<int32_t> RandTableItem();
	//��ո�����Ϣ
	void clearPrInfos();
	//���Ӹ�����Ϣ
	void addPrInfo(TablePrInfo& info);
	//��ȡ���и���
	inline vector<TablePrInfo>& GetPrInfos() { return m_prInfos; }
	//��ȡ�����ĸ���
	inline vector<TablePrInfo>& GetValidFastestHuPrInfos() { return m_validFastHuPrInfos; }
	//��ȡ�����ĸ���
	inline vector<TablePrInfo>& GetFastestHuPrInfos() { return m_fastHuPrInfos; }
	//���������Ƹ���
	int64_t calculateFastestHuPr(array<int32_t, 9> leftTable);
	//�洢����
	void dumpPrInfos(string folderName, string filename);
	//��ϸ���
	inline void SetPartProbality(int64_t probality) { m_partProbality = probality; };
	//�����С����
	inline void SetPartMinNeed(int32_t minNeed) { m_partMinNeed = minNeed; }
	//���Ÿ���
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

