/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:34
 * @LastEditTime: 2026-07-17 15:07:35
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

    typedef struct
    {
        char topic[64];
        char data[QUEUE_DATA_LEN];

    } queue_msg_t;

    void my_queue_init(void);
    int my_queue_push(queue_msg_t *msg);
    int my_queue_pop(queue_msg_t *msg);
    void my_queue_test(void);

#ifdef __cplusplus
}
#endif

#endif