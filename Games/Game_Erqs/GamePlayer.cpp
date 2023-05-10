#include "GamePlayer.h"
#include <algorithm>
#include "CMD_Game.h"
#include <sstream>

CGamePlayer::CGamePlayer()
{
	m_huType = hu_invalid;
	m_option = 0;
	m_bIsBanker = false;
	m_bTing = false;
	m_addCardCnt = 0;
	m_hosting = false;
	m_timeoutCnt = 0;
}

CGamePlayer::CGamePlayer(shared_ptr<IServerUserItem> pServerUser)
    :m_pServerUser(pServerUser)
{
    m_huType = hu_invalid;
    m_option = 0;
    m_bIsBanker = false;
    m_bTing = false;
    m_addCardCnt = 0;
	m_hosting = false;
	m_timeoutCnt = 0;
}

CGamePlayer::~CGamePlayer()
{

}

bool CGamePlayer::Ting(int32_t outCard)
{
    if(m_bTing)
    {
        WARNLOG("");
        return false;
    }
    for(auto tingOptInfo : m_tingOptInfos)
    {
        if(tingOptInfo.outCard == outCard)
        {
            if(outCard != GAME_INVALID_CARD)
            {
                if(!OutCard(outCard))
                {
                    WARNLOG("");
                    return false;
                }
            }
            m_tingInfos = tingOptInfo.tingInfos;
            m_bTing = true;
            return true;
        }
    }
    WARNLOG("");
    return false;
}

bool CGamePlayer::AddHuaCard(int32_t card)
{
    if(getColor(card) != 4)
    {
        WARNLOG("");
        return false;
    }
    m_huaCards.push_back(card);
    return RemoveHandCards(card);
}

bool CGamePlayer::OutCard(int32_t card)
{
    if(m_bTing)
    {
        if(card != m_handCards.back())
        {
            WARNLOG("");
            return false;
        }
    }
    if(m_handCards.size()%3 != 2)
    {
        WARNLOG("");
        return false;
    }
    if(!RemoveHandCards(card))
    {
        WARNLOG("");
        return false;
    }
    m_outCards.push_back(card);
    return true;
}

int32_t CGamePlayer::GetCpgIdx(vector<int32_t>& cards)
{
	std::sort(cards.begin(), cards.end());
	for (uint32_t i = 0; i < m_cpgs.size(); ++i)
	{
		if (m_cpgs[i].cards.size() == cards.size())
		{
			vector<uint32_t> sortedCards(m_cpgs[i].cards.begin(), m_cpgs[i].cards.end());
			std::sort(sortedCards.begin(), sortedCards.end());
			bool equal = true;
			for (uint32_t j = 0; j < cards.size(); ++j)
			{
				if (cards[j] != sortedCards[j])
				{
					equal = false;
					break;
				}
			}
			if (equal)
			{
				return i;
			}
		}
	}
	return -1;
}

bool CGamePlayer::Chi(vector<int32_t>& cards)
{
    int32_t idx = GetOptionalCpgIdx(cards);
    if(idx == -1)
    {
        WARNLOG("");
        return false;
    }
    for (auto card : m_optionalCpgs[idx].cards)
    {
        if (card != m_optionalCpgs[idx].targetCard)
        {
            if(!RemoveHandCards(card))
            {
                WARNLOG("");
                return false;
            }
        }
    }
    m_cpgs.push_back(m_optionalCpgs[idx]);
    return true;
}

bool CGamePlayer::Peng()
{
    int32_t idx = GetOptionalCpgIdxByType(mj_peng);
    if(idx == -1)
    {
        WARNLOG("");
        return false;
    }
    if(!RemoveHandCards(m_optionalCpgs[idx].targetCard, 2))
    {
        WARNLOG("");
        return false;
    }
    m_cpgs.push_back(m_optionalCpgs[idx]);
    return true;
}

bool CGamePlayer::Gang(vector<int32_t>& cards)
{
	int32_t idx = GetOptionalCpgIdx(cards);
    if(idx == -1)
    {
        WARNLOG("");
        return false;
    }
    if(m_optionalCpgs[idx].type == mj_an_gang)
    {
		if (!RemoveHandCards(m_optionalCpgs[idx].targetCard, 4))
		{
			WARNLOG("");
			return false;
		}
		m_cpgs.push_back(m_optionalCpgs[idx]);
    }
    else if(m_optionalCpgs[idx].type == mj_dian_gang)
    {
		if (!RemoveHandCards(m_optionalCpgs[idx].targetCard, 3))
		{
			WARNLOG("");
			return false;
		}
		m_cpgs.push_back(m_optionalCpgs[idx]);
    }
    else if(m_optionalCpgs[idx].type == mj_jia_gang)
    {
		if (!RemoveHandCards(m_optionalCpgs[idx].targetCard, 1))
		{
			WARNLOG("");
			return false;
		}
		if (!ChangePengToJiaGang(m_optionalCpgs[idx].targetCard))
		{
			WARNLOG("");
			return false;
		}
    }
    else
    {
        WARNLOG("");
        return false;
    }
    return true;
}

void CGamePlayer::Hu(int32_t targetCard)
{
    MjInfo mjInfo = { m_cpgs, m_handCards, targetCard, m_huaCards.size(), m_huType,  m_tingInfos};
    findMaxCompose(mjInfo, m_composeCards, m_huCompose);
}

bool CGamePlayer::CanHu(int32_t targetCard)
{
    if (!canHu(m_handCards, targetCard, m_composeCards))
    {
        WARNLOG("");
        return false;
    }
    return true;
}

void CGamePlayer::AddHandCard(int32_t card)
{
    m_handCards.push_back(card);
}

bool CGamePlayer::RemoveHandCards(int32_t card, uint32_t num)
{
    uint32_t removedNum = 0;
    for (auto it = m_handCards.begin(); it != m_handCards.end(); )
    {
        if (*it == card)
        {
            it = m_handCards.erase(it);
            removedNum++;
            if(removedNum == num)
            {
                return true;
            }
        }
        else
        {
            ++it;
        }
    }
    return removedNum == num;
}

void CGamePlayer::RemoveOutCard(int32_t card)
{
    for(auto it=m_outCards.begin(); it!=m_outCards.end(); ++it)
    {
        if(*it == card)
        {
            m_outCards.erase(it);
            return;
        }
    }
}

void CGamePlayer::UpdateOption(int32_t targetCard, uint32_t targetChair, bool canGang, int32_t updateOption)
{
    MjInfo mjInfo = { m_cpgs, m_handCards, targetCard, m_huaCards.size(), m_huType, m_tingInfos };
    m_optionalCpgs.clear();
    findOptionalCpgs(mjInfo, m_optionalCpgs, updateOption);
    if(GAME_INVALID_CHAIR != targetChair)
    {
        //只能吃上家的牌
        for(auto it=m_optionalCpgs.begin(); it!=m_optionalCpgs.end(); )
        {
            uint32_t nextChair = NEXT_CHAIR(targetChair);
            if(isChiType(it->type) && m_pServerUser->GetChairId() != nextChair)
            {
                it = m_optionalCpgs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    //牌库不足不能杠
    if(!canGang)
    {
        for(auto it=m_optionalCpgs.begin(); it!=m_optionalCpgs.end(); )
        {
            if(isGangType(it->type))
            {
                it = m_optionalCpgs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    //听牌后的操作只有不影响听牌的杠
    if(m_bTing)
    {
        updateCpgsAfterTing(mjInfo, m_optionalCpgs);
    }
    for(auto& optionalCpg : m_optionalCpgs)
    {
        optionalCpg.targetChair = targetChair;
    }

    m_option = 0;
	for (auto cpg : m_optionalCpgs)
	{
		m_option |= cpg.type;
	}
    if(updateOption & mj_hu)
    {
        vector<ComposeCard> composeCards;
        if (canHu(m_handCards, targetCard, composeCards))
        {
            m_option |= mj_hu;
        }
    }
	m_tingOptInfos.clear();
    if(!m_bTing && (updateOption & mj_ting))
    {
        findTingOptInfos(mjInfo, m_tingOptInfos);
        if(m_tingOptInfos.size() > 0)
        {
            m_option |= mj_ting;
        }
    }
}

int32_t CGamePlayer::GetOptionalCpgIdx(vector<int32_t> &cards) const
{
    std::sort(cards.begin(),cards.end());
    for(uint32_t i=0; i<m_optionalCpgs.size(); ++i)
    {
        if(m_optionalCpgs[i].cards.size() == cards.size())
        {
            vector<uint32_t> sortedCards(m_optionalCpgs[i].cards.begin(), m_optionalCpgs[i].cards.end());
            std::sort(sortedCards.begin(), sortedCards.end());
            bool equal = true;
            for(uint32_t j=0; j<cards.size(); ++j)
            {
                if(cards[j] != sortedCards[j])
                {
                    equal = false;
                    break;
                }
            }
            if(equal)
            {
                return i;
            }
        }
    }
    return -1;
}

int32_t CGamePlayer::GetOptionalCpgIdxByType(MjType type) const
{
    for(uint32_t i=0; i<m_optionalCpgs.size(); ++i)
    {
        if(m_optionalCpgs[i].type == type)
        {
            return i;
        }
    }
    return -1;
}

void CGamePlayer::SearchHuaCardsInHandCards(vector<int32_t>& huaCards)
{
    for(auto card : m_handCards)
    {
        if(getColor(card) == 4)
        {
            huaCards.push_back(card);
        }
    }
}

bool CGamePlayer::ChangePengToJiaGang(int32_t targetCard)
{
	for (auto& cpg : m_cpgs)
	{
		if (cpg.targetCard == targetCard && cpg.type == mj_peng)
		{
			cpg.type = mj_jia_gang;
			cpg.cards = { targetCard, targetCard, targetCard, targetCard };
			return true;
		}
	}
	return false;
}

void CGamePlayer::ChangeJiaGangToPeng(int32_t card)
{
    for(auto& cpg : m_cpgs)
    {
        if(cpg.targetCard == card && cpg.type == mj_jia_gang)
        {
            cpg.type == mj_peng;
            cpg.cards = { card, card, card };
        }
    }
}

void CGamePlayer::AddCpg(CPG& cpg)
{
	m_cpgs.push_back(cpg);
}

void CGamePlayer::Timeout()
{
	m_timeoutCnt++;
	if (m_timeoutCnt >= 2)
	{
		m_hosting = true;
	}
}

void CGamePlayer::ClearTimeout()
{
	m_timeoutCnt = 0;
	m_hosting = false;
}

bool CGamePlayer::NeedCard(int32_t card)
{
	if (m_bTing)
	{
		for (auto tingInfo : m_tingInfos)
		{
			if (card == tingInfo.card)
			{
				return true;
			}
		}
	}
	else
	{
		for (auto needCard : m_prInfo.needCards)
		{
			if (card == needCard)
			{
				return true;
			}
		}
	}
	return false;
}

string CGamePlayer::DebugString()
{
	stringstream ss;
	ss << endl;
	ss << "座位 " << m_pServerUser->GetChairId() << endl;
	//手牌
	ss << "手牌" << endl;
	vector<int32_t> handCards;
	if (m_handCards.size()%3 == 2)
	{
		handCards.insert(handCards.begin(), m_handCards.begin(), m_handCards.end() - 1);
		sort(handCards.begin(), handCards.end());
		handCards.push_back(m_handCards.back());
	}
	else
	{
		handCards.insert(handCards.begin(), m_handCards.begin(), m_handCards.end());
		sort(handCards.begin(), handCards.end());
	}
	ss << "[ ";
	for (auto card : handCards)
	{
		ss << hex << card << " ";
	}
	ss << "]" << endl;
	ss << "[ ";
	for (auto card : handCards)
	{
		ss << getCardStr(card) << " ";
	}
	ss << "]" << endl;
	//出牌
	ss << "出牌" << endl;
	ss << "[ ";
	for (auto card : m_outCards)
	{
		ss << hex << card << " ";
	}
	ss << "]" << endl;
	ss << "[ ";
	for (auto card : m_outCards)
	{
		ss << getCardStr(card) << " ";
	}
	ss << "]" << endl;
	//吃碰杠
	ss << "吃碰杠" << endl;
	for (auto cpg : m_cpgs)
	{
		ss << "[ ";
		for (auto card : cpg.cards)
		{
			ss << hex << card << " ";
		}
		ss << "]" << endl;
		ss << "[ ";
		for (auto card : cpg.cards)
		{
			ss << getCardStr(card) << " ";
		}
		ss << "]" << endl;
	}
	return ss.str();
}
