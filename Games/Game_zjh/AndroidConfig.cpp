#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string> 
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/zjh.Message.pb.h"

#include "MSG_ZJH.h"
#include "../QPAlgorithm/zjh.h"
#include "../QPAlgorithm/cfg.h"

#include "AndroidConfig.h"
#include "GameProcess.h"

//更新机器人配置
void CGameProcess::UpdateConfig() {
	CINIHelp ini;
	ini.Open(INI_FILENAME);
	char val[20];
	//豹子
	{
		snprintf(val, sizeof(val), "%.0f|%.0f", robot_._30.first, robot_._30.second);
		ini.SetString("winProbability", "_30", val);
	}
	//同花顺
	{
		snprintf(val, sizeof(val), "%.0f|%.0f", robot_._123sc.first, robot_._123sc.second);
		ini.SetString("winProbability", "_123sc", val);
	}
	//同花
	{
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Asc.first, robot_._Asc.second);
			ini.SetString("winProbability", "_Asc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Ksc.first, robot_._Ksc.second);
			ini.SetString("winProbability", "_Ksc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Qsc.first, robot_._Qsc.second);
			ini.SetString("winProbability", "_Qsc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Jsc.first, robot_._Jsc.second);
			ini.SetString("winProbability", "_Jsc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Tsc.first, robot_._Tsc.second);
			ini.SetString("winProbability", "_Tsc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._9sc.first, robot_._9sc.second);
			ini.SetString("winProbability", "_9sc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._8sc.first, robot_._8sc.second);
			ini.SetString("winProbability", "_8sc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._7sc.first, robot_._7sc.second);
			ini.SetString("winProbability", "_7sc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._6sc.first, robot_._6sc.second);
			ini.SetString("winProbability", "_6sc", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._5sc.first, robot_._5sc.second);
			ini.SetString("winProbability", "_5sc", val);
		}
	}
	//顺子
	{
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._QKA.first, robot_._QKA.second);
			ini.SetString("winProbability", "_QKA", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._JQK.first, robot_._JQK.second);
			ini.SetString("winProbability", "_JQK", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._TJQ.first, robot_._TJQ.second);
			ini.SetString("winProbability", "_TJQ", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._9TJ.first, robot_._9TJ.second);
			ini.SetString("winProbability", "_9TJ", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._89T.first, robot_._89T.second);
			ini.SetString("winProbability", "_89T", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._789.first, robot_._789.second);
			ini.SetString("winProbability", "_789", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._678.first, robot_._678.second);
			ini.SetString("winProbability", "_678", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._567.first, robot_._567.second);
			ini.SetString("winProbability", "_567", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._456.first, robot_._456.second);
			ini.SetString("winProbability", "_456", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._345.first, robot_._345.second);
			ini.SetString("winProbability", "_345", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._234.first, robot_._234.second);
			ini.SetString("winProbability", "_234", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._A23.first, robot_._A23.second);
			ini.SetString("winProbability", "_A23", val);
		}
	}
	//对子
	{
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._AA.first, robot_._AA.second);
			ini.SetString("winProbability", "_AA", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._KK.first, robot_._KK.second);
			ini.SetString("winProbability", "_KK", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._QQ.first, robot_._QQ.second);
			ini.SetString("winProbability", "_QQ", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._JJ.first, robot_._JJ.second);
			ini.SetString("winProbability", "_JJ", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._TT.first, robot_._TT.second);
			ini.SetString("winProbability", "_TT", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._99.first, robot_._99.second);
			ini.SetString("winProbability", "_99", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._88.first, robot_._88.second);
			ini.SetString("winProbability", "_88", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._77.first, robot_._77.second);
			ini.SetString("winProbability", "_77", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._66.first, robot_._66.second);
			ini.SetString("winProbability", "_66", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._55.first, robot_._55.second);
			ini.SetString("winProbability", "_55", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._44.first, robot_._44.second);
			ini.SetString("winProbability", "_44", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._33.first, robot_._33.second);
			ini.SetString("winProbability", "_33", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._22.first, robot_._22.second);
			ini.SetString("winProbability", "_22", val);
		}
	}
	//散牌
	{
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._A.first, robot_._A.second);
			ini.SetString("winProbability", "_A", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._K.first, robot_._K.second);
			ini.SetString("winProbability", "_K", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._Q.first, robot_._Q.second);
			ini.SetString("winProbability", "_Q", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._J.first, robot_._J.second);
			ini.SetString("winProbability", "_J", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._10.first, robot_._10.second);
			ini.SetString("winProbability", "_T", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._9.first, robot_._9.second);
			ini.SetString("winProbability", "_9", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._8.first, robot_._8.second);
			ini.SetString("winProbability", "_8", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._7.first, robot_._7.second);
			ini.SetString("winProbability", "_7", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._6.first, robot_._6.second);
			ini.SetString("winProbability", "_6", val);
		}
		{
			snprintf(val, sizeof(val), "%.0f|%.0f", robot_._5.first, robot_._5.second);
			ini.SetString("winProbability", "_5", val);
		}
	}
}

//读取机器人配置
void  CGameProcess::ReadConfig() {
	CINIHelp ini;
	ini.Open(INI_FILENAME);
	//控制
	{
		//开启随时看牌
		std::string str1(ini.GetString("control", "freeLookover_"));
		freeLookover_ = atoi(str1.c_str());
		//开启随时弃牌
		std::string str2(ini.GetString("control", "freeGiveup_"));
		freeGiveup_ = atoi(str2.c_str());
		//启用三套决策(0,1,2)
		std::string str3(ini.GetString("control", "useStrategy_"));
		useStrategy_ = atoi(str3.c_str());
		//游戏结束是否全部摊牌
		std::string str4(ini.GetString("control", "openAllCards_"));
		openAllCards_ = atoi(str4.c_str());
		//判断特殊牌型
		std::string str5(ini.GetString("control", "special235_"));
		special235_ = atoi(str5.c_str());
	}
	//放大倍数
	{
		std::string str1(ini.GetString("winProbability", "scale"));
		robot_.scale = atoi(str1.c_str());
	}
	//看牌参数，权值倍率，修正参数(不知道什么jb玩意)
	for (int i = 0; i < 3; ++i) {
		std::string K_ = "K_" + to_string(i);
		std::string str1(ini.GetString("winProbability", K_.c_str()));
		robot_.param[i].K_[0] = atof(str1.substr(0, str1.find_first_of('|')).c_str());
		robot_.param[i].K_[1] = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		std::string W1_ = "W1_" + to_string(i);
		std::string str2(ini.GetString("winProbability", W1_.c_str()));
		robot_.param[i].W1_[0] = atof(str2.substr(0, str2.find_first_of('|')).c_str());
		robot_.param[i].W1_[1] = atof(str2.substr(str2.find_first_of('|') + 1, -1).c_str());
		std::string P1_ = "P1_" + to_string(i);
		std::string str3(ini.GetString("winProbability", P1_.c_str()));
		robot_.param[i].P1_ = atof(str3.c_str());
		std::string W2_ = "W2_" + to_string(i);
		std::string str4(ini.GetString("winProbability", W2_.c_str()));
		robot_.param[i].W2_[0] = atof(str4.substr(0, str4.find_first_of('|')).c_str());
		robot_.param[i].W2_[1] = atof(str4.substr(str4.find_first_of('|') + 1, -1).c_str());
		std::string P2_ = "P2_" + to_string(i);
		std::string str5(ini.GetString("winProbability", P2_.c_str()));
		robot_.param[i].P2_ = atof(str5.c_str());
	}
	//看牌概率
	for (int i = 0; i < 3; ++i) {
		std::string ratioKP_ = "ratioKP_" + to_string(i);
		std::string str1(ini.GetString("winProbability", ratioKP_.c_str()));
		robot_.ratioKP[i] = atoi(str1.c_str());
	}
	//制造各牌型的权重
	{
		std::string str1(ini.GetString("Probability", "ratioMadeCard_"));
		int i = 1, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioMadeCard_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioMadeCard_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//小黑屋个人库存控制的基本底分倍数
	{
		std::string str1(ini.GetString("Probability", "kcBaseTimes_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.kcBaseTimes_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.kcBaseTimes_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//小黑屋个人库存控制的概率
	{
		std::string str1(ini.GetString("Probability", "ratiokcBaseTimes_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratiokcBaseTimes_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratiokcBaseTimes_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}

	//每回合的弃牌概率
	string strIndex;
	for (int i = 1; i < 9; ++i) {
        strIndex = "ratioQiPai_"+to_string(i);
        std::string str1(ini.GetString("QiPaiProbability", strIndex.c_str()));
		int j = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioQiPai[i][j++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioQiPai[i][j++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//每回合的加注概率
	for (int i = 1; i < 9; ++i) {
		strIndex = "ratioJiaZhu_"+to_string(i);
		std::string str1(ini.GetString("JiaZhuProbability", strIndex.c_str()));
		int j = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioJiaZhu[i][j++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioJiaZhu[i][j++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}	
	//加注筹码概率
	for (int i = 1; i < 9; ++i) {
		strIndex = "ratioJiaZhuChip_"+to_string(i);
		std::string str1(ini.GetString("JiaZhuChipProbability", strIndex.c_str()));
		int j = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioJiaZhuChip[i][j++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioJiaZhuChip[i][j++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//每回合的比牌概率
	for (int i = 1; i < 9; ++i) {
		strIndex = "ratioBiPai_"+to_string(i);
		std::string str1(ini.GetString("BiPaiProbability", strIndex.c_str()));
		int j = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioBiPai[i][j++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioBiPai[i][j++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//收分时三套决策(0,1,2)几率
	{
		std::string str1(ini.GetString("winProbability", "SP0"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.SP0[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.SP0[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//正常时三套决策(0,1,2)几率
	{
		std::string str1(ini.GetString("winProbability", "SP1"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.SP1[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.SP1[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//放分时三套决策(0,1,2)几率
	{
		std::string str1(ini.GetString("winProbability", "SP2"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.SP2[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.SP2[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//豹子
	{
		std::string str1(ini.GetString("winProbability", "_30"));
		robot_._30.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
		robot_._30.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
	}
	//同花顺
	{
		std::string str1(ini.GetString("winProbability", "_123sc"));
		robot_._123sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
		robot_._123sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
	}
	//同花
	{
		{
			std::string str1(ini.GetString("winProbability", "_Asc"));
			robot_._Asc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Asc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_Ksc"));
			robot_._Ksc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Ksc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_Qsc"));
			robot_._Qsc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Qsc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_Jsc"));
			robot_._Jsc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Jsc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_Tsc"));
			robot_._Tsc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Tsc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_9sc"));
			robot_._9sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._9sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_8sc"));
			robot_._8sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._8sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_7sc"));
			robot_._7sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._7sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_6sc"));
			robot_._6sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._6sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_5sc"));
			robot_._5sc.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._5sc.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
	}
	//顺子
	{
		{
			std::string str1(ini.GetString("winProbability", "_QKA"));
			robot_._QKA.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._QKA.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_JQK"));
			robot_._JQK.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._JQK.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_TJQ"));
			robot_._TJQ.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._TJQ.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_9TJ"));
			robot_._9TJ.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._9TJ.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_89T"));
			robot_._89T.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._89T.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_789"));
			robot_._789.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._789.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_678"));
			robot_._678.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._678.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_567"));
			robot_._567.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._567.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_456"));
			robot_._456.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._456.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_345"));
			robot_._345.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._345.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_234"));
			robot_._234.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._234.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_A23"));
			robot_._A23.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._A23.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
	}
	//对子
	{
		{
			std::string str1(ini.GetString("winProbability", "_AA"));
			robot_._AA.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._AA.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_KK"));
			robot_._KK.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._KK.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_QQ"));
			robot_._QQ.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._QQ.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_JJ"));
			robot_._JJ.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._JJ.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_TT"));
			robot_._TT.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._TT.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_99"));
			robot_._99.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._99.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_88"));
			robot_._88.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._88.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_77"));
			robot_._77.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._77.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_66"));
			robot_._66.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._66.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_55"));
			robot_._55.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._55.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_44"));
			robot_._44.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._44.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_33"));
			robot_._33.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._33.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_22"));
			robot_._22.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._22.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
	}
	//散牌
	{
		{
			std::string str1(ini.GetString("winProbability", "_A"));
			robot_._A.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._A.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_K"));
			robot_._K.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._K.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_Q"));
			robot_._Q.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._Q.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_J"));
			robot_._J.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._J.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_T"));
			robot_._10.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._10.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_9"));
			robot_._9.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._9.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_8"));
			robot_._8.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._8.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_7"));
			robot_._7.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._7.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_6"));
			robot_._6.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._6.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
		{
			std::string str1(ini.GetString("winProbability", "_5"));
			robot_._5.first = atof(str1.substr(0, str1.find_first_of('|')).c_str());
			robot_._5.second = atof(str1.substr(str1.find_first_of('|') + 1, -1).c_str());
		}
	}
}

//牌型获取胜率和权值
std::pair<double, double>* CGameProcess::GetWinRatioAndWeight(uint32_t chairId) {
	ZJH::HandTy ty = ZJH::HandTy(handCardsType_[chairId]);
	switch (ty) {
	case ZJH::Ty30: {//豹子
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) <<" ["
			<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._30 ratioWin = " << robot_._30.first << " weight = " << robot_._30.second;
		return &robot_._30;
	}
	case ZJH::Ty123sc: {//同花顺
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
			<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._123sc ratioWin = " << robot_._123sc.first << " weight = " << robot_._123sc.second;
		return &robot_._123sc;
	}
	case ZJH::Tysc: {//同花
		if (ZJH::A == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Asc ratioWin = " << robot_._Asc.first << " weight = " << robot_._Asc.second;
			return &robot_._Asc;
		}
		else if (ZJH::K == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Ksc ratioWin = " << robot_._Ksc.first << " weight = " << robot_._Ksc.second;
			return &robot_._Ksc;
		}
		else if (ZJH::Q == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Qsc ratioWin = " << robot_._Qsc.first << " weight = " << robot_._Qsc.second;
			return &robot_._Qsc;
		}
		else if (ZJH::J == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Jsc ratioWin = " << robot_._Jsc.first << " weight = " << robot_._Jsc.second;
			return &robot_._Jsc;
		}
		else if (ZJH::T == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Tsc ratioWin = " << robot_._Tsc.first << " weight = " << robot_._Tsc.second;
			return &robot_._Tsc;
		}
		else if (9 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._9sc ratioWin = " << robot_._9sc.first << " weight = " << robot_._9sc.second;
			return &robot_._9sc;
		}
		else if (8 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._8sc ratioWin = " << robot_._8sc.first << " weight = " << robot_._8sc.second;
			return &robot_._8sc;
		}
		else if (7 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._7sc ratioWin = " << robot_._7sc.first << " weight = " << robot_._7sc.second;
			return &robot_._7sc;
		}
		else if (6 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._6sc ratioWin = " << robot_._6sc.first << " weight = " << robot_._6sc.second;
			return &robot_._6sc;
		}
		else if (5 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._5sc ratioWin = " << robot_._5sc.first << " weight = " << robot_._5sc.second;
			return &robot_._5sc;
		}
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
			<< ZJH::CGameLogic::StringHandTy(ty) << "]";
		LOG(WARNING) << "MaxCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]));
		LOG(WARNING) << "MinCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]));
		assert(false);
		break;
	}
	case ZJH::Ty123: {//顺子
		if (ZJH::A == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			ZJH::Q == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._QKA ratioWin = " << robot_._QKA.first << " weight = " << robot_._QKA.second;
			return &robot_._QKA;
		}
		else if (ZJH::K == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
				ZJH::J == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._JQK ratioWin = " << robot_._JQK.first << " weight = " << robot_._JQK.second;
			return &robot_._JQK;
		}
		else if (ZJH::Q == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
				ZJH::T == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._TJQ ratioWin = " << robot_._TJQ.first << " weight = " << robot_._TJQ.second;
			return &robot_._TJQ;
		}
		else if (ZJH::J == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
				9 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._9TJ ratioWin = " << robot_._9TJ.first << " weight = " << robot_._9TJ.second;
			return &robot_._9TJ;
		}
		else if (ZJH::T == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
				8 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._89T ratioWin = " << robot_._89T.first << " weight = " << robot_._89T.second;
			return &robot_._89T;
		}
		else if (9 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
				7 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._789 ratioWin = " << robot_._789.first << " weight = " << robot_._789.second;
			return &robot_._789;
		}
		else if (8 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			6 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._678 ratioWin = " << robot_._678.first << " weight = " << robot_._678.second;
			return &robot_._678;
		}
		else if (7 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			5 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._567 ratioWin = " << robot_._567.first << " weight = " << robot_._567.second;
			return &robot_._567;
		}
		else if (6 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			4 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._456 ratioWin = " << robot_._456.first << " weight = " << robot_._456.second;
			return &robot_._456;
		}
		else if (5 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			3 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._345 ratioWin = " << robot_._345.first << " weight = " << robot_._345.second;
			return &robot_._345;
		}
		else if (4 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			2 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._234 ratioWin = " << robot_._234.first << " weight = " << robot_._234.second;
			return &robot_._234;
		}
		else if (ZJH::A == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0])) &&
			2 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._A23 ratioWin = " << robot_._A23.first << " weight = " << robot_._A23.second;
			return &robot_._A23;
		}
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
			<< ZJH::CGameLogic::StringHandTy(ty) << "]";
		LOG(WARNING) << "MaxCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]));
		LOG(WARNING) << "MinCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]));
		assert(false);
		break;
	}
	case ZJH::Ty20: {//对子
		if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], ZJH::A, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._AA ratioWin = " << robot_._AA.first << " weight = " << robot_._AA.second;
			return &robot_._AA;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], ZJH::K, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._KK ratioWin = " << robot_._KK.first << " weight = " << robot_._KK.second;
			return &robot_._KK;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], ZJH::Q, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._QQ ratioWin = " << robot_._QQ.first << " weight = " << robot_._QQ.second;
			return &robot_._QQ;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], ZJH::J, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._JJ ratioWin = " << robot_._JJ.first << " weight = " << robot_._JJ.second;
			return &robot_._JJ;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], ZJH::T, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 [" << chairId << "] "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._TT ratioWin = " << robot_._TT.first << " weight = " << robot_._TT.second;
			return &robot_._TT;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 9, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._99 ratioWin = " << robot_._99.first << " weight = " << robot_._99.second;
			return &robot_._99;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 8, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._88 ratioWin = " << robot_._88.first << " weight = " << robot_._88.second;
			return &robot_._88;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 7, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._77 ratioWin = " << robot_._77.first << " weight = " << robot_._77.second;
			return &robot_._77;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 6, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._66 ratioWin = " << robot_._66.first << " weight = " << robot_._66.second;
			return &robot_._66;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 5, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._55 ratioWin = " << robot_._55.first << " weight = " << robot_._55.second;
			return &robot_._55;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 4, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._44 ratioWin = " << robot_._44.first << " weight = " << robot_._44.second;
			return &robot_._44;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 3, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._33 ratioWin = " << robot_._33.first << " weight = " << robot_._33.second;
			return &robot_._33;
		}
		else if (ZJH::CGameLogic::HasCardValue(&(handCards_[chairId])[0], 2, 2)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._22 ratioWin = " << robot_._22.first << " weight = " << robot_._22.second;
			return &robot_._22;
		}
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
			<< ZJH::CGameLogic::StringHandTy(ty) << "]";
		LOG(WARNING) << "MaxCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]));
		LOG(WARNING) << "MinCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]));
		assert(false);
		break;
	}
	case ZJH::Tysp: {//散牌
		if (ZJH::A == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._A ratioWin = " << robot_._A.first << " weight = " << robot_._A.second;
			return &robot_._A;
		}
		else if (ZJH::K == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._K ratioWin = " << robot_._K.first << " weight = " << robot_._K.second;
			return &robot_._K;
		}
		else if (ZJH::Q == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._Q ratioWin = " << robot_._Q.first << " weight = " << robot_._Q.second;
			return &robot_._Q;
		}
		else if (ZJH::J == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._J ratioWin = " << robot_._J.first << " weight = " << robot_._J.second;
			return &robot_._J;
		}
		else if (ZJH::T == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._10 ratioWin = " << robot_._10.first << " weight = " << robot_._10.second;
			return &robot_._10;
		}
		else if (9 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._9 ratioWin = " << robot_._9.first << " weight = " << robot_._9.second;
			return &robot_._9;
		}
		else if (8 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._8 ratioWin = " << robot_._8.first << " weight = " << robot_._8.second;
			return &robot_._8;
		}
		else if (7 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._7 ratioWin = " << robot_._7.first << " weight = " << robot_._7.second;
			return &robot_._7;
		}
		else if (6 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._6 ratioWin = " << robot_._6.first << " weight = " << robot_._6.second;
			return &robot_._6;
		}
		else if (5 == ZJH::CGameLogic::GetCardValue(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]))) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
				<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
				<< ZJH::CGameLogic::StringHandTy(ty) << "] ---------------- robot_._5 ratioWin = " << robot_._5.first << " weight = " << robot_._5.second;
			return &robot_._5;
		}
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " "
			<< ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " 机器人["
			<< ZJH::CGameLogic::StringHandTy(ty) << "]";
		LOG(WARNING) << "MaxCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MaxCard(&(handCards_[chairId])[0]));
		LOG(WARNING) << "MinCard: " << ZJH::CGameLogic::StringCard(ZJH::CGameLogic::MinCard(&(handCards_[chairId])[0]));
		assert(false);
		break;
	}
	default: assert(false); break;
	}
	return NULL;
}

//序列化机器人配置
void CGameProcess::SerialConfig(std::vector<std::pair<double, double>>& robot) {
	robot.clear();
	//豹子
	robot.push_back(robot_._30);
	//同花顺
	robot.push_back(robot_._123sc);
	//同花
	robot.push_back(robot_._Asc);
	robot.push_back(robot_._Ksc);
	robot.push_back(robot_._Qsc);
	robot.push_back(robot_._Jsc);
	robot.push_back(robot_._Tsc);
	robot.push_back(robot_._9sc);
	robot.push_back(robot_._8sc);
	robot.push_back(robot_._7sc);
	robot.push_back(robot_._6sc);
	robot.push_back(robot_._5sc);
	//顺子
	robot.push_back(robot_._QKA);
	robot.push_back(robot_._JQK);
	robot.push_back(robot_._TJQ);
	robot.push_back(robot_._9TJ);
	robot.push_back(robot_._89T);
	robot.push_back(robot_._789);
	robot.push_back(robot_._678);
	robot.push_back(robot_._567);
	robot.push_back(robot_._456);
	robot.push_back(robot_._345);
	robot.push_back(robot_._234);
	robot.push_back(robot_._A23);
	//对子
	robot.push_back(robot_._AA);
	robot.push_back(robot_._KK);
	robot.push_back(robot_._QQ);
	robot.push_back(robot_._JJ);
	robot.push_back(robot_._TT);
	robot.push_back(robot_._99);
	robot.push_back(robot_._88);
	robot.push_back(robot_._77);
	robot.push_back(robot_._66);
	robot.push_back(robot_._55);
	robot.push_back(robot_._44);
	robot.push_back(robot_._33);
	robot.push_back(robot_._22);
	//散牌
	robot.push_back(robot_._A);
	robot.push_back(robot_._K);
	robot.push_back(robot_._Q);
	robot.push_back(robot_._J);
	robot.push_back(robot_._10);
	robot.push_back(robot_._9);
	robot.push_back(robot_._8);
	robot.push_back(robot_._7);
	robot.push_back(robot_._6);
	robot.push_back(robot_._5);
}