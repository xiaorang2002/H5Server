#pragma once

#include <vector>

#include "ServerUserItem.h"
#include "MjAlgorithm.h"

class CGamePlayer
{
public:
	CGamePlayer();
    CGamePlayer(shared_ptr<IServerUserItem> pServerUser);
    virtual ~CGamePlayer();
    //用户信息
    inline shared_ptr<IServerUserItem> GetServerUser() { return m_pServerUser; }
    //庄家标识
    inline void SetBanker(bool bIsBanker) { m_bIsBanker = bIsBanker; }
    inline bool IsBanker() { return m_bIsBanker; }
    //手牌
    inline vector<int32_t>& GetHandCards() { return m_handCards; }
    void AddHandCard(int32_t card);
    bool RemoveHandCards(int32_t card, uint32_t num = 1);
    //花牌
    inline vector<int32_t>& GetHuaCards() { return m_huaCards; }
    bool AddHuaCard(int32_t card);
    void SearchHuaCardsInHandCards(vector<int32_t>& huaCards);
    //出牌
    inline vector<int32_t>& GetOutCards() { return m_outCards; }
    bool OutCard(int32_t card);
    void RemoveOutCard(int32_t card);
    //吃碰杠
    inline vector<CPG>& GetCpgs() { return m_cpgs; }
	int32_t GetCpgIdx(vector<int32_t>& cards);
    bool Chi(vector<int32_t>& cards);
    bool Peng();
    bool Gang(vector<int32_t>& cards);
	bool ChangePengToJiaGang(int32_t targetCard);
    void ChangeJiaGangToPeng(int32_t card);
	void AddCpg(CPG& cpg);
    //听
    inline vector<TingOptInfo>& GetTingOptInfos() { return m_tingOptInfos; }
    inline vector<TingInfo>& GetTingInfos() { return m_tingInfos; }
	inline void SetTingOptInfos(vector<TingOptInfo>& tingOptInfos) { m_tingOptInfos = tingOptInfos; }
    bool Ting(int32_t outCard);
    inline bool IsTing() { return m_bTing; }
    //胡
    bool CanHu(int32_t targetCard);
    void Hu(int32_t targetCard);
    //选项
    inline vector<CPG>& GetOptionalCpgs() { return m_optionalCpgs; }
    int32_t GetOptionalCpgIdx(vector<int32_t>& cards) const;
    int32_t GetOptionalCpgIdxByType(MjType type) const;
    inline uint32_t GetOption() { return m_option; }
    inline bool HasOption() { return m_option != 0; }
    inline bool HasOptionType(MjType type) { return (m_option & static_cast<uint32_t>(type)); }
    inline void ResetOption() { m_option = 0; }
    void UpdateOption(int32_t targetCard, uint32_t targetChair, bool canGang, int32_t updateOption);
	inline void SetOption(int32_t option) { m_option = option;  }
	inline void SetOptionCpgs(vector<CPG>& optionCpgs) { m_optionalCpgs = optionCpgs; }
    //胡牌信息
    inline Compose& GetHuCompose() { return m_huCompose; }
    inline uint32_t GetHuType() { return m_huType; }
    inline void AddHuType(HuType type) { m_huType |= type; }
    inline void RemoveHuType(HuType type) { m_huType &= ~type; }
	inline void IncreaseAddHandCnt() { m_addCardCnt++;  }
    inline uint32_t GetAddCardCnt() { return m_addCardCnt; }
	//托管
	inline bool IsHosting() { return m_hosting; }
	inline void SetHosting(bool hosting) {
		m_hosting = hosting; m_timeoutCnt = 0;
	}
	inline int32_t GetTimeoutCnt() { return m_timeoutCnt; }
	void Timeout();
	void ClearTimeout();
	//胡牌概率
	inline const PrInfo& GetPrInfo() { return m_prInfo; }
	inline void SetPrInfo(PrInfo& prInfo) { m_prInfo = prInfo; }
	bool NeedCard(int32_t card);
	//调试
	string DebugString();

private:
    shared_ptr<IServerUserItem> m_pServerUser;
    vector<int32_t> m_handCards;
    vector<int32_t> m_outCards;
    vector<CPG> m_cpgs;
    vector<CPG> m_optionalCpgs;
    vector<TingOptInfo> m_tingOptInfos;
    vector<TingInfo> m_tingInfos;
    vector<int32_t> m_huaCards;
    uint32_t m_option;
    vector<ComposeCard> m_composeCards;
    Compose m_huCompose;
    bool m_bIsBanker;
    bool m_bTing;
    uint32_t m_huType;
    uint32_t m_addCardCnt; //摸牌次数
	bool m_hosting; //是否托管
	int32_t m_timeoutCnt; //超时次数
	PrInfo m_prInfo; //胡牌概率信息
};
