#include "my_web_server.h"

static void http_callback(struct mg_connection *c, int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = ev_data;

        // 网站输入 http://192.168.12.138:8080/api/device/list 触发
        if (mg_strcmp(hm->uri, mg_str("/api/device/list")) == 0)
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

            return;
        }

        struct mg_str id;
        if (mg_match(hm->uri, mg_str("/api/device/*/cmd"), &id))
        {
            printf("id=%.*s\n", (int)id.len, id.buf);
            my_device_manager_send_cmd(1, hm->body.buf);

            mg_http_reply(c, 200, "", "ok");

            return;
        }

        mg_http_reply(c, 404, "", "not found\n");
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
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return NULL;
}

void my_web_server_init(void)
{
    pthread_t tid;

    pthread_create(&tid, NULL, web_task, NULL);

    pthread_detach(tid);
}