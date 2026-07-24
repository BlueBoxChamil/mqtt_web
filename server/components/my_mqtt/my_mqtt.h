/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2025-04-09 21:30:00
 * @LastEditTime: 2026-07-24 10:28:19
 * @FilePath: /components/my_mqtt/my_mqtt.h
 * @Description:
 * Copyright (c) 2025 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */
#ifndef MQTT_H
#define MQTT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include "MQTTClient.h"
#include <pthread.h>

#define QUEUE_SIZE 10
#define QUEUE_DATA_LEN 256

    typedef struct
    {
        char topic[64];
        char data[QUEUE_DATA_LEN];

    } my_mqtt_queue_t;

    typedef struct
    {
        const char *topic;   // 遗嘱要发布到的主题,例如 "device/1/status"
        char message[64]; // 遗嘱内容,例如 "{\"id\":1,\"online\":0}"
        int qos;             // 0/1/2
        int retained;        // 1 = 保留消息,新订阅者一连上就能立刻拿到最新状态
    } my_mqtt_will_t;

    int my_mqtt_init(void);
    int my_mqtt_deinit(void);
    int my_mqtt_publish(const char *topic, const char *data);

    typedef void (*mqtt_recv_callback_t)(const char *topic, const char *data);
    void my_mqtt_register_callback(mqtt_recv_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif