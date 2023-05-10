#include "MatchRoom.h"
#include "proto/Game.Common.pb.h"
#include "public/GlobalFunc.h"
#include "GameServer/ServerUserManager.h"
#include "MatchRoomManager.h"
#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/ThreadLocalSingletonRedisClient.h"
#include "AndroidUserManager.h"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

#include "MatchServer.h"
extern int g_bisDisEncrypt;

MatchRoom::MatchRoom(uint16_t matchId)
    :m_cur_round_finish_count(0)
{
    match_status_=MATCH_WAIT_START;
    cur_round_count_=0;
    m_is_StartAllcateTimer_=false;
    winlost_Score_=0;
    curStock_=0;
    matchId_ = matchId;
}


bool MatchRoom::CanJoinMatch(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    if(match_status_==MATCH_WAIT_START)
        return true;

    return false;
}
//ip...
bool MatchRoom::CheckJionMatch(shared_ptr<CServerUserItem> &pIServerUserItem)
{

}

bool MatchRoom::CanLeftMatch()
{
    if(match_status_ == MATCH_GAMING)
        return false;

    return true;
}

bool MatchRoom::OnUserOffline(shared_ptr<CServerUserItem> &pIServerUserItem)
{
	//用户离线，强制退赛 ///
    if (!LeftMatch(pIServerUserItem->GetUserId()))
        return true;

    return true;
}

bool MatchRoom::OnUserOnline(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    SendMatchScene(pIServerUserItem);
}

bool MatchRoom::JoinMatch(shared_ptr<CServerUserItem>& pIServerUserItem,Header *commandHeader)
{
    do
    {
        if(!pIServerUserItem || pIServerUserItem->GetUserId() <= 0)
        {
			LOG_DEBUG << "--- *** 加入比赛失败 !!!"
				<< " userId = " << (!pIServerUserItem ? 65535 : pIServerUserItem->GetUserId());
            break;
        }

        uint32_t maxPlayerNum = p_MatchRoomInfo_->maxPlayerNum;
        uint32_t maxRealPlayerNum = maxPlayerNum-(8); //最少有8个机器人
        if (list_MatchUser_.size() >= maxPlayerNum)
        {
			LOG_DEBUG << "--- *** 加入比赛失败 !!!"
				<< " 当前人数 = " << list_MatchUser_.size()
				<< " 最大人数 = " << maxPlayerNum;
            break;
        }
        //真人玩家加入比赛,延时加入
        if (!pIServerUserItem->IsAndroidUser())
        {
            uint32_t realNum = 0;
            for(auto User : list_MatchUser_)
            {
                if(User->ptrUserItem&&!User->ptrUserItem->IsAndroidUser())
                {
                    realNum++;
                }
            }
            if(realNum>=maxRealPlayerNum)
            {
                break;
            }


            string str = "matchUser_" + to_string(pIServerUserItem->GetUserId()) ;
            int32_t leftTime =REDISCLIENT.TTL(str);
            if(leftTime > 0)
            {
                ::MatchServer::MSG_S2C_UserEnterMatchResp response;
                Game::Common::Header* header = response.mutable_header();
                header->set_sign(HEADER_SIGN);
                response.set_retcode(17);
                response.set_errormsg(to_string(leftTime));

                int16_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
                int16_t subId = ::MatchServer::SUB_S2C_ENTER_MATCH_RESP;

                string content = response.SerializeAsString();
                SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);
				LOG_DEBUG << "--- *** 加入比赛失败 !!!"
					<< " " << str << " leftTime = " << leftTime;
                break;
            }

        }


        shared_ptr<UserInfo> userInfo(new UserInfo);
        userInfo->curGameCount=0;
        userInfo->ptrUserItem=pIServerUserItem;
        userInfo->realScore=pIServerUserItem->GetUserScore();
        userInfo->ptrUserItem->SetUserScore(p_MatchRoomInfo_->initCoins);

        list_MatchUser_.push_back(userInfo);
        
        pIServerUserItem->SetMatchId(matchId_);
        pIServerUserItem->setClientReady(true);
        pIServerUserItem->SetUserStatus(sFree);
		LOG_DEBUG << "--- *** 加入比赛"
			<< (pIServerUserItem->IsAndroidUser() ? " 机器人 " : " 玩家 ")
			<< pIServerUserItem->GetUserId();
        OnUserEnterAction(userInfo,commandHeader);
        if(maxPlayerNum<=list_MatchUser_.size())
        {
            if(GetPlayerCount(true)==0)
            {
                clearAllUser();
            }else
                StartMatch();
        }
        return true;
    }while(0);

    return false;
}

void MatchRoom::TableFinish(uint32_t tableID)
{
    //LOG_ERROR<<"TableFinish-------->"<<tableID;
    for(auto it=list_UsedTable_.begin();it!=list_UsedTable_.end() ; it++)
    {
        if((*it)->GetTableId() == tableID)
        {
          //  LOG_ERROR<<"Clear Succees"<<tableID;
            list_UsedTable_.erase(it);
            break;
        }
    }
//    SortUser(); //
//    updateRankInfo();

//    if(!is_StartAllcateTimer_)
//    {

//        is_StartAllcateTimer_ =true;
//    }
}
//玩家离开桌子
void MatchRoom::UserOutTable(uint32_t userId)
{
    //比赛无法为每个游戏控制节奏，玩家退出桌子时间由子游戏自己确定，一般都需要延时踢玩家出桌子
   // LOG_ERROR<<"UserOutTable--------->"<<userId;
    for(auto User : list_MatchUser_)
    {
        if(User->ptrUserItem->GetUserId()==userId && ! User->waitStart)
        {
            User->ptrUserItem->SetUserStatus(sFree);
            User->curGameCount++;
            User->waitStart = true;
            //LOG_ERROR<<"curGameCount------->"<<User->curGameCount;
            if(User->curGameCount < p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]) //还没到达到该轮的固定场次 等待开始下局
            {
                vec_WaitUser_.push_back(User);
            }
            else
            {
                m_cur_round_finish_count++;//完成游戏人数
            }

        }
    }
}
/*
    采用粗略的控制办法，在比赛里面控制桌子库存的状态
    比如：比赛场次输钱时，一定概率控制玩家在库存线下，影响玩家胜率
    1控制玩家赢  0随意 -1控制玩家输
*/
int8_t MatchRoom::GetStorageStatus()
{
    //策划那边也没有很具体的方案。我也不知道具体怎么弄先这样搞着
    int8_t status = 0;
    //低于库存线
    if(curStock_ < lowestStock_)
    {
        if(rand()%100 < 30)
            status = -1;

    }else if(curStock_ > highStock_)
    {
        if(rand()%100 < 30)
            status = 1;

    }
    return status;
}


void MatchRoom::StartMatch()
{
    match_status_=MATCH_GAMING;
    for(auto pUser :list_MatchUser_)
    {
        if(!pUser->ptrUserItem->IsAndroidUser())
        {
            winlost_Score_ +=p_MatchRoomInfo_->joinNeedSore;
        }
    }
    //
    //LOG_INFO<<"收取报名费 :"<<winlost_Score_;
    initStorageScore();
    vec_WaitUser_.assign(list_MatchUser_.begin(),list_MatchUser_.end());
    m_cur_round_finish_count = 0;
    AllocateRoom();
    m_allocateTimer=ptr_LoopThread_->getLoop()->runEvery(3,bind(&MatchRoom::AllocateUser,this));
}


void MatchRoom::AllocateRoom()
{
    shared_ptr<UserInfo> pUser;
    shared_ptr<CTableFrame> pTableFrame;

    while(!vec_WaitUser_.empty())
    {
        pUser=vec_WaitUser_.back();
        pTableFrame=getSuitTable(pUser->ptrUserItem);

        //原设计分数太低下局不好压分的时候，给玩家一个最低分。
        //策划说他会将初始分弄到很大，不会出现负分。我觉得这个设计会有问题
        //但是没办法，先留着
        if(p_MatchRoomInfo_->atLeastNeedScore && pUser->ptrUserItem->GetUserScore() < p_MatchRoomInfo_->atLeastNeedScore)
        {
            //LOG_ERROR<<"LOW SCORE !!!!!!!! :"<<pUser->ptrUserItem->GetUserScore();
            pUser->ptrUserItem->SetUserScore(p_MatchRoomInfo_->atLeastNeedScore);
        }

        if(!pTableFrame) break;
        if((pTableFrame)->RoomSitChair(pUser->ptrUserItem, NULL, false))
        {
            //LOG_WARN<<"RoomSitChair Success ";
        }else
        {
            //LOG_WARN<<"RoomSitChair Failure  !!!!!!!!!!!!!!! ";
            break;
        }
        pUser->waitStart = false;
        vec_WaitUser_.pop_back();
    }
}

void MatchRoom::AllocateUser()
{
    assert(match_status_==MATCH_GAMING);
    assert(m_cur_round_finish_count <=list_MatchUser_.size());
    //cout<<"AllocateUser"<<endl;
    m_is_StartAllcateTimer_=false;

    if(list_UsedTable_.size()==0 && m_cur_round_finish_count == list_MatchUser_.size()) //等待列表没人了，说明全部都完成了目标局数
    {
        //LOG_ERROR<<"等待列表没人了,并且桌子为空";

        SortUser();
        updateRankInfo();
        //淘汰人数并进入下个轮回
        uint32_t passCount=p_MatchRoomInfo_->upGradeUserNum[cur_round_count_];//进入下局人数
        uint32_t rank=list_MatchUser_.size();
        uint32_t outCount=rank - passCount; //淘汰人数

        Landy::CMatchServer* server = dynamic_cast<Landy::CMatchServer *> (p_ILogicThread_) ;
        for(uint16_t i=outCount; i != 0 ;i--)
        {
            rank--;
            shared_ptr<UserInfo> pUser = list_MatchUser_.back();
            int64_t award = 0;
            if(p_MatchRoomInfo_->awards.size()>rank)
                award=p_MatchRoomInfo_->awards[rank];

            //跑马灯消息
            if (rank == 0) {
                SendGameMessage(pUser->ptrUserItem, SMT_GLOBAL | SMT_SCROLL, award, rank + 1);
            }

            if(!pUser->ptrUserItem->IsAndroidUser())
                OnUserFinish(pUser,award,rank+1);

            list_MatchUser_.pop_back();

            if(server)
                server->DelUserFromServer(pUser->ptrUserItem);
        }

        if(passCount == 0) //比赛结束
        {
            ptr_LoopThread_->getLoop()->cancel(m_allocateTimer);
            updateStorageScore(winlost_Score_);
            match_status_=MATCH_END;
            m_is_StartAllcateTimer_=false;
			//生成场次编号
            resetMatch();
            MatchRoomManager::get_mutable_instance().FreeNormalMatch(matchId_);
           // LOG_ERROR<<"Match Over!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
            return;
        }
        cur_round_count_++;

        rank=0;
        for(auto User : list_MatchUser_)
        {
            User->curGameCount=0;  //新的一轮，玩过的局数需重置
            //发送晋级
            rank++;
            OnUserUpgrade(User->ptrUserItem,rank);
        }
        m_cur_round_finish_count = 0;
        vec_WaitUser_.assign(list_MatchUser_.begin(),list_MatchUser_.end());
        //晋级下个定时器开始
        return;
    }

    if(vec_WaitUser_.size() == 0)
        return;


    //固定分配 比赛机制可能不一致，暂不考虑
    if (p_MatchRoomInfo_->fixedAllocate)
    {
        //玩家退出桌子才更新局数，分数在结束时就算好了，在这里更新两个都是最新的
        SortUser();
        updateRankInfo();
        //vec_WaitUser_ ;
        AllocateRoom();

    }//随机匹配需要等人
    else if(list_UsedTable_.size()==0 &&list_MatchUser_.size() ==vec_WaitUser_.size()) //所有桌子完成
    {
        //玩家退出桌子才更新局数，分数在结束时就算好了，在这里更新两个都是最新的
        SortUser();
        updateRankInfo();
        random_shuffle(vec_WaitUser_.begin(),vec_WaitUser_.end());
        AllocateRoom();
    }

}

