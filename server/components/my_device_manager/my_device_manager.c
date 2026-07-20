

#include "my_device_manager.h"

static void my_device_manager_update(const char *topic, const char *json);

static my_device_t devices[DEVICE_MAX_NUM];
static my_device_t list[DEVICE_MAX_NUM];
static device_state_callback_t state_callback = NULL;

void my_device_manager_init(void)
{
    memset(devices, 0, sizeof(devices));
    my_mqtt_register_callback(my_device_manager_update);

    printf("device manager init\n");
}

// 查找设备id，不存在则创建
static my_device_t *my_device_manager_find(uint32_t id)
{
    for (uint32_t i = 0; i < DEVICE_MAX_NUM; i++)
    {
        if (devices[i].id == id)
            return &devices[i];
    }

    // 如果设备不存在create
    for (uint32_t i = 0; i < DEVICE_MAX_NUM; i++)
    {
        if (devices[i].id == 0)
        {
            devices[i].id = id;
            return &devices[i];
        }
    }

    return NULL;
}

// 解析设备id
static uint32_t my_parse_device_id(const char *topic)
{
    uint32_t id = 0;

    sscanf(topic, "device/%u/state", &id);

    return id;
}

// 解析设备更新信息
static void my_devide_manager_state_update(const char *topic, const char *json)
{
    uint32_t id = my_parse_device_id(topic);
    if (id == 0)
        return;

    my_device_t *dev = my_device_manager_find(id);
    if (dev == NULL)
        return;

    dev->online = 1;
    strncpy(dev->state, json, sizeof(dev->state) - 1);
    dev->last_update = time(NULL);

    time_t now = time(NULL);
    if (now == (time_t)-1)
    {
        printf("time error");
        return;
    }
    printf("time:%s\n", ctime(&now));

    printf("\ndevice state update\n");
    printf("type = %s\n", dev->type);
    printf("name = %s\n", dev->name);
    printf("id = %d\n", dev->id);
    printf("state = %s\n", dev->state);

    // uint32_t num = my_device_manager_get_list(list, DEVICE_MAX_NUM);
    // printf("test demo = %d\n", num);
    // for (uint32_t i = 0; i < num; i++)
    // {
    //     printf("id:%d\n", list[i].id);
    //     printf("name:%s\n", list[i].name);
    //     printf("type:%s\n", list[i].type);
    //     printf("online:%d\n", list[i].online);
    //     printf("state:%s\n", list[i].state);
    // }
}

// 解析设备登陆信息
static void my_device_manager_info_update(const char *topic, const char *json)
{
    uint32_t id = my_parse_device_id(topic);
    if (id == 0)
        return;

    my_device_t *dev = my_device_manager_find(id);

    if (dev == NULL)
        return;

    cJSON *root = cJSON_Parse(json);

    if (root == NULL)
        return;

    cJSON *name = cJSON_GetObjectItem(root, "name");

    cJSON *type = cJSON_GetObjectItem(root, "type");

    if (cJSON_IsString(name))
        strcpy(dev->name, name->valuestring);

    if (cJSON_IsString(type))
        strcpy(dev->type, type->valuestring);

    cJSON_Delete(root);

    printf("\ndevice info update\n");
    printf("type = %s\n", dev->type);
    printf("name = %s\n", dev->name);
    printf("id = %d\n", dev->id);
    // my_device_manager_send_cmd(1, "{\"light\":1}");
}

void my_device_manager_register_callback(device_state_callback_t cb)
{
    state_callback = cb;
}

// 解析收到的mqtt信息
static void my_device_manager_update(const char *topic, const char *json)
{
    // printf("\n===== DEVICE TASK =====\n");
    // printf("topic:%s\n", topic);
    // printf("data:%s\n", json);
    // printf("=======================\n");

    if (strstr(topic, "/state"))
    {
        my_devide_manager_state_update(topic, json);
    }
    else if (strstr(topic, "/info"))
    {
        my_device_manager_info_update(topic, json);
        if (state_callback)
        {
            state_callback(json);
        }
    }
}

// eg: my_device_manager_send_cmd(1, "{\"light\":1}");
// 向设备发送cmd信息
int my_device_manager_send_cmd(uint32_t id, const char *json)
{
    char topic[64];

    snprintf(topic, sizeof(topic), "device/%u/cmd", id);

    printf("send cmd:%s\n", topic);
    printf("data:%s\n", json);

    return my_mqtt_publish(topic, json);
}

// 获取设备列表
uint32_t my_device_manager_get_list(my_device_t *list, uint32_t max_num)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < DEVICE_MAX_NUM; i++)
    {
        if (devices[i].id != 0)
        {
            if (count >= max_num)
                break;

            memcpy(&list[count], &devices[i], sizeof(my_device_t));

            count++;
        }
    }

    return count;
}

// 获取单个设备信息
int my_device_manager_get(uint32_t id, my_device_t *device)
{
    for (int i = 0; i < DEVICE_MAX_NUM; i++)
    {
        if (devices[i].id == id)
        {
            memcpy(device, &devices[i], sizeof(my_device_t));
            return 0;
        }
    }

    return -1;
}