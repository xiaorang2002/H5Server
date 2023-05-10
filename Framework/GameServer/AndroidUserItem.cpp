#include "AndroidUserItem.h"

#include <sys/time.h>
#include <stdarg.h>

#include "AndroidUserManager.h"
#include "GameTableManager.h"
#include "../../proto/Game.Common.pb.h"


CAndroidUserItem::CAndroidUserItem()
{
    m_isAndroidUser = true;
}

CAndroidUserItem::~CAndroidUserItem()
{

}

void CAndroidUserItem::ResetUserItem()
{
    CServerUserItem::ResetUserItem();
}

bool CAndroidUserItem::IsAndroidUser()
{
    return m_isAndroidUser;
}

shared_ptr<IAndroidUserItemSink> CAndroidUserItem::GetAndroidUserItemSink()
{
    return m_pAndroidUserSink;
}

bool CAndroidUserItem::SendUserMessage(uint8_t mainId, uint8_t subId, const uint8_t* data, uint32_t size)
{
    if((mainId == 0) || (mainId == Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC))
    {
        if(m_pAndroidUserSink)
        {
            m_pAndroidUserSink->OnGameMessage(subId, data, size);
        }
    }else
    {
//         LOG_WARN << "Unknown command arrived for android send user message mainId:" << mainId << ", subId:" << subId;
    }
    return true;
}

bool CAndroidUserItem::SendSocketData(uint8_t subId, const uint8_t *data, uint32_t len)
{
    bool bSuccess = false;
    if(INVALID_TABLE != m_tableId)
    {
        shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(m_tableId);
        if(pITableFrame)
        {
            bSuccess = pITableFrame->OnGameEvent(m_chairId, subId, data, len);
        }else
        {
            LOG_ERROR << "android user item get table frame failed, table Id:" << m_tableId;
        }
    }else
    {
        LOG_ERROR << "android user item SendSocketData, table Id invalidate.";
    }
    return (bSuccess);
}

// set the special user ready status.
void CAndroidUserItem::setUserReady()
{
    if(INVALID_TABLE != m_tableId)
    {
        shared_ptr<ITableFrame> pITableFrame = CGameTableManager::get_mutable_instance().GetTable(m_tableId);
        if (pITableFrame)
        {
            uint32_t chairId = GetChairId();
            pITableFrame->SetUserReady(chairId);
        }else
        {
            LOG_ERROR << "android user item get table frame failed, table Id:" << m_tableId;
        }
    }else
    {
        LOG_ERROR << "android user item SendSocketData, table id invalidate.";
    }
}

// try to set the special android user item sink content item value data.
void CAndroidUserItem::SetAndroidUserItemSink(shared_ptr<IAndroidUserItemSink> pSink)
{
    m_pAndroidUserSink = pSink;
}

// try to add the special play count.
//void CAndroidUserItem::AddPlayCount()
//{
//    ++m_wPlayCount;
//}

// // try to get the special play count.
//dword CAndroidUserItem::GetPlayCount()
//{
//    return (m_wPlayCount);
//}

