#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "IAicsdragon.h"
#include "ini/INIHelp.h"
#include "PathMagic.h"
#include "weights.h"


#define INVALID_CHAIR (0xFFFF)

#define GAME_PLAYER	10001

std::string CNGateString(int i) {
	switch (i)
	{
	case eDGGateCode::DGGATE_0: return "和";
	case eDGGateCode::DGGATE_1: return "龙";
	case eDGGateCode::DGGATE_2: return "虎";
	default:
		break;
	}
	return "";
}

std::string CNPokerValue(uint8 pocket) {
	switch (pocket)
	{
	case enumPocketType::PKTYPE_A: return "A";
	case enumPocketType::PKTYPE_J: return "J";
	case enumPocketType::PKTYPE_Q: return "Q";
	case enumPocketType::PKTYPE_K: return "K";
	default: {
		char ch[3] = { 0 };
		sprintf(ch, "%d", pocket + 1);
		return ch;
	}
	}
	return "";
}

int roomID = 9001;
int tableID = 0;

void openLog(const char *p, ...)
{
	time_t tt;
	time(&tt);
	struct tm* tm = localtime(&tt);
	//aicslog
	char aicslog[256] = { 0 };
	snprintf(aicslog, sizeof(aicslog), "./aicslog_%04d",
		roomID);
	//date
	char date[256] = { 0 };
	snprintf(date, sizeof(date), "%04d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	//filename
	char filename[256] = { 0 };
	snprintf(filename, sizeof(filename), "room%04d-tab%04d.2.log",
		roomID,
		tableID);
	
	//filepath=aicslog/date/filename
	char filepath[1024] = { 0 };
	lstrcat(filepath, aicslog);
	lstrcat(filepath, SPLIT_SLASH);
	lstrcat(filepath, date);

	static char lastdate[32] = { 0 };
	if (lstrcmpi(filepath, lastdate) != 0) {
		CPathMagic::CreateDirectory(filepath);
	}

	lstrcat(filepath, SPLIT_SLASH);
	lstrcat(filepath, filename);
	FILE *fp = fopen(filepath, "a");
	if (!fp) {
		assert(false);
		return;
	}
	va_list arg;
	va_start(arg, p);
	fprintf(fp, "[%d%d%d %02d:%02d:%02d]", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	vfprintf(fp, p, arg);
	fprintf(fp, "\n");
	fflush(fp);
	fclose(fp);
}

#if 0
int testAicsDragon()
{
	////////////////////////////////////////
	//获取接口，房间dll加载初始化获取
	IAicsDragonEngine* sp = GetAicsDragonEngine();
	if (!sp) {
		printf("GetAicsDragonEngine failed\n");
		return -1;
	}
	////////////////////////////////////////
	//初始化接口
	tagDGSettings gs = { 0 };
	gs.nToubbili = 1;				//转换比例(1:1)
	gs.nMaxPlayer = GAME_PLAYER;	//桌面的玩家数量
	gs.nMaxTable = 3;				//当前房间桌子数
	int status = sp->Init(&gs);
	if (sOk != status) {
		printf("sp->Init status:[%d]\n", status);
		return -1;
	}
	////////////////////////////////////////
	//选择房间
	int roomID = 9001;
	status = sp->SetRoomID(roomID);
	if (sOk != status) {
		printf("sp->SetRoomID:[%d] status:[%d]\n", roomID, status);
		return -1;
	}
	int i = -1;
	////////////////////////////////////////
	//庄家累计赢分
	double dblBankerWinner = 0;
	////////////////////////////////////////
	//算法重置，清零数据，庄家本轮赢分
	double dblBankerWinnerRound = 0;
	////////////////////////////////////////
	//记录当前桌台初始流水号
	uint32 savedseqid = sp->GetTableSeqID(0);
	do {
		////////////////////////////////////////
		//游戏进行到第几局
		++i;
		////////////////////////////////////////
		WORD banker = INVALID_CHAIR;
		////////////////////////////////////////
		//构造输入输出参数
		tagDGGateInfo info = { 0 };
		////////////////////////////////////////
		//玩家每门(和/龙/虎)总下注
		int scorePlaced[DGGATE_MAX] = { 300,1000,2000 };
		////////////////////////////////////////
		//机器人每门(和/龙/虎)总下注
		int robotPlaced[DGGATE_MAX] = { 600,1500,3000 };
		////////////////////////////////////////
		//玩家庄
		//banker = 1;
		////////////////////////////////////////
		//系统庄
		info.bisUserBanker = (INVALID_CHAIR != banker);
		////////////////////////////////////////
		//玩家和机器人押注积分
		for (int i = 0; i < DGGATE_MAX; ++i) {
			//玩家每门(和/龙/虎)总下注
			tagDGGateIn& in = info.in[i];
			//玩家每门(和/龙/虎)总共下了多少分
			in.lscorePlaced = scorePlaced[i];
			in.sign = AICSDG_SIGN;
			//机器人每门(和/龙/虎)总下注
			tagDGGateIn& bot = info.botin[i];
			//机器人每门(和/龙/虎)总共下了多少分
			bot.lscorePlaced = robotPlaced[i];
			bot.sign = AICSDG_SIGN;
		}
		////////////////////////////////////////
		//计算押注结果
		status = sp->dragonOnebk(0, 0, 0, &info);
		if (status < 0) {
			printf("dragonOnebk call failed, status:[%d]\n", status);
			return -1;
		}
		////////////////////////////////////////
		//0-中和门 1-中龙门 2-中虎门
		assert(int(info.outwingate) >= DGGATE_0);
		assert(int(info.outwingate) <= DGGATE_2);
		////////////////////////////////////////
		//庄家本局赢分
		double dblCurrentWinner = 0;
		////////////////////////////////////////
		//押中和门
		if (DGGATE_0 == info.outwingate) {
			//庄家输分：玩家押和总注数 * 8倍赔率
			dblCurrentWinner -= info.in[DGGATE_0].lscorePlaced * 8.0f;
		}
		////////////////////////////////////////
		//押中龙门或虎门
		else {
			//玩家押三门总注数：压和总下注 + 压龙总下注 + 压虎总下注
			dblCurrentWinner += info.in[DGGATE_0].lscorePlaced + info.in[DGGATE_1].lscorePlaced + info.in[DGGATE_2].lscorePlaced;
			//庄家赢分：玩家押三门总注数 - 押中门玩家总注数 * 2倍(比如押中虎，300 + 1000 + 2000 - 2 * 2000)
			dblCurrentWinner -= info.in[info.outwingate].lscorePlaced * 2.0f;
		}
		////////////////////////////////////////
		//庄家累计赢分
		dblBankerWinner += dblCurrentWinner;
		////////////////////////////////////////
		//庄家本轮赢分
		dblBankerWinnerRound += dblCurrentWinner;
		//sign标识当前整个流程，整个流程走完则更新
		//assert(info.out[DGGATE_0].sign == info.out[DGGATE_1].sign);
		//assert(info.out[DGGATE_0].sign == info.out[DGGATE_2].sign);
		printf("\n\n--- *** =========================== 房间[%d]桌子[%d]流水号[%d] ===========================\n",
			roomID, 0, sp->GetTableSeqID(0));
		if (info.bisUserBanker) {
			printf("--- *** sign:[%x] 第[%d]局 玩家庄 - 龙[%s] 和[%d] 虎[%s] 中[%s]门\n",
				info.out[DGGATE_0].sign,
				i + 1,
				CNPokerValue(info.out[DGGATE_1].pocket).c_str(),
				info.out[DGGATE_0].pocket,
				CNPokerValue(info.out[DGGATE_2].pocket).c_str(),
				CNGateString(info.outwingate).c_str());
		}
		else {
			//系统庄，影响算法结果因素：真实玩家下注和累计得分
			//系统会一直累计得分很多局，直到整个流程走完自动清零，重新开始，
			//系统会不断调整整个流程总局数，直到满足设定的盈利条件就清零，落袋为安，重新开始
			//游戏每局累计得分写入磁盘，若游戏中异常关闭，会丢失当前这一局还没来得及写的数据
			printf("--- *** sign:[%x] 第[%d]局 系统庄 - 龙[%s] 和[%d] 虎[%s] 中[%s]门\n",
				info.out[DGGATE_0].sign,
				i + 1,
				CNPokerValue(info.out[DGGATE_1].pocket).c_str(),
				info.out[DGGATE_0].pocket,
				CNPokerValue(info.out[DGGATE_2].pocket).c_str(),
				CNGateString(info.outwingate).c_str());
		}
		assert(info.outwingate >= DGGATE_0 &&
			   info.outwingate <= DGGATE_2);
		assert(info.out[info.outwingate].iswin > 0);
		if (info.outwingate == DGGATE_0) {
			assert(info.out[DGGATE_1].iswin == 0);
			assert(info.out[DGGATE_2].iswin == 0);
		}
		else if (info.outwingate == DGGATE_1) {
			assert(info.out[DGGATE_0].iswin == 0);
			assert(info.out[DGGATE_2].iswin == 0);
		}
		else if (info.outwingate == DGGATE_2) {
			assert(info.out[DGGATE_0].iswin == 0);
			assert(info.out[DGGATE_1].iswin == 0);
		}
		printf("--- *** 所有<玩家>下注\n");
		printf("--- *** \t\t和[%0.02lf]赔率[%0.02lf]\n",
			info.in[DGGATE_0].lscorePlaced,
			info.out[DGGATE_0].multi);
		printf("--- *** 龙[%0.02lf]赔率[%0.02lf]\t虎[%0.02lf]赔率[%0.02lf]\n",
			info.in[DGGATE_1].lscorePlaced,
			info.out[DGGATE_1].multi,
			info.in[DGGATE_2].lscorePlaced,
			info.out[DGGATE_2].multi);
		printf("--- *** 所有<机器人>下注\n");
		printf("--- *** \t\t和[%0.02lf]赔率[%0.02lf]\n",
			info.botin[DGGATE_0].lscorePlaced,
			info.out[DGGATE_0].multi);
		printf("--- *** 龙[%0.02lf]赔率[%0.02lf]\t虎[%0.02lf]赔率[%0.02lf]\n",
			info.botin[DGGATE_1].lscorePlaced,
			info.out[DGGATE_1].multi,
			info.botin[DGGATE_2].lscorePlaced,
			info.out[DGGATE_2].multi);
		printf("--- *** 以上所有总下注\n");
		printf("--- *** \t\t和[%0.02lf]赔率[%0.02lf]此门嬴分[%0.02lf]\n",
			info.in[DGGATE_0].lscorePlaced + info.botin[DGGATE_0].lscorePlaced,
			info.out[DGGATE_0].multi,
			info.out[DGGATE_0].lscoreWinned);
		printf("--- *** 龙[%0.02lf]赔率[%0.02lf]此门嬴分[%0.02lf]\t虎[%0.02lf]赔率[%0.02lf]此门嬴分[%0.02lf]\n",
			info.in[DGGATE_1].lscorePlaced + info.botin[DGGATE_1].lscorePlaced,
			info.out[DGGATE_1].multi,
			info.out[DGGATE_1].lscoreWinned,
			info.in[DGGATE_2].lscorePlaced + info.botin[DGGATE_2].lscorePlaced,
			info.out[DGGATE_2].multi,
			info.out[DGGATE_2].lscoreWinned);
		printf("--- *** 游戏局数[%d] 庄家本局赢分[%2.2f] 累计赢分[%2.2f]\n",
			i + 1,
			dblCurrentWinner,
			dblBankerWinner);
		if (sp->IsTableReset(0)) {
			////////////////////////////////////////
			//本次流程局数
			uint32 round = sp->GetTableSeqID(0) - savedseqid;
			////////////////////////////////////////
			//记录当前桌台初始流水号
			savedseqid = sp->GetTableSeqID(0);
			printf("--- *** 算法重置清零数据，本轮局数[%d] 本轮赢分[%2.2f]\n", round, dblBankerWinnerRound);
			dblBankerWinnerRound = 0;
			if ('q' == getchar()) {
				break;
			}
		}
	} while (true);
	printf("exit.\n");
	return 0;
}
#else
int testAicsDragon()
{
	////////////////////////////////////////
	//读取配置
	CINIHelp ini;
	ini.Open("AicsDragonEngineTest_linux.ini");
	std::string srand0(ini.GetString("scorePlaced", "DGGATE_he"));
	std::string srand1(ini.GetString("scorePlaced", "DGGATE_long"));
	std::string srand2(ini.GetString("scorePlaced", "DGGATE_hu"));
	size_t npos = srand0.find_first_of(',');
	std::string min0 = srand0.substr(0, npos);
	std::string max0 = srand0.substr(npos + 1, -1);
	npos = srand1.find_first_of(',');
	std::string min1 = srand1.substr(0, npos);
	std::string max1 = srand1.substr(npos + 1, -1);
	npos = srand2.find_first_of(',');
	std::string min2 = srand2.substr(0, npos);
	std::string max2 = srand2.substr(npos + 1, -1);
	////////////////////////////////////////
	//获取接口，房间dll加载初始化获取
	IAicsDragonEngine* sp = GetAicsDragonEngine();
	if (!sp) {
		printf("GetAicsDragonEngine failed\n");
		return -1;
	}
	////////////////////////////////////////
	//初始化接口
	tagDGSettings gs = { 0 };
	gs.nToubbili = 1;				//转换比例(1:1)
	gs.nMaxPlayer = GAME_PLAYER;	//桌面的玩家数量
	gs.nMaxTable = 3;				//当前房间桌子数
	int status = sp->Init(&gs);
	if (sOk != status) {
		printf("sp->Init status:[%d]\n", status);
		return -1;
	}
	////////////////////////////////////////
	//选择房间
	//int roomID = 9001,tableID = 0;
	status = sp->SetRoomID(roomID);
	if (sOk != status) {
		printf("sp->SetRoomID:[%d] status:[%d]\n", roomID, status);
		return -1;
	}

	int i = -1, j = 0;
	////////////////////////////////////////
	//庄家累计赢分
	double dblBankerWinner = 0;
	////////////////////////////////////////
	//算法重置，清零数据，庄家本轮赢分
	double dblBankerWinnerRound = 0;
	////////////////////////////////////////
	//记录当前桌台初始流水号
	uint32 savedseqid = sp->GetTableSeqID(tableID);
	////////////////////////////////////////
	openLog("%-9s%-9s%-9s%-13s%-10s%-12s%-12s%-12s" \
		"%-12s%-12s%-11s%-5s%-4s%-9s%-12s%-12s" \
		"%-12s%-14s%-12s%-9s%-14s%-15s",
		"房号", "桌号", "局数", "流水号", "庄ID", "和注", "龙注", "虎注",
		"AI和注", "AI龙注", "AI虎注", "龙", "虎", "中门", "和赢", "龙赢",
		"虎赢", "本局赢分", "累计", "轮数", "本轮局数", "赢分变化");
	do {
		////////////////////////////////////////
		//游戏进行到第几局
		++i;
		////////////////////////////////////////
		WORD banker = INVALID_CHAIR;
		////////////////////////////////////////
		//构造输入输出参数
		tagDGGateInfo info = { 0 };
		////////////////////////////////////////
		//玩家每门(和/龙/虎)总下注
		int scorePlaced[DGGATE_MAX] = {
			RandomBetween(atoi(min0.c_str()),atoi(max0.c_str())),
			RandomBetween(atoi(min1.c_str()),atoi(max1.c_str())),
			RandomBetween(atoi(min2.c_str()),atoi(max2.c_str())),
		};
		////////////////////////////////////////
		//机器人每门(和/龙/虎)总下注
		int robotPlaced[DGGATE_MAX] = { 600,1500,3000 };
		////////////////////////////////////////
		//玩家庄
		//banker = 1;
		////////////////////////////////////////
		//系统庄
		info.bisUserBanker = (INVALID_CHAIR != banker);
		////////////////////////////////////////
		//玩家和机器人押注积分
		for (int i = 0; i < DGGATE_MAX; ++i) {
			//玩家每门(和/龙/虎)总下注
			tagDGGateIn& in = info.in[i];
			//玩家每门(和/龙/虎)总共下了多少分
			in.lscorePlaced = scorePlaced[i];
			in.sign = AICSDG_SIGN;
			//机器人每门(和/龙/虎)总下注
			tagDGGateIn& bot = info.botin[i];
			//机器人每门(和/龙/虎)总共下了多少分
			bot.lscorePlaced = robotPlaced[i];
			bot.sign = AICSDG_SIGN;
		}
		////////////////////////////////////////
		//计算押注结果
		status = sp->dragonOnebk(0, tableID, 0, &info);
		if (status < 0) {
			printf("dragonOnebk call failed, status:[%d]\n", status);
			return -1;
		}
		////////////////////////////////////////
		//0-中和门 1-中龙门 2-中虎门
		assert(int(info.outwingate) >= DGGATE_0);
		assert(int(info.outwingate) <= DGGATE_2);
		////////////////////////////////////////
		//庄家本局赢分
		double dblCurrentWinner = 0;
		////////////////////////////////////////
		//押中和门
		if (DGGATE_0 == info.outwingate) {
			//庄家输分：玩家押和总注数 * 8倍赔率
			dblCurrentWinner -= info.in[DGGATE_0].lscorePlaced * 8.0f;
		}
		////////////////////////////////////////
		//押中龙门或虎门
		else {
			//玩家押三门总注数：压和总下注 + 压龙总下注 + 压虎总下注
			dblCurrentWinner += info.in[DGGATE_0].lscorePlaced + info.in[DGGATE_1].lscorePlaced + info.in[DGGATE_2].lscorePlaced;
			//庄家赢分：玩家押三门总注数 - 押中门玩家总注数 * 2倍(比如押中虎，300 + 1000 + 2000 - 2 * 2000)
			dblCurrentWinner -= info.in[info.outwingate].lscorePlaced * 2.0f;
		}
		////////////////////////////////////////
		//庄家累计赢分
		dblBankerWinner += dblCurrentWinner;
		////////////////////////////////////////
		//庄家本轮赢分
		dblBankerWinnerRound += dblCurrentWinner;
		assert(info.outwingate >= DGGATE_0 &&
			info.outwingate <= DGGATE_2);
		assert(info.out[info.outwingate].iswin > 0);
		if (info.outwingate == DGGATE_0) {
			assert(info.out[DGGATE_1].iswin == 0);
			assert(info.out[DGGATE_2].iswin == 0);
		}
		else if (info.outwingate == DGGATE_1) {
			assert(info.out[DGGATE_0].iswin == 0);
			assert(info.out[DGGATE_2].iswin == 0);
		}
		else if (info.outwingate == DGGATE_2) {
			assert(info.out[DGGATE_0].iswin == 0);
			assert(info.out[DGGATE_1].iswin == 0);
		}
		////////////////////////////////////////
		//本次流程局数
		uint32 round = sp->GetTableSeqID(tableID) - savedseqid;
		////////////////////////////////////////
		openLog("%-8d%-6d%-7d%-10d%-9d" \
			"%-10.02lf%-10.02lf%-10.02lf" \
			"%-10.02lf%-10.02lf%-10.02lf" \
			"%-3s%-3s%-8s" \
			"%-10.02lf%-10.02lf%-10.02lf" \
			"%-10.2f%-11.2f" \
			"%-10d%-7d%-8.2f",
			roomID, tableID, i + 1, sp->GetTableSeqID(tableID), banker,
			info.in[DGGATE_0].lscorePlaced, info.in[DGGATE_1].lscorePlaced, info.in[DGGATE_2].lscorePlaced,
			info.botin[DGGATE_0].lscorePlaced, info.botin[DGGATE_1].lscorePlaced, info.botin[DGGATE_2].lscorePlaced,
			CNPokerValue(info.out[DGGATE_1].pocket).c_str(), CNPokerValue(info.out[DGGATE_2].pocket).c_str(), CNGateString(info.outwingate).c_str(),
				info.out[DGGATE_0].lscoreWinned, info.out[DGGATE_1].lscoreWinned, info.out[DGGATE_2].lscoreWinned,
				dblCurrentWinner, dblBankerWinner,
				j + 1, round, dblBankerWinnerRound);
		if (sp->IsTableReset(tableID)) {
			////////////////////////////////////////
			//本次流程局数
			//uint32 round = sp->GetTableSeqID(tableID) - savedseqid;
			////////////////////////////////////////
			//记录当前桌台初始流水号
			savedseqid = sp->GetTableSeqID(tableID);
			time_t tt;
			time(&tt);
			struct tm* tm = localtime(&tt);
			printf("[%d%d%d %02d:%02d:%02d]算法重置清零数据，第%d轮 本轮局数[%d] 本轮赢分[%2.2f]\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
				j + 1, round, dblBankerWinnerRound);
			openLog("算法重置清零数据，第%d轮 本轮局数[%d] 本轮赢分[%2.2f]\n", j + 1, round, dblBankerWinnerRound);
			dblBankerWinnerRound = 0;
			++j;
			if ('q' == getchar()) {
				break;
			}
			////////////////////////////////////////
			openLog("%-9s%-9s%-9s%-13s%-10s%-12s%-12s%-12s" \
				"%-12s%-12s%-11s%-5s%-4s%-9s%-12s%-12s" \
				"%-12s%-14s%-12s%-9s%-14s%-15s",
				"房号", "桌号", "局数", "流水号", "庄ID", "和注", "龙注", "虎注",
				"AI和注", "AI龙注", "AI虎注", "龙", "虎", "中门", "和赢", "龙赢",
				"虎赢", "本局赢分", "累计", "轮数", "本轮局数", "赢分变化");
		}
	} while (true);
	printf("exit.\n");
	return 0;
}
#endif