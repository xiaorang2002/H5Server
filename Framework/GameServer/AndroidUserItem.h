#ifndef ANDROIDUSERITEM_H
#define ANDROIDUSERITEM_H

#include "ServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/Globals.h"


class CAndroidUserItem : public CServerUserItem
{
public:
    CAndroidUserItem();
    virtual ~CAndroidUserItem();

public:
    virtual void ResetUserItem();

public:
    virtual bool IsAndroidUser();
    virtual shared_ptr<IAndroidUserItemSink> GetAndroidUserItemSink();

public:
    virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, const uint8_t* data, uint32_t size);
    virtual bool SendSocketData(uint8_t subId, const uint8_t *data, uint32_t len);

public:
    virtual void setUserReady();

public:
    void SetAndroidUserItemSink(shared_ptr<IAndroidUserItemSink> pSink);


protected:
    shared_ptr<IAndroidUserItemSink>   m_pAndroidUserSink;

private:
    bool                               m_isAndroidUser;


};

#endif // ANDROIDUSERITEM_H
