#include "ConfigManager.h"
#include <stdio.h>
#include<string.h>
#include <fstream>
#include <iostream>


ConfigManager * ConfigManager::m_Instance = NULL;

#define _atoi64(val)     strtoll(val, NULL, 10)
 
ConfigManager::ConfigManager()
{
    m_LastReadTime = 0L;
    m_CurrentRound = 0;
}

ConfigManager::~ConfigManager()
{

}

//测试
int ConfigManager::InitTest(tagAllConfig allConfig){
 
    LOG(WARNING) << "m_MaxBet " << allConfig.m_MaxBet;
    LOG(WARNING) << "m_MinBet " << allConfig.m_MinBet;
    //记录日志
    LOG(WARNING) << "nKickOutTime " << allConfig.nKickOutTime;
    //拉动时间间隔
    LOG(WARNING) << "nPullInterval " << allConfig.nPullInterval;
    // if(Index % 5 == 4){
    //     LOG(WARNING) <<szFile;
    //     strcpy(szFile,"");
    // }
    LOG(WARNING) << "------UserProbabilityItem start------ ";
    char szFile[256] = { 0 };
    char szTemp[64] = { 0 }; 
    for (size_t k = 0; k < 3; k++)
    {
        LOG(WARNING) << "---------- "<< k <<" ---------";
        strcpy(szFile,"");

        for (size_t n = 0; n < FRUIT_MAX; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vProbability[k].nFruitPro[n]);
            strcat(szFile,szTemp);  
        }

        LOG(WARNING) <<"nFruitPro:"<<szFile;
        strcpy(szFile,"");

//        for (size_t n = 0; n < 9; n++)
//        {
//            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vProbability[k].nBounsPro[n]);
//            strcat(szFile,szTemp);
//        }
//        LOG(WARNING) <<"nBounsPro:"<<szFile;
     
        strcpy(szFile,"");
        for (size_t n = 0; n < 5; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.m_line[k][n]);
            strcat(szFile,szTemp);  
        }
        LOG(WARNING) <<"Test:"<<szFile;
    }
    LOG(WARNING) << "------UserProbabilityItem end------ ";
    
    return 1;
}
 

int ConfigManager::Init(const char AppName[],std::string szConfigName)//const char szConfigName[])//std::
{ 
    // allConfig.CopyTo(m_Config);
    return 0;
}

//检查并读取配置
int	 ConfigManager::CheckReadConfig()
{
    if ((time(NULL) - m_LastReadTime) <= 1){ 
        return 0;
    }
    m_LastReadTime = time(NULL); 

    std::string backPondDir;  
    //2,读取游戏设置数据
    std::string configDir; 
    configDir = str(boost::format("./conf/csd_config.ini"));
    if(!boost::filesystem::exists(configDir)) {
        LOG(WARNING) << "./conf/csd_config.ini not exists";
        return 0;
    }

    //读取游戏设置数据configDir
    boost::property_tree::ptree  pt;
    boost::property_tree::read_ini(configDir, pt);

	LOG(WARNING) << "=========>>>>>> FruitConfig 1 ";

    //读取游戏设置数据
    memset(&m_Config, 0, sizeof(m_Config));
   


    m_Config.nKickOutTime  = pt.get<int32_t>("FruitConfig.kickOutTime", 120); 
    m_Config.nPullInterval = pt.get<int32_t>("FruitConfig.PullInterval", 2); 
    m_Config.m_CtrlVal = pt.get<int64_t>("FruitConfig.ControlVal", 1000); 
    // 奖池押分抽水率(百分比)
    m_Config.m_BetRateVal = pt.get<int32_t>("FruitConfig.BetRateVal", 1); 
    m_Config.m_MsgItemCount = pt.get<int32_t>("FruitConfig.MsgItemCount", 10); 
    m_Config.m_MiniJackPotId = pt.get<int32_t>("FruitConfig.MiniJackPotId", 10); 
    m_Config.m_PullJackpotRecInterval = pt.get<int32_t>("FruitConfig.PullJackpotRecInterval", 300); 

    //最大机器人数量
    m_Config.m_MaxAndroidCount = pt.get<int32_t>("ANDROID_CONFIG.MaxAndroidCount", 5); 
    //最小播报倍数条件
    m_Config.m_MinBoardCastOdds = pt.get<int32_t>("ANDROID_CONFIG.MinBoardCastOdds", 10); 
    LOG(WARNING) <<"最大机器人数量"<< m_Config.m_MaxAndroidCount <<" BoardCast:"<< m_Config.m_MinBoardCastOdds;
   
    // 获Scatter对应获奖比例
    m_Config.m_ScatterJackPot[0] = pt.get<int32_t>("FruitConfig.ScatterJackPot_3", 5); 
    m_Config.m_ScatterJackPot[1] = pt.get<int32_t>("FruitConfig.ScatterJackPot_4", 10); 
    m_Config.m_ScatterJackPot[2] = pt.get<int32_t>("FruitConfig.ScatterJackPot_5", 20); 

    //测试是否开启
    m_Config.m_testCount = pt.get<int64_t>("FruitConfig.testCount", 1000); 
    m_Config.m_testWeidNum = pt.get<int64_t>("FruitConfig.testWeidNum", 1); 
    m_Config.m_runAlg = pt.get<int64_t>("FruitConfig.runAlg", 0); 

    m_Config.stockweak=pt.get<double>("FruitConfig.STOCK_WEAK", 1.0);
    int betItemtCount       = pt.get<int32_t>("FruitConfig.BetItemCount", 9);

    m_Config.m_BestWinNum = pt.get<int64_t>("FruitConfig.BestWinNum", 10000000);


	LOG(WARNING) << "=========>>>>>> FruitConfig 2 "<< m_Config.nKickOutTime <<" "<< m_Config.nPullInterval<< " "<< betItemtCount;

    const TCHAR * p = nullptr;

    //押分列表
    {
        vBetItemList.clear();

        int nMinBet = 0;
        int nMaxBet = 0;

        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("FruitConfig");
        TCHAR KeyName[64] = TEXT("");
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("BetItemList"));

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"1,1", OutBuf, sizeof(OutBuf), configDir.c_str());
            LOG(WARNING) << " OutBuf "<< OutBuf;

            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr){
                LOG(WARNING) << " BetItemList nullptr ";
                return 1;
            }

            int32_t temp = atoi(p);
            vBetItemList.push_back(temp);

            nMinBet = temp;
            nMaxBet = temp;

            for (size_t i = 0; i < betItemtCount - 1; i++)
            {
                p = mystchr(p, ',') + 1;
                temp = atoi(p);
                vBetItemList.push_back(temp);
                if(nMinBet > temp) nMinBet = temp;
                if(nMaxBet < temp) nMaxBet = temp;
            }

        }

        LOG(WARNING) << "==========>>>>>> BetItemList "<< vBetItemList.size() <<" "<< nMinBet<<" "<< nMaxBet;
        //9根线

        if(vBetItemList.size() > 0){
            m_Config.m_MaxBet = nMaxBet * 9; 
            m_Config.m_MinBet = nMinBet * 9; 
        }
    }

    //派奖获奖比例(千分比)
    {
        vRewardList.clear(); 

        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("FruitConfig");
        TCHAR KeyName[64] = TEXT("");
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("RewardList"));

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"1", OutBuf, sizeof(OutBuf), configDir.c_str());
            LOG(WARNING) << " OutBuf "<< OutBuf;

            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr){
                LOG(WARNING) << " vRewardList nullptr ";
                return 1;
            }

            int32_t temp = atoi(p);
            vRewardList.push_back(temp);

            for (size_t i = 0; i < 9; i++)
            {
                p = mystchr(p, ',') + 1;
                temp = atoi(p);
                vRewardList.push_back(temp);
                LOG(WARNING) << i <<" "<<temp ;
            }
        }

        LOG(WARNING) << "==========>>>>>> vRewardList "<< vRewardList.size();
    }

    //库存概率
    {
        TCHAR DefualtBuf[255] = TEXT("");
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Store");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        {
            for(int i=0;i<4;i++)
            {
                UserProbabilityItem Item;

                ZeroMemory(KeyName,sizeof(KeyName));
                wsprintf(KeyName, TEXT("NetScoreConfig%d"), i+1 );
                // 
                int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
                // LOG(WARNING) << " OutBuf "<< OutBuf;

                ASSERT(readSize > 0);
                p = OutBuf;
                if(p==nullptr){
                    LOG(WARNING) << " Store nullptr ";
                    return 1;
                }
                //解析
                {
                    //每个水果的概率
                    int32_t temp = atoi(p);
                    Item.nFruitPro[0] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[1] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[2] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[3] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[4] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[5] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[6] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[7] = temp;
                    p = mystchr(p, ',') + 1;  temp = atoi(p);
                    Item.nFruitPro[8] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[9] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    Item.nFruitPro[10] = temp;                 
                }
                m_Config.vProbability[i]= Item;
            }

            LOG(WARNING) << "====================>>>>>> FruitConfig 3 ";
        }
    }

    //测试
    {
        
        m_Config.m_testLine  = pt.get<int64_t>("Test.testLine", 0); 

        TCHAR DefualtBuf[255] = TEXT("");
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Test");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        if( m_Config.m_testLine > 0 )
        {
            for(int i=0;i<3;i++)
            {
                ZeroMemory(KeyName,sizeof(KeyName));
                wsprintf(KeyName, TEXT("line%d"), i+1 );
                // 
                int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
                LOG(WARNING) << " OutBuf "<< OutBuf;

                ASSERT(readSize > 0);
                p = OutBuf;
                if(p==nullptr){
                    LOG(WARNING) << " Test nullptr ";
                    return 1;
                }
                //解析
                {
                    //每个水果的概率
                    int32_t temp = atoi(p);
                    m_Config.m_line[i][0] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.m_line[i][1] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.m_line[i][2] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.m_line[i][3] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.m_line[i][4] = temp;
                } 
            }

            LOG(WARNING) << "====================>>>>>> Test.testLine 3 ";
        }
    }
 
    LOG(WARNING) << m_Config.m_testCount <<" WeidNum:"<<m_Config.m_testWeidNum<<" runAlg:"<<m_Config.m_runAlg;

    InitTest(m_Config);

    return 0;
}

void ConfigManager::GetConfig(tagAllConfig &config) 
{
   config.m_CtrlVal = m_Config.m_CtrlVal;
   config.m_MaxBet = m_Config.m_MaxBet;
   config.m_MinBet = m_Config.m_MinBet;
   config.nKickOutTime = m_Config.nKickOutTime;
   config.nPullInterval = m_Config.nPullInterval;
   config.m_testCount = m_Config.m_testCount;
   config.m_testWeidNum = m_Config.m_testWeidNum;
   config.m_runAlg = m_Config.m_runAlg;
   config.m_BestWinNum = m_Config.m_BestWinNum;
   config.m_BetRateVal = m_Config.m_BetRateVal;
   config.m_MsgItemCount = m_Config.m_MsgItemCount; 
   config.m_MiniJackPotId = m_Config.m_MiniJackPotId; 
   config.m_PullJackpotRecInterval = m_Config.m_PullJackpotRecInterval; 

   config.m_MaxAndroidCount = m_Config.m_MaxAndroidCount;  
   config.m_MinBoardCastOdds = m_Config.m_MinBoardCastOdds;  

   config.m_testLine= m_Config.m_testLine;

   config.stockweak= m_Config.stockweak;

   for (size_t i = 0; i < 3; i++)
   {
       config.vProbability[i] = m_Config.vProbability[i];
       //
       config.m_ScatterJackPot[i] = m_Config.m_ScatterJackPot[i];

       for (size_t n = 0; n < 5;n++)
          config.m_line[i][n] = m_Config.m_line[i][n]; 
    }
}
  
vector<int> ConfigManager::GetBetItemList()
{
    return vBetItemList;
}

vector<int> ConfigManager::GetRewardList()
{
    return vRewardList;
}

int ConfigManager::GetCurrentRound()
{
    if (m_CurrentRound < 0)
        return 0;
    m_CurrentRound++;
    if (m_CurrentRound == INT_MAX)	m_CurrentRound = 0;
    int  CurrentRound = m_CurrentRound;
    return CurrentRound;
}

void ConfigManager::ResetCurrentRound()
{
    m_CurrentRound = 0;
}

 /* 
    cout << boost::format("%1%"  
    "%1t 十进制 = [%2$d]\n"  
    "%1t 格式化的十进制 = [%2$5d]\n" 
    "%1t 格式化十进制，前补'0' = [%2$05d]\n" 
    "%1t 十六进制 = [%2$x]\n" 
    "%1t 八进制 = [%2$o]\n" 
    "%1t 浮点 = [%3$f]\n" 
    "%1t 格式化的浮点 = [%3$3.3f]\n" 
    "%1t 科学计数 = [%3$e]\n" 
    ) % "example :\n" % 15 % 15.01 << endl; 
     */ 
