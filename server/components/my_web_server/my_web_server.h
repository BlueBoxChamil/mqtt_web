
#ifndef my_web_server_H
#define my_web_server_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "stdint.h"
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "mongoose.h"
#include <pthread.h>
#include "my_device_manager.h"


void my_web_server_init(void);

#ifdef __cplusplus
}
#endif

#endif