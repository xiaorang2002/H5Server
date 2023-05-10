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
	//�����ӿ�
	static CTableMgr* Instance();
	//���ɱ�
	void GenTable();
	//�洢��
	void DumpTable();
	//���ر�
	void LoadTable();
	//��ȡ
	shared_ptr<CTable> GetTable(int32_t sum, bool zi);
	shared_ptr<CTable> GetQiDuiTable(int32_t sum, bool zi);
	//�洢���ʱ�-���ڲ���
	void DumpProbability(string folderName);
	//��ո���-���ڲ���
	void ClearProbability();
private:
	//��������Ԫ��
	void AddItemToTable(vector<int32_t>& cards);
	//��������Ԫ�ؼ���Ԫ�����д���Ԫ��
	void AddItemAndAllEyeToTable(vector<int32_t>& cards);
	//������Ͳ���Ʊ�Ԫ��
	void GenTableItem(vector<int32_t>& cards, int32_t level);
	//�������Ʊ�Ԫ��
	void GenZiTableItem(vector<int32_t>& cards, int32_t level);
	//�����߶Ա�Ԫ��
	void GenQiDuiTableItem(vector<int32_t>& cards, int32_t level);
private:
	//��Ͳ����ɫ���Ʊ�
	unordered_map<int32_t, shared_ptr<CTable>> m_tables;
	//�ֺ��Ʊ�
	unordered_map<int32_t, shared_ptr<CTable>> m_ziTables;
	//�߶Ա�
	unordered_map<int32_t, shared_ptr<CTable>> m_qiduiTables;
	unordered_map<int32_t, shared_ptr<CTable>> m_ziQiduiTables;
	static CTableMgr* s_instance;
};

