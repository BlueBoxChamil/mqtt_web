/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:34
 * @LastEditTime: 2026-07-20 13:34:27
 * @FilePath: /components/my_queue/my_queue.h
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#ifndef my_queue_H
#define my_queue_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define QUEUE_SIZE 10
#define QUEUE_DATA_LEN 256

#define WEB_QUEUE_SIZE 10

    typedef struct
    {
        char topic[64];
        char data[QUEUE_DATA_LEN];

    } queue_msg_t;

    typedef struct
    {
        char data[512];

    } web_msg_t;

    void my_queue_init(void);
    int my_queue_push(queue_msg_t *msg);
    int my_queue_pop(queue_msg_t *msg);
    void web_queue_push(const char *json);
    int web_queue_pop(web_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif