/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:11
 * @LastEditTime: 2026-07-17 15:10:34
 * @FilePath: /components/my_queue/my_queue.c
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#include "my_queue.h"

static queue_msg_t queue[QUEUE_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void my_queue_init(void)
{
    head = 0;
    tail = 0;
    count = 0;
}

int my_queue_push(queue_msg_t *msg)
{
    pthread_mutex_lock(&mutex);
    if(count >= QUEUE_SIZE)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    memcpy(&queue[tail], msg, sizeof(queue_msg_t));
    tail++;
    if(tail >= QUEUE_SIZE)
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
    while(count == 0)
    {
        pthread_cond_wait(&cond,&mutex);
    }

    memcpy(msg,&queue[head],sizeof(queue_msg_t));
    head++;

    if(head >= QUEUE_SIZE)
    {
        head = 0;
    }
    count--;

    pthread_mutex_unlock(&mutex);
    return 0;
}

void my_queue_test(void)
{
    return;
}