/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-20 09:13:18
 * @LastEditTime: 2026-07-20 15:16:55
 * @FilePath: /components/my_web_server/my_web_server.h
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#ifndef my_web_server_H
#define my_web_server_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "stdint.h"
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "mongoose.h"
#include <pthread.h>
#include "my_device_manager.h"

#define WEB_QUEUE_SIZE 10

    typedef struct
    {
        char data[512];

    } web_msg_t;

    void my_web_server_init(void);
    void my_web_server_notify(const char *json);

#ifdef __cplusplus
}
#endif

#endif