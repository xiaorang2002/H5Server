
TEMPLATE = app
CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG -= qt



DESTDIR = ../bin

DEFINES += MUDUO_STD_STRING
DEFINED += _USE_REDLOCK_



LIBS += ../Framework/libs/libmuduo.a
#LIBS += $$PWD/libAicsEngineLinux.so
INCLUDEPATH += ../..
INCLUDEPATH += ../Framework/include
INCLUDEPATH += ../public

INCLUDEPATH += ../Framework/libs

LIBS +=  -lrt -lm -lz -ldl -pthread

LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread

LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0
INCLUDEPATH += /usr/include
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    algorithmlogic.cpp \
    ../public/ThreadLocalSingletonMongoDBClient.cpp \


HEADERS +=\
    algorithmlogic.h \
    ../public/ThreadLocalSingletonMongoDBClient.h \
