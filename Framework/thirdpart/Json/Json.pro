#-------------------------------------------------
#
# Project created by QtCreator 2018-07-04T17:43:03
#
#-------------------------------------------------

QT       -= core gui

TARGET = Json
TEMPLATE = lib
CONFIG += staticlib c++11

DESTDIR = ../../../libs

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ../../Sources

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/lib_json/json_reader.cpp \
    src/lib_json/json_value.cpp \
    src/lib_json/json_valueiterator.inl \
    src/lib_json/json_writer.cpp




HEADERS += \
    include/json/assertions.h \
    include/json/autolink.h \
    include/json/config.h \
    include/json/features.h \
    include/json/forwards.h \
    include/json/json.h \
    include/json/reader.h \
    include/json/value.h \
    include/json/version.h \
    include/json/writer.h \
    src/lib_json/json_tool.h \
    src/lib_json/version.h.in




unix {
    target.path = /usr/lib
    INSTALLS += target
}

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include
