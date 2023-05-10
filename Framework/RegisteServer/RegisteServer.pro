TEMPLATE = app
CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING
DEFINED += _USE_REDLOCK_
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

LIBS +=-pthread -lrt -lm

LIBS += $$PWD/../libs/libmuduo.a
LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lssl -lcrypto -lsnappy -pthread -lrt -lm -lz -ldl -lcpr

INCLUDEPATH += ../../
INCLUDEPATH += ../../public
INCLUDEPATH += ../include
INCLUDEPATH += ../include/hiredis-vip
INCLUDEPATH += ../include/Json/include

INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a
LIBS += $$PWD/../libs/libJson.a

#LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0
LIBS += $$PWD/../libs/libmongocxx.so
LIBS += $$PWD/../libs/libbsoncxx.so

LIBS += $$PWD/../libs/libgameCrypto.a
INCLUDEPATH += /usr/local/include
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
    RegisteServer.cpp \
    ../../public/IPFinder.cpp \
    ../../public/mymd5.cpp \
    ../../public/urlcodec.cpp \
    ../../public/base64.cpp \
    ../../public/crypto.cpp \
    ../../public/RedisClient/RedisClient.cpp \   
    ../../public/GlobalFunc.cpp \
    #../../public/RedisClient/redlock.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/zookeeperclient/zookeeperclient.cpp \
    ../../public/zookeeperclient/zookeeperclientutils.cpp \
    ../../proto/Game.Common.pb.cc \

HEADERS += \
    RegisteServer.h \
    ../../public/IPFinder.h \
    ./../public/AsciiEntity.hpp \
    ../../public/mymd5.h \
    ../../public/urlcodec.h \
    ../../public/base64.h \
    ../../public/crypto.h \
    ../../public/RedisClient/RedisClient.h \
    #../../public/RedisClient/redlock.h \     
    ../../public/gameDefine.h \
    ../../public/GlobalFunc.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    ../../proto/Game.Common.pb.h \
DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/ProxyServer.Message.proto \
    ../../proto/src/HallServer.Message.proto \
    ../../proto/src/GameServer.Message.proto


unix {
    target.path = /usr/lib
    INSTALLS += target
}

