
#ifndef GAME_SERVICE_MANAGER_H_
#define GAME_SERVICE_MANAGER_H_
#pragma once

#ifdef _WIN32
#else
#include <types.h>
#include <uuid/uuid.h>
#include <dlfcn.h>

typedef uuid_t* REFGUID;
typedef double   SCORE;

#define LEN_KIND					32									//类型长度
#define LEN_SERVER					32									//房间长度
#define LEN_PROCESS					32									//进程长度

//服务属性
struct tagGameServiceAttrib
{
	//内核属性
	WORD							wKindID;							//名称号码
	WORD							wChairCount;						//椅子数目
	WORD							wSupporType;						//支持类型
	TCHAR							szGameName[LEN_KIND];				//游戏名字

	//功能标志
	BYTE							cbAndroidUser;						//机器标志
	BYTE							cbDynamicJoin;						//动态加入
	BYTE							cbOffLineTrustee;					//断线代打

	//服务属性
	DWORD							dwServerVersion;					//游戏版本
	DWORD							dwClientVersion;					//游戏版本
	TCHAR							szDataBaseName[32];					//游戏库名
	TCHAR							szServerDLLName[LEN_PROCESS];		//进程名字
	TCHAR							szClientEXEName[LEN_PROCESS];		//进程名字
};


//服务配置
struct tagGameServiceOption
{
	//挂接属性
	WORD							wKindID;							//挂接类型
	WORD							wNodeID;							//挂接节点
	WORD							wSortID;							//排列标识
	WORD							wServerID;							//房间标识

	//税收配置
	LONG							lCellScore;							//单位积分
	WORD							wRevenueRatio;						//税收比例
	SCORE							lServiceScore;						//服务费用

	//房间配置
	SCORE							lRestrictScore;						//限制积分
	SCORE							lMinTableScore;						//最低积分
	SCORE							lMinEnterScore;						//最低积分
	SCORE							lMaxEnterScore;						//最高积分

	//会员限制
	BYTE							cbMinEnterMember;					//最低会员
	BYTE							cbMaxEnterMember;					//最高会员

	//房间配置
	DWORD							dwServerRule;						//房间规则
	DWORD							dwAttachUserRight;					//附加权限

	//房间属性
	WORD							wMaxPlayer;							//最大数目
	WORD							wTableCount;						//桌子数目
	WORD							wServerPort;						//服务端口
	WORD							wServerType;						//房间类型
	TCHAR							szServerName[LEN_SERVER];			//房间名称

	//分组设置
	BYTE							cbDistributeRule;					//分组规则
	WORD							wMinDistributeUser;					//最少人数
	WORD							wMaxDistributeUser;					//最多人数
	WORD							wDistributeTimeSpace;				//分组间隔
	WORD							wDistributeDrawCount;				//分组局数
	WORD							wDistributeStartDelay;				//开始延时

	//连接设置
	TCHAR							szDataBaseAddr[16];					//连接地址
	TCHAR							szDataBaseName[32];					//数据库名

	//自定规则
	BYTE							cbCustomRule[1024];					//自定规则
};


class IGameServiceManager
{

};


#endif//_WIN32


class GameServiceManager : public IGameServiceManager {
 public:
  GameServiceManager();
  virtual ~GameServiceManager();

  virtual void Release() {}
  virtual void* QueryInterface(REFGUID guid, DWORD query_ver);

  virtual void* CreateTableFrameSink(REFGUID guid, DWORD query_ver);
#ifdef PLATFORM_SICHUAN
  virtual void* CreateTableFrameSink(REFGUID guid, DWORD query_ver, DWORD dwServerID);
#endif
  virtual void* CreateAndroidUserItemSink(REFGUID guid, DWORD query_ver);
  virtual void* CreateGameDataBaseEngineSink(REFGUID guid, DWORD query_ver);

  virtual bool GetServiceAttrib(tagGameServiceAttrib& game_service_attrib);
  virtual bool RectifyParameter(tagGameServiceOption& game_service_option);

 private:
  tagGameServiceAttrib game_service_attrib_;
//   CGameServiceManagerHelper game_service_manager_helper_;
};

#endif // GAME_SERVICE_MANAGER_H_
