#include "DefGameAndroidExport.h"
#include "AndroidUserItemSink.h"


//得到桌子实例
extern "C" IAndroidUserItemSink* CreateAndroidSink()
{
	IAndroidUserItemSink* pSink = new CAndroidUserItemSink();
	return pSink;
}

//删除桌子实例
extern "C" void DeleteAndroidSink(IAndroidUserItemSink* pIAndroidUserItemSink)
{
	if (nullptr != pIAndroidUserItemSink)
	{
		delete pIAndroidUserItemSink;
	}
	pIAndroidUserItemSink = nullptr;
}
