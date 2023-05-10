#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TARGET = Game_qzxszsg
TEMPLATE = lib



DEFINES += Game_qzxszsg
DESTDIR = ../../bin

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
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
LIBS += $$PWD/../../Framework/libs/libmuduo.a


LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread



SOURCES += \
    Log.cpp \
    GameLogic.cpp \
    TableFrameSink.cpp \
	../../public/GlobalFunc.cpp \
	../../public/weights.cpp \
    ../../proto/QZXszSg.Message.pb.cc

HEADERS += \
    Log.h \
    globlefuction.h \
    GameLogic.h \
    CMD_Game.h \
    TableFrameSink.h \
	../../public/Globals.h \
	../../public/weights.h \
    ../../public/GlobalFunc.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Sources/GameServer/ServerUserItem.h \
    ../../../proto/QZXszSg.Message.pb.h \
    ../../GameServer/ServerUserItem.h \
    iunknown.h \
    ../../proto/QZXszSg.Message.pb.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}


SUBDIRS += \
    Game_QZXszSgLandy.pro

DISTFILES += \
    ../../proto/src/QZXszSg.Message.proto


