// Copyright (c) 2019, Landy
// All rights reserved.

#include "ThreadLocalSingletonMongoDBClient.h"


using namespace std;

__thread mongocxx::client* ThreadLocalSingletonMongoDBClient::t_value_ = 0;
typename ThreadLocalSingletonMongoDBClient::Deleter ThreadLocalSingletonMongoDBClient::deleter_;
string ThreadLocalSingletonMongoDBClient::m_mongoDBConntionString = "";
mongocxx::options::client ThreadLocalSingletonMongoDBClient::m_options;
