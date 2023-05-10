#include "algorithmc.h"
bool Algorithmc::isopen=false;
Algorithmc::Algorithmc()
{
    Algorithm=GetAicsErbaEngine();
}
void Algorithmc::InitAlgor(shared_ptr<ITableFrame>  mPItableFrame)
{

    printf("==============================================================");

    if(!isopen)
    {
        tagErbaGs tgo;
        tgo.nMaxPlayer=mPItableFrame->GetMaxPlayerCount();
        //tgo.nMaxTable=mPItableFrame->GetGameKindInfo()->nTableCount;
         tgo.nMaxTable=mPItableFrame->GetGameRoomInfo()->tableCount;
        tgo.nToubbili=1;
        Algorithm->Init(&tgo);
        date=-1;
        //date=(int32)Algorithm->SetRoomID(mPItableFrame->GetGameRoomInfo()->roomId);
        printf("SetRoomID(%d),status:%d ",mPItableFrame->GetGameRoomInfo()->roomId,date );
        isopen=true;
    }

}
bool Algorithmc::GetResult(tagErbaGateInfo *tag)
{
    printf("Algorithm:[%x]\n",Algorithm);
    printf("tagerbagangGateInfo:[%x]\n",tag);
   int status = Algorithm->erbaOnebk(1,0,0,tag);
   if(status<0)
   {
       return false ;
   }
   return true ;
}
