TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#target.path = /home/bin
#INSTALLS += target



DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING


INCLUDEPATH += ../..
INCLUDEPATH += ../../public
INCLUDEPATH += ../thirdpart
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../public/zookeeperclient
INCLUDEPATH += ../../public/TraceMsg
INCLUDEPATH += /usr/local/include/zookeeper
INCLUDEPATH += /usr/local/include/google/protobuf
INCLUDEPATH += ../thirdpart/hiredis-vip
INCLUDEPATH += /usr/local/include/mysql
INCLUDEPATH += /usr/local/include/mysql/jdbc
#INCLUDEPATH += /usr/local/include/hiredis

LIBS += /usr/local/lib/libprotobuf.a
#LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += /usr/local/lib/libzookeeper_mt.a
LIBS += /usr/local/lib/libmysqlclient.a
LIBS += /usr/local/lib/libmysqlcppconn-static.a
#LIBS += /usr/local/lib/libhiredis.a

INCLUDEPATH += ../thirdpart/Json/include
LIBS += $$PWD/../libs/libJson.a

#LIBS += $$PWD/../libs/libRedisClient.a
LIBS += $$PWD/../libs/libhiredis-vip.a

LIBS += $$PWD/../libs/libgameCrypto.a

LIBS += $$PWD/../libs/libmuduo.a


#INCLUDEPATH += /usr/local/mongocxx/include/bsoncxx/v_noabi
#INCLUDEPATH += /usr/local/mongocxx/include/mongocxx/v_noabi
#LIBS += -L/usr/local/mongocxx/lib64 -lbsoncxx -lmongocxx
INCLUDEPATH += ../include

LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0

#LIBS += $$PWD/../libs/libmongocxx.so.3.4.0
#LIBS += $$PWD/../libs/libbsoncxx.so.3.4.0

LIBS += $$PWD/../libs/libmongocxx.so
LIBS += $$PWD/../libs/libbsoncxx.so


LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl -lcpr


SOURCES += main.cpp \
    GameServer.cpp \
    ServerUserItem.cpp \
    ServerUserManager.cpp \
    TableFrame.cpp \
    GameTableManager.cpp \
    AndroidUserItem.cpp \
    AndroidUserManager.cpp \
    ConnectionPool.cpp \
    ../../public/TaskService.cpp \
    ../../public/GlobalFunc.cpp \
    ../../public/GameGlobalFunc.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/ThreadLocalSingletonRedisClient.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../public/zookeeperclient/zookeeperclient.cpp \
    ../../public/zookeeperclient/zookeeperclientutils.cpp \
    ../../public/zookeeperclient/zookeeperlocker.cpp \
    ../../public/zookeeperclient/zoo_lock.c \
    ../../public/RedisClient/RedisClient.cpp \
    ../../public/RedisClient/redlock.cpp \
    ../../public/TraceMsg/FormatCmdId.cpp \
    ../../proto/GameServer.Message.pb.cc

HEADERS += \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/Game.Common.pb.h \
    BaseServerUserItem.h \
    GameServer.h \
    GameServerDefine.h \
    ServerUserItem.h \
    ServerUserManager.h \
    TableFrame.h \
    GameTableManager.h \
    AndroidTimedLimit.h \
    AndroidUserMgr.h \
    AndroidUserManager.h \
    AndroidUserItem.h \
    ConnectionPool.h \
    ../../public/TaskService.h \
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
    ../../public/zookeeperclient/zookeeperclient.h \
    ../../public/zookeeperclient/zookeeperclientutils.h \
    ../../public/zookeeperclient/zookeeperlocker.h \
    ../../public/zookeeperclient/zoo_lock.h \
    ../../public/RedisClient/RedisClient.h \
    ../../public/RedisClient/redlock.h \
    ../../public/TraceMsg/FormatCmdId.h

DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/GameServer.Message.proto \


#unix:!macx: LIBS += -lcryptopp
