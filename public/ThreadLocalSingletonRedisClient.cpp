// Copyright (c) 2019, Landy
// All rights reserved.

#include "ThreadLocalSingletonRedisClient.h"


using namespace std;

__thread RedisClient* ThreadLocalSingletonRedisClient::t_value_ = 0;
typename ThreadLocalSingletonRedisClient::Deleter ThreadLocalSingletonRedisClient::deleter_;
string ThreadLocalSingletonRedisClient::m_mongoDBConntionString = "";

