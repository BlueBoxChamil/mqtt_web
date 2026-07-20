/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-20 09:13:18
 * @LastEditTime: 2026-07-20 11:54:57
 * @FilePath: /components/my_device_manager/my_device_manager.h
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#ifndef my_device_manager_H
#define my_device_manager_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "stdint.h"
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "string.h"
#include <time.h>
#include "mqtt.h"

#define DEVICE_MAX_NUM 3

    typedef struct
    {
        uint32_t id;

        char name[32];
        char type[32];
        uint8_t online;
        char state[512];
        uint32_t last_update;

    } my_device_t;

    void my_device_manager_init(void);
    void my_device_manager_update(const char *topic, const char *json);
    int my_device_manager_send_cmd(uint32_t id, const char *json);
    uint32_t my_device_manager_get_list(my_device_t *list, uint32_t max_num);
    int my_device_manager_get(uint32_t id, my_device_t *device);

    typedef void (*device_state_callback_t)(const char *json);

    void my_device_manager_register_callback(device_state_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif