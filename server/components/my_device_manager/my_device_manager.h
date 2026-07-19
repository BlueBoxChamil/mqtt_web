
#ifndef my_device_manager_H
#define my_device_manager_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "stdint.h"
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "string.h"
#include <time.h>
#include "mqtt.h"

#define DEVICE_MAX_NUM 3

    typedef struct
    {
        uint32_t id;

        char name[32];
        char type[32];
        uint8_t online;
        char state[512];
        uint32_t last_update;

    } my_device_t;

    void my_device_manager_init(void);
    void my_device_manager_update(const char *topic, const char *json);
    void my_device_manager_send_cmd(uint32_t id, const char *json);
    uint32_t my_device_manager_get_list(my_device_t *list,uint32_t max_num);

#ifdef __cplusplus
}
#endif

#endif