#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T13:14:04
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET = Game_sghjk_score_android
TEMPLATE = lib

DEFINES += GAME_SGBJK_SCORE_ANDROID_LIBRARY
DESTDIR = ../../bin/

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


INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
LIBS += $$PWD/../../Framework/libs/libmuduo.a

INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto

LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog



SOURCES += \
    AndroidUserItemSink.cpp \
    ../../public/GlobalFunc.cpp \
    ../../proto/sghjk.Message.pb.cc \

HEADERS += \
    AndroidUserItemSink.h \
    stdafx.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/IServerUserItem.h \
    ../../public/AndroidUserItem.h \
    ../../public/GlobalFunc.h \
    ../../proto/sghjk.Message.pb.h \

unix {
    target.path = /usr/lib
    INSTALLS += target
}



DISTFILES += \
    ../../proto/src/sghjk.Message.proto


