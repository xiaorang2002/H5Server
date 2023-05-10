QT       -= core gui
CONFIG   += c++11 plugin

TARGET   = Game_ldyx
TEMPLATE = lib


DESTDIR = ../../bin

DEFINES += Game_ldyx
DEFINES += MUDUO_STD_STRING
DEFINES += QT_DEPRECATED_WARNINGS


INCLUDEPATH += ../..
INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a
 
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/thirdpart/Json/include
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a


LIBS += $$PWD/../../Framework/libs/libJson.a


LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread

SOURCES += \
    ../../proto/ldyx.Message.pb.cc \
    ../../public/GlobalFunc.cpp \
    GameLogic.cpp \
    ConfigManager.cpp \
    TableFrameSink.cpp

HEADERS += \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/ldyx.Message.pb.h \
    ../../public/GlobalFunc.h \
    GameLogic.h \
    ConfigManager.h \
    TableFrameSink.h \
    CMD_Game.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
