/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:11
 * @LastEditTime: 2026-07-17 16:26:04
 * @FilePath: /components/my_thread/my_thread.c
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#include "my_thread.h"

static void *device_task(void *arg)
{
    queue_msg_t msg;
    // 在这里处理mqtt信息
    while (1)
    {
        my_queue_pop(&msg);
        my_device_manager_update(msg.topic,msg.data);
    }

    return NULL;
}

void my_thread_init(void)
{
    pthread_t tid;
    pthread_create(
        &tid,
        NULL,
        device_task,
        NULL);

    pthread_detach(tid);
}

void my_thread_test(void)
{
    return;
}