/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/


// 仅用于上下分服拉取天下汇分数


/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
 
#include "FunClass.h"
 
 
// 成员函数定义，包括构造函数
CFunClass::CFunClass(RegisteServer* loginserver)
{
	// threadTimer_ = loop;
    cout << "CFunClass Object is being create" << endl;
}

void CFunClass::startInit()
{
	// 先加载一次配置
	LoadInitConfig();
 
	LOG_WARN << "--- CFunClass start is being load";
}

 