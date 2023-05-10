
#include <boost/format.hpp>
#include <muduo/base/Logging.h>
#include "PlayLog.h"
#include <glog/logging.h>

PlayLog::PlayLog()
{
    m_mPaoInfo.clear();
}

PlayLog::~PlayLog()
{

}

//创建结果
// @paoValues  炮值，@fishId鱼typeId，@odds 鱼的倍数，@nWinScore打死这条鱼的得分
int PlayLog::AddHitRecord(int32_t paoValues,int32_t fishId,int32_t odds,int32_t nWinScore, bool bCatchSpecialFireInGlobalBomb)
{ 
    if(fishId >= MAX_FISH_COUNT){  LOG_ERROR << "---鱼ID太大---"<<fishId; return -1;}

     LOG(ERROR) << paoValues << " "<< fishId <<" "<< odds <<" "<< nWinScore;
    // 查找
    auto iter = m_mPaoInfo.find(paoValues);
    if ( iter != m_mPaoInfo.end() ){
		if (!bCatchSpecialFireInGlobalBomb)
		{
			iter->second.fireCount++;
		}
        iter->second.allWin += nWinScore;

        if(odds==0)
        {

        }
        else
        {
            auto iter1 = iter->second.hitFsInfo.find(odds);
            if(iter1 != iter->second.hitFsInfo.end())
            {
                iter->second.hitFsInfo[odds].count++;
                iter->second.hitFsInfo[odds].win += nWinScore;
                iter->second.hitFsInfo[odds].fishId = fishId;
                iter->second.hitFsInfo[odds].odds = (nWinScore > 0)?odds:0;
            }
            else
            {
                HitFishInfo _paoInfo = {0};
                // 统计
                _paoInfo.count = 1;
                _paoInfo.win = nWinScore;
                _paoInfo.fishId = fishId;
                _paoInfo.odds=odds;
                iter->second.hitFsInfo.insert(make_pair(odds, _paoInfo));
            }
        }

//        if(nWinScore > 0)
//        {
//            iter->second.hitFsInfo[fishId].count++;
//            iter->second.hitFsInfo[fishId].win += nWinScore;
//            iter->second.hitFsInfo[fishId].fishId = fishId;
//        }
        //
//        iter->second.hitFsInfo[fishId].odds = (nWinScore > 0)?odds:0;
        // LOG_WARN << " 已经存储有鱼信息 "<< m_mPaoInfo.size();
        return 0;
    }
    else{
        LOG_WARN << " 新存储炮值 "<< paoValues << " " << m_mPaoInfo.size();
        // 赋值
        PaoInfo _paoInfo = {0};
        // 统计
        _paoInfo.fireCount = 1;
        _paoInfo.paoValues = paoValues;
        _paoInfo.allWin = nWinScore;

        HitFishInfo paoInfo = {0};
        // 统计
        paoInfo.count ++;
        paoInfo.win = nWinScore;
        paoInfo.fishId = fishId;
        paoInfo.odds=odds;
        int index=0;
        if(odds!=0)
        {
            index=odds;
            _paoInfo.hitFsInfo.insert(make_pair(index, paoInfo));
        }

        // 添加映射
        m_mPaoInfo.insert(make_pair(paoValues, _paoInfo));
    }

	LOG(ERROR) << " 存储 "<< paoValues << " " << m_mPaoInfo.size();

    return 0;
}

// 获取结果字符串
// @recoreStr 传死
int PlayLog::GetRecordStr(string & recoreStr)
{ 
    if(m_mPaoInfo.size() == 0){return -1;}
    // 清空
    recoreStr.clear();
    PaoInfo _paoInfo = {0};
    int32_t paoValues = 0;
    int32_t curIdx = 0;
    int32_t allSize = m_mPaoInfo.size();
    
    // LOG_WARN << " ------GetRecordStr----- "<< allSize;
    // 
    for(auto iter = m_mPaoInfo.begin(); iter != m_mPaoInfo.end(); iter++){
        paoValues    = iter->first;
        _paoInfo = iter->second;

        // 组之间符号%
        if(curIdx > 0){ recoreStr += "$"; }
        curIdx++;
        // 炮值，发数，总赢分
        recoreStr += str(boost::format("%d,%d,%d%s") % _paoInfo.paoValues % _paoInfo.fireCount % _paoInfo.allWin % ":");
        // LOG_WARN << " ------recoreStr----- "<< recoreStr;

        int32_t curFsIdx = 0;
        // 组装鱼
        HitFishInfo fsInfo = {0};
        for (size_t i = 0; i < _paoInfo.hitFsInfo.size(); i++)
        {
            fsInfo = _paoInfo.hitFsInfo[i];
            
            // 如果不存在这条鱼则下一条
            if( fsInfo.odds == 0 ) continue; 

            if(curFsIdx > 0){ recoreStr += "|";}
            // 组装鱼
            recoreStr += str(boost::format("%d,%d,%d,%d") % fsInfo.fishId % fsInfo.odds % fsInfo.count % fsInfo.win);

            // LOG_INFO << " make fish: "<< strtmp;
            // 统计
            curFsIdx++;
        } 
    }

    LOG_WARN << " 清除记录 "<< m_mPaoInfo.size() << " recoreStr length:"<< recoreStr.length();
    // 清除记录
    m_mPaoInfo.clear();

    return 0;
}
 
