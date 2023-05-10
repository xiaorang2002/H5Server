
#include <iostream>
#include <stdlib.h>
#include "../QPAlgorithm/PJ.h"
#include "../QPAlgorithm/cfg.h"
#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

#include <sstream>
#include <vector>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include "../public/StdRandom.h"

int main()
{
	std::vector<std::string> lines;
	//读取配置
	readFile("./conf/qzpj_cardList.ini", lines, ";;");
	//1->文件读取手牌 0->随机生成13张牌
	int flag = atoi(lines[0].c_str());
	//1->文件读取手牌 0->随机生成13张牌
	if (flag > 0) {
		PJ::CGameLogic g;
		uint8_t cards[32] = { 0 };
		uint8_t ty;
		int n = PJ::CGameLogic::MakeCardList(lines[4], cards, 32);
		//assert(n == 2);
		//打印n张牌
		PJ::CGameLogic::PrintCardList(cards, n);
		//手牌排序
		PJ::CGameLogic::SortCards(cards, n);
		//手牌牌型
		//ty = PJ::CGameLogic::GetHandCardsType(&cards[0]);
		//printf("手牌[%s] 牌型[%s]\n",
		//	PJ::CGameLogic::StringCards(&cards[0], MAX_COUNT).c_str(),
		//	PJ::CGameLogic::StringHandTy(PJ::HandTy(ty)).c_str());
	}
	return 0;
}
