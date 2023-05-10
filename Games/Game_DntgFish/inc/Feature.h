#define MULTIPLE_LANGUAGE

// 服务定义.
#if defined __DNTG__            // 大闹天宫.
//#define __GAMEPLAY_PAIPAILE__ // 拍拍乐玩法

#define FISH_VERSION_CLIENT     PROCESS_VERSION(6,0,3)
#define KIND_ID                 530 // 模块标识.
#define CLIENT_EXE_NAME         "dntgfish.exe"
#define SERVER_DLL_NAME         "dntgfish_server.dll"
#define GAME_NAME               "大闹天宫"
#define VENDOR_ID               1087
#define PRODUCT_ID              4
#define GAME_VERSION            16061022

#elif defined __HYZX__          // 海洋之星.

#define FISH_VERSION_CLIENT     PROCESS_VERSION(6,0,3)
#define KIND_ID                 170 // 模块标识.
#define CLIENT_EXE_NAME         "xlfish.exe"
#define SERVER_DLL_NAME         "xlfish_server.dll"
#define GAME_NAME               "海洋之星"
#define VENDOR_ID               1087
#define PRODUCT_ID              5
#define GAME_VERSION            16073000

#elif defined __DFW__           // 大富翁

#define FISH_VERSION_CLIENT     PROCESS_VERSION(6,0,3)
#define KIND_ID                 188 // 模块标识.
#define CLIENT_EXE_NAME         "dfwfish.exe"
#define SERVER_DLL_NAME         "dfwfish_server.dll"
#define GAME_NAME               "霹雳捕鱼"
#define VENDOR_ID               1087
#define PRODUCT_ID              5
#define GAME_VERSION            17032500

#elif defined __HJSDS__         // 黄金圣斗士.

//#define ENABLE_SHADOW
//#define __GAMEPLAY_COLLECT__  // 收集玩法   // modify by James 181027-no collect gameplay.
//#define __GAMEPLAY_MSYQ__     // 马上有钱玩法

#define FISH_VERSION_CLIENT     PROCESS_VERSION(6,0,3)
#define KIND_ID                 168 // 模块标识.
#define CLIENT_EXE_NAME         "fish.exe"
#define SERVER_DLL_NAME         "fish_server.dll"
#define GAME_NAME               "黄金圣斗士"
#define VENDOR_ID               1087
#define PRODUCT_ID              3
#define GAME_VERSION            16101412

#endif




