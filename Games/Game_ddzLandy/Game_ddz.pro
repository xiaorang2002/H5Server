#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TARGET = Game_ddz
TEMPLATE = lib

DEFINES += MUDUO_STD_STRING

DEFINES += Game_ddz
DESTDIR = ../../bin

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += /usr/local/include/linux_include
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
#INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
#INCLUDEPATH += ../../Framework/include/muduo/
#INCLUDEPATH += ../../Framework/include/crypto/
INCLUDEPATH += ../..

LIBS += -ldl -lpthread -lglog
LIBS += -lcrypto -lssl

LIBS += /usr/local/include/linux_include/xml/libtinyxml.a
LIBS += $$PWD/../../Framework/libs/libgameCrypto.a
LIBS += $$PWD/../../Framework/libs/libmuduo.a
LIBS += /usr/local/lib/libuuid.a

INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

SOURCES += \
    Log.cpp \
    ../../public/weights.cpp \
    GameLogic.cpp \
    HistoryScore.cpp \
    TableFrameSink.cpp \
    ../../proto/ddz.Message.pb.cc

HEADERS += \
    Log.h \
    globlefuction.h \
    GameLogic.h \
    HistoryScore.h \
    CMD_Game.h \
    CString.h \
    ../../public/weights.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../GameServer/ServerUserItem.h \
    TableFrameSink.h \
#   ../../../proto/Game.Common.pb.h \
#    ../../proto/GameServer.Message.pb.h \
    ../../proto/ddz.Message.pb.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}

#LIBS += $$PWD/../../libs/libgameCrypto.a
#LIBS += $$PWD/../../libs/libmuduo.a
#LIBS += $$PWD/../../libs/libprotobuf.a

SUBDIRS += \
    Game_ddz.pro


