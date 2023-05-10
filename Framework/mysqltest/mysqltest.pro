TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#target.path = /home/bin
#INSTALLS += target



DESTDIR = ../../bin

DEFINES += MUDUO_STD_STRING



LIBS += ../../Framework/libs/libmuduo.a

INCLUDEPATH += ../..
INCLUDEPATH += ../../Framework/include
INCLUDEPATH += /usr/local/include/mysql
INCLUDEPATH += /usr/local/include/mysql/jdbc
INCLUDEPATH += ../../Framework/libs
INCLUDEPATH +=../../public

LIBS += /usr/local/lib/libmysqlclient.a
LIBS += /usr/local/lib/libmysqlcppconn-static.a


#LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0

LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl
#INCLUDEPATH += /usr/include
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    algorithmlogic.cpp \
    GameLogic.cpp \
    ConnectionPool.cpp \
    ../../public/GlobalFunc.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    redblackalgorithm.cpp


HEADERS +=\
    algorithmlogic.h \
    IAicsEngine.h \
    GameLogic.h \
    ConnectionPool.h \
    ../../public/GlobalFunc.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    redblackalgorithm.h
