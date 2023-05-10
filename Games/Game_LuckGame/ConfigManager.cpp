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
    LOG(WARNING) << "------WeidghtItem start------ ";
    char szFile[256] = { 0 };
    char szTemp[64] = { 0 }; 
    for (size_t k = 0; k < 3; k++)
    {
        LOG(WARNING) << "---------- "<< k <<" ---------";
        strcpy(szFile,"");

        for (size_t n = 0; n < AWARD_COUNT; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vProbability[k].Weight[n]);
            strcat(szFile,szTemp);  
        }

        LOG(WARNING) <<"Weight:"<<szFile;
        strcpy(szFile,"");

        for (size_t n = 0; n < 9; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vProbability[k].Gold[n]);
            strcat(szFile,szTemp);  
        }
        LOG(WARNING) <<"Gold:"<<szFile;
     
        strcpy(szFile,"");
        for (size_t n = 0; n < 5; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.m_line[k][n]);
            strcat(szFile,szTemp);  
        }
        LOG(WARNING) <<"Test:"<<szFile;
    }
    LOG(WARNING) << "------WeidghtItem end------ ";
    
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
    configDir = str(boost::format("./conf/lucky_game_config.ini"));
    if(!boost::filesystem::exists(configDir)) {
        LOG(WARNING) << "./conf/lucky_game_config.ini not exists";
        return 0;
    }

    //读取游戏设置数据configDir
    boost::property_tree::ptree  pt;
    boost::property_tree::read_ini(configDir, pt);

	LOG(WARNING) << "=========>>>>>> LuckConfig 1 ";

    //读取游戏设置数据
    memset(&m_Config, 0, sizeof(m_Config));
   
    m_Config.nKickOutTime   = pt.get<int64_t>("LuckConfig.kickOutTime", 120); 
    m_Config.nPullInterval  = pt.get<int64_t>("LuckConfig.PullInterval", 2); 
    m_Config.m_CtrlVal      = pt.get<int64_t>("LuckConfig.ControlVal", 1000); 
    m_Config.m_ExChangeRate = pt.get<int64_t>("LuckConfig.ExChangeRate", 5000); 

    //测试是否开启
    m_Config.m_testCount    = pt.get<int64_t>("LuckConfig.testCount", 1000); 
    m_Config.m_testWeidNum  = pt.get<int64_t>("LuckConfig.testWeidNum", 1); 
    m_Config.m_runAlg       = pt.get<int64_t>("LuckConfig.runAlg", 0); 

    
    int betItemtCount       = pt.get<int32_t>("LuckConfig.BetItemCount", 9);


	LOG(WARNING) << "=========>>>>>> LuckConfig 2 "<< m_Config.nKickOutTime <<" "<< m_Config.nPullInterval<< " "<< betItemtCount;

    const TCHAR * p = nullptr;

    //押分列表
    {
        vBetItemList.clear();

        int nMinBet = 0;
        int nMaxBet = 0;

        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("LuckConfig");
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

    //Weight
    {
        TCHAR DefualtBuf[255] = TEXT("");
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Weight");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        {
            for(int i=0;i<3;i++)
            { 
                ZeroMemory(KeyName,sizeof(KeyName));
                wsprintf(KeyName, TEXT("Weight%d"), i );
                // 
                int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
                LOG(WARNING) << " OutBuf "<< OutBuf;

                ASSERT(readSize > 0);
                p = OutBuf;
                if(p==nullptr){
                    LOG(WARNING) << " Store nullptr ";
                    return 1;
                }
                //解析
                {
                    //Weight的值
                    int32_t temp = atoi(p);
                    m_Config.vProbability[i].Weight[0] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[1] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[2] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[3] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[4] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[5] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[6] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[7] = temp;
                    p = mystchr(p, ',') + 1;  temp = atoi(p);
                    m_Config.vProbability[i].Weight[8] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[9] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[10] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Weight[11] = temp; 
                } 
            }

            LOG(WARNING) << "====================>>>>>> Weight 3 ";
        }
    }

     //Gold
    {
        TCHAR DefualtBuf[255] = TEXT("");
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Gold");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        {
            for(int i=0;i<3;i++)
            { 
                ZeroMemory(KeyName,sizeof(KeyName));
                wsprintf(KeyName, TEXT("Gold%d"), i );
                // 
                int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
                LOG(WARNING) << " OutBuf "<< OutBuf;

                ASSERT(readSize > 0);
                p = OutBuf;
                if(p==nullptr){
                    LOG(WARNING) << " Store nullptr ";
                    return 1;
                }
                //解析
                {
                    //Gold的值
                    int32_t temp = atoi(p);
                    m_Config.vProbability[i].Gold[0] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[1] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[2] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[3] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[4] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[5] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[6] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[7] = temp;
                    p = mystchr(p, ',') + 1;  temp = atoi(p);
                    m_Config.vProbability[i].Gold[8] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[9] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[10] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Gold[11] = temp; 
                } 
            }

            LOG(WARNING) << "====================>>>>>> Gold 3 ";
        }
    }

     //Gold
    {
        TCHAR DefualtBuf[255] = TEXT("");
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Icon");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        {
            for(int i=0;i<3;i++)
            { 
                ZeroMemory(KeyName,sizeof(KeyName));
                wsprintf(KeyName, TEXT("Icon%d"), i );
                // 
                int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
                LOG(WARNING) << " OutBuf "<< OutBuf;

                ASSERT(readSize > 0);
                p = OutBuf;
                if(p==nullptr){
                    LOG(WARNING) << " Store nullptr ";
                    return 1;
                }
                //解析
                {
                    //Gold的值
                    int32_t temp = atoi(p);
                    m_Config.vProbability[i].Icon[0] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[1] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[2] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[3] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[4] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[5] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[6] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[7] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[8] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[9] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[10] = temp;
                    p = mystchr(p, ',') + 1; temp = atoi(p);
                    m_Config.vProbability[i].Icon[11] = temp; 
                } 
            }

            LOG(WARNING) << "====================>>>>>> Icon 3 ";
        }
    }

    //测试        
    m_Config.m_testLine  = pt.get<int64_t>("Test.testLine", 0); 
 
 
    LOG(WARNING) << m_Config.m_testCount <<" WeidNum:"<<m_Config.m_testWeidNum<<" runAlg:"<<m_Config.m_runAlg;

    InitTest(m_Config);

    return 0;
}

void ConfigManager::GetConfig(tagAllConfig &config) 
{
   config.m_ExChangeRate = m_Config.m_ExChangeRate;
   config.m_CtrlVal = m_Config.m_CtrlVal;
   config.m_MaxBet = m_Config.m_MaxBet;
   config.m_MinBet = m_Config.m_MinBet;
   config.nKickOutTime = m_Config.nKickOutTime;
   config.nPullInterval = m_Config.nPullInterval;
   config.m_testCount = m_Config.m_testCount;
   config.m_testWeidNum = m_Config.m_testWeidNum;
   config.m_runAlg = m_Config.m_runAlg;

   config.m_testLine= m_Config.m_testLine;

   for (size_t i = 0; i < 3; i++)
   {
       config.vProbability[i] = m_Config.vProbability[i]; 

       for (size_t n = 0; n < 5;n++)
          config.m_line[i][n] = m_Config.m_line[i][n]; 
    }
}
  
vector<int> ConfigManager::GetBetItemList()
{
    return vBetItemList;
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
