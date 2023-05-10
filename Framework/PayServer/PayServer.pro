TEMPLATE = app
CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG -= qt

#目标输出目录
DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING
DEFINED += _USE_REDLOCK_ 

#头文件目录
INCLUDEPATH += ./include
INCLUDEPATH += ../../
INCLUDEPATH += ../../public
INCLUDEPATH += ../include
INCLUDEPATH += ../include/hiredis-vip
INCLUDEPATH += ../include/Json/include
INCLUDEPATH += ../include/zookeeper
INCLUDEPATH += ../include/zookeeperclient
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/google/protobuf

# 系统根目录文件
LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lssl -lcrypto -lsnappy -pthread -lrt -lm -lz -ldl -lcpr

# libs目录文件
LIBS += $$PWD/../libs/libprotobuf.a 
LIBS += $$PWD/../libs/libzookeeper_mt.a 
LIBS += $$PWD/../libs/libJson.a 
LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += $$PWD/../libs/libhiredis-vip.a
LIBS += $$PWD/../libs/libmuduo.a

#数据库库
LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0

#源文件
SOURCES += ./src/main.cpp \
    ./src/PayServer.cpp \
    ../../public/IPFinder.cpp \
    ../../public/mymd5.cpp \
    ../../public/urlcodec.cpp \
    ../../public/base64.cpp \
    ../../public/crypto.cpp \
    ../../public/RedisClient/RedisClient.cpp \   
    ../../public/GlobalFunc.cpp \ 
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/zookeeperclient/zookeeperclient.cpp \
    ../../public/zookeeperclient/zookeeperclientutils.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/HallServer.Message.pb.cc \

#头文件
HEADERS += \
    PayServer.h \
    ../../public/IPFinder.h \
    ../../public/mymd5.h \
    ../../public/urlcodec.h \
    ../../public/base64.h \
    ../../public/crypto.h \
    ../../public/RedisClient/RedisClient.h \    
    ../../public/gameDefine.h \
    ../../public/GlobalFunc.h \
    ../../proto/Game.Common.pb.h \ 
    ../../proto/HallServer.Message.pb.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \


#DISTFILES += \
#    ../../proto/src/Game.Common.proto \
#    ../../proto/src/ProxyServer.Message.proto \
#    ../../proto/src/HallServer.Message.proto \
#    ../../proto/src/GameServer.Message.proto

unix {
    target.path = /usr/lib /usr/local/lib/
    INSTALLS += target
}

