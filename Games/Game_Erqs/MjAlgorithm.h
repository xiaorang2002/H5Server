#pragma once
#include<string>
#include<vector>
#include<array>
#include <functional>
#include <unordered_map>
#include <algorithm>

using namespace std;

//表第一个元素只作占位
#define TABLE_SIZE	(43)
//不可见的牌
#define INVISIBLE_CARD	(0)
//无效的牌
#define GAME_INVALID_CARD    (-1)
enum MutuallyType
{
    none=0,
    sansanyi3n2,
    qiduizi222,
    otherhu000,
    maxvalue
};
enum ComposeType
{
	invalid = 0,

	dasixi, //大四喜
	xiaosixi, //小四喜
	dasanfeng, //大三风
	xiaosanfeng, //小三风

	dasanyuan, //大三元
	xiaosanyuan, //小三元
	shuangjianke, //双箭刻
	jianke, //箭刻

	sigang, //四杠
	sangang, //三杠
	shuangangang, //双暗杠
	shuangminggang, //双明杠
	angang, //暗杠
	minggang, //明杠

	sianke, //四暗刻
	sananke, //三暗刻
	shuanganke, //双暗刻
	silianke, //四连刻
	sanlianke, //三连刻
	sizike, //四字刻

	sibugao, //四步高
	sanbugao, //三步高
	yibangao, //一般高

	sitongshun, //四同顺
	santongshun, //三同顺

	shuanglonghui, //双龙会
	laoshaofu, //老少副

	hunyaojiu, //混幺九
	quandaiyao, //全带幺
	duanyao, //断幺

	ziyise, //字一色
	qingyise, //清一色
	hunyise, //混一色

	qinglong, //清龙
	lianliu, //连六

	dayuwu, //大于五
	xiaoyuwu, //小于五

	quanqiuren, //全求人
	buqiuren, //不求人

	daqixing, //大七星
	lianqidui, //连七对
	sanyuanqiduizi, //三元七对子
	sixiqiduizi, //四喜七对子
	qiduizi, //七对子

	jiulianbaodeng, //九莲宝灯
	pengpenghu, //碰碰和
	pinghu, //平和
	siguiyi, //四归一
	menqianqing, //门前清
	bianzhang, //边张
	kanzhang, //砍张
	dandiaojiang, //单钓将

	tianhu, //天胡
	dihu, //地胡
	renhu, //人胡
	tianting, //天听
	quanhua, //全花
	miaoshouhuichun, //妙手回春
	haidilaoyue, //海底捞月
	gangshangkaihua, //杠上开花
	qiangganghu, //抢杠胡
	hujuezhang, //和绝张
	zimo, //自摸
	huapai, //花牌
	baoting, //报听

	max_compose_type_size
};

namespace std {
	template<>
	struct hash<ComposeType>
	{
		typedef ComposeType argument_type;
		typedef size_t result_type;

		result_type operator () (const argument_type& x) const
		{
			using type = typename std::underlying_type<argument_type>::type;
			return std::hash<type>()(static_cast<type>(x));
		}
	};
}

//麻将选项类型
enum MjType
{
	mj_invalid = 0,
	mj_left_chi = 1 << 1, //2
	mj_center_chi = 1 << 2, //4
	mj_right_chi = 1 << 3, //8
	mj_peng = 1 << 4, //16
	mj_an_gang = 1 << 5, //32
	mj_dian_gang = 1 << 6, //64
	mj_jia_gang = 1 << 7, //128
	mj_ting = 1 << 8, //256
	mj_hu = 1 << 9, //512
	mj_max
};

//动作类型
enum MjAction
{
	action_invalid = 0,
	action_chi = 1 << 1,
	action_peng = 1 << 2,
	action_gang = 1 << 3,
	action_ting = 1 << 4,
	action_hu = 1 << 5,
	action_out_card = 1 << 6,
	action_replace_hua = 1 << 7,
	action_max
};

//胡法类型
enum HuType
{
	hu_invalid = 0,
	hu_tianhu = 1 << 0, //天胡
	hu_dihu = 1 << 1, //地胡
	hu_renhu = 1 << 2, //人胡
	hu_tianting = 1 << 3, //天听
	hu_baoting = 1 << 4, //报听
	hu_miaoshouhuichun = 1 << 5, //妙手回春
	hu_haidilaoyue = 1 << 6, //海底捞月
	hu_gangshangkaihua = 1 << 7, //杠上开花
	hu_qiangganghu = 1 << 8, //抢杠胡
	hu_juezhang = 1 << 9, //和绝张
	hu_zimo = 1 << 10, //自摸
	hu_max_size
};

//目标牌使用类型
enum TargetUseType
{
	target_use_invalid = 0,
	target_use_straight = 1 << 0, //顺牌
	target_use_repeat = 1 << 1, //刻牌
	target_use_eye = 1 << 2, //将
	target_use_max
};

//目标牌类型
enum TargetType
{
	target_invalid = 0,
	target_out = 1, //出牌
	target_jia_gang = 2, //加杠牌
	target_max
};

extern const std::string CARDS_STR[];
extern const std::string CARAD_COLORS[];
extern const int32_t CARDS[];

//固定牌型
extern const array<int32_t, TABLE_SIZE> TABLE_JIULIANBAODENG;	//九莲宝灯

enum class ThreeType : uint32_t
{
	invalid = 0,
	straight, //顺牌
	repeat, //刻牌
};

//目标牌信息
struct TargetInfo
{
	int32_t card;
	TargetType type;
	int32_t chair;
};

//吃碰杠
struct CPG {
	MjType type;
	vector<int32_t> cards;
	int32_t targetCard;
	uint32_t targetChair;
};

//一副牌, 刻牌或者连牌
struct ThreeCard {
	array<int32_t, 3> cards;
	ThreeType type;
};

//组合牌 M*ThreeCard+1将牌 M=[0,4]
struct ComposeCard {
	ComposeCard() {
		eye[0] = eye[1] = GAME_INVALID_CARD;
	}
	vector<ThreeCard> threes;
	int32_t eye[2];
};

//用于做搜索组合的map
using ComposeCardMap = unordered_map<string, ComposeCard>;

//组合
struct Compose {
	ComposeCard composeCard; //组合牌
	vector<ComposeType> types; //牌型
	uint32_t times; //番数
};

//听信息
struct TingInfo {
	int32_t card; //听牌
	uint32_t num; //听牌剩余张数
	Compose compose; //每张听牌对应一个组合
};

//听操作信息
struct TingOptInfo {
	int32_t outCard; //出牌
	vector<TingInfo> tingInfos; //听列表
};

//牌型信息
struct ComposeTypeInfo {
	ComposeType type; //牌型
    MutuallyType mutuallytype;//三大类型
	string desc; //描述
	uint32_t times; //倍数
	vector<ComposeType> mutexes; //不计
};

//麻将信息 用于算法接口参数
struct MjInfo {
	const vector<CPG>& cpgs;
	const vector<int32_t>& cards;
	const int32_t targetCard;
	const uint32_t huaNum;
	const uint32_t huType;
	const vector<TingInfo>& tingInfos;
};

//牌型检测函数类型
using ComposeTypeCheckFunc = std::function<uint32_t(MjInfo& mjInfo, const ComposeCard& composeCard)>;
//牌型检测器
struct ComposeTypeChecker {
	ComposeTypeInfo checkInfo; //检测的牌型信息
	ComposeTypeCheckFunc checkFunc; //检测函数
};
//牌型检测器列表
extern const vector<ComposeTypeChecker> composeTypeCheckers;

//胡牌概率信息
struct PrInfo {
	PrInfo() {
		outCard = GAME_INVALID_CARD;
		need = 0;
		probability = 0;
	}
	//最佳出牌
	int32_t outCard;
	////一种可能的胡牌
	//vector<int32_t> onePrHuCards;
	////一种可能的需要牌
	//vector<int32_t> onePrNeedCards;
	//所有可能的需要牌
	vector<int32_t> needCards;
	//最少需要的张数
	int32_t need;
	//可能性总数
	int64_t probability;
};
//胡牌表胡牌概率信息
struct TablePrInfo {
	array<int32_t, 9> huTable;
	array<int32_t, 9> needTable;
	int32_t need;
	int64_t probability;
};

//是否合法的手牌总数
bool isValidHandSum(int32_t sum);
uint32_t getCardIdx(int32_t card);
int32_t getCardByIdx(uint32_t idx);
std::string getCardStr(int32_t card);
uint32_t getColor(int32_t card);
std::string getColorStr(int32_t card);
int32_t getCardValue(int32_t card);
bool isValidCard(int32_t card);
bool isChiType(MjType type);
bool isGangType(MjType type);

//牌转换成表.表定义:下标表示对应牌，值表示牌有几张，比如我有2个1万，则cards[1]=2
void cardsToTable(const vector<int32_t>& cards, array<int32_t, TABLE_SIZE>& table);
void tableToCards(const array<int32_t, TABLE_SIZE>& table, vector<int32_t>& cards);

//是否能胡
bool canHu(const vector<int32_t>& cards, int32_t targetCard, vector<ComposeCard>& composeCards);
bool canHu(array<int32_t, TABLE_SIZE>& table, vector<ComposeCard>& composeCards);
//查找听
vector<TingOptInfo>& findTingOptInfos(MjInfo& mjInfo, vector<TingOptInfo>& tingInfos);
//查找吃碰杠
vector<CPG>& findOptionalCpgs(MjInfo& mjInfo, vector<CPG>& optionalCpgs, uint32_t findOption);
//更新听后的选项
void updateCpgsAfterTing(MjInfo& mjInfo, vector<CPG>& optionalCpgs);
//查找最大牌型组合
Compose& findMaxCompose(MjInfo& mjInfo, const vector<ComposeCard>& composeCards, Compose& maxCompose);
void addComposeType(vector<ComposeType>& composeTypes, ComposeType composeType);
uint32_t getComposeTypeCheckerIdx(ComposeType type);
//获取牌型倍数
uint32_t getTimes(uint32_t huaNum, vector<ComposeType>& composeTypes);

//查找吃碰杠基础接口
vector<CPG>& findChi(const vector<int32_t>& cards, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findChiInTable(array<int32_t, TABLE_SIZE>& table, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findPeng(const vector<int32_t>& cards, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findPengInTable(array<int32_t, TABLE_SIZE>& table, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findAnGang(const vector<int32_t>& cards, vector<CPG>& optionalCpgs);
vector<CPG>& findAnGangInTable(array<int32_t, TABLE_SIZE>&, vector<CPG>& optionalCpgs);
vector<CPG>& findDianGang(const vector<int32_t>& cards, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findDianGangInTable(array<int32_t, TABLE_SIZE>& table, int32_t targetCard, vector<CPG>& optionalCpgs);
vector<CPG>& findJiaGang(const vector<int32_t>& cards, const vector<CPG>& cpgs, vector<CPG>& optionalCpgs);
vector<CPG>& findJiaGangInTable(array<int32_t, TABLE_SIZE>& table, const vector<CPG>& cpgs, vector<CPG>& optionalCpgs);

//牌型算法基础接口
vector<vector<int32_t>>& combination(vector<vector<int32_t>>& idxCombinations, const uint32_t choose, const uint32_t from);
vector<string>& combination(vector<string>& strs, const uint32_t& choose, const uint32_t& from);
bool isRepeatThree(const ThreeCard& three, int32_t card);
bool isRepeatThree(const ThreeCard& three, int32_t card_start, int32_t card_end);
bool isYao9Card(int32_t card);
bool isYaoCard(int32_t card);
bool isYao9Three(const ThreeCard& three);
bool isYaoThree(const ThreeCard& three);
vector<ThreeCard>& findRepeat(const vector<CPG>& cpgs, const ComposeCard& composeCard, int32_t card_start, int32_t card_end, vector<ThreeCard>& repeatThrees);
vector<ThreeCard>& findCloseRepeat(const vector<CPG>& cpgs, const ComposeCard& composeCard, int32_t card_start, int32_t card_end, vector<ThreeCard>& repeatThrees);
vector<ThreeCard>& findStraight(const vector<CPG>& cpgs, const ComposeCard& composeCard, vector<ThreeCard>& straightThrees);
vector<ThreeCard>& findThree(const vector<CPG>& cpgs, const ComposeCard& composeCard, vector<ThreeCard>& threes);
vector<int32_t>& findAllCards(MjInfo& mjInfo, vector<int32_t>& allCards);
uint32_t getGangNum(const vector<CPG>& cpgs);
uint32_t getTargetUseType(const ComposeCard& composeCard, const vector<int32_t>& cards, int32_t targetCard);
//般逢老连计番处理
void CalculateBFLL(MjInfo& mjInfo, const ComposeCard& composeCard, vector<ComposeType>& composeTypes);

//风牌类
uint32_t isDaSiXi(MjInfo& mjInfo, const ComposeCard& composeCard); //大四喜
uint32_t isXiaoSiXi(MjInfo& mjInfo, const ComposeCard& composeCard); //小四喜
uint32_t isDaSanFeng(MjInfo& mjInfo, const ComposeCard& composeCard); //大三风
uint32_t isXiaoSanFeng(MjInfo& mjInfo, const ComposeCard& composeCard); //小三风

//箭牌类
uint32_t isDaSanYuan(MjInfo& mjInfo, const ComposeCard& composeCard); //大三元
uint32_t isXiaoSanYuan(MjInfo& mjInfo, const ComposeCard& composeCard); //小三元
uint32_t isShuangJianKe(MjInfo& mjInfo, const ComposeCard& composeCard); //双箭刻
uint32_t isJianKe(MjInfo& mjInfo, const ComposeCard& composeCard); //箭刻

//四只归一系列
uint32_t isSiGang(MjInfo& mjInfo, const ComposeCard& composeCard); //四杠
uint32_t isSanGang(MjInfo& mjInfo, const ComposeCard& composeCard); //三杠
uint32_t isShuangAnGang(MjInfo& mjInfo, const ComposeCard& composeCard); //双暗杠
uint32_t isAnGang(MjInfo& mjInfo, const ComposeCard& composeCard); //暗杠
uint32_t isShuangMingGang(MjInfo& mjInfo, const ComposeCard& composeCard); //双明杠
uint32_t isMingGang(MjInfo& mjInfo, const ComposeCard& composeCard); //明杠
uint32_t isSiGuiYi(MjInfo& mjInfo, const ComposeCard& composeCard); //四归一

//暗刻类
uint32_t isSiAnKe(MjInfo& mjInfo, const ComposeCard& composeCard); //四暗刻
uint32_t isSanAnKe(MjInfo& mjInfo, const ComposeCard& composeCard); //三暗刻
uint32_t isShuangAnKe(MjInfo& mjInfo, const ComposeCard& composeCard); //双暗刻

//其他刻类
uint32_t isSiZiKe(MjInfo& mjInfo, const ComposeCard& composeCard); //四字刻

//连刻类
bool isLianKe(const vector<ThreeCard>& repeatThrees); //连刻
uint32_t isSiLianKe(MjInfo& mjInfo, const ComposeCard& composeCard); //四连刻
uint32_t isSanLianKe(MjInfo& mjInfo, const ComposeCard& composeCard); //三连刻

//步高类
uint32_t isBuGao(const vector<CPG>& cpgs, const ComposeCard& composeCard, uint32_t len, uint32_t min_diff_value, uint32_t max_diff_value); //步高
uint32_t isSiBuGao(MjInfo& mjInfo, const ComposeCard& composeCard); //四步高
uint32_t isSanBuGao(MjInfo& mjInfo, const ComposeCard& composeCard); //三步高

//同顺类
bool isTongShun(const vector<CPG>& cpgs, const ComposeCard& composeCard, uint32_t len); //同顺
uint32_t isSiTongShun(MjInfo& mjInfo, const ComposeCard& composeCard); //四同顺
uint32_t isSanTongShun(MjInfo& mjInfo, const ComposeCard& composeCard); //三同顺
uint32_t isYiBanGao(MjInfo& mjInfo, const ComposeCard& composeCard); //一般高

//老少类
uint32_t isLaoShaoFu(MjInfo& mjInfo, const ComposeCard& composeCard); //老少副

//幺九类
uint32_t isHunYaoJiu(MjInfo& mjInfo, const ComposeCard& composeCard); //混幺九
uint32_t isQuanDaiYao(MjInfo& mjInfo, const ComposeCard& composeCard); //全带幺
uint32_t isDuanYao(MjInfo& mjInfo, const ComposeCard& composeCard); //断幺

//牌色组合类
uint32_t isZiYiSe(MjInfo& mjInfo, const ComposeCard& composeCard); //字一色
uint32_t isQingYiSe(MjInfo& mjInfo, const ComposeCard& composeCard); //清一色
uint32_t isHunYiSe(MjInfo& mjInfo, const ComposeCard& composeCard); //混一色

//龙类
uint32_t isQingLong(MjInfo& mjInfo, const ComposeCard& composeCard); //清龙
uint32_t isLianLiu(MjInfo& mjInfo, const ComposeCard& composeCard); //连六

//包含类
uint32_t isDaYuWu(MjInfo& mjInfo, const ComposeCard& composeCard); //大于五
uint32_t isXiaoYuWu(MjInfo& mjInfo, const ComposeCard& composeCard); //小于五
uint32_t isShuangLongHui(MjInfo& mjInfo, const ComposeCard& composeCard); //双龙会
uint32_t isJiuLianBaoDeng(MjInfo& mjInfo, const ComposeCard& composeCard); //九莲宝灯

//门前类
uint32_t isQuanQiuRen(MjInfo& mjInfo, const ComposeCard& composeCard); //全求人
uint32_t isBuQiuRen(MjInfo& mjInfo, const ComposeCard& composeCard); //不求人
uint32_t isMenQianQing(MjInfo& mjInfo, const ComposeCard& composeCard); //门前清

//牌型结构类
// 1普通类:碰碰和，平和
// 2七对类：七对
uint32_t isPengPengHu(MjInfo& mjInfo, const ComposeCard& composeCard); //碰碰和
uint32_t isPingHu(MjInfo& mjInfo, const ComposeCard& composeCard); //平和
int32_t GetEyeNum(array<int32_t, TABLE_SIZE>& table);
bool isTableQiDui(array<int32_t, TABLE_SIZE>& table);
uint32_t isQiDui(MjInfo& mjInfo, const ComposeCard& composeCard); //七对子
uint32_t isDaQiXing(MjInfo& mjInfo, const ComposeCard& composeCard); //大七星
uint32_t isLianQiDui(MjInfo& mjInfo, const ComposeCard& composeCard); //连七对
uint32_t isSanYuanQiDui(MjInfo& mjInfo, const ComposeCard& composeCard); //三元七对子
uint32_t isSiXiQiDui(MjInfo& mjInfo, const ComposeCard& composeCard); //四喜七对子

//和牌方式类
uint32_t isTianHu(MjInfo& mjInfo, const ComposeCard& composeCard); //天胡
uint32_t isDiHu(MjInfo& mjInfo, const ComposeCard& composeCard); //地胡
uint32_t isRenHu(MjInfo& mjInfo, const ComposeCard& composeCard); //人胡
uint32_t isTianTing(MjInfo& mjInfo, const ComposeCard& composeCard); //天听
uint32_t isBaoTing(MjInfo& mjInfo, const ComposeCard& composeCard); //报听
uint32_t isHuJueZhang(MjInfo& mjInfo, const ComposeCard& composeCard); //和绝张
uint32_t isMiaoShouHuiChun(MjInfo& mjInfo, const ComposeCard& composeCard); //妙手回春
uint32_t isHaiDiLaoYue(MjInfo& mjInfo, const ComposeCard& composeCard); //海底捞月
uint32_t isGangShangKaiHua(MjInfo& mjInfo, const ComposeCard& composeCard); //杠上开花
uint32_t isQiangGangHu(MjInfo& mjInfo, const ComposeCard& composeCard); //抢杠胡
uint32_t isZiMo(MjInfo& mjInfo, const ComposeCard& composeCard); //自摸
uint32_t isQuanHua(MjInfo& mjInfo, const ComposeCard& composeCard); //全花
uint32_t isHuaPai(MjInfo& mjInfo, const ComposeCard& composeCard); //花牌

//听牌方式类
uint32_t isDanDiaoJiang(MjInfo& mjInfo, const ComposeCard& composeCard); //单钓将
uint32_t isBianZhang(MjInfo& mjInfo, const ComposeCard& composeCard); //边张
uint32_t isKanZhang(MjInfo& mjInfo, const ComposeCard& composeCard); //砍张

//AI算法
PrInfo& findPrInfo(const vector<int32_t>& cards, const vector<int32_t> outCardsExclude, array<int32_t, TABLE_SIZE>& leftTable, PrInfo& fastHuPrInfo);
PrInfo& findHandCardsPrInfo(const vector<int32_t>& handCards, PrInfo& fastHuPrInfo);

//Ai基础接口
double c_n_m(int8_t n, int8_t m);
void calculateFastestHuPrInfo(int32_t handNum, vector<array<int32_t, 9>>& handPartTables, vector<array<int32_t, 9>>& leftPartTables, int32_t eyeNum, PrInfo& fastestHuPrInfo);
int32_t GetZiCardNum(vector<int32_t>& cards);

//发牌控制
vector<int32_t> GetGoodHandCards(vector<int32_t>& cards, int32_t num, vector<int8_t>& part);
vector<int32_t> GetBadHandCards(vector<int32_t>& cards, int32_t num);
vector<int32_t> GetHandCardsCtrl(vector<int32_t>& cards, int32_t num, vector<int8_t>& part, bool good);
