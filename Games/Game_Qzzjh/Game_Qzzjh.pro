#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TARGET = Game_qzzjh
TEMPLATE = lib


DESTDIR = ../../bin


DEFINES += GAME_QZZJH_LIBRARY

#DESTDIR = ../../bin/
#DESTDIR = /home/Landy/TianXia/Program/Game/server/bin
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../..
#INCLUDEPATH += /usr/include

INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
LIBS += $$PWD/../../Framework/libs/libmuduo.a
#LIBS += /usr/local/lib/libjsoncpp.a

LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
#LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog -I/usr/local/lib -ljsoncpp

SOURCES += \
	../QPAlgorithm/zjh.cpp \
	../QPAlgorithm/cfg.cpp \
    Log.cpp \
    TableFrameSink.cpp \
    ../../public/weights.cpp \
    ../../public/GlobalFunc.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/Qzzjh.Message.pb.cc

HEADERS += \
	../QPAlgorithm/zjh.h \
	../QPAlgorithm/cfg.h \
    Log.h \
    CMD_Game.h \
    TableFrameSink.h \
	../../public/types.h \
	../../public/INIHelp.h \
	PathMagic.h \
	../../public/Random.h \
	../../public/weights.h \
	../../public/SubNetIP.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/GlobalFunc.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/Qzzjh.Message.pb.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    ../../proto/src/Qzzjh.Message.proto


