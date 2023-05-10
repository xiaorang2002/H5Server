#-------------------------------------------------
#
# Project created by QtCreator 2018-07-13T16:20:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = hiredis-vip
TEMPLATE = lib
CONFIG += staticlib c++11

DESTDIR = ../../../libs


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adlist.c \
    async.c \
    command.c \
    crc16.c \
    dict.c \
    hiarray.c \
    hircluster.c \
    hiredis.c \
    hiutil.c \
    net.c \
    read.c \
    sds.c

HEADERS += \
    adapters/libevent.h \
    adlist.h \
    async.h \
    command.h \
    dict.h \
    fmacros.h \
    hiarray.h \
    hircluster.h \
    hiredis.h \
    hiutil.h \
    net.h \
    read.h \
    sds.h \
    win32.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    CHANGELOG.md \
    README.md \
    hiredis_vip.pc \
    COPYING
