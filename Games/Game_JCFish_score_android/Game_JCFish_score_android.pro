#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T13:14:04
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET = Game_JCFish_score_android
TEMPLATE = lib

DEFINES += GAME_JCFISH_SCORE_ANDROID_LIBRARY
DESTDIR   = ../bin

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
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
//INCLUDEPATH += ../../Framework/include/muduo/
//INCLUDEPATH += ../../Framework/include/crypto/
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
    AndroidUserItemSink.cpp  \
    DefGameAndroidExport.cpp \
    ../../proto/Fish.Message.pb.cc

HEADERS += \
    AndroidUserItemSink.h  \
    DefGameAndroidExport.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/IServerUserItem.h \
    ../../public/AndroidUserItem.h \
    ../../proto/Fish.Message.pb.h \
    ../Game_JCFish/FishProduceUtils/CMD_Fish.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}


DISTFILES += \
    ../../proto/src/Fish.Message.proto

#unix:!macx: LIBS += -L$$PWD/../../libs/ -lglog
#INCLUDEPATH += $$PWD/../../libs
#DEPENDPATH += $$PWD/../../libs
#unix:!macx: PRE_TARGETDEPS += $$PWD/../../libs/libglog.so

