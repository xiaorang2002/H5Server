#ifndef ANIMATEPLAY_INCLUDE_H
#define ANIMATEPLAY_INCLUDE_H

#include <chrono>

//////////////////////////////////////////////////////////////////////////
//动画类型
//////////////////////////////////////////////////////////////////////////
enum AnimateE {
	kAniNULL            = 0,
	kStartGameQ         = 1,  //开始游戏动画
	kStartCallBankerQ   = 2,  //开始抢庄动画
	kCallBankerQ        = 3,  //抢庄倒计时动画

	kMakeBankerQ        = 4,  //定庄动画
	kAddScoreQ          = 5,  //下注倒计时动画

	kShakeQ             = 6,  //摇骰子动画
	kSendCardQ          = 7,  //发牌动画
	kOpenCardCountDownQ = 8,  //开牌倒计时动画

	kOpenCardQ          = 9,  //开牌动画
	kNextCountDownQ     = 10, //下一轮开始倒计时动画
	kWinLostQ           = 11, //输赢动画
};

//////////////////////////////////////////////////////////////////////////
//动画播放时间 - 需要客户端配置
//////////////////////////////////////////////////////////////////////////
#define StartGame						1.5 //开始游戏
#define StartCallBanker					1 //开始抢庄
#define CallBankerCountDown				5 //抢庄倒计时
#define MakeBanker						1 //定庄动画
#define AddScoreCountDown				5 //下注倒计时
#define Shake							3 //摇骰子
#define SendCard						2 //发牌
#define OpenCardCountDown				1 //开牌倒计时
#define OpenCard						3 //开牌动画
#define NextCountDown                   3 //下一轮开始倒计时
#define WinLost							3 //输赢动画

/*
叫庄倒计时定时器延迟：
		 开始游戏 + 开始抢庄 + 抢庄倒计时

下注倒计时定时器延迟：
		 定庄动画 + 下注倒计时

开牌倒计时定时器延迟：
		 摇骰子 + 发牌 + 开牌倒计时

下一轮开始倒计时定时器延迟：
		 开牌动画 + 下一轮开始倒计时

游戏结束倒计时定时器延迟(1轮或2轮)：
		 开牌动画 + 输赢动画
*/
//////////////////////////////////////////////////////////////////////////
//定时器延迟时间
//////////////////////////////////////////////////////////////////////////
//叫庄倒计时      开始游戏 + 开始抢庄 + 抢庄倒计时
#define DELAY_Callbanker				(StartGame+StartCallBanker+CallBankerCountDown)
//下注倒计时      定庄动画 + 下注倒计时
#define DELAY_AddScore					(MakeBanker+AddScoreCountDown)
//开牌倒计时      摇骰子 + 发牌 + 开牌倒计时
#define DELAY_OpenCard					(Shake+SendCard+OpenCardCountDown)
//下一轮开始倒计时   开牌动画 + 下一轮开始倒计时
#define DELAY_NextRound					(OpenCard+NextCountDown)
//游戏结束倒计时   开牌动画 + 输赢动画
#define DELAY_GameEnd					(OpenCard+WinLost)

//////////////////////////////////////////////////////////////////////////
//动画播放时间线控制 AnimatePlay
// 1.当前动画播放类型 AnimateE
// 2.当前播放剩余时间 leftTime
//////////////////////////////////////////////////////////////////////////
struct AnimatePlay {

	enum StateE {
		CallE,	  //抢庄
		ScoreE,	  //下注
		OpenE,	  //开牌
		NextE,	  //下一轮
		GameEndE, //结束
		MaxE,
	};
	//添加开始时间
	void add_START_time(StateE state, chrono::system_clock::time_point startTime) {
		startTimes[int(state)] = startTime;
	}
	//计算动画剩余播放时长
	//chrono::system_clock::now()
	void calcAnimateDelay(int status, chrono::system_clock::time_point now) {
		aniTy = kAniNULL;
		leftTime = 0;
		//开始时间
		time_t startTime = Get_START_time(status);
		//当前时间
		time_t curTime = chrono::system_clock::to_time_t(now);
		//截止时间 = 开始时间 + 延迟时间
		//剩余时间 = 截止时间 - 当前时间
		switch (status) {
		case GAME_STATUS_CALL:
			//if (elapsedTime < StartGame) {
			//	//开始游戏动画
			//	aniTy = kStartGameQ;
			//	leftTime = StartGame - elapsedTime;
			//}
			//else if (elapsedTime < (StartGame + StartCallBanker)) {
				//开始抢庄动画
			//	aniTy = kStartCallBankerQ;
			//	leftTime = (StartGame + StartCallBanker) - elapsedTime;
			//}
			//else if (elapsedTime < DELAY_Callbanker) {
				//抢庄倒计时动画
				//aniTy = kCallBankerQ;
				leftTime = (double)startTime + DELAY_Callbanker - (double)curTime;
				//leftTime = DELAY_Callbanker - elapsedTime;
			//}
			break;
		case GAME_STATUS_SCORE:
			//if (elapsedTime < MakeBanker) {
			//	//定庄动画
			//	aniTy = kMakeBankerQ;
			//	leftTime = MakeBanker - elapsedTime;
			//}
			//else if (elapsedTime < DELAY_AddScore) {
			//	//下注倒计时动画
			//	aniTy = kAddScoreQ;
				leftTime = (double)startTime + DELAY_AddScore - (double)curTime;
				//leftTime = DELAY_AddScore - elapsedTime;
			//}
			break;
		case GAME_STATUS_OPEN:
			//if (elapsedTime < Shake) {
			//	//摇骰子动画
			//	aniTy = kShakeQ;
			//	leftTime = Shake - elapsedTime;
			//}
			//else if (elapsedTime < (Shake + SendCard)) {
			//	//发牌动画
			//	aniTy = kSendCardQ;
			//	leftTime = (Shake + SendCard) - elapsedTime;
			//}
			//else if (elapsedTime < DELAY_OpenCard) {
			//	//开牌倒计时动画
			//	aniTy = kOpenCardCountDownQ;
				leftTime = (double)startTime + DELAY_AddScore - (double)curTime;
				//leftTime = DELAY_OpenCard - elapsedTime;
			//}
			break;
		case GAME_STATUS_NEXT:
			//if (elapsedTime < OpenCard) {
			//	//开牌动画
			//	aniTy = kOpenCardQ;
			//	leftTime = OpenCard - elapsedTime;
			//}
			//else if (elapsedTime < DELAY_NextRound) {
			//	//下一轮开始倒计时
			//	aniTy = kNextCountDownQ;
				leftTime = (double)startTime + DELAY_NextRound - (double)curTime;
				//leftTime = DELAY_NextRound - elapsedTime;
			//}
			break;
		case GAME_STATUS_PREEND:
		case GAME_STATUS_END:
			//if (elapsedTime < OpenCard) {
			//	//开牌动画
			//	aniTy = kOpenCardQ;
			//	leftTime = OpenCard - elapsedTime;
			//}
			//else if (elapsedTime < DELAY_GameEnd) {
			//	//输赢动画
			//	aniTy = kWinLostQ;
				leftTime = (double)startTime + DELAY_GameEnd - (double)curTime;
				//leftTime = DELAY_GameEnd - elapsedTime;
			//}
			break;
		}
	}
private:
	time_t Get_START_time(int status) {
		switch (status) {
		case GAME_STATUS_CALL: return chrono::system_clock::to_time_t(startTimes[CallE]);
		case GAME_STATUS_SCORE: return chrono::system_clock::to_time_t(startTimes[ScoreE]);
		case GAME_STATUS_OPEN: return chrono::system_clock::to_time_t(startTimes[OpenE]);
		case GAME_STATUS_NEXT: return chrono::system_clock::to_time_t(startTimes[NextE]);
		case GAME_STATUS_PREEND:return chrono::system_clock::to_time_t(startTimes[GameEndE]);
		case GAME_STATUS_END: return chrono::system_clock::to_time_t(startTimes[GameEndE]);
		}
		return 0;
	}
private:
	chrono::system_clock::time_point startTimes[MaxE];
public:
	//动画类型
	AnimateE aniTy;
	//剩余时间
	double leftTime;
};

#endif