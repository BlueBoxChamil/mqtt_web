

#include "my_device_manager.h"

static void my_device_manager_update(const char *topic, const char *json);

static my_device_t devices[DEVICE_MAX_NUM];
static my_device_t list[DEVICE_MAX_NUM];
static device_state_callback_t state_callback = NULL;

// 扫描所有设备,超时未上报的标记为离线
static void my_device_manager_check_online(void)
{
    // 设备每 10s 发一次 state(哪怕没变化),判定为设备在线
    time_t now = time(NULL);

    for (uint32_t i = 0; i < DEVICE_MAX_NUM; i++)
    {
        if (devices[i].id == 0)
            continue; // 这个槽位还没被用过,跳过

        if (devices[i].online == 0)
            continue; // 已经是离线状态,不用重复处理

        if (now - devices[i].last_update > DEVICE_OFFLINE_TIMEOUT_SEC)
        {
            devices[i].online = 0;
            printf("device %u offline (timeout)\n", devices[i].id);

            // 可选:通知前端离线了
            if (state_callback)
            {
                char msg[64];
                snprintf(msg, sizeof(msg), "{\"id\":%u,\"online\":0}", devices[i].id);
                state_callback(msg);
            }
        }
    }
}

// 定时扫描线程
static void *my_device_manager_check_task(void *arg)
{
    while (1)
    {
        // 服务器每 5s 扫描一次所有设备
        sleep(DEVICE_CHECK_INTERVAL_SEC);
        my_device_manager_check_online();
    }
    return NULL;
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

    dev->last_update = time(NULL);   // 加这一行:收到 info 也算一次心跳

    cJSON *root = cJSON_Parse(json);

    if (root == NULL)
        return;

    cJSON *name = cJSON_GetObjectItem(root, "name");
    cJSON *type = cJSON_GetObjectItem(root, "type");
    cJSON *model = cJSON_GetObjectItem(root, "model");
    cJSON *fw_version = cJSON_GetObjectItem(root, "fw_version");
    cJSON *mac = cJSON_GetObjectItem(root, "mac");
    cJSON *capabilities = cJSON_GetObjectItem(root, "capabilities");

    printf("\ndevice info update\n");
    printf("id = %d\n", dev->id);
    if (cJSON_IsString(name))
    {
        strncpy(dev->name, name->valuestring, sizeof(dev->name) - 1);
        printf("name = %s\n", dev->name);
    }

    if (cJSON_IsString(type))
    {
        strncpy(dev->type, type->valuestring, sizeof(dev->type) - 1);
        printf("type = %s\n", dev->type);
    }

    if (cJSON_IsString(model))
    {
        strncpy(dev->model, model->valuestring, sizeof(dev->model) - 1);
        printf("model = %s\n", dev->model);
    }

    if (cJSON_IsString(fw_version))
    {
        strncpy(dev->fw_version, fw_version->valuestring, sizeof(dev->fw_version) - 1);
        printf("fw_version = %s\n", dev->fw_version);
    }

    if (cJSON_IsString(mac))
    {
        strncpy(dev->mac, mac->valuestring, sizeof(dev->mac) - 1);
        printf("mac = %s\n", dev->mac);
    }

    // capabilities 是数组,拼成逗号分隔字符串存起来
    if (cJSON_IsArray(capabilities))
    {
        dev->capabilities[0] = '\0';
        int cap_count = cJSON_GetArraySize(capabilities);
        for (int i = 0; i < cap_count; i++)
        {
            cJSON *item = cJSON_GetArrayItem(capabilities, i);
            if (!cJSON_IsString(item))
                continue;

            if (i > 0)
                strncat(dev->capabilities, ",", sizeof(dev->capabilities) - strlen(dev->capabilities) - 1);

            strncat(dev->capabilities, item->valuestring, sizeof(dev->capabilities) - strlen(dev->capabilities) - 1);
        }
        printf("capabilities = %s\n", dev->capabilities);
    }
    cJSON_Delete(root);
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
        if (state_callback)
        {
            state_callback(json);
        }
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

void my_device_manager_init(void)
{
    memset(devices, 0, sizeof(devices));
    my_mqtt_register_callback(my_device_manager_update);

    pthread_t tid;
    pthread_create(&tid, NULL, my_device_manager_check_task, NULL);
    pthread_detach(tid);

    printf("device manager init\n");
}
