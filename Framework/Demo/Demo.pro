#QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG -= qt

DESTDIR = ../../bin

DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += MUDUO_STD_STRING  

LIBS += -pthread -lrt -lm

LIBS += $$PWD/../libs/libmuduo.a
LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lssl -lcrypto -lsnappy -pthread -lrt -lm -lz -ldl
 
LIBS += -L$$PWD/../libs -lgtest -lgtest_main -lcurl -lcpr


INCLUDEPATH += ../../
INCLUDEPATH += ../../public
INCLUDEPATH += ../include
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/include
  

SOURCES += \
    main.cpp


#HEADERS += \
#    ./websocket/html.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
