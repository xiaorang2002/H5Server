#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET   = Game_hhdn
TEMPLATE = lib


DESTDIR = ../../bin

DEFINES += Game_hhdn

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
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
INCLUDEPATH += ../../Aics/linux-include
INCLUDEPATH += ../../Aics/publics
#INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a


LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread


SOURCES += \
    CTableFrameSink.cpp \
    GameLogic.cpp \
    HhdnAlgorithm.cpp \
    ../../proto/Hhdn.Message.pb.cc\
    ../../public/GlobalFunc.cpp

HEADERS += \
    CTableFrameSink.h \
    GameLogic.h \
    CMD_Game.h \
    IAicsRedxBlack.h \
    globlefuction.h \
    HhdnAlgorithm.h \
    ../../proto/Hhdn.Message.pb.h \
    types.h \
    IAicsRedxBlack.h \
    ../../public/Globals.h \
    ../../public/GlobalFunc.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../GameServer/ServerUserItem.h


SUBDIRS += \
    Game_HongHeiDouNiu.pro\


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    ../../proto/Common.Message.proto \
    ../../proto/src/Hhdn.Message.proto




