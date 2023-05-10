#include "RocketMQ.h"


RocketMQ::RocketMQ() : m_consumer("DCS"),  m_producer("DCS")
{

}

RocketMQ::~RocketMQ()
{
    if(m_bConsumerFlag)
    {
        // 停止消费者
        printf("[RocketMq]stop");
        m_consumer.shutdown();
    }
    if(m_bProducerFlag)
    {
        // 停止生产者
        printf("[RocketMq]stop");
        m_producer.shutdown();
    }
}

void RocketMQ::Start(string &strAdd, string groupName, string topic, string tag, MessageModel messageModel)
{
    Init(strAdd, groupName, topic, tag, messageModel);
    m_bConsumerFlag = true;
    m_bProducerFlag = false;
}

void RocketMQ::Start(string &strAdd, string groupName)
{
    Init(strAdd, groupName);
    m_bProducerFlag = true;
    m_bConsumerFlag = false;
}

void RocketMQ::shutdownConsumer()
{
    if(m_bConsumerFlag)
    {
        // 停止消费者
        printf("[RocketMq]stop");
        m_consumer.shutdown();
    }
}


void RocketMQ::Init(string &strAdd, string &groupName, string &topic, string &tag, MessageModel messageModel)
{
    // 设置MQ的NameServer地址
    printf("[RocketMq]Addr: %s\n", strAdd.c_str());
    m_consumer.setNamesrvAddr(strAdd);
    m_consumer.setGroupName(groupName);
    m_consumer.setInstanceName(groupName);
    // 设置消费模式，CLUSTERING-集群模式，BROADCASTING-广播模式
//    printf("[RocketMq]Model: %s\n", getMessageModelString(CLUSTERING));
    m_consumer.setMessageModel(messageModel);
    m_consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
    m_consumer.subscribe(topic, tag);
//    m_consumer.setConsumeThreadCount(15);
    m_consumer.setTcpTransportTryLockTimeout(1000);
    m_consumer.setTcpTransportConnectTimeout(400);

    m_consumer.registerMessageListener(this);

    printf("[RocketMq]start\n");
    m_consumer.start();
}

void RocketMQ::Init(string &strAdd, string &groupName)
{
    // 设置MQ的NameServer地址
    printf("[RocketMq]Addr: %s\n", strAdd.c_str());
    m_producer.setNamesrvAddr(strAdd);
    m_producer.setGroupName(groupName);
    // 启动消费者
    printf("[RocketMq]start\n");
    m_producer.start();
}


//int64_t RocketMQ::GetData(string strTopic, list<string>& lstrData)
//{
//    while(true)
//    {
//        // 获取指定topic的路由信息
//        std::vector<MQMessageQueue> messageQueue;
//         m_consumer.fetchSubscribeMessageQueues(strTopic, messageQueue);
////        bool nFirst = true;
//        for(MQMessageQueue &mq : messageQueue)
//        {
//            while(true)
//            {
//                try
//                {
//                    // 拉取消息
//                    //从指定set开始读取
//                    PullResult pullResult = m_consumer.pull(mq, "*", m_llOffSet, 32);
//                    if((pullResult.pullStatus == FOUND) && (!pullResult.msgFoundList.empty()))
//                    {
//                        printf("[RocketMQ]获取到消息 %d 条,当前位置 %lld ,整体位置 %lld\n", pullResult.msgFoundList.size(), pullResult.nextBeginOffset, m_llOffSet);
//                        for(MQMessageExt &messageExt : pullResult.msgFoundList)
//                        {
//                            lstrData.push_back(messageExt.getBody());
//                        }
//                        //以防万一，此返回值存数大的
//                        m_llOffSet = (pullResult.nextBeginOffset > m_llOffSet ? pullResult.nextBeginOffset : m_llOffSet);
//                        //更新进度还用原来的
//                        m_consumer.updateConsumeOffset(mq, pullResult.nextBeginOffset);
//                    }else
//                    {
//                        break;
//                    }
//                }catch (MQException& e)
//                {
//                    std::cout<<e<<std::endl;
//                }
//            }
//        }
//        if (!lstrData.empty())
//        {
//            return m_llOffSet;
//        }
//    }
//    return m_llOffSet;
//}

//发送数据
void RocketMQ::SendData(string strTopic, string strTag, string strKey, string &strData)
{
    try
    {
        MQMessage msg(strTopic, strTag, strKey, strData);
        // 同步生产消息
        m_producer.send(msg);
    }catch (MQClientException& e)
    {
        printf("[RocketMQ]数据发送失败： [topic]%s[tag]%s[key]%s[info]%s[reason]%s\n", strTopic.c_str(), strTag.c_str(), strKey.c_str(), strData.c_str(), e.what());
    }
}

ConsumeStatus RocketMQ::consumeMessage(const std::vector<MQMessageExt> &msgs)
{
    if(m_messageCallback)
        m_messageCallback(msgs);
    return CONSUME_SUCCESS;
}



