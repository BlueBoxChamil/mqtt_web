/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:11
 * @LastEditTime: 2026-07-17 16:26:04
 * @FilePath: /components/my_thread/my_thread.c
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#include "my_thread.h"

void device1_state_process(const char *json)
{
    // 在这里再解析json数据

    cJSON *root = cJSON_Parse(json);
    if (root == NULL)
    {
        printf("json error\n");
        return;
    }

    cJSON *light = cJSON_GetObjectItem(root, "light");

    if (cJSON_IsNumber(light))
    {
        if (light->valueint == 1)
        {
            printf("turn light on\n");
        }
        else
        {
            printf("turn light off\n");
        }
    }

    cJSON_Delete(root);
}

void device1_cmd_process(const char *json)
{
    // 在这里再解析json数据
}

void dispatcher_process(queue_msg_t *msg)
{
    if (strcmp(msg->topic, "device/1/state") == 0)
    {
        device1_state_process(msg->data);
    }
    else if (strcmp(msg->topic, "device/1/cmd") == 0)
    {
        device1_cmd_process(msg->data);
    }
}

static void *device_task(void *arg)
{
    queue_msg_t msg;
    // 在这里处理mqtt信息
    while (1)
    {
        my_queue_pop(&msg);
        printf("\n===== DEVICE TASK =====\n");
        printf("topic:%s\n", msg.topic);
        printf("data:%s\n", msg.data);
        printf("=======================\n");

        dispatcher_process(&msg);
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