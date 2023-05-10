#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += c++11 plugin

TARGET   = Game_brnn
TEMPLATE = lib


DESTDIR = ../../bin

DEFINES += Game_brnn
DEFINES += MUDUO_STD_STRING




# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += /usr/local/include/google/protobuf


LIBS += /usr/local/lib/libprotobuf.a


INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a


LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread


SOURCES += \
    CTableFrameSink.cpp \
    GameLogic.cpp \
    ../../proto/Brnn.Message.pb.cc \
    ../../public/GlobalFunc.cpp \
    BrnnAlgorithm.cpp

HEADERS += \
    CTableFrameSink.h \
    globlefuction.h \
    GameLogic.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Brnn.Message.pb.h \
    ../../public/GlobalFunc.h \
    BrnnAlgorithm.h



SUBDIRS += \
    Game_Brnn.pro

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    ../../proto/src/Brnn.Message.proto






