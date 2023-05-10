#ifndef ROCKETMQ_H
#define ROCKETMQ_H


#include <iostream>
#include <string>
#include <list>
#include <functional>

#include "include/DefaultMQProducer.h"
#include "include/DefaultMQPullConsumer.h"
#include "include/DefaultMQPushConsumer.h"


using namespace rocketmq;
using namespace std;

class RocketMQ : public MessageListenerConcurrently
{
public:
    typedef function<uint32_t(const vector<MQMessageExt> &)> MessageCallback;
public:
    RocketMQ();
    ~RocketMQ();

public:
//    //consumer接收消息，接收前需设置set（init）
//    int64_t GetData(string strTopic, list<string>& lstrData);

    //consumer初始化
    void Start(string &strAdd, string groupName, string topic, string tag, MessageModel messageModel);
    //productor初始化
    void Start(string &strAdd, string groupName);

    void SetMesageCallback(const MessageCallback &cb)
    { m_messageCallback = cb; }

    void shutdownConsumer();
    //发送消息
    void SendData(string strTopic, string strTag, string strKey, string &strData);

private:
    void Init(string &strAdd,string &groupName, string &topic, string &tag, MessageModel messageModel=MessageModel::CLUSTERING);
    void Init(string &strAdd, string &groupName);

private:
    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs);

private:

    MessageCallback m_messageCallback;


//    DefaultMQPullConsumer m_consumer;
    DefaultMQPushConsumer m_consumer;
    DefaultMQProducer m_producer;
    int64_t m_llOffSet;
    bool m_bConsumerFlag;
    bool m_bProducerFlag;

};



#endif // ROCKETMQ_H
