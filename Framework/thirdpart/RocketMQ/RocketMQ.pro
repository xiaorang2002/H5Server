#-------------------------------------------------
#
# Project created by QtCreator 2018-05-07T20:45:28
#
#-------------------------------------------------

QT       -= core gui

TARGET = RocketMQ
TEMPLATE = lib
CONFIG += staticlib


DESTDIR = ../../../libs/mq


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += MUDUO_STD_STRING



LIBS += lib/librocketmq.a


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        RocketMQ.cpp

HEADERS += \
        RocketMQ.h \
    include/Arg_helper.h \
    include/AsyncCallback.h \
    include/BatchMessage.h \
    include/CCommon.h \
    include/CMessage.h \
    include/CMessageExt.h \
    include/CMessageQueue.h \
    include/CMQException.h \
    include/ConsumeType.h \
    include/CProducer.h \
    include/CPullConsumer.h \
    include/CPullResult.h \
    include/CPushConsumer.h \
    include/CSendResult.h \
    include/DefaultMQProducer.h \
    include/DefaultMQPullConsumer.h \
    include/DefaultMQPushConsumer.h \
    include/MQClient.h \
    include/MQClientException.h \
    include/MQConsumer.h \
    include/MQMessage.h \
    include/MQMessageExt.h \
    include/MQMessageListener.h \
    include/MQMessageQueue.h \
    include/MQProducer.h \
    include/MQSelector.h \
    include/MQueueListener.h \
    include/PullResult.h \
    include/QueryResult.h \
    include/RocketMQClient.h \
    include/SendMessageHook.h \
    include/SendResult.h \
    include/SessionCredentials.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    lib/librocketmq.a
