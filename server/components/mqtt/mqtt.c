/*
 * @Author: BlueboxChamil
 * @Date: 2026-07-17 11:18:21
 * @LastEditTime: 2026-07-17 15:44:04
 * @FilePath: /components/mqtt/mqtt.c
 * @Description:
 * Copyright (c) 2026 by BlueboxChamil, All Rights Reserved.
 */

#include "mqtt.h"

#define MQTT_ADDRESS "tcp://192.168.164.128:1883"
#define MQTT_CLIENT_ID "linux_mqtt_client_001"
#define MQTT_USERNAME "user2"
#define MQTT_PASSWORD "2"
// #define MQTT_SUB_TOPIC "device/1/state"
static const char *sub_topics[] = {
    "device/1/state",
    "device/1/cmd",
    "device/1/info"};

static MQTTClient client;

static int mqtt_message_callback(
    void *context,
    char *topicName,
    int topicLen,
    MQTTClient_message *message)
{
    // printf("\n========== MQTT RX ==========\n");
    // printf("topic:%s\n",
    //        topicName);

    // printf("data:%.*s\n",
    //        message->payloadlen,
    //        (char *)message->payload);
    // printf("=============================\n");

    queue_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.topic, topicName);
    memcpy(msg.data, message->payload, message->payloadlen);
    my_queue_push(&msg);

    return 1;
}

int mqtt_init(void)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    // 创建client
    int ret = MQTTClient_create(
        &client,
        MQTT_ADDRESS,
        MQTT_CLIENT_ID,
        MQTTCLIENT_PERSISTENCE_NONE,
        NULL);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf(
            "mqtt create failed:%d\n",
            ret);
        return -1;
    }
    // 设置用户名密码
    conn_opts.username = MQTT_USERNAME;
    conn_opts.password = MQTT_PASSWORD;

    /*
     * 设置异步接收回调
     */
    MQTTClient_setCallbacks(
        client,
        NULL,
        NULL,
        mqtt_message_callback,
        NULL);

    // 连接broker
    ret = MQTTClient_connect(
        client,
        &conn_opts);

    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf(
            "mqtt connect failed:%d\n",
            ret);

        return -2;
    }
    printf("mqtt connected\n");

    // 订阅
    for (uint32_t i = 0; i < sizeof(sub_topics) / sizeof(sub_topics[0]); i++)
    {
        MQTTClient_subscribe(client, sub_topics[i], 1);
        printf("subscribe:%s\n", sub_topics[i]);
    }
    return 0;
}

int mqtt_publish(const char *topic, const char *data)
{

    MQTTClient_message msg = MQTTClient_message_initializer;

    MQTTClient_deliveryToken token;

    msg.payload = (void *)data;
    msg.payloadlen = strlen(data);
    msg.qos = 1;
    msg.retained = 0;

    int ret = MQTTClient_publishMessage(client, topic, &msg, &token);

    if (ret != MQTTCLIENT_SUCCESS)
    {
        return -1;
    }

    MQTTClient_waitForCompletion(client, token, 1000);
    return 0;
}

int mqtt_deinit(void)
{
    int ret;
    for (uint32_t i = 0; i < sizeof(sub_topics) / sizeof(sub_topics[0]); i++)
    {
        ret = MQTTClient_unsubscribe(client, sub_topics[i]);
        if (ret != MQTTCLIENT_SUCCESS)
        {
            printf("unsubscribe[%d] failed:%d\n", i, ret);
        }
    }

    ret = MQTTClient_disconnect(client, 1000);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf("disconnect failed:%d\n", ret);
    }

    MQTTClient_destroy(&client);

    printf("mqtt disconnected\n");

    return 0;
}

void mqtt_test(void)
{
    if (mqtt_init() != 0)
    {
        printf("mqtt init failed\n");

        return;
    }

    printf("succeed mqtt connect\n");
    // mqtt_publish( "device/1/state", "{\"state\":\"on\"}");
    // while (1)
    // {
    //     sleep(5);
    // }
}