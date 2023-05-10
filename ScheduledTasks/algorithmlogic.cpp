#include "algorithmlogic.h"
#include <iostream>
muduo::AsyncLogging* g_asyncLog = NULL;
AlgorithmLogic::AlgorithmLogic():
    timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "ThreadInitCallback"))
{



    if(boost::filesystem::exists("./conf/task.conf"))
    {

        boost::property_tree::ptree pt;
        boost::property_tree::read_ini("./conf/task.conf", pt);
        m_mongoDBServerAddr = pt.get<string>("MongoDB.Url");
        jetLag = pt.get<int32_t>("Global.jetLag",8);
        isOpenAlltime = pt.get<int32_t>("Global.isOpenAlltime",0);


        m_iyear = pt.get<int32_t>("Global.m_iyear",0);
        m_iday = pt.get<int32_t>("Global.m_iday",0);
        m_imonth = pt.get<int32_t>("Global.m_imonth",0);


        m_dyear = pt.get<int32_t>("Global.m_dyear",0);
        m_dday = pt.get<int32_t>("Global.m_dday",0);
        m_dmonth = pt.get<int32_t>("Global.m_dmonth",0);

        isOpenDelete= pt.get<int32_t>("Global.isOpenDelete",0);

        onlyDelete = pt.get<int32_t>("Global.onlyDelete",0);

        m_insertDistanceTime = pt.get<int32_t>("Global.m_insertDistanceTime",24);
        m_deleteDistanceTime = pt.get<int32_t>("Global.m_deleteDistanceTime",24);
        mongocxx::instance instance{};
        ThreadLocalSingletonMongoDBClient::setUri(m_mongoDBServerAddr);
        mongocxx::database db = MONGODBCLIENT["gamemain"];
        InitDate();

        OnMakeDir();
//        muduo::TimeZone beijing(60*60*8, "CST");
//        muduo::Logger::setTimeZone(beijing);

//        muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
//        muduo::Logger::setOutput(asyncOutput);

//        if(!boost::filesystem::exists("./log/task/"))
//        {
//            boost::filesystem::create_directories("./log/task/");
//        }
//        int loglevel = pt.get<int>("Global.loglevel",1);
//        muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
//        muduo::AsyncLogging log(::basename("task"), 1024*1024*500);
//        log.start();
//        g_asyncLog = &log;



        timelooper->getLoop()->runInLoop(bind(&AlgorithmLogic::InItAgorithm,this));
    }

}

void AlgorithmLogic::InitDate()
{
    timelooper->startLoop();

}
AlgorithmLogic::~AlgorithmLogic()
{

}
bool AlgorithmLogic::InItAgorithm()
{   

    userMap.clear();
    StartTimeTnterval = 1;
    //timelooper->getLoop()->cancel(StartProgramTime);
    StartProgramTime = timelooper->getLoop()->runAfter(StartTimeTnterval,bind(&AlgorithmLogic::FucTimer,this));
    return true;
}

time_point<system_clock> AlgorithmLogic::getTimePoint(tm t)
{


    struct tm* tx = (struct tm*)malloc(sizeof(struct tm));
    tx->tm_year = t.tm_year ;
    tx->tm_mon = t.tm_mon;
    tx->tm_mday = t.tm_mday;
    tx->tm_hour = t.tm_hour;
    tx->tm_min = t.tm_min;
    tx->tm_sec = t.tm_sec;
    time_t shijianlong= mktime(tx);
    time_point<system_clock> then_tp = system_clock::from_time_t(shijianlong);
    delete tx;
    return then_tp;
}
int64_t AlgorithmLogic::GetStartTimeSeconds()
{
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    time_t tx = time(NULL); //获取目前秒时间
    //struct timeval local;

    struct tm * local;
    local = localtime(&tx); //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r
    struct tm* t = (struct tm*)malloc(sizeof(struct tm));
    t->tm_year = local->tm_year ;
    t->tm_mon = local->tm_mon;
    t->tm_mday = local->tm_mday;
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    time_t shijianlong= mktime(t);


    delete t;
    //早上0时0分0秒 到计算完的时间 单位秒
    int64_t beginStart=tx-shijianlong;

    openLog("  当前启动时间到 早上0分0秒的时间差是  %d  秒",beginStart);
    //今天结束以后，从0秒加上一天的秒数，加一分钟，也就是第二天0时一分钟开始启动
    return ONE_DAY_HAS_SECONDS-beginStart+60;
}
//新注册玩家汇总 ,并且查出所有投注玩家的上级id
void AlgorithmLogic::NewRegisteredUser()
{

    try
    {
        mongocxx::options::find opts = mongocxx::options::find{};
        opts.projection(document{} << "userid" << 1 << "agentid" << 1 << "superior" << 1 << "subagents" << 1 << finalize);
        //opts.projection(bsoncxx::builder::stream::document{}<<"isandroid"<<false<< bsoncxx::builder::stream::finalize);
        //opts.sort(document{} << "gameendtime" << -1 << finalize); //此处后期可以调整

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        auto query_value = document{} << "registertime" <<open_document<<
                                         "$gte" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)-std::chrono::hours(m_insertDistanceTime))<<
                                         "$lt" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag))<<  close_document<<finalize;


        NewUserMap.clear();
        auto maybe_result = coll.find(query_value.view(),opts);


        int64_t dbUserId=0;
        int32_t dbAgentId = 0;
        int64_t dbUpUserId = 0;
        for(auto &view : maybe_result)
        {

            NewUserAccumulate newUser;
            dbUserId    = view["userid"].get_int64();
            dbAgentId   = view["agentid"].get_int32();
            if(view["superior"].type()!=bsoncxx::type::k_null)
            {
                dbUpUserId  = view["superior"].get_int64();
                if(dbUpUserId!=0)
                {
                    if(view["subagents"].type()!=bsoncxx::type::k_null)
                    {
                        auto arr = view["subagents"].get_array();
                        for(auto user:arr.value)
                        {
                            int64_t upuser=user.get_int32();
                            if(upuser!=dbUpUserId)
                            newUser.uplist.push_back(upuser);

                        }
                    }

                }

            }
            else
            {
                dbUpUserId = 0;
            }
            newUser.agentid = dbAgentId;
            newUser.userid = dbUserId;
            newUser.upUserid = dbUpUserId;
            NewUserMap[dbUserId] = newUser;
        }
    }
    catch(exception &e)
    {
        openLog(" 读取当天新注册玩家出现异常 ");
    }
}
//所有产生新增玩家的列表统计
void AlgorithmLogic::HaveAddNewRegisteredUser()
{
    HaveAddNewUserMap.clear();
    int64_t newusercount = 0;
    int64_t underusercount = 0;
    for(auto &newuser:NewUserMap)
    {
        newusercount++;
        if(newuser.second.upUserid!=0)
        {
            underusercount++;
            if(HaveAddNewUserMap.find(newuser.second.upUserid)!=HaveAddNewUserMap.end())
            {
                HaveAddNewUserMap[newuser.second.upUserid].directlyAdd+=1;
                HaveAddNewUserMap[newuser.second.upUserid].teamAdd +=1;
            }
            else
            {
                NewUserAccumulate addUser;
                addUser.directlyAdd+=1;
                addUser.userid = newuser.second.upUserid;
                addUser.agentid = newuser.second.agentid;
                addUser.teamAdd +=1;
                HaveAddNewUserMap[addUser.userid] = addUser;
            }
            for(auto user:newuser.second.uplist)
            {
                if(HaveAddNewUserMap.find(user)!=HaveAddNewUserMap.end())
                {
                    HaveAddNewUserMap[user].teamAdd +=1;
                }
                else
                {
                    NewUserAccumulate addUser;
                    addUser.userid = user;
                    addUser.agentid = newuser.second.agentid;
                    addUser.teamAdd +=1;
                    HaveAddNewUserMap[user] = addUser;
                }
            }
        }
    }
    openLog(" 当天新注册的玩家数是 %d    通过扫码产生的玩家数是   %d",newusercount,underusercount);
}
//汇总当天玩家投注 ,并且查出所有投注玩家的上级id
void AlgorithmLogic::AggregatePlayerBets()
{
    int64_t bettingNum = 0; //
    try
    {
        mongocxx::options::find opts = mongocxx::options::find{};
        //opts.projection(document{} << "userid" << 1 << "account" << 1 << "agentid" << 1 << "gameid" << 1 << finalize);
        opts.projection(bsoncxx::builder::stream::document{}<<"isandroid"<<false<< bsoncxx::builder::stream::finalize);
        opts.sort(document{} << "gameendtime" << -1 << finalize); //此处后期可以调整

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
        auto query_value = document{} << "gameendtime" <<open_document<<
                                         "$gte" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)-std::chrono::hours(m_insertDistanceTime))<<
                                         "$lt" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag))<<
                                         close_document<<finalize;


        mongocxx::cursor  maybe_result = coll.find(query_value.view(),opts);


        int64_t dbUserId=0;
        int64_t dbBetScore =0;
        int64_t dbRevent = 0;
        int32_t dbAgentId = 0;
        int64_t dbWinScore = 0;


        userMap.clear();


        for(auto &view :  maybe_result)
        {


            //::document::view xxxx = elem.view();


            //dbAccount   = view["gameinfoid"].get_utf8().value.to_string();
            dbUserId    = view["userid"].get_int64();
            dbBetScore  = view["rwinscore"].get_int64();
            dbRevent    = view["revenue"].get_int64();
            dbAgentId   = view["agentid"].get_int32();
            dbWinScore  = view["winscore"].get_int64();

            //出去以前的代理号
            if(find(proxyEffectivevct.begin(),proxyEffectivevct.end(),dbAgentId)!=proxyEffectivevct.end())
            {

            }
            else
            {
                continue;
            }
            //int64_t timeSeconds=view["gameendtime"].get_date();
            //string date = InitialConversion(timeSeconds/1000);



            if(userMap.find(dbUserId)!=userMap.end())
            {
                userMap[dbUserId].userid = dbUserId;
                userMap[dbUserId].agentid = dbAgentId;
                //税收汇总
                userMap[dbUserId].revent +=dbRevent;
                //投注量汇总
                userMap[dbUserId].bettingScore +=dbBetScore;
                userMap[dbUserId].winScore += dbWinScore;
            }
            else
            {
                bettingNum++;
                StruserInformation userInfo;
                userInfo.userid = dbUserId;
                userInfo.agentid = dbAgentId;
                userInfo.revent = dbRevent;
                userInfo.bettingScore = dbBetScore;
                userInfo.winScore = dbWinScore;
                mongocxx::options::find opts1 = mongocxx::options::find{};
                mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                auto query_user = document{} << "userid" << dbUserId << finalize;
                // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
                bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                if(results)
                {
                     //添加玩家所有上级id
                     bsoncxx::document::view view = results->view();

                     if(view["superior"])
                     {
                         int64_t superiorid = view["superior"].get_int64();
                         if(view["subagents"]&&superiorid!=0)
                         {
                             auto arr = view["subagents"].get_array();
                             for(auto user:arr.value)
                             {
                                 int64_t upuser=user.get_int64();
                                 userInfo.superiorAgent.push_back(upuser);
                             }
                         }

                     }
                }
                else
                {
                    //
                    openLog("no   up user  userid = %d",dbUserId);
                }

                userMap[dbUserId] = userInfo;
            }
        }
    }
    catch(exception & e)
    {
        openLog(" 汇总当天投注的玩家的时候产生异常 ");

    }


    openLog("maybe_result  size =%d  ",bettingNum);
    LOG_ERROR <<" maybe_result  size      " << bettingNum;
}
void AlgorithmLogic::CalculatAllBenefitsUser()
{
    incomeUserMap.clear();
    for(auto &betuser:userMap)
    {

        if(incomeUserMap.find(betuser.second.userid)!=incomeUserMap.end())
        {
            incomeUserMap[betuser.second.userid].selfBetting = betuser.second.bettingScore;
        }
        else
        {
            StrIncomeUser benifiuser;
            benifiuser.userid = betuser.second.userid;
            benifiuser.agentid = betuser.second.agentid;
            benifiuser.selfBetting = betuser.second.bettingScore;


            mongocxx::options::find opts1 = mongocxx::options::find{};
            mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
            auto query_user = document{} << "userid" << benifiuser.userid  << finalize;
            // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
            bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
            if(results)
            {
                //添加玩家所有上级id,查询玩家保底值
                bsoncxx::document::view view = results->view();
                if(view["superior"])
                {
                    benifiuser.upUserid = view["superior"].get_int64();
                }
                if(view["minrebaterate"])
                {
                    benifiuser.guarantee= view["minrebaterate"].get_int32();
                }

            }
            incomeUserMap[benifiuser.userid] = benifiuser;
        }

        for(int i=0;i<betuser.second.superiorAgent.size();i++)
        {
            //给他上级玩家的团队业绩累加
            if(incomeUserMap.end()!=incomeUserMap.find(betuser.second.superiorAgent[i]))
            {
                incomeUserMap[betuser.second.superiorAgent[i]].teamBetting+=betuser.second.bettingScore;
            }
            else
            {
                StrIncomeUser benifinew;
                benifinew.userid = betuser.second.superiorAgent[i];
                benifinew.agentid = betuser.second.agentid;
                benifinew.teamBetting = betuser.second.bettingScore;

                benifinew.upUserid = betuser.second.userid;

                mongocxx::options::find opts1 = mongocxx::options::find{};
                mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                auto query_user = document{} << "userid" << benifinew.userid  << finalize;
                // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
                bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                if(results)
                {
                    //查询玩家保底值
                    bsoncxx::document::view view = results->view();
                    benifinew.guarantee= view["minrebaterate"].get_int32();
                }
                incomeUserMap[betuser.second.superiorAgent[i]] = benifinew;
            }
        }
    
    
    
       
    
    }
    //上面已经找出所有产生效益的玩家，以及算出每个玩家团队业绩和自身业绩
    //但是还没有保底值，保底值还要通过玩家总业绩算出来
    //proxyinfoMap 这个列表需要在最前面的时候查出来
    for(auto &benifiuser:incomeUserMap)
    {
        int64_t allbetting = benifiuser.second.teamBetting+benifiuser.second.selfBetting;
        if(proxyinfoMap.find(benifiuser.second.agentid)!=proxyinfoMap.end())
        {
            for(auto &lev:proxyinfoMap[benifiuser.second.agentid])
            {

               if(allbetting>=lev.minvalidbet*100&&allbetting<lev.maxvalidbet*100)
               {
                   benifiuser.second.guarantee = lev.brokerage;
                   break;
               }
            }
        }
        benifiuser.second.selfProfit=benifiuser.second.selfBetting*benifiuser.second.guarantee/10000;//自营利润，万分比
    }
    for(auto &benifiuser:incomeUserMap)
    {
        int64_t allbetting = benifiuser.second.teamBetting+benifiuser.second.selfBetting;
        if(allbetting==0||benifiuser.second.upUserid==0)
        {
            continue;
        }
        if(incomeUserMap[benifiuser.second.upUserid].guarantee<benifiuser.second.guarantee)
        {
            continue;
        }
        openLog(" 团队业绩的下级id是 %d   累计的玩家是",benifiuser.second.userid,benifiuser.second.upUserid);

        openLog(" 下级玩家的团队业绩是 %d       自营业绩是 %d",benifiuser.second.teamBetting,benifiuser.second.selfBetting);

        openLog(" 上级佣金比 %d       下级佣金比 %d",incomeUserMap[benifiuser.second.upUserid].guarantee,benifiuser.second.guarantee);
        incomeUserMap[benifiuser.second.upUserid].teamProfit+=allbetting*(incomeUserMap[benifiuser.second.upUserid].guarantee-benifiuser.second.guarantee)/10000;//累加上级玩家的团队业绩
        benifiuser.second.contribution=allbetting*(incomeUserMap[benifiuser.second.upUserid].guarantee-benifiuser.second.guarantee)/10000;//累加上级玩家的团队业绩
    }

    //到这里所有玩家的自营业绩和团队业绩都算出来了
    //下面是新增玩家，假如有业绩，就把新增值加到业绩注单，没有就新加对象，写一条没有业绩的新增条码
    for(auto &regis:HaveAddNewUserMap)
    {
        if(incomeUserMap.find(regis.second.userid)!=incomeUserMap.end())
        {
            incomeUserMap[regis.second.userid].newUserZhi = regis.second.directlyAdd;
            incomeUserMap[regis.second.userid].newUserTuan= regis.second.teamAdd;
        }
        else
        {
            //上面找不到的玩家证明是没有任何业绩的玩家，只写入新增注册即可
            StrIncomeUser benifinew;
            benifinew.userid =  regis.second.userid;
            benifinew.agentid = regis.second.agentid;
            benifinew.newUserTuan = regis.second.teamAdd;
            benifinew.newUserZhi  = regis.second.directlyAdd;
            incomeUserMap[regis.second.userid] = benifinew;
        }
    }


//    ///算出玩家给上级算出的佣金贡献值
//    for(auto &benifiuser:incomeUserMap)
//    {
//        if(benifiuser.second.upUserid==0)
//        {
//            continue;
//        }
//        //返佣差
//        int64_t difference = incomeUserMap[benifiuser.second.upUserid].guarantee - benifiuser.second.guarantee;
//        benifiuser.second.contribution = difference*(benifiuser.second.selfBetting+benifiuser.second.teamProfit);
//    }
}
//把所有当天产生的玩家业绩收入写成注单
void AlgorithmLogic::InsertUserProfitTable()
{
    //删除当天的这个表的数据
    try
    {
        mongocxx::collection collx = MONGODBCLIENT["gamemain"]["play_commission"];
        auto query_value = document{} << "profitinserttime" <<open_document<<
                                         "$gte" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)-std::chrono::hours(24))<<
                                         "$lt" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag))<<
                                         close_document <<"status"<<0
                                         <<finalize;
       bsoncxx::stdx::optional<mongocxx::result::delete_result> result =
         collx.delete_many(query_value.view()); //删除未审核的
       if(result)
       {
           LOG_ERROR <<"delete MongoDB count: " << result->deleted_count();
       }
    }
    catch(exception &e)
    {
        openLog(" 删除当天插入的表异常 ");
    }



    for(auto &userprofit:incomeUserMap)
    {
        if(isOpenAlltime)
        {
            //假如是设定截止时间，那插入时间就是截止时间加上60秒
            try
            {
                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_commission"];
                auto cursorpz = coll.find_one(document{} <<"userid"<<userprofit.second.userid<< "profitinserttime" <<open_document<<
                                              "$gte" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)-std::chrono::hours(m_insertDistanceTime))<<
                                              "$lt" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag))<<
                                              close_document<<finalize);// (query_value.view());li

                if(!cursorpz)
                {
                    bsoncxx::builder::stream::document builder{};
                    auto insert_value = builder
                    << "userid" << (int64_t)userprofit.second.userid
                    << "agentid" << userprofit.second.agentid
                    << "selfbetting" << userprofit.second.selfBetting
                    << "guarantee" << userprofit.second.guarantee
                    << "teambetting" << userprofit.second.teamBetting
                    << "selfprofit" << userprofit.second.selfProfit
                    << "teamprofit" << userprofit.second.teamProfit
                    << "commissioncontribution" << userprofit.second.contribution
                    << "audituser"  <<""
                    << "audittime"   << bsoncxx::types::b_date(chrono::system_clock::now())
                    << "insertdate"   <<  to_string(m_EndDate.tm_year+1900)+"-"+to_string(m_EndDate.tm_mon+1)+"-"+to_string(m_EndDate.tm_mday)
                    << "status"<<(int32_t)0
                    << "directlyuserincrease" <<(int32_t)userprofit.second.newUserZhi
                    << "teamuserincrease" << (int32_t)userprofit.second.newUserTuan
                    << "profitinserttime" << bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)+std::chrono::seconds(60)-std::chrono::hours(24))
                    << bsoncxx::builder::stream::finalize;

                    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
                    if (!result)
                    {
                        LOG_ERROR << "play_commission insert exception: ";
                    }

                    time_t rawtime;
                    tm * timeinfo;
                    time(&rawtime);
                    timeinfo=localtime(&rawtime);
                    //星期天重新计算
                    if(timeinfo->tm_wday==0)
                    {
                        mongocxx::collection colluser = MONGODBCLIENT["gamemain"]["game_user"];
                        colluser.update_one(document{} << "userid" << (int64_t)(userprofit.second.userid) << bsoncxx::builder::stream::finalize,
                                                   document{} << "$set" << open_document
                                                   << "gradesweek"  <<(int64_t) (userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << "gradesday"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << finalize);
                    }
                    else

                    {
                        mongocxx::collection colluser = MONGODBCLIENT["gamemain"]["game_user"];
                        colluser.update_one(document{} << "userid" << (int64_t)userprofit.second.userid << bsoncxx::builder::stream::finalize,
                                        document{} << "$inc" << open_document
                                                   << "gradesweek"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << "$set" << open_document
                                                   << "gradesday"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << finalize);
                    }

                    int64_t allBet = 0;
                    string accountid="";
                    int32_t agentid=0;
                    mongocxx::options::find opts1 = mongocxx::options::find{};
                    opts1.projection(bsoncxx::builder::stream::document{} << "totalvalidbet" << 1
                                                                          << "account"<<1
                                                                          << "agentid"<<1
                                                                         << bsoncxx::builder::stream::finalize);
                    mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                    auto query_user = document{} << "userid" << userprofit.second.userid << finalize;
                    // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
                    bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                    if(results)
                    {

                        bsoncxx::document::view view = results->view();
                        allBet = view["totalvalidbet"].get_int64();
                        agentid = view["agentid"].get_int32();
                        accountid = view["account"].get_utf8().value.to_string();
                    }



                    //cha ru  wan jia zhudan
                    mongocxx::collection bankcoll = MONGODBCLIENT["gamemain"]["user_sign_in_info"];
                    auto query_value = document{} << "userid" << userprofit.second.userid << finalize;
                    auto seq_updateValue = document{} << "$set" << open_document << "lastvalidbet"
                                                      << allBet
                                                      << close_document << finalize;
                    //update options
                    //mongocxx::options::update options = mongocxx::options::update{};
                    bsoncxx::stdx::optional<bsoncxx::document::value> result1 = bankcoll.find_one(query_value.view());

                    if(!result1)
                    {
                        time_t curstamp = time(nullptr);  // UTC秒数
                        time_t forwardtimestamp = time_t(curstamp-86400);//求出前一天的对应点的时间戳
                        tm *tpforward = localtime(&forwardtimestamp);           //求出前一天的时间结构体
                        int64_t  lasttimestamp = getZeroTimeStamp(tpforward);

                        auto insertValue = bsoncxx::builder::stream::document{}
                                                        << "userid" << userprofit.second.userid  << "agentid" << agentid << "account" << accountid
                                                        << "seriesday" << 0 << "lastday" << 0                //时间日期 //连续签到天数
                                                        << "today" << tpforward->tm_mday  << "lastvalidbet" << allBet   //今天签到时间 签到时的流水 totalvalidbet
                                                        << "curmon" << (tpforward->tm_mon + 1) << "lastmon" << 0       //今天签到时间 签到时的流水
                                                        << "lasttimestamp" <<  lasttimestamp         //当前日期零点时间戳
                                                        << "createtime" << bsoncxx::types::b_date{chrono::system_clock::now()}          //创建时间
                                                        <<  bsoncxx::builder::stream::finalize;
                        if(!bankcoll.insert_one(insertValue.view())) {
                            LOG_ERROR << "---签到表增加玩家失败,userId["<< userprofit.second.userid << ",agentid[" << agentid << "]";
                        }
                    }
                    else
                    {
                        bankcoll.update_one(query_value.view(), seq_updateValue.view());
                    }
                    LOG_ERROR <<" user_sign_in_info:   userid" <<userprofit.second.userid<<"   lastvalidbet   "<<userprofit.second.selfBetting;

                }



            }
            catch (const std::exception &e)
            {
                LOG_ERROR <<" MongoDB exception: " << e.what();
            }
        }
        else
        {
            //假如不设定截止时间，那插入时间就是当前时间
            try
            {
                mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_commission"];
                auto cursorpz = coll.find_one(document{} <<"userid"<<userprofit.second.userid<< "profitinserttime" <<open_document<<
                                              "$gte" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)-std::chrono::hours(m_insertDistanceTime))<<
                                              "$lt" <<bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag))<<
                                              close_document<<finalize);// (query_value.view());li

                if(!cursorpz)
                {
                    bsoncxx::builder::stream::document builder{};
                    auto insert_value = builder
                            << "userid" << (int64_t)userprofit.second.userid
                            << "agentid" << userprofit.second.agentid
                            << "selfbetting" << userprofit.second.selfBetting
                            << "guarantee" << userprofit.second.guarantee
                            << "teambetting" << userprofit.second.teamBetting
                            << "selfprofit" << userprofit.second.selfProfit
                            << "teamprofit" << userprofit.second.teamProfit
                            << "commissioncontribution" << userprofit.second.contribution
                            << "audituser"  <<""
                            << "audittime"   << bsoncxx::types::b_date(chrono::system_clock::now())
                            << "insertdate"   <<  to_string(m_EndDate.tm_year+1900)+"-"+to_string(m_EndDate.tm_mon+1)+"-"+to_string(m_EndDate.tm_mday)
                            << "status"<<(int32_t)0
                            << "directlyuserincrease" <<(int32_t)userprofit.second.newUserZhi
                            << "teamuserincrease" << (int32_t)userprofit.second.newUserTuan
                            << "profitinserttime" << bsoncxx::types::b_date(getTimePoint(m_EndDate)+std::chrono::hours(jetLag)+std::chrono::seconds(60)-std::chrono::hours(24))
                            << bsoncxx::builder::stream::finalize;
                    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_commission"];
                    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
                    if (!result)
                    {
                        LOG_ERROR << "play_commission insert exception: ";
                    }
                    time_t rawtime;
                    tm * timeinfo;
                    time(&rawtime);
                    timeinfo=localtime(&rawtime);
                    //星期天重新计算
                    if(timeinfo->tm_wday==0)
                    {
                        mongocxx::collection colluser = MONGODBCLIENT["gamemain"]["game_user"];
                        colluser.update_one(document{} << "userid" << (int64_t)(userprofit.second.userid) << bsoncxx::builder::stream::finalize,
                                                   document{} << "$set" << open_document
                                                   << "gradesweek"  <<(int64_t) (userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << "gradesday"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << finalize);
                    }
                    else
                    {
                        mongocxx::collection colluser = MONGODBCLIENT["gamemain"]["game_user"];
                        colluser.update_one(document{} << "userid" << (int64_t)userprofit.second.userid << bsoncxx::builder::stream::finalize,
                                        document{} << "$inc" << open_document
                                                   << "gradesweek"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << "$set" << open_document
                                                   << "gradesday"  << (int64_t)(userprofit.second.teamBetting+userprofit.second.selfBetting)
                                                   << close_document
                                                   << finalize);
                    }


                    int64_t allBet = 0;
                    string accountid="";
                    int32_t agentid=0;
                    mongocxx::options::find opts1 = mongocxx::options::find{};
                    opts1.projection(bsoncxx::builder::stream::document{} << "totalvalidbet" << 1
                                                                          << "account"<<1
                                                                          << "agentid"<<1
                                                                         << bsoncxx::builder::stream::finalize);
                    mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                    auto query_user = document{} << "userid" << userprofit.second.userid << finalize;
                    // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
                    bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                    if(results)
                    {

                        bsoncxx::document::view view = results->view();
                        allBet = view["totalvalidbet"].get_int64();
                        agentid = view["agentid"].get_int32();
                        accountid = view["account"].get_utf8().value.to_string();
                    }



                    //cha ru  wan jia zhudan
                    mongocxx::collection bankcoll = MONGODBCLIENT["gamemain"]["user_sign_in_info"];
                    auto query_value = document{} << "userid" << userprofit.second.userid << finalize;
                    auto seq_updateValue = document{} << "$set" << open_document << "lastvalidbet"
                                                      << allBet
                                                      << close_document << finalize;
                    //update options
                    //mongocxx::options::update options = mongocxx::options::update{};
                    bsoncxx::stdx::optional<bsoncxx::document::value> result1 = bankcoll.find_one(query_value.view());

                    if(!result1)
                    {
                        time_t curstamp = time(nullptr);  // UTC秒数
                        time_t forwardtimestamp = time_t(curstamp-86400);//求出前一天的对应点的时间戳
                        tm *tpforward = localtime(&forwardtimestamp);           //求出前一天的时间结构体
                        int64_t  lasttimestamp = getZeroTimeStamp(tpforward);

                        auto insertValue = bsoncxx::builder::stream::document{}
                                                        << "userid" << userprofit.second.userid  << "agentid" << agentid << "account" << accountid
                                                        << "seriesday" << 0 << "lastday" << 0                //时间日期 //连续签到天数
                                                        << "today" << tpforward->tm_mday  << "lastvalidbet" << allBet   //今天签到时间 签到时的流水 totalvalidbet
                                                        << "curmon" << (tpforward->tm_mon + 1) << "lastmon" << 0       //今天签到时间 签到时的流水
                                                        << "lasttimestamp" <<  lasttimestamp         //当前日期零点时间戳
                                                        << "createtime" << bsoncxx::types::b_date{chrono::system_clock::now()}          //创建时间
                                                        <<  bsoncxx::builder::stream::finalize;
                        if(!bankcoll.insert_one(insertValue.view())) {
                            LOG_ERROR << "---签到表增加玩家失败,userId["<< userprofit.second.userid << ",agentid[" << agentid << "]";
                        }
                    }
                    else
                    {
                        bankcoll.update_one(query_value.view(), seq_updateValue.view());
                    }
                    LOG_ERROR <<" user_sign_in_info:   userid" <<userprofit.second.userid<<"   lastvalidbet   "<<userprofit.second.selfBetting;

                }



            }
            catch (const std::exception &e)
            {
                LOG_ERROR <<" MongoDB exception: " << e.what();
                openLog(" 插入表异常 ");
            }
        }


    }
}
void AlgorithmLogic::ReadProxyInfo()
{
    try
    {
        proxyinfoMap.clear();
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["platform_proxy"];
        mongocxx::cursor cursor = coll.find({});// (query_value.view());li
        for(auto& cur:cursor)
        {
            if(cur["agentid"]&&cur["brokerage"]&&cur["minvalidbet"]&&cur["maxvalidbet"])
            {
                int32_t proxyid = cur["agentid"].get_int32();
                StrProxyInfo proxy;
                proxy.brokerage = cur["brokerage"].get_int32();
                proxy.level = cur["level"].get_int32();
                proxy.minvalidbet = cur["minvalidbet"].get_int64();
                proxy.maxvalidbet = cur["maxvalidbet"].get_int64();
                if(proxyinfoMap.find(proxyid)!=proxyinfoMap.end())
                {
                    proxyinfoMap[proxyid].push_back(proxy);
                }
                else
                {
                    vector<StrProxyInfo>  vecpro;
                    vecpro.clear();
                    vecpro.push_back(proxy);
                    proxyinfoMap[proxyid]=vecpro;
                }
            }
        }
        proxyEffectivevct.clear();
        mongocxx::collection coll1 = MONGODBCLIENT["gamemain"]["proxy_info"];
        mongocxx::cursor cur1 = coll1.find({});// (query_value.view());li
        for(auto& cur:cur1)
        {
            if(cur["merchant"])
            {
                //属于平台号
                int32_t merchant = cur["merchant"].get_int32();
                if(merchant==2)
                {
                    int32_t proxyid= cur["agentid"].get_int32();
                    proxyEffectivevct.push_back(proxyid);
                }
            }
        }
    }
    catch(exception &e)
    {
        LOG_ERROR << "读取代理表异常:"<<e.what();
        openLog(" 读取代理表异常 ");
    }
}
//重新设定定时器
void AlgorithmLogic::StartGameNestTime()
{
    //所有程序执行完，设置重新启动的时间
    StartTimeTnterval = GetStartTimeSeconds();

    timelooper->getLoop()->cancel(StartProgramTime);
    openLog("下次启动的时间是  %d  秒以后",StartTimeTnterval);
    StartProgramTime=timelooper->getLoop()->runAfter(StartTimeTnterval,bind(&AlgorithmLogic::FucTimer,this));

}

void AlgorithmLogic::StartTimeAndEndTime()
{
    time_t tx = time(NULL); //获取目前秒时间


    struct tm * local;
    if(isOpenAlltime)
    {

        m_EndDate = { 0 };
        m_EndDate.tm_year = m_iyear-1900 ;
        m_EndDate.tm_mon =  m_imonth-1;
        m_EndDate.tm_mday = m_iday;
        m_EndDate.tm_hour = 0;
        m_EndDate.tm_min = 0;
        m_EndDate.tm_sec = 0;
    }
    else
    {

        m_EndDate = { 0 };

        local = localtime(&tx); //转为本地时间，注意，该函数非线程安全，下面的例子会使用线程安全函数localtime_r

        m_EndDate.tm_year = local->tm_year ;
        m_EndDate.tm_mon =  local->tm_mon;
        m_EndDate.tm_mday = local->tm_mday;
        m_EndDate.tm_hour = 0;
        m_EndDate.tm_min = 0;
        m_EndDate.tm_sec = 0;
    }



    openLog("当前时间搓 %d  日期年 =%d  ,  月 =%d  ,  日 =%d",tx,m_EndDate.tm_year+1900,m_EndDate.tm_mon+1,m_EndDate.tm_mday);
}
void AlgorithmLogic::FucTimer()
{
    startPoint = time(NULL);

    if(isOpenDelete)
    {
        m_StartDate = { 0 };
        m_StartDate.tm_year = m_dyear-1900 ;
        m_StartDate.tm_mon =  m_dmonth-1;
        m_StartDate.tm_mday = m_dday;
        m_StartDate.tm_hour = 0;
        m_StartDate.tm_min = 0;
        m_StartDate.tm_sec = 0;
        DeleteUserInsertList();
    }
    if(onlyDelete)
    {
        return ;
    }
    ReadProxyInfo();


    //当天所有新注册的玩家统计
    NewRegisteredUser();
    //有直属新增玩家或者团队新增玩家的玩家列表统计
    HaveAddNewRegisteredUser();
    //计算出要统计的注单的时间段
    StartTimeAndEndTime();

    //汇总玩家游戏注单
    AggregatePlayerBets();
    //计算产生业绩的玩家的团队业绩以及个人业绩
    CalculatAllBenefitsUser();
    //插入玩家业绩表
    InsertUserProfitTable();


    //周分红处理
    WeekTotalFunction();



    //重新设置第二天准时启动计划任务
    StartGameNestTime();

    endPoint = time(NULL);
    openLog("跑程序耗费的时间秒数是： = %d ",endPoint-startPoint);
}
//删除设定时间段的注单,时间是设定的时间以及往前推的时间
void AlgorithmLogic::DeleteUserInsertList()
{
    try
    {
        //删除当天的这个表的数据
        mongocxx::collection collx = MONGODBCLIENT["gamemain"]["play_commission"];
        if(!collx) return ;
        auto query_value = document{} << "profitinserttime" <<open_document<<
                                         "$gte" <<bsoncxx::types::b_date(getTimePoint(m_StartDate)+std::chrono::hours(jetLag)-std::chrono::hours(m_deleteDistanceTime))<<
                                         "$lt" <<bsoncxx::types::b_date(getTimePoint(m_StartDate)+std::chrono::hours(jetLag))<<
                                         close_document<<finalize;
         bsoncxx::stdx::optional<mongocxx::result::delete_result> result =
         collx.delete_many(query_value.view());
    }
    catch(exception & e)
    {
         openLog(" 删除报异常 ");
    }

}
void AlgorithmLogic::ThreadInitCallback()
{

}
void  AlgorithmLogic::openLog(const char *p, ...)
{
    //if (m_IsEnableOpenlog)
    {

        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/task/task_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}
bool  AlgorithmLogic::OnMakeDir()
{
    std::string dir="./log";
    if(access(dir.c_str(), 0) == -1)
    {
        if(mkdir(dir.c_str(), 0755)== 0)
        {
            dir="./log/task";

            if(access(dir.c_str(), 0) == -1)
            {
                return (mkdir(dir.c_str(), 0755)== 0);
            }
        }
    }

    return false;
}
void AlgorithmLogic::ReadPlayDB()
{
    //struct timeval t;

    //gettimeofday(&t, NULL);
    //chrono::system_clock::from_time_t(t);
    //bsoncxx::types::b_date(ISODate("2016-01-24T12:52:33.341Z").valueOf()) ;

    //::system_clock::time_point mtt;
     //auto maybe_result = coll.find({});
    //ISODate("2019-08-23T18:51:41.963Z");
    //const char * filiter = "{\"gameendtime\": {\"$lte\": \"60\", \"$gte\": \"100\"}, \"course\": \"math\"}";
    //const boost::posix_time::ptime time_with_ms = boost::date_time::parse_delimited_time<boost::posix_time::ptime>("2013-05-31T09:00:00.123", 'T');
    // auto filter = bsoncxx::builder::stream::document{} << "i" << bsoncxx::builder::stream::open_document <<
     // 			"$gt" << 50 << "$lte" << 100 << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize;
    // {"gameendtime" : { "$gte" : ISODate("2019-08-23T18:51:41.963Z")
    //    , "$lt" : ISODate("2019-08-23T18:51:59.585Z") }}
    //db.CollectionAAA.find({ "CreateTime" : { "$gte" : ISODate("2017-04-20T00:00:00Z")
    //, "$lt" : ISODate("2017-04-21T00:00:00Z") } })
    //mongocxx::cursor
    //    time_t shijianlong1 = mktime(&t1);
    //    time_t shijianlong2 = mktime(&t2);

    //    time_point<system_clock> then_tp1 = system_clock::from_time_t(shijianlong1);
    //    time_point<system_clock> then_tp2 = system_clock::from_time_t(shijianlong2);

    //    //int64_t ceshi = then_tp1.time_since_epoch().count;

    //    time_t shijianlongx1 = bsoncxx::types::b_date(then_tp1+std::chrono::hours(8));
    //    time_t shijianlongx2 = bsoncxx::types::b_date(then_tp2+std::chrono::hours(8));

    //    time_t shijianlongx3 = bsoncxx::types::b_date(chrono::system_clock::now());
    //    //chrono::system_clock::to_time_t()
//    int64_t userId = 10003;

//    mongocxx::options::find opts = mongocxx::options::find{};
//    opts.projection(document{} << "userid" << 1 << "account" << 1 << "agentid" << 1 << "headindex" << 1 << "nickname" << 1 << "txhstatus" << 1 << "score" << 1 << "status" << 1 << "integralvalue" << 1 << "lastloginip" << 1 << "lastlogintime" << 1 << finalize);

//    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
//    auto query_value = document{} << "userid" << userId << finalize;



//   // auto filter = bsoncxx::builder::stream::document{} << "i" << bsoncxx::builder::stream::open_document <<
//    // 			"$gt" << 50 << "$lte" << 100 << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize;
//   // {"gameendtime" : { "$gte" : ISODate("2019-08-23T18:51:41.963Z")
//   //    , "$lt" : ISODate("2019-08-23T18:51:59.585Z") }}

//    auto maybe_result = coll.find_one(query_value.view(),opts);


//    int64_t dbUserId=0;
//    string dbAccount="";
//    int32_t dbAgentId=0;
//    int32_t dbHeadId=0;
//    string  dbNickName="";
//    int64_t dbScore=0;
//    int32_t dbstatus = 0;
//    int32_t dbtxhstatus = 0;
//    if( maybe_result )
//    {
//        bsoncxx::document::view view = maybe_result->view();
//        dbUserId    = view["userid"].get_int64();
//        dbAccount   = view["account"].get_utf8().value.to_string();
//        dbAgentId   = view["agentid"].get_int32();
//        dbHeadId    = view["headindex"].get_int32();
//        dbNickName  = view["nickname"].get_utf8().value.to_string();
//        dbScore     = view["score"].get_int64();
//        dbstatus    = view["status"].get_int32();
//        dbtxhstatus = view["txhstatus"].get_int32();
//    }
}

void AlgorithmLogic::WeekTotalFunction()
{
    //因为定时器是每天跑一次，所以判断是周一再进入
    time_t tx = time(NULL);
    struct tm * local;
    local = localtime(&tx);

    todayHighest=getZeroTimeStamp(local);

    //周一零时零分零秒
    weekHighest =todayHighest-(local->tm_wday-1)*24*60*60;
    //是星期一就执行下面的,调试期间去掉
    if(local->tm_wday!=1)
    {
        return;
    }
    try
    {
        //删除当天的这个表的数据
        mongocxx::collection collx = MONGODBCLIENT["gamemain"]["user_dividends"];
        if(!collx) return ;
        auto query_value = document{} << "inserttime" <<open_document<<
                                         "$gte" <<bsoncxx::types::b_date(system_clock::from_time_t(weekHighest))<<
                                         "$lt" <<bsoncxx::types::b_date(system_clock::from_time_t(weekHighest)+std::chrono::hours(7*24))<<
                                         close_document<<finalize;
         bsoncxx::stdx::optional<mongocxx::result::delete_result> result =
         collx.delete_many(query_value.view());
    }
    catch(exception & e)
    {
         openLog(" 删除报异常 ");
    }


    //读取周分红配置表
    ReadWeekInformation();
    //注意要分代理存配置
    ReadWeekCondiction();
    //汇总这周的玩家注单表
    WeekDividends();
    //
    weekCalculate();
}
//周分红代理配置
void AlgorithmLogic::ReadWeekCondiction()
{
    try
    {
        //读取周配置并且排序
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["platform_dividend_level"];
        mongocxx::cursor cur = coll.find({});// (query_value.view());li
        weekCondictionMap.clear();
        for(auto& cu:cur)
        {
            WeekDividendCondition cdtion;
            cdtion.proxyid = cu["agentid"].get_int32();
            cdtion.level = cu["level"].get_int32();
            cdtion.share = cu["share"].get_int32();
            cdtion.agentname = cu["agentname"].get_utf8().value.to_string();
            cdtion.minPerformance = cu["minPerformance"].get_int64();
            cdtion.maxPerformance = cu["maxPerformance"].get_int64();

            if(weekCondictionMap.find(cdtion.proxyid)!=weekCondictionMap.end())
            {
                weekCondictionMap[cdtion.proxyid].push_back(cdtion);
            }
            else
            {
                vector<WeekDividendCondition>  vecpro;
                vecpro.clear();
                vecpro.push_back(cdtion);
                weekCondictionMap[cdtion.proxyid]=vecpro;
            }

        }
        //排序
         for(auto &condition:weekCondictionMap)
         {
             for(int i=0;i<condition.second.size();i++)
             {
                 for(int j=i+1;j<condition.second.size();j++)
                 {
                     if(condition.second[j].level<condition.second[i].level)
                     {
                         WeekDividendCondition cdti;
                         cdti=condition.second[i];
                         condition.second[i] = condition.second[j];
                         condition.second[j] =cdti;
                     }
                 }
             }
         }

    }
    catch(exception &e)
    {

    }


}
void AlgorithmLogic::ReadWeekInformation()
{
    try
    {
        //读取周配置并且排序
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["platform_dividend_config"];
        mongocxx::cursor cur = coll.find({});// (query_value.view());li
        OverallCondition.clear();
        for(auto& cu:cur)
        {
            WeekInformation cdtion;
            cdtion.agentid = cu["agentid"].get_int32();
            cdtion.minperformance = cu["minPerformance"].get_int64();
            cdtion.dividendScale = cu["dividendScale"].get_double();

            OverallCondition[cdtion.agentid]=cdtion;

        }
    }
    catch(exception &e)
    {

    }
}
void AlgorithmLogic::WeekDividends()
{
    try
    {
//        mongocxx::options::find opts = mongocxx::options::find{};
//        //opts.projection(document{} << "userid" << 1 << "account" << 1 << "agentid" << 1 << "gameid" << 1 << finalize);
//        opts.projection(bsoncxx::builder::stream::document{}<<"isandroid"<<false<< bsoncxx::builder::stream::finalize);
//        opts.sort(document{} << "gameendtime" << -1 << finalize); //此处后期可以调整


        //通过时间戳，读取玩家日汇总表，然后汇总成周的表,传两个时间，一个周开始，一个周结束
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["report_dailyplayer"];
        auto query_value = document{} << "StatDate" <<open_document<<
                                         "$gte" <<weekHighest-7*24*60*60<<
                                         "$lt" <<weekHighest<<
                                         close_document<<finalize;

        auto maybe_result = coll.find(query_value.view());




        int64_t dbUserId=0;
        int64_t dbBetScore =0;
        int64_t dbRevent = 0;
        int32_t dbAgentId = 0;
        int64_t dbWinScore = 0;

        userWeekInfo.clear();

        int bettingNum =0;
        for(auto &view :  maybe_result)
        {


            //::document::view xxxx = elem.view();


            //dbAccount   = view["gameinfoid"].get_utf8().value.to_string();
            dbUserId    = view["UserId"].get_int64();
            dbBetScore  = view["TotalValidBet"].get_int64();
            dbRevent    = view["TotalRevenue"].get_int64();
            dbAgentId   = view["AgentId"].get_int32();
            dbWinScore  = view["TotalWinLose"].get_int64();


             openLog("serid =%d 本周玩家业绩 = %d  u",dbUserId,dbBetScore);
            //出去以前的代理号
            if(find(proxyEffectivevct.begin(),proxyEffectivevct.end(),dbAgentId)!=proxyEffectivevct.end())
            {

            }
            else
            {
                continue;
            }

            if(userWeekInfo.find(dbUserId)!=userWeekInfo.end())
            {
                userWeekInfo[dbUserId].userid = dbUserId;
                userWeekInfo[dbUserId].agentid = dbAgentId;
                //税收汇总
                userWeekInfo[dbUserId].totalrevenue +=dbRevent;
                //投注量汇总
                userWeekInfo[dbUserId].totalvalidbet +=dbBetScore;
                userWeekInfo[dbUserId].totalwinlose += dbWinScore;
            }
            else
            {
                bettingNum++;
                userWeekDividendInfo userInfo;
                userInfo.userid = dbUserId;
                userInfo.agentid = dbAgentId;
                userInfo.totalrevenue = dbRevent;
                userInfo.totalvalidbet = dbBetScore;
                userInfo.totalwinlose = dbWinScore;
                mongocxx::options::find opts1 = mongocxx::options::find{};
                mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                auto query_user = document{} << "userid" << dbUserId << finalize;
                // LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
                bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                if(results)
                {
                     //添加玩家所有上级id
                     bsoncxx::document::view view = results->view();

                     if(view["superior"])
                     {
                         int64_t superiorid = view["superior"].get_int64();
                         if(view["subagents"]&&superiorid!=0)
                         {
                             auto arr = view["subagents"].get_array();
                             for(auto user:arr.value)
                             {
                                 int64_t upuser=user.get_int64();
                                 userInfo.superiorAgent.push_back(upuser);
                             }
                         }

                     }
                }
                else
                {
                    //
                    openLog("no   up user  userid = %d",dbUserId);
                }

                userWeekInfo[dbUserId] = userInfo;
            }
        }

     }
    catch(exception &e)
    {

    }

}
//
void AlgorithmLogic::weekCalculate()
{
        userResultInfo.clear();
        for(auto &betuser:userWeekInfo)
        {

            if(OverallCondition.find(betuser.second.agentid)!=OverallCondition.end())
            {
                //累计平台周总盈利，因为是玩家的盈利所减
                OverallCondition[betuser.second.agentid].AllProfit-=betuser.second.totalwinlose;
                OverallCondition[betuser.second.agentid].AllBeting+=betuser.second.totalvalidbet;
            }
            if(userResultInfo.find(betuser.second.userid)!=userResultInfo.end())
            {
                userResultInfo[betuser.second.userid].selfBetting = betuser.second.totalvalidbet;
            }
            else
            {
                StrUserWeekInfo benifiuser;
                benifiuser.userid = betuser.second.userid;
                benifiuser.agentid = betuser.second.agentid;
                benifiuser.selfBetting = betuser.second.totalvalidbet;


                mongocxx::options::find opts1 = mongocxx::options::find{};
                mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
                auto query_user = document{} << "userid" << benifiuser.userid  << finalize;
                bsoncxx::stdx::optional<bsoncxx::document::value> results = collUser.find_one(query_user.view(),opts1);
                if(results)
                {
                    //添加玩家所有上级id,查询玩家保底值
                    bsoncxx::document::view view = results->view();
                    if(view["superior"])
                    {
                        benifiuser.upUserid = view["superior"].get_int64();
                        benifiuser.nikeName = view["account"].get_utf8().value.to_string();

                    }

                }
                userResultInfo[benifiuser.userid] = benifiuser;
            }

            for(int i=0;i<betuser.second.superiorAgent.size();i++)
            {
                //给他上级玩家的团队业绩累加
                if(userResultInfo.end()!=userResultInfo.find(betuser.second.superiorAgent[i]))
                {
                    userResultInfo[betuser.second.superiorAgent[i]].teamBetting+=betuser.second.totalvalidbet;
                }
                else
                {
                    StrUserWeekInfo benifinew;
                    benifinew.userid = betuser.second.superiorAgent[i];
                    benifinew.agentid = betuser.second.agentid;
                    benifinew.teamBetting = betuser.second.totalvalidbet;                  
                    userResultInfo[betuser.second.superiorAgent[i]] = benifinew;
                }
            }

        }



        //下级玩家数累加
        for(auto &benifiuser:userResultInfo)
        {

            mongocxx::collection collUser = MONGODBCLIENT["gamemain"]["game_user"];
            auto query_user = document{} << "superior" << benifiuser.second.userid  << finalize;
            auto maybe_result = collUser.find(query_user.view());

            for(auto &view :  maybe_result)
            {
                int64_t userid = view["userid"].get_int64();
                benifiuser.second.belowUser.push_back(userid);
            }

        }
        for(auto &benifiuser:userResultInfo)
        {
            int64_t allbetting = benifiuser.second.teamBetting+benifiuser.second.selfBetting;
            //小于三十万，或者下级玩家少于两个
            if(OverallCondition.find(benifiuser.second.agentid)!=OverallCondition.end())
            {

            }
            else
            {
                continue;
            }
            if(allbetting<OverallCondition[benifiuser.second.agentid].minperformance||benifiuser.second.belowUser.size()<=1)
            {
                openLog("不符合条件的玩家 = %d          allbetting=%d   usercount = %d ",benifiuser.second.userid,allbetting,benifiuser.second.belowUser.size());
                continue;
            }
            //判断等级
            int32_t numPerW = 0;
            if(weekCondictionMap.find(benifiuser.second.agentid)!=weekCondictionMap.end())
            {
                for(int i=0;i<weekCondictionMap[benifiuser.second.agentid].size();i++)
                {
                    if(weekCondictionMap[benifiuser.second.agentid][i].minPerformance>allbetting||(allbetting>=weekCondictionMap[benifiuser.second.agentid][i].maxPerformance&&i!=weekCondictionMap[benifiuser.second.agentid].size()-1))
                    {

                        continue;
                    }
                    else
                    {
                        numPerW = weekCondictionMap[benifiuser.second.agentid][i].share;
                        benifiuser.second.level = weekCondictionMap[benifiuser.second.agentid][i].level;
                        benifiuser.second.teamShares = (numPerW*allbetting)/10000;
                        benifiuser.second.selfShares = benifiuser.second.teamShares;
                    }
                }
            }
            else
            {
                openLog("不符合条件的玩家 = %d          找不到本代理配置",benifiuser.second.userid);
                continue;
            }

        }

        for(auto &benifiuser:userResultInfo)
        {

            for(auto &User:benifiuser.second.belowUser)
            {
                if(userResultInfo.find(User)!=userResultInfo.end())
                {
                    benifiuser.second.selfShares-= userResultInfo[User].teamShares;
                }
            }
        }
        for(auto &benifiuser:userResultInfo)
        {
            int64_t allbetting = benifiuser.second.teamBetting+benifiuser.second.selfBetting;
            int64_t condition = 10000000000;
            if(OverallCondition.find(benifiuser.second.agentid)!=OverallCondition.end())
               condition = OverallCondition[benifiuser.second.agentid].minperformance;

            if(benifiuser.second.belowUser.size()<=1||allbetting<condition||benifiuser.second.selfShares<0)
            {
                benifiuser.second.selfShares = 0;
            }
            //累计平台总的分红份额
            if(OverallCondition.find(benifiuser.second.agentid)!=OverallCondition.end())
            {
                OverallCondition[benifiuser.second.agentid].AllShares+=benifiuser.second.selfShares;
            }
        }
        //此处顺序不可调
        for(auto &con:OverallCondition)
        {
            con.second.AllDividendsProfit=con.second.AllProfit*con.second.dividendScale/100;
            if(con.second.AllShares>0)
            {
                con.second.moneyPerShare =(double)con.second.AllDividendsProfit / (double)con.second.AllShares;

                int64_t quchu = con.second.moneyPerShare*100;
                con.second.moneyPerShare = quchu/100.0;
            }
        }


        for(auto &benifiuser:userResultInfo)
        {
            int64_t allbetting = benifiuser.second.teamBetting+benifiuser.second.selfBetting;
            int64_t condition = 10000000000;
            if(OverallCondition.find(benifiuser.second.agentid)!=OverallCondition.end())
               condition = OverallCondition[benifiuser.second.agentid].minperformance;

            if(benifiuser.second.belowUser.size()<=1||allbetting<condition||benifiuser.second.selfShares<0)
            {
                benifiuser.second.selfShares = 0;
                openLog("不符合条件的玩家 = %d          allbetting=%d   usercount = %d     selfshares = %d",benifiuser.second.userid,allbetting,benifiuser.second.belowUser.size(),benifiuser.second.selfShares);
                continue;
            }
            //计算玩家获得的分红数
            if(OverallCondition.find(benifiuser.second.agentid)!=OverallCondition.end())
            {
                benifiuser.second.moneyPerShare = OverallCondition[benifiuser.second.agentid].moneyPerShare;
                benifiuser.second.userProfit = benifiuser.second.moneyPerShare*benifiuser.second.selfShares ;
                benifiuser.second.lastAllBetting =OverallCondition[benifiuser.second.agentid].AllBeting;
                benifiuser.second.allDividends =OverallCondition[benifiuser.second.agentid].AllDividendsProfit;
                //写入分红表 user_dividends

                try
                {
                    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_dividends"];
                    auto cursorpz = coll.find_one(document{} <<"userid"<<benifiuser.second.userid<< "inserttime" <<open_document<<
                                                  "$gte" <<bsoncxx::types::b_date(system_clock::from_time_t(weekHighest))<<
                                                  "$lt" <<bsoncxx::types::b_date(system_clock::from_time_t(weekHighest)+std::chrono::hours(7*24))<<
                                                  close_document<<finalize);// (query_value.view());li

                    if(!cursorpz)
                    {

                        //selfBetting;    //自营业绩
                        //teamBetting;    //团队业绩
                        //agentid;        //代理id
                        //userid;         //玩家id
                        //upUserid;       //直接上级id
                        //teamShares;     //团队分红份额，每w多少份*多少w =分红份额
                        //selfShares;     //自己的分红份额=团队分红份额-下级分红份额
                        //userProfit;     //本周获得的分红钱数
                        //moneyPerShare;  //每份股份的钱数
                        //nikeName        //昵称
                        //level           //等级
                        //lastAllBetting  //平台总业绩
                        //allDividends    //平台总分红钱数
                        //dateTime        //上周一零分零秒时间戳
                        bsoncxx::builder::stream::document builder{};
                        auto insert_value = builder
                        << "userid" << (int64_t)benifiuser.second.userid
                        << "agentid" << benifiuser.second.agentid
                        << "lastAllBetting" <<benifiuser.second.lastAllBetting
                        << "selfBetting" << benifiuser.second.selfBetting
                        << "teamBetting" << benifiuser.second.teamBetting
                        << "teamShares" << benifiuser.second.teamShares
                        << "selfShares" << benifiuser.second.selfShares
                        << "userProfit" << benifiuser.second.userProfit
                        << "moneyPerShare"  <<benifiuser.second.moneyPerShare
                        << "nikeName"  <<benifiuser.second.nikeName
                        << "level"  <<benifiuser.second.level
                        << "allDividends"  <<benifiuser.second.allDividends
                        << "dateTime" <<(int64_t)(weekHighest-7*24*60*60)
                        << "inserttime"   << bsoncxx::types::b_date(chrono::system_clock::now())
                        << bsoncxx::builder::stream::finalize;
                        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
                        if (!result)
                        {
                            LOG_ERROR << "play_commission insert exception: ";
                        }

                    }



                }
                catch (const std::exception &e)
                {
                    LOG_ERROR <<" MongoDB exception: " << e.what();
                }
            }
        }
        ///到这里已经计算完了玩家的分红份额
}
