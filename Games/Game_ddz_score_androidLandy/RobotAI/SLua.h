
#pragma once

#pragma warning(disable:4244)		
#pragma warning(disable:4800)		
#pragma warning(disable:4099)	

#define	 Def_Lua_Max_Num		99999999999
//#define  Def_Lua_Buf_Len		200

#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include <types.h>

using namespace std;

enum SType
{
	TP_NIL,
	TP_NUMBER,
	TP_STRING,
	TP_TABLE,
	TP_NULL,
};

//牌色，牌花
enum emPokeColor
{
	emPokeColor_Error,										//错误花色
	emPokeColor_TingYong,
	emPokeColor_NoneColor,									//无花色牌	
	emPokeColor_Diamonds,									//方块
	emPokeColor_Clubs,										//梅花
	emPokeColor_Hearts,										//红桃	
	emPokeColor_Spades,										//黑桃
	emPokeColor_End,
};
//牌值，牌点
enum emPokeValue
{
	emPokeValue_Error = 0,								//错误点数
	emPokeValue_Two = 2,
	emPokeValue_Three = 3,
	emPokeValue_Four = 4,
	emPokeValue_Five = 5,
	emPokeValue_Six = 6,
	emPokeValue_Seven = 7,
	emPokeValue_Eight = 8,
	emPokeValue_Nine = 9,
	emPokeValue_Ten = 10,
	emPokeValue_J = 11,
	emPokeValue_Q = 12,
	emPokeValue_K = 13,
	emPokeValue_A = 14,
	emPokeValue_SmallKing = 15,							//小王
	emPokeValue_BigKing = 16,							//大王
	emPokeValue_TingYongPai = 17,							//听用牌初始点数
	emPokeValue_End,
};

enum emCODDZPaiXing											//牌型(一定要按照牌型大小依次写，因为有直接比较)
{
	emCODDZPaiXing_Invalid,									//无效牌
	emCODDZPaiXing_DanPai,									//单牌
	emCODDZPaiXing_DuiZi,									//对子
	emCODDZPaiXing_LianDui,									//连对
	emCODDZPaiXing_SanZhang,								//三张
	emCODDZPaiXing_SanDaiDanPai,							//三带一：三张带单张
	emCODDZPaiXing_SanDaiDuiZi,								//三带二：三张带对子
	emCODDZPaiXing_ShunZi,									//顺子：五张或以上点数相连的牌（不包括2和王）(即连牌)
	emCODDZPaiXing_SanShunZi,               				//三顺：二个或更多连续的三张牌(也叫连飞机)
	emCODDZPaiXing_FeiJiDaiDanPai,							//飞机带单牌
	emCODDZPaiXing_FeiJiDaiDuiZi,							//飞机带对子
	emCODDZPaiXing_4Dai2DanPai,								//4带2单牌
	emCODDZPaiXing_4Dai2DuiZi,								//4带2对子
	emCODDZPaiXing_YingZha,									//炸弹：四张同点牌，也称硬炸
	emCODDZPaiXing_HuoJian,									//火箭：即双王（双鬼牌）
};

class SVar
{
	friend class SSerialize;
	friend class SLua;
	friend class SVarFunctor;
public:
	SVar();
	SVar(int n);
	SVar(unsigned int n);
    SVar(int64_t n);
	SVar(double n);
	SVar(const char *p);
	SVar(const string &str);
	SVar(const SVar &s);
	~SVar();
	SVar&    						operator=(int n);
	SVar&    						operator=(unsigned int n);
    SVar&    						operator=(int64_t n);
	SVar&    						operator=(double n);
	SVar&    						operator=(const char *p);
	SVar&    						operator=(const string &str);
	SVar&    						operator=(const SVar &s);
public:
	template<typename T, typename U>
	void							Insert(T key, U value)
	{
		if (Push(new SVar(key), new SVar(value)) == false)
			throw("Insert 错误1");
	};
	template<typename U>
	void							Insert(U value)
	{
		SVar *pKey = new SVar(1);
		for (int i = 0; i<(int)m_dbNumber; i += 2)
		{
			if (m_pTable[i]->m_type == TP_NUMBER)
			{
				if (m_pTable[i]->m_dbNumber >= pKey->m_dbNumber)
				{
					pKey->m_dbNumber = (long long)(m_pTable[i]->m_dbNumber + 1);
				}
			}
		}

		if (Push(pKey, new SVar(value)) == false)
			throw("Insert 错误1");
	};
public:
	void							Clear();
	void							ClearTable();
	SType							Type();
	void							SetType(SType tp);
	bool							IsEmpty();
	bool							IsNil();
	bool							IsNumber();
	bool							IsString();
	bool							IsTable();
public:
	template<typename T>
	T         						ToNumber()
	{
		return (T)m_dbNumber;
	}
	string		 					ToString();
public:
	bool							Find(int n);
	bool							Find(unsigned int n);
    bool							Find(int64_t n);
	bool							Find(double n);
	bool							Find(const char *p);
	bool							Find(const string &str);

	SVar&							operator[](int n);
	SVar&							operator[](unsigned int n);
    SVar&							operator[](int64_t n);
	SVar&							operator[](double n);
	SVar&							operator[](const char *p);
	SVar&							operator[](const string &str);
	SVar&							operator[](SVar *p);

	int								GetSize();
	SVar&							GetKey(int i);
	SVar&							GetValue(int i);
	bool							Push(SVar *pKey, SVar *pValue);
	bool							Push(SVar *pValue);

protected:
	template<typename T>
	bool			CheckNumber(T &nNum)				//触发作弊
	{
        LONGLONG nMax = Def_Lua_Max_Num;
		if (nNum>(T)nMax)
		{
			return false;
		}
		if (nNum<-(T)nMax)
			return false;
		return true;
	}
private:
	SType     						m_type;
	double      					m_dbNumber;
	string      					m_str;
	SVar*							*m_pTable;
};




struct	ACard
{
public:
	emPokeColor	 n8CardColor;		// ASCII表中，3，4，5，6分别代表红桃，方块，梅花，黑桃。2代表无花色。0为错误牌
	emPokeValue	 n8CardNumber;		// 大小对应为2345678910JQKA小王大王听用牌=234567891011121314151617。 0为错误牌
	
	void FromByte(uint8_t uc)
	{
		switch (uc & 0xF0)
		{
		case 0:
			n8CardColor = emPokeColor_Diamonds;
			break;
		case 0x10:
			n8CardColor = emPokeColor_Clubs;
			break;
		case 0x20:
			n8CardColor = emPokeColor_Hearts;
			break;
		case 0x30:
			n8CardColor = emPokeColor_Spades;
			break;
		}

		n8CardNumber = emPokeValue_TingYongPai;

		switch (uc & 0x0F)
		{
		case 1:
			n8CardNumber = emPokeValue_A;
			break;
		case 2:
			n8CardNumber = emPokeValue_Two;
			break;
		case 3:
			n8CardNumber = emPokeValue_Three;
			break;
		case 4:
			n8CardNumber = emPokeValue_Four;
			break;
		case 5:
			n8CardNumber = emPokeValue_Five;
			break;
		case 6:
			n8CardNumber = emPokeValue_Six;
			break;
		case 7:
			n8CardNumber = emPokeValue_Seven;
			break;
		case 8:
			n8CardNumber = emPokeValue_Eight;
			break;
		case 9:
			n8CardNumber = emPokeValue_Nine;
			break;
		case 0xa:
			n8CardNumber = emPokeValue_Ten;
			break;
		case 0xb:
			n8CardNumber = emPokeValue_J;
			break;
		case 0xc:
			n8CardNumber = emPokeValue_Q;
			break;
		case 0xd:
			n8CardNumber = emPokeValue_K;
			break;
		case 0xe:
			n8CardNumber = emPokeValue_SmallKing;
			break;
		case 0xf:
			n8CardNumber = emPokeValue_BigKing;
			break;
		}
	}

	uint8_t toByte()
	{
		uint8_t uc=0x40;
		switch (n8CardColor)
		{
		case	emPokeColor_Diamonds://								//方块
			uc = 0;
			break;
		case	emPokeColor_Clubs://								//梅花
			uc = 0x10;
			break;
		case 	emPokeColor_Hearts:									//红桃	
			uc = 0x20;
			break;
		case	emPokeColor_Spades:										//黑桃
			uc = 0x30;
			break;
		}

		switch (n8CardNumber)
		{
		case 	emPokeValue_Two:
			uc = uc | 0x02;
			break;
		case	emPokeValue_Three:
			uc = uc | 0x03;
			break;
		case	emPokeValue_Four:
			uc = uc | 0x04;
			break;
		case	emPokeValue_Five:
			uc = uc | 0x05;
			break;
		case	emPokeValue_Six:
			uc = uc | 0x06;
			break;
		case	emPokeValue_Seven:
			uc = uc | 0x07;
			break;
		case	emPokeValue_Eight:
			uc = uc | 0x08;
			break;
		case	emPokeValue_Nine:
			uc = uc | 0x09;
			break;
		case	emPokeValue_Ten:
			uc = uc | 0x0a;
			break;
		case	emPokeValue_J:
			uc = uc | 0x0b;
			break;
		case	emPokeValue_Q:
			uc = uc | 0x0c;
			break;
		case	emPokeValue_K:
			uc = uc | 0x0d;
			break;
		case	emPokeValue_A:
			uc = uc | 0x01;
			break;
		case	emPokeValue_SmallKing://小王
			uc = uc | 0x0e;
			break;
		case	emPokeValue_BigKing:	//大王
			uc = uc | 0x0f;
			break;
		}
		return uc;
	}

	ACard()
	{
		Clear();
	}
	int	GetKey()
	{
		return n8CardColor * 100 + n8CardNumber;
	}
	ACard(emPokeColor nColor, emPokeValue nNumber)
	{
		n8CardColor = nColor;
		n8CardNumber = nNumber;
	}
	bool IsInvalid()	//是无效牌true无效 false有效
	{
		return IsVaild() == false;
	}
	bool IsKing()
	{//是否是王牌
		if (IsVaild() == false)
			return false;
		return n8CardNumber == emPokeValue_BigKing || n8CardNumber == emPokeValue_SmallKing;
	}
	bool IsBigKing()
	{
		if (IsVaild() == false)
			return false;
		return n8CardNumber == emPokeValue_BigKing;

	}
	bool IsDiZhuDaPai()
	{//是否是大牌
		return n8CardNumber>emPokeValue_J || n8CardNumber == emPokeValue_Two;
	}

	bool IsVaild()//有效?
	{
		return n8CardNumber>emPokeValue_Error &&n8CardNumber <= emPokeValue_BigKing && n8CardColor>emPokeColor_Error &&n8CardColor <= emPokeColor_Spades;
	}
	void SetACard(emPokeColor CardColor, emPokeValue CardNumber) //设置一张牌
	{
		n8CardColor = CardColor;
		n8CardNumber = CardNumber;
	}
	void SetDefACard(emPokeValue emV)
	{//设置为默认的牌
		n8CardColor = emPokeColor_Spades;
		n8CardNumber = emV;

	}
	bool SetACard(SVar &s)
	{
		if (s.Find("cardcolor") && s["cardcolor"].IsNumber() &&
			s.Find("cardnumber") && s["cardnumber"].IsNumber())
		{
			n8CardColor = (emPokeColor)s["cardcolor"].ToNumber<int>();
			n8CardNumber = (emPokeValue)s["cardnumber"].ToNumber<int>();
			return true;
		}
		return false;
	}
	bool SetACardEx(SVar &s)
	{
		if (s.Find("color") && s["color"].IsNumber() &&
			s.Find("number") && s["number"].IsNumber())
		{
			n8CardColor = (emPokeColor)s["color"].ToNumber<int>();
			n8CardNumber = (emPokeValue)s["number"].ToNumber<int>();
			return true;
		}
		return false;
	}
	void Clear()
	{
		n8CardColor = emPokeColor_Error;
		n8CardNumber = emPokeValue_Error;
	}
	void	SetUnValidPai()
	{//设置为无效的牌
		n8CardColor = emPokeColor_Error;
		n8CardNumber = emPokeValue_Error;
	}
	void	SetTingYong()
	{//设置为听用牌
		n8CardColor = emPokeColor_TingYong;
		n8CardNumber = emPokeValue_TingYongPai;
	}

	bool	CanCalShuiZhi()
	{//判断是否能计算顺子(不包括听用牌)		
		return n8CardNumber >= emPokeValue_Three&& n8CardNumber <= emPokeValue_A;
	}
	bool	IsSame(ACard &s)
	{//判断是否是相同的牌
		return n8CardColor == s.n8CardColor && n8CardNumber == s.n8CardNumber;
	}
	void	PushNode(SVar &s)
	{//Push数据
		s["cardcolor"] = (int)n8CardColor;
		s["cardnumber"] = n8CardNumber;
	}
	void	PushNodeEx(SVar &s)
	{//Push数据
		s["color"] = (int)n8CardColor;
		s["number"] = n8CardNumber;
	}
	string GetstrDesc()
	{
		char szDesc[30] = {};
        snprintf(szDesc, 30, "Poke:%d Point:%d,", n8CardColor, n8CardNumber);
		return szDesc;
	}
	string GetstrLog()
	{
		char szDesc[30] = {};
        snprintf(szDesc, 30, "%s%s", GetcstrPokeColor(n8CardColor).c_str(), GetcstrPokeVaule(n8CardNumber).c_str());
		return szDesc;
	}

	static string GetcstrPokeColor(emPokeColor em)
	{
		switch (em)
		{
		case emPokeColor_TingYong:
			return "听";
		case emPokeColor_NoneColor:									//无花色牌	
			return "";
		case emPokeColor_Diamonds:									//方块
			return "方";
		case emPokeColor_Clubs:										//梅花
			return "梅";
		case emPokeColor_Hearts:										//红桃	
			return "红";
		case emPokeColor_Spades:										//黑桃
			return "黑";
		default:
			break;
			//case emPokeColor_Error,										//错误花色
			//case emPokeColor_End,
		}
		return "error";

	}
	static string GetcstrPokeVaule(emPokeValue em)
	{
		//牌值，牌点
		switch (em)
		{
		case emPokeValue_Two:
			return "2";
		case emPokeValue_Three:
			return "3";
		case emPokeValue_Four:
			return "4";
		case emPokeValue_Five:
			return "5";
		case emPokeValue_Six:
			return "6";
		case emPokeValue_Seven:
			return "7";
		case emPokeValue_Eight:
			return "8";
		case emPokeValue_Nine:
			return "9";
		case emPokeValue_Ten:
			return "10";
		case emPokeValue_J:
			return "J";
		case emPokeValue_Q:
			return "Q";
		case emPokeValue_K:
			return "K";
		case emPokeValue_A:
			return "A";
		case emPokeValue_SmallKing:
			return "SK";
		case emPokeValue_BigKing:
			return "BK";
		case emPokeValue_TingYongPai:
			return"";
		default:
			break;
		};
		return "error";
	}
	static string GetstrDesc(vector<ACard>& v)
	{
		string str;
		for (unsigned int i = 0; i<v.size(); i++)
		{
			str += v[i].GetstrDesc();
		}
		return str;
	}
	int	GetNiuNum()
	{//得到组牛Num
	 //if(n8CardNumber==0)
		if (IsVaild() == false)
		{
			//SHOW("GetNum()..出现无效牌:%s", GetstrDesc().c_str());
			return 0;
		}
		if (n8CardNumber>10)
			return 10;
		return n8CardNumber;
	}
	bool IsTingYong()
	{//判断是否是听用牌

		return n8CardColor == emPokeColor_TingYong || n8CardNumber == (int)emPokeValue_TingYongPai;
	}
	static bool	Check(emPokeColor CardColor, emPokeValue CardNumber)
	{
		if (CardColor<emPokeColor_NoneColor || CardColor>emPokeColor_Spades)
			return false;

		if (CardColor == emPokeColor_NoneColor)
		{
			return CardNumber == (int)emPokeValue_SmallKing || CardNumber == (int)emPokeValue_BigKing;
		}
		if (CardColor == emPokeColor_TingYong)
			return CardNumber == (int)emPokeValue_TingYongPai;

		return CardNumber >= (int)emPokeValue_Two&&CardNumber <= (int)emPokeValue_A;
	}
	static ACard SVarToACard(SVar &sVar, string paiName)
	{
		ACard vCard;
		for (int i = 0; i<sVar.GetSize(); ++i)
		{
			SVar& sKeyTemp = sVar.GetKey(i);//pai
			SVar& sValueTemp = sVar.GetValue(i);//具体值
			string strKey;
			if (sKeyTemp.IsString())
				strKey = sKeyTemp.ToString();

			if (!sKeyTemp.IsString() || sKeyTemp.ToString() != paiName)
				continue;
			if (vCard.SetACard(sValueTemp) == true)
				break;
		}

		return vCard;
	}
	static vector<ACard> SVarToACards(SVar &sVar, string paiName)
	{
		vector<ACard> vCard;
		for (int i = 0; i<sVar.GetSize(); ++i)
		{
			SVar& sKeyTemp = sVar.GetKey(i);//pai
			SVar& sValueTemp = sVar.GetValue(i);//具体值
			string strKey;
			if (sKeyTemp.IsString())
				strKey = sKeyTemp.ToString();

			if (!sKeyTemp.IsString() || sKeyTemp.ToString() != paiName)
				continue;
			for (int j = 0; j<sValueTemp.GetSize(); ++j)
			{
				SVar& sValueTempSecond = sValueTemp.GetValue(j);//具体值sVar["cardcolor"]/sVar["cardcolor"]				
				ACard eachCard;
				if (eachCard.SetACard(sValueTempSecond) == true)
					vCard.push_back(eachCard);
			}
		}
		return vCard;
	}
	static vector<ACard> SVarToACardsEx(SVar &sVar, string paiName)
	{
		vector<ACard> vCard;
		if (sVar.Find(paiName) == false)
			return vCard;
		SVar&s = sVar[paiName];
		if (s.IsTable() == false)
			return vCard;
		ACard info;
		for (int i = 0; i<s.GetSize(); i++)
		{
			SVar &sVal = s.GetValue(i);

			if (info.SetACardEx(sVal))
			{
				vCard.push_back(info);
			}
		}
		return vCard;
	}
	static void ACardToSVar(ACard &vCard, string paiName, SVar &sVar)
	{//将Card转成SVar
		SVar sTempVar;
		vCard.PushNode(sTempVar);
		sVar[paiName] = sTempVar;
	}
	static void ACardsToSVar(vector<ACard> &vCard, string paiName, SVar &sVar)
	{//将多张牌转成SVar
		SVar sAllPaiVar;
		SVar sSEachPaiVar;
		for (unsigned int i = 0; i < vCard.size(); i++)
		{
			sSEachPaiVar.Clear();
			vCard[i].PushNode(sSEachPaiVar);
			sAllPaiVar[i] = sSEachPaiVar;
		}
		sVar[paiName] = sAllPaiVar;
	}
	static void ACardsToSVar(vector<ACard> &vCard, SVar &sVar)
	{//将多张牌转成SVar		
		SVar sOne;
		for (unsigned int i = 0; i < vCard.size(); i++)
		{
			vCard[i].PushNode(sOne);
			sVar.Insert(sOne);
		}
	}
	static string  PushLogRec(vector<ACard> &vCard, string paiName)
	{//将多张牌转成SVar		
		string str = paiName;
		for (unsigned int i = 0; i < vCard.size(); i++)
		{
			str += vCard[i].GetstrLog().c_str();
			str += " ";
		}
		return str;
	}

	static void ACardsToSVarEx(vector<ACard> &vCard, string paiName, SVar &sVar)
	{//将多张牌转成SVar
		SVar sAllPaiVar;
		SVar sOne;
		for (unsigned int i = 0; i < vCard.size(); i++)
		{
			vCard[i].PushNodeEx(sOne);
			sAllPaiVar.Insert(sOne);
		}
		sVar[paiName] = sAllPaiVar;
	}
	static bool	 DelCards(vector<ACard>&vSrc, vector<ACard> &vDel)
	{//从vSrc里删除vDel数据
		bool bRe = false;
		for (unsigned int i = 0; i<vDel.size(); i++)
		{
			for (vector<ACard>::iterator it = vSrc.begin(); it != vSrc.end(); it++)
			{
				if (*it == vDel[i])
				{
					vSrc.erase(it);
					bRe = true;
					break;
				}
			}
		}
		return bRe;

	}
	static bool	 DelCards_Point(vector<ACard>&vSrc, vector<ACard> &vDel)
	{//从vSrc里删除vDel数据
		bool bRe = false;
		for (unsigned int i = 0; i<vDel.size(); i++)
		{
			for (vector<ACard>::iterator it = vSrc.begin(); it != vSrc.end(); it++)
			{
				if ((*it).n8CardNumber == vDel[i].n8CardNumber)
				{
					vSrc.erase(it);
					bRe = true;
					break;
				}
			}
		}
		return bRe;
	}
	static bool DelCards_Point(vector<ACard>&v, emPokeValue em)
	{//从v中删除指定的牌点
		bool bRe = false;
		for (vector<ACard>::iterator it = v.begin(); it != v.end();)
		{
			if ((*it).n8CardNumber == em)
			{
				it = v.erase(it);
				bRe = true;
			}
			else
			{
				it++;
				continue;
			}
		}
		return bRe;
	}
	bool operator ==(const ACard &card)
	{
		return n8CardColor == card.n8CardColor&&n8CardNumber == card.n8CardNumber;
	}
	bool operator >(const ACard &card)
	{
		if (n8CardNumber>card.n8CardNumber)
			return true;
		else if (n8CardNumber == card.n8CardNumber)
		{
			if (n8CardColor>card.n8CardColor)
				return true;
		}
		return false;
	}
	bool operator <(const ACard &card)
	{
		if (n8CardNumber<card.n8CardNumber)
			return true;
		else if (n8CardNumber == card.n8CardNumber)
		{
			if (n8CardColor<card.n8CardColor)
				return true;
		}
		return false;
	}
	static bool SortNumber(const ACard& a1, const ACard& a2)
	{//按牌点升序
		return a1.n8CardNumber<a2.n8CardNumber;

	}
	static void	SetOrderCards(vector<ACard>&card)
	{
		if (card.size()>1)
			sort(card.begin(), card.end(), ACard::SortNumber);
	}
	static void _Del_Card_Color(vector<ACard> &v, emPokeColor em)
	{//删除相同花色的所有牌	
		for (vector<ACard>::iterator it = v.begin(); it != v.end();)
		{
			ACard &info = *it;
			if (info.n8CardColor == em)
				it = v.erase(it);
			else
				it++;
		}
	}
	static map<int, int> TotalPaiNum(vector<ACard> &v)
	{//统计牌点数量
		map<int, int> m;
		for (unsigned int i = 0; i<v.size(); i++)
		{
			m[v[i].n8CardNumber] += 1;
		}
		return m;
	}
	static map<int, int> TotalPaiColor(vector<ACard>&v)
	{//统计牌色
		map<int, int> m;
		for (unsigned int i = 0; i<v.size(); i++)
		{
			m[v[i].n8CardColor] += 1;
		}
		return m;
	}
	static bool		IsSameColor(vector<ACard> &v)
	{//是否是相同花色
		if (v.size() <= 1) return false;
		return TotalPaiColor(v).size() == 1;
	}
	static bool	IsShunZi(vector<int>&v)
	{//检测是否是顺子
		if (v.size() <= 1)return false;
		sort(v.begin(), v.end());
		for (unsigned int i = 0; i<v.size() - 1; i++)
		{
			if (v[i] + 1 != v[i + 1])
				return false;
		}
		return true;

	}
	static void  Vec_Trans_Map(vector<ACard> &v, map<emPokeValue, int>&m)
	{
		for (unsigned int i = 0; i<v.size(); i++)
		{
			m[v[i].n8CardNumber]++;
		}
	}
};
