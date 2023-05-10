#-------------------------------------------------
#
# Project created by QtCreator 2018-06-30T03:40:19
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += c++11 plugin

TARGET   = Game_toubao
TEMPLATE = lib


DESTDIR = ../../bin

DEFINES += Game_toubao
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
#INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a


INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a

INCLUDEPATH += ../../public

LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog -lstdc++
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread


SOURCES += \
    TableFrameSink.cpp \
#    Algorithmc.cpp \
    BetArea.cpp \
    ../../proto/Tb.Message.pb.cc \
    ../../public/GlobalFunc.cpp \
    TbAlgorithm.cpp

HEADERS += \
    TableFrameSink.h \
#    Algorithmc.h \
    BetArea.h \
    ../../public/gameDefine.h \
    ../../public/Globals.h \
    ../../public/ITableFrame.h \
    ../../public/ITableFrameSink.h \
    ../../public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Tb.Message.pb.h \
    ../../public/GlobalFunc.h \
    TbAlgorithm.h



unix {
    target.path = /usr/lib
    INSTALLS += target
}

#DISTFILES += \
#   ../../proto/src/Bcbm.Message.proto






