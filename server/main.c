/*
 * @Author: BlueboxChamil
 * @Date: 2026-07-17 11:18:21
 * @LastEditTime: 2026-07-20 15:19:53
 * @FilePath: /main.c
 * @Description:
 * Copyright (c) 2026 by BlueboxChamil, All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_mqtt.h"
#include "my_device_manager.h"
#include "my_web_server.h"

// 要实现的功能，在c的工程中调用一个c++生成的库
int main()
{
    printf("Hello, from myPro!\n");

    my_mqtt_init();
    my_device_manager_init();
    my_web_server_init();

    while (1)
    {
        sleep(5);
    }
}
