#-------------------------------------------------
#
# Project created by QtCreator 2018-05-07T20:45:28
#
#-------------------------------------------------

QT       -= core gui

TARGET = gameCrypto
TEMPLATE = lib
CONFIG += staticlib


DESTDIR = ../../../libs


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += MUDUO_STD_STRING


INCLUDEPATH += ../../..
#INCLUDEPATH += ../../include/



LIBS += /usr/local/lib/libboost_filesystem.a
LIBS += /usr/local/lib/libboost_system.a


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        crypto.cpp \
    ../../../public/GlobalFunc.cpp

HEADERS += \
        crypto.h \
    ../../../public/GlobalFunc.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}








