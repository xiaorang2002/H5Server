
#include "StdAfx.h"
#include "game_service_manager.h"
#include "table_frame_sink.h"

#ifdef _WIN32
#else

//游戏模式
#define GAME_GENRE_GOLD				0x0001								//金币类型
#define GAME_GENRE_SCORE			0x0002								//点值类型
#define GAME_GENRE_MATCH			0x0004								//比赛类型
#define GAME_GENRE_EDUCATE			0x0008								//私人场类型

#define szTreasureDB				"THTreasureDB"

#endif//_WIN32

GameServiceManager::GameServiceManager() 
{
  game_service_attrib_.wKindID = KIND_ID;
  game_service_attrib_.wChairCount = 4;
  game_service_attrib_.wSupporType = GAME_GENRE_GOLD | GAME_GENRE_EDUCATE | GAME_GENRE_MATCH | GAME_GENRE_SCORE;
  game_service_attrib_.cbDynamicJoin = TRUE;
  game_service_attrib_.cbAndroidUser = FALSE;
  game_service_attrib_.cbOffLineTrustee = FALSE;
  game_service_attrib_.dwServerVersion = FISH_VERSION_SERVER;
  game_service_attrib_.dwClientVersion = FISH_VERSION_CLIENT;
  lstrcpyn(game_service_attrib_.szGameName, _T(GAME_NAME), CountArray(game_service_attrib_.szGameName));
  lstrcpyn(game_service_attrib_.szDataBaseName, szTreasureDB, CountArray(game_service_attrib_.szDataBaseName));
  lstrcpyn(game_service_attrib_.szClientEXEName, TEXT(CLIENT_EXE_NAME), CountArray(game_service_attrib_.szClientEXEName));
  lstrcpyn(game_service_attrib_.szServerDLLName, TEXT(SERVER_DLL_NAME), CountArray(game_service_attrib_.szServerDLLName));
}

GameServiceManager::~GameServiceManager() {
	
}

void* GameServiceManager::QueryInterface(REFGUID guid, DWORD query_ver) 
{
#ifdef _WIN32
  QUERYINTERFACE(IGameServiceManager, guid, query_ver);
  QUERYINTERFACE_IUNKNOWNEX(IGameServiceManager, guid, query_ver);
#endif//_WIN32
  return NULL;
}

void* GameServiceManager::CreateTableFrameSink(REFGUID guid, DWORD query_ver) {
  TableFrameSink* table_frame_sink = new TableFrameSink();
  void* ret = table_frame_sink->QueryInterface(guid, query_ver);
  if (ret == NULL) SafeDelete(table_frame_sink);
  return ret;
}

#ifdef PLATFORM_SICHUAN
void* GameServiceManager::CreateTableFrameSink(REFGUID guid, DWORD query_ver, DWORD dwServerID) {
	return CreateTableFrameSink(guid, query_ver);
}
#endif

void* GameServiceManager::CreateAndroidUserItemSink(REFGUID guid, DWORD query_ver) {
	/*if (game_service_manager_helper_.GetInterface() == NULL) {
		game_service_manager_helper_.SetModuleCreateInfo(TEXT("fish_android.dll"), "CreateGameServiceManager");
		if (!game_service_manager_helper_.CreateInstance()) return NULL;
	}
	return game_service_manager_helper_->CreateAndroidUserItemSink(guid, query_ver);*/
	return NULL;
}

void* GameServiceManager::CreateGameDataBaseEngineSink(REFGUID guid, DWORD query_ver) {
  return NULL;
}

bool GameServiceManager::GetServiceAttrib(tagGameServiceAttrib& game_service_attrib) {
  game_service_attrib = game_service_attrib_;
  return true;
}

bool GameServiceManager::RectifyParameter(tagGameServiceOption& game_service_option) {
// 	TRACE_INFO(_T("RectifyParameter update room id to:[%d],table count:[%d],chair count:[%d]"),game_service_option.wServerID,game_service_option.wTableCount,game_service_attrib_.wChairCount);
	return true;
}

extern HINSTANCE g_hInst;
extern "C" __declspec(dllexport) void* CreateGameServiceManager(REFGUID guid, DWORD interface_ver)
{
  static GameServiceManager game_service_manager;
  return game_service_manager.QueryInterface(guid, interface_ver);
}
