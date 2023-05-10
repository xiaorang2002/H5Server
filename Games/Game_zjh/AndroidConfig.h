#pragma once

#include <utility>

struct param_t {
	//看牌参数
	float K_[2];
	//牌比玩家大，权值倍率，修正参数
	float W1_[2], P1_;
	//牌比玩家小，权值倍率，修正参数
	float W2_[2], P2_;
};

//机器人牌型对应胜率/权值
struct robotconfig_t {
	//放大倍数
	int scale;
	//收分[0]/正常[1]/放分[2]，看牌参数，权值倍率，修正参数
	param_t param[3];
	//看牌概率
	int ratioKP[3];
	//收分时三套决策(0,1,2)几率
	int SP0[3];
	//正常时三套决策(0,1,2)几率
	int SP1[3];
	//放分时三套决策(0,1,2)几率
	int SP2[3];

	//每回合的弃牌概率
	int ratioQiPai[9][20];
	//每回合的加注概率
	int ratioJiaZhu[9][20];
	//加注筹码概率
	int ratioJiaZhuChip[9][5];
	//每回合的比牌概率
	int ratioBiPai[9][20];
	//制造各牌型的权重【1:散牌(小对子)|2:大对子|3:顺子|4:金花|5:顺金|6:豹子】
    int ratioMadeCard_[7];
    //个人库存控制的基本底分倍数
	int kcBaseTimes_[6];
	//个人库存控制的概率
	int ratiokcBaseTimes_[6];
	//豹子
	std::pair<double, double> _30;
	//同花顺
	std::pair<double, double> _123sc;
	//同花
	std::pair<double, double> _Asc;
	std::pair<double, double> _Ksc;
	std::pair<double, double> _Qsc;
	std::pair<double, double> _Jsc;
	std::pair<double, double> _Tsc;
	std::pair<double, double> _9sc;
	std::pair<double, double> _8sc;
	std::pair<double, double> _7sc;
	std::pair<double, double> _6sc;
	std::pair<double, double> _5sc;
	//顺子
	std::pair<double, double> _QKA;
	std::pair<double, double> _JQK;
	std::pair<double, double> _TJQ;
	std::pair<double, double> _9TJ;
	std::pair<double, double> _89T;
	std::pair<double, double> _789;
	std::pair<double, double> _678;
	std::pair<double, double> _567;
	std::pair<double, double> _456;
	std::pair<double, double> _345;
	std::pair<double, double> _234;
	std::pair<double, double> _A23;
	//对子
	std::pair<double, double> _AA;
	std::pair<double, double> _KK;
	std::pair<double, double> _QQ;
	std::pair<double, double> _JJ;
	std::pair<double, double> _TT;
	std::pair<double, double> _99;
	std::pair<double, double> _88;
	std::pair<double, double> _77;
	std::pair<double, double> _66;
	std::pair<double, double> _55;
	std::pair<double, double> _44;
	std::pair<double, double> _33;
	std::pair<double, double> _22;
	//散牌
	std::pair<double, double> _A;
	std::pair<double, double> _K;
	std::pair<double, double> _Q;
	std::pair<double, double> _J;
	std::pair<double, double> _10;
	std::pair<double, double> _9;
	std::pair<double, double> _8;
	std::pair<double, double> _7;
	std::pair<double, double> _6;
	std::pair<double, double> _5;
};