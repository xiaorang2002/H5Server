#include "IMatchRoom.h"
#include "proto/Game.Common.pb.h"
#include "public/GlobalFunc.h"
#include "GameServer/ServerUserManager.h"
#include "MatchRoomManager.h"
#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "ThreadLocalSingletonRedisClient.h"
#include "AndroidUserManager.h"
#include "TraceMsg/FormatCmdId.h"
#include "json/json.h"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

#include "MatchServer.h"
extern int g_bisDisEncrypt;

double  IMatchRoom::lowestStock_ = 0;
double  IMatchRoom::highStock_ = 0;

bool IMatchRoom::InitMathRoom(shared_ptr<EventLoopThread> &game_logic_thread, MatchRoomInfo *pMatchRoomInfo, ILogicThread *pLogicThread)
{
    p_MatchRoomInfo_=pMatchRoomInfo;
    ptr_LoopThread_=game_logic_thread;
    p_ILogicThread_=pLogicThread;

	//生成场次编号
    resetMatch();
}

//生成场次编号
bool IMatchRoom::resetMatch()
{
    cur_round_count_=0;
    winlost_Score_=0;
    list_UsedTable_.clear();
    list_MatchUser_.clear();
    vec_WaitUser_.clear();

    strRoundId_.clear();
    strRoundId_ = to_string(p_MatchRoomInfo_->matchRoomId) + "-";

    int64_t seconds_since_epoch = chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    strRoundId_ += to_string(seconds_since_epoch) + "-";
    strRoundId_ += to_string(::getpid()) + "-";
    strRoundId_ += to_string(matchId_)+"-";
    strRoundId_ += to_string(rand()%10);

	LOG_DEBUG << "--- *** ["
		<< strRoundId_ << "] "
		<< p_MatchRoomInfo_->matchRoomId << " { "
		<< p_MatchRoomInfo_->matchName << " }";
}


bool IMatchRoom::OnUserOffline(shared_ptr<CServerUserItem> &pIServerUserItem)
{
	//用户离线，强制退赛 ///
    if (!LeftMatch(pIServerUserItem->GetUserId()))
        return true;

    return true;
}

bool IMatchRoom::OnUserOnline(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    SendMatchScene(pIServerUserItem);
}

bool IMatchRoom::LeftMatch(uint32_t userId)
{
    if(match_status_ == MATCH_GAMING)
        return false;

	//用户退赛，清理信息
     shared_ptr<UserInfo> pUserInfo;
     for(auto it = list_MatchUser_.begin();it!=list_MatchUser_.end();it++)
     {
        if((*it)->ptrUserItem->GetUserId() == userId )
        {
            pUserInfo=*it;
            list_MatchUser_.erase(it);
			LOG_DEBUG << "--- *** 退出比赛"
				<< ((*it)->ptrUserItem->IsAndroidUser() ? " 机器人 " : " 玩家 ")
				<< (*it)->ptrUserItem->GetUserId();
            break;
        }
     }

     if(pUserInfo)
     {
        writeUserScoreToDB(userId,p_MatchRoomInfo_->joinNeedSore);
        AddScoreChangeRecordToDB(pUserInfo->ptrUserItem->GetUserBaseInfo(),pUserInfo->realScore
                                 ,p_MatchRoomInfo_->joinNeedSore,pUserInfo->realScore+p_MatchRoomInfo_->joinNeedSore);
        updateMatchRecord(pUserInfo->ptrUserItem->GetUserBaseInfo(),
                          p_MatchRoomInfo_->joinNeedSore,pUserInfo->realScore,pUserInfo->realScore+p_MatchRoomInfo_->joinNeedSore,-1,pUserInfo->ptrUserItem->IsAndroidUser());
        string str = "matchUser_" + to_string(userId);
        REDISCLIENT.set(str,"1",p_MatchRoomInfo_->leftWaitSeconds);
        Landy::CMatchServer* server = dynamic_cast<Landy::CMatchServer *> (p_ILogicThread_) ;
        if(server)
            server->DelUserFromServer(pUserInfo->ptrUserItem);
		//真人玩家通知计数
		int c = 0;
		//退出比赛，刷新人数
		for (auto user : list_MatchUser_)
		{
			//只广播通知真人玩家
			if (!user->ptrUserItem->IsAndroidUser())//&&user->ptrUserItem->GetUserId()!=pIServerUserItem->GetUserId()
			{
				LOG_DEBUG << "--- *** 退出比赛，刷新人数：" << GetAllPlyerCount() << " 广播次数：" << ++c;
				updateMatchPlayer(user->ptrUserItem);
			}
		}
     }

    return true;
}

uint16_t IMatchRoom::GetPlayerCount(bool isOnlyReal)
{
    uint16_t count = 0;
    for(auto User : list_MatchUser_)
    {
        if(isOnlyReal && ! User->ptrUserItem->IsAndroidUser())
        {
            count++;
        }else if(!isOnlyReal)
        {
            count++;
        }
    }
    return count;
}
/*
    排名每次需要根据上一次排名次序决定同分数先后
    上一局分数在前，下一局分数也在前
*/
void IMatchRoom::SortUser()
{
//    for(auto &User :list_MatchUser_)
//    {
//        cout<<User->ptrUserItem->GetUserId()<<endl;
//    }

//    list_MatchUser_.sort([]( shared_ptr<UserInfo> lhs,  shared_ptr<UserInfo> rhs)
//    {
//        return lhs->ptrUserItem->GetUserScore() >= rhs->ptrUserItem->GetUserScore();
//    });

      list<shared_ptr<UserInfo>> tmpList;
      while(!list_MatchUser_.empty())
      {
          auto bgn=list_MatchUser_.begin();
          auto tmp=bgn;
          while(tmp!=list_MatchUser_.end())
          {
              if((*bgn)->ptrUserItem->GetUserScore()<(*tmp)->ptrUserItem->GetUserScore())
              {
                bgn=tmp;
              }
              tmp++;
          }
          tmpList.push_back(*bgn);
          list_MatchUser_.erase(bgn);
      }
      list_MatchUser_.swap(tmpList);

//    for(auto &User :list_MatchUser_)
//    {
//        cout<<User->ptrUserItem->GetUserId()<<" Score:"<<User->ptrUserItem->GetUserScore()<<endl;
//    }
}

void IMatchRoom::GetRanks(MatchServer::MSG_S2C_UserRankResp &ranks)
{
    uint32_t userRank=1;
    for(auto User : list_MatchUser_)
    {
        MatchServer::rankInfo* rank = ranks.add_ranks();
        rank->set_rank(userRank);
        rank->set_score(User->ptrUserItem->GetUserScore());
        rank->set_userid(User->ptrUserItem->GetUserId());
        userRank++;
    }
    ranks.set_roundnum(cur_round_count_);
    ranks.set_gamenum(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]);
    ranks.set_passusernum(p_MatchRoomInfo_->upGradeUserNum[cur_round_count_]);
}



void IMatchRoom::clearAllUser()
{
    Landy::CMatchServer* server = dynamic_cast<Landy::CMatchServer *> (p_ILogicThread_) ;
    int32_t outCount=list_MatchUser_.size();
    for(uint16_t i=outCount;i!=0;i--)
    {
        shared_ptr<UserInfo> pUser = list_MatchUser_.back();
        list_MatchUser_.pop_back();
        if(server)
            server->DelUserFromServer(pUser->ptrUserItem);
    }
}

shared_ptr<CTableFrame> IMatchRoom::getSuitTable(shared_ptr<CServerUserItem>& pIServerUserItem)
{
    shared_ptr<ITableFrame> pITable;
    shared_ptr<CTableFrame> pTableFrame;
    for(auto TableFrame :list_UsedTable_)
    {
        if(TableFrame->CanJoinTable(pIServerUserItem))
        {
            pTableFrame=TableFrame;
            pITable = dynamic_pointer_cast<ITableFrame>(pTableFrame);
            break;
        }
    }

    if(!pTableFrame )
    {
        pITable = CGameTableManager::get_mutable_instance().FindNormalSuitTable(INVALID_TABLE);
        pTableFrame= dynamic_pointer_cast<CTableFrame> (pITable);
        if(!pITable)
        {
            LOG_ERROR<<"Fuck !!!  No enough Table=============";
            return NULL;
        }

        LOG_WARN<<"Change Table!!!   "<<pITable->GetTableId();
        pTableFrame->SetMatchRoom(this);
        list_UsedTable_.push_back(pTableFrame);
    }

    if(pTableFrame)
    {
        if(pIServerUserItem->IsAndroidUser())
        {
            shared_ptr<IAndroidUserItemSink> pSink = pIServerUserItem->GetAndroidUserItemSink();
            if(pSink)
            {
                pSink->SetTableFrame(pITable);
            }
        }
    }

    return pTableFrame;
}

void IMatchRoom::EnterRoomFinish(shared_ptr<CServerUserItem> &pIServerUserItem)
{

    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_ENTER_MATCH_RESP;

    ::MatchServer::MSG_S2C_UserEnterMatchResp response ;
    Game::Common::Header* header = response.mutable_header();
    header->set_sign(HEADER_SIGN);

    response.set_retcode(0);
    string content = response.SerializeAsString();
    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);
}

void IMatchRoom::SendMatchScene(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_MATCH_SCENE;
    ::MatchServer::MSG_S2C_Match_Scene Sence;

    Game::Common::Header* header = Sence.mutable_header();
    header->set_sign(HEADER_SIGN);

    Sence.set_score(pIServerUserItem->GetUserScore());

    int16_t rank=0;
    for(auto pUser : list_MatchUser_)
    {
        rank++;
        if(pUser->ptrUserItem->GetUserId() == pIServerUserItem->GetUserId())
        {
            Sence.set_sort(rank);
                Sence.set_letfroundgamecount(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]-pUser->curGameCount);
            break;
        }
    }
    Sence.set_totalroundgamecount(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]);
    Sence.set_passusercount(p_MatchRoomInfo_->upGradeUserNum[cur_round_count_]);
    Sence.set_needcount(p_MatchRoomInfo_->maxPlayerNum);
    Sence.set_usercount(GetAllPlyerCount());
    Sence.set_status(match_status_);

    string content = Sence.SerializeAsString();
    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);
}

void IMatchRoom::initStorageScore()
{
    try
    {
        mongocxx::options::find opts = mongocxx::options::find{};
        opts.projection(document{} <<"totalstock"<< 1 <<"totalstocklowerlimit"<<1 <<"totalstockhighlimit"<<1 << finalize);
        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["match_kind"];

        bsoncxx::stdx::optional<bsoncxx::document::value> result =
        coll.find_one(document{} <<"matchid"<<(int32_t)p_MatchRoomInfo_->matchRoomId << finalize, opts);
        if(result)
        {
            bsoncxx::document::view view = result->view();
            curStock_=view["totalstock"].get_int64();
            lowestStock_ =view["totalstocklowerlimit"].get_int64();
            highStock_=view["totalstockhighlimit"].get_int64();
        }

    }catch(exception &exp)
    {
        LOG_ERROR << "initStorageScore  exception:  "<< exp.what();
    }

    //根据库存跟报名情况判断是否控制输赢
    //应该是要根据奖品然后算库存，最后算玩家在库存上中下的概率
    curStock_ += winlost_Score_;

}

void IMatchRoom::updateStorageScore(int32_t score)
{
    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["match_kind"];
        coll.update_one(document{} << "matchid" << (int32_t)p_MatchRoomInfo_->matchRoomId << finalize,
                        document{} << "$inc" << open_document
                        <<"totalstock" << score << close_document
                        << finalize);
    }catch(exception &exp)
    {
        LOG_ERROR << "updateStorageScore err score:" <<score << " exception: " << exp.what();
    }
}
bool IMatchRoom::AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore)
{
    bool bSuccess = false;
//    try
//    {
        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << userBaseInfo.userId
                << "account" << userBaseInfo.account
                << "agentid" << (int32_t)userBaseInfo.agentId
                << "changetype" << (int32_t) p_MatchRoomInfo_->matchRoomId
                << "changemoney" << addScore
                << "beforechange" << sourceScore
                << "afterchange" << targetScore
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

#if 1
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if(result)
        {
        }
#else
        m_pLogicThread->RunWriteDataToMongoDB("gamemain", "user_score_record", INSERT_ONE, insert_value.view(), bsoncxx::document::view());
#endif

        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}

bool IMatchRoom::AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo)
{
    bool bSuccess = false;
//    try
//    {
        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << scoreInfo->userId
                << "account" << scoreInfo->account
                << "agentid" << (int32_t)scoreInfo->agentId
                << "linecode" <<scoreInfo->lineCode
                << "changetype" << (int32_t)p_MatchRoomInfo_->matchRoomId
                << "changemoney" << scoreInfo->addScore
                << "beforechange" << scoreInfo->beforeScore
                << "afterchange" << scoreInfo->beforeScore + scoreInfo->addScore
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if(result)
        {
        }

        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}

void IMatchRoom::writeUserScoreToDB(int32_t userId,int64_t score)
{
    bool bSuccess = false;
//    try
//    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                        <<"winorlosescore" << score
                        <<"score" << score << close_document
                        << finalize);
        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
}
//rank -1 退报名费/ 0 报名费 / 1~n 名次
void IMatchRoom::updateMatchRecord(GSUserBaseInfo& UserBase, int64_t score,int64_t beforechange,int64_t afterchange, int32_t rank,bool isAndroid)
{
    bool bSuccess = false;

    //    try
//    {
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                << "matchroomid" << strRoundId_
                << "userid" << UserBase.userId
                << "account" << UserBase.account
                << "agentid" << (int32_t)UserBase.agentId
                << "gameid" << (int32_t)p_MatchRoomInfo_->gameId
                << "roomid" << (int32_t)p_MatchRoomInfo_->matchRoomId
                << "matchname" <<p_MatchRoomInfo_->matchName
                << "rank"  << rank
                << "beforechange" <<beforechange
                << "score" << score
                << "afterchange"<<afterchange
                << "time" << bsoncxx::types::b_date(chrono::system_clock::now());

        auto doc = insert_value << bsoncxx::builder::stream::finalize;

		LOG_DEBUG << "--- *** ["
			<< strRoundId_ << "] "
			<< p_MatchRoomInfo_->matchRoomId << " { "
			<< p_MatchRoomInfo_->matchName << " } 更新 match_record";

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> _result = coll.insert_one(doc.view());
       
        bSuccess = true;
        if(!isAndroid && rank != 0 && rank != -1)  {
            // 更新任务系统
            TaskService::get_mutable_instance().updateTaskSystem(UserBase.userId, UserBase.agentId, 0, 0, p_MatchRoomInfo_->gameId, p_MatchRoomInfo_->matchRoomId,true);
        }

//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
}

void IMatchRoom::updateBestRecord(shared_ptr<CServerUserItem> &pIServerUserItem, int32_t score, int32_t rank)
{
    try
    {
        mongocxx::options::find opts = mongocxx::options::find{};
        opts.projection(document{} << "rank" << 1 <<"awardscore"<< 1 << finalize);
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_best_record"];
        bsoncxx::stdx::optional<bsoncxx::document::value> result =
        coll.find_one(document{} << "userid" << pIServerUserItem->GetUserId() <<"matchid"<<(int32_t)p_MatchRoomInfo_->matchRoomId << finalize, opts);
        bool updateTime=false;
        if(result)
        {
            bsoncxx::document::view view = result->view();
            score =score + view["awardscore"].get_int32();
            if(view["rank"].get_int32()>=rank)
            {
               // LOG_ERROR<<"update rank ============="<<rank;
                updateTime =true;
            }else
                rank = view["rank"].get_int32();
        }

        if(updateTime||!result)
        {
            mongocxx::options::update update=mongocxx::options::update();
            coll.update_one(document{} << "userid" << pIServerUserItem->GetUserId()
                            <<"matchid"<<p_MatchRoomInfo_->matchRoomId  << finalize,
                        document{} << "$set" << open_document
                        <<"awardscore" << score
                        <<"rank" << rank
                        <<"gameid" << (int32_t) p_MatchRoomInfo_->gameId
                        <<"time" << bsoncxx::types::b_date(chrono::system_clock::now())
                        << close_document
                        << finalize ,update.upsert(true));
        }else
        {
            mongocxx::options::update update=mongocxx::options::update();
            coll.update_one(document{}  <<"userid" << pIServerUserItem->GetUserId()
                            <<"matchid"<<p_MatchRoomInfo_->matchRoomId <<finalize,
                            document{} << "$set" << open_document
                            <<"awardscore" << score
                            <<"rank" << rank
                            <<"gameid" << (int32_t) p_MatchRoomInfo_->gameId << close_document
                            << finalize);
        }
    }
    catch(exception &e)
    {
        LOG_ERROR << "exception: " << e.what();
        throw;
    }

}

void IMatchRoom::OnUserEnterAction(shared_ptr<UserInfo>& pUserInfo, Header *commandHeader)
{
//    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
//    int subId  = ::MatchServer::SUB_S2C_ENTER_MATCH_RESP;
//    ::MatchServer::MSG_S2C_UserEnterMatchResp response ;
//    Game::Common::Header* header = response.mutable_header();
//    header->set_sign(HEADER_SIGN);

//    response.set_retcode(0);
//    string content = response.SerializeAsString();
//    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), commandHeader);
    shared_ptr<CServerUserItem>& pIServerUserItem = pUserInfo->ptrUserItem;
	//通知该玩家加入比赛成功
    EnterRoomFinish(pIServerUserItem);
	//通知该玩家刷新比赛人数
    SendMatchScene(pIServerUserItem);
	//真人玩家加入比赛
    if(!pIServerUserItem->IsAndroidUser())
    {
        writeUserScoreToDB(pIServerUserItem->GetUserId(),-p_MatchRoomInfo_->joinNeedSore);
        AddScoreChangeRecordToDB(pIServerUserItem->GetUserBaseInfo(),
                                 pUserInfo->realScore,-p_MatchRoomInfo_->joinNeedSore, pUserInfo->realScore-p_MatchRoomInfo_->joinNeedSore);

        updateMatchRecord(pIServerUserItem->GetUserBaseInfo(),
                          -p_MatchRoomInfo_->joinNeedSore,pUserInfo->realScore,pUserInfo->realScore-p_MatchRoomInfo_->joinNeedSore,0,false);
        pUserInfo->realScore -= p_MatchRoomInfo_->joinNeedSore ;
    }
	//真人玩家通知计数
	int c = 0;
	//比赛房间内广播通知刷新比赛人数
    for(auto user:list_MatchUser_)
    {
		//只广播通知真人玩家，忽略当前加入比赛房间的玩家，针对该玩家前面的SendMatchScene通知更新了
        if(!user->ptrUserItem->IsAndroidUser() &&user->ptrUserItem->GetUserId()!=pIServerUserItem->GetUserId())
        {
			LOG_DEBUG << "--- *** 加入比赛，刷新人数：" << GetAllPlyerCount() << " 广播次数：" << ++c;
            updateMatchPlayer(user->ptrUserItem);
        }
    }
}

void IMatchRoom::updateMatchPlayer(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_MATCH_PLAYER_UPDATE;
    ::MatchServer::MSG_S2C_Match_Player_Update update_player ;
    Game::Common::Header* header = update_player.mutable_header();
    header->set_sign(HEADER_SIGN);

    update_player.set_usercount(GetAllPlyerCount());
    string content = update_player.SerializeAsString();
    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);
}

void IMatchRoom::updateRankInfo()
{
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_MATCH_RANK_UPDATE;
    ::MatchServer::MSG_S2C_Match_Rank_Update update_rank ;
    Game::Common::Header* header = update_rank.mutable_header();
    header->set_sign(HEADER_SIGN);

    update_rank.set_passusernum(p_MatchRoomInfo_->upGradeUserNum[cur_round_count_]);
    update_rank.set_usernum(GetAllPlyerCount());
    int16_t rank=0;
    for(auto pUser : list_MatchUser_)
    {
        rank++;
        pUser->ptrUserItem->SetRank(rank);
        if(pUser->ptrUserItem->IsAndroidUser())
            continue;

        update_rank.set_letfroundgamecount(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]-pUser->curGameCount);
        update_rank.set_totalroundgamecount(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]);
        //本轮结束的玩家更新桌子数
        if(pUser->curGameCount >=p_MatchRoomInfo_->upgradeGameNum[cur_round_count_])
            update_rank.set_lefttablenum(list_UsedTable_.size());
        else
            update_rank.set_lefttablenum(-1);

        update_rank.set_rank(rank);
        string content = update_rank.SerializeAsString();
        SendPackedData(pUser->ptrUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);
    }

}

bool IMatchRoom::SendGameMessage(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t msgType, int64_t awardScore, uint32_t rank) {
	uint8_t mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC;
	uint8_t subId = 21;//::GameServer::SUB_GF_SYSTEM_MESSAGE;
	
	if (!pIServerUserItem) {
		return false;
	}
	if ((msgType & SMT_GLOBAL) && (msgType & SMT_SCROLL)) {
		GSUserBaseInfo userBaseItem = pIServerUserItem->GetUserBaseInfo();

		Json::Value root;
		root["gameId"] = p_MatchRoomInfo_->gameId;
		root["nickName"] = pIServerUserItem->GetNickName();
		root["agentId"] = userBaseItem.agentId;
		root["roomName"] = p_MatchRoomInfo_->matchName;
		root["roomId"] = p_MatchRoomInfo_->matchRoomId;
		root["msgId"] = rand() % 7;//szMessage;
		root["score"] = awardScore;
		root["rank"] = rank;
		Json::FastWriter writer;
		string msg = writer.write(root);
		LOG_DEBUG << "--- *** " << "跑马灯消息\n" << msg;
		REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_marquee, msg);
		return true;
	}
	return false;
}

void IMatchRoom::OnUserFinish(shared_ptr<UserInfo> &pUserInfo, int64_t awardScore,uint32_t rank)
{

    //怎么写到数据库更新还不知道 待确定
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_MATCH_FINISH;
    ::MatchServer::MSG_S2C_Match_Finish finish ;
    Game::Common::Header* header = finish.mutable_header();
    header->set_sign(HEADER_SIGN);
    finish.set_rank(rank);
    finish.set_awardscore(awardScore);
    shared_ptr<CServerUserItem> &pIServerUserItem = pUserInfo->ptrUserItem;
    //被淘汰的人
    if(p_MatchRoomInfo_->upGradeUserNum[cur_round_count_]>0)
        finish.set_finishstatus(1);
    else
        finish.set_finishstatus(0);

    string content = finish.SerializeAsString();
    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);

    if(awardScore!=0)
    {
        winlost_Score_ -=awardScore;
        writeUserScoreToDB(pIServerUserItem->GetUserId(),awardScore);
        AddScoreChangeRecordToDB(pIServerUserItem->GetUserBaseInfo(),pUserInfo->realScore,awardScore,pUserInfo->realScore+awardScore);
    }
    //为0的暂时先记录，不知道是否有用

    updateMatchRecord(pIServerUserItem->GetUserBaseInfo(),awardScore,pUserInfo->realScore,pUserInfo->realScore+awardScore,rank,pIServerUserItem->IsAndroidUser());
    updateBestRecord(pIServerUserItem,awardScore,rank);
}

void IMatchRoom::OnUserUpgrade(shared_ptr<CServerUserItem> &pIServerUserItem, uint16_t rank)
{
    //怎么写到数据库更新还不知道 待确定
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER;
    int subId  = ::MatchServer::SUB_S2C_UPGRADE;
    ::MatchServer::MSG_S2C_Upgrade upgrade ;
    Game::Common::Header* header = upgrade.mutable_header();
    header->set_sign(HEADER_SIGN);

    upgrade.set_rank(rank);
    upgrade.set_gamenum(p_MatchRoomInfo_->upgradeGameNum[cur_round_count_]);
    upgrade.set_roundnum(cur_round_count_+1);
    upgrade.set_passusernum(p_MatchRoomInfo_->upGradeUserNum[cur_round_count_]);

    string content = upgrade.SerializeAsString();
    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), NULL);

}
bool IMatchRoom::SendPackedData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                                 const uint8_t* data, int size, Header *commandHeader)
{
    bool bSuccess = false;


    do
    {
        vector<unsigned char> vecdata(size);
        memmove(&vecdata[0],data,size);

        int enctype = PUBENC__PROTOBUF_AES; // no encrypt.
        if(g_bisDisEncrypt)
        {
            enctype = PUBENC__PROTOBUF_NONE;
        }
        bSuccess = sendData(pIServerUserItem, mainId, subId, vecdata, commandHeader, enctype);
    }   while (0);
//Cleanup:
    return (bSuccess);
}


//
bool IMatchRoom::sendData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                           vector<uint8_t> &data, Header *commandHeader, int enctype)
{
    bool bSuccess = false;
    do
    {
        if(!pIServerUserItem)
            break;
        if(pIServerUserItem && !pIServerUserItem->IsAndroidUser())
        {
            string aesKey = "";

            uint32_t realSize = data.size();

            int64_t userId = pIServerUserItem->GetUserId();
            boost::tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>> mytuple = p_ILogicThread_->GetProxyConnectionWeakPtrFromUserId(userId);
            TcpConnectionWeakPtr weakConn = mytuple.get<0>();
            shared_ptr<internal_prev_header> internal_header_ptr = mytuple.get<1>();
            TcpConnectionPtr conn(weakConn.lock());
            if(likely(conn) && internal_header_ptr)
            {
                // try to check if the special interal header has breen ok and get the content now.
                if (!internal_header_ptr)
                {
                    LOG_ERROR << " >>> sendData get internal_header failed error!,userId:" << pIServerUserItem->GetUserId();
                    break;
                }

                int ret = -1;
                vector<unsigned char> encryptedData;
                {
                    int packsize = sizeof(internal_prev_header) + sizeof(Header) + data.size();
                    encryptedData.resize(packsize);
                    memcpy(&encryptedData[sizeof(internal_prev_header)+sizeof(Header)], &data[0], data.size());

                    internal_prev_header* internal = (internal_prev_header*)&encryptedData[0];
                    memcpy(internal, internal_header_ptr.get(), sizeof(internal_prev_header));
                    internal->len = packsize;
                    GlobalFunc::SetCheckSum(internal);
//                    internal++;

                    Header* pubheader = (Header*)(&encryptedData[sizeof(internal_prev_header)]);
                    if(commandHeader)
                    {
                        memcpy(pubheader, commandHeader, sizeof(Header));
                    }else
                    {
                        pubheader->ver = PROTO_VER;
                        pubheader->sign = HEADER_SIGN;
                        pubheader->reserve = 0;
                        pubheader->reqId = 0;
                    }
                    pubheader->mainId = mainId;
                    pubheader->subId = subId;
                    pubheader->encType = PUBENC__PROTOBUF_NONE;
                    pubheader->realSize = realSize;
                    pubheader->len = realSize + sizeof(Header);
                    pubheader->realSize = realSize;
                    pubheader->crc = GlobalFunc::GetCheckSum(&encryptedData[sizeof(internal_prev_header)+4], pubheader->len - 4);
                    ret = packsize;
                }

                if (likely(ret > 0 && !encryptedData.empty()))
                {
					TRACE_COMMANDID(mainId, subId);
                    // send the special data content.
                    conn->send(&encryptedData[0], ret);
                    bSuccess = true;
                }
            }
            else
            {
                int64_t userId = pIServerUserItem->GetUserId();
                bool  isAndroid = pIServerUserItem->IsAndroidUser();
                LOG_WARN <<"CTableFrame sendData TcpConnectionPtr closed. userId:" << userId << ", isAndroid:" << isAndroid << ",mainid:" << mainId << "subid:" << subId;
            }
        }
        else
        {
            // try to send the special user data item value content value content value data now value data now.
            //bSuccess = pIServerUserItem->SendUserMessage(mainId, subId, (const uint8_t*)&data[0], data.size());
        }
    }while (0);

    return (bSuccess);
}
