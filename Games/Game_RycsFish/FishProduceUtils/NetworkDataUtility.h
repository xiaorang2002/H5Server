#ifndef __NetworkDataUtilitySimple_h__
#define __NetworkDataUtilitySimple_h__
#ifndef bit8
#define bit8 char
#endif
#ifndef bit16
#define bit16 short
#endif
#ifndef bit32
#define bit32 int
#endif
#ifndef bit64
#define bit64 long long
#endif

#include <glog/logging.h>

#include "CMD_Fish.h"
#include "FishProduceData.h"
#include "FishProduceAllocator.h"
#include "FishBasicDataManager.h"
#include "FishShoalInfoManager.h"
#include "FishSwimmingPatternManager.h"
#include <vector>
#include <map>
//#include "../fish_server/stdafx.h"

#include "rycsfish.Message.pb.h"

#define NetworkUtility	NetworkDataUtilitySimple

#define RBITS(x, num) ((x) & ((1 << (num)) - 1))

extern void log_fishes(const char* fmt,...);

class NetworkDataUtilitySimple
{
public:
    static unsigned short encodeFishBasicData(const FishBasicData& cmdData,
                                              ::rycsfish::FishBasicData* pBasicData)
    {
        /*
#ifdef __USE_LIFE2__
        const unsigned short DATA_SIZE = 14;
#else
        const unsigned short DATA_SIZE = 10;
#endif
        if (bufferSize < DATA_SIZE) {
            return 0;
        }
        // FishType fishType; type-5 king-4
        // unsigned short lifeMin; 12
        // unsigned short lifeMax; 12
        // unsigned short boundingWidth; 11
        // unsigned short boundingHeight; 11
        // unsigned short fishName[200]; 0
        memcpy(buffer, &cmdData.fishType.type, 1);
        memcpy(buffer + 1, &cmdData.fishType.king, 1);
        memcpy(buffer + 2, &cmdData.lifeMin, 2);
        memcpy(buffer + 4, &cmdData.lifeMax, 2);
        memcpy(buffer + 6, &cmdData.boundingWidth, 2);
        memcpy(buffer + 8, &cmdData.boundingHeight, 2);
#ifdef __USE_LIFE2__
        memcpy(buffer + 10, &cmdData.life2Min, 2);
        memcpy(buffer + 12, &cmdData.life2Max, 2);
#endif
        return DATA_SIZE;
        */

        int bytesize = 0;
        do
        {
            if (!pBasicData) break;
            // try to serial the special data to the protobuf content.
            ::rycsfish::FishType* fishtype = pBasicData->mutable_fishtype();
            fishtype->set_type(cmdData.fishType.type);
            fishtype->set_king(cmdData.fishType.king);

            pBasicData->set_lifemin(cmdData.lifeMin);
            pBasicData->set_lifemax(cmdData.lifeMax);
    #ifdef __USE_LIFE2__
            pBasicData->set_life2min(cmdData.life2Min);
            pBasicData->set_life2max(cmdData.life2Max);
    #endif

            pBasicData->set_boundingwidth(cmdData.boundingWidth);
            pBasicData->set_boundingheight(cmdData.boundingHeight);
            // serial the special data buffer.
            bytesize = pBasicData->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeFishBasicData(FishBasicData& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
#ifdef __USE_LIFE2__
        const unsigned short DATA_SIZE = 14;
#else
        const unsigned short DATA_SIZE = 10;
#endif
        if (bufferSize < DATA_SIZE) {
            return false;
        }
        // FishType fishType; type-5 king-4
        // unsigned short lifeMin; 12
        // unsigned short lifeMax; 12
        // unsigned short boundingWidth; 11
        // unsigned short boundingHeight; 11
        // unsigned short fishName[200]; 0
        cmdData.fishType.type = *buffer;
        cmdData.fishType.king = *(buffer + 1);
        cmdData.lifeMin = *(bit16*)(buffer + 2);
        cmdData.lifeMax = *(bit16*)(buffer + 4);
        cmdData.boundingWidth = *(bit16*)(buffer + 6);
        cmdData.boundingHeight = *(bit16*)(buffer + 8);
#ifdef __USE_LIFE2__
        cmdData.life2Min = *(bit16*)(buffer + 10);
        cmdData.life2Max = *(bit16*)(buffer + 12);
#endif
        if (dataSize) {
            *dataSize = DATA_SIZE;
        }

        return true;
    } */


    static unsigned short encodeFishShoalInfo(const FishShoalInfo& cmdData,
                                              ::rycsfish::FishShoalInfo* pShoalInfo)
    {
        /*
        if (bufferSize < 7)
        {
            return 0;
        }
        // FishType fishType; type-5 king-4
        // unsigned short genRatio; 7
        // unsigned short amountMin; 5
        // unsigned short amountMax; 5
        // char allowFollow; // ���� 1
        // char allowLump; // ��Ⱥ 1
        memcpy(buffer, &cmdData.fishType.type, 1);
        memcpy(buffer + 1, &cmdData.fishType.king, 1);
        memcpy(buffer + 2, &cmdData.genRatio, 1);
        memcpy(buffer + 3, &cmdData.amountMin, 1);
        memcpy(buffer + 4, &cmdData.amountMax, 1);
        memcpy(buffer + 5, &cmdData.allowFollow, 1);
        memcpy(buffer + 6, &cmdData.allowLump, 1);
        return 7;
        */

        int bytesize = 0;
        do
        {
            if (!pShoalInfo) break;
            // try to serial the special data to the protobuf content.
            ::rycsfish::FishType* fishtype = pShoalInfo->mutable_fishtype();
            fishtype->set_type(cmdData.fishType.type);
            fishtype->set_king(cmdData.fishType.king);

            pShoalInfo->set_genratio(cmdData.genRatio);
            pShoalInfo->set_amountmin(cmdData.amountMin);
            pShoalInfo->set_amountmax(cmdData.amountMax);

            pShoalInfo->set_allowfollow(cmdData.allowFollow);
            pShoalInfo->set_allowlump(cmdData.allowLump);

            // serial the special data buffer.
            bytesize = pShoalInfo->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeFishShoalInfo(FishShoalInfo& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 7) {
            return false;
        }
        // FishType fishType; type-5 king-4
        // unsigned short genRatio; 7
        // unsigned short amountMin; 5
        // unsigned short amountMax; 5
        // char allowFollow; // ���� 1
        // char allowLump; // ��Ⱥ 1
        cmdData.fishType.type = *buffer;
        cmdData.fishType.king = *(buffer + 1);
        cmdData.genRatio = *(buffer + 2);
        cmdData.amountMin = *(buffer + 3);
        cmdData.amountMax = *(buffer + 4);
        cmdData.allowFollow = *(buffer + 5);
        cmdData.allowLump = *(buffer + 6);
        if (dataSize) {
            *dataSize = 7;
        }

        return true;
    }
    */

    static unsigned short encodeFishSwimmingPattern(const FishSwimmingPattern& cmdData,
                                                    ::rycsfish::FishSwimmingPattern* pPattern)
    {
        /*
        unsigned short dataSize = 3;
        if (bufferSize < dataSize) {
            return 0;
        }
        // FishType fishType; type-5 king-4
        // unsigned char paramDataCount; 4
        // FishSwimmingParam swimmingParam[5];
        memcpy(buffer, &cmdData.fishType.type, 1);
        memcpy(buffer + 1, &cmdData.fishType.king, 1);
        memcpy(buffer + 2, &cmdData.paramDataCount, 1);
        for (unsigned char i = 0; i < cmdData.paramDataCount; ++i) {
            unsigned short tempSize = encodeFishSwimmingParam(buffer + dataSize, bufferSize - dataSize, cmdData.swimmingParam[i]);
            if (!tempSize) {
                return 0;
            }
            dataSize += tempSize;
        }

        return dataSize;
        */

        int bytesize = 0;
        do
        {
            if (!pPattern) break;
            // try to serial the special data to the protobuf content.
            ::rycsfish::FishType* fishtype = pPattern->mutable_fishtype();
            fishtype->set_type(cmdData.fishType.type);
            fishtype->set_king(cmdData.fishType.king);
            // initialize the content value data for user content.
            pPattern->set_paramdatacount(cmdData.paramDataCount);
            for (int idx=0;idx<cmdData.paramDataCount;idx++)
            {
                ::rycsfish::FishSwimmingParam* param = pPattern->add_paramitem();
                unsigned short tempsize = encodeFishSwimmingParam(cmdData.swimmingParam[idx],param);
                if (!tempsize) {
                    return 0;
                }
            }

            // serial the special data buffer.
            bytesize = pPattern->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);


    }

    /*
    static bool decodeFishSwimmingPattern(FishSwimmingPattern& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        unsigned short currentSize = 3;
        if (bufferSize < currentSize) {
            return false;
        }
        // FishType fishType; type-5 king-4
        // unsigned char paramDataCount; 4
        // FishSwimmingParam swimmingParam[5];
        cmdData.fishType.type= *buffer;
        cmdData.fishType.king= *(buffer + 1);
        cmdData.paramDataCount= *(buffer + 2);
        unsigned short tempSize;
        for (unsigned char i = 0; i < cmdData.paramDataCount; ++i) {
            if (!decodeFishSwimmingParam(cmdData.swimmingParam[i], buffer + currentSize, bufferSize - currentSize, &tempSize)) {
                return false;
            }
            currentSize += tempSize;
        }
        if (dataSize) {
            *dataSize = currentSize;
        }

        return true;
    }
    */

    static unsigned short encodeFishSwimmingParam(const FishSwimmingParam& cmdData,
                                                  ::rycsfish::FishSwimmingParam* pSwimParam)
    {
        /*
        // PathType pathType; 4
        // short speed; 10
        // unsigned short minLength; 10
        // unsigned short maxLength; 10
        // short minRotation; 9+1(����λ)
        // short maxRotation; 9+1(����λ)
        if (ptLine == cmdData.pathType) {
            if (bufferSize < 3) {
                return 0;
            }
            memcpy(buffer, &cmdData.pathType, 1);
            memcpy(buffer + 1, &cmdData.speed, 2);
            return 3;
        } else if (ptCurve == cmdData.pathType || ptEaseOut == cmdData.pathType) {
            if (bufferSize < 11) {
                return 0;
            }
            memcpy(buffer, &cmdData.pathType, 1);
            memcpy(buffer + 1, &cmdData.speed, 2);
            memcpy(buffer + 3, &cmdData.minLength, 2);
            memcpy(buffer + 5, &cmdData.maxLength, 2);
            memcpy(buffer + 7, &cmdData.minRotation, 2);
            memcpy(buffer + 9, &cmdData.maxRotation, 2);
            return 11;
        } else {
            return 0;
        }
        */

        int bytesize = 0;
        do
        {
            if (!pSwimParam) break;
            pSwimParam->set_pathtype(cmdData.pathType);
            switch (cmdData.pathType)
            {
            case ptLine:
                pSwimParam->set_pathtype(cmdData.pathType);
                pSwimParam->set_speed(cmdData.speed);
                break;
            case ptCurve:
            case ptEaseOut:
                pSwimParam->set_pathtype(cmdData.pathType);
                pSwimParam->set_speed(cmdData.speed); // speed.
                pSwimParam->set_minlength(cmdData.minLength);
                pSwimParam->set_maxlength(cmdData.maxLength);
                pSwimParam->set_minrotation(cmdData.minRotation);
                pSwimParam->set_maxrotation(cmdData.maxRotation);
                break;
            }

            // try to get the special byte size.
            bytesize = pSwimParam->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeFishSwimmingParam(FishSwimmingParam& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 1) {
            return false;
        }
        // PathType pathType; 4
        // short speed; 10
        // unsigned short minLength; 10
        // unsigned short maxLength; 10
        // short minRotation; 9+1(����λ)
        // short maxRotation; 9+1(����λ)
        cmdData.pathType = *buffer;
        if (ptLine == cmdData.pathType) {
            if (bufferSize < 3) {
                return false;
            }
            cmdData.speed= *(bit16*)(buffer + 1);
            if (dataSize) {
                *dataSize = 3;
            }
        } else if (ptCurve == cmdData.pathType || ptEaseOut == cmdData.pathType) {
            if (bufferSize < 11) {
                return false;
            }
            cmdData.speed = *(bit16*)(buffer + 1);
            cmdData.minLength = *(bit16*)(buffer + 3);
            cmdData.maxLength = *(bit16*)(buffer + 5);
            cmdData.minRotation = *(bit16*)(buffer + 7);
            cmdData.maxRotation = *(bit16*)(buffer + 9);
            if (dataSize) {
                *dataSize = 11;
            }
        } else {
            return false;
        }

        return true;
    }
    */

    static unsigned short encodeProduceFish(const FishProduceData& fishProduceData,
                                            ::rycsfish::FishProduceData* pProduceData)
    {
        /*
        if (bufferSize < 11) {
            return 0;
        }
        // unsigned int randSeed; 32
        // short elapsed; 16
        // FISH_ID fishID; 12
        // FishType fishType; type-5 king-4
        // unsigned char isAlive; 1
        short elapsed = 0;
        unsigned int randomSeed = fishProduceData.getRandomSeed();
        unsigned char isAlive = 0;
        if (fishProduceData.isAlive()) {
            isAlive = 1;
        }
        if (FishPathData* pathData = fishProduceData.getFishPathData()) {
            elapsed = static_cast<short>(pathData->getElapsed() * 100); // ��λ��0.01��
        }
        memcpy(buffer, &randomSeed, 4);
        memcpy(buffer + 4, &elapsed, 2);
        memcpy(buffer + 6, &fishProduceData.getFishKind().id, 2);
        memcpy(buffer + 8, &fishProduceData.getFishKind().fishType.type, 1);
        memcpy(buffer + 9, &fishProduceData.getFishKind().fishType.king, 1);
        memcpy(buffer + 10, &isAlive, 1);

        return 11;
        */

        int bytesize = 0;
        do
        {
            // check the pointer value.
            if (!pProduceData) break;
            // initialized.
            int elapsed = 0;
            int durtion =0;
            unsigned int randomSeed = fishProduceData.getRandomSeed();
            int isAlive = 0;
            if (fishProduceData.isAlive()) {
                isAlive = 1;
            }

            // try to set the special elapsed content value for later user.
            if (FishPathData* pathData = fishProduceData.getFishPathData())
            {
                elapsed = (pathData->getElapsed() * 100);
                durtion = pathData->getDuration() * 100;
            }

            // try to build the full data buffer content value for later.
            pProduceData->set_fishlive(fishProduceData.getFishKind().life);
            LOG(WARNING)<<"fishlife==="<<fishProduceData.getFishKind().life<<"  **********id==="<<fishProduceData.getFishKind().id;
            pProduceData->set_randomseed(fishProduceData.getRandomSeed());
            pProduceData->set_elapsed(elapsed);
            pProduceData->set_duration(durtion);
            pProduceData->set_fishid(fishProduceData.getFishKind().id);
            ::rycsfish::FishType* fishType = pProduceData->mutable_fishtype();
            fishType->set_type(fishProduceData.getFishKind().fishType.type);
            fishType->set_king(fishProduceData.getFishKind().fishType.king);
            pProduceData->set_isalive(isAlive);
            pProduceData->set_soalnum(fishProduceData.getLeaderFishNum());

            pProduceData->set_soaltype(fishProduceData.getShoalKind());

            // try to get the special buffer size.
            bytesize = pProduceData->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeProduceFish(CMD_S_ProduceFish& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 11) {
            return false;
        }
        // unsigned int randSeed; 32
        // short elapsed; 16
        // FISH_ID fishID; 12
        // FishType fishType; type-5 king-4
        // unsigned char isAlive; 1
        cmdData.randomSeed = *(bit32*)(buffer);
        cmdData.elapsed = *(bit16*)(buffer + 4);
        cmdData.fishID = *(bit16*)(buffer + 6);
        cmdData.fishType.type = *(buffer + 8);
        cmdData.fishType.king = *(buffer + 9);
        cmdData.isAlive = *(buffer + 10);
        if (dataSize) {
            *dataSize = 11;
        }

        return true;
    }
    */

    static unsigned short encodeProduceFishShoal(FishProduceData& fishProduceDataShoal,
                                                 ::rycsfish::CMD_S_ProduceFishShoal* pSceneShoal)
    {
        /*
        if (bufferSize < 5) {
            return 0;
        }
        // FISH_ID fishID; 12
        // short elapsed; 16
        // ShoalType shoalType; 3
        short elapsed = 0;
        char shoalType = fishProduceDataShoal.getShoalType();
        if (FishPathData* pathData = fishProduceDataShoal.getFishPathData()) {
            elapsed = static_cast<short>(pathData->getElapsed() * 100); // ��λ��0.01��
        }
        memcpy(buffer, &fishProduceDataShoal.getFishKind().id, 2);
        memcpy(buffer + 2, &elapsed, 2);
        memcpy(buffer + 4, &shoalType, 1);

        return 5;
        */

        int bytesize = 0;
        do
        {
            if (!pSceneShoal) break;
            int elapsed = 0;
            int shoalType = fishProduceDataShoal.getShoalType();
            if (FishPathData* pathData = fishProduceDataShoal.getFishPathData())
            {
                elapsed = (pathData->getElapsed() * 100);
            }

            // update the special field value for the produce fish shoal.
            pSceneShoal->set_fishid(fishProduceDataShoal.getFishKind().id);
            pSceneShoal->set_elapsed(elapsed);
            pSceneShoal->set_shoaltype(shoalType);
            bytesize = pSceneShoal->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeProduceFishShoal(CMD_S_ProduceFishShoal& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 5) {
            return false;
        }
        // FISH_ID fishID; 12
        // short elapsed; 16
        // ShoalType shoalType; 3
        cmdData.fishID= *(bit16*)buffer;
        cmdData.elapsed = *(bit16*)(buffer + 2);
        cmdData.shoalType = *(buffer + 4);
        if (dataSize) {
            *dataSize = 5;
        }

        return true;
    } */

    static unsigned short encodeProduceFishArray(const CMD_S_ProduceFishArray& cmdData,
                                                 ::rycsfish::CMD_S_ProduceFishArray* pFishArray)
    {
        /*
        if (bufferSize < 7) {
            return 0;
        }
        // unsigned int randSeed; 32
        // FISH_ID beginFishID; 12
        // unsigned char fishArrayID; 4
        memcpy(buffer, &cmdData.randomSeed, 4);
        memcpy(buffer + 4, &cmdData.firstFishID, 2);
        memcpy(buffer + 6, &cmdData.fishArrayID, 1);

        return 7;
        */

        int bytesize = 0;
        do
        {
            if (!pFishArray) break;
            pFishArray->set_randomseed(cmdData.randomSeed);
            pFishArray->set_firstfishid(cmdData.firstFishID);
            pFishArray->set_fisharrayid(cmdData.fishArrayID);

            bytesize = pFishArray->ByteSize();

        } while (0);
    //Cleanup:
        return (bytesize);
    }

    static unsigned short encodeProduceFishArray(unsigned char* buffer, unsigned short bufferSize,const CMD_S_ProduceFishArray& cmdData)
    {
        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_ProduceFishArray fishArray;
            bytesize = encodeProduceFishArray(cmdData,&fishArray);
            if (!bytesize) {
                LOG(ERROR) << "encodeProduceFishArray buffer failed.";
            }

            // serialize the data to the special buffer.
            fishArray.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }


    /*
    static bool decodeProduceFishArray(CMD_S_ProduceFishArray& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 7) {
            return false;
        }
        // unsigned int randSeed; 32
        // FISH_ID beginFishID; 12
        // unsigned char fishArrayID; 4
        cmdData.randomSeed = *(bit32*)buffer;
        cmdData.firstFishID = *(bit16*)(buffer + 4);
        cmdData.fishArrayID = *(buffer + 6);
        if (dataSize) {
            *dataSize = 7;
        }

        return true;
    }
    */

    /*
    static bool decodeProduceFishes(std::vector<CMD_S_ProduceFish*>& fishLeaders, std::vector<CMD_S_ProduceFishShoal*>& fishShoal, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize < 2) {
            return false;
        }
        unsigned short i;
        unsigned short dataSize = 2;
        unsigned short tempSize = 0;
        unsigned short numberOfFishLeaders = *reinterpret_cast<const unsigned short*>(buffer);
        for (i = 0; i < numberOfFishLeaders; ++i) {
            CMD_S_ProduceFish* produceFish = FishProduceAllocator::getInstance()->allocCMD_S_ProduceFish();
            if (produceFish) {
                if (!decodeProduceFish(*produceFish, buffer + dataSize, bufferSize - dataSize, &tempSize)) {
                    FishProduceAllocator::getInstance()->freeCMD_S_ProduceFish(produceFish);
                    return false;
                }
                fishLeaders.push_back(produceFish);
            } else {
                return false;
            }
            dataSize += tempSize;
        }

        while (dataSize < bufferSize) {
            CMD_S_ProduceFishShoal* produceFishShoal = FishProduceAllocator::getInstance()->allocCMD_S_ProduceFishShoal();
            if (produceFishShoal) {
                if (!decodeProduceFishShoal(*produceFishShoal, buffer + dataSize, bufferSize - dataSize, &tempSize)) {
                    FishProduceAllocator::getInstance()->freeCMD_S_ProduceFishShoal(produceFishShoal);
                    return false;
                }
                fishShoal.push_back(produceFishShoal);
                dataSize += tempSize;
            } else {
                return false;
            }
        }
        return true;
    }
    */

    // try to encode the special produce fishs.
    static unsigned short encodeProduceFishes(const FishProduceDataMap& produceFishes,::rycsfish::CMD_S_ProduceFish* pSceneFishes)
    {
        // get byte size.
        int bytesize = 0;
        do
        {
            if (!pSceneFishes) break;
            // initialize the special content.
            FishProduceDataMap leaderFishes;
            std::vector<FishProduceData*> freeDataList;
            std::vector<FishProduceData*> fishShoal;
            std::vector<FishProduceData*> fishShoalForSearch;
            FishProduceData* p;
            unsigned short dataSize = 2;
            unsigned short tempSize = 0;
            unsigned short i;
            unsigned short numberOfFishLeaders = 0;
            FishProduceDataMap::const_iterator iter = produceFishes.begin();


             iter = produceFishes.begin();
            // loop to initialize the data buffer.
            while (iter != produceFishes.end())
            {
                p = iter->second;
                ShoalType shoalType = p->getShoalType();
                if (stSingle == shoalType)
                {
                    leaderFishes.insert(FishProduceDataMap::value_type(p->getFishKind().id, p));
                }
                else
                {
                    if (stFollow == shoalType || stLump == shoalType)
                    {
                        FishProduceDataMap::iterator iterFind = leaderFishes.find(p->getLeaderFishID());
                        if (iterFind == leaderFishes.end())
                        {
                            fishShoalForSearch.push_back(p);
                        }
                    }

                    fishShoal.push_back(p);
                }
                ++iter;
            }

//            while (iter != produceFishes.end())
//            {
//                p = iter->second;
//                ShoalType shoalType = p->getShoalType();
//                if (stSingle != shoalType)
//                {
//                    if (stFollow == shoalType || stLump == shoalType)
//                    {
//                        FishProduceDataMap::iterator iterFind = leaderFishes.find(p->getLeaderFishID());
//                        if (iterFind == leaderFishes.end())
//                        {
//                            fishShoalForSearch.push_back(p);
//                        }
//                    }
//                    fishShoal.push_back(p);
//                }

//                ++iter;
//            }
            // build the special leader fish array content now.
            for (i = 0; i != fishShoalForSearch.size(); ++i) {
                p = fishShoalForSearch.at(i);
                FishProduceDataMap::iterator iterFind = leaderFishes.find(p->getLeaderFishID());
                if (iterFind == leaderFishes.end()) {
                    FishProduceData* leaderFishProduceData = FishProduceAllocator::getInstance()->allocFishProduceData();
                    if (leaderFishProduceData) {
                        p->makeLeaderFishProduceData(leaderFishProduceData);
                        leaderFishes.insert(FishProduceDataMap::value_type(leaderFishProduceData->getFishKind().id, leaderFishProduceData));
                        freeDataList.push_back(leaderFishProduceData);
                    }
                }
            }


            iter = leaderFishes.begin();
            // loop to initialize the data buffer.
            while (iter != leaderFishes.end())
            {
                FishProduceData* leader=iter->second;
                FISH_ID fishid = iter->first;
                for (i = 0; i != fishShoal.size(); ++i)
                {
                   p = fishShoal.at(i);
                   if(fishid==p->getLeaderFishID())
                   {

                       leader->setLeaderFishNum(1);
                       leader->setShoalKind(p->getShoalType());
                   }
                }
                ++iter;
            }
            // build the encode produce content value.
            numberOfFishLeaders = leaderFishes.size();
            pSceneFishes->set_numberoffishleaders(numberOfFishLeaders);
            // encodeProduceFish
            iter = leaderFishes.begin();
            while (iter != leaderFishes.end())
            {
                p = iter->second;
                ::rycsfish::FishProduceData* pProduceData = pSceneFishes->add_producefish();
                tempSize = encodeProduceFish(*p, pProduceData);
                if (!tempSize) {
                    LOG(ERROR) << "encodeProduceFishes.encodeProduceFish failed.";
                }
                p->clearFishNum();
                // add the byte size.
                bytesize += tempSize;
                ++iter;
            }

            // loop to initialize the data list content.
            for (i = 0; i != freeDataList.size(); ++i) {
                FishProduceAllocator::getInstance()->freeFishProduceData(freeDataList.at(i));
            }

            // encodeProduceFishShoal
            for (i = 0; i != fishShoal.size(); ++i)
            {
                p = fishShoal.at(i);
                ::rycsfish::CMD_S_ProduceFishShoal* pFishShoal = pSceneFishes->add_fishshoal();
                tempSize = encodeProduceFishShoal( *p, pFishShoal);
                if (!tempSize) {
                    LOG(ERROR) << "encodeProduceFishes.encodeProduceFishShoal failed.";
                }

                // add the byte size.
                bytesize += tempSize;
            }

        } while (0);
    //Cleanup:
        return (bytesize);
    }

    // try to encode the special produce fishs content value data.
    static unsigned short encodeProduceFishes(unsigned char* buffer,
                                              unsigned short bufferSize,
                                              const FishProduceDataMap& produceFishes)
    {
        int bytesize = 0;
        try
        {
            ::rycsfish::CMD_S_ProduceFish sceneProduceFishes;
            encodeProduceFishes(produceFishes,&sceneProduceFishes);
            bytesize = sceneProduceFishes.ByteSize();
            if (bytesize)
            {
                sceneProduceFishes.SerializeToArray(buffer,bytesize);
            }

        } catch (...)
        {
            LOG(ERROR) << "encodeProduceFishes catched failed!";
        }
    //Cleanup:
        return (bytesize);
    }


    /*
    static bool decodeGameScene(CMD_S_GameScene& cmdData, std::vector<CMD_S_ProduceFish*>& fishLeaders, std::vector<CMD_S_ProduceFishShoal*>& fishShoal, const unsigned char* buffer, unsigned short bufferSize) {
        unsigned short dataSize = 3;
        unsigned short tempSize;
        if (bufferSize < dataSize) {
            return false;
        }
        unsigned char i;
        unsigned char fishBasicDataCount = 0;
        unsigned char fishSwimmingPatternCount = 0;
        unsigned char fishShoalInfoCount = 0;
        fishBasicDataCount = *buffer;
        fishSwimmingPatternCount = *(buffer + 1);
        fishShoalInfoCount = *(buffer + 2);
        FishBasicData fishBasicData;
        for (i = 0; i < fishBasicDataCount; ++i) {
            if (!decodeFishBasicData(fishBasicData, buffer + dataSize, bufferSize - dataSize, &tempSize)) {
                return false;
            }
            FishBasicDataManager::getInstance()->setFishBasicData(fishBasicData);
            dataSize += tempSize;
        }
        FishSwimmingPattern fishSwimmingPattern;
        for (i = 0; i < fishSwimmingPatternCount; ++i) {
            if (!decodeFishSwimmingPattern(fishSwimmingPattern, buffer + dataSize, bufferSize - dataSize, &tempSize)) {
                return false;
            }
            FishSwimmingPatternManager::getInstance()->setFishSwimmingPattern(fishSwimmingPattern);
            dataSize += tempSize;
        }
        FishShoalInfo fishShoalInfo;
        for (i = 0; i < fishShoalInfoCount; ++i) {
            if (!decodeFishShoalInfo(fishShoalInfo, buffer + dataSize, bufferSize - dataSize, &tempSize)) {
                return false;
            }
            FishShoalInfoManager::getInstance()->setFishShoalInfo(fishShoalInfo);
            dataSize += tempSize;
        }
        // CMD_S_GameConfig gameConfig;
        // CMD_S_ProduceFishArray fishArray;
        // unsigned char sceneID; 3
        // unsigned char pause; 1
        tempSize = sizeof(CMD_S_GameConfig);
        if (bufferSize < tempSize + dataSize) {
            return false;
        }
        memcpy(&cmdData.gameConfig, buffer + dataSize, tempSize);
        dataSize += tempSize;

        unsigned short fishArraySize = 0;
        if (!decodeProduceFishArray(cmdData.fishArray, buffer + dataSize, bufferSize - dataSize, &fishArraySize)) {
            return false;
        }
        dataSize += fishArraySize;

        if (bufferSize < dataSize + 2) {
            return false;
        }
        memcpy(&cmdData.sceneID, buffer + dataSize, 1);
        memcpy(&cmdData.pause, buffer + dataSize + 1, 1);
        dataSize += 2;

        return decodeProduceFishes(fishLeaders, fishShoal, buffer + dataSize, bufferSize - dataSize);
    }
    */

    // encode fish array parameter.
    static unsigned short encodeFishArrayParam(const CMD_S_GameConfig& gameConfig,
                                               ::rycsfish::FishArrayParam* pFishArrayParam)
    {
        int bytesize = 0;
        do
        {
            if (!pFishArrayParam) break;
            ::rycsfish::FishArrayParam1* param1 = pFishArrayParam->mutable_param1();
            if (param1)
            {
                const FishArrayParam1& fishparam1 = gameConfig.fishArrayParam.param1;
                param1->set_small_fish_type(fishparam1.small_fish_type);
                param1->set_small_fish_count(fishparam1.small_fish_count);
                param1->set_move_speed_small(fishparam1.move_speed_small);
                param1->set_move_speed_big(fishparam1.move_speed_big);
                param1->set_space(fishparam1.space);

                int kindCount = CountArray(fishparam1.big_fish_kind_count);
                for (int i=0;i<kindCount;i++)
                {
                    param1->add_big_fish_kind_count(fishparam1.big_fish_kind_count[i]);
                    // count the special big fish parameter count now item data now.
                    int nBigCount = fishparam1.big_fish_kind_count[i];
                    const FishType* pType = fishparam1.big_fish[i];
                    for (int j=0;j<nBigCount;j++) {
                        ::rycsfish::FishType* fishType = param1->add_big_fish();
                        fishType->set_type(pType[j].type);
                        fishType->set_king(pType[j].king);
                    }
                }


                // try to set the special big fish time item content.
                param1->set_big_fish_time(fishparam1.big_fish_time);
            }

            ::rycsfish::FishArrayParam2* param2 = pFishArrayParam->mutable_param2();
            if (param2)
            {
                const FishArrayParam2& fishparam2 = gameConfig.fishArrayParam.param2;
                param2->set_small_fish_type(fishparam2.small_fish_type);
                param2->set_small_fish_count(fishparam2.small_fish_count);
                param2->set_move_speed_small(fishparam2.move_speed_small);
                param2->set_move_speed_big(fishparam2.move_speed_big);

                param2->set_big_fish_count(fishparam2.big_fish_count);
                param2->set_big_fish_kind_count(fishparam2.big_fish_kind_count);

                int nBigCount = fishparam2.big_fish_kind_count;
                for (int i=0;i<nBigCount;i++) {
                    ::rycsfish::FishType* fishType = param2->add_big_fish();
                    fishType->set_type(fishparam2.big_fish[i].type);
                    fishType->set_king(fishparam2.big_fish[i].king);
                }

                param2->set_big_fish_time_interval(fishparam2.big_fish_time_interval);
                param2->set_small_fish_time_interval(fishparam2.small_fish_time_interval);
            }

            ::rycsfish::FishArrayParam3* param3 = pFishArrayParam->mutable_param3();
            if (param3)
            {
                const FishArrayParam3& fishparam3 = gameConfig.fishArrayParam.param3;
                param3->set_rat_angles(fishparam3.rot_angles);
                param3->set_rot_speed(fishparam3.rot_speed);
                param3->set_time_interval(fishparam3.time_interval);
                param3->set_param_count(fishparam3.param_count);

                int boomCount = fishparam3.param_count;
                for (int i=0; i<boomCount; i++) {
                    ::rycsfish::CirboomParam* param = param3->add_param();
                    param->set_delay(fishparam3.param[i].delay);
                    param->set_fish_type(fishparam3.param[i].fish_type);
                    param->set_count(fishparam3.param[i].count);
                    param->set_move_speed(fishparam3.param[i].move_speed);
                    param->set_radius(fishparam3.param[i].radius);
                }
            }

            ::rycsfish::FishArrayParam4* param4 = pFishArrayParam->mutable_param4();
            if (param4) {
                const FishArrayParam4& fishparam4 = gameConfig.fishArrayParam.param4;
                param4->set_param_count(fishparam4.param_count);

                int boomCount = fishparam4.param_count;
                for (int i=0;i<boomCount;i++) {
                    ::rycsfish::CirboomParam* param = param4->add_param();
                    param->set_delay(fishparam4.param[i].delay);
                    param->set_fish_type(fishparam4.param[i].fish_type);
                    param->set_count(fishparam4.param[i].count);
                    param->set_move_speed(fishparam4.param[i].move_speed);
                    param->set_radius(fishparam4.param[i].radius);
                }
            }

            ::rycsfish::FishArrayParam5* param5 = pFishArrayParam->mutable_param5();
            if (param5) {
                const FishArrayParam5& fishparam5 = gameConfig.fishArrayParam.param5;
                param5->set_rot_angles(fishparam5.rot_angles);
                param5->set_rot_speed(fishparam5.rot_speed);
                param5->set_time_interval(fishparam5.time_interval);

                int boomCount = CountArray(fishparam5.param_count);
                for (int i=0;i<boomCount;i++)
                {
                    // add the param_count.
                    param5->add_param_count(fishparam5.param_count[i]);

                    // loop to add the special data item value.
                    int paramCount = fishparam5.param_count[i];
                    const Circle2Param* pCirparam = fishparam5.param[i];
                    for (int i=0;i<paramCount;i++) {
                        ::rycsfish::Circle2Param* param = param5->add_param();
                        param->set_fish_type(pCirparam[i].fish_type);
                        param->set_count(pCirparam[i].count);
                        param->set_move_speed(pCirparam[i].move_speed);
                        param->set_radius(pCirparam[i].radius);
                        param->set_order(pCirparam[i].order);
                        pCirparam++;
                    }
                }

            }

            ::rycsfish::FishArrayParam6* param6 = pFishArrayParam->mutable_param6();
            if (param6) {
                const FishArrayParam6& fishparam6 = gameConfig.fishArrayParam.param6;
                param6->set_move_speed(fishparam6.move_speed);

                int radiusCount = CountArray(fishparam6.max_radius);
                for (int i=0;i<radiusCount;i++) {
                    param6->add_max_radius(fishparam6.max_radius[i]);
                }

                int paramCount = CountArray(fishparam6.param_count);
                for (int i=0;i<paramCount;i++)
                {
                    // add param_count.
                    param6->add_param_count(fishparam6.param_count[i]);

                    // add param item.
                    int crossCount = fishparam6.param_count[i];
                    const CrossoutParam* pCross = fishparam6.param[i];
                    for (int j=0;j<crossCount;j++) {
                        ::rycsfish::CrossoutParam* param = param6->add_param();
                        param->set_fish_type(pCross[j].fish_type);
                        param->set_count(pCross[j].count);
                        param->set_radius(pCross[j].radius);
                    }
                }
            }

            ::rycsfish::FishArrayParam7* param7 = pFishArrayParam->mutable_param7();
            if (param7) {
                const FishArrayParam7& fishparam7 = gameConfig.fishArrayParam.param7;
                param7->set_param_count(fishparam7.param_count);

                int paramCount = CountArray(fishparam7.param);
                const CenterOutParam* pOut = fishparam7.param;
                for (int i=0;i<paramCount;i++) {
                    ::rycsfish::CenterOutParam* param = param7->add_param();
                    param->set_fish_type(pOut[i].fish_type);
                    param->set_count(pOut[i].count);
                    param->set_dirs(pOut[i].dirs);
                    param->set_move_speed(pOut[i].move_speed);
                    param->set_interval(pOut[i].interval);
                }

                param7->set_fish_time_delay(fishparam7.fish_time_delay);
                param7->set_small_fish_time_interval(fishparam7.small_fish_time_interval);
            }

            ::rycsfish::FishArrayParam8* param8 = pFishArrayParam->mutable_param8();
            if (param8)
            {
                const FishArrayParam8& fishparam8 = gameConfig.fishArrayParam.param8;
                param8->set_small_fish_type(fishparam8.small_fish_type);
                param8->set_small_fish_count(fishparam8.small_fish_count);
                param8->set_move_speed_small(fishparam8.move_speed_small);

                param8->set_move_speed_big(fishparam8.move_speed_big);
                int bigkindCount = CountArray(fishparam8.big_fish_kind_count);
                for (int i=0;i<bigkindCount;i++)
                {
                    param8->add_big_fish_kind_count(fishparam8.big_fish_kind_count[i]);
                    // loop to add all the element item value item now.
                    int typeCount = fishparam8.big_fish_kind_count[i];
                    const FishType* pType = fishparam8.big_fish[i];
                    for (int j=0;j<typeCount;j++)
                    {
                        ::rycsfish::FishType* fishType = param8->add_big_fish();
                        fishType->set_king(pType[j].king);
                        fishType->set_type(pType[j].type);
                    }
                }

                param8->set_big_fish_time_interval(fishparam8.big_fish_time_interval);
                param8->set_small_fish_time_interval(fishparam8.small_fish_time_interval);
            }

            // try to get the special byte size now.
            bytesize = pFishArrayParam->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    // encode game config.
    static unsigned short encodeGameConfig(const CMD_S_GameConfig& gameConfig,
                                           ::rycsfish::CMD_S_GameConfig* pGamecfg)
    {
        int bytesize = 0;
        do
        {
            if (!pGamecfg) break;
            pGamecfg->set_version(gameConfig.version);
            pGamecfg->set_ticketratio(gameConfig.ticketRatio);

            pGamecfg->set_bulletstrengtheningscale(gameConfig.bulletStrengtheningScale);
            pGamecfg->set_cannonpowermin(gameConfig.cannonPowerMin);
            pGamecfg->set_cannonpowermax(gameConfig.cannonPowerMax);

            pGamecfg->set_bulletspeed(gameConfig.bulletSpeed);
            pGamecfg->set_consoletype(gameConfig.consoleType);

            pGamecfg->set_prizescore(gameConfig.prizeScore);
            pGamecfg->set_exchangeratiouserscore(gameConfig.exchangeRatioUserScore);

            pGamecfg->set_exchangeratiofishscore(gameConfig.exchangeRatioFishScore);
            pGamecfg->set_decimalplaces(gameConfig.decimalPlaces);
            // pGamecfg->set_exchangeratio(gameConfig.exchangeRatio); // no used.

            int exchangeCount = CountArray(gameConfig.exchangeFishScore);
            for (int i=0;i<exchangeCount;i++) {
                pGamecfg->add_exchangefishscore(gameConfig.exchangeFishScore[i]);
            }

            int fishscoreCount = CountArray(gameConfig.fishScore);
            for (int i=0;i<fishscoreCount;i++) {
                pGamecfg->add_fishscore(gameConfig.fishScore[i]);
            }

#ifdef __USE_LIFE2__
            pGamecfg->set_exchangeratioingot(gameConfig.exchangeRatioIngot);
            pGamecfg->set_exchangeratioscore2(gameConfig.exchangeRatioScore2);
            pGamecfg->set_fishscore2(gameConfig.fishScore2);
            pGamecfg->set_exchangefishscore2(gameConfig.exchangeFishScore2);
#endif//

            pGamecfg->set_isexperienceroom(gameConfig.isExperienceRoom);
            pGamecfg->set_isanticheatroom(gameConfig.isAntiCheatRoom);
            pGamecfg->set_timeoutquitcheck(gameConfig.timeOutQuitCheck);
            pGamecfg->set_timeoutquitmsg(gameConfig.timeOutQuitMsg);
            pGamecfg->set_fishlockable(gameConfig.fishLockable);

            ::rycsfish::FishArrayParam* pArrayParam = pGamecfg->mutable_fisharrayparam();
            int tempsize = encodeFishArrayParam(gameConfig, pArrayParam);
            if (tempsize < 0) {
                LOG(ERROR) << "encodeFishArrayParam failed.";
            }

            pGamecfg->set_supportedgameplays(gameConfig.supportedGameplays);

    // 动态换坐位玩法.
#ifdef __DYNAMIC_CHAIR__
            int chairCount = CountArray(gameConfig.dynamicChair);
            for (int i=0;i<chairCount;i++) {
                pGamecfg->add_dynamicchair(gameConfig.dynamicChair[i]);
            }
#endif

#ifdef __GAMEPLAY_COLLECT__
            ::rycsfish::CollectData* collectData = pGamecfg->mutable_collectdata();
            collectData->set_collectcount(gameConfig.collectData.collectCount);

            int idCount   = CountArray(gameConfig.collectData.collectFishId);
            for (int i=0;i<idCount;i++) {
                collectData->add_collectfishid(gameConfig.collectData.collectFishId[i]);
            }

            int statCount = CountArray(gameConfig.collectData.collectStats);
            for (int i=0;i<statCount;i++) {
                collectData->add_collectstats(gameConfig.collectData.collectStats[i]);
            }
#endif

#ifdef __GAMEPLAY_MSYQ__
            ::rycsfish::MsyqData* pMsyqdata = pGamecfg->mutable_msyqdata();
            int nmsyqCount = CountArray(gameConfig.msyqData.msyqPercent);
            for (int i=0;i<msyqCount;i++) {
                pMsyqdata->add_msyqpercent(gameConfig.msyqData.msyqPercent[i]);
            }
#endif

#ifdef __GAMEPLAY_PAIPAILE__
            int pplCount = CountArray(gameConfig.pplData);
            for (int i=0;i<pplCount;i++) {
                ::rycsfish::PplData* pplData = pGamecfg->add_ppldata();
                pplData->set_isplaying(gameConfig.pplData.isPlaying);
                pplData->set_hits(gameConfig.pplData.hits);
            }

            ::rycsfish::PplXmlConfig* pplXmlConfig = pGamecfg->mutable_pplxmlconfig();
            pplXmlConfig->set_fishtype(gameConfig.pplXmlConfig.fishType);
            pplXmlConfig->set_fishlife(gameConfig.pplXmlConfig.fishLife);
#endif
			pGamecfg->set_nflymaxfiretime(gameConfig.nFlyMaxFireTime);
            // try to get the special byte size.
            bytesize = pGamecfg->ByteSize();
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    // CMD_S_GameScene + first fish + (CMD_S_ProduceFish) + fishID list
    static unsigned short encodeGameScene(unsigned char* buffer, unsigned short bufferSize, const CMD_S_GameScene& cmdData, const FishProduceDataMap& produceFishes,bool isChangescene, bool bBorebitStartRotate)
    {
        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_GameScene gameScene;

            const FishBasicDataMap& fishBasicDataMap = FishBasicDataManager::getInstance()->getAllFishBasicData();
            const FishSwimmingPatternMap& fishSwimmingPatternMap = FishSwimmingPatternManager::getInstance()->getAllFishSwimmingPattern();
            const FishShoalInfoMap& fishShoalInfoMap = FishShoalInfoManager::getInstance()->getAllFishShoalInfo();
            unsigned char fishBasicCount     = fishBasicDataMap.size();
            unsigned char fishSwimPatternCnt = fishSwimmingPatternMap.size();
            unsigned char fishShoalInfoCount = fishShoalInfoMap.size();

            // initialize the special count value data value.
            gameScene.set_fishbasicdatacount(fishBasicCount);
            gameScene.set_fishswimmingpattercount(fishSwimPatternCnt);
            gameScene.set_fishshoalinfocount(fishShoalInfoCount);
            gameScene.set_scenetime(cmdData.sceneTime);
            FishBasicDataMap::const_iterator iterFishBasicData = fishBasicDataMap.begin();
            while (iterFishBasicData != fishBasicDataMap.end())
            {
                ::rycsfish::FishBasicData* pBasicData = gameScene.add_fishbasicdata();
                int tempSize = encodeFishBasicData(iterFishBasicData->second, pBasicData);
                if (!tempSize) {
                    LOG(ERROR) << "encodeGameScene -> encodeFishBasicData failed.";
                    return 0;
                }

                // add the byte size.
                ++iterFishBasicData;
            }

            FishSwimmingPatternMap::const_iterator iterFishSwimmingPattern = fishSwimmingPatternMap.begin();
            while (iterFishSwimmingPattern != fishSwimmingPatternMap.end())
            {
                ::rycsfish::FishSwimmingPattern* pPattern = gameScene.add_fishswimmingpattern();
                int tempSize = encodeFishSwimmingPattern(iterFishSwimmingPattern->second, pPattern);
                if (!tempSize) {
                    LOG(ERROR) << "encodeGameScene -> encodeFishSwimmingPattern failed.";
                    return 0;
                }

                // add the byte size value.
                ++iterFishSwimmingPattern;
            }

            FishShoalInfoMap::const_iterator iterFishShoalInfo = fishShoalInfoMap.begin();
            while (iterFishShoalInfo != fishShoalInfoMap.end())
            {
                ::rycsfish::FishShoalInfo* pShoalInfo = gameScene.add_fishshoal();
                int tempSize = encodeFishShoalInfo(iterFishShoalInfo->second, pShoalInfo);
                if (!tempSize) {
                    return 0;
                }

                // add the byte size.
                ++iterFishShoalInfo;
            }

            // CMD_S_GameConfig gameConfig;
            // CMD_S_ProduceFishArray fishArray;
            // unsigned char sceneID; 3
            // unsigned char pause; 1
            ::rycsfish::CMD_S_GameConfig* pGamecfg = gameScene.mutable_gameconfig();
            // encode the special game config data to protobuf.
            int tempsize = encodeGameConfig(cmdData.gameConfig, pGamecfg);
            if (!tempsize) {
                LOG(ERROR) << "encodeGameConfig failed.";
            }

            ::rycsfish::CMD_S_ProduceFishArray* pFishArray = gameScene.mutable_fisharray();
            tempsize = encodeProduceFishArray(cmdData.fishArray,pFishArray);
            if (!tempsize) {
                LOG(ERROR) << "encodeProduceFishArray failed.";
            }

            // try to set the fish scene id pause.
            gameScene.set_sceneid(cmdData.sceneID);
            gameScene.set_pause(cmdData.pause);
            for(int i=0;i<MAX_PLAYERS;i++)
            {
                //this player is in game
                if(cmdData.players[i].isExist)
                {
                    ::rycsfish::CMD_S_PlayerInfo *plays=gameScene.add_players();
                    plays->set_chariid(cmdData.players[i].chairid);
                    plays->set_nikename(cmdData.players[i].nikename);
                    plays->set_scores(cmdData.players[i].score);
                    plays->set_userid(cmdData.players[i].userid);
                    plays->set_userstatus(cmdData.players[i].status);
                }
				if (cmdData.specialFire[i].bHaveSpecialFire)
				{
					::rycsfish::CMD_S_SpecialFire *specials = gameScene.add_specialfire();
					specials->set_userid(cmdData.players[i].userid);
					specials->set_chariid(cmdData.specialFire[i].chairid);
					specials->set_firetype(cmdData.specialFire[i].FireType);
					specials->set_bhavefire(cmdData.specialFire[i].bHaveFire);
					specials->set_firecounttime(cmdData.specialFire[i].FireCountTime);
					specials->set_fireangle(cmdData.specialFire[i].FireAngle);
					specials->set_firepasstime(cmdData.specialFire[i].FirePassTime);
					specials->set_specialscore(cmdData.specialFire[i].specialScore);
					if (bBorebitStartRotate)
					{
						specials->set_fireendcounttime(cmdData.specialFire[i].FireEndCountTime);
					}
				}
            }
            if(isChangescene)
            {
                for(int i=0;i<cmdData.fishLiveArray.size();i++)
                {
                    gameScene.add_livefishs(cmdData.fishLiveArray[i]);
                }
//                FishProduceDataMap::const_iterator fishlive = produceFishes.begin();
//                for(;fishlive!=produceFishes.end();fishlive++)
//                {

//                }
            }
            // check if any data exist.
            if (produceFishes.size())
            {
                LOG(INFO) << "encodeGameScene produceFishes size:" << produceFishes.size();

                ::rycsfish::CMD_S_ProduceFish* pFishes = gameScene.mutable_fishes();
                int tempSize = encodeProduceFishes(produceFishes, pFishes);
                if (!tempSize) {
                    LOG(ERROR) << "encodeProduceFishes failed!";
                }
            }   else
            {
                // try to encode the special game scene data content for later user now.
                LOG(INFO) << "encodeGameScene produceFishes size is 0. no fish on init.";
            }

            // serialize data to output buffer.
            bytesize = gameScene.ByteSize();
            gameScene.SerializeToArray(buffer, bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static unsigned short encodeUserFire(unsigned char* buffer, unsigned short bufferSize, const CMD_C_UserFire& cmdData)
    {
        /*
        if (bufferSize < 9) {
            return 0;
        }
        // FISH_ID lockedFishID; 12
        // BulletScoreT cannonPower; 22
        // short angle; 9+1(����λ)
        // char bulletID; 4
        memcpy(buffer, &cmdData.lockedFishID, 2);
        memcpy(buffer + 2, &cmdData.cannonPower, 4);
        memcpy(buffer + 6, &cmdData.angle, 2);
        memcpy(buffer + 8, &cmdData.bulletID, 1);

        return 9;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_C_UserFire userFire;
            userFire.set_lockedfishid(cmdData.lockedFishID);
            userFire.set_cannonpower(cmdData.cannonPower);
            userFire.set_angle(cmdData.angle);
            userFire.set_bulletid(cmdData.bulletID);
            // serial data to data buffer.
            bytesize = userFire.ByteSize();
            userFire.SerializeToArray(buffer, bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static bool decodeUserFire(CMD_C_UserFire& cmdData, const unsigned char* buffer, unsigned short bufferSize)
    {
        /*
        if (bufferSize != 9) {
            return false;
        }
        // FISH_ID lockedFishID; 12
        // BulletScoreT cannonPower; 22
        // short angle; 9+1(����λ)
        // char bulletID; 4
        cmdData.lockedFishID = *(bit16*)buffer;
        cmdData.cannonPower = *(bit32*)(buffer + 2);
        cmdData.angle = *(bit16*)(buffer + 6);
        cmdData.bulletID = *(buffer + 8);

        return true;
        */

        bool bSuccess = false;
        try
        {

            if ((!buffer)||(!bufferSize)) throw;
            ::rycsfish::CMD_C_UserFire userFire;
            if (userFire.ParseFromArray(buffer,bufferSize))
            {
                cmdData.lockedFishID = userFire.lockedfishid();
                cmdData.cannonPower  = userFire.cannonpower();
                cmdData.angle        = userFire.angle();
                cmdData.bulletID     = userFire.bulletid();
                cmdData.fireType     = userFire.firetype();
                bSuccess = true;
            }

        } catch(...)
        {
            LOG(ERROR) << "decodeUserFire failed!";
        }
    //Cleanup:
        return (bSuccess);
    }

    static unsigned short encodeUserFire(unsigned char* buffer, unsigned short bufferSize, const CMD_S_UserFire& cmdData)
    {
        /*
        if (bufferSize < 10) {
            return 0;
        }
        // FISH_ID lockedFishID; 12
        // BulletScoreT cannonPower; 22
        // short angle; 9+1(����λ)
        // char bulletID; 4
        // playerID; 3
        memcpy(buffer, &cmdData.c.lockedFishID, 2);
        memcpy(buffer + 2, &cmdData.c.cannonPower, 4);
        memcpy(buffer + 6, &cmdData.c.angle, 2);
        memcpy(buffer + 8, &cmdData.c.bulletID, 1);
        memcpy(buffer + 9, &cmdData.playerID, 1);
        return 10;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_UserFire userFire;
            ::rycsfish::CMD_C_UserFire* pUserFire = userFire.mutable_c();
            pUserFire->set_lockedfishid(cmdData.c.lockedFishID);
            pUserFire->set_cannonpower(cmdData.c.cannonPower);
            pUserFire->set_angle(cmdData.c.angle); // angle.
			pUserFire->set_bulletid(cmdData.c.bulletID);
			pUserFire->set_firetype(cmdData.c.fireType);
            userFire.set_playerid(cmdData.playerID);
            // get user fire byte size now.
            bytesize = userFire.ByteSize();
            userFire.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }


    /*
    static bool decodeUserFire(CMD_S_UserFire& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 10) {
            return false;
        }
        // FISH_ID lockedFishID; 12
        // BulletScoreT cannonPower; 22
        // short angle; 9+1(����λ)
        // char bulletID; 4
        // playerID; 3
        cmdData.c.lockedFishID = *(bit16*)(buffer);
        cmdData.c.cannonPower = *(bit32*)(buffer + 2);
        cmdData.c.angle = *(bit16*)(buffer + 6);
        cmdData.c.bulletID = *(buffer + 8);
        cmdData.playerID = *(buffer + 9);

        return true;
    }
    */

    static unsigned short encodeGameplay(unsigned char* buffer, unsigned short bufferSize, const CMD_C_Gameplay& cmdData)
    {
        /*
        if (bufferSize < 2) {
            return 0;
        }
        // unsigned char gameplayID; 3
        // unsigned char commandType; 5
        memcpy(buffer, &cmdData.gameplayID, 1);
        memcpy(buffer + 1, &cmdData.commandType, 1);

        return 2;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_C_Gameplay gamePlay;
            gamePlay.set_gameplayid(cmdData.gameplayID);
            gamePlay.set_commandtype(cmdData.commandType);
            // serialize the data to buffer.
            bytesize = gamePlay.ByteSize();
            gamePlay.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static bool decodeGameplay(CMD_C_Gameplay& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL)
    {
        /*
        if (bufferSize < 2) {
            return false;
        }
        // unsigned char gameplayID; 3
        // unsigned char commandType; 5
        cmdData.gameplayID = *buffer;
        cmdData.commandType = *(buffer + 1);
        if (dataSize) {
            *dataSize = 2;
        }

        return true;
        */

        bool bSuccess = false;
        try
        {
            ::rycsfish::CMD_C_Gameplay gamePlay;
            if (gamePlay.ParseFromArray(buffer, bufferSize)) {
                cmdData.gameplayID  = gamePlay.gameplayid();
                cmdData.commandType = gamePlay.commandtype();
                bSuccess = true;
            }

        }  catch (...) {
            LOG(ERROR) << "decodeGameplay failed!";
        }
    //Cleanup:
        return (bSuccess);
    }

    static bool decodeAddLevel(CMD_C_AddLevel& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL)
    {

        bool bSuccess = false;
        try
        {
            ::rycsfish::CMD_C_AddLevel userLevel;
            if (userLevel.ParseFromArray(buffer, bufferSize)) {
                cmdData.userid  = userLevel.playerid();
                cmdData.powerlevel = userLevel.cannonpower();
                bSuccess = true;
            }

        }  catch (...) {
            LOG(ERROR) << "decodeGameplay failed!";
        }
    //Cleanup:
        return (bSuccess);
    }
    static unsigned short encodeGameplay(unsigned char* buffer, unsigned short bufferSize, const CMD_S_Gameplay& cmdData)
    {
        /*
        if (bufferSize < 3) {
            return 0;
        }
        // unsigned char gameplayID; 3
        // unsigned char commandType; 5
        // playerID; 3
        memcpy(buffer, &cmdData.c.gameplayID, 1);
        memcpy(buffer + 1, &cmdData.c.commandType, 1);
        memcpy(buffer + 2, &cmdData.playerID, 1);

        return 3;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_Gameplay gamePlay;
            ::rycsfish::CMD_C_Gameplay* pGamePlay = gamePlay.mutable_c();
            pGamePlay->set_gameplayid(cmdData.c.gameplayID);
            pGamePlay->set_commandtype(cmdData.c.commandType);
            gamePlay.set_playerid(cmdData.playerID);
            // initialize the content value.
            bytesize = gamePlay.ByteSize();
            gamePlay.SerializeToArray(buffer,bytesize);
        }  while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeGameplay(CMD_S_Gameplay& cmdData, const unsigned char* buffer, unsigned short bufferSize, unsigned short* dataSize = NULL) {
        if (bufferSize < 3) {
            return false;
        }
        // unsigned char gameplayID; 3
        // unsigned char commandType; 5
        // playerID; 3
        cmdData.c.gameplayID = *buffer;
        cmdData.c.commandType = *(buffer + 1);
        cmdData.playerID = *(buffer + 2);
        if (dataSize) {
            *dataSize = 3;
        }

        return true;
    } */

    static unsigned short encodeScoreUp(unsigned char* buffer, unsigned short bufferSize, const CMD_C_ScoreUp& cmdData)
    {
        /*
        if (bufferSize < 1) {
            return 0;
        }
        // char inc; 1
        memcpy(buffer, &cmdData.inc, 1);

        return 1;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_C_ScoreUp scoreUp;
            scoreUp.set_inc(cmdData.inc);
            bytesize = scoreUp.ByteSize();
            scoreUp.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeScoreUp(CMD_C_ScoreUp& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 1) {
            return false;
        }
        // char inc; 1
        cmdData.inc = *buffer;

        return true;
    } */

    static unsigned short encodeScoreUp(unsigned char* buffer, unsigned short bufferSize, const CMD_S_ScoreUp& cmdData)
    {
        /*
        if (bufferSize < 10) {
            return 0;
        }
        // long long playerScore; 64
        // char type; 4
        // char playerID; 3
        memcpy(buffer, &cmdData.playerScore, 8);
        memcpy(buffer + 8, &cmdData.type, 1);
        memcpy(buffer + 9, &cmdData.playerID, 1);

        return 10;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_ScoreUp scoreUp;
            scoreUp.set_playerscore(cmdData.playerScore);
            scoreUp.set_score(cmdData.score);
            scoreUp.set_type(cmdData.type);
            scoreUp.set_playerid(cmdData.playerID);
			scoreUp.set_specialscore(cmdData.specialScore);
            // initialize the content value.
            bytesize = scoreUp.ByteSize();
            scoreUp.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeScoreUp(CMD_S_ScoreUp& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 10) {
            return false;
        }
        // long long playerScore; 64
        // int score; 32
        // char type; 4
        // char playerID; 3
        cmdData.playerScore = *(bit64*)buffer;
        cmdData.type = *(buffer + 8);
        cmdData.playerID = *(buffer + 9);

        return true;
    } */

#ifdef __USE_LIFE2__
    static unsigned short encodeScoreUp2(unsigned char* buffer, unsigned short bufferSize, const CMD_S_ScoreUp2& cmdData)
    {
        /*
        if (bufferSize < 6) {
            return 0;
        }
        // int score; 32
        // int score2; 32
        // char type; 4
        // char playerID; 3
        memcpy(buffer, &cmdData.score, 4);
        memcpy(buffer + 4, &cmdData.score2, 4);
        memcpy(buffer + 8, &cmdData.type, 1);
        memcpy(buffer + 9, &cmdData.playerID, 1);

        return 10;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_ScoreUp2 scoreUp2;
            scoreUp2.set_score(cmdData.score);
            scoreUp2.set_score2(cmdData.score2);
            scoreUp2.set_type(cmdData.type);
            scoreUp2.set_playerid(cmdData.playerID);
            // try to get the buffer size.
            bytesize = scoreUp2.ByteSize();
            scoreUp2.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeScoreUp2(CMD_S_ScoreUp2& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 10) {
            return false;
        }
        // int score; 32
        // int score2; 32
        // char type; 4
        // char playerID; 3
        cmdData.score = *(bit32*)buffer;
        cmdData.score2, *(bit32*)(buffer + 4);
        cmdData.type = *(buffer + 8);
        cmdData.playerID, *(buffer + 9);

        return true;
    }
    */
#endif

    static unsigned short encodeExchangeScore(unsigned char* buffer, unsigned short bufferSize, const CMD_S_ExchangeScore& cmdData)
    {
        /*
        if (bufferSize < 10) {
            return 0;
        }
        // long long score; 64
        // char inc; 1
        // char playerID; 3
        memcpy(buffer, &cmdData.score, 8);
        memcpy(buffer + 8, &cmdData.inc, 1);
        memcpy(buffer + 9, &cmdData.playerID, 1);

        return 10;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_ExchangeScore exchangeScore;
            exchangeScore.set_score(cmdData.score);
            exchangeScore.set_inc(cmdData.inc);
            exchangeScore.set_playerid(cmdData.playerID);
            // change the score content value now.
            bytesize = exchangeScore.ByteSize();
            exchangeScore.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeExchangeScore(CMD_S_ExchangeScore& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 10) {
            return false;
        }
        // long long score; 64
        // char playerID; 3
        cmdData.score = *(bit64*)buffer;
        cmdData.inc = *(buffer + 8);
        cmdData.playerID = *(buffer + 9);

        return true;
    }
    */

    static bool decodeExchangeScore(CMD_C_Exchange_Score& exchangeScore, const unsigned char* buffer, unsigned short bufferSize)
    {
        bool bSuccess = false;
        try
        {
            if ((!buffer)||(!bufferSize)) throw;

            ::rycsfish::CMD_C_ExchangeScore fishScore;
            if (fishScore.ParseFromArray(buffer, bufferSize)) {
                exchangeScore.userID = fishScore.userid();
               bSuccess = true;
            }

        }  catch (...) {
            LOG(ERROR) << "decodeExchangeScore failed!";
        }
    //Cleanup:
        return (bSuccess);
    }

    static unsigned short encodeHitFish(unsigned char* buffer, unsigned short bufferSize, const CMD_C_HitFish& cmdData, const FISH_ID* fishIDs, unsigned short fishCount)
    {
        /*
        unsigned short dataSize = 1;
        if (bufferSize < dataSize) {
            return 0;
        }
        // unsigned char bulletID; 4
        // 		memset(buffer, 0, bufferSize);
        memcpy(buffer, &cmdData.bulletID, 1);

        if (fishIDs && fishCount) {
            unsigned short dataSize0 = encodeFishIDs(buffer + dataSize, bufferSize - dataSize, fishIDs, fishCount);
            if (!dataSize) {
                return 0;
            }
            dataSize += dataSize0;
        }

        return dataSize;
        */

        int bytesize = 0;
        do
        {
            // check the special input data.
            if ((!fishIDs)||(!fishCount)) {
                LOG(ERROR) << "encodeHitFish input fishIDs fishCount is invalidate, fishIDS:" << fishIDs << ", fishCount:" << fishCount;
                break;
            }

            ::rycsfish::CMD_C_HitFish hitFish;
            hitFish.set_bulletid(cmdData.bulletID);
            for (int i=0;i<fishCount;i++) {
                hitFish.add_fishids(fishIDs[i]);
            }

            // initialize special byte size.
            int bytesize = hitFish.ByteSize();
            hitFish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static bool decodeHitFish(CMD_C_HitFish& cmdData, FISH_ID* fishIDs, unsigned short& fishCount, unsigned short maxFishSize, const unsigned char* data, unsigned short dataSize)
    {
        /*
        if (dataSize < 1) {
            return false;
        }
        // unsigned char bulletID; 4
        cmdData.bulletID = *data;
        return decodeFishIDs(fishIDs, fishCount, maxFishSize, data + 1, dataSize - 1);
        */

        bool bSuccess = false;
        try
        {
            ::rycsfish::CMD_C_HitFish hitFish;
            if (hitFish.ParseFromArray(data,dataSize))
            {
                cmdData.bulletID = hitFish.bulletid();
                int size = hitFish.fishids_size();
                if ((size<0) || (size>=maxFishSize)) {
                    LOG(ERROR) << "fishIDs data error!";
                    throw;
                }

                fishCount = size;
                for (int i=0;i<fishCount;i++)
                {
                    fishIDs[i] = hitFish.fishids(i);
                }
                cmdData.hitType = hitFish.hittype();
                // Succeeded now.
                bSuccess = true;
            }

        } catch (...)
        {
            LOG(ERROR) << "decodeHitFish failed!";
        }
    //Cleanup:
        return (bSuccess);
    }

    static unsigned short encodeCatchFish(unsigned char* buffer, unsigned short bufferSize, const CMD_S_CatchFish& cmdData, const FISH_ID* fishIDs, unsigned short fishCount,const FISH_ID* hits,unsigned short hitcount)
    {
        /*
        unsigned short dataSize = 6;
        if (bufferSize < dataSize) {
            return 0;
        }
        // unsigned char bulletID; 4
        // char playerID; 4
        // BulletScoreT cannonPower; 24
        // 		memset(buffer, 0, bufferSize);
        memcpy(buffer, &cmdData.bulletID, 1);
        memcpy(buffer + 1, &cmdData.playerID, 1);
        memcpy(buffer + 2, &cmdData.cannonPower, 4);
        if (fishIDs && fishCount) {
            unsigned short tempSize = encodeFishIDs(buffer + dataSize, bufferSize - dataSize, fishIDs, fishCount);
            if (!dataSize) {
                return 0;
            }
            dataSize += tempSize;
        }

        return dataSize;
        */

        int bytesize = 0;

        do
        {
            // try to serialize the data value.
            ::rycsfish::CMD_S_CatchFish catchFish;
            catchFish.set_bulletid(cmdData.bulletID);
            catchFish.set_playerid(cmdData.playerID);
            catchFish.set_cannonpower(cmdData.cannonPower);
            for (int i=0;i<fishCount;i++)
            {
                catchFish.add_fishids(fishIDs[i]);

            }
            for(int i=0;i<hitcount;i++)
            {
                 //LOG(WARNING) << "*************************fishid:" << hits[i];
                catchFish.add_hitfishid(hits[i]);
            }

            // try to get special byte size.
            bytesize = catchFish.ByteSize();
            catchFish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeCatchFish(CMD_S_CatchFish& cmdData, FISH_ID* fishIDs, unsigned short& fishCount, unsigned short maxFishSize, const unsigned char* data, unsigned short dataSize) {
        if (dataSize < 6) {
            return false;
        }
        // unsigned char bulletID; 4
        // char playerID; 4
        // unsigned short cannonPower; 24
        // 		memset(fishIDs, 0, sizeof(FISH_ID) * maxFishSize);
        cmdData.bulletID = *data;
        cmdData.playerID = *(data + 1);
        cmdData.cannonPower=*(bit32*)(data + 2);

        return decodeFishIDs(fishIDs, fishCount, maxFishSize, data + 6, dataSize - 6);
    }
    */

    static unsigned short encodeSwitchScene(unsigned char* buffer, unsigned short bufferSize, const CMD_S_SwitchScene& cmdData)
    {
        /*
        if (bufferSize < 2) {
            return 0;
        }
        // unsigned char sceneID; 3
        // unsigned char switchTimeLength; 5
        memcpy(buffer, &cmdData.sceneID, 1);
        memcpy(buffer + 1, &cmdData.switchTimeLength, 1);

        return 2;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_SwitchScene switchScene;
            switchScene.set_sceneid(cmdData.sceneID);
            switchScene.set_switchtimelength(cmdData.switchTimeLength);
            bytesize = switchScene.ByteSize();
            switchScene.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeSwitchScene(CMD_S_SwitchScene& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 2) {
            return false;
        }
        // unsigned char sceneID;
        // unsigned char switchTimeLength;
        cmdData.sceneID = *buffer;
        cmdData.switchTimeLength = *(buffer + 1);

        return true;
    } */

    static unsigned short encodeLockFish(unsigned char* buffer, unsigned short bufferSize, const CMD_C_LockFish& cmdData)
    {
        /*
        if (bufferSize < 2) {
            return 0;
        }
        // FISH_ID fishID; 12
        memcpy(buffer, &cmdData.fishID, 2);

        return 2;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_C_LockFish lockfish;
            lockfish.set_fishid(cmdData.fishID);
            bytesize = lockfish.ByteSize();
            lockfish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static bool decodeLockFish(CMD_C_LockFish& cmdData, const unsigned char* buffer, unsigned short bufferSize)
    {
        /*
        if (bufferSize != 2) {
            return false;
        }
        // FISH_ID fishID; 12
        cmdData.fishID = *(bit16*)(buffer);

        return true;
        */

        bool bSuccess = false;
        try
        {
            if ((!buffer)||(!bufferSize)) throw;
            ::rycsfish::CMD_C_LockFish lockFish;
            if (lockFish.ParseFromArray(buffer,bufferSize))
            {
                cmdData.fishID = lockFish.fishid();
                bSuccess = true;
            }

        } catch (...)
        {
            LOG(ERROR) << "decodeLockFish error!";
        }
    //Cleanup:
        return (bSuccess);
    }

    static unsigned short encodeLockFish(unsigned char* buffer, unsigned short bufferSize, const CMD_S_LockFish& cmdData)
    {
        /*
        if (bufferSize < 3) {
            return 0;
        }
        // FISH_ID fishID; 12
        // char playerID; 3
        memcpy(buffer, &cmdData.c.fishID, 2);
        memcpy(buffer + 2, &cmdData.playerID, 1);

        return 3;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_LockFish lockfish;
            ::rycsfish::CMD_C_LockFish* cc = lockfish.mutable_c();
            cc->set_fishid(cmdData.c.fishID);
            lockfish.set_playerid(cmdData.playerID);
            bytesize = lockfish.ByteSize();
            lockfish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeLockFish(CMD_S_LockFish& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 3) {
            return false;
        }
        // FISH_ID fishID; 12
        // char playerID; 3
        cmdData.c.fishID = *(bit16*)buffer;
        cmdData.playerID = *(buffer + 2);

        return true;
    } */

    static unsigned short encodeUnlockFish(unsigned char* buffer, unsigned short bufferSize, const CMD_S_UnlockFish& cmdData)
    {
        /*
        if (bufferSize < 1) {
            return 0;
        }
        // char playerID; 3
        memcpy(buffer, &cmdData.playerID, 1);

        return 1;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_UnlockFish unlockfish;
            unlockfish.set_playerid(cmdData.playerID);
            bytesize = unlockfish.ByteSize();
            unlockfish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeUnlockFish(CMD_S_UnlockFish& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 1) {
            return false;
        }
        // char playerID; 3
        cmdData.playerID = *buffer;

        return true;
    } */

    static unsigned short encodePauseFish(unsigned char* buffer, unsigned short bufferSize, const CMD_S_PauseFish& cmdData)
    {
        /*
        if (bufferSize < 1) {
            return 0;
        }
        // unsigned char pause; 1
        memcpy(buffer, &cmdData.pause, 1);

        return 1;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_PauseFish pausefish;
            pausefish.set_pause(cmdData.pause);
            bytesize = pausefish.ByteSize();
            pausefish.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodePauseFish(CMD_S_PauseFish& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 1) {
            return false;
        }
        // unsigned char pause; 1
        cmdData.pause = *buffer;

        return true;
    } */

    static unsigned short encodeWashPercent(unsigned char* buffer, unsigned short bufferSize, const CMD_S_WashPercent& cmdData)
    {
        /*
        if (bufferSize < 2) {
            return 0;
        }
        // WashPercent percent; 8
        // char playerID; 3
        memcpy(buffer, &cmdData.percent, 1);
        memcpy(buffer + 1, &cmdData.playerID, 1);

        return 2;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_WashPercent washPercent;
            washPercent.set_washpercent(cmdData.percent);
            washPercent.set_playerid(cmdData.playerID);
            bytesize = washPercent.ByteSize();
            washPercent.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeWashPercent(CMD_S_WashPercent& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != 2) {
            return false;
        }
        // WashPercent percent; 8
        // char playerID; 3
        cmdData.percent = *buffer;
        cmdData.playerID = *(buffer + 1);

        return true;
    }*/
	static unsigned short encodeSpecialFireEnd(unsigned char* buffer, unsigned short bufferSize, const CMD_S_SpecialFireEnd& cmdData)
	{
		int bytesize = 0;
		do
		{
			::rycsfish::CMD_S_SpecialFireEnd specialFireEnd;
            specialFireEnd.set_hittype(cmdData.hitType);
			specialFireEnd.set_playerid(cmdData.playerID);
			// initialize the content value.
			bytesize = specialFireEnd.ByteSize();
			specialFireEnd.SerializeToArray(buffer, bytesize);
		} while (0);
		//Cleanup:
		return (bytesize);
	}

	static unsigned short encodeBorebitStartRotate(unsigned char* buffer, unsigned short bufferSize, const CMD_S_StartRotate& cmdData)
	{
		int bytesize = 0;
		do
		{
			::rycsfish::CMD_S_StartRotate startRotate;
			startRotate.set_bstartrotate(cmdData.bStartRotate);
			startRotate.set_chairid(cmdData.chairid);

			// initialize the content value.
			bytesize = startRotate.ByteSize();
			startRotate.SerializeToArray(buffer, bytesize);
		} while (0);
		//Cleanup:
		return (bytesize);
	}

	static bool decodeSpecialRotate(CMD_C_SpecialRotate& cmdData, const unsigned char* buffer, unsigned short bufferSize)
	{
		bool bSuccess = false;
		try
		{

			if ((!buffer) || (!bufferSize)) throw;
			::rycsfish::CMD_C_SpecialRotate specialRotate;
			if (specialRotate.ParseFromArray(buffer, bufferSize))
			{
				cmdData.angle = specialRotate.angle();
				cmdData.hitType = specialRotate.hittype();
				bSuccess = true;
			}

		}
		catch (...)
		{
			LOG(ERROR) << "decodeSpecialRotate failed!";
		}
		//Cleanup:
		return (bSuccess);
	}

	static unsigned short decodeSpecialRotate(unsigned char* buffer, unsigned short bufferSize, const CMD_S_SpecialRotate& cmdData)
	{
		int bytesize = 0;
		do
		{
			::rycsfish::CMD_S_SpecialRotate specialRotate;
			specialRotate.set_chariid(cmdData.chairid);
			specialRotate.set_hittype(cmdData.hitType);
			specialRotate.set_angle(cmdData.angle);
			// get user fire byte size now.
			bytesize = specialRotate.ByteSize();
			specialRotate.SerializeToArray(buffer, bytesize);
		} while (0);
		//Cleanup:
		return (bytesize);
	}

#ifdef __DYNAMIC_CHAIR__
    static unsigned short encodeDynamicChair(unsigned char* buffer, unsigned short bufferSize, const CMD_C_DynamicChair& cmdData)
    {
        /*
        if (bufferSize < 1) {
            return 0;
        }
        // unsigned short newChairID; 1
        memcpy(buffer, &cmdData.newChairID, 1);

        return 1;
        */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_C_DynamicChair dynamicChair;
            dynamicChair.set_newchairid(cmdData.newChairID);

            bytesize = dynamicChair.ByteSize();
            dynamicChair.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    static bool decodeDynamicChair(CMD_C_DynamicChair& cmdData, const unsigned char* buffer, unsigned short bufferSize)
    {
        /*
        if (bufferSize != 1) {
            return false;
        }
        // unsigned short newChairID; 1
        cmdData.newChairID = *buffer;

        return true;
        */

        bool bSuccess = false;
        try
        {
            if ((!buffer)||(!bufferSize)) throw;
            ::rycsfish::CMD_C_DynamicChair dynamicChair;
            if (dynamicChair.ParseFromArray(buffer,bufferSize))
            {
                cmdData.newChairID = dynamicChair.newchairid();
                bSuccess = true;
            }

        } catch (...)
        {
            LOG(ERROR) << "decodeDynamicChair data failed.";
        }
    //Cleanup:
        return (bSuccess);
    }

    static unsigned short encodeDynamicChair(unsigned char* buffer, unsigned short bufferSize, const CMD_S_DynamicChair& cmdData)
    {
        /*
        if (bufferSize < sizeof(cmdData)) {
            return 0;
        }
        //char newChairID;
        //char playerID;
        memcpy(buffer, &cmdData, sizeof(cmdData));

        return sizeof(cmdData); */

        int bytesize = 0;
        do
        {
            ::rycsfish::CMD_S_DynamicChair dynamicChair;
            dynamicChair.set_newchairid(cmdData.newChairID);
            dynamicChair.set_playerid(cmdData.playerID);
            // try to get the special data size.
            bytesize = dynamicChair.ByteSize();
            dynamicChair.SerializeToArray(buffer,bytesize);
        }   while (0);
    //Cleanup:
        return (bytesize);
    }

    /*
    static bool decodeDynamicChair(CMD_S_DynamicChair& cmdData, const unsigned char* buffer, unsigned short bufferSize) {
        if (bufferSize != sizeof(cmdData)) {
            return false;
        }
        //char newChairID;
        //char playerID;
        cmdData.newChairID = *buffer;
        cmdData.playerID = *(buffer + 1);

        return true;
    } */

#endif

protected:
    /*
    static unsigned short encodeFishIDs(unsigned char* buffer, unsigned short bufferSize, const FISH_ID* fishIDs, unsigned short fishCount)
    {
        if (!fishIDs || !fishCount || !bufferSize) {
            return 0;
        }
        unsigned short dataSize = fishCount * 2;
        memcpy(buffer, fishIDs, dataSize);

        return dataSize;
    } */


    /*
    static bool decodeFishIDs(FISH_ID* fishIDs, unsigned short& fishCount, unsigned short maxFishSize, const unsigned char* data, unsigned short dataSize) {
        if (!dataSize) {
            fishCount = 0;
            return true;
        }
        if (!maxFishSize) {
            return false;
        }
        fishCount = dataSize / 2;
        if (fishCount && dataSize % 2 == 0) {
            memcpy(fishIDs, data, dataSize);
        } else {
            return false;
        }

        return true;
    } */
};
#endif//__NetworkDataUtilitySimple_h__
