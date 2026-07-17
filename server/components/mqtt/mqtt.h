/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2025-04-09 21:30:00
 * @LastEditTime: 2026-07-17 15:39:12
 * @FilePath: /components/mqtt/mqtt.h
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
#include "my_queue.h"

    void mqtt_test(void);

    int mqtt_init(void);
    int mqtt_deinit(void);
    int mqtt_publish(
        const char *topic,
        const char *data);
#ifdef __cplusplus
}
#endif

#endif