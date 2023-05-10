--
-- Author: pengke
-- Date: 2017-03-04 13-19-26
--
NetworkDataUtilitySimple = NetworkDataUtilitySimple or {}
local fishLeaders = {}
for i=1,200 do
    local cmdData = {
		randomSeed = 0,
		elapsed = 0,
	    fishID = 0,
	    fishType = {
            type = 0,
            king = 0,
        },
	    isAlive = 0,
    }
    fishLeaders[i] = cmdData
end
local fishShoal = {}
for i=1,600 do
    local cmdData = {
		fishID = 0,
		elapsed = 0,
		shoalType = 0,
	}
    fishShoal[i] = cmdData
end

function NetworkDataUtilitySimple.decodeFishBasicData(byteArray)
    if byteArray:getAvailable() < 10 then
        return false
    end
    assert(byteArray:getAvailable() >= 10, "decode error in decodeFishBasicData(). 10/" .. byteArray:getAvailable())
    -- FishType fishType; type-5 king-4
	-- unsigned short lifeMin; 12
	-- unsigned short lifeMax; 12
	-- unsigned short boundingWidth; 11
	-- unsigned short boundingHeight; 11
	-- unsigned short fishName[200]; 0
	local cmdData = {
        fishType = {
            type = byteArray:readUByte(),
            king = byteArray:readUByte(),
        },
		lifeMin = byteArray:readUShort(),
		lifeMax = byteArray:readUShort(),
		boundingWidth = byteArray:readUShort(),
		boundingHeight = byteArray:readUShort(),
    }

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeFishShoalInfo(byteArray)
    if byteArray:getAvailable() < 7 then
        return false
    end
    assert(byteArray:getAvailable() >= 7, "decode error in decodeFishShoalInfo(). 7/" .. byteArray:getAvailable())
	-- FishType fishType; type-5 king-4
	-- unsigned short genRatio; 7
	-- unsigned short amountMin; 5
	-- unsigned short amountMax; 5
	-- char allowFollow; -- 跟随 1
	-- char allowLump; -- 鱼群 1
    local cmdData = {
        fishType = {
            type = byteArray:readUByte(),
            king = byteArray:readUByte(),
        },
	    genRatio = byteArray:readUByte(),
	    amountMin = byteArray:readUByte(),
	    amountMax = byteArray:readUByte(),
	    allowFollow = byteArray:readByte(),
	    allowLump = byteArray:readByte(),
    }

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeFishSwimmingPattern(byteArray)
    if byteArray:getAvailable() < 3 then
        return false
    end
    assert(byteArray:getAvailable() >= 3, "decode error in decodeFishSwimmingPattern(). 3/" .. byteArray:getAvailable())
	-- FishType fishType; type-5 king-4
	-- unsigned char paramDataCount; 4
	-- FishSwimmingParam swimmingParam[5];
    local succ
    local cmdData = {
        fishType = {
            type = byteArray:readUByte(),
            king = byteArray:readUByte(),
        },
        paramDataCount = byteArray:readUByte(),
        swimmingParam = {},
    }
	for i = 1, cmdData.paramDataCount do
		succ, cmdData.swimmingParam[i] = NetworkDataUtilitySimple.decodeFishSwimmingParam(byteArray)
        if not succ then
			return false
		end
	end

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeFishSwimmingParam(byteArray)
    if byteArray:getAvailable() < 1 then
        return false
    end
    assert(byteArray:getAvailable() >= 1, "decode error in decodeFishSwimmingParam(). 1/" .. byteArray:getAvailable())
	-- PathType pathType; 4
	-- short speed; 10
	-- unsigned short minLength; 10
	-- unsigned short maxLength; 10
	-- short minRotation; 9+1(符号位)
	-- short maxRotation; 9+1(符号位)
	local cmdData = {
        pathType = byteArray:readUByte(),
    }
	if CMD_Fish.PathType.ptLine == cmdData.pathType then
        if byteArray:getAvailable() < 3 then
            return false
        end
        assert(byteArray:getAvailable() >= 3, "decode error in decodeFishSwimmingParam(). 3/" .. byteArray:getAvailable())
		cmdData.speed = byteArray:readShort()
	elseif CMD_Fish.PathType.ptCurve == cmdData.pathType or CMD_Fish.PathType.ptEaseOut == cmdData.pathType then
        if byteArray:getAvailable() < 11 then
            return false
        end
        assert(byteArray:getAvailable() >= 11, "decode error in decodeFishSwimmingParam(). 11/" .. byteArray:getAvailable())
		cmdData.speed = byteArray:readShort()
		cmdData.minLength = byteArray:readUShort()
		cmdData.maxLength = byteArray:readUShort()
		cmdData.minRotation = byteArray:readShort()
		cmdData.maxRotation = byteArray:readShort()
    else
		return false
	end

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeProduceFish(byteArray, cmdData)
    if byteArray:getAvailable() < 11 then
        return false
    end
    assert(byteArray:getAvailable() >= 11, "decode error in decodeProduceFish(). 11/" .. byteArray:getAvailable())
	-- unsigned int randSeed; 32
	-- short elapsed; 16
	-- FISH_ID fishID; 12
	-- FishType fishType; type-5 king-4
	-- unsigned char isAlive; 1
	cmdData.randomSeed = byteArray:readUInt()
	cmdData.elapsed = byteArray:readShort()
	cmdData.fishID = byteArray:readUShort()
	cmdData.fishType.type = byteArray:readUByte()
    cmdData.fishType.king = byteArray:readUByte()
	cmdData.isAlive = byteArray:readUByte()

	return true
end

function NetworkDataUtilitySimple.decodeProduceFishShoal(byteArray, cmdData)
    if byteArray:getAvailable() < 5 then
        return false
    end
    assert(byteArray:getAvailable() >= 5, "decode error in decodeProduceFishShoal(). 5/" .. byteArray:getAvailable())
	-- FISH_ID fishID; 12
	-- short elapsed; 16
	-- ShoalType shoalType; 3
	cmdData.fishID = byteArray:readUShort()
	cmdData.elapsed = byteArray:readShort()
	cmdData.shoalType = byteArray:readByte()
	return true
end

function NetworkDataUtilitySimple.decodeProduceFishArray(byteArray)
    if byteArray:getAvailable() < 7 then
        return false
    end
    assert(byteArray:getAvailable() >= 7, "decode error in decodeProduceFishArray(). 7/" .. byteArray:getAvailable())
	-- unsigned int randSeed; 32
	-- FISH_ID beginFishID; 12
	-- unsigned char fishArrayID; 4
	local cmdData = {
        randomSeed = byteArray:readUInt(),
	    firstFishID = byteArray:readUShort(),
	    fishArrayID = byteArray:readUByte(),
    }

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeProduceFishes(byteArray)
    if byteArray:getAvailable() < 2 then
        return false
    end
    assert(byteArray:getAvailable() >= 2, "decode error in decodeProduceFishes(). 2/" .. byteArray:getAvailable())
	--local fishLeaders = {}
	--local fishShoal = {}
	local numberOfFishLeaders = byteArray:readUShort()
	local succ, tempData
	for i = 1, numberOfFishLeaders do
		succ = NetworkDataUtilitySimple.decodeProduceFish(byteArray, fishLeaders[i])
		if not succ then
			return false
		end
	end
    --print("numberOfFishLeaders=" .. numberOfFishLeaders)

    local numberOfFishShoals = 1
	while byteArray:getAvailable() > 0 do
        --print("numberOfFishShoals=" .. numberOfFishShoals)
		succ = NetworkDataUtilitySimple.decodeProduceFishShoal(byteArray, fishShoal[numberOfFishShoals])
		if not succ then
			return false
		end
        numberOfFishShoals = numberOfFishShoals + 1
	end
    --print("decodeProduceFishes end.")
	return true, fishLeaders, fishShoal, numberOfFishLeaders, numberOfFishShoals - 1
end

function NetworkDataUtilitySimple.decodeGameScene(gameConfig, byteArray)
    if byteArray:getAvailable() < 3 then
        return false
    end
    assert(byteArray:getAvailable() >= 3, "decode error in decodeGameScene(). 3/" .. byteArray:getAvailable())
	local fishBasicDataCount = byteArray:readUByte()
	local fishSwimmingPatternCount = byteArray:readUByte()
	local fishShoalInfoCount = byteArray:readUByte()
	local succ, tempData
	for i = 1, fishBasicDataCount do
		succ, tempData = NetworkDataUtilitySimple.decodeFishBasicData(byteArray)
        if not succ then
			return false
		end
		g_FishBasicDataManager:getInstance():setFishBasicData(tempData)
	end

	for i = 1, fishSwimmingPatternCount do
		succ, tempData = NetworkDataUtilitySimple.decodeFishSwimmingPattern(byteArray)
        if not succ then
			return false
		end
		g_FishSwimmingPatternManager:getInstance():setFishSwimmingPattern(tempData)
	end

	for i = 1, fishShoalInfoCount do
		succ, tempData = NetworkDataUtilitySimple.decodeFishShoalInfo(byteArray)
        if not succ then
        	return false
		end
		g_FishShoalInfoManager:getInstance():setFishShoalInfo(tempData)
	end
	-- CMD_S_GameConfig gameConfig;
	-- CMD_S_ProduceFishArray fishArray;
	-- unsigned char sceneID; 3
	-- unsigned char pause; 1
    succ = NetworkDataUtilitySimple.decodeGameConfig(gameConfig, byteArray)
    if not succ then
        return false
	end
    local cmdData = {}

	succ, cmdData.fishArray = NetworkDataUtilitySimple.decodeProduceFishArray(byteArray)
	if not succ then
		return false
	end
    if byteArray:getAvailable() < 2 then
        return false
    end
    assert(byteArray:getAvailable() >= 2, "decode error in decodeGameScene(). 2/" .. byteArray:getAvailable())
	cmdData.sceneID = byteArray:readUByte()
	cmdData.pause = byteArray:readUByte()
	local succFish, leaders, shoal, numberOfFishLeaders, numberOfFishShoals = NetworkDataUtilitySimple.decodeProduceFishes(byteArray)
	if not succFish then
		return false
	end
    if byteArray:getAvailable() ~= 0 then
        return false
    end
    assert(byteArray:getAvailable() == 0, "decode error in decodeGameScene(). 0/" .. byteArray:getAvailable())
	return true, cmdData, leaders, shoal, numberOfFishLeaders, numberOfFishShoals
end

function NetworkDataUtilitySimple.decodeGameConfig(gameConfig, byteArray)
    if byteArray:getAvailable() < CMD_Fish.BUFFER_SIZE_OF_GAME_CONFIG then
        return false
    end
    assert(byteArray:getAvailable() >= CMD_Fish.BUFFER_SIZE_OF_GAME_CONFIG, "decode error in decodeGameConfig(). BUFFER_SIZE_OF_GAME_CONFIG/" .. byteArray:getAvailable())
    local function readCirboomParam()
        return {
            delay = byteArray:readFloat(),
	        fish_type = byteArray:readUByte(),
	        count = byteArray:readUByte(),
	        move_speed = byteArray:readUShort(),
	        radius = byteArray:readUShort(),
        }
    end
    local function readCircle2Param()
        return {
	        fish_type = byteArray:readUByte(),
	        count = byteArray:readUByte(),
	        move_speed = byteArray:readUShort(),
	        radius = byteArray:readUShort(),
            order = byteArray:readUByte(),
        }
    end
    local function readCrossoutParam()
        return {
	        fish_type = byteArray:readUByte(),
	        count = byteArray:readUByte(),
	        radius = byteArray:readUShort(),
        }
    end
    local function readCenterOutParam()
        return {
	        fish_type = byteArray:readUByte(),
	        count = byteArray:readUByte(),
	        dirs = byteArray:readUByte(),
	        move_speed = byteArray:readUShort(),
            interval = byteArray:readFloat(),
        }
    end
    local function readFishType()
        return {
	        type = byteArray:readUByte(),
	        king = byteArray:readUByte(),
        }
    end
    local function readCollectData()
        return {
	        collectCount = byteArray:readUByte(),
	        collectFishId = {
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
            },
            collectStats = {
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
            },
        }
    end
    local function readMsyqData()
        return {
            msyqPercent = {
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
                byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(), byteArray:readUByte(),
            },
        }
    end
    local function readPplData()
        return {
            isPlaying = byteArray:readUByte(),
            hits = byteArray:readUShort(),
        }
    end
    local function readPplXmlConfig()
        return {
            fishType = byteArray:readUByte(),
            fishLife = byteArray:readUShort(),
        }
    end
    local function readInt64()
        return byteArray:readUInt() + byteArray:readUInt() * 4294967296
    end
	gameConfig.version = byteArray:readUInt() -- 版本号.
	gameConfig.ticketRatio = byteArray:readUShort() -- 当前机器的投币比例(渔币).
	gameConfig.bulletStrengtheningScale = byteArray:readUInt() -- 加炮幅度.
	gameConfig.cannonPowerMin = byteArray:readUInt() -- 最小炮数.
	gameConfig.cannonPowerMax = byteArray:readUInt() -- 最大炮数.
	gameConfig.bulletSpeed = byteArray:readUShort() -- 子弹飞行速度.
	gameConfig.consoleType = byteArray:readUShort() -- 机台类型.
	gameConfig.prizeScore= byteArray:readUInt() -- 爆机分数.
	gameConfig.exchangeRatioUserScore = byteArray:readUShort() -- 兑换比率(金币).
	gameConfig.exchangeRatioFishScore = byteArray:readUShort() -- 兑换比率(渔币).
	gameConfig.decimalPlaces = byteArray:readUByte() -- 小数位数
    --[[if GlobalPlatInfo.isInReview then
        gameConfig.decimalPlaces = 0
    end]]
	gameConfig.exchangeFishScore = {
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
	}
	gameConfig.fishScore = {
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
		readInt64(),
	}
	gameConfig.isExperienceRoom = byteArray:readUByte()
	gameConfig.isAntiCheatRoom = byteArray:readUByte()
	gameConfig.timeOutQuitCheck = byteArray:readUShort()
	gameConfig.timeOutQuitMsg = byteArray:readUShort()
	gameConfig.fishLockable = byteArray:readUByte()
	gameConfig.fishArrayParam = {
		param1 = {
			small_fish_type = byteArray:readUByte(),
			small_fish_count = byteArray:readShort(),
			move_speed_small = byteArray:readShort(),
			move_speed_big = byteArray:readShort(),
			space = byteArray:readShort(),
			big_fish_kind_count = {
				byteArray:readUByte(),
				byteArray:readUByte(),
			},
			big_fish = {
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
			},
			big_fish_time = byteArray:readShort(),
		},
		param2 = {
			small_fish_type = byteArray:readUByte(),
			small_fish_count = byteArray:readShort(),
			move_speed_small = byteArray:readShort(),
			move_speed_big = byteArray:readShort(),
			big_fish_count = byteArray:readUByte(),
			big_fish_kind_count = byteArray:readUByte(),
			big_fish = {
				readFishType(), readFishType(), readFishType(),
				readFishType(), readFishType(), readFishType(),
				readFishType(), readFishType(), readFishType(),
				readFishType(), readFishType(), readFishType(),
				readFishType(), readFishType(), readFishType(),
			},
			big_fish_time_interval = byteArray:readUShort(),
			small_fish_time_interval = byteArray:readUShort(),
		},
		param3 = {
			rot_angles = byteArray:readShort(),
			rot_speed = byteArray:readShort(),
			time_interval = byteArray:readFloat(),
			param_count = byteArray:readUByte(),
			param = {
				readCirboomParam(), readCirboomParam(), readCirboomParam(), readCirboomParam(),
				readCirboomParam(), readCirboomParam(), readCirboomParam(), readCirboomParam(),
				readCirboomParam(), readCirboomParam(),
			}
		},
		param4 = {
			param_count = byteArray:readUByte(),
			param = {
				readCirboomParam(), readCirboomParam(), readCirboomParam(), readCirboomParam(),
				readCirboomParam(), readCirboomParam(), readCirboomParam(), readCirboomParam(),
				readCirboomParam(), readCirboomParam(),
			}
		},
		param5 = {
			rot_angles = byteArray:readShort(),
			rot_speed = byteArray:readShort(),
			time_interval = byteArray:readFloat(),
			param_count = {
				byteArray:readUByte(),
				byteArray:readUByte(),
			},
			param = {
				{
					readCircle2Param(), readCircle2Param(), readCircle2Param(),
					readCircle2Param(), readCircle2Param(),
				},
				{
					readCircle2Param(), readCircle2Param(), readCircle2Param(),
					readCircle2Param(), readCircle2Param(),
				},
			},
		},
		param6 = {
			move_speed = byteArray:readShort(),
			max_radius = {
				byteArray:readShort(),
				byteArray:readShort(),
			},
			param_count = {
				byteArray:readUByte(),
				byteArray:readUByte(),
			},
			param = {
				{
					readCrossoutParam(), readCrossoutParam(), readCrossoutParam(),
					readCrossoutParam(), readCrossoutParam(),
				},
				{
					readCrossoutParam(), readCrossoutParam(), readCrossoutParam(),
					readCrossoutParam(), readCrossoutParam(),
				},
			},
		},
		param7 = {
			param_count = byteArray:readUByte(),
			param = {
				readCenterOutParam(), readCenterOutParam(), readCenterOutParam(), readCenterOutParam(),
				readCenterOutParam(), readCenterOutParam(), readCenterOutParam(), readCenterOutParam(),
			},
			fish_time_delay = byteArray:readFloat(),
			small_fish_time_interval = byteArray:readFloat(),
		},
		param8 = {
			small_fish_type = byteArray:readUByte(),
			small_fish_count = byteArray:readUShort(),
			move_speed_small = byteArray:readUShort(),
			move_speed_big = byteArray:readShort(),
			big_fish_kind_count = {
				byteArray:readUByte(),
				byteArray:readUByte(),
				byteArray:readUByte(),
				byteArray:readUByte(),
			},
			big_fish = {
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
				{
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
					readFishType(), readFishType(), readFishType(),
				},
			},
			big_fish_time_interval = byteArray:readFloat(),
			small_fish_time_interval = byteArray:readFloat(),
		},
	}
	gameConfig.supportedGameplays = byteArray:readUByte() -- 支持的玩法ID，最多八个玩法
	gameConfig.collectData = readCollectData()
	gameConfig.msyqData = readMsyqData()
	gameConfig.pplData = {
		readPplData(), readPplData(), readPplData(), readPplData(),
		readPplData(), readPplData(), readPplData(), readPplData(),
	}
	gameConfig.pplXmlConfig = readPplXmlConfig()
    return true
end

function NetworkDataUtilitySimple.encodeUserFire(cmdData)
	local byteArray = ByteArray.new()
	-- FISH_ID lockedFishID; 12
	-- BulletScoreT cannonPower; 22
	-- short angle; 9+1(符号位)
	-- char bulletID; 4
	byteArray:writeUShort(cmdData.lockedFishID)
	byteArray:writeInt(cmdData.cannonPower)
	byteArray:writeShort(cmdData.angle)
	byteArray:writeUByte(cmdData.bulletID)
	byteArray:setPos(1)

	return byteArray
end

function NetworkDataUtilitySimple.decodeUserFire(byteArray)
    if byteArray:getAvailable() ~= 10 then
        return false
    end
    assert(byteArray:getAvailable() == 10, "decode error in decodeUserFire(). 10/" .. byteArray:getAvailable())
	-- FISH_ID lockedFishID; 12
	-- BulletScoreT cannonPower; 22
	-- short angle; 9+1(符号位)
	-- char bulletID; 4
	-- playerID; 3
	local cmdData = {
		c = {
			lockedFishID = byteArray:readUShort(),
			cannonPower = byteArray:readInt(),
			angle = byteArray:readShort(),
			bulletID = byteArray:readByte(),
		},
		playerID = byteArray:readByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.encodeGameplay(cmdData)
	local byteArray = ByteArray.new()
	-- unsigned char gameplayID; 3
	-- unsigned char commandType; 5
	byteArray:writeUByte(cmdData.gameplayID)
	byteArray:writeUByte(cmdData.commandType)

	return byteArray
end

function NetworkDataUtilitySimple.decodeGameplay(byteArray)
    if byteArray:getAvailable() < 3 then
        return false
    end
    assert(byteArray:getAvailable() >= 3, "decode error in decodeGameplay(). 3/" .. byteArray:getAvailable())
	-- unsigned char gameplayID; 3
	-- unsigned char commandType; 5
	-- playerID; 3
	local cmdData = {
		c = {
			gameplayID = byteArray:readUByte(),
			commandType = byteArray:readUByte(),
		},
		playerID = byteArray:readByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.encodeScoreUp(cmdData)
    local byteArray = ByteArray.new()
	-- char inc; 1
	byteArray:writeByte(cmdData.inc)
    byteArray:setPos(1)

	return byteArray
end

function NetworkDataUtilitySimple.decodeScoreUp(byteArray)
    if byteArray:getAvailable() ~= 6 then
        return false
    end
    assert(byteArray:getAvailable() == 6, "decode error in decodeScoreUp(). 6/" .. byteArray:getAvailable())
	-- int score; 32
	-- char type; 4
	-- char playerID; 3
	local cmdData = {
		score = byteArray:readInt(),
		type = byteArray:readByte(),
		playerID = byteArray:readByte(),
	}

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeExchangeScore(byteArray)
    if byteArray:getAvailable() ~= 10 then
        return false
    end
    assert(byteArray:getAvailable() == 10, "decode error in decodeExchangeScore(). 10/" .. byteArray:getAvailable())
	-- int score; 32
	-- char type; 4
	-- char playerID; 3
	local cmdData = {
		score = byteArray:readUInt() + byteArray:readUInt() * 4294967296,
		inc = byteArray:readByte(),
		playerID = byteArray:readByte(),
	}

	return true, cmdData
end

function NetworkDataUtilitySimple.encodeHitFish(cmdData, fishIDs, fishCount)
	local byteArray = ByteArray.new()
	-- unsigned char bulletID; 4
	byteArray:writeUByte(cmdData.bulletID)
    for i=1,fishCount do
        byteArray:writeUShort(fishIDs[i])
    end
    byteArray:setPos(1)
--	NetworkDataUtilitySimple.encodeFishIDs(byteArray, fishIDs, fishCount)
	return byteArray
end

function NetworkDataUtilitySimple.decodeCatchFish(byteArray, fishIDs)
    if byteArray:getAvailable() < 6 then
        return false
    end
    assert(byteArray:getAvailable() >= 6, "decode error in decodeCatchFish(). 6/" .. byteArray:getAvailable())
	-- unsigned char bulletID; 4
	-- char playerID; 4
	-- unsigned short cannonPower; 24
-- 		memset(fishIDs, 0, sizeof(FISH_ID) * maxFishSize);
	local cmdData = {
		bulletID = byteArray:readByte(),
		playerID = byteArray:readByte(),
		cannonPower = byteArray:readInt(),
	}
	return true, cmdData, NetworkDataUtilitySimple.decodeFishIDs(byteArray, fishIDs)
end

function NetworkDataUtilitySimple.decodeSwitchScene(byteArray)
    if byteArray:getAvailable() ~= 2 then
        return false
    end
    assert(byteArray:getAvailable() == 2, "decode error in decodeSwitchScene(). 2/" .. byteArray:getAvailable())
	-- unsigned char sceneID;
	-- unsigned char switchTimeLength;
	local cmdData = {
		sceneID = byteArray:readUByte(),
		switchTimeLength = byteArray:readUByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.encodeLockFish(cmdData)
	-- FISH_ID fishID; 12
	local byteArray = ByteArray.new()
	byteArray:writeUShort(g_CommonUtility:RBITS(cmdData.fishID, 12))
	byteArray:setPos(1)

	return byteArray
end

function NetworkDataUtilitySimple.decodeLockFish(byteArray)
    if byteArray:getAvailable() ~= 3 then
        return false
    end
    assert(byteArray:getAvailable() == 3, "decode error in decodeLockFish(). 3/" .. byteArray:getAvailable())
	-- FISH_ID fishID; 12
	-- char playerID; 3
	local cmdData = {
		c = {
			fishID = byteArray:readUShort(),
		},
		playerID = byteArray:readByte(),
	}

	return true, cmdData
end

function NetworkDataUtilitySimple.decodeUnlockFish(byteArray)
    if byteArray:getAvailable() ~= 1 then
        return false
    end
    assert(byteArray:getAvailable() == 1, "decode error in decodeUnlockFish(). 1/" .. byteArray:getAvailable())
	-- char playerID; 3
	local cmdData = {
		playerID = byteArray:readByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.decodePauseFish(byteArray)
    if byteArray:getAvailable() ~= 1 then
        return false
    end
    assert(byteArray:getAvailable() == 1, "decode error in decodePauseFish(). 1/" .. byteArray:getAvailable())
	-- unsigned char pause; 1
	local cmdData = {
		pause = byteArray:readUByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.decodeWashPercent(byteArray)
    if byteArray:getAvailable() ~= 2 then
        return false
    end
    assert(byteArray:getAvailable() == 2, "decode error in decodeWashPercent(). 2/" .. byteArray:getAvailable())
	-- WashPercent percent; 8
	-- char playerID; 3
	local cmdData = {
		percent = byteArray:readUByte(),
		playerID = byteArray:readByte(),
	}
	return true, cmdData
end

function NetworkDataUtilitySimple.encodeFishIDs(byteArray, fishIDs, fishCount)
	if fishCount < 1 then
        byteArray:setPos(1)
		return true
	end

    for i=1,fishCount do
        byteArray:writeUShort(fishIDs[i])
    end
    byteArray:setPos(1)
	return true
end

function NetworkDataUtilitySimple.decodeFishIDs(byteArray, fishIDs)
	local dataSize = byteArray:getAvailable()
    local fishCount = dataSize / 2
    if fishCount > 0 and dataSize % 2 == 0 then
        for i = 1, fishCount do
        	fishIDs[i] = byteArray:readUShort()
        end
    end

	return fishCount
end