TEMPLATE = app
CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = ../../bin
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

LIBS +=-pthread -lrt -lm

LIBS += $$PWD/../libs/libmuduo.a
LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread  -lboost_regex
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl  -lcpr

INCLUDEPATH += ../..
INCLUDEPATH += ../include
INCLUDEPATH += ../include/Json/include
INCLUDEPATH += ../../public
INCLUDEPATH += ../thirdpart/hiredis-vip

INCLUDEPATH += /usr/local /include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a
LIBS += $$PWD/../libs/libJson.a
#LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0

LIBS += $$PWD/../libs/libmongocxx.so
LIBS += $$PWD/../libs/libbsoncxx.so

LIBS += $$PWD/../libs/libgameCrypto.a

#LIBS += $$PWD/../libs/libmongocxx.so.3.4.0
#LIBS += $$PWD/../libs/libbsoncxx.so.3.4.0

INCLUDEPATH += /usr/local/include/zookeeper
LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += /usr/local/lib/libzookeeper_mt.a

#LIBS += $$PWD/../libs/libRedisClient.a
LIBS += $$PWD/../libs/libhiredis-vip.a
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    HttpServer.cc \
    ../../public/GlobalFunc.cpp \
    ../../public/RedisClient/RedisClient.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/HallServer.Message.pb.cc \
    ../../proto/ProxyServer.Message.pb.cc \
    ../../proto/GameServer.Message.pb.cc

HEADERS += \
    HttpServer.h \
    ../../public/gameDefine.h \
    ../../public/GlobalFunc.h \
    ../../public/RedisClient/RedisClient.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/HallServer.Message.pb.h \
    ../../proto/ProxyServer.Message.pb.h \
    ../../proto/GameServer.Message.pb.h

DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/ProxyServer.Message.proto \
    ../../proto/src/HallServer.Message.proto \
    ../../proto/src/GameServer.Message.proto

