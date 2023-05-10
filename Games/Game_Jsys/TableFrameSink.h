#pragma once


#define MAX_PLAYER          300
#define BET_ITEM_COUNT       8 + 4 // 飞禽、走兽、金鲨鱼、银鲨鱼
#define BET_IDX_BEAST       11 //飞禽 
#define BET_IDX_BIRD        10 //走兽

#define RES_RECORD_COUNT			20			//记录数量
#define BEST_PLAYER_LIST_COUNT		20			//最好成绩玩家记录数量
#define OPEN_RESULT_LIST_COUNT		100			//开奖结果记录数量

/*
* 位置是15-16-17是兔子， 0
* 位置是19-20-21是燕子，1
* 位置是12-13是猴子，2
* 位置是23-24是鸽子，3
* 位置是9-10是熊猫，4
* 位置是26-27是孔雀，5
* 位置5-6-7是狮子，6
* 位置1-2-3是老鹰，7
* 位置0-8-14-22是银鲨，8
* 位置4-11-18-25是金鲨，9
*/ 
    
struct tagBetInfo
{
    int32_t betScore;
    int32_t betIdx;
};


using namespace std;

class UserInfo
{
public:
    UserInfo()
    {
        JettonValues.clear();
        ListWinOrLose.clear();
        ListAllJetton.clear();
		ListAllWinScore.clear();
        dCurrentPlayerJetton = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            dLastJetton[i] = 0;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
        }

        wCharid = -1;
        wUserid = -1;
		headerId = 0;
        Isplaying = false;
        Isleave = false;
        m_winAfterTax = 0;
        YingshuJuduizhi = 0;
        platformTax = 0;

        dAllOut = 0;
        TotalIn = 0;
        m_userTax = 0;
        ReturnMoney = 0;
        RealMoney =0;
        taxRate = 0.05;
        NotBetJettonCount = 0;
		lTwentyAllUserScore = 0;	//20局总下注
		lTwentyWinScore = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount = 0;        //20局嬴次数. 在线列表显示用
		iResultId = -1;
		iResultId_sd = -1;
		iResultMul_sd = 0;
		iResultMul_cj = 0;
		lTwentyAllUserScore_last = 0;	 //20局总下注
		lTwentyWinScore_last = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount_last = 0;        //20局嬴次数. 在线列表显示用
    }
    // 清空玩家全部数据
    void Allclear()
    {
        JettonValues.clear();
        ListWinOrLose.clear();
        ListAllJetton.clear();
		ListAllWinScore.clear();
        dCurrentPlayerJetton = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            iMulticle[i] = -1;
            dLastJetton[i] = 0;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
        }

        wCharid = -1;
        wUserid = -1;
        Isplaying = false;
        Isleave = false;
        m_winAfterTax = 0;
        YingshuJuduizhi = 0;
        platformTax = 0;
        m_userTax = 0;
        ReturnMoney = 0;
        RealMoney = 0;
        NotBetJettonCount = 0;
		lTwentyAllUserScore = 0;	//20局总下注
		lTwentyWinScore = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount = 0;        //20局嬴次数. 在线列表显示用
		iResultId = -1;
		iResultId_sd = -1;
		iResultMul_sd = 0;
		iResultMul_cj = 0;
    }
    void clearJetton()
    {
        JettonValues.clear();
    }
    void clear()
    {
        dCurrentPlayerJetton = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            iMulticle[i] = -1;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
        }
        YingshuJuduizhi = 0;
        m_winAfterTax = 0;
        dAllOut = 0;
        if (Isleave)
        {
            Isplaying = false;
            Isleave = false;
        }
        platformTax = 0;
        m_userTax = 0;
        ReturnMoney = 0; 
        RealMoney = 0;
		//lTwentyAllUserScore = 0;	//20局总下注
		//lTwentyWinScore = 0;        //20局嬴分值. 在线列表显示用
		//lTwentyWinCount = 0;        //20局嬴次数. 在线列表显示用
		iResultId = -1;
		iResultId_sd = -1;
		iResultMul_sd = 0;
		iResultMul_cj = 0;
    }
    void clearOtherJetton()
    {
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dsliceSecondsAllAreaJetton[i] = 0;
        }
    }
    // 是否飞禽
    bool IsFeiQin( int Idx) 
    {
        if(Idx <  (int)AnimType::eSilverSharp){
            return (Idx % 2 == 0);
        }
        return false;
    }
    // 计算得分
    void Culculate( float valshui,int32_t iRetId) //shared_ptr<ITableFrame> &pITableFrame,
    {
        int64_t res = 0;
        taxRate = valshui;
        YingshuJuduizhi = 0;
        m_winAfterTax = 0;
        platformTax = 0;
        
        dAllOut = 0;
        TotalIn = 0;
        m_userTax = 0; 
        int64_t resTax = 0;
		RealMoney = 0;

        //飞禽走兽
        if (iRetId <= (int)AnimType::eGoldSharp)//
        {
            int64_t tempWin = 0;

            if(iRetId >= (int)AnimType::eSilverSharp){
                // 净赢分(减去押分本金)
                resTax = -(dAreaJetton[BET_IDX_BIRD] + dAreaJetton[BET_IDX_BEAST]);
                // 飞禽走兽玩家得分
                tempWin = resTax;

                dAreaWin[BET_IDX_BIRD] = -dAreaJetton[BET_IDX_BIRD];
                dAreaWin[BET_IDX_BEAST] = -dAreaJetton[BET_IDX_BEAST];
            }
            else
            {
                // 飞禽走兽玩家得分
                tempWin =  dAreaJetton[BET_IDX_BEAST] * iMulticle[BET_IDX_BEAST]; 
                tempWin +=  dAreaJetton[BET_IDX_BIRD] * iMulticle[BET_IDX_BIRD]; 
                // 净赢分(减去押分本金)
                if(iMulticle[BET_IDX_BIRD] >= 0 ){
					resTax = dAreaJetton[BET_IDX_BIRD] * (iMulticle[BET_IDX_BIRD] - 1);
					dAreaWin[BET_IDX_BIRD] = resTax;
                    resTax -= dAreaJetton[BET_IDX_BEAST];
                    dAreaWin[BET_IDX_BEAST] = -dAreaJetton[BET_IDX_BEAST];
                }
                else{
					resTax = dAreaJetton[BET_IDX_BEAST] * (iMulticle[BET_IDX_BEAST] - 1);
					dAreaWin[BET_IDX_BEAST] = resTax;
                    resTax -= dAreaJetton[BET_IDX_BIRD];
                    dAreaWin[BET_IDX_BIRD] = -dAreaJetton[BET_IDX_BIRD];
                }

            }

            //赢输绝对值
            YingshuJuduizhi += tempWin;
            // 总赢分
            TotalIn         += tempWin;
            // 玩家本局总得分
            ReturnMoney     += tempWin;

            LOG(WARNING) << "飞禽走兽得分 " << tempWin << " 得分增量 " << resTax << endl;
        }

        //动物计分
        for (int i = 0; i <= (int)AnimType::eGoldSharp; i++)
        {
            // 本押注区开奖
            int64_t tempWin = 0;
            if (iMulticle[i] > 0)
            {				
				if (i == (int)AnimType::eSilverSharp || i == (int)AnimType::eGoldSharp)
				{
					tempWin = dAreaJetton[8] * (iMulticle[i]);
					// 净赢分(减去押分本金)
					res = dAreaJetton[8] * (iMulticle[i] - 1);
					//送灯的倍数分,以押鲨鱼的投注来赔
					if (dAreaJetton[8] > 0)
					{
						tempWin += dAreaJetton[8] * iResultMul_sd;
						res += dAreaJetton[8] * iResultMul_sd;
					}
					//开金鲨送的彩金fen
					if (i == (int)AnimType::eGoldSharp)
					{
						tempWin += dAreaJetton[8] * iResultMul_cj;
						res += dAreaJetton[8] * iResultMul_cj;
					}
					//赢输绝对值
					YingshuJuduizhi += tempWin;
					// 总赢分
					TotalIn += tempWin;
					// 玩家本局总得分
					ReturnMoney += tempWin;
					RealMoney += res;

					dAreaWin[8] = res;
					// 税后赢分
					m_winAfterTax += res;
					// 玩家本局总押分
					dAllOut += dAreaJetton[8];
					// 平台税收
					platformTax += (int32_t)(taxRate * dAreaJetton[8]);
					m_userTax += 0;
				} 
				else
				{
					tempWin = dAreaJetton[i] * (iMulticle[i]);
					// 净赢分(减去押分本金)
					res = dAreaJetton[i] * (iMulticle[i] - 1);

					//赢输绝对值
					YingshuJuduizhi += tempWin;
					// 总赢分
					TotalIn += tempWin;
					// 玩家本局总得分
					ReturnMoney += tempWin;
					RealMoney += res;

					dAreaWin[i] = res;
					// 税后赢分
					m_winAfterTax += res;
					// 玩家本局总押分
					dAllOut += dAreaJetton[i];
					// 平台税收
					platformTax += (int32_t)(taxRate * dAreaJetton[i]);
					m_userTax += 0;
				}
                
                
                LOG(WARNING) << "动物得分 " << tempWin << " 总得分 "<< ReturnMoney << " res " << res + resTax << endl;
            }
            else if(i < (int)AnimType::eSilverSharp)
            {
                YingshuJuduizhi += dAreaJetton[i];
                res = dAreaJetton[i] * iMulticle[i];
                ReturnMoney += res;
                RealMoney += res;

				dAreaWin[i] = res;
				// 税后赢分
				m_winAfterTax += res;
				// 玩家本局总押分
				dAllOut += dAreaJetton[i];
				// 平台税收
				platformTax += (int32_t)(taxRate * dAreaJetton[i]);
				m_userTax += 0;
            }            
			else if (i == (int)AnimType::eSilverSharp && iResultId != (int)AnimType::eGoldSharp)
			{
				YingshuJuduizhi += dAreaJetton[i];
				res = dAreaJetton[i] * iMulticle[i];
				ReturnMoney += res;
				RealMoney += res;

				dAreaWin[i] = res;
				// 税后赢分
				m_winAfterTax += res;
				// 玩家本局总押分
				dAllOut += dAreaJetton[i];
				// 平台税收
				platformTax += (int32_t)(taxRate * dAreaJetton[i]);
				m_userTax += 0;
			}
        }
        RealMoney += resTax;
        m_winAfterTax += resTax;

        // 赢输记录
        if ((ListWinOrLose.size()) >= 20)
            ListWinOrLose.erase(ListWinOrLose.begin());
        ListWinOrLose.push_back((m_winAfterTax > 0) ? 1 : 0);

        // 当前玩家押分记录
        if ((ListAllJetton.size()) >= 20)
            ListAllJetton.erase(ListAllJetton.begin());
        ListAllJetton.push_back(dCurrentPlayerJetton);

		// 当前玩家输赢分数记录
		if ((ListAllWinScore.size()) >= 20)
			ListAllWinScore.erase(ListAllWinScore.begin());
		ListAllWinScore.push_back(RealMoney);
    }
    
    /* 
    兔子 0 燕子 1  猴子 2 鸽子 3  熊猫  4 孔雀  5  狮子 6 老鹰 7  银鲨 8 金鲨 9  走兽 10  飞禽 11
    */
    // @index 开奖下标 m_iResultId 
    // @mutical 开奖对应动物倍数，index_sd:鲨鱼的送灯，mutical_cj:金鲨送彩金的倍数
    void SetPlayerMuticl(int index, int mutical, int index_sd = -1, int mul_sd = 0, int mutical_cj = 0)
    {
        if (index < (int)AnimType::eRabbit || index > (int)AnimType::eGoldSharp || mutical < 0) {
            LOG(ERROR) << "No Resulet form Algorithmc"<<"m_iResultId:"<<index;
            return;
        }

        for (int i = 0; i <= (int)AnimType::eGoldSharp; i++) {
            iMulticle[i] = (index == i) ? mutical : (-1);
        }

        iMulticle[BET_IDX_BEAST] = IsFeiQin(index) ? -1 : 2;
        iMulticle[BET_IDX_BIRD] = IsFeiQin(index) ? 2 : -1;

		iResultId = index;
		iResultId_sd = index_sd;
		iResultMul_sd = mul_sd;
		iResultMul_cj = mutical_cj;
    }
    int64_t GetTwentyWin()
    {
        if (ListWinOrLose.size() <= 0) 
            return 0;
        
		lTwentyWinCount = 0;
        for (vector<int64_t>::iterator it = ListWinOrLose.begin(); it != ListWinOrLose.end(); it++)
        {
            if ((*it) > 0)
				lTwentyWinCount += 1;
        }
        return lTwentyWinCount_last;
    }
    int64_t GetTwentyJetton()
    {
        if (ListAllJetton.size() <= 0)
            return 0; 

		lTwentyAllUserScore = 0;
        for (vector<int64_t>::iterator it = ListAllJetton.begin(); it != ListAllJetton.end(); it++)
        {
			lTwentyAllUserScore += (*it);
        }
        return lTwentyAllUserScore_last;
    }
    int64_t GetTwentyWinScore()
	{
		if (ListAllWinScore.size() <= 0)
			return 0;

		lTwentyWinScore = 0;
        for (vector<int64_t>::iterator it = ListAllWinScore.begin(); it != ListAllWinScore.end(); it++)
		{
			lTwentyWinScore += (*it);
		}
		return lTwentyWinScore_last;
	}
	void RefreshGameRusult(bool isGameEnd)
	{
		if (isGameEnd)
		{
			lTwentyAllUserScore_last = lTwentyAllUserScore;
			lTwentyWinScore_last = lTwentyWinScore;
			lTwentyWinCount_last = lTwentyWinCount;
		}
	}
    vector<int64_t> ListWinOrLose;
    vector<int64_t> ListAllJetton;
    vector<int64_t> ListAllWinScore; //20局的输赢
    vector<tagBetInfo> JettonValues; //所有押分筹码值

    int64_t dCurrentPlayerJetton;
    // 
    int64_t dAreaJetton[BET_ITEM_COUNT];
    int64_t dAreaWin[BET_ITEM_COUNT];
    int64_t dLastJetton[BET_ITEM_COUNT];

    int64_t TotalIn;
    int32_t iMulticle[BET_ITEM_COUNT];
    int64_t dAreaOut[BET_ITEM_COUNT];
    int64_t dAllOut;
    int64_t YingshuJuduizhi;
    int64_t m_winAfterTax; //税后赢分
    int64_t platformTax;
    int64_t m_userTax;
    int64_t ReturnMoney;
    int64_t RealMoney;
    int32_t NotBetJettonCount; // 连续不下注的局数
    int wCharid;
    int64_t wUserid;
	uint8_t     headerId;	//玩家头像.
	string      nickName;	//玩家昵称.
	string      location;
    // 是否游戏中
    bool Isplaying;
    bool Isleave;
    float taxRate;
    //一段时间内各区域其他玩家的总下注
    int64_t dsliceSecondsAllJetton;
    int64_t dsliceSecondsAllAreaJetton[BET_ITEM_COUNT];

//private:
	int64_t  lTwentyAllUserScore;		//20局总下注
	int64_t  lTwentyWinScore;			//20局嬴分值. 在线列表显示用
	uint8_t  lTwentyWinCount;			//20局嬴次数. 在线列表显示用
	int		 iResultId;					//本局开奖结果
	int		 iResultId_sd;				//若开金鲨或银鲨赠送的动物类型
	int		 iResultMul_sd;				//赠送的动物的倍数
	int		 iResultMul_cj;				//开金鲨赠送的彩金倍数
    int64_t  lTwentyAllUserScore_last;	//20局总下注
    int64_t  lTwentyWinScore_last;		//20局嬴分值. 在线列表显示用
    int64_t  lTwentyWinCount_last;		//20局嬴次数. 在线列表显示用

};

class OpenResultInfo
{
public:
	OpenResultInfo()
	{
		iResultId = 0;
		notOpenCount = 0;
	}
	void clear()
	{
		iResultId = 0;
		notOpenCount = 0;
	}
	// 是否飞禽
	bool IsFeiQin(int Idx)
	{
		if (Idx < (int)AnimType::eSilverSharp) {
			return (Idx % 2 == 0);
		}
		return false;
	}

	int iResultId;
	int notOpenCount;
};

class TableFrameSink : public ITableFrameSink
{

public:
    // 能取上限
    static int RandomInt(int a, int b)
    {
        return rand() % (b - a + 1) + a;
    }

    float m_fTaxRate; 
    float m_fTMJettonBroadCard; 
	  int32_t m_testRet;
    int m_testInex;
    int m_testResultInex;
	int m_testInex_sd;
	int m_testResultInex_sd;

    struct BestPlayerList
    {
        BestPlayerList()
        {
            NikenName = "";
            WinScore = 0;
            onTime = "";
            bestID = 0;
        }
        string NikenName;
        int64_t WinScore;
        string onTime;
        int bestID;
    };
	// 20局输赢的次数排序
    static bool Comparingconditions(shared_ptr<UserInfo> a, shared_ptr<UserInfo> b)
    {
        if (a->GetTwentyWin() > b->GetTwentyWin())
        {
            return true;
        }
        else if (a->GetTwentyWin() == b->GetTwentyWin() && a->GetTwentyJetton() > b->GetTwentyJetton())
        {
            return true;
        }
        return false;
    }
	//投注量从大到小排序
    static bool Comparingconditions1(shared_ptr<UserInfo> a, shared_ptr<UserInfo> b)
    {
        if (a->GetTwentyJetton() > b->GetTwentyJetton())
        {
            return true;
        }
        else if (a->GetTwentyWin() > b->GetTwentyWin() && a->GetTwentyJetton() == b->GetTwentyJetton())
        {
            return true;
        }
        return false;
    }	
	//20局累计输赢从大到小排序
    static bool Comparingconditions2(UserInfo a, UserInfo b)
	{		
//		if (!a && b)
//		{
//			LOG(ERROR) << "********** Comparingconditions2 a == NULL ";
//			return false;
//		}
//		if (a && !b)
//		{
//			LOG(ERROR) << "********** Comparingconditions2 b == NULL " ;
//			return true;
//		}
//		if (!a && !b)
//		{
//			LOG(ERROR) << "********** Comparingconditions2 a == NULL  b == NULL ";
//			return false;
//		}
        if (a.GetTwentyWinScore() > b.GetTwentyWinScore())
		{
			return true;
		}
        else if (a.GetTwentyJetton() > b.GetTwentyJetton() && a.GetTwentyWinScore() == b.GetTwentyWinScore())
		{
			return true;
		}
        else if (a.GetTwentyWin() > b.GetTwentyWin() && a.GetTwentyJetton() == b.GetTwentyJetton())
		{
			return true;
		}
		return false;
	}
    TableFrameSink();
    virtual ~TableFrameSink();

public: //game.
    virtual void OnGameStart();
    virtual bool OnEventGameConclude(uint32_t chairid, uint8_t nEndTag);
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);
    virtual void OnEventGameEnd(int32_t chairid, uint8_t nEndTag);

public: //users.
    virtual bool OnUserEnter(int64_t userid, bool islookon);
    virtual bool OnUserReady(int64_t userid, bool islookon);
    virtual bool OnUserLeft(int64_t userid, bool islookon);
    virtual bool CanJoinTable(shared_ptr<CServerUserItem> &pUser);
    virtual bool CanLeftTable(int64_t userId);
  
public: //events.
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
public: // setting.
    virtual bool SetTableFrame(shared_ptr<ITableFrame> &pTableFrame);

public:
    void GamefirstInGame();
    void NormalCalculate();
    void NormalGameStart();
    bool GamePlayerJetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void GamePlayerJetton1(int32_t score, int index, int chairid,shared_ptr<CServerUserItem> &pSelf);
    void SendGameSceneStart(int64_t userid, bool lookon);
    void SendGameSceneEnd(int64_t userid, bool lookon);
    void PlayerRichSorting(bool iChooseShenSuanZi = true);
    void PlayerCalculate();
    void ResetTableDate();
    void SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void WriteUserScore();
    void writeRecored();
    void readRecored();
    void ArgorithResult();
    void Algorithm();
    void BulletinBoard();
    void GamePlayerQueryPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd = false);
    void ReadInI();
    void GameTimerStart();
    void GameTimerEnd();
	  int PlayerJettonPanDuan(int index, int32_t score, int32_t chairid, shared_ptr<CServerUserItem> &play);
    void QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void ReadStorageInfo();
    void OnTimerSendOtherBetJetton();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
    //测试游戏数据
    void TestGameMessage(uint32_t wChairID,int type);
    void openSLog(const char *p, ...);
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
	//更新100局开奖结果信息
	void updateResultList(int resultId);
	// 100局游戏记录
	void QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	void sendGameRecord(int32_t chairid);
	// 获取本局金鲨开奖的倍数
	void getJSodds();
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	//重排近20局玩家盈利
	void SortPlayerList();
private:
    int	 m_iGameState; 
    int	 m_iCurrBankerId;
    int	 m_iShenSuanZiChairId;
    int	 m_iShenSuanZiUserId;
    int	 m_iRetMul;
    int	 m_iResultId;
    int	 m_iStopIndex;
    int	 m_BestPlayerUserId;
    int	 m_BestPlayerChairId;
    int	 m_bIsIncludeAndroid;
    int	 m_nReadCount;
    int	 m_betCountDown;
    int	 m_endCountDown; 
    int	 m_timetick;
    int	 m_timetickAll;
    bool m_bIsOpen;
    bool m_bIsContinue;
    bool m_bIsClearUser;
	float m_addCountDown_sd;		//开金鲨银鲨送灯增加的时间
	float m_addCountDown_cj;		//开金鲨彩金动画增加的时间
	float m_fGameStartAniTime;		//预留前端开始动画的时间
	float m_fGameEndAniTime;		//预留前端结束动画的时间
	int	 m_iResultId_sd;			//若开金鲨或银鲨赠送的动物类型
	int	 m_iResultMul_js;			//开金鲨的倍数
	int	 m_iResultMul_cj;			//开金鲨赠送的彩金倍数
	int	 m_iStopIndex_sd;			//若开金鲨或银鲨赠送的动物停止的位置ID　

	float m_fResetPlayerTime;		//本局游戏结束刷新玩家排序的时间

    //干涉概率
    float m_ctrlRate;
    float m_stockPercentRate;
    int32_t					m_lastRetId;

    int64_t					m_BestPlayerWinScore;
    int32_t					m_EapseTime; 
    int64_t					m_MarqueeScore;
    int64_t					m_nAreaMaxJetton[BET_ITEM_COUNT];
    int32_t					m_retWeight[BET_ITEM_COUNT];
    int32_t					m_nMuilt[BET_ITEM_COUNT];// 倍率表
	

	int64_t                          m_lShenSuanZiJettonScore[BET_ITEM_COUNT];
	int64_t                          m_ShenSuanZiId;

    // new玩家列表
    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    // 玩家列表
    vector<shared_ptr<UserInfo>>     m_pPlayerInfoList;
	// 20局玩家列表排序
    vector<UserInfo>     m_pPlayerInfoList_20;
    // 最佳玩家列表
    vector<BestPlayerList>  m_vPlayerList;
    vector<int64_t>         m_vResultRecored;
    vector<int32_t>         m_cbJettonMultiple; //筹码值
	// 100局开奖记录
    map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount;
	map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount_last;

    shared_ptr<BetArea>     m_pBetArea;
    shared_ptr<ITableFrame> m_pITableFrame;
private:
    tagGameRoomInfo         *m_pGameRoomInfo;
    tagGameReplay           m_replay; 
    string                  m_strRoundId;
    uint32_t                m_dwReadyTime;
    chrono::system_clock::time_point  m_startTime;

    muduo::MutexLock        m_mutexLock;
    muduo::net::TimerId     m_TimerIdStart;
    muduo::net::TimerId     m_TimerIdEnd;
    muduo::net::TimerId     m_TimerIdTest; 
    muduo::net::TimerId     m_TimerOtherBetJetton;
	muduo::net::TimerId     m_TimerIdResetPlayerList;
    shared_ptr<muduo::net::EventLoopThread> m_LoopThread;
private: 
    static int64_t		m_llStorageControl;									//库存控制
    static int64_t		m_llStorageLowerLimit;								//库存控制下限值
    static int64_t		m_llStorageHighLimit;								//库存控制上限值
    static int64_t      m_llStorageAverLine;          //average line
    static int64_t      m_llGap;            //refer gap:for couting rate
    int32_t             m_lOpenResultCount;                                 //累计开奖次数
    int32_t             m_lOpenResultIndexCount[BET_ITEM_COUNT];            //各下注项累计开奖次数
    int32_t             m_bTest;            //是否测试
    int32_t             m_nTestTimes;        //测试的局数
    int32_t             m_nTestType;        //测试的类型
    bool                m_bTestAgain;       
    // 是否写日记
    bool                m_bWritelog;
    double              stockWeak;

    int                 m_lLimitPoints;
    double              m_dControlkill;

    JsysAlgorithm       jsysAlgorithm;


    //
    int64_t                 m_useIntelligentChips;
    int64_t                 m_userScoreLevel[4];
    int64_t                 m_userChips[4][6];
    int64_t                 m_currentchips[MAX_PLAYER][6];

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
	bool					m_bGameEnd;		//本局是否结束
	bool					m_bRefreshPlayerList;		//本局是否刷新玩家排序

	//开金鲨是的倍数和彩金倍数
	int32_t m_ctrlGradePercentRate[2];  // 比例[下标0:低倍;1:中倍]
	int32_t m_ctrlGradeMul[2];			// 倍数[下标0:金鲨倍数;1:彩金倍数]

};

bool Compare(shared_ptr<CServerUserItem> ait, shared_ptr<CServerUserItem> bit)
{
    if (ait->GetUserScore() > bit->GetUserScore())
    {
        return true;
    }
    else if (ait->GetUserScore() > bit->GetUserScore() && ait->GetUserId() < bit->GetUserId())
    {
        return true;
    }
    else
    {
        return false;
    }
}

// ITableFrameSink *GetTableFrameSinkFromDLL(void);
