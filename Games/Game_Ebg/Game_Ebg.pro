#-------------------------------------------------
#
# Project created by QtCreator 2018-07-04T19:08:30
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin
TARGET = Game_ebg
TEMPLATE = lib
DEFINES += GAME_EBG_LIBRARY

DESTDIR = ../../bin

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING



INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
INCLUDEPATH += ../../Aics/linux-include
INCLUDEPATH += ../../Aics/publics
LIBS += $$PWD/../../Framework/libs/libmuduo.a
#LIBS += $$PWD/../../Framework/libs/libprotobuf.a



LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog -lcurl
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread

INCLUDEPATH += ../../Framework/thirdpart/Json/include
LIBS += $$PWD/../../Framework/libs/libJson.a

# You can also make your code fail to compile if you use deprecated APIs.////
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TableFrameSink.cpp \
    betarea.cpp \
    bankemanager.cpp \
    ../../proto/ErBaGang.Message.pb.cc \
    ../../public/GlobalFunc.cpp \
    cardlogic.cpp \
    EbgAlgorithm.cpp

HEADERS += \
    game_brnn_global.h \
    TableFrameSink.h \
    betarea.h \
    bankemanager.h \
    ../../proto/ErBaGang.Message.pb.h \
    INIHelp.h \
    types.h \
    cardlogic.h\
    ../../public/GlobalFunc.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    EbgAlgorithm.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    ../../proto/src/ErBaGang.Message.proto


