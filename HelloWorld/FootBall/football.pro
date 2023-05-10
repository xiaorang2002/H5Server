QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

LIBS +=  -lrt -lm -lz -ldl

DESTDIR = ../bin
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS



LIBS += ../../Framework/libs/libmuduo.a
#LIBS += $$PWD/libAicsEngineLinux.so
INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += ../../Public

INCLUDEPATH += ../../Framework/libs

LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread

LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0
INCLUDEPATH += /usr/include
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    algorithmlogic.cpp \
    GameLogic.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    redblackalgorithm.cpp


HEADERS +=\
    algorithmlogic.h \
    IAicsEngine.h \
    GameLogic.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    redblackalgorithm.h
