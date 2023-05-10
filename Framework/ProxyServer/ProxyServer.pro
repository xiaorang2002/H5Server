TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING


INCLUDEPATH += ../..
INCLUDEPATH += ../thirdpart
INCLUDEPATH += ../thirdpart/hiredis-vip

INCLUDEPATH += /usr/local/include/boost

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


#INCLUDEPATH += ../thirdpart/RocketMQ
#LIBS += $$PWD/../libs/mq/libRocketMQ.a
#LIBS += $$PWD/../libs/librocketmq.a


#INCLUDEPATH += /usr/local/mongocxx/include/bsoncxx/v_noabi
#INCLUDEPATH += /usr/local/mongocxx/include/mongocxx/v_noabi
#LIBS += -L/usr/local/mongocxx/lib64 -lbsoncxx -lmongocxx
#INCLUDEPATH += ../include/RocketMQ

INCLUDEPATH += ../include



LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0
#LIBS += $$PWD/../libs/libmongocxx.so.3.4.0
#LIBS += $$PWD/../libs/libbsoncxx.so.3.4.0



LIBS += -lboost_filesystem -lboost_system
LIBS +=  -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl -lcpr -lboost_regex


SOURCES += main.cpp \
    ProxyServer.cpp \
    ../../public/RedisClient/RedisClient.cpp \  
    ../../public/RedisClient/redlock.cpp \      
    ../../public/MyEventLoopThreadPool.cc \
    ../../public/GlobalFunc.cpp \
    ../../public/base64.cpp \
    ../../public/GameGlobalFunc.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/HallServer.Message.pb.cc \
    ../../proto/ProxyServer.Message.pb.cc \
    ../../proto/GameServer.Message.pb.cc

HEADERS += \
    ProxyServer.h \
    ../../public/RedisClient/RedisClient.h \
    ../../public/RedisClient/redlock.h \    
    ../../public/MyEventLoopThreadPool.h \
    ../../public/gameDefine.h \
    ../../public/GlobalFunc.h \
    ../../public/base64.h \
    ../../public/GameGlobalFunc.h \
    ../../public/Globals.h \
    ../../public/ThreadLocalMongoDBClient.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/HallServer.Message.pb.h \
    ../../proto/ProxyServer.Message.pb.h \
    ../../proto/GameServer.Message.pb.h

DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/ProxyServer.Message.proto \
    ../../proto/src/HallServer.Message.proto \
    ../../proto/src/GameServer.Message.proto





