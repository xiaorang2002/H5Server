QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TARGET = Game_tbnn
TEMPLATE = lib


DESTDIR = ../../bin

DEFINES += Game_tbnn_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING


INCLUDEPATH += ../..


INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a

INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto

LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog

SOURCES += \
    Log.cpp \
    GameLogic.cpp \
    TableFrameSink.cpp \
    ../../public/GlobalFunc.cpp \
	../../public/weights.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/Tbnn.Message.pb.cc \
    TestCase.cpp

HEADERS += \
    Log.h \
    GameLogic.h \
    CMD_Game.h \
    TableFrameSink.h \
    ../../public/Globals.h \
    ../../public/GlobalFunc.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/gameDefine.h \
	../../public/weights.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/Tbnn.Message.pb.h \
    TestCase.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}


DISTFILES += \
    ../../proto/src/Tbnn.Message.proto
