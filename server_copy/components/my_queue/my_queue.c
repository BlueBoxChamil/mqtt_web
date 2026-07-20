/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:11
 * @LastEditTime: 2026-07-20 13:34:10
 * @FilePath: /components/my_queue/my_queue.c
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#include "my_queue.h"

// mqtt
static queue_msg_t queue[QUEUE_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// web
static web_msg_t web_queue[WEB_QUEUE_SIZE];
static int web_head = 0;
static int web_tail = 0;
static int web_count = 0;
static pthread_mutex_t web_mutex = PTHREAD_MUTEX_INITIALIZER;

void my_queue_init(void)
{
    head = 0;
    tail = 0;
    count = 0;
}

int my_queue_push(queue_msg_t *msg)
{
    pthread_mutex_lock(&mutex);
    if (count >= QUEUE_SIZE)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    memcpy(&queue[tail], msg, sizeof(queue_msg_t));
    tail++;
    if (tail >= QUEUE_SIZE)
    {
        tail = 0;
    }
    count++;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return 0;
}

int my_queue_pop(queue_msg_t *msg)
{
    pthread_mutex_lock(&mutex);
    while (count == 0)
    {
        pthread_cond_wait(&cond, &mutex);
    }

    memcpy(msg, &queue[head], sizeof(queue_msg_t));
    head++;

    if (head >= QUEUE_SIZE)
    {
        head = 0;
    }
    count--;

    pthread_mutex_unlock(&mutex);
    return 0;
}

void web_queue_push(const char *json)
{
    pthread_mutex_lock(&web_mutex);
    if (web_count < WEB_QUEUE_SIZE)
    {
        strcpy(web_queue[web_tail].data, json);
        web_tail++;

        if (web_tail >= WEB_QUEUE_SIZE)
            web_tail = 0;

        web_count++;
    }
    pthread_mutex_unlock(&web_mutex);
}

int web_queue_pop(web_msg_t *msg)
{
    int ret = -1;
    pthread_mutex_lock(&web_mutex);
    if (web_count > 0)
    {
        memcpy(msg, &web_queue[web_head], sizeof(web_msg_t));
        web_head++;

        if (web_head >= WEB_QUEUE_SIZE)
            web_head = 0;
        web_count--;
        ret = 0;
    }
    pthread_mutex_unlock(&web_mutex);
    return ret;
}