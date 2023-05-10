#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class CPart
{
public:
	CPart();
	~CPart();
	//µ¥Àý
	static CPart* Instance();
	void Init();
	vector<vector<int8_t>>& GetParts(int32_t sum);
	inline vector<vector<int8_t>>& GetQiDuiPart() { return m_qiduiParts; }
	vector<int8_t>& RandPart(int32_t sum);
	vector<int8_t>& RandQiDuiPart();
private:
	void InitParts();
	void InitOnePart(uint32_t idx, vector<int8_t>& composes, vector<vector<int8_t>>& parts, vector<int8_t>& onePart);
	void InitQiDuiParts();
	void InitOneQiDuiPart(uint32_t idx, vector<int8_t>& composes, vector<int8_t>& oneQiDuiPart);

private:
	static CPart* s_instance;
	unordered_map<int8_t, vector<vector<int8_t>>> m_partsMap;
	unordered_set<int32_t> m_partIds;
	vector<vector<int8_t>> m_qiduiParts;
	unordered_set<int32_t> m_qiduiPartIds;
};

