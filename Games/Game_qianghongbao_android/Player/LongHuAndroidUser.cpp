#include "LongHuAndroidUser.h"
#include "Longhu.Message.pb.h"
#include "Utility.h"
CLongHuAndroidUser::CLongHuAndroidUser()
{

}
void CLongHuAndroidUser::InitPlayerMgr(ITableFrame *pTableFram)
{

}

void CLongHuAndroidUser::LoadJetAreaMultiplierCfg()
{
    this->m_JetAreaMulitiplter.clear();
    int mu = 0;
    for(int area = Longhu::JET_AREA::AREA_BEGIN+1;area < Longhu::JET_AREA::AREA_MAX;area++)
    {
        int mulitiplter =   GET_APP_CONFIG_INSTNACE(this->m_ConfigPath)->GetConfigInt("ANDROID_CONFIG_RADOM_WIDGET_CONFIG",this->getAreaElemTypeName(area),33);
        mu = mu + mulitiplter;
        this->m_JetAreaMulitiplter.insert(std::make_pair(area,mu));
    }
}
int CLongHuAndroidUser::GetDiscardArea()
{
    return Longhu::JET_AREA::AREA_HEE;
}

string CLongHuAndroidUser::getAreaElemTypeName(int type)
{
    switch(type)
    {
        case Longhu::JET_AREA::AREA_BEGIN      : return "AREA_HEE";break;
        case Longhu::JET_AREA::AREA_HEE        : return "AREA_HEE";break;
        case Longhu::JET_AREA::AREA_LONG        : return "AREA_LONG";break;
        case Longhu::JET_AREA::AREA_HU       :  return "AREA_HU";break;
        case Longhu::JET_AREA::AREA_MAX        :  return "AREA_HU";break;
        default: return "error"; break;
    }
}

extern "C" IAndroidUserItemSink *CreateAndroidSink(void)
{
    Utility::InitGlobalEvent();
    CLongHuAndroidUser *pAndroidUser = new CLongHuAndroidUser();
    pAndroidUser->setGameConfigPath("conf/longhu_config.ini");
    return dynamic_cast<IAndroidUserItemSink*>(pAndroidUser);
}
extern "C" void DeleteAndroidSink(IAndroidUserItemSink* p)
{
    ((Utility::CBaseAndroidUserItem*)p)->RepositionSink();
    delete p;
}
