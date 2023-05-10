#ifndef LONGHUANDROIDUSER_H
#define LONGHUANDROIDUSER_H
#include "Utility.h"
#include "../../../server/public/ITableFrame.h"
class CLongHuAndroidUser:public Utility::CBaseAndroidUserItem
{
public:
    CLongHuAndroidUser();
    virtual void InitPlayerMgr(ITableFrame* pTableFram);
    virtual std::string getGameName() {return "LongHu";}
    virtual void LoadJetAreaMultiplierCfg();
    virtual int GetDiscardArea();
    virtual string getAreaElemTypeName(int type);
};

#endif // LONGHUANDROIDUSER_H
