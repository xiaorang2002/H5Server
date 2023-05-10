TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#target.path = /home/bin
#INSTALLS += target

TARGET = MatchServer

DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING

INCLUDEPATH += ../
INCLUDEPATH += ../..
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto/
INCLUDEPATH += ../include
INCLUDEPATH += ../../public/zookeeperclient
INCLUDEPATH += ../include/Json/include
INCLUDEPATH += ../include/RedisClient
INCLUDEPATH += ../thirdpart
INCLUDEPATH += ../thirdpart/hiredis-vip
INCLUDEPATH += ../../Framework/MatchServer
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += /usr/local/include/zookeeper
INCLUDEPATH += /usr/local/include/google/protobuf
INCLUDEPATH += /usr/local/include/mysql
INCLUDEPATH += /usr/local/include/mysql/jdbc
LIBS += /usr/local/lib/libprotobuf.a
#LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += /usr/local/lib/libzookeeper_mt.a

LIBS += $$PWD/../libs/libJson.a
LIBS += /usr/local/lib/libmysqlclient.a
LIBS += /usr/local/lib/libmysqlcppconn-static.a
#LIBS += $$PWD/../libs/libRedisClient.a
LIBS += $$PWD/../libs/libhiredis-vip.a

LIBS += $$PWD/../libs/libgameCrypto.a
INCLUDEPATH += ../../public/TraceMsg
LIBS += $$PWD/../libs/libmuduo.a


#INCLUDEPATH += /usr/local/mongocxx/include/bsoncxx/v_noabi
#INCLUDEPATH += /usr/local/mongocxx/include/mongocxx/v_noabi
#LIBS += -L/usr/local/mongocxx/lib64 -lbsoncxx -lmongocxx

LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0


LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl -lcpr


SOURCES += main.cpp \
    MatchServer.cpp \
    ../GameServer/AndroidUserItem.cpp \
    ../GameServer/ServerUserItem.cpp \
    ../GameServer/ServerUserManager.cpp \
    TableFrame.cpp \
    GameTableManager.cpp \
    ConnectionPool.cpp \
    AndroidUserManager.cpp \
    ../../public/RedisClient/RedisClient.cpp \
    ../../public/TraceMsg/FormatCmdId.cpp \
    ../../public/GlobalFunc.cpp \
    ../../public/GameGlobalFunc.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/ThreadLocalSingletonRedisClient.cpp \
    ../../public/TaskService.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../public/zookeeperclient/zookeeperclient.cpp \
    ../../public/zookeeperclient/zookeeperclientutils.cpp \
    ../../public/zookeeperclient/zookeeperlocker.cpp \
    ../../public/zookeeperclient/zoo_lock.c \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/MatchServer.Message.pb.cc \
    MatchRoom.cpp \
    IMatchRoom.cpp \
    MatchRoomManager.cpp

HEADERS += \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/MatchServer.Message.pb.h \
    ../../proto/Game.Common.pb.h \
    BaseServerUserItem.h \
    MatchServer.h \
    ../GameServer/ServerUserItem.h \
    ../GameServer/ServerUserManager.h \
    TableFrame.h \
    GameTableManager.h \
    AndroidTimedLimit.h \
    AndroidUserMgr.h \
    AndroidUserManager.h \
    ConnectionPool.h \
    ../GameServer/AndroidUserItem.h \
    ../../public/RedisClient/RedisClient.h \
    ../../public/TraceMsg/FormatCmdId.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/IServerUserItem.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/GlobalFunc.h \
    ../../public/GameGlobalFunc.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    ../../public/ThreadLocalSingletonRedisClient.h \
    ../../public/SubNetIP.h \
    ../../public/ConsoleClr.h \
    ../../public/zookeeperclient/zookeeperclient.h \
    ../../public/zookeeperclient/zookeeperclientutils.h \
    ../../public/zookeeperclient/zookeeperlocker.h \
    ../../public/zookeeperclient/zoo_lock.h \
    ../../public/TaskService.h \
    ../../public/FormatCmdId.h \
    MatchRoom.h \
    IMatchRoom.h \
    MatchRoomManager.h


DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/GameServer.Message.proto \
    ../../proto/src/MatchServer.Message.proto \

unix:!macx: LIBS += -lcrypto
