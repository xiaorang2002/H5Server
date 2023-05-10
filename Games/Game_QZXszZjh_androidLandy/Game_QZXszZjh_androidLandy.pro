#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T13:14:04
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET = Game_qzxszzjh_score_android
TEMPLATE = lib

DEFINES += GAME_XZJH_SCORE_ANDROID_LIBRARY
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

#INCLUDEPATH += "../../public/"
#INCLUDEPATH += "../../private/thirdpart/protobuf-3.5.1"

#INCLUDEPATH += ../../private/thirdpart/boost
#INCLUDEPATH += "../../GameServer"
#INCLUDEPATH += "/usr/local/include/lsm"
#INCLUDEPATH += "../../../proto"

INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/thirdpart

INCLUDEPATH += /usr/local/include/google/protobuf

LIBS += $$PWD/../../Framework/libs/libmuduo.a
LIBS += /usr/local/lib/libprotobuf.a




LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread



SOURCES += \
    AndroidUserItemSink.cpp \
    ../../proto/QZXszZjh.Message.pb.cc \

HEADERS += \
    AndroidUserItemSink.h \
    stdafx.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/IServerUserItem.h \
    ../../public/AndroidUserItem.h \
    ../../proto/QZXszZjh.Message.pb.h \

unix {
    target.path = /usr/lib
    INSTALLS += target
}


#unix:!macx: LIBS += -L$$PWD/../../libs/ -lmuduo
#INCLUDEPATH += $$PWD/../../libs
#DEPENDPATH += $$PWD/../../libs
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libmuduo.a

#unix:!macx: LIBS += -L$$PWD/../../libs/ -lprotobuf
#INCLUDEPATH += $$PWD/../../libs
#DEPENDPATH += $$PWD/../../libs
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libprotobuf.a

#unix:!macx: LIBS += -L$$PWD/../../libs/ -lglog
#INCLUDEPATH += $$PWD/../../libs
#DEPENDPATH += $$PWD/../../libs
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libglog.so

