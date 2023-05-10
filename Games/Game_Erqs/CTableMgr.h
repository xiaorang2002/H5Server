#pragma once

#include <unordered_map>
#include "CTable.h"
#include <memory>
#include <vector>
#include <cstdint>

using namespace std;

class CTableMgr
{
public:
	CTableMgr();
	~CTableMgr();
	//单例接口
	static CTableMgr* Instance();
	//生成表
	void GenTable();
	//存储表
	void DumpTable();
	//加载表
	void LoadTable();
	//获取
	shared_ptr<CTable> GetTable(int32_t sum, bool zi);
	shared_ptr<CTable> GetQiDuiTable(int32_t sum, bool zi);
	//存储概率表-用于测试
	void DumpProbability(string folderName);
	//清空概率-用于测试
	void ClearProbability();
private:
	//往表增加元素
	void AddItemToTable(vector<int32_t>& cards);
	//往表增加元素及该元素所有带将元素
	void AddItemAndAllEyeToTable(vector<int32_t>& cards);
	//生成万筒条牌表元素
	void GenTableItem(vector<int32_t>& cards, int32_t level);
	//生成字牌表元素
	void GenZiTableItem(vector<int32_t>& cards, int32_t level);
	//生成七对表元素
	void GenQiDuiTableItem(vector<int32_t>& cards, int32_t level);
private:
	//万筒条花色胡牌表
	unordered_map<int32_t, shared_ptr<CTable>> m_tables;
	//字胡牌表
	unordered_map<int32_t, shared_ptr<CTable>> m_ziTables;
	//七对表
	unordered_map<int32_t, shared_ptr<CTable>> m_qiduiTables;
	unordered_map<int32_t, shared_ptr<CTable>> m_ziQiduiTables;
	static CTableMgr* s_instance;
};

