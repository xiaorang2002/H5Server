#-------------------------------------------------
#
# Project created by QtCreator 2018-07-07T21:06:31
#
#-------------------------------------------------

# debug:
# qmake Game_Fish.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug

# release:
# qmake Game_Fish.pro -spec linux-g++

QT       -= core gui
CONFIG   += plugin

CONFIG += c++11

TEMPLATE  = lib
DESTDIR   = ../../bin

# aics engine support.
DEFINES += AICS_ENGINE

# JCFISH
DEFINES += __JCBY__
TARGET  = Game_jcfish

DEFINES += GAME_FISH_LIBRARY
#DEFINES += __DYNAMIC_CHAIR__
DEFINES += __MULTI_THREAD__

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#INCLUDEPATH += ../../Framework/thirdpart/boost/
INCLUDEPATH += /usr/local/include/linux_include
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/include/linux_include
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
#INCLUDEPATH += ../../Framework/include/muduo/
#INCLUDEPATH += ../../Framework/include/crypto/
INCLUDEPATH += ../..

LIBS += -ldl -lpthread -lglog
LIBS += -lcrypto -lssl

LIBS += /usr/local/include/linux_include/xml/libtinyxml.a
LIBS += $$PWD/../../Framework/libs/libgameCrypto.a
LIBS += $$PWD/../../Framework/libs/libmuduo.a
LIBS += $$PWD/../../Framework/libs/libuuid.a
LIBS += /usr/local/lib/libuuid.a

INCLUDEPATH += /usr/local/include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a

#LIBS += /usr/local/mysqlconncpp/lib64/libmysqlcppconn-static.a
#LIBS += /usr/local/mysql/lib/libmysqlclient.a

LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog -lcrypto -lssl -I/usr/local/lib -luuid
//LIBS += -lsnappy -pthread -lrt -lm -lz -ldl -lglog -I/usr/local/lib -ljsoncpp

SOURCES += \
    fish_server/FishProduceController.cpp \
    fish_server/FishProduceManager.cpp \
    fish_server/table_frame_sink.cpp \
    FishProduceUtils/AutoProduceFish.cpp \
    FishProduceUtils/FishBasicDataManager.cpp \
    FishProduceUtils/FishProduceAllocator.cpp \
    FishProduceUtils/FishProduceDataManager.cpp \
    FishProduceUtils/FishProduceData.cpp \
    FishProduceUtils/FishShoalInfoManager.cpp \
    FishProduceUtils/FishSwimmingPatternManager.cpp \
    FishProduceUtils/Random.cpp \
    FishProduceUtils/AutoFishPath.cpp \
    FishProduceUtils/FishPathData.cpp \
    fish_server/UserPrefs.cpp \
    ../../proto/Fish.Message.pb.cc \
    user_check/NewUserCheck.cpp \
    user_check/AicsNewUserCheck.cpp \
    fish_server/PlayLog.cpp

HEADERS += \
    fish_server/ArithmLog.h \
    fish_server/FishProduceController.h \
    fish_server/FishProduceManager.h \
    fish_server/ISendTableData.h \
    fish_server/ITimerEvent.h \
    fish_server/SceneManager.h \
    fish_server/table_frame_sink.h \
    fish_server/TraceInfo.h \
    ../../public/ITableFrameSink.h \
    FishProduceUtils/NetworkDataUtilitySimple.h \
    ../../public/ITableFrame.h \
    FishProduceUtils/CMD_Fish.h \
    FishProduceUtils/Random.h \
    FishProduceUtils/NetworkDataUtility.h \
    ../../proto/Fish.Message.pb.h \
    inc/aics/IAicsEngine.h \
    FishProduceUtils/MemAlloc.h \
    inc/Feature.h \
    user_check/NewUserCheck.h \
    fish_server/UserPrefs.h \
    user_check/AicsNewUserCheck.h \
    user_check/BlackListData.h \
    fish_server/PlayLog.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
DISTFILES += \
    ../../proto/src/Fish.Message.proto
