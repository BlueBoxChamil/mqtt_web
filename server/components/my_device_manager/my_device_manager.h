/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-20 09:13:18
 * @LastEditTime: 2026-07-23 14:42:57
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
#include "my_mqtt.h"
#include <pthread.h>

#define DEVICE_MAX_NUM 3

#define DEVICE_OFFLINE_TIMEOUT_SEC 30 // 超过这个时间没上报就判定离线
#define DEVICE_CHECK_INTERVAL_SEC 5   // 扫描线程检查间隔

    typedef struct
    {
        uint32_t id;

        char name[32];         // info上报
        char type[32];         // info上报
        char model[32];        // info上报:设备型号
        char fw_version[16];   // info上报:固件版本号,如 "1.0.3"
        char mac[18];          // info上报:MAC 地址,如 "AA:BB:CC:DD:EE:FF" + '\0'
        char capabilities[256]; // info上报:能力列表,存成逗号分隔字符串,如 "switch,brightness"
        uint8_t online;
        char state[512];
        time_t last_update;

    } my_device_t;

    void my_device_manager_init(void);
    int my_device_manager_send_cmd(uint32_t id, const char *json);
    uint32_t my_device_manager_get_list(my_device_t *list, uint32_t max_num);
    int my_device_manager_get(uint32_t id, my_device_t *device);

    typedef void (*device_state_callback_t)(const char *json);
    void my_device_manager_register_callback(device_state_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif