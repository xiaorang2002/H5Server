TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING

INCLUDEPATH += ../include
INCLUDEPATH += ../..
INCLUDEPATH += ../thirdpart
INCLUDEPATH += ../../public
INCLUDEPATH += ../../public/TraceMsg
INCLUDEPATH += ../include/hiredis-vip
INCLUDEPATH += /usr/local/include/mysql
INCLUDEPATH += /usr/local/include/mysql/jdbc

INCLUDEPATH += ../thirdpart/hiredis-vip

INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += /usr/local/include/zookeeper
LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += /usr/local/lib/libzookeeper_mt.a

INCLUDEPATH += ../thirdpart/Json/include
LIBS += $$PWD/../libs/libJson.a

#LIBS += $$PWD/../libs/libRedisClient.a
LIBS += $$PWD/../libs/libhiredis-vip.a

LIBS += $$PWD/../libs/libgameCrypto.a

LIBS += $$PWD/../libs/libmuduo.a


INCLUDEPATH += ../thirdpart/RocketMQ
LIBS += $$PWD/../libs/mq/libRocketMQ.a
LIBS += $$PWD/../libs/librocketmq.a

LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0


LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl -lcpr
#LIBS += /usr/local/lib/libmysqlclient.a
#LIBS += /usr/local/lib/libmysqlcppconn-static.a



SOURCES += main.cpp \
    ../../public/RedisClient/RedisClient.cpp \
    ../../public/RedisClient/redlock.cpp \    
    ../../public/TraceMsg/FormatCmdId.cpp \
    ../../public/GlobalFunc.cpp \
    ../../public/GameGlobalFunc.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/HallServer.Message.pb.cc \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/ThreadLocalSingletonRedisClient.cpp \
    ../../public/TaskService.cpp \
    IPFinder.cpp \  
    LuckyGame.cpp \
    HallServer.cpp


HEADERS += \
    ../../public/gameDefine.h \
    ../../public/RedisClient/RedisClient.h \
    ../../public/RedisClient/redlock.h \    
    ../../public/TraceMsg/FormatCmdId.h \
    ../../public/Globals.h \
    ../../public/GlobalFunc.h \
    ../../public/GameGlobalFunc.h \
    ../../public//gameDefine.h \
    ../../public/TaskService.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/HallServer.Message.pb.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    ../../public/ThreadLocalSingletonRedisClient.h \
    IPFinder.h \
    LuckyGame.h \
    HallServer.h


DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/GameServer.Message.proto \
    ../../proto/src/HallServer.Message.proto \




