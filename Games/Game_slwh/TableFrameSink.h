#pragma once

#define CHIP_COUNT					5			//筹码个数
#define MAX_PLAYER					500
#define BET_ITEM_COUNT				15			// 0:红兔子 1:红猴子 2:红熊猫 3:红狮子 4:绿兔子 5:绿猴子 6:绿熊猫 7:绿狮子 8:黄兔子 9:黄猴子 10:黄熊猫 11:黄狮子 12:和 13:庄 14:闲
#define CIRCLE_COUNT				24			// 动物圈、颜色去圈 24个区域

#define BET_IDX_HE					12			//和
#define BET_IDX_ZHUANG				13			//庄 
#define BET_IDX_XIAN				14			//闲

#define COLOUR_RED					0			//红色
#define COLOUR_GREEN				1			//绿色
#define COLOUR_YELLOW				2			//黄色

#define RES_RECORD_COUNT			20			//记录数量
#define BEST_PLAYER_LIST_COUNT		20			//最好成绩玩家记录数量
#define OPEN_RESULT_LIST_COUNT		100			//开奖结果记录数量

#define TEST_COUNT					2400			//测试打印记录数量
#define TEST_ALLCOUNT				7200		//一次测试的局数

/*
* 动物圈顺时针:
* 位置0-5-12-17是狮子，3
* 位置4-8-16-20是熊猫，2
* 位置2-6-10-13-15-18-22是猴子，1
* 位置1-3-7-9-11-14-19-21-23是兔子，0
*/ 

/*
* 颜色圈:
* 红色，0
* 绿色，1
* 黄色，2
*/
    
struct tagBetInfo
{
    int64_t betScore;
    int32_t betIdx;
};

//每局开奖结果
struct tagResultInfo
{
	tagResultInfo()
	{
		iHZXId = -1;
		iAnimalId = -1;
		iAnimalId_sd = -1;
		iOpenType = -1;
	}
	int		iHZXId;				//和庄闲
	int		iAnimalId;			//动物ID
	int		iAnimalId_sd;		//送灯的动物ID
	int		iOpenType;			//0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜
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
		iResultOpenType = 0;
		lTwentyAllUserScore_last = 0;	 //20局总下注
		lTwentyWinScore_last = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount_last = 0;        //20局嬴次数. 在线列表显示用
		iResultId_zx = -1;
		iResultMul_zx = -1;

		lLastWinOrLost = true;
		lLastJetton = 0;
		iDoubleCastTimes = 0;
		iLoseTimes = 0;
		isDoubleCast = false;
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
		iResultOpenType = 0;
		iResultId_zx = -1;
		iResultMul_zx = -1;
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
		iResultId = -1;
		iResultId_sd = -1;
		iResultMul_sd = 0;
		iResultOpenType = 0;
		iResultId_zx = -1;
		iResultMul_zx = -1;
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
    /*bool IsFeiQin( int Idx) 
    {
        if(Idx <  (int)AnimType::eSilverSharp){
            return (Idx % 2 == 0);
        }
        return false;
    }*/
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

        //动物计分
        for (int i = 0; i <= (int)AnimType::eAreaXian; i++)
        {
            // 本押注区开奖
            int64_t tempWin = 0;
            if (iMulticle[i] > 0)
            {				
				tempWin = dAreaJetton[i] * (iMulticle[i]);
				// 净赢分(减去押分本金)
				res = dAreaJetton[i] * (iMulticle[i] - 1);
				//如果是庄闲就要抽水
				if (i == iResultId_zx && iResultId_zx != (int)AnimType::eAreaHe)
				{
					m_userTax = (int32_t)(taxRate * res);
					res -= m_userTax;
				}
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
                
                if (i > (int)AnimType::eYellowLion && i == iResultId_zx)
				{
					string strHZX = "";
					if (i == (int)AnimType::eAreaHe)
						strHZX = "    开和: ";
					else if (i == (int)AnimType::eAreaZhuang)
						strHZX = "    开庄: ";
					else if (i == (int)AnimType::eAreaXian)
						strHZX = "    开闲: ";
					LOG(WARNING) << "开奖类型: " << iResultOpenType << " 开奖动物: " << iRetId << "" << strHZX << i << " 押注:" << dAreaJetton[i] << " 赔率:" << iMulticle[i] << " 得分:" << tempWin << " 累计总得分:" << ReturnMoney << " res:" << res + resTax << " wUserid:" << wUserid << endl;
                } 
                else
				{
					LOG(WARNING) << "开奖类型: " << iResultOpenType << " 开奖动物: " << iRetId << " 中奖动物: " << i << " 押注:" << dAreaJetton[i] << " 赔率:" << iMulticle[i] << " 得分:" << tempWin << " 累计总得分:" << ReturnMoney << " res:" << res + resTax << " wUserid:" << wUserid << endl;
                }
            }
            else if (i < (int)AnimType::eAreaHe)
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
			else
			{
				if (iResultId_zx != (int)AnimType::eAreaHe)
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
 *  0:红兔子 1:红猴子 2:红熊猫 3:红狮子 4:绿兔子 5:绿猴子 6:绿熊猫 7:绿狮子 8:黄兔子 9:黄猴子 10:黄熊猫 11:黄狮子 12:和 13:庄 14:闲
 *  @index 开奖下标 m_iResultId 
 *  @mutical 开奖对应动物倍数，index_sd:送灯，mul_sd:送灯倍数，openTypeTag:开奖类型[0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜]
 *  @index_zx:和庄闲开奖id @mutical_zx:和庄闲开奖的倍数
*/
    void SetPlayerMuticl(int index, int mutical, int *nAreaMuilt, int index_sd, int mul_sd, int openTypeTag, int index_zx = (int)AnimType::eAreaZhuang)
    {
        if (index < (int)AnimType::eRedRabbit || index > (int)AnimType::eYellowLion || mutical < 0) {
            LOG(ERROR) << "No Resulet form Algorithmc" << " m_iResultId:"<< index;
            return;
        }
		memcpy(nMuilt, nAreaMuilt, sizeof(nMuilt));
		memset(iMulticle, -1, sizeof(iMulticle));
		if (openTypeTag == (int)OpenType::eNormal)
		{
			for (int i = 0; i <= (int)AnimType::eYellowLion; i++) {
				iMulticle[i] = (index == i) ? mutical : (-1);
			}
		} 
		else
		{
			if (openTypeTag == (int)OpenType::ePiliShanDian)
			{
				for (int i = 0; i <= (int)AnimType::eYellowLion; i++) {
					iMulticle[i] = (index == i) ? mutical * 2 : (-1);
				}
			} 
			else if (openTypeTag == (int)OpenType::eSongDeng)
			{
				for (int i = 0; i <= (int)AnimType::eYellowLion; i++) 
				{					
					if (index == i) 
						iMulticle[i] = mutical;
					else if (index_sd == i) 
						iMulticle[i] = mul_sd;
					else
						iMulticle[i] = -1;
				}
			}
			else if (openTypeTag == (int)OpenType::eDaSanYuan)
			{
				int animId = index % 4;				
				for (int i = 0; i < 3; i++)
				{
					iMulticle[animId + i * 4] = nMuilt[animId + i * 4];
				}
			}
			else if (openTypeTag == (int)OpenType::eDaSiXi)
			{
				int animId = index / 4;
				for (int i = 0; i < 4; i++)
				{
					iMulticle[animId * 4 + i] = nMuilt[animId * 4 + i];
				}
			}
		}

		iResultId = index;
		iResultId_sd = index_sd;
		iResultMul_sd = mul_sd;
		iResultOpenType = openTypeTag;
		iResultId_zx = index_zx;
		iResultMul_zx = nMuilt[index_zx];
		iMulticle[iResultId_zx] = iResultMul_zx;
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
	int		 iResultId_sd;				//开送灯时赠送的动物类型
	int		 iResultMul_sd;				//赠送的动物的倍数
    int64_t  lTwentyAllUserScore_last;	//20局总下注
    int64_t  lTwentyWinScore_last;		//20局嬴分值. 在线列表显示用
	int32_t  lTwentyWinCount_last;		//20局嬴次数. 在线列表显示用
	int		 iResultOpenType;			//本局开奖的类型[0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜]
	int32_t  nMuilt[BET_ITEM_COUNT];	//赔率
	int		 iResultId_zx;				//和庄闲开奖的ID
	int		 iResultMul_zx;				//和庄闲开奖的倍数

	// 和庄闲是否倍投
	bool        lLastWinOrLost;          //庄闲上一局玩家的赢输记录
	int64_t     lLastJetton;
	int32_t     iDoubleCastTimes;        //出现倍投的次数
	int32_t     iLoseTimes;              //输钱的次数
	bool        isDoubleCast;            //本次是否是倍投

};

class OpenResultInfo
{
public:
	OpenResultInfo()
	{
		iHZXId = -1;
		iAnimalId = -1;
		iAnimalId_sd = -1;
		iopenTypeTag = -1;
		notOpenCount = 0;
	}
	void clear()
	{
		iHZXId = -1;
		iAnimalId = -1;
		iAnimalId_sd = -1;
		iopenTypeTag = -1;
		notOpenCount = 0;
	}

	int		iHZXId;				//和庄闲
	int		iAnimalId;			//动物ID
	int		iAnimalId_sd;		//送灯的动物ID
	int		iopenTypeTag;		//本局开奖的类型[0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜]
	int		notOpenCount;		//连续多少局没开
	
};

class TableFrameSink : public ITableFrameSink
{

public:
    // 能取上限
    static int RandomInt(int a, int b)
    {
        return rand() % (b - a + 1) + a;
    }

    float	m_fTaxRate; 
    float	m_fTMJettonBroadCard; 
	int32_t m_testRet;
    int		m_testInex;
    int		m_testResultInex;
	int		m_testInex_sd;
	int		m_testResultInex_sd;
	int		m_testOpenType;
	int		m_testResultOpenType;
	int		m_testResultInex_zx;
	int		m_testInex_zx;

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
    void GamePlayerJetton1(int64_t score, int index, int chairid,shared_ptr<CServerUserItem> &pSelf);
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
	void updateResultList(tagResultInfo resultInfo);
	// 100局游戏记录
	void QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	void sendGameRecord(int32_t chairid);
	// 获取本局颜色圈排序
	void getColourCircle();
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	//获取停止的ID
    void GetStopIndex(int animalid, int& stopIndex, int& colourIndex, int& stopIndex_sd, int& colourIndex_sd);
	// 统计随机开奖时开奖类型 
	// 0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜
	void CalculatWeightAndOpenType(int &typeTag);
	//获取本局的动物平均赔率(0:所有动物;1:霹雳闪电;2:送灯;3:大三元:4:大四喜)
	int getAverageMuilt(int type, int animalid);
	// 按动物权重获取动物
	int getRandAnimalID(int type = 0);
	//计算测试赢分
	long CalculatTestWinScore(int winTag, int typeTag, int winTagHzx);
	//判断玩家是不是倍投庄闲
	void JudgmentUserIsDoubleCast();
	//重排近20局玩家盈利
	void SortPlayerList();

private:
    int						m_iGameState; 
    int						m_iCurrBankerId;
    int						m_iShenSuanZiChairId;
    int						m_iShenSuanZiUserId;
    int						m_iRetMul;
    int						m_iResultId;
    int						m_iStopIndex;
    int						m_BestPlayerUserId;
    int						m_BestPlayerChairId;
    int						m_bIsIncludeAndroid;
    int						m_nReadCount;
    int						m_betCountDown;
    int						m_endCountDown; 
    int						m_timetick;
    int						m_timetickAll;
    bool					m_bIsOpen;
    bool					m_bIsContinue;
    bool					m_bIsClearUser;
	float					m_addCountDown_sd;				//开金鲨银鲨送灯增加的时间
	float					m_addCountDown_cj;				//开金鲨彩金动画增加的时间
	float					m_fGameStartAniTime;			//预留前端开始动画的时间
	float					m_fGameEndAniTime;				//预留前端结束动画的时间
	int						m_iResultId_sd;					//赠送的动物类型
	int						m_iStopIndex_sd;				//送灯时赠送的动物停止的位置ID　

	float					m_fResetPlayerTime;				//本局游戏结束刷新玩家排序的时间

	int 					m_nColourCircle[CIRCLE_COUNT];	//颜色圈排列
	int 					m_nAnimalCircle[CIRCLE_COUNT];	//动物圈排列
	int						m_iChooseMuilt;					//本局选择的赔率表
	int						m_iColourCount[3][3];			//本局选择的颜色数目
	int32_t					m_nAllMuilt[3][BET_ITEM_COUNT];	//三种倍率表
	int						m_iResultHZXId;					//和庄闲结果
	int						m_iResultMul_hzx;				//和庄闲开奖的倍数
	int						m_iStopColourIndex;			    //选择颜色停止的位置ID　
	int						m_iStopColourIndex_sd;			//送灯时选择颜色停止的位置ID
	int						m_iResultTypeTag;				//0:正常开奖 1:霹雳闪电 2:送灯 3:大三元 4:大四喜
	int						m_iResultId_tmp[5];				//特殊开奖时的临时结果
	int32_t					m_lOpenTypeCount[5];            //累计开奖类型次数
	bool					m_bHaveColWeight;				//是否已经计算了动物权重
	int32_t					m_TmpRetWeight[12];				//12个动物临时权重
	int32_t					m_TmpnMuilt[12];				//12个动物平均倍数
	float					m_fAverageMuilt[5];				//设置特殊类型开奖权重是平均动物倍数的几倍
	int32_t					m_HzxRetWeight[3];				//和庄闲权重

	int32_t					m_lastStopIndex;				//上局指针停止位置
	int32_t					m_lastResultHZXId;				//上局和庄闲结果
    int64_t					m_lMustKillScore;				//黑名单必杀押注门槛
	int32_t					m_lMustKillRate;				//黑名单必杀概率

    //干涉概率
    float					m_ctrlRate;
    float					m_stockPercentRate;

    int64_t					m_BestPlayerWinScore;
    int32_t					m_EapseTime; 
    int64_t					m_MarqueeScore;
    int64_t					m_nAreaMaxJetton[BET_ITEM_COUNT];
    int32_t					m_retWeight[BET_ITEM_COUNT+1];
    int32_t					m_nMuilt[BET_ITEM_COUNT];// 倍率表
	

	int64_t                 m_lShenSuanZiJettonScore[BET_ITEM_COUNT];
	int64_t                 m_ShenSuanZiId;

    // new玩家列表
    map<int64_t, shared_ptr<UserInfo>>		m_UserInfo;
    // 玩家列表
    vector<shared_ptr<UserInfo>>			m_pPlayerInfoList;
	// 20局玩家列表排序
    vector<UserInfo>						m_pPlayerInfoList_20;
    // 最佳玩家列表
    vector<BestPlayerList>					m_vPlayerList;
	vector<tagResultInfo>					m_vResultRecored;	//
	tagResultInfo							m_lastResultInfo;
    vector<int64_t>							m_cbJettonMultiple; //筹码值

	// 100局开奖记录
    map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount;
	map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount_last;


    shared_ptr<BetArea>						m_pBetArea;
    shared_ptr<ITableFrame>					m_pITableFrame;
private:
    tagGameRoomInfo							*m_pGameRoomInfo;
    tagGameReplay							m_replay; 
    string									m_strRoundId;
    uint32_t								m_dwReadyTime;
    chrono::system_clock::time_point		m_startTime;

    muduo::MutexLock						m_mutexLock;
    muduo::net::TimerId						m_TimerIdStart;
    muduo::net::TimerId						m_TimerIdEnd;
    muduo::net::TimerId						m_TimerIdTest; 
    muduo::net::TimerId						m_TimerOtherBetJetton;
	muduo::net::TimerId						m_TimerIdResetPlayerList;
    shared_ptr<muduo::net::EventLoopThread> m_LoopThread;
private: 
    static int64_t							m_llStorageControl;									//库存控制
    static int64_t							m_llStorageLowerLimit;								//库存控制下限值
    static int64_t							m_llStorageHighLimit;								//库存控制上限值
    static int64_t							m_llStorageAverLine;          //average line
    static int64_t							m_llGap;            //refer gap:for couting rate
	int64_t									m_lOpenResultAllCount;                              //累计开奖总局数
    int32_t									m_lOpenResultCount;                                 //累计开奖次数
    int32_t									m_lOpenResultIndexCount[BET_ITEM_COUNT];            //各下注项累计开奖次数
    int32_t									m_bTest;            //是否测试
    int32_t									m_nTestTimes;        //测试的局数
    int32_t									m_nTestType;        //测试的类型
    bool									m_bTestAgain;       
    // 是否写日记
    bool									m_bWritelog;
    double									stockWeak;
    int										m_lLimitPoints;
    double									m_dControlkill;

    SlwhAlgorithm							slwhAlgorithm;

    int64_t									m_useIntelligentChips;
    int64_t									m_userScoreLevel[4];
    int64_t									m_userChips[4][CHIP_COUNT];
    int64_t									m_currentchips[MAX_PLAYER][CHIP_COUNT];

    STD::Random								m_random;
    int32_t									m_difficulty;               //玩家难度 千分制
	bool									m_bGameEnd;					//本局是否结束
	bool									m_bRefreshPlayerList;		//本局是否刷新玩家排序

	// 测试权重开奖数据
    int64_t m_testdAreaJetton[BET_ITEM_COUNT];
	int64_t m_testTotalIn;
	int64_t m_testdAllOut;
	int64_t m_TotalIn;
	int64_t m_TotalOut;
	int32_t m_lTestOpenCount;                                 //累计开奖次数
	float   m_fTestTime;
	int32_t m_testBetType;
	int32_t m_tsetOdds;
	int32_t m_testTimes;
	int32_t m_testBetIndex;
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

