/*
 * @Author: BlueboxChamil
 * @Date: 2026-07-17 11:18:21
 * @LastEditTime: 2026-07-17 16:27:54
 * @FilePath: /main.c
 * @Description:
 * Copyright (c) 2026 by BlueboxChamil, All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>

#include "mqtt.h"
#include "my_queue.h"
#include "my_thread.h"

// 要实现的功能，在c的工程中调用一个c++生成的库
int main()
{
    printf("Hello, from myPro!\n");

    my_queue_init();
    my_thread_init();
    mqtt_init();

    while (1)
    {
        sleep(5);
    }
}
