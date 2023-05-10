#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T22:32:22
#
#-------------------------------------------------

QT       -= core gui
CONFIG   += plugin

TARGET = Game_ddz_score_android
TEMPLATE = lib
DEFINES += GAME_DDZ_SCORE_ANDROID_LIBRARY
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
INCLUDEPATH += /usr/local/include/linux_include
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../public
INCLUDEPATH += ../../proto
# INCLUDEPATH += ../../Framework/include/muduo/
# INCLUDEPATH += ../../Framework/include/crypto/
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
    ../../proto/ddz.Message.pb.cc \
    # ../../public/GameTimer.cpp \
    GameLogic.cpp \
    AndroidUserItemSink.cpp \
    RobotAI/ddz_move.cpp \
    RobotAI/ddz_split_node.cpp \
    RobotAI/ddz_state.cpp \
    RobotAI/mcts_node.cpp \
    RobotAI/random_engine.cpp \
    RobotAI/SLua.cpp \
    RobotAI/stgy_initiative_play_card.cpp \
    RobotAI/stgy_passive_play_card.cpp \
    RobotAI/stgy_split_card.cpp \
    RobotAI/tinymt32.cpp \
    DouDiZhuRobot_VM.cpp

HEADERS += \
        game_ddz_score_android.h \
        game_ddz_score_android_global.h \
   #../Game_ddznew/Card/DDZCard.h \
   # ../Game_ddznew/Card/DDZCardAlloterMgr.h \
   # ../Game_ddznew/Card/DDZCardGroup.h \
    ../../proto/ddz.Message.pb.h \
    ../Game_ddznew/CMD_Game.h \
    # ../../public/GameTimer.h \
    types.h \
    GameLogic.h \
    AndroidUserItemSink.h \
    RobotAI/ddz_move.h \
    RobotAI/ddz_split_node.h \
    RobotAI/ddz_state.h \
    RobotAI/mcts_move.h \
    RobotAI/mcts_node.h \
    RobotAI/mcts_state.h \
    RobotAI/random_engine.h \
    RobotAI/SLua.h \
    RobotAI/stgy_initiative_play_card.h \
    RobotAI/stgy_passive_play_card.h \
    RobotAI/stgy_split_card.h \
    RobotAI/tinymt32.h \
    DouDiZhuRobot_VM.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}







