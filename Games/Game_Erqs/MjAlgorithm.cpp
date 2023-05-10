#include "MjAlgorithm.h"
#include "CPart.h"
#include "CTableMgr.h"
#include <cmath>
#include <ctime>
#include <iostream>
#include <sstream>

using namespace std;

// 万 筒 条
const std::string CARDS_STR[] = {
    u8"none",u8"1万",u8"2万",u8"3万",u8"4万",u8"5万",u8"6万",u8"7万",u8"8万",u8"9万",

    u8"1条",u8"2条",u8"3条",u8"4条",u8"5条",u8"6条",u8"7条",u8"8条",u8"9条",

    u8"1筒",u8"2筒",u8"3筒",u8"4筒",u8"5筒",u8"6筒",u8"7筒",u8"8筒",u8"9筒",

    u8"东",u8"南",u8"西",u8"北",u8"中",u8"发",u8"白",

    u8"春",u8"夏",u8"秋",u8"冬",u8"梅",u8"兰",u8"竹",u8"菊"
};

const std::string CARAD_COLORS[] = {
    u8"none",u8"万",u8"筒",u8"条",u8"字",u8"花"
};

// 万 筒 条
const int32_t CARDS[] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09, // 1-9万
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19, // 1-9条
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29, // 1-9筒
    0x31,0x32,0x33,0x34,0x35,0x36,0x37, // 东南西北中发白
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48  //春夏秋冬 梅兰竹菊
};

const array<int32_t, TABLE_SIZE> TABLE_JIULIANBAODENG = {
    0,
    3,1,1,1,1,1,1,1,3,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

struct HuisuItem
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    bool eye;
};

HuisuItem huisu_items1[] = {
    {1,1,1,false}
};

HuisuItem huisu_items2[] = {
    { 2,0,0,true},
    { 2,2,2,false}
};

HuisuItem huisu_items3[] = {
    { 3,0,0,false},
    { 3,1,1,true},
    { 3,3,3,false}
};

HuisuItem huisu_items4[] = {
    { 4,1,1,false},
    { 4,2,2,true},
    { 4,4,4,false}
};

bool isValidHandSum(int32_t sum)
{
    int32_t sums[] = { 2, 5, 8, 11, 14 };
    for (auto item : sums)
    {
        if (item == sum)
        {
            return true;
        }
    }
    return false;
}

uint32_t getCardIdx(int32_t card)
{
    if (card >= 0x01 && card <= 0x09)
    {
        return card;
    }
    else if (card >= 0x11 && card <= 0x19)
    {
        return card - 0x10 + 9;
    }
    else if (card >= 0x21 && card <= 0x29)
    {
        return card - 0x20 + 18;
    }
    else if (card >= 0x31 && card <= 0x37)
    {
        return card - 0x30 + 27;
    }
    else if (card >= 0x41 && card <= 0x48)
    {
        return card - 0x40 + 27 + 7;
    }
    return 0;
}

int32_t getCardByIdx(uint32_t idx)
{
    if (idx >= 1 && idx <= 9)
    {
        return idx;
    }
    else if (idx >= 10 && idx <= 18)
    {
        return 0x10 + idx - 9;
    }
    else if (idx >= 19 && idx <= 27)
    {
        return 0x20 + idx - 18;
    }
    else if (idx >= 28 && idx <= 34)
    {
        return 0x30 + idx - 27;
    }
    else if (idx >= 35 && idx <= 42)
    {
        return 0x40 + idx - 34;
    }
    return 0;
}

std::string getCardStr(int32_t card)
{
    return CARDS_STR[getCardIdx(card)];
}

uint32_t getColor(int32_t card)
{
    return (card & 0xf0) >> 4;
}

std::string getColorStr(int32_t card)
{
    return CARAD_COLORS[getColor(card)];
}

int32_t getCardValue(int32_t card)
{
    return card & 0x0f;
}

bool isChiType(MjType type)
{
    return (type == mj_left_chi || type == mj_center_chi || type == mj_right_chi);
}

bool isGangType(MjType type)
{
    return (type == mj_an_gang || type == mj_dian_gang || type == mj_jia_gang);
}

void cardsToTable(const vector<int32_t> & cards, array<int32_t, TABLE_SIZE> & table) {
    for (auto card : cards)
    {
        table[getCardIdx(card)]++;
    }
}

void tableToCards(const array<int32_t, TABLE_SIZE> & table, vector<int32_t> & cards)
{
    for (int32_t i = 1; i < TABLE_SIZE; ++i)
    {
        for (int32_t k = 0; k < table[i]; ++k)
        {
            cards.push_back(getCardByIdx(i));
        }
    }
}

vector<CPG>& findChi(const vector<int32_t> & cards, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    findChiInTable(table, targetCard, optionalCpgs);
    return optionalCpgs;
}

vector<CPG>& findChiInTable(array<int32_t, TABLE_SIZE> & table, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    if (GAME_INVALID_CARD == targetCard)
    {
        return optionalCpgs;
    }
    if (targetCard > 0x29)
    {
        return optionalCpgs;
    }
    uint32_t targetIdx = getCardIdx(targetCard);
    uint32_t targetValue = getCardValue(targetCard);
    //牌值7以下才有左吃
    if (targetValue <= 7 && table[targetIdx + 1] > 0 && table[targetIdx + 2] > 0) {
        CPG cpg;
        cpg.type = mj_left_chi;
        cpg.cards = vector<int32_t>();
        cpg.cards.push_back(targetCard);
        cpg.cards.push_back(targetCard + 1);
        cpg.cards.push_back(targetCard + 2);
        cpg.targetCard = targetCard;
        optionalCpgs.push_back(cpg);
    }
    //牌值不等1和9才有中吃
    if (targetValue != 1 && targetValue != 9 && table[targetIdx - 1] > 0 && table[targetIdx + 1] > 0) {
        CPG cpg;
        cpg.type = mj_center_chi;
        cpg.cards = vector<int32_t>();
        cpg.cards.push_back(targetCard - 1);
        cpg.cards.push_back(targetCard);
        cpg.cards.push_back(targetCard + 1);
        cpg.targetCard = targetCard;
        optionalCpgs.push_back(cpg);
    }
    //牌值3以上才有右吃
    if (targetValue >= 3 && table[targetIdx - 1] > 0 && table[targetIdx - 2] > 0) {
        CPG cpg;
        cpg.type = mj_right_chi;
        cpg.cards = vector<int32_t>();
        cpg.cards.push_back(targetCard - 2);
        cpg.cards.push_back(targetCard - 1);
        cpg.cards.push_back(targetCard);
        cpg.targetCard = targetCard;
        optionalCpgs.push_back(cpg);
    }
    return optionalCpgs;
}

vector<CPG>& findPeng(const vector<int32_t> & cards, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    findPengInTable(table, targetCard, optionalCpgs);
    return optionalCpgs;
}

vector<CPG>& findPengInTable(array<int32_t, TABLE_SIZE> & table, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    if (GAME_INVALID_CARD == targetCard)
    {
        return optionalCpgs;
    }
    uint32_t targetIdx = getCardIdx(targetCard);
    if (table[targetIdx] >= 2 && table[targetIdx] <= 3)
    {
        CPG cpg;
        cpg.type = mj_peng;
        cpg.cards = vector<int32_t>(3, targetCard);
        cpg.targetCard = targetCard;
        optionalCpgs.push_back(cpg);
    }
    return optionalCpgs;
}

vector<CPG>& findAnGang(const vector<int32_t> & cards, vector<CPG> & optionalCpgs)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    findAnGangInTable(table, optionalCpgs);
    return optionalCpgs;
}

vector<CPG>& findAnGangInTable(array<int32_t, TABLE_SIZE> & table, vector<CPG> & optionalCpgs)
{
    for (int i = 1; i < TABLE_SIZE; ++i)
    {
        if (table[i] == 4)
        {
            CPG cpg;
            cpg.type = mj_an_gang;
            cpg.cards = vector<int32_t>(4, getCardByIdx(i));
            cpg.targetCard = getCardByIdx(i);
            optionalCpgs.push_back(cpg);
        }
    }
    return optionalCpgs;
}

vector<CPG>& findDianGang(const vector<int32_t> & cards, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    findDianGangInTable(table, targetCard, optionalCpgs);
    return optionalCpgs;
}

vector<CPG>& findDianGangInTable(array<int32_t, TABLE_SIZE> & table, int32_t targetCard, vector<CPG> & optionalCpgs)
{
    if (GAME_INVALID_CARD == targetCard)
    {
        return optionalCpgs;
    }
    uint32_t targetIdx = getCardIdx(targetCard);
    if (table[targetIdx] == 3)
    {
        CPG cpg;
        cpg.type = mj_dian_gang;
        cpg.cards = vector<int32_t>(4, targetCard);
        cpg.targetCard = targetCard;
        optionalCpgs.push_back(cpg);
    }
    return optionalCpgs;
}

vector<CPG>& findJiaGang(const vector<int32_t> & cards, const vector<CPG> & cpgs, vector<CPG> & optionalCpgs)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    findJiaGangInTable(table, cpgs, optionalCpgs);
    return optionalCpgs;
}

vector<CPG>& findJiaGangInTable(array<int32_t, TABLE_SIZE> & table, const vector<CPG> & cpgs, vector<CPG> & optionalCpgs)
{
    for (auto cpg : cpgs)
    {
        if (cpg.type == mj_peng)
        {
            uint32_t targetIdx = getCardIdx(cpg.targetCard);
            if (table[targetIdx] == 1)
            {
                CPG optionalCpg;
                optionalCpg.type = mj_jia_gang;
                optionalCpg.cards = vector<int32_t>(4, cpg.targetCard);
                optionalCpg.targetCard = cpg.targetCard;
                optionalCpgs.push_back(optionalCpg);
            }
        }
    }
    return optionalCpgs;
}

vector<CPG>& findOptionalCpgs(MjInfo & mjInfo, vector<CPG> & optionalCpgs, uint32_t findOption)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (GAME_INVALID_CARD != mjInfo.targetCard)
    {
        if (findOption & mj_left_chi || findOption & mj_center_chi || findOption & mj_right_chi)
        {
            findChiInTable(table, mjInfo.targetCard, optionalCpgs);
        }
        if (findOption & mj_peng)
        {
            findPengInTable(table, mjInfo.targetCard, optionalCpgs);
        }
        if (findOption & mj_dian_gang)
        {
            findDianGangInTable(table, mjInfo.targetCard, optionalCpgs);
        }
    }
    if (findOption & mj_an_gang)
    {
        findAnGangInTable(table, optionalCpgs);
    }
    if (findOption & mj_jia_gang)
    {
        findJiaGangInTable(table, mjInfo.cpgs, optionalCpgs);
    }
    return optionalCpgs;
}

void updateCpgsAfterTing(MjInfo & mjInfo, vector<CPG> & optionalCpgs)
{
    for (auto itr = optionalCpgs.begin(); itr != optionalCpgs.end(); )
    {
        if (isChiType(itr->type) || itr->type == mj_peng)
        {
            optionalCpgs.erase(itr);
        }
        else if (itr->type == mj_jia_gang)
        {
            itr++;
        }
        else //点杠 暗杠
        {
            vector<int32_t> tempCards;
            for (auto card : mjInfo.cards)
            {
                if (card != itr->targetCard)
                {
                    tempCards.push_back(card);
                }
            }
            MjInfo tempMjInfo = { mjInfo.cpgs, tempCards, mjInfo.targetCard, mjInfo.huaNum, mjInfo.huType, mjInfo.tingInfos };
            vector<TingOptInfo> tempTingInfos;
            findTingOptInfos(tempMjInfo, tempTingInfos);
            if (tempTingInfos.size() == 0)
            {
                optionalCpgs.erase(itr);
                continue;
            }
            TingOptInfo tempTingInfo = tempTingInfos[0];
            if (tempTingInfo.tingInfos.size() != mjInfo.tingInfos.size())
            {
                optionalCpgs.erase(itr);
                continue;
            }
            bool isSame = true;
            for (uint32_t i = 0; i < mjInfo.tingInfos.size(); ++i)
            {
                if (tempTingInfo.tingInfos[i].card != mjInfo.tingInfos[i].card)
                {
                    isSame = false;
                    break;
                }
            }
            if (!isSame)
            {
                optionalCpgs.erase(itr);
            }
            else
            {
                itr++;
            }
        }
    }
}

HuisuItem* get_huisu_ops(uint32_t cur_num, uint32_t * op_num)
{
    *op_num = 0;
    if (cur_num == 1) {
        *op_num = 1;
        return huisu_items1;
    }
    if (cur_num == 2) {
        *op_num = 2;
        return huisu_items2;
    }
    if (cur_num == 3) {
        *op_num = 3;
        return huisu_items3;
    }
    if (cur_num == 4) {
        *op_num = 3;
        return huisu_items4;
    }
}

// 字牌是否满足胡牌条件
bool check_zi(const array<int32_t, TABLE_SIZE> & table, bool& eye, ComposeCard & composeCard)
{
    for (uint32_t i = 28; i <= 34; ++i) {
        if (table[i] == 0) continue;

        if (table[i] == 1 || table[i] == 4) return false;

        if (eye && table[i] == 2) return false;

        if (table[i] == 2)
        {
            eye = true;
            composeCard.eye[0] = getCardByIdx(i);
            composeCard.eye[1] = getCardByIdx(i);
        }
        if (table[i] == 3)
        {
            ThreeCard three;
            int32_t card = getCardByIdx(i);
            three.cards = { card, card, card };
            three.type = ThreeType::repeat;
            composeCard.threes.push_back(three);
        }
    }

    return true;
}

void split_color(string curPath, vector<string> & paths, uint32_t idx, uint32_t offset, array<int32_t, TABLE_SIZE> & table, ComposeCardMap & composeCardMap)
{
    uint32_t cur = offset + idx;
    uint32_t end = offset + 9;
    if (cur >= end)
    {
        paths.push_back(curPath);
        return;
    }
    uint32_t n = table[cur];
    if (n == 0)
    {
        split_color(curPath, paths, idx + 1, offset, table, composeCardMap);
        return;
    }
    // 获取所有可拆解情况
    uint32_t op_num = 0;
    HuisuItem* p = NULL;
    p = get_huisu_ops(n, &op_num);
    for (uint32_t i = 0; i < op_num; ++i)
    {
        HuisuItem& pi = p[i];
        if (cur + 1 >= end && pi.b > 0) continue;
        if (cur + 2 >= end && pi.c > 0) continue;
        if (pi.b > 0 && pi.b > table[cur + 1] || pi.c > 0 && table[cur + 2] < pi.c) continue;

        table[cur] = 0;
        table[cur + 1] -= pi.b;
        table[cur + 2] -= pi.c;
        //保存组合牌
        int32_t repeat = pi.a;
        ComposeCard composeCard;
        if (pi.eye)
        {
            composeCard.eye[0] = composeCard.eye[1] = getCardByIdx(cur);
            repeat = pi.a - 2;
        }
        repeat = repeat - pi.b;
        if (repeat == 3)
        {
            ThreeCard three;
            int32_t card = getCardByIdx(cur);
            three.cards = { card, card, card };
            three.type = ThreeType::repeat;
            composeCard.threes.push_back(three);
        }
        for (int j = 0; j < pi.b; ++j)
        {
            ThreeCard three;
            int32_t card = getCardByIdx(cur);
            three.cards = { card, card + 1, card + 2 };
            three.type = ThreeType::straight;
            composeCard.threes.push_back(three);
        }
        string path;
        if (curPath == "")
        {
            path = to_string((idx + 1) * 10 + i);
        }
        else
        {
            path = curPath + "-" + to_string((idx + 1) * 10 + i);
        }
        composeCardMap[path] = composeCard;
        split_color(path, paths, idx + 1, offset, table, composeCardMap);
        table[cur] = p[i].a;
        table[cur + 1] += pi.b;
        table[cur + 2] += pi.c;
    }
}

//使用路径搜索组合
void searchComposeCards(ComposeCardMap & composeCardMap, vector<string> & paths, vector<ComposeCard> & composeCards)
{
    for (auto path : paths)
    {
        ComposeCard composeCard;
        bool eye = false;
        bool pathValid = true;
        for (uint32_t i = 2; i <= path.length(); i += 3)
        {
            string key = path.substr(0, i);
            auto itr = composeCardMap.find(key);
            if (itr == composeCardMap.end())
            {
                pathValid = false;
                break;
            }
            if (itr->second.eye[0] != GAME_INVALID_CARD && eye)
            {
                pathValid = false;
                break;
            }
            composeCard.threes.insert(composeCard.threes.begin(), itr->second.threes.begin(), itr->second.threes.end());
            if (itr->second.eye[0] != GAME_INVALID_CARD)
            {
                composeCard.eye[0] = composeCard.eye[1] = itr->second.eye[0];
                eye = true;
            }
        }
        if (pathValid)
        {
            composeCards.push_back(composeCard);
        }
    }
}

bool check_color(vector<string> & paths, uint32_t offset, array<int32_t, TABLE_SIZE> & table, ComposeCardMap & composeCardMap, vector<ComposeCard> & composeCards)
{
    uint32_t start = offset;
    uint32_t end = offset + 9;
    uint32_t sum = 0;
    for (uint32_t i = start; i < end; i++) {
        sum += table[i];
    }
    if (sum == 0) return true;
    uint32_t yu = sum % 3;
    if (yu == 1) return false;

    split_color("", paths, 0, offset, table, composeCardMap);
    searchComposeCards(composeCardMap, paths, composeCards);
    return !composeCards.empty();
}

bool mergeComposeCards(ComposeCard & composeZi, array<vector<ComposeCard>, 3> & composeColors, vector<ComposeCard> & composeCards)
{

    auto compare = [&](vector<ComposeCard> a, vector<ComposeCard> b)
    {
        return a.size() > b.size();
    };
    sort(composeColors.begin(), composeColors.end(), compare);
    auto mergeComposeCard = [&](ComposeCard & des, ComposeCard & src)
    {
        if (des.eye[0] != GAME_INVALID_CARD && src.eye[0] != GAME_INVALID_CARD)
        {
            return false;
        }
        des.threes.insert(des.threes.begin(), src.threes.begin(), src.threes.end());
        if (src.eye[0] != GAME_INVALID_CARD)
        {
            des.eye[0] = des.eye[1] = src.eye[0];
        }
        return true;
    };
    for (auto composeCard1 : composeColors[0])
    {
        ComposeCard composeCard = composeZi;
        bool mergeResult = true;
        mergeResult = mergeComposeCard(composeCard, composeCard1);
        if (!mergeResult)
        {
            continue;
        }
        for (auto composeCard2 : composeColors[1])
        {
            mergeResult = mergeComposeCard(composeCard, composeCard2);
            if (!mergeResult)
            {
                continue;
            }
            for (auto composeCard3 : composeColors[2])
            {
                mergeResult = mergeComposeCard(composeCard, composeCard3);
                if (!mergeResult)
                {
                    continue;
                }
            }
        }
        if (mergeResult)
        {
            composeCards.push_back(composeCard);
        }
    }
    if (composeColors[0].size() == 0)
    {
        composeCards.push_back(composeZi);
    }
    return !composeCards.empty();
}

bool canHu(const vector<int32_t> & cards, int32_t targetCard, vector<ComposeCard> & composeCards) {
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cards, table);
    if (targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(targetCard)]++;
    }
    return canHu(table, composeCards);
}

bool canHu(array<int32_t, TABLE_SIZE> & table, vector<ComposeCard> & composeCards)
{
    // 有花牌不能胡
    for (uint32_t i = 35; i <= 42; ++i) {
        if (table[i] != 0) return false;
    }
    bool qidui = false;
    //检查七对
    if (isTableQiDui(table))
    {
        qidui = true;
    }
    bool eye = false;
    // 检查东西南北中发白
    ComposeCard composeZi;
    if (!check_zi(table, eye, composeZi)) return qidui;
    //检查万
    ComposeCardMap composeCardMap;
    vector<string> paths;
    array<vector<ComposeCard>, 3> composeColors;
    if (!check_color(paths, 1, table, composeCardMap, composeColors[0])) return qidui;
    //检查条
    composeCardMap.clear();
    paths.clear();
    if (!check_color(paths, 10, table, composeCardMap, composeColors[1])) return qidui;
    //检查筒
    composeCardMap.clear();
    paths.clear();
    if (!check_color(paths, 19, table, composeCardMap, composeColors[2])) return qidui;
    //合并所有组合
    composeCards.clear();
    if (!mergeComposeCards(composeZi, composeColors, composeCards)) return qidui;
    return true;
}

vector<TingOptInfo>& findTingOptInfos(MjInfo & mjInfo, vector<TingOptInfo> & tingOptInfos)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    auto searchTingInfo = [&](uint32_t outCard)
    {
        TingOptInfo tingOptInfo;
        tingOptInfo.outCard = outCard;
        vector<ComposeCard> composeCards;
        for (uint32_t j = 1; j <= 34; ++j)
        {
            table[j]++;
            bool hu = canHu(table, composeCards);
            table[j]--;
            if (hu)
            {
                int32_t targetCard = getCardByIdx(j);
                Compose compose;
                findMaxCompose(mjInfo, composeCards, compose);
                TingInfo tingInfo;
                tingInfo.card = targetCard;
                tingInfo.num = 0;
                tingInfo.compose = compose;
                tingOptInfo.tingInfos.push_back(tingInfo);
            }
        }
        if (tingOptInfo.tingInfos.size() > 0)
        {
            tingOptInfos.push_back(tingOptInfo);
        }
    };
    if (mjInfo.cards.size() % 3 == 1)
    {
        searchTingInfo(GAME_INVALID_CARD);
    }
    else
    {
        for (uint32_t i = 0; i < TABLE_SIZE; ++i)
        {
            if (table[i] > 0)
            {
                table[i]--;
                searchTingInfo(getCardByIdx(i));
                table[i]++;
            }
        }
    }
    return tingOptInfos;
}

Compose& findMaxCompose(MjInfo & mjInfo, const vector<ComposeCard> & composeCards, Compose & maxCompose)
{
    //七对有可能无组合牌
    vector<vector<ComposeType>> allComposeTypes;
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }



    for (auto composeCard : composeCards)
    {
        array<bool, max_compose_type_size> checkSwitches;
        checkSwitches.fill(true);
        allComposeTypes.push_back(vector<ComposeType>());
        uint32_t idx = allComposeTypes.size() - 1;
        for (auto composeTypeChecker : composeTypeCheckers)
        {
            if(composeTypeChecker.checkInfo.mutuallytype==sansanyi3n2||composeTypeChecker.checkInfo.mutuallytype==otherhu000)
            {
                if (checkSwitches[composeTypeChecker.checkInfo.type])
                {
                    uint32_t num = composeTypeChecker.checkFunc(mjInfo, composeCard);
                    for (uint32_t i = 0; i < num; ++i)
                    {
                        allComposeTypes[idx].push_back(composeTypeChecker.checkInfo.type);
                        for (auto mutex_type : composeTypeChecker.checkInfo.mutexes)
                        {
                            checkSwitches[mutex_type] = false;
                        }
                    }
                }
            }

        }
        CalculateBFLL(mjInfo, composeCard, allComposeTypes[idx]);

        if(isTableQiDui(table))
        {
            array<bool, max_compose_type_size> checkSwitches;
            checkSwitches.fill(true);
            allComposeTypes.push_back(vector<ComposeType>());
            uint32_t idx = allComposeTypes.size() - 1;
            for (auto composeTypeChecker : composeTypeCheckers)
            {
                if(composeTypeChecker.checkInfo.mutuallytype==qiduizi222||composeTypeChecker.checkInfo.mutuallytype==otherhu000)
                {
                    if (checkSwitches[composeTypeChecker.checkInfo.type])
                    {
                        uint32_t num = composeTypeChecker.checkFunc(mjInfo, maxCompose.composeCard);
                        for (uint32_t i = 0; i < num; ++i)
                        {
                            allComposeTypes[idx].push_back(composeTypeChecker.checkInfo.type);
                            for (auto mutex_type : composeTypeChecker.checkInfo.mutexes)
                            {
                                checkSwitches[mutex_type] = false;
                            }
                        }
                    }
                }

            }
        }

        //计算最高分值得牌型
        uint32_t maxTimes = 0;
        uint32_t maxIdx = 0;
        for (uint32_t i = 0; i < allComposeTypes.size(); ++i)
        {
            uint32_t times = getTimes(mjInfo.huaNum, allComposeTypes[i]);
            if (times > maxTimes)
            {
                maxTimes = times;
                maxIdx = i;
            }
        }
        if(!isTableQiDui(table))
        {
            maxCompose.composeCard = composeCards[maxIdx];
        }
        maxCompose.types = allComposeTypes[maxIdx];
        maxCompose.times = maxTimes;


    }
    return maxCompose;
}

void addComposeType(vector<ComposeType> & composeTypes, ComposeType composeType)
{
    bool exist = false;
    for (auto type : composeTypes)
    {
        if (type == composeType)
        {
            exist = true;
            break;
        }
    }
    if (!exist)
    {
        composeTypes.push_back(composeType);
    }
}

uint32_t getComposeTypeCheckerIdx(ComposeType type)
{
    for (uint32_t i = 0; i < composeTypeCheckers.size(); ++i)
    {
        if (type == composeTypeCheckers[i].checkInfo.type)
        {
            return i;
        }
    }
    return -1;
}

vector<vector<int32_t>>& combination(vector<vector<int32_t>> & idxCombinations, const uint32_t choose, const uint32_t from)
{
    vector<string> strs;
    combination(strs, choose, from);
    for (uint32_t i = 0; i != strs.size(); ++i) {
        vector<int32_t> idxCombination;
        for (uint32_t j = 0; j != from; ++j) {
            if (strs[i][j] == '1') {
                idxCombination.push_back(j);
            }
        }
        idxCombinations.push_back(idxCombination);
    }
    return idxCombinations;
}

vector<string>& combination(vector<string> & strs, const uint32_t & choose, const uint32_t & from)
{
    string wk = string(choose, '1') + string(from - choose, '0');
    strs.push_back(wk);
    size_t found = string::npos;
    while ((found = wk.find("10")) != string::npos) {
        // 1. swap found
        wk[found] ^= wk[found + 1];
        wk[found + 1] ^= wk[found];
        wk[found] ^= wk[found + 1];
        // 2. sort before
        sort(wk.begin(), wk.begin() + found, [](const char& lhs, const char& rhs) { return lhs > rhs; });
        strs.push_back(wk);
    }
    return strs;
}

bool isRepeatThree(const ThreeCard & three, int32_t card)
{
    return (three.type == ThreeType::repeat && three.cards[0] == card);
}

bool isRepeatThree(const ThreeCard & three, int32_t card_start, int32_t card_end)
{
    if (three.type != ThreeType::repeat)
    {
        return false;
    }
    if (three.cards[0] >= card_start && three.cards[0] <= card_end)
    {
        return true;
    }
    return false;
}

bool isYao9Card(int32_t card)
{
    if (isYaoCard(card))
    {
        return true;
    }
    return getCardValue(card) == 9;
}

bool isYaoCard(int32_t card)
{
    if (card >= 0x31 && card <= 0x37)
    {
        return true;
    }
    return getCardValue(card) == 1;
}

bool isYao9Three(const ThreeCard & three)
{
    for (auto card : three.cards)
    {
        if (isYao9Card(card))
        {
            return true;
        }
    }
    return false;
}

bool isYaoThree(const ThreeCard & three)
{
    for (auto card : three.cards)
    {
        if (isYaoCard(card))
        {
            return true;
        }
    }
    return false;
}

vector<ThreeCard>& findRepeat(const vector<CPG> & cpgs, const ComposeCard & composeCard, int32_t card_start, int32_t card_end, vector<ThreeCard> & repeatThrees)
{
    for (auto cpg : cpgs)
    {
        if (isChiType(cpg.type))
        {
            continue;
        }
        if (cpg.cards[0] >= card_start && cpg.cards[0] <= card_end)
        {
            ThreeCard three;
            three.cards = { cpg.cards[0], cpg.cards[1], cpg.cards[2] };
            three.type = ThreeType::repeat;
            repeatThrees.push_back(three);
        }
    }
    for (auto three : composeCard.threes)
    {
        if (isRepeatThree(three, card_start, card_end))
        {
            repeatThrees.push_back(three);
        }
    }
    return repeatThrees;
}

vector<ThreeCard>& findCloseRepeat(const vector<CPG> & cpgs, const ComposeCard & composeCard, int32_t card_start, int32_t card_end, vector<ThreeCard> & repeatThrees)
{
    for (auto cpg : cpgs)
    {
        if (cpg.type == mj_an_gang)
        {
            if (cpg.cards[0] >= card_start && cpg.cards[0] <= card_end)
            {
                ThreeCard three;
                three.cards = { cpg.cards[0], cpg.cards[1], cpg.cards[2] };
                three.type = ThreeType::repeat;
                repeatThrees.push_back(three);
            }
        }
    }
    for (auto three : composeCard.threes)
    {
        if (isRepeatThree(three, card_start, card_end))
        {
            repeatThrees.push_back(three);
        }
    }
    return repeatThrees;
}

vector<ThreeCard>& findStraight(const vector<CPG> & cpgs, const ComposeCard & composeCard, vector<ThreeCard> & straightThrees)
{
    for (auto cpg : cpgs)
    {
        if (isChiType(cpg.type))
        {
            ThreeCard three;
            three.cards = { cpg.cards[0], cpg.cards[1], cpg.cards[2] };
            three.type = ThreeType::straight;
            straightThrees.push_back(three);
        }
    }
    for (auto three : composeCard.threes)
    {
        if (three.type == ThreeType::straight)
        {
            straightThrees.push_back(three);
        }
    }
    return straightThrees;
}

vector<ThreeCard>& findThree(const vector<CPG> & cpgs, const ComposeCard & composeCard, vector<ThreeCard> & threes)
{
    for (auto cpg : cpgs)
    {
        ThreeCard three;
        three.cards = { cpg.cards[0],cpg.cards[1],cpg.cards[2] };
        if (isChiType(cpg.type))
        {
            three.type = ThreeType::straight;
        }
        else
        {
            three.type = ThreeType::repeat;
        }
        threes.push_back(three);
    }
    for (auto three : composeCard.threes)
    {
        threes.push_back(three);
    }
    return threes;
}

vector<int32_t>& findAllCards(MjInfo & mjInfo, vector<int32_t> & allCards)
{
    for (auto cpg : mjInfo.cpgs)
    {
        for (auto card : cpg.cards)
        {
            allCards.push_back(card);
        }
    }
    for (auto card : mjInfo.cards)
    {
        allCards.push_back(card);
    }
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        allCards.push_back(mjInfo.targetCard);
    }
    return allCards;
}

uint32_t getGangNum(const vector<CPG> & cpgs)
{
    uint32_t gang_num = 0;
    for (auto cpg : cpgs)
    {
        if (isGangType(cpg.type))
        {
            gang_num++;
        }
    }
    return gang_num;
}

uint32_t getTargetUseType(const ComposeCard & composeCard, const vector<int32_t> & cards, int32_t targetCard)
{
    uint32_t type = target_use_invalid;
    if (targetCard == GAME_INVALID_CARD)
    {
        targetCard = cards.back();
    }
    for (auto three : composeCard.threes)
    {
        for (auto card : three.cards)
        {
            if (card == targetCard)
            {
                if (three.type == ThreeType::straight)
                {
                    type |= target_use_straight;
                }
                else
                {
                    type |= target_use_repeat;
                }
                break;
            }
        }
    }
    if (composeCard.eye[0] == targetCard)
    {
        type |= target_use_eye;
    }
    return type;
}

void CalculateBFLL(MjInfo & mjInfo, const ComposeCard & composeCard, vector<ComposeType> & composeTypes)
{
    bool isFourStraightType = false;
    bool isThreeStraightType = false;
    vector<ComposeType> threeStraightTypes;
    vector<ComposeType> oneStraightTypes;
    for (auto type : composeTypes)
    {
        if (type == shuanglonghui || type == sitongshun || type == sibugao)
        {
            isFourStraightType = true;
        }
        if (type == santongshun || type == qinglong || type == sanbugao)
        {
            isThreeStraightType = true;
            threeStraightTypes.push_back(type);
        }
        if (type == yibangao || type == laoshaofu || type == lianliu)
        {
            oneStraightTypes.push_back(type);
        }
    }
    auto removeTypes = [&](vector<ComposeType> & types)
    {
        for (auto type : types)
        {
            for (auto itr = composeTypes.begin(); itr != composeTypes.end(); ++itr)
            {
                if (type == *itr)
                {
                    composeTypes.erase(itr);
                    break;
                }
            }
        }
    };
    if (isFourStraightType)
    {
        removeTypes(threeStraightTypes);
        removeTypes(oneStraightTypes);
    }
    else if (isThreeStraightType)
    {
        //不可能同时存在两个三顺子牌型
        vector<ComposeType> needRemoveTypes = vector<ComposeType>(threeStraightTypes.begin() + 1, threeStraightTypes.end());
        removeTypes(needRemoveTypes);
        if (oneStraightTypes.size() > 1)
        {
            vector<ComposeType> needRemoveTypes = vector<ComposeType>(oneStraightTypes.begin() + 1, oneStraightTypes.end());
            removeTypes(needRemoveTypes);
        }
    }
    else
    {
        vector<ThreeCard> straightThrees;
        findStraight(mjInfo.cpgs, composeCard, straightThrees);
        if (straightThrees.size() == 4)
        {
            if (oneStraightTypes.size() == 6)
            {
                vector<ComposeType> needRemoveTypes = vector<ComposeType>(oneStraightTypes.begin(), oneStraightTypes.begin() + 3);
                removeTypes(needRemoveTypes);
            }
            else if (oneStraightTypes.size() > 2 && oneStraightTypes.size() <= 5)
            {
                vector<ComposeType> needRemoveTypes = vector<ComposeType>(oneStraightTypes.begin() + 2, oneStraightTypes.end());
                removeTypes(needRemoveTypes);
            }
        }
        else if (straightThrees.size() == 3)
        {
            if (oneStraightTypes.size() > 2)
            {
                vector<ComposeType> needRemoveTypes = vector<ComposeType>(oneStraightTypes.begin() + 2, oneStraightTypes.end());
                removeTypes(needRemoveTypes);
            }
        }
    }
}

uint32_t isDaSiXi(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x31, 0x34, repeatThrees);
    return repeatThrees.size() == 4;
}

uint32_t isXiaoSiXi(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x31, 0x34, repeatThrees);
    if (repeatThrees.size() != 3)
    {
        return false;
    }
    if (composeCard.eye[0] < 0x31 || composeCard.eye[0]>0x34)
    {
        return false;
    }
    return true;
}

uint32_t isDaSanFeng(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x31, 0x34, repeatThrees);
    return repeatThrees.size() == 3;
}

uint32_t isXiaoSanFeng(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x31, 0x34, repeatThrees);
    if (repeatThrees.size() != 2)
    {
        return false;
    }
    if (composeCard.eye[0] < 0x31 || composeCard.eye[0] > 0x34)
    {
        return false;
    }
    return true;
}

uint32_t isDaSanYuan(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x35, 0x37, repeatThrees);
    return repeatThrees.size() == 3;
}

uint32_t isXiaoSanYuan(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x35, 0x37, repeatThrees);
    if (repeatThrees.size() != 2)
    {
        return false;
    }
    if (composeCard.eye[0] < 0x35 || composeCard.eye[0]>0x37)
    {
        return false;
    }
    return true;
}

uint32_t isShuangJianKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x35, 0x37, repeatThrees);
    return repeatThrees.size() == 2;
}

uint32_t isJianKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x35, 0x37, repeatThrees);
    return repeatThrees.size() == 1;
}

uint32_t isSiGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return getGangNum(mjInfo.cpgs) == 4;
}

uint32_t isSanGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return getGangNum(mjInfo.cpgs) == 3;
}

uint32_t isShuangAnGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t num = 0;
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type == mj_an_gang)
        {
            num++;
        }
    }
    return num == 2;
}

uint32_t isShuangMingGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t num = 0;
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type == mj_dian_gang || cpg.type == mj_jia_gang)
        {
            num++;
        }
    }
    return num == 2;
}

uint32_t isAnGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t num = 0;
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type == mj_an_gang)
        {
            num++;
        }
    }
    return num == 1;
}

uint32_t isMingGang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t num = 0;
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type == mj_dian_gang || cpg.type == mj_jia_gang)
        {
            num++;
        }
    }
    return num == 1;
}

uint32_t isSiAnKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findCloseRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    uint32_t len = repeatThrees.size();
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        return len == 4;
    }
    if (len > 0 && getTargetUseType(composeCard, mjInfo.cards, mjInfo.targetCard) == target_use_repeat)
    {
        len--;
    }
    return  len == 4;
}

uint32_t isSanAnKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findCloseRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    uint32_t len = repeatThrees.size();
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        return len == 3;
    }
    if (len > 0 && getTargetUseType(composeCard, mjInfo.cards, mjInfo.targetCard) == target_use_repeat)
    {
        len--;
    }
    return len == 3;
}

uint32_t isShuangAnKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findCloseRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    uint32_t len = repeatThrees.size();
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        return len == 2;
    }
    if (len > 0 && getTargetUseType(composeCard, mjInfo.cards, mjInfo.targetCard) == target_use_repeat)
    {
        len--;
    }
    return  len == 2;
}

uint32_t isSiZiKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x31, 0x37, repeatThrees);
    return repeatThrees.size() == 4;
}

bool isLianKe(const vector<ThreeCard> & repeatThrees)
{
    if (repeatThrees.size() < 2)
    {
        return false;
    }
    uint32_t color = getColor(repeatThrees[0].cards[0]);
    if (color > 2)
    {
        return false;
    }
    int value = repeatThrees[0].cards[0];
    for (uint32_t i = 1; i < repeatThrees.size(); ++i)
    {
        if (getColor(repeatThrees[i].cards[0]) != color)
        {
            return false;
        }
        if ((value + i) != repeatThrees[i].cards[0])
        {
            return false;
        }
    }
    return true;
}

uint32_t isSiLianKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    if (repeatThrees.size() != 4)
    {
        return false;
    }
    sort(repeatThrees.begin(), repeatThrees.end(), [](ThreeCard a, ThreeCard b) {
        return a.cards[0] < b.cards[0];
    });
    if (!isLianKe(repeatThrees))
    {
        return false;
    }
    return true;
}

uint32_t isSanLianKe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    if (repeatThrees.size() < 3)
    {
        return false;
    }
    sort(repeatThrees.begin(), repeatThrees.end(), [](ThreeCard a, ThreeCard b) {
        return a.cards[0] < b.cards[0];
    });
    if (repeatThrees.size() == 4)
    {
        vector<ThreeCard> repeatThrees1(repeatThrees.begin(), repeatThrees.begin() + 3);
        if (isLianKe(repeatThrees1))
        {
            return true;
        }
        vector<ThreeCard> repeatThrees2(repeatThrees.begin() + 1, repeatThrees.end());
        if (isLianKe(repeatThrees2))
        {
            return true;
        }
        return false;
    }
    else
    {
        return isLianKe(repeatThrees);
    }
}

uint32_t isBuGao(const vector<CPG> & cpgs, const ComposeCard & composeCard, uint32_t len, uint32_t min_diff_value, uint32_t max_diff_value)
{
    uint32_t buGaoNum = 0;
    vector<ThreeCard> straightThrees;
    findStraight(cpgs, composeCard, straightThrees);
    if (straightThrees.size() < len)
    {
        return buGaoNum;
    }
    sort(straightThrees.begin(), straightThrees.end(), [](ThreeCard a, ThreeCard b) {return a.cards[0] < b.cards[0]; });
    vector<vector<int32_t>> idxCombinations;
    idxCombinations = combination(idxCombinations, len, straightThrees.size());
    auto testBuGao = [&](vector<int32_t> & idxCombination)
    {
        vector<ThreeCard> threes;
        for (auto idx : idxCombination)
        {
            threes.push_back(straightThrees[idx]);
        }
        uint32_t color = getColor(threes[0].cards[0]);
        uint32_t diff_value = threes[1].cards[0] - threes[0].cards[0];
        if (diff_value < min_diff_value || diff_value > max_diff_value)
        {
            return false;
        }
        for (uint32_t i = 1; i < threes.size(); ++i)
        {
            if (getColor(threes[i].cards[0]) != color)
            {
                return false;
            }
            if ((threes[i].cards[0] - threes[i - 1].cards[0]) != diff_value)
            {
                return false;
            }
        }
        return true;
    };
    for (auto idxCombination : idxCombinations)
    {
        if (testBuGao(idxCombination))
        {
            buGaoNum++;
        }
    }
    return buGaoNum;
}

uint32_t isSiBuGao(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return isBuGao(mjInfo.cpgs, composeCard, 4, 1, 2);
}

uint32_t isSanBuGao(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return isBuGao(mjInfo.cpgs, composeCard, 3, 1, 2);
}

uint32_t isYiBanGao(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return isBuGao(mjInfo.cpgs, composeCard, 2, 0, 0);
}

bool isTongShun(const vector<CPG> & cpgs, const ComposeCard & composeCard, uint32_t len)
{
    vector<ThreeCard> straightThrees;
    findStraight(cpgs, composeCard, straightThrees);
    if (straightThrees.size() < len)
    {
        return false;
    }
    uint32_t maxLen = 0;
    for (auto three1 : straightThrees)
    {
        uint32_t itemLen = 0;
        int32_t card = three1.cards[0];
        for (auto three2 : straightThrees)
        {
            if (card == three2.cards[0])
            {
                itemLen++;
            }
        }
        if (itemLen > maxLen)
        {
            maxLen = itemLen;
        }
    }
    return maxLen == len;
}

uint32_t isSiTongShun(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return isTongShun(mjInfo.cpgs, composeCard, 4);
}

uint32_t isSanTongShun(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return isTongShun(mjInfo.cpgs, composeCard, 3);
}

uint32_t isShuangLongHui(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (!isQingYiSe(mjInfo, composeCard))
    {
        return false;
    }
    vector<ThreeCard> straightThrees;
    findStraight(mjInfo.cpgs, composeCard, straightThrees);
    if (straightThrees.size() < 4)
    {
        return 0;
    }
    if (getCardValue(composeCard.eye[0]) != 0x05)
    {
        return 0;
    }
    sort(straightThrees.begin(), straightThrees.end(), [](ThreeCard a, ThreeCard b) {return a.cards[0] < b.cards[0]; });
    int32_t firstCards[4] = { 0x01,0x01,0x07,0x07 };
    for (uint32_t i = 0; i < straightThrees.size(); ++i)
    {
        if (getCardValue(straightThrees[i].cards[0]) != firstCards[i])
        {
            return 0;
        }
    }
    return 1;
}

uint32_t isLaoShaoFu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t laoShaoFuNum = 0;
    vector<ThreeCard> straightThrees;
    findStraight(mjInfo.cpgs, composeCard, straightThrees);
    if (straightThrees.size() < 2)
    {
        return laoShaoFuNum;
    }
    sort(straightThrees.begin(), straightThrees.end(), [](ThreeCard a, ThreeCard b) {return a.cards[0] < b.cards[0]; });
    vector<vector<int32_t>> idxCombinations;
    idxCombinations = combination(idxCombinations, 2, straightThrees.size());
    auto testLaoShaoFu = [&](vector<int32_t> & idxCombination)
    {
        uint32_t idx1 = idxCombination[0];
        uint32_t idx2 = idxCombination[1];
        if (getColor(straightThrees[idx1].cards[0]) == getColor(straightThrees[idx2].cards[0]))
        {
            if (getCardValue(straightThrees[idx1].cards[0]) == 1 && getCardValue(straightThrees[idx2].cards[0]) == 7)
            {
                return true;
            }
        }
        return false;
    };
    for (auto idxCombination : idxCombinations)
    {
        if (testLaoShaoFu(idxCombination))
        {
            laoShaoFuNum++;
        }
    }
    return laoShaoFuNum;
}

uint32_t isHunYaoJiu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    for (auto card : allCards)
    {
        if (!isYao9Card(card))
        {
            return 0;
        }
    }
    return 1;
}

uint32_t isQuanDaiYao(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> threes;
    findThree(mjInfo.cpgs, composeCard, threes);
    for (auto three : threes)
    {
        if (!isYao9Three(three))
        {
            return false;
        }
    }
    if (!isYao9Card(composeCard.eye[0]))
    {
        return false;
    }
    return true;
}

uint32_t isDuanYao(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    for (auto card : allCards)
    {
        if (isYao9Card(card))
        {
            return false;
        }
    }
    return true;
}

uint32_t isZiYiSe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    for (auto card : allCards)
    {
        if (card < 0x31 || card > 0x37)
        {
            return false;
        }
    }
    return true;
}

uint32_t isQingYiSe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    uint32_t color = getColor(allCards[0]);
    if (color > 2)
    {
        return false;
    }
    for (uint32_t i = 1; i < allCards.size(); ++i)
    {
        if (getColor(allCards[i]) != color)
        {
            return false;
        }
    }
    return true;
}

uint32_t isHunYiSe(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    bool colorCardExist = false;
    bool ziCardExist = false;
    for (auto cpg : mjInfo.cpgs)
    {
        for (auto card : cpg.cards)
        {
            if (getColor(card) < 3)
            {
                colorCardExist = true;
            }
            else
            {
                ziCardExist = true;
            }
        }
    }
    for (auto card : mjInfo.cards)
    {
        if (getColor(card) < 3)
        {
            colorCardExist = true;
        }
        else
        {
            ziCardExist = true;
        }
    }
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        if (getColor(mjInfo.targetCard) < 3)
        {
            colorCardExist = true;
        }
        else
        {
            ziCardExist = true;
        }
    }
    return colorCardExist && ziCardExist;
}

uint32_t isQingLong(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> straightThrees;
    findStraight(mjInfo.cpgs, composeCard, straightThrees);
    if (straightThrees.size() < 3)
    {
        return false;
    }
    sort(straightThrees.begin(), straightThrees.end(), [](ThreeCard a, ThreeCard b) { return a.cards[0] < b.cards[0]; });
    vector<int32_t> firstCardValues = { 1, 4, 7 };
    vector<vector<int32_t>> idxCombinations;
    idxCombinations = combination(idxCombinations, 3, straightThrees.size());
    auto testQingLong = [&](vector<int32_t> & idxCombination)
    {
        vector<ThreeCard> threes;
        for (auto idx : idxCombination)
        {
            threes.push_back(straightThrees[idx]);
        }
        uint32_t color = getColor(threes[0].cards[0]);
        for (uint32_t i = 0; i < threes.size(); ++i)
        {
            if (getColor(threes[i].cards[0]) != color)
            {
                return false;
            }
            if (getCardValue(threes[i].cards[0]) != firstCardValues[i])
            {
                return false;
            }
        }
        return true;
    };
    for (auto idxCombination : idxCombinations)
    {
        if (testQingLong(idxCombination))
        {
            return true;
        }
    }
    return false;
}

uint32_t isLianLiu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t lianLiuNum = 0;
    vector<ThreeCard> straightThrees;
    findStraight(mjInfo.cpgs, composeCard, straightThrees);
    if (straightThrees.size() < 2)
    {
        return lianLiuNum;
    }
    sort(straightThrees.begin(), straightThrees.end(), [](ThreeCard a, ThreeCard b) {return a.cards[0] < b.cards[0]; });
    vector<vector<int32_t>> idxCombinations;
    idxCombinations = combination(idxCombinations, 2, straightThrees.size());
    auto testLianLiu = [&](vector<int32_t> & idxCombination)
    {
        uint32_t idx1 = idxCombination[0];
        uint32_t idx2 = idxCombination[1];
        if (getColor(straightThrees[idx1].cards[0]) == getColor(straightThrees[idx2].cards[0]))
        {
            if (straightThrees[idx2].cards[0] - straightThrees[idx1].cards[0] == 3)
            {
                return true;
            }
        }
        return false;
    };
    for (auto idxCombination : idxCombinations)
    {
        if (testLianLiu(idxCombination))
        {
            lianLiuNum++;
        }
    }
    return lianLiuNum;
}

uint32_t isDaYuWu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    for (auto card : allCards)
    {
        if (card >= 0x31 || getCardValue(card) < 6)
        {
            return false;
        }
    }
    return true;
}

uint32_t isXiaoYuWu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<int32_t> allCards;
    findAllCards(mjInfo, allCards);
    for (auto card : allCards)
    {
        if (card >= 0x31 || getCardValue(card) > 4)
        {
            return false;
        }
    }
    return true;
}

uint32_t isQuanQiuRen(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (isZiMo(mjInfo, composeCard))
    {
        return false;
    }
    if (mjInfo.cpgs.size() != 4)
    {
        return false;
    }
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type == mj_an_gang)
        {
            return false;
        }
    }
    return true;
}

uint32_t isBuQiuRen(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (!isZiMo(mjInfo, composeCard))
    {
        return false;
    }
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type != mj_an_gang)
        {
            return false;
        }
    }
    return true;
}

int32_t GetEyeNum(array<int32_t, TABLE_SIZE> & table)
{
    uint32_t num = 0;
    for (auto i : table)
    {
        if (i >= 35)
        {
            return false;
        }
        if (i == 0 || i == 1)
        {
            continue;
        }
        if (i == 2 || i == 3)
        {
            num += 1;
        }
        else
        {
            num += 2;
        }
    }
    return num;
}

bool isTableQiDui(array<int32_t, TABLE_SIZE> & table)
{
    return GetEyeNum(table) == 7;
}

uint32_t isQiDui(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (isHunYaoJiu(mjInfo, composeCard))
    {
        return 0;
    }
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }
    return isTableQiDui(table);
}

uint32_t isDaQiXing(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }
    for (uint32_t i = 28; i <= 34; ++i)
    {
        if (table[i] != 2)
        {
            return false;
        }
    }
    return true;
}

uint32_t isLianQiDui(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }
    if (!isTableQiDui(table))
    {
        return false;
    }
    for (uint32_t i = 0; i < TABLE_SIZE; ++i)
    {
        if (table[i] == 2)
        {
            int32_t card = getCardByIdx(i);
            if (getColor(card) > 2)
            {
                return false;
            }
            if (getCardValue(card) > 3)
            {
                return false;
            }
            uint32_t num = 1;
            uint32_t next = i + 1;
            while (table[next] == 2)
            {
                num++;
                next++;
            }
            return num == 7;
        }
    }
    return true;
}

uint32_t isSanYuanQiDui(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }
    if (!isTableQiDui(table))
    {
        return false;
    }
    for (uint32_t i = 32; i <= 34; ++i)
    {
        if (table[i] == 0)
        {
            return false;
        }
        if ((table[i] % 2) != 0)
        {
            return false;
        }
    }
    return true;
}

uint32_t isSiXiQiDui(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(mjInfo.cards, table);
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        table[getCardIdx(mjInfo.targetCard)]++;
    }
    if (!isTableQiDui(table))
    {
        return false;
    }
    for (uint32_t i = 28; i <= 31; ++i)
    {
        if (table[i] == 0)
        {
            return false;
        }
        if ((table[i] % 2) != 0)
        {
            return false;
        }
    }
    return true;
}

uint32_t isTianHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_tianhu) ? 1 : 0;
}

uint32_t isDiHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_dihu) ? 1 : 0;
}

uint32_t isRenHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_renhu) ? 1 : 0;
}

uint32_t isTianTing(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_tianting) ? 1 : 0;
}

uint32_t isBaoTing(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_baoting) ? 1 : 0;
}

uint32_t isHuJueZhang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_juezhang) ? 1 : 0;
}

uint32_t isMiaoShouHuiChun(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_miaoshouhuichun) ? 1 : 0;
}

uint32_t isHaiDiLaoYue(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_haidilaoyue) ? 1 : 0;
}

uint32_t isGangShangKaiHua(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_gangshangkaihua) ? 1 : 0;
}

uint32_t isQiangGangHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_qiangganghu) ? 1 : 0;
}

uint32_t isZiMo(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return (mjInfo.huType & hu_zimo) ? 1 : 0;
}

uint32_t isQuanHua(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return mjInfo.huaNum == 8;
}

uint32_t isHuaPai(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    return mjInfo.huaNum;
}

uint32_t isJiuLianBaoDeng(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (!isQingYiSe(mjInfo, composeCard))
    {
        return false;
    }
    //先转换成万字花色
    vector<int32_t> cardsWan;
    for (auto card : mjInfo.cards)
    {
        cardsWan.push_back(getCardValue(card));
    }
    //	if (mjInfo.targetCard != GAME_INVALID_CARD)
    //	{
    //		cardsWan.push_back(getCardValue(mjInfo.targetCard));
    //	}
    array<int, TABLE_SIZE> table = { 0 };
    cardsToTable(cardsWan, table);
    for (uint32_t i = 0; i < TABLE_SIZE; ++i)
    {
        table[i] = table[i] - TABLE_JIULIANBAODENG[i];
        if (table[i] < 0)
        {
            return false;
        }
    }
    for (uint32_t i = 1; i <= 9; ++i)
    {
        if (table[i] > 0)
        {
            return true;
        }
    }
    return false;
}

uint32_t isPengPengHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> repeatThrees;
    findRepeat(mjInfo.cpgs, composeCard, 0x01, 0x37, repeatThrees);
    return repeatThrees.size() == 4;
}

uint32_t isPingHu(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    vector<ThreeCard> straightThrees;
    findStraight(mjInfo.cpgs, composeCard, straightThrees);
    if (straightThrees.size() != 4)
    {
        return false;
    }
    if (composeCard.eye[0] >= 0x31)
    {
        return false;
    }
    return true;
}

uint32_t isSiGuiYi(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t siGuiYiNum = 0;
    vector<int32_t> allCards;
    for (auto cpg : mjInfo.cpgs)
    {
        if (isChiType(cpg.type) || cpg.type == mj_peng)
        {
            for (auto card : cpg.cards)
            {
                allCards.push_back(card);
            }
        }
    }
    for (auto card : mjInfo.cards)
    {
        allCards.push_back(card);
    }
    if (mjInfo.targetCard != GAME_INVALID_CARD)
    {
        allCards.push_back(mjInfo.targetCard);
    }
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(allCards, table);
    for (uint32_t i = 0; i < TABLE_SIZE; ++i)
    {
        if (table[i] == 4)
        {
            siGuiYiNum++;
        }
    }
    return siGuiYiNum;
}

uint32_t isMenQianQing(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    if (isZiMo(mjInfo, composeCard))
    {
        return false;
    }
    uint32_t num = 0;
    for (auto cpg : mjInfo.cpgs)
    {
        if (cpg.type != mj_an_gang)
        {
            return false;
        }
    }
    return true;
}

// 单钓将:可以多面听，但只能单面钓, 因此1234钓1或4不能算作单钓将，类似5666和5这种情况，虽然是听457三张牌，但只能钓5，和4或7就不是钓，因此5666和5仍算作单钓将
uint32_t isDanDiaoJiang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t useType = getTargetUseType(composeCard, mjInfo.cards, mjInfo.targetCard);
    if ((useType & target_use_eye) == 0)
    {
        return false;
    }
    int32_t huCard = mjInfo.targetCard;
    vector<int32_t> cardsWithoutTarget;
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        huCard = mjInfo.cards.back();
        cardsWithoutTarget = { mjInfo.cards.begin(), mjInfo.cards.begin() + mjInfo.cards.size() - 1 };
    }
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cardsWithoutTarget, table);
    for (uint32_t j = 1; j <= 34; ++j)
    {
        if (getCardByIdx(j) == huCard)
        {
            continue;
        }
        vector<ComposeCard> tempComposeCards;
        table[j]++;
        bool hu = canHu(table, tempComposeCards);
        table[j]--;
        if (hu)
        {
            for (auto tempComposeCard : tempComposeCards)
            {
                uint32_t useType = getTargetUseType(tempComposeCard, cardsWithoutTarget, getCardByIdx(j));
                if ((useType & target_use_eye) != 0)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

uint32_t isBianZhang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t huCard;
    vector<int32_t> cardsWithoutTarget;
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        cardsWithoutTarget = { mjInfo.cards.begin(), mjInfo.cards.begin() + mjInfo.cards.size() - 1 };
        huCard = mjInfo.cards.back();
    }
    else
    {
        huCard = mjInfo.targetCard;
    }
    if (getColor(huCard) > 2)
    {
        return false;
    }
    if (getCardValue(huCard) != 3 && getCardValue(huCard) != 7)
    {
        return false;
    }
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cardsWithoutTarget, table);
    for (uint32_t j = 1; j <= 34; ++j)
    {
        if (getCardByIdx(j) == huCard)
        {
            continue;
        }
        vector<ComposeCard> tempComposeCards;
        table[j]++;
        bool hu = canHu(table, tempComposeCards);
        table[j]--;
        if (hu)
        {
            for (auto tempComposeCard : tempComposeCards)
            {
                uint32_t useType = getTargetUseType(tempComposeCard, cardsWithoutTarget, getCardByIdx(j));
                if ((useType & target_use_straight) != 0 || (useType & target_use_repeat) != 0)
                {
                    return false;
                }
            }
        }
    }
    uint32_t color = getColor(huCard);
    uint32_t value = getCardValue(huCard);
    for (auto three : composeCard.threes)
    {
        if (three.type == ThreeType::straight)
        {
            if (value == 3)
            {
                if (getColor(three.cards[0]) == color && getCardValue(three.cards[0]) == 1)
                {
                    return true;
                }
            }
            else if (value == 7)
            {
                if (getColor(three.cards[0]) == color && getCardValue(three.cards[0]) == 7)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

uint32_t isKanZhang(MjInfo & mjInfo, const ComposeCard & composeCard)
{
    uint32_t huCard;
    vector<int32_t> cardsWithoutTarget;
    if (mjInfo.targetCard == GAME_INVALID_CARD)
    {
        cardsWithoutTarget = { mjInfo.cards.begin(), mjInfo.cards.begin() + mjInfo.cards.size() - 1 };
        huCard = mjInfo.cards.back();
    }
    else
    {
        huCard = mjInfo.targetCard;
    }
    if (getColor(huCard) > 2)
    {
        return false;
    }
    array<int32_t, TABLE_SIZE> table = { 0 };
    cardsToTable(cardsWithoutTarget, table);
    for (uint32_t j = 1; j <= 34; ++j)
    {
        if (getCardByIdx(j) == huCard)
        {
            continue;
        }
        vector<ComposeCard> tempComposeCards;
        table[j]++;
        bool hu = canHu(table, tempComposeCards);
        table[j]--;
        if (hu)
        {
            for (auto tempComposeCard : tempComposeCards)
            {
                uint32_t useType = getTargetUseType(tempComposeCard, cardsWithoutTarget, getCardByIdx(j));
                if ((useType & target_use_straight) != 0 || (useType & target_use_repeat) != 0)
                {
                    return false;
                }
            }
        }
    }
    for (auto three : composeCard.threes)
    {
        if (three.type == ThreeType::straight)
        {
            if (three.cards[1] == huCard)
            {
                return true;
            }
        }
    }
    return false;
}

uint32_t getTimes(uint32_t huaNum, vector<ComposeType> & composeTypes)
{
    uint32_t times = 0;
    for (auto composeType : composeTypes)
    {
        uint32_t idx = getComposeTypeCheckerIdx(composeType);
        times += composeTypeCheckers[idx].checkInfo.times;
    }
    return times;
}

bool isValidCard(int32_t card)
{
    if (card >= 0x01 && card <= 0x09)
    {
        return true;
    }
    else if (card >= 0x11 && card <= 0x19)
    {
        return true;
    }
    else if (card >= 0x21 && card <= 0x29)
    {
        return true;
    }
    else if (card >= 0x31 && card <= 0x37)
    {
        return true;
    }
    else if (card >= 0x41 && card <= 0x48)
    {
        return true;
    }
    else
    {
        return false;
    }
}

PrInfo& findPrInfo(const vector<int32_t> & cards, const vector<int32_t> outCardsExclude, array<int32_t, TABLE_SIZE> & leftTable, PrInfo & fastHuPrInfo)
{
    if (cards.size() % 3 != 2 && cards.size() % 3 != 1)
    {
        return fastHuPrInfo;
    }
    array<int32_t, TABLE_SIZE> handTable = { 0 };
    cardsToTable(cards, handTable);
    int32_t eyeNum = GetEyeNum(handTable);
    vector<array<int32_t, 9>> handPartTables;
    vector<array<int32_t, 9>> outPartTables;
    vector<array<int32_t, 9>> leftPartTables;
    uint32_t startIdxs[2] = { 1, 28 };
    for (uint32_t i = 0; i < 2; ++i)
    {
        array<int32_t, 9> handPart = { 0 };
        array<int32_t, 9> leftPart = { 0 };
        for (uint32_t k = 0; k < 9; ++k)
        {
            handPart[k] = handTable[startIdxs[i] + k];
            leftPart[k] = leftTable[startIdxs[i] + k];
        }
        handPartTables.push_back(handPart);
        leftPartTables.push_back(leftPart);
    }
    if (cards.size() % 3 == 1)
    {
        calculateFastestHuPrInfo(cards.size() + 1, handPartTables, leftPartTables, eyeNum, fastHuPrInfo);
        return fastHuPrInfo;
    }
    //控制有些牌不能出
    auto isExclude = [&](int32_t card) {
        for (auto excludeCard : outCardsExclude)
        {
            if (card == excludeCard)
            {
                return true;
            }
        }
        return false;
    };
    int32_t outCardIdx = 0;
    bool outCardSelIsZi = false;
    for (uint32_t i = 1; i < TABLE_SIZE; ++i)
    {
        if (handTable[i] > 0)
        {
            if (isExclude(getCardByIdx(i)))
            {
                continue;
            }
            handTable[i]--;
            bool outCardIsZi = true;
            if (i <= 9)
            {
                handPartTables[0][i - 1]--;
                outCardIsZi = false;
            }
            else if (i >= 28 && i <= 34)
            {
                handPartTables[1][i - 28]--;
                outCardIsZi = true;
            }
            PrInfo prInfo;
            calculateFastestHuPrInfo(cards.size(), handPartTables, leftPartTables, eyeNum, prInfo);
            if (prInfo.probability == 0)
            {
                continue;
            }
            CTableMgr::Instance()->DumpProbability(std::to_string(getCardByIdx(i)));
            //优先胡牌最快
            if (fastHuPrInfo.need == 0 || prInfo.need < fastHuPrInfo.need)
            {
                fastHuPrInfo = prInfo;
                outCardIdx = i;
                outCardSelIsZi = outCardIsZi;
            }
            else if (prInfo.need == fastHuPrInfo.need)
            {
                //胡牌速度相同,则比概率
                if (prInfo.probability > fastHuPrInfo.probability)
                {
                    fastHuPrInfo = prInfo;
                    outCardIdx = i;
                    outCardSelIsZi = outCardIsZi;
                }
                else if (prInfo.probability == fastHuPrInfo.probability)
                {
                    //概率相同,听牌时,优先听字
                    if (prInfo.need == 1)
                    {
                        int32_t prZiNum = GetZiCardNum(prInfo.needCards);
                        int32_t fastPrZiNum = GetZiCardNum(fastHuPrInfo.needCards);
                        if (prZiNum > fastPrZiNum)
                        {
                            fastHuPrInfo = prInfo;
                            outCardIdx = i;
                            outCardSelIsZi = outCardIsZi;
                        }
                        else if (prZiNum == fastPrZiNum)
                        {
                            srand(time(nullptr));
                            if (rand() % 2 == 0)
                            {
                                fastHuPrInfo = prInfo;
                                outCardIdx = i;
                                outCardSelIsZi = outCardIsZi;
                            }
                        }
                    }
                    //概率相同,非听牌,优先出字
                    else
                    {
                        //优先出字
                        if (!outCardSelIsZi && outCardIsZi)
                        {
                            fastHuPrInfo = prInfo;
                            outCardIdx = i;
                            outCardSelIsZi = outCardIsZi;
                        }
                        //随机
                        else if (!(outCardSelIsZi && !outCardIsZi))
                        {
                            srand(time(nullptr));
                            if (rand() % 2 == 0)
                            {
                                fastHuPrInfo = prInfo;
                                outCardIdx = i;
                                outCardSelIsZi = outCardIsZi;
                            }
                        }
                    }
                }
            }
            if (i <= 9)
            {
                handPartTables[0][i - 1]++;
            }
            else if (i >= 28 && i <= 34)
            {
                handPartTables[1][i - 28]++;
            }
            handTable[i]++;
        }
    }
    if (outCardIdx != 0)
    {
        fastHuPrInfo.outCard = getCardByIdx(outCardIdx);
    }
    return fastHuPrInfo;
}

PrInfo& findHandCardsPrInfo(const vector<int32_t> & handCards, PrInfo & fastHuPrInfo)
{
    array<int32_t, TABLE_SIZE> handTable = { 0 };
    cardsToTable(handCards, handTable);
    vector<array<int32_t, 9>> handPartTables;
    vector<array<int32_t, 9>> leftPartTables;
    uint32_t startIdxs[2] = { 1, 28 };
    for (uint32_t i = 0; i < 2; ++i)
    {
        array<int32_t, 9> handPart = { 0 };
        array<int32_t, 9> leftPart = { 0 };
        for (uint32_t k = 0; k < 9; ++k)
        {
            handPart[k] = handTable[startIdxs[i] + k];
            leftPart[k] = 4;
        }
        handPartTables.push_back(handPart);
        leftPartTables.push_back(leftPart);
    }
    calculateFastestHuPrInfo(14, handPartTables, leftPartTables, 5, fastHuPrInfo);
    return fastHuPrInfo;
}

double c_n_m(int8_t n, int8_t m)
{
    int8_t m_itr = 1;
    int64_t m_factorial = m_itr;
    int64_t n_m_factorial = n;
    while (m_itr < m)
    {
        n_m_factorial *= (n - m_itr);
        m_itr++;
        m_factorial *= m_itr;
    }
    return n_m_factorial / m_factorial;
}

void calculateFastestHuPrInfo(int32_t handNum, vector<array<int32_t, 9>> & handPartTables, vector<array<int32_t, 9>> & leftPartTables, int32_t eyeNum, PrInfo & fastestHuPrInfo)
{
    CTableMgr::Instance()->ClearProbability();
    int32_t minNeed = 0;
    vector<pair<vector<int8_t>, bool>> fastHuParts;
    auto calculateFastestHuParts = [&](vector<vector<int8_t>> & parts, bool qidui)
    {
        for (auto part : parts)
        {
            int32_t onePartMinNeed = 0;
            bool partValid = true;
            for (uint32_t i = 0; i < part.size(); ++i)
            {
                if (part[i] == 0)
                {
                    continue;
                }
                shared_ptr<CTable> huTbl = qidui ? CTableMgr::Instance()->GetQiDuiTable(part[i], i == 1) : CTableMgr::Instance()->GetTable(part[i], i == 1);
                if (!huTbl)
                {
                    continue;
                }
                array<int32_t, 9>& handPartTable = handPartTables[i];
                array<int32_t, 9>& leftPartTable = leftPartTables[i];
                int32_t oneTableItemMinNeed = -1;
                for (auto huTable : huTbl->GetTables())
                {
                    double oneTableItemProbability = 0;
                    int32_t allNeed = 0;
                    array<int32_t, 9> needTable = { 0 };
                    bool huAble = true;
                    for (int32_t m = 0; m < 9; ++m)
                    {
                        if (huTable[m] == 0)
                        {
                            continue;
                        }
                        int32_t need = huTable[m] - handPartTable[m];
                        if (need <= 0)
                        {
                            continue;
                        }
                        allNeed += need;
                        //牌不够
                        if (leftPartTable[m] <= 0 || leftPartTable[m] - need < 0)
                        {
                            huAble = false;
                            oneTableItemProbability = 0;
                            break;
                        }
                        needTable[m] = need;
                        if (oneTableItemProbability == 0)
                        {
                            oneTableItemProbability = 1;
                        }
                        oneTableItemProbability *= c_n_m(leftPartTable[m], need);
                    }
                    if (!huAble)
                    {
                        continue;
                    }
                    TablePrInfo info;
                    info.huTable = huTable;
                    info.need = allNeed;
                    info.probability = oneTableItemProbability;
                    info.needTable = needTable;
                    huTbl->addPrInfo(info);
                    if (allNeed <= oneTableItemMinNeed || oneTableItemMinNeed == -1)
                    {
                        oneTableItemMinNeed = allNeed;
                    }
                }
                if (huTbl->GetPrInfos().empty())
                {
                    partValid = false;
                    break;
                }
                onePartMinNeed += oneTableItemMinNeed;
            }
            if (!partValid)
            {
                continue;
            }
            //存在胡牌概率的且最快听牌优先
            if (minNeed == 0 || onePartMinNeed < minNeed)
            {
                minNeed = onePartMinNeed;
                fastHuParts.clear();
                fastHuParts.push_back(make_pair(part, qidui));
            }
            else if (onePartMinNeed == minNeed)
            {
                fastHuParts.push_back(make_pair(part, qidui));
            }
        }
    };
    //非七对胡
    vector<vector<int8_t>>& parts = CPart::Instance()->GetParts(handNum);
    calculateFastestHuParts(parts, false);
    //14张牌且五对以上，计算七对胡
    if (handNum == 14 && eyeNum >= 5)
    {
        vector<vector<int8_t>>& qiDuiParts = CPart::Instance()->GetQiDuiPart();
        calculateFastestHuParts(qiDuiParts, true);
    }
    auto addNeedCard = [&](int32_t needCard) {
        bool exist = false;
        for (auto card : fastestHuPrInfo.needCards)
        {
            if (card == needCard)
            {
                exist = true;
            }
        }
        if (!exist)
        {
            fastestHuPrInfo.needCards.push_back(needCard);
        }
    };
    int64_t probability = 0;
    //一种可能性设置标识
    bool onePrSeted[2] = { false, false };
    for (auto part : fastHuParts)
    {
        int64_t partProbability = 0;
        for (uint32_t i = 0; i < part.first.size(); ++i)
        {
            if (part.first[i] == 0)
            {
                onePrSeted[i] = true;
                continue;
            }
            shared_ptr<CTable> fastHuTbl = part.second ? CTableMgr::Instance()->GetQiDuiTable(part.first[i], i == 1) : CTableMgr::Instance()->GetTable(part.first[i], i == 1);
            if (!fastHuTbl)
            {
                onePrSeted[i] = true;
                continue;
            }
            fastHuTbl->SetBest(true);
            array<int32_t, 9>& leftPartTable = leftPartTables[i];
            int64_t tableProbability = fastHuTbl->calculateFastestHuPr(leftPartTable);
            if (partProbability == 0 || tableProbability == 0)
            {
                partProbability += tableProbability;
            }
            else
            {
                partProbability *= tableProbability;
            }
            int32_t startIdx = fastHuTbl->GetStartIdx();
            for (auto prInfo : fastHuTbl->GetFastestHuPrInfos())
            {
                for (int32_t k = 0; k < 9; ++k)
                {
                    if (prInfo.needTable[k] > 0)
                    {
                        addNeedCard(getCardByIdx(startIdx + k));
                    }
                    if (!onePrSeted[i])
                    {
                        //if (prInfo.needTable[k] > 0)
                        //{
                        //	fastestHuPrInfo.onePrNeedCards.push_back(getCardByIdx(startIdx + k));
                        //}
                        //if (prInfo.huTable[k] > 0)
                        //{
                        //	fastestHuPrInfo.onePrHuCards.push_back(getCardByIdx(startIdx + k));
                        //}
                        onePrSeted[i] = true;
                    }
                }
            }
        }
        probability += partProbability;
    }
    fastestHuPrInfo.need = minNeed;
    fastestHuPrInfo.probability = probability;
    for (auto part : fastHuParts)
    {
        for (uint32_t i = 0; i < part.first.size(); ++i)
        {
            if (part.first[i] == 0)
            {
                continue;
            }
            shared_ptr<CTable> fastHuTbl = part.second ? CTableMgr::Instance()->GetQiDuiTable(part.first[i], i == 1) : CTableMgr::Instance()->GetTable(part.first[i], i == 1);
            if (!fastHuTbl)
            {
                continue;
            }
            fastHuTbl->SetPartMinNeed(minNeed);
            fastHuTbl->SetPartProbality(probability);
        }
    }
}

int32_t GetZiCardNum(vector<int32_t> & cards)
{
    int32_t ziNum = 0;
    for (auto card : cards)
    {
        if (getColor(card) == 3)
        {
            ziNum++;
        }
    }
    return ziNum;
}

vector<int32_t> GetGoodHandCards(vector<int32_t> & cards, int32_t num, vector<int8_t> & part)
{
    array<int32_t, TABLE_SIZE> leftTable = { 0 };
    cardsToTable(cards, leftTable);
    vector<int32_t> goodCards;
    array<int32_t, TABLE_SIZE> goodTable = { 0 };
    bool valid = false;
    do
    {
        goodCards.clear();
        shared_ptr<CTable> pTable = CTableMgr::Instance()->GetTable(part[0], false);
        shared_ptr<CTable> pZiTable = CTableMgr::Instance()->GetTable(part[1], true);
        if (pTable)
        {
            vector<int32_t> cards = CTableMgr::Instance()->GetTable(part[0], false)->RandTableItem();
            goodCards.insert(goodCards.begin(), cards.begin(), cards.end());
        }
        if (pZiTable)
        {
            vector<int32_t> ziCards = CTableMgr::Instance()->GetTable(part[1], true)->RandTableItem();
            goodCards.insert(goodCards.begin(), ziCards.begin(), ziCards.end());
        }
        random_shuffle(goodCards.begin(), goodCards.end());
        goodCards.erase(goodCards.begin() + num, goodCards.end());
        goodTable.fill(0);
        cardsToTable(goodCards, goodTable);
        valid = true;
        for (int32_t i = 1; i < TABLE_SIZE; ++i)
        {
            if (goodTable[i] > leftTable[i])
            {
                valid = false;
            }
        }
    } while (!valid);
    for (int32_t i = 1; i < TABLE_SIZE; ++i)
    {
        leftTable[i] -= goodTable[i];
    }
    cards.clear();
    tableToCards(leftTable, cards);
    random_shuffle(cards.begin(), cards.end());
    return goodCards;
}

vector<int32_t> GetBadHandCards(vector<int32_t> & cards, int32_t num)
{
    array<int32_t, TABLE_SIZE> leftTable = { 0 };
    cardsToTable(cards, leftTable);
    vector<int32_t> singleCards;
    for (int32_t i = 1; i < TABLE_SIZE; ++i)
    {
        if (i >= 35)
        {
            break;
        }
        if (leftTable[i] > 0)
        {
            singleCards.push_back(getCardByIdx(i));
        }
    }
    vector<int32_t> bagCards;
    auto addCard = [&](int32_t card)
    {
        if (getColor(card) == 3)
        {
            bagCards.push_back(card);
            return true;
        }
        else
        {
            for (auto badCard : bagCards)
            {
                if (getColor(badCard) == getColor(card))
                {
                    if (abs(badCard - card) < 2)
                    {
                        return false;
                    }
                }
            }
            bagCards.push_back(card);
            return true;
        }
    };
    int32_t curNum = 0;
    do
    {
        srand(time(nullptr));
        int32_t idx = rand() % singleCards.size();
        if (addCard(singleCards[idx]))
        {
            curNum++;
            leftTable[getCardIdx(singleCards[idx])]--;
        }
        singleCards.erase(singleCards.begin() + idx);
    } while (curNum < num && !singleCards.empty());
    cards.clear();
    tableToCards(leftTable, cards);
    random_shuffle(cards.begin(), cards.end());
    return bagCards;
}

vector<int32_t> GetHandCardsCtrl(vector<int32_t> & cards, int32_t num, vector<int8_t> & part, bool good)
{
    array<int32_t, TABLE_SIZE> leftTable = { 0 };
    cardsToTable(cards, leftTable);
    vector<int32_t> goodCards;
    array<int32_t, TABLE_SIZE> goodTable = { 0 };
    bool valid = false;
    do
    {
        goodCards.clear();
        shared_ptr<CTable> pTable = CTableMgr::Instance()->GetTable(part[0], false);
        shared_ptr<CTable> pZiTable = CTableMgr::Instance()->GetTable(part[1], true);
        if (pTable)
        {
            vector<int32_t> cards = CTableMgr::Instance()->GetTable(part[0], false)->RandTableItem();
            goodCards.insert(goodCards.begin(), cards.begin(), cards.end());
        }
        if (pZiTable)
        {
            vector<int32_t> ziCards = CTableMgr::Instance()->GetTable(part[1], true)->RandTableItem();
            goodCards.insert(goodCards.begin(), ziCards.begin(), ziCards.end());
        }
        random_shuffle(goodCards.begin(), goodCards.end());
        if (!good)
        {
            goodCards.erase(goodCards.begin() + num, goodCards.end());
        }
        goodTable.fill(0);
        cardsToTable(goodCards, goodTable);
        valid = true;
        for (int32_t i = 1; i < TABLE_SIZE; ++i)
        {
            if (goodTable[i] > leftTable[i])
            {
                valid = false;
            }
        }
        if (good)
        {
            goodCards.erase(goodCards.begin() + num, goodCards.end());
        }
    } while (!valid);
    for (int32_t i = 1; i < TABLE_SIZE; ++i)
    {
        leftTable[i] -= goodTable[i];
    }
    cards.clear();
    tableToCards(leftTable, cards);
    random_shuffle(cards.begin(), cards.end());
    return goodCards;
}
