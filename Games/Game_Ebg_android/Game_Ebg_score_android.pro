#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T13:14:04
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET = Game_ebg_score_android
TEMPLATE = lib

DEFINES += GAME_EBG_SCORE_ANDROID_LIBRARY

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
INCLUDEPATH += ../../Framework/include


INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

LIBS += $$PWD/../../Framework/libs/libmuduo.a
#LIBS += $$PWD/../../Framework/libs/libprotobuf.a



LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog
LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread

SOURCES += \
    AndroidUserItemSink.cpp \
    ../../proto/ErBaGang.Message.pb.cc \

HEADERS += \
    AndroidUserItemSink.h \
    ../../proto/ErBaGang.Message.pb.h \
    INIHelp.h \

unix {
    target.path = /usr/lib
    INSTALLS += target
}

#unix:!macx: LIBS += -L$$PWD/../../libs/ -lUtility
#INCLUDEPATH += $$PWD/../../libs/Utility
#DEPENDPATH += $$PWD/../../libs/Utility
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libUtility.a


#unix:!macx: LIBS += -L$$PWD/../../libs/ -lmuduo
#INCLUDEPATH += $$PWD/../../libs
#DEPENDPATH += $$PWD/../../libs
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libmuduo.a

unix:!macx: LIBS += -L$$PWD/../../Framework/libs/ -lprotobuf
INCLUDEPATH += $$PWD/../../Framework/libs
DEPENDPATH += $$PWD/../../Framework/libs
unix:!macx: PRE_TARGETDEPS += $$PWD/../../Framework/libs/libprotobuf.a

#SUBDIRS += \
#    Game_longhu_score_android.pro \
#    Game_Longhu_score_android.pro

#DISTFILES += \
#    Game_longhu_score_android.pro.user


