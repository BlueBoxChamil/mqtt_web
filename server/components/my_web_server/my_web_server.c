/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2026-07-20 09:13:18
 * @LastEditTime: 2026-07-20 17:21:32
 * @FilePath: /components/my_web_server/my_web_server.c
 * @Description:
 * Copyright (c) 2026 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */
#include "my_web_server.h"
static struct mg_connection *ws_client = NULL;
static web_msg_t web_queue[WEB_QUEUE_SIZE];
static int web_head = 0;
static int web_tail = 0;
static int web_count = 0;
static pthread_mutex_t web_mutex = PTHREAD_MUTEX_INITIALIZER;

void my_web_queue_push(const char *json)
{
    pthread_mutex_lock(&web_mutex);
    if (web_count < WEB_QUEUE_SIZE)
    {
        strncpy(web_queue[web_tail].data, json, sizeof(web_queue[web_tail].data) - 1);
        web_tail++;

        if (web_tail >= WEB_QUEUE_SIZE)
            web_tail = 0;

        web_count++;
    }
    pthread_mutex_unlock(&web_mutex);
}

int my_web_queue_pop(web_msg_t *msg)
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

static uint32_t parse_id(struct mg_str id)
{
    uint32_t value = 0;
    for (int i = 0; i < id.len; i++)
    {
        value = value * 10 + (id.buf[i] - '0');
    }
    return value;
}

static void api_device_list(struct mg_connection *c)
{
    my_device_t list[10];
    int num = my_device_manager_get_list(list, 10);

    cJSON *array = cJSON_CreateArray();

    for (int i = 0; i < num; i++)
    {
        cJSON *obj = cJSON_CreateObject();

        cJSON_AddNumberToObject(obj, "id", list[i].id);
        cJSON_AddStringToObject(obj, "name", list[i].name);
        cJSON_AddStringToObject(obj, "type", list[i].type);
        cJSON_AddNumberToObject(obj, "online", list[i].online);
        cJSON_AddItemToArray(array, obj);
    }
    char *json = cJSON_Print(array);
    mg_http_reply(
        c,
        200,
        "Content-Type: application/json\r\n",
        "%s",
        json);
    free(json);
    cJSON_Delete(array);
}

static void api_device_get(struct mg_connection *c, struct mg_str id)
{
    uint32_t device_id = parse_id(id);
    my_device_t device;

    if (my_device_manager_get(device_id, &device) != 0)
    {
        mg_http_reply(c, 404, "", "device not found");
        return;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", device.id);
    cJSON_AddStringToObject(root, "name", device.name);
    cJSON_AddStringToObject(root, "type", device.type);
    cJSON_AddNumberToObject(root, "online", device.online);
    cJSON_AddStringToObject(root, "state", device.state);

    char *json = cJSON_Print(root);
    mg_http_reply(
        c,
        200,
        "Content-Type: application/json\r\n",
        "%s",
        json);
    free(json);
    cJSON_Delete(root);
}

static void api_device_cmd(
    struct mg_connection *c,
    struct mg_http_message *hm,
    struct mg_str id)
{
    uint32_t device_id = parse_id(id);
    printf("device_id = %d\n", device_id);
    if (my_device_manager_send_cmd(device_id, hm->body.buf) == 0)
    {
        mg_http_reply(c, 200, "", "ok");
    }
    else
    {
        mg_http_reply(c, 500, "", "mqtt error");
    }
}

static void http_callback(struct mg_connection *c, int ev, void *ev_data)
{
    // printf("event=%d\n", ev);
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = ev_data;
        printf("uri=%.*s\n", (int)hm->uri.len, hm->uri.buf);

        if (mg_strcmp(hm->uri, mg_str("/ws")) == 0)
        {
            mg_ws_upgrade(c, hm, NULL);
            return;
        }

        // 网站输入 http://192.168.12.138:8080/api/device/list 触发
        if (mg_strcmp(hm->uri, mg_str("/api/device/list")) == 0)
        {
            api_device_list(c);
            return;
        }

        // curl -X POST http://192.168.5.194:8080/api/device/1/cmd -d '{"light":4}'
        // 运行这个就跑飞了
        struct mg_str id;
        char id_buf[20];
        id.buf = id_buf;

        if (mg_match(hm->uri, mg_str("/api/device/*/cmd"), &id))
        {
            api_device_cmd(c, hm, id);
            return;
        }

        if (mg_match(hm->uri, mg_str("/api/device/*"), &id))
        {
            api_device_get(c, id);
            return;
        }

        mg_http_reply(c, 404, "", "not found\n");
        // mg_http_reply(c, 200, "", "hello sss\n");
    }

    if (ev == MG_EV_WS_OPEN)
    {
        printf("websocket connected\n");
        ws_client = c;
        mg_ws_send(
            c,
            "hello websocket",
            strlen("hello websocket"),
            WEBSOCKET_OP_TEXT);

        return;
    }

    if (ev == MG_EV_CLOSE)
    {
        if (c == ws_client)
        {
            printf("websocket disconnected\n");
            ws_client = NULL;
        }
        return;
    }
}

static void *web_task(void *arg)
{
    struct mg_mgr mgr;

    mg_mgr_init(&mgr);

    mg_http_listen(&mgr, "http://0.0.0.0:8080", http_callback, NULL);

    printf("web server start:8080\n");

    while (1)
    {
        mg_mgr_poll(&mgr, 10);

        web_msg_t msg;
        while (my_web_queue_pop(&msg) == 0)
        {
            if (ws_client)
            {
                // printf("into ws_client\n");
                mg_ws_send(ws_client, msg.data, strlen(msg.data), WEBSOCKET_OP_TEXT);
            }
        }
    }

    mg_mgr_free(&mgr);

    return NULL;
}

void my_web_server_notify(const char *json)
{
    // printf("into my_web_server_notify\n");
    my_web_queue_push(json);
    // if (ws_client)
    // {
    //     printf("into ws_client\n");
    //     mg_ws_send(ws_client, json, strlen(json), WEBSOCKET_OP_TEXT);
    // }
}

void my_web_server_init(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, web_task, NULL);
    pthread_detach(tid);

    my_device_manager_register_callback(my_web_server_notify);
}
