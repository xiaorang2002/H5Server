#ifndef ALGORITHMC_H
#define ALGORITHMC_H
#include "../../Aics/publics/IAicsErbaGun.h"
#include "../../public/ITableFrame.h"
class Algorithmc
{
public:
    Algorithmc();
    ~Algorithmc(){}
    static Algorithmc* Instence ()
    {
        static Algorithmc obj;
        return &obj;
    }
    static bool isopen;
    void InitAlgor(shared_ptr<ITableFrame>  mPItableFrame);

    bool GetResult(tagErbaGateInfo *tag);
    IAicsErbaEngine* Algorithm;

    int32 date;
};

#endif // ALGORITHMC_H
