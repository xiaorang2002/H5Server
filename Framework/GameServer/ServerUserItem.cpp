#include "ServerUserItem.h"

#include <muduo/base/Logging.h>
#include "muduo/base/TimeZone.h"

#include "proto/GameServer.Message.pb.h"

#include "public/gameDefine.h"
#include "public/EntryPtr.h"
#include "crypto/crypto.h"

// constract the special content.
CServerUserItem::CServerUserItem()
{
    m_tableId    = INVALID_TABLE;
    m_chairId    = INVALID_CHAIR;

    m_userStatus = sFree;
    m_trustShip  = false;
    m_bClientReady = false;

    m_bisAndroid = false;
    m_sMthBlockList.resize(20);
    m_rank = 0;

}

void CServerUserItem::ResetUserItem()
{
    //LOG_DEBUG << ">>> ResetUserItem called, tableid:" << m_tableId << ", chairId:" << m_chairId
    //          << ", userId:" << m_userBaseInfo.userId;

    m_tableId    = INVALID_TABLE;
    m_chairId    = INVALID_CHAIR;

    m_userStatus = sFree;
    m_trustShip  = false;
    m_bClientReady = false;

    m_bisAndroid = false;

    m_userBaseInfo = GSUserBaseInfo();
    m_blacklistInfo.status = 0;
    m_rank      = 0;
//    m_userScoreInfo = GSUserScoreInfo();
}

bool CServerUserItem::IsAndroidUser()
{
    return m_bisAndroid;
}

bool CServerUserItem::SendUserMessage(uint8_t mainId, uint8_t subId, const uint8_t* data, uint32_t len)
{
    LOG_ERROR << "SendUserMessage only used for android user item.";
    return true;

    /*
    LOG_INFO << " >>> SendUserMesage mainid:" << mainid << ", subid:" << subid << "isAndroid:" << this->IsAndroidUser();

    bool bSuccess = true;
    TcpConnectionPtr conn(m_pSocket.lock());
    if(likely(conn))
    {
        internal_prev_header *internal_header = m_clientSession.get();
        string aesKey(internal_header->aesKey, sizeof(internal_header->aesKey));

        vector<unsigned char> encryptedData;
        int ret = Landy::Crypto::aesEncrypt(aesKey, (unsigned char*)data, len, encryptedData, internal_header);
        if(likely(ret > 0 && !encryptedData.empty()))
        {
            conn->send(&encryptedData[0], ret);
        }
        else
        {
            LOG_INFO << " >>> SendUserMesage failed! mainid:" << mainid << ", subid:" << subid << "isAndroid:" << this->IsAndroidUser();
            bSuccess = false;
        }
    } while (0);
//Cleanup:
    return (bSuccess);
    */
}

// send socket data use MAINGS_GAME
bool CServerUserItem::SendSocketData(uint8_t subId, const uint8_t* data, uint32_t datasize)
{
    LOG_ERROR << "SendSocketData only used for android user item";
    return true;
}

bool CServerUserItem::isValided()
{
    return m_tableId != INVALID_TABLE && m_chairId != INVALID_CHAIR && GetUserId() > 0;
}
