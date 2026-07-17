/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-17 14:58:34
 * @LastEditTime: 2026-07-17 16:15:00
 * @FilePath: /components/my_thread/my_thread.h
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */

#ifndef my_thread_H
#define my_thread_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "my_queue.h"
#include "cJSON.h"

    void my_thread_init(void);

#ifdef __cplusplus
}
#endif

#endif