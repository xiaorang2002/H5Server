#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T13:14:04
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin
CONFIG += c++11

TARGET = Game_Erqs_score_android
TEMPLATE = lib


DESTDIR =../../bin/

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += GAME_ERQS_ANDROID_LIBRARY
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
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
INCLUDEPATH += ../Game_Gswz
LIBS += $$PWD/../../Framework/libs/libmuduo.a




LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog




SOURCES += \
    AndroidUserItemSink.cpp  \
	../Game_Erqs/CTable.cpp \
    ../Game_Erqs/CTableMgr.cpp \
    ../Game_Erqs/CPart.cpp \
    ../Game_Erqs/ComposeTypeCheckers.cpp \
    ../Game_Erqs/MjAlgorithm.cpp \
    ../Game_Erqs/GamePlayer.cpp \
    ../../proto/Erqs.Message.pb.cc \
    ../../public/GameTimer.cpp \
    ../../public/weights.cpp \
    ../../public/GlobalFunc.cpp


HEADERS += \
    AndroidUserItemSink.h  \
	../Game_Erqs/CTable.h \
    ../Game_Erqs/CTableMgr.h \
    ../Game_Erqs/CPart.h \
    ../Game_Erqs/GameDefine.h \
    ../Game_Erqs/MjAlgorithm.h \
    ../Game_Erqs/GamePlayer.h \
    ../../public/IAndroidUserItemSink.h \
    ../../public/IServerUserItem.h \
    ../../public/AndroidUserItem.h \
    ../../public/ITableFrame.h \
    ../../proto/Erqs.Message.pb.h \
    ../../public/GameTimer.h \
    ../../public/GlobalFunc.h
