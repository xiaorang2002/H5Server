
#include "StdAfx.h"
#include "UserPrefs.h"
#include "../inc/aics/IAicsEngine.h"
#include <glog/logging.h>

#ifdef _WIN32
#include <atlstr.h>
#include "../AICSNewUserCheck/AICSNewUserCheck_i.c"
#endif//_WIN32

UserPrefs::UserPrefs(WORD nConfigId)
{
#ifdef _WIN32
    spNewuid = NULL;
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_CAICSNewUserCheck,NULL,CLSCTX_SERVER,IID_ICAICSNewUserCheck,(LPVOID*)&spNewuid);
		if (!SUCCEEDED(hr))
		{
			spNewuid = NULL;
		}
	}
#else
    // try to new the special content now.
    spNewuid = new CCAICSNewUserCheck();
#endif//_WIN32

    // initialize the database content.
    // m_playerLogMDB.init(nConfigId);
#ifdef __MULTI_THREAD__
    CCriticalSectionEx::Create(&m_cs);
#endif//

}

UserPrefs::~UserPrefs()
{
#ifdef  _WIN32
	if (spNewuid)
	{
		spNewuid->Release();
		spNewuid = NULL;
	}
	CoUninitialize();
#else
    if (spNewuid) {
        delete spNewuid;
        spNewuid=NULL;
    }
#endif//_WIN32

#ifdef __MULTI_THREAD__
    CCriticalSectionEx::Destroy(&m_cs);
#endif//
}

void UserPrefs::onUserSitdown(shared_ptr<CServerUserItem> serverUserItem, WORD nConfigId, DWORD ticketRatio,
                              double exchangeRatio, DWORD maxGain,
                              DWORD maxWinCoin, DWORD beginUserID)
{
    char info[1024]={0};
    DWORD userID = serverUserItem->GetUserId();
    tagUserPrefs* sp = getUserPrefs(userID, nConfigId);
    if (!sp) {
         sp = allocUserPrefs();
	}

    // check content.
    if (sp != NULL)
    {
        // initialize value.
        sp->dRecharge = 0.0;
        sp->dWinCoin  = 0.0;
        sp->sumGain   = 0.0;
        sp->sumLost   = 0.0;
        sp->beginTime = getCurrentTime();
        sp->userID    = userID;
        sp->tableID   = serverUserItem->GetTableId();
        sp->chairID   = serverUserItem->GetChairId();
        sp->configID  = nConfigId;
        sp->ticketRatio = ticketRatio;
        sp->exchangeRatio = exchangeRatio;
        sp->nickName  = serverUserItem->GetNickName();
        sp->srcGold   = serverUserItem->GetUserScore() * exchangeRatio;
        //sp->dWinCoin  = serverUserItem->GetAllWinScore();------
        //sp->dRecharge = serverUserItem->GetAllChargeAmount();------
        // m_playerLogMDB.onUserSitdown(userID, nConfigId, sp->dWinCoin, sp->dRecharge);

#ifdef _WIN32
		if (spNewuid)
		{
            spNewuid->OnUserSitdown(userID, nConfigId, sp->dWinCoin, sp->dRecharge);
		}
		// 		if (userID < beginUserID) {
		// 			TRACE_LOG(_T("玩家昵称=%s,玩家ID=%d,房间号=%d)玩家坐下!!! 用户ID小于等于XML定义的ID(%d), 直接视为老用户。"),
        // 				serverUserItem->GetNickName(), userID, sp->nConfigId, beginUserID);
        // 			sp->isOldUser = TRUE;
		// 		}
#else
        // sitdown now.
        if (spNewuid) {
            spNewuid->OnUserSitdown(userID, nConfigId, sp->dWinCoin, sp->dRecharge);
        }
#endif//_WIN32
        addUserPrefs(sp);
        //snprintf(info,sizeof(info), "(玩家昵称=%s,玩家ID=%d,房间号=%d,充值数=%.02f,赢利数=%.02f)玩家坐下!!!",
        //    serverUserItem->GetNickName(), userID, sp->configID, sp->dRecharge, sp->dWinCoin);
        //LOG(WARNING) << info;
	}
	else
	{
        //snprintf(info,sizeof(info), "(玩家昵称=%s,玩家ID=%d,房间号=%d)玩家坐下!!!(数据错误)",
        //    serverUserItem->GetNickName(), userID, nConfigId);
        //LOG(INFO) << info;
	}
}

void UserPrefs::onUserStandup(shared_ptr<CServerUserItem> serverUserItem, WORD nConfigId)
{
    char info[1024]={0};
    DWORD userID = serverUserItem->GetUserId();
    tagUserPrefs* p = getUserPrefs(userID, nConfigId);
    if (p != NULL)
	{
        /* modify by James 180911-no use record_player,record will insert to record_user_inout.
        char sql[1024]={0};
        tstring endTime = getCurrentTime();
        snprintf(sql,sizeof(sql),
                 "INSERT INTO record_player(user_id, room_id, gain_score, lost_score,  begin_time, end_time, source_gold, target_gold) VALUES(%d, %d, %lf, %lf, '%s', '%s', %lf, %lf)",
                 userID, nConfigId, p->sumGain, p->sumLost, p->beginTime.c_str(), endTime.c_str(), p->srcGold, serverUserItem->GetUserScore() * p->exchangeRatio);
        m_playerLogMDB.addSqlCommand(sql);
        */

        // check content.
        if (spNewuid) {
            spNewuid->OnUserStandup(userID, nConfigId);
        }

        // LOG(INFO) << "UserPrefs::OnUserStandup, sql:" << sql;

        //snprintf(info,sizeof(info),"(玩家昵称=%s,玩家ID=%d,房间号=%d)玩家起立!!!",
        //         serverUserItem->GetNickName(), userID, nConfigId);
        //LOG(WARNING) << info;
        // delete user now.
		removeUserPrefs(p);
	}
	else
	{
        //snprintf(info,sizeof(info),"(玩家昵称=%s,玩家ID=%d,房间号=%d)玩家起立!!!(数据错误)",
        //         serverUserItem->GetNickName(), userID, nConfigId);
        //LOG(INFO) << info;
	}
}

void UserPrefs::onUserFired(DWORD userID, WORD nConfigId, int fireScore)
{
    updateGainLost(userID, nConfigId, fireScore, 0);
}
void UserPrefs::onFishCatched(DWORD userID, WORD nConfigId, LONG score)
{
    updateGainLost(userID, nConfigId, 0, score);
}

void UserPrefs::updateGainLost(DWORD userID, WORD nConfigId, int fireScore, DWORD score)
{
    tagUserPrefs* p = getUserPrefs(userID, nConfigId);
    if (p != NULL)
	{
		double gain = (double)fireScore / p->ticketRatio;
        double lost = (double)score     / p->ticketRatio;
		p->sumGain += gain;
		p->sumLost += lost;

#ifdef _WIN32
		// 		if (!p->isOldUser) {
		if (spNewuid)
		{
            p->isOldUser = spNewuid->OnUpdateGainLost(userID, nConfigId, gain * p->exchangeRatio, lost * p->exchangeRatio);
		}
		// 			if (p->isOldUser) {
		// 				TRACE_LOG(_T("(玩家昵称=%s,玩家ID=%d,是否老玩家=%d,房间号=%d) 玩家升级!!!"),
        // 					p->nickName.c_str(), userID, p->isOldUser, nConfigId);
		// 			}
		// 		} else {
        // 			spNewuid->OnUpdateGainLost(userID, nConfigId, gain, lost);
		// 		}
#else
        // check value.
        if (spNewuid){
            p->isOldUser = spNewuid->OnUpdateGainLost(userID, nConfigId, gain, lost);
        }
#endif//_WIN32
	}
	else
	{
        TRACE_LOG(_T("(玩家ID=%d,房间号=%d) 洗码量更新!!!(数据错误)"), userID, nConfigId);
	}
}

tagUserPrefs* UserPrefs::allocUserPrefs()
{
#ifdef __MULTI_THREAD__
    // initialize the locker.
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__
	return m_storageUserPrefs.alloc();
}
void UserPrefs::freeUserPrefs(tagUserPrefs* userPrefs)
{
	m_storageUserPrefs.free(userPrefs);
}

int64_t UserPrefs::getKeyOfUserPrefs(DWORD userID, WORD nConfigId)
{
    int64_t key = nConfigId;
	key <<= 32;
	key |= userID;
	return key;
}

void UserPrefs::addUserPrefs(tagUserPrefs* userPrefs)
{
#ifdef __MULTI_THREAD__
    // initialize the locker.
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__

    int64_t key = getKeyOfUserPrefs(userPrefs->userID, userPrefs->configID);
	m_allUserPrefs[key] = userPrefs;
}

tagUserPrefs* UserPrefs::getUserPrefs(DWORD userID, WORD nConfigId)
{
#ifdef __MULTI_THREAD__
    // initialize the locker.
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__

    int64_t key = getKeyOfUserPrefs(userID, nConfigId);
	UserPrefsMap::iterator iter = m_allUserPrefs.find(key);
	if (iter != m_allUserPrefs.end()) {
		return iter->second;
	}
//Cleanup:
	return NULL;
}

void UserPrefs::removeUserPrefs(tagUserPrefs* userPrefs)
{
    int64_t key = getKeyOfUserPrefs(userPrefs->userID, userPrefs->configID);

#ifdef __MULTI_THREAD__
    // initialize the locker.
    CCriticalSectionEx lock;
    lock.Lock(&m_cs);
#endif//__MULTI_THREAD__

	m_allUserPrefs.erase(key);
	freeUserPrefs(userPrefs);
}

tstring UserPrefs::getCurrentTime()
{
    TCHAR szResult[100]=TEXT("");
#ifdef _WIN32
    SYSTEMTIME st = {0};

	GetLocalTime(&st);
	// build the full time string value time string value content item value.
	wsprintf(szResult,_T("%d-%d-%d %d:%d:%d.%d"),
		st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);

#else
    time_t now;
    struct tm *st = 0;
    time(&now);
    st = localtime(&now);
    // build the full time string value time string value content.
    snprintf(szResult,sizeof(szResult),_T("%d-%d-%d %d:%d:%d"),
            st->tm_year+1900,st->tm_mon+1,st->tm_mday,
             st->tm_hour,st->tm_min,st->tm_sec);
#endif//_WIN32
//Cleanup:
	return szResult;
}
