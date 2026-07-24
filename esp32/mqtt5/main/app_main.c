/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM 3

#define EXAMPLE_LED_NUMBERS 2

static const char *TAG = "mqtt5_example";

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

typedef struct
{
    char name[32];          // info上报
    char type[32];          // info上报
    char model[32];         // info上报:设备型号
    char fw_version[16];    // info上报:固件版本号,如 "1.0.3"
    char mac[18];           // info上报:MAC 地址,如 "AA:BB:CC:DD:EE:FF" + '\0'
    char capabilities[256]; // info上报:能力列表,存成逗号分隔字符串,如 "switch,brightness"
    char state[512];

} my_info_t;

typedef struct
{
    uint8_t light;      // 灯开关
    uint8_t mode;       // 灯模式
    uint16_t h;         // 色相
    uint8_t s;          // 饱和度
    uint8_t l;          // 明度
    uint8_t brightness; // 灯亮度
    uint8_t white;      // 白平衡
} my_state_t;

my_info_t my_info =
    {
        .name = "desk light",
        .type = "light",
        .capabilities = "switch,brightness,white,rgb",
};

my_state_t my_state =
    {

        .light = 0,
        .mode = 0,
        .h = 30,
        .s = 100,
        .l = 60,
        .brightness = 77,
        .white = 125};

char *my_info_to_json(const my_info_t *dev)
{
    if (dev == NULL)
    {
        ESP_LOGE(TAG, "Device pointer is NULL");
        return NULL;
    }

    // 1. 创建根对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return NULL;
    }

    // 2. 添加基本字符串字段
    cJSON_AddStringToObject(root, "name", dev->name);
    cJSON_AddStringToObject(root, "type", dev->type);
    cJSON_AddStringToObject(root, "model", dev->model);
    cJSON_AddStringToObject(root, "fw_version", dev->fw_version);
    cJSON_AddStringToObject(root, "mac", dev->mac);
    cJSON_AddStringToObject(root, "state", dev->state);

    // 3. 处理 capabilities：逗号分隔字符串 -> JSON 数组
    cJSON *cap_array = cJSON_CreateArray();
    if (cap_array == NULL)
    {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Failed to create capabilities array");
        return NULL;
    }

    // 复制字符串，因为 strtok 会修改原字符串
    char *cap_copy = strdup(dev->capabilities);
    if (cap_copy == NULL)
    {
        cJSON_Delete(cap_array);
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Failed to duplicate capabilities string");
        return NULL;
    }

    // 按逗号分割
    char *token = strtok(cap_copy, ",");
    while (token != NULL)
    {
        // 去除前导空格（如果有）
        while (*token == ' ')
            token++;
        // 去除尾部空格（可选）
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ')
            end--;
        *(end + 1) = '\0';

        // 添加到 JSON 数组
        cJSON *item = cJSON_CreateString(token);
        if (item == NULL)
        {
            // 如果创建失败，清理并退出
            free(cap_copy);
            cJSON_Delete(cap_array);
            cJSON_Delete(root);
            ESP_LOGE(TAG, "Failed to create string item");
            return NULL;
        }
        cJSON_AddItemToArray(cap_array, item);

        token = strtok(NULL, ",");
    }

    free(cap_copy); // 释放副本

    // 将数组挂载到根对象
    cJSON_AddItemToObject(root, "capabilities", cap_array);

    // 4. 渲染为字符串
    char *json_str = cJSON_Print(root);
    if (json_str == NULL)
    {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return NULL;
    }

    // 5. 释放根对象（json_str 是独立分配的，需要单独释放）
    cJSON_Delete(root);

    return json_str;
}

char *my_state_to_json(const my_state_t *state)
{
    if (state == NULL)
    {
        ESP_LOGE(TAG, "State pointer is NULL");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return NULL;
    }

    cJSON_AddNumberToObject(root, "light", state->light);
    cJSON_AddNumberToObject(root, "mode", state->mode);
    cJSON_AddNumberToObject(root, "h", state->h);
    cJSON_AddNumberToObject(root, "s", state->s);
    cJSON_AddNumberToObject(root, "l", state->l);
    cJSON_AddNumberToObject(root, "brightness", state->brightness);
    cJSON_AddNumberToObject(root, "white", state->white);

    char *json_str = cJSON_Print(root);
    if (json_str == NULL)
    {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);
    return json_str;
}

bool my_json_to_state(const char *json_str, my_state_t *state)
{
    if (json_str == NULL || state == NULL)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // 1. 解析 JSON 字符串
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return false;
    }

// 2. 辅助宏：读取数字字段（uint8_t）
#define GET_UINT8(obj, field, target)                                        \
    do                                                                       \
    {                                                                        \
        cJSON *item = cJSON_GetObjectItem(obj, field);                       \
        if (item != NULL)                                                    \
        {                                                                    \
            if (cJSON_IsNumber(item))                                        \
            {                                                                \
                target = (uint8_t)(item->valueint & 0xFF);                   \
            }                                                                \
            else                                                             \
            {                                                                \
                ESP_LOGW(TAG, "Field '%s' is not a number, ignored", field); \
            }                                                                \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            ESP_LOGW(TAG, "Field '%s' not found, keeping default", field);   \
        }                                                                    \
    } while (0)

    // 3. 解析 light（支持布尔和数字）
    cJSON *light_item = cJSON_GetObjectItem(root, "light");
    if (light_item != NULL)
    {
        if (cJSON_IsBool(light_item))
        {
            state->light = cJSON_IsTrue(light_item) ? 1 : 0;
        }
        else if (cJSON_IsNumber(light_item))
        {
            state->light = (uint8_t)(light_item->valueint != 0);
        }
        else
        {
            ESP_LOGW(TAG, "Field 'light' has unsupported type, ignored");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Field 'light' not found, keeping default");
    }

    // 4. 解析其他数字字段
    GET_UINT8(root, "mode", state->mode);
    // GET_UINT8(root, "h", state->h);
    GET_UINT8(root, "s", state->s);
    GET_UINT8(root, "l", state->l);
    GET_UINT8(root, "brightness", state->brightness);
    GET_UINT8(root, "white", state->white);

    cJSON *h_item = cJSON_GetObjectItem(root, "h");
    if (h_item != NULL)
    {
        if (cJSON_IsNumber(h_item))
        {
            int val = h_item->valueint;
            if (val >= 0 && val <= 65535)
            {
                state->h = (uint16_t)val;
            }
            else
            {
                ESP_LOGW(TAG, "Field 'h' value %d out of range (0-65535), ignored", val);
            }
        }
        else
        {
            ESP_LOGW(TAG, "Field 'h' is not a number, ignored");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Field 'h' not found, keeping default");
    }

#undef GET_UINT8

    // 5. 释放根对象
    cJSON_Delete(root);
    return true;
}

#include <stdint.h>

void led_strip_hsl2rgb(uint32_t h, uint32_t s, uint32_t l,
                       uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360;

    // HSL 全灰色
    if (s == 0)
    {
        uint32_t gray = l * 255 / 100;

        *r = gray;
        *g = gray;
        *b = gray;
        return;
    }

    // 转换范围到浮点
    float H = h / 360.0f;
    float S = s / 100.0f;
    float L = l / 100.0f;

    float q;

    if (L < 0.5f)
    {
        q = L * (1.0f + S);
    }
    else
    {
        q = L + S - L * S;
    }

    float p = 2.0f * L - q;

    float rgb[3];

    float t[3] = {
        H + 1.0f / 3.0f,
        H,
        H - 1.0f / 3.0f};

    for (uint8_t i = 0; i < 3; i++)
    {
        if (t[i] < 0)
            t[i] += 1;

        if (t[i] > 1)
            t[i] -= 1;

        if (t[i] < 1.0f / 6.0f)
        {
            rgb[i] = p + (q - p) * 6.0f * t[i];
        }
        else if (t[i] < 1.0f / 2.0f)
        {
            rgb[i] = q;
        }
        else if (t[i] < 2.0f / 3.0f)
        {
            rgb[i] = p + (q - p) * (2.0f / 3.0f - t[i]) * 6.0f;
        }
        else
        {
            rgb[i] = p;
        }
    }

    *r = (uint32_t)(rgb[0] * 255);
    *g = (uint32_t)(rgb[1] * 255);
    *b = (uint32_t)(rgb[2] * 255);
}

void white_to_rgb_int(uint8_t white, uint32_t *r, uint32_t *g, uint32_t *b) {
    const uint8_t stops[3][3] = {
        {255, 158,  68},
        {255, 255, 255},
        {163, 201, 255}
    };

    if (white <= 127) {
        // 0~127 映射到 0~255 的混合因子
        uint16_t factor = ((uint16_t)white * 255) / 127;  // 0..255
        *r = stops[0][0] + ((stops[1][0] - stops[0][0]) * factor + 128) / 255;
        *g = stops[0][1] + ((stops[1][1] - stops[0][1]) * factor + 128) / 255;
        *b = stops[0][2] + ((stops[1][2] - stops[0][2]) * factor + 128) / 255;
    } else {
        // 128~255 映射到 0~255 的混合因子
        uint16_t factor = ((uint16_t)(white - 128) * 255) / 127;
        *r = stops[1][0] + ((stops[2][0] - stops[1][0]) * factor + 128) / 255;
        *g = stops[1][1] + ((stops[2][1] - stops[1][1]) * factor + 128) / 255;
        *b = stops[1][2] + ((stops[2][2] - stops[1][2]) * factor + 128) / 255;
    }
}

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
rmt_channel_handle_t led_chan = NULL;
rmt_encoder_handle_t led_encoder = NULL;
rmt_transmit_config_t tx_config = {
    .loop_count = 0, // no transfer loop
};
uint32_t red = 0;
uint32_t green = 0;
uint32_t blue = 0;

void my_set_light(my_state_t *p)
{
    uint8_t mode = p->mode;

    if (p->light == 0)
    {
        for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++)
        {
            led_strip_pixels[i * 3 + 0] = 0; // g
            led_strip_pixels[i * 3 + 1] = 0; // r
            led_strip_pixels[i * 3 + 2] = 0; // b
        }
        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        return;
    }

    // 仅有开关的灯
    if (mode == 0)
    {
        green = 255;
        red = 255;
        blue = 255;
    }
    // 带色温的灯
    else if (mode == 1)
    {
        white_to_rgb_int(p->white, &red, &green, &blue);
    }
    // rgb灯
    else if (mode == 2)
    {
        led_strip_hsl2rgb(p->h, p->s, p->l, &red, &green, &blue);
    }
    // 带亮度控制的灯
    else if (mode == 3)
    {
        green = p->brightness;
        red = p->brightness;
        blue = p->brightness;
    }

    for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++)
    {
        led_strip_pixels[i * 3 + 0] = green; // g
        led_strip_pixels[i * 3 + 1] = red;   // r
        led_strip_pixels[i * 3 + 2] = blue;  // b
    }

    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}
static void my_devide_manager_cmd_update(const char *topic, const char *json)
{
    my_state_t tem;
    bool res = my_json_to_state(json, &tem);
    if (!res)
        return;

    // 暂时不做处理，直接原数据返回
    char *state_json = my_state_to_json(&tem);
    if (state_json)
    {
        int msg_id = esp_mqtt_client_publish(s_mqtt_client, "device/1/state", state_json, 0, 0, 0);
        cJSON_free(state_json);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    }

    // 先在下边添加rgb控制
    my_set_light(&tem);
}

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    s_mqtt_client = client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        // 添加cmd订阅
        msg_id = esp_mqtt_client_subscribe(client, "device/1/cmd", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // 发送info
        char *payload = my_info_to_json(&my_info);
        if (payload)
        {
            msg_id = esp_mqtt_client_publish(client, "device/1/info", payload, 0, 0, 0);
            cJSON_free(payload);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        }

        // 发送state
        char *state_json = my_state_to_json(&my_state);
        if (state_json)
        {
            msg_id = esp_mqtt_client_publish(client, "device/1/state", state_json, 0, 0, 0);
            cJSON_free(state_json);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        }

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        esp_mqtt_client_disconnect(client);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        if (strstr(event->topic, "cmd"))
        {
            my_devide_manager_cmd_update(event->topic, event->data);
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);

        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt5_app_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "/test/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = "user1",
        .credentials.authentication.password = "1",
        .session.last_will.topic = "/topic/will",
        .session.last_will.msg = "i will leave",
        .session.last_will.msg_len = 12,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

    esp_mqtt5_client_set_connect_property(client, &connect_property);

    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_INFO);
    esp_log_level_set("mqtt_example", ESP_LOG_INFO);
    esp_log_level_set("transport_base", ESP_LOG_WARN);
    esp_log_level_set("esp-tls", ESP_LOG_WARN);
    esp_log_level_set("transport", ESP_LOG_WARN);
    esp_log_level_set("outbox", ESP_LOG_INFO);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    ESP_LOGI(TAG, "Create RMT TX channel");

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    my_set_light(&my_state);
    // for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++)
    // {
    //     // Build RGB pixels
    //     // hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
    //     led_strip_hsl2rgb(30, 100, 50, &red, &green, &blue);
    //     led_strip_pixels[i * 3 + 0] = green; // g
    //     led_strip_pixels[i * 3 + 1] = red;   // r
    //     led_strip_pixels[i * 3 + 2] = blue;  // b
    // }
    // // Flush RGB values to LEDs
    // ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    // ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    ESP_ERROR_CHECK(example_connect());

    mqtt5_app_start();
}
