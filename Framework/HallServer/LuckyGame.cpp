

#include <time.h>
#include "public/types.h"
#include <vector>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <list>

#include "LuckyGame.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "public/INIHelp.h"
#include "public/Random.h"
#include <muduo/base/Logging.h>

//宏定义
#ifndef _UNICODE
#define myprintf	_snprintf
#define mystrcpy	strcpy
#define mystrlen	strlen
#define myscanf		_snscanf
#define	myLPSTR		LPCSTR
#define mystchr		strchr
#define myatoi		atoi
#define mychar		char
#else
#define myprintf	swprintf
#define mystrcpy	wcscpy
#define mystrlen	wcslen
#define myscanf		_snwscanf
#define	myLPSTR		LPWSTR
#define mystchr		wcschr
#define myatoi		_wtoi
#define mychar		TCHAR
#endif


LuckyGame::LuckyGame()
{
    memset(m_nSum,	0, sizeof(m_nSum));
    m_bIsloadOK = false;
	m_ConfigMap.clear();
}
LuckyGame::~LuckyGame()
{
}
//创建结果
int LuckyGame::GetGameResult(int32_t nBetIndex,int32_t & nBetScore,int64_t & nWinScore, int32_t currency)
{
    AssertReturn(nBetIndex >= 0 && nBetIndex < m_nBetItemCount, return -1);
	
    // 产生结果
    int ret = RandomArea(nBetIndex,m_ConfigMap[currency].vItemArry[nBetIndex].Weight);

    // 中奖分
    AssertReturn(ret >= 0 && ret < AWARD_COUNT, return -1);
    nWinScore  = m_ConfigMap[currency].vItemArry[nBetIndex].Gold[ret] * 100;

    // 押分值
    nBetScore = m_ConfigMap[currency].vBetItemList[nBetIndex];
   
    return ret;
}
// 取一个随机的假的奖励分
bool LuckyGame::GetFakeResult(int32_t & nBetScore,int32_t & nFakeWinScore, int32_t currency)
{
    int32_t nBetIndex = rand() % 3;
    int nRandIdx = rand() % 3;
    int fakeIdx[3] = {1,3,5};
    int ret = fakeIdx[nRandIdx];
    nFakeWinScore  = m_ConfigMap[currency].vItemArry[nBetIndex].Gold[ret] * 100; 

    // 押分值
    nBetScore = m_ConfigMap[currency].vBetItemList[nBetIndex];

    int fakeRandom[3] = {100,100,100};
    int nRandNum = CRandom::Instance().Random_Int(1, fakeRandom[nBetIndex]);
    //if(nRandNum == 10)
    //    LOG_WARN << nBetIndex <<" "<< nRandIdx << " "<< ret << " "<< nFakeWinScore << " " << fakeRandom[nBetIndex] <<" "<<nRandNum;
    return nRandNum == 10;
}
//检查并读取配置"./conf/lucky_game_config.ini"; 
int	LuckyGame::LoadConfig(const char*  path)
{
    //1,读取游戏设置数据
    std::string configDir = "./conf/lucky_game_config.ini";
    if(!boost::filesystem::exists(configDir)) {
        LOG_INFO << "./conf/lucky_game_config.ini not exists";
        return 1;
    }

    //读取游戏设置数据
    boost::property_tree::ptree  pt;
    boost::property_tree::read_ini(configDir, pt);

    memset(&m_Config, 0, sizeof(m_Config));
    m_Config.nPullInterval  = pt.get<int32_t>("LuckConfig.PullInterval", 2); 
    m_Config.nExChangeRate = pt.get<int32_t>("LuckConfig.ExChangeRate", 5000); 

    m_Config.nGameId  = pt.get<int32_t>("LuckConfig.GameId", 1350); 
    m_Config.nRoomId  = pt.get<int32_t>("LuckConfig.RoomId", 13501);  
    
    int betItemtCount  = pt.get<int32_t>("LuckConfig.BetItemCount", 3);
    m_nBetItemCount = betItemtCount; 

    // 开放时间段
    int openItemCount  = pt.get<int32_t>("LuckConfig.openItemCount", 2);
    m_openItemCount = openItemCount;
    if( openItemCount > 3 ){  
        LOG_ERROR << "开放时间段参数有误."<< openItemCount;
        return 3;
    }

    //消息列表数量
    m_Config.MsgItemCount  = pt.get<int32_t>("LuckConfig.MsgItemCount", 10);
    m_Config.RecordDays    = pt.get<int32_t>("LuckConfig.RecordDays", 21);
    m_Config.RecordItemCount    = pt.get<int32_t>("LuckConfig.RecordItemCount", 20);//三个星期
    m_Config.bulletinMin    = pt.get<int32_t>("LuckConfig.bulletinMin", 100);//默认100金币提示
    
    

	LOG_INFO << "==>>>>>> LuckConfig 1 "<< m_Config.nPullInterval<< " "<< betItemtCount <<" "<< m_Config.nExChangeRate;
    LOG_INFO << "==>>>>>> LuckConfig 2 "<< m_Config.nGameId<< " "<< m_Config.nRoomId;

    const TCHAR * p = nullptr;
    //开放时间段
    {
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("OpenTime");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        
        for(int i = 0;i < openItemCount;i++)
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("open%d"), i );
            // 
            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"10,2", OutBuf, sizeof(OutBuf), configDir.c_str());
            AssertReturn(readSize > 0, return 3);
            LOG_INFO << " OutBuf "<< OutBuf;

            p = OutBuf;
            if( p == nullptr ){  return 3; }

            //解析open的值
            for (size_t k = 0; k < 2; k++){

                if( k > 0 )  
                    p = mystchr(p, ',') + 1;
                
                int32_t temp = atoi(p);
                if (k == 0)
                    m_Config.vOpenArry[i].startTime = temp; 
                else
                    m_Config.vOpenArry[i].duration = temp; 
            }
        }
       // LOG_INFO << "====================>>>>>> open ";
    }

    //押分列表
    { 
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("LuckConfig");
        TCHAR KeyName[64] = TEXT("");
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("BetItemList"));

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"10,50,200", OutBuf, sizeof(OutBuf), configDir.c_str());
            AssertReturn(readSize > 0,return 2);

            LOG_INFO << " OutBuf "<< OutBuf;

            p = OutBuf;
            if( p == nullptr ){ return 2;}

            int32_t temp = atoi(p);
            m_Config.vBetItemList.push_back(temp);

            for (size_t i = 0; i < betItemtCount - 1; i++)
            {
                p = mystchr(p, ',') + 1;
                temp = atoi(p);
                m_Config.vBetItemList.push_back(temp);
            }
        }

        LOG_INFO << "==========>>>>>> BetItemList "<< m_Config.vBetItemList.size();
    }

    TCHAR DefualtBuf[255] = TEXT("1,1,1,1,1,1,1,1,1,1,1,1");
    //Weight
    {
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Weight");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        
        for(int i=0;i<3;i++)
        { 
            m_nSum[i] = 0;
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("Weight%d"), i );
            // 
            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
            AssertReturn(readSize > 0, return 3);
            LOG_INFO << " OutBuf "<< OutBuf;

            p = OutBuf;
            if( p == nullptr ){  return 3; }

            //解析Weight的值
            for (size_t k = 0; k < AWARD_COUNT; k++){

                if( k > 0 )  
                    p = mystchr(p, ',') + 1;
                
                int32_t temp = atoi(p);

                m_Config.vItemArry[i].Weight[k] = temp;

                //累加权重
                m_nSum[i] += temp;
            }
        }
        //LOG_INFO << "====================>>>>>> Weight 3 ";
    }

    //Gold
    {
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Gold");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        
        for(int i=0;i<3;i++)
        { 
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("Gold%d"), i );
            // 
            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
            AssertReturn(readSize > 0, return 4);
            LOG_INFO << " OutBuf "<< OutBuf;
            
            p = OutBuf;
            if( p == nullptr ){ return 4;}

            //解析Gold的值
            for (size_t k = 0; k < AWARD_COUNT; k++) {

                if( k > 0 )  
                    p = mystchr(p, ',') + 1;
              
                int32_t temp = atoi(p);

                m_Config.vItemArry[i].Gold[k] = temp;
            }
        }
        //LOG_INFO << "====================>>>>>> Gold 3 ";
    }

     //Icon
    {
        TCHAR OutBuf[255] = TEXT("");
        TCHAR INI_SECTION_CONFIG[30] = TEXT("Icon");
        TCHAR KeyName[64] = TEXT("");
        memset(KeyName, 0, sizeof(KeyName));
        
        for(int i=0;i<3;i++)
        { 
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("Icon%d"), i );
            // 
            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
            AssertReturn(readSize > 0,return 5);
            LOG_INFO << " OutBuf "<< OutBuf;
            
            p = OutBuf;
            if( p == nullptr ){ return 5;}

            //解析Icon的值
            for (size_t k = 0; k < AWARD_COUNT; k++) {
                if( k > 0 )  
                    p = mystchr(p, ',') + 1;  

                int32_t temp = atoi(p);

                m_Config.vItemArry[i].Icon[k] = temp;
            }
        }
       // LOG_INFO << "====================>>>>>> Icon 3 ";
    }

    InitTest(m_Config);

    //m_bIsloadOK = true;

    return 0;
}
int	LuckyGame::LoadCurrencyConfig(const char*  path, int32_t currency)
{
	//1,读取游戏设置数据
	std::string configDir = "./conf/lucky_game_config.ini";
	if (!boost::filesystem::exists(configDir)) {
		LOG_INFO << "./conf/lucky_game_config.ini not exists" << " currency:" << currency;
		return 1;
	}

	//读取游戏设置数据
	boost::property_tree::ptree  pt;
	boost::property_tree::read_ini(configDir, pt);
	tagConfig	        tmp_Config;
	memset(&tmp_Config, 0, sizeof(tmp_Config));
	string strCurrency = "_" + std::to_string(currency);
	tmp_Config.nPullInterval = pt.get<int32_t>("LuckConfig.PullInterval", 2);
	tmp_Config.nExChangeRate = pt.get<int32_t>("LuckConfig.ExChangeRate" + strCurrency, 5000);

	tmp_Config.nGameId = pt.get<int32_t>("LuckConfig.GameId", 1350);
	tmp_Config.nRoomId = pt.get<int32_t>("LuckConfig.RoomId", 13501);

	int betItemtCount = pt.get<int32_t>("LuckConfig.BetItemCount", 3);
	m_nBetItemCount = betItemtCount;

	// 开放时间段
	int openItemCount = pt.get<int32_t>("LuckConfig.openItemCount", 2);
	m_openItemCount = openItemCount;
	if (openItemCount > 3) {
		LOG_ERROR << "开放时间段参数有误." << openItemCount;
		return 3;
	}

	//消息列表数量
	tmp_Config.MsgItemCount = pt.get<int32_t>("LuckConfig.MsgItemCount", 10);
	tmp_Config.RecordDays = pt.get<int32_t>("LuckConfig.RecordDays", 21);
	tmp_Config.RecordItemCount = pt.get<int32_t>("LuckConfig.RecordItemCount", 20);//三个星期
	tmp_Config.bulletinMin = pt.get<int32_t>("LuckConfig.bulletinMin", 100);//默认100金币提示


	LOG_INFO << "==>>>>>> LoadCurrencyConfig currency  " << currency;
	LOG_INFO << "==>>>>>> LuckConfig 1 " << tmp_Config.nPullInterval << " " << betItemtCount << " " << tmp_Config.nExChangeRate;
	LOG_INFO << "==>>>>>> LuckConfig 2 " << tmp_Config.nGameId << " " << tmp_Config.nRoomId;

	const TCHAR * p = nullptr;
	//开放时间段
	{
		TCHAR OutBuf[255] = TEXT("");
		TCHAR INI_SECTION_CONFIG[30] = TEXT("OpenTime");
		TCHAR KeyName[64] = TEXT("");
		memset(KeyName, 0, sizeof(KeyName));

		for (int i = 0;i < openItemCount;i++)
		{
			ZeroMemory(KeyName, sizeof(KeyName));
			wsprintf(KeyName, TEXT("open%d"), i);
			// 
			int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"10,2", OutBuf, sizeof(OutBuf), configDir.c_str());
			AssertReturn(readSize > 0, return 3);
			LOG_INFO << " OutBuf " << OutBuf;

			p = OutBuf;
			if (p == nullptr) { return 3; }

			//解析open的值
			for (size_t k = 0; k < 2; k++) {

				if (k > 0)
					p = mystchr(p, ',') + 1;

				int32_t temp = atoi(p);
				if (k == 0)
					tmp_Config.vOpenArry[i].startTime = temp;
				else
					tmp_Config.vOpenArry[i].duration = temp;
			}
		}
		// LOG_INFO << "====================>>>>>> open ";
	}

	//押分列表
	{
		TCHAR OutBuf[255] = TEXT("");
		TCHAR INI_SECTION_CONFIG[30] = TEXT("LuckConfig");
		TCHAR KeyName[64] = TEXT("");
		{
			ZeroMemory(KeyName, sizeof(KeyName));
			wsprintf(KeyName, TEXT("BetItemList_%d"), currency);

			int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"10,50,200", OutBuf, sizeof(OutBuf), configDir.c_str());
			AssertReturn(readSize > 0, return 2);

			LOG_INFO << " OutBuf " << OutBuf;

			p = OutBuf;
			if (p == nullptr) { return 2; }

			int32_t temp = atoi(p);
			tmp_Config.vBetItemList.push_back(temp);

			for (size_t i = 0; i < betItemtCount - 1; i++)
			{
				p = mystchr(p, ',') + 1;
				temp = atoi(p);
				tmp_Config.vBetItemList.push_back(temp);
			}
		}

		LOG_INFO << "==========>>>>>> BetItemList " + strCurrency << " ," << tmp_Config.vBetItemList.size() << " ," << tmp_Config.vBetItemList[0] << " ," << tmp_Config.vBetItemList[1] << " ," << tmp_Config.vBetItemList[2];
	}

	TCHAR DefualtBuf[255] = TEXT("1,1,1,1,1,1,1,1,1,1,1,1");
	//Weight
	{
		TCHAR OutBuf[255] = TEXT("");
		TCHAR INI_SECTION_CONFIG[30] = TEXT("Weight");
		TCHAR KeyName[64] = TEXT("");
		memset(KeyName, 0, sizeof(KeyName));

		for (int i = 0;i < 3;i++)
		{
			m_nSum[i] = 0;
			ZeroMemory(KeyName, sizeof(KeyName));
			wsprintf(KeyName, TEXT("Weight%d"), i);
			// 
			int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
			AssertReturn(readSize > 0, return 3);
			LOG_INFO << " OutBuf " << OutBuf;

			p = OutBuf;
			if (p == nullptr) { return 3; }

			//解析Weight的值
			for (size_t k = 0; k < AWARD_COUNT; k++) {

				if (k > 0)
					p = mystchr(p, ',') + 1;

				int32_t temp = atoi(p);

				tmp_Config.vItemArry[i].Weight[k] = temp;

				//累加权重
				m_nSum[i] += temp;
			}
		}
		//LOG_INFO << "====================>>>>>> Weight 3 ";
	}

	//Gold
	{
		TCHAR OutBuf[255] = TEXT("");
		TCHAR INI_SECTION_CONFIG[30] = TEXT("");
		ZeroMemory(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG));
		wsprintf(INI_SECTION_CONFIG, TEXT("Gold_%d"), currency);

		TCHAR KeyName[64] = TEXT("");
		memset(KeyName, 0, sizeof(KeyName));

		for (int i = 0;i < 3;i++)
		{
			ZeroMemory(OutBuf, sizeof(OutBuf));
			ZeroMemory(KeyName, sizeof(KeyName));
			wsprintf(KeyName, TEXT("Gold_%d_%d"), currency, i);
			// 
			int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
			AssertReturn(readSize > 0, return 4);
			LOG_INFO << " INI_SECTION_CONFIG " << INI_SECTION_CONFIG << " OutBuf " << OutBuf;

			p = OutBuf;
			if (p == nullptr) { return 4; }

			//解析Gold的值
			for (size_t k = 0; k < AWARD_COUNT; k++) {

				if (k > 0)
					p = mystchr(p, ',') + 1;

				int32_t temp = atoi(p);

				tmp_Config.vItemArry[i].Gold[k] = temp;
			}
		}
		//LOG_INFO << "====================>>>>>> Gold 3 ";
	}

	//Icon
	{
		TCHAR OutBuf[255] = TEXT("");
		TCHAR INI_SECTION_CONFIG[30] = TEXT("Icon");
		TCHAR KeyName[64] = TEXT("");
		memset(KeyName, 0, sizeof(KeyName));

		for (int i = 0;i < 3;i++)
		{
			ZeroMemory(KeyName, sizeof(KeyName));
			wsprintf(KeyName, TEXT("Icon%d"), i);
			// 
			int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, DefualtBuf, OutBuf, sizeof(OutBuf), configDir.c_str());
			AssertReturn(readSize > 0, return 5);
			LOG_INFO << " OutBuf " << OutBuf;

			p = OutBuf;
			if (p == nullptr) { return 5; }

			//解析Icon的值
			for (size_t k = 0; k < AWARD_COUNT; k++) {
				if (k > 0)
					p = mystchr(p, ',') + 1;

				int32_t temp = atoi(p);

				tmp_Config.vItemArry[i].Icon[k] = temp;
			}
		}
		// LOG_INFO << "====================>>>>>> Icon 3 ";
	}

	//InitTest(m_Config);

	//m_bIsloadOK = true;
	
	auto it = m_ConfigMap.find(currency);
	if (m_ConfigMap.end() == it)
	{
		m_ConfigMap[currency] = tmp_Config;
	}
	if (currency == 5)
	{
		m_bIsloadOK = true;
	}
	return 0;
}
//测试
int LuckyGame::InitTest(tagConfig allConfig){
   
   // LOG_INFO << "nPullInterval " << allConfig.nPullInterval;

    LOG_INFO << "------WeidghtItem start------ ";

    char szFile[256] = { 0 };
    char szTemp[64] = { 0 }; 
    for (size_t k = 0; k < 3; k++)
    {
        LOG_INFO << "---------- "<< k <<" ---------";

        strcpy(szFile,"");
        for (size_t n = 0; n < AWARD_COUNT; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vItemArry[k].Weight[n]);
            strcat(szFile,szTemp);  
        }
        LOG_INFO <<"Weight:"<<szFile;

        strcpy(szFile,"");
        for (size_t n = 0; n < AWARD_COUNT; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vItemArry[k].Gold[n]);
            strcat(szFile,szTemp);  
        }
        LOG_INFO <<"Gold:"<<szFile;


        strcpy(szFile,"");
        for (size_t n = 0; n < AWARD_COUNT; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d ", allConfig.vItemArry[k].Icon[n]);
            strcat(szFile,szTemp);  
        }
        LOG_INFO <<"Icon:"<<szFile;

        strcpy(szFile,"");
        for (size_t n = 0; n < m_openItemCount; n++)
        {
            snprintf(szTemp, sizeof(szTemp), "%d %d ", allConfig.vOpenArry[k].startTime,allConfig.vOpenArry[k].duration);
            strcat(szFile,szTemp);  
        }
        LOG_INFO <<"Open:"<<szFile;
    
    }
    LOG_INFO << "------WeidghtItem end------ ";
    
    return 1;
}

//随机区域
int LuckyGame::RandomArea(int typeindex,int32_t nArray[])
{
    int nIndex = 0;
    int nSum = m_nSum[typeindex];
   
    AssertReturn(nSum > 0 , return -1);
   
    int nRandNum = CRandom::Instance().Random_Int(0, nSum - 1);
    for (size_t i = 0; i < AWARD_COUNT; i++)
    {
        nRandNum -= nArray[i];
        if (nRandNum < 0)
        {
            nIndex = i;
            break;
        }
    }

    return nIndex;
}

// 检查时间
bool LuckyGame::CheckIsOpen(int32_t currency)
{
    time_t t = time(nullptr);  // UTC秒数
	tm *tp = localtime(&t);  

    // 北京时间
    printf("%d/%d/%d\n", tp->tm_mon + 1, tp->tm_mday, tp->tm_year + 1900);
    printf("%d:%d:%d\n", tp->tm_hour, tp->tm_min, tp->tm_sec); //15:50:7

    for (size_t n = 0; n < m_openItemCount; n++)
    {
        if(m_ConfigMap[currency].vOpenArry[n].startTime + m_ConfigMap[currency].vOpenArry[n].duration <= 24 ){
            //顺时间
            if( tp->tm_hour >= m_ConfigMap[currency].vOpenArry[n].startTime){ 
                LOG_WARN << "--检查时间- " << tp->tm_hour << " "<< m_ConfigMap[currency].vOpenArry[n].startTime <<" "<<  m_ConfigMap[currency].vOpenArry[n].duration; 
                if( tp->tm_hour <= m_ConfigMap[currency].vOpenArry[n].startTime + m_ConfigMap[currency].vOpenArry[n].duration ){
                    if( tp->tm_min <= 59 && tp->tm_sec <= 59){
                        return true; 
                    }
                }
            }
        }
        else
        {
            int end = ( m_ConfigMap[currency].vOpenArry[n].startTime + m_ConfigMap[currency].vOpenArry[n].duration ) % 24;
            LOG_WARN << "--检查时间 2- " << tp->tm_hour << " "<< m_ConfigMap[currency].vOpenArry[n].startTime <<" "<<  m_ConfigMap[currency].vOpenArry[n].duration << " " << end;
             //顺时间
            if( tp->tm_hour >= m_ConfigMap[currency].vOpenArry[n].startTime){
                 if( tp->tm_hour < 24 ){
                    if( tp->tm_min <= 59 && tp->tm_sec <= 59){
                        return true; 
                    }
                }
            }
            else if( tp->tm_hour < end ){
                return true; 
            }
        }  
    }

    return false;
}