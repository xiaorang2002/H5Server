#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T22:35:10
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TARGET = Game_gswz
TEMPLATE = lib


DESTDIR = ../../bin


DEFINES += GAME_GSWZ_LIBRARY

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


INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../proto

LIBS += $$PWD/../../Framework/libs/libmuduo.a




LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog




SOURCES +=  \
    GameLogic.cpp \
    GameProcess.cpp \
    ../../public/weights.cpp \
    ../../public/GlobalFunc.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/Gswz.Message.pb.cc

HEADERS += \
    GameLogic.h \
    GameProcess.h \
    MSG_GSWZ.h \
	../../public/weights.h \
	../../public/SubNetIP.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/GlobalFunc.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Gswz.Message.pb.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/GameServer.Message.pb.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    src/Gswz.Message.proto

