#include "my_mqtt.h"

#define MQTT_ADDRESS "tcp://192.168.12.171:1883"
#define MQTT_CLIENT_ID "linux_mqtt_client_001"
#define MQTT_USERNAME "user2"
#define MQTT_PASSWORD "2"

static const char *sub_topics[] = {
    "device/1/state", // 设备 → 服务器 上报当前状态
    //"device/1/cmd",  // 服务器 → 设备 下发控制命令,server 不用订阅
    "device/1/info", // 设备 → 服务器 设备信息注册
};
static MQTTClient client;
static mqtt_recv_callback_t recv_cb = NULL;

static my_mqtt_queue_t my_mqtt_queue[QUEUE_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void my_mqtt_register_callback(mqtt_recv_callback_t cb)
{
    recv_cb = cb;
}

static int my_mqtt_queue_push(my_mqtt_queue_t *msg)
{
    pthread_mutex_lock(&mutex);
    if (count >= QUEUE_SIZE)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    memcpy(&my_mqtt_queue[tail], msg, sizeof(my_mqtt_queue_t));
    tail++;
    if (tail >= QUEUE_SIZE)
    {
        tail = 0;
    }
    count++;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return 0;
}

static int my_mqtt_queue_pop(my_mqtt_queue_t *msg)
{
    pthread_mutex_lock(&mutex);
    while (count == 0)
    {
        pthread_cond_wait(&cond, &mutex);
    }

    memcpy(msg, &my_mqtt_queue[head], sizeof(my_mqtt_queue_t));
    head++;

    if (head >= QUEUE_SIZE)
    {
        head = 0;
    }
    count--;

    pthread_mutex_unlock(&mutex);
    return 0;
}

static void *my_queue_thread_task(void *arg)
{
    my_mqtt_queue_t msg;
    // 在这里处理mqtt信息
    while (1)
    {
        my_mqtt_queue_pop(&msg);
        
        // 传入到my_device_manager_update中处理
        if (recv_cb)
        {
            recv_cb(msg.topic, msg.data);
        }
    }

    return NULL;
}

static int my_mqtt_message_callback(
    void *context,
    char *topicName,
    int topicLen,
    MQTTClient_message *message)
{
    // printf("\n========== MQTT RX ==========\n");
    // printf("topic:%s\n",
    //        topicName);

    // printf("data:%.*s\n",
    //        message->payloadlen,
    //        (char *)message->payload);
    // printf("=============================\n");

    my_mqtt_queue_t msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.topic, topicName);
    memcpy(msg.data, message->payload, message->payloadlen);
    my_mqtt_queue_push(&msg);

    return 1;
}

int my_mqtt_init(void)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    // 创建client
    int ret = MQTTClient_create(
        &client,
        MQTT_ADDRESS,
        MQTT_CLIENT_ID,
        MQTTCLIENT_PERSISTENCE_NONE,
        NULL);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf(
            "mqtt create failed:%d\n",
            ret);
        return -1;
    }
    // 设置用户名密码
    conn_opts.username = MQTT_USERNAME;
    conn_opts.password = MQTT_PASSWORD;

    /*
     * 设置异步接收回调
     */
    MQTTClient_setCallbacks(
        client,
        NULL,
        NULL,
        my_mqtt_message_callback,
        NULL);

    // 连接broker
    ret = MQTTClient_connect(client, &conn_opts);

    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf("mqtt connect failed:%d\n", ret);
        return -2;
    }
    printf("mqtt connected\n");

    // 订阅
    for (uint32_t i = 0; i < sizeof(sub_topics) / sizeof(sub_topics[0]); i++)
    {
        MQTTClient_subscribe(client, sub_topics[i], 1);
        printf("subscribe:%s\n", sub_topics[i]);
    }

    // 创建线程
    pthread_t tid;
    pthread_create(&tid, NULL, my_queue_thread_task, NULL);
    pthread_detach(tid);

    return 0;
}

int my_mqtt_publish(const char *topic, const char *data)
{

    MQTTClient_message msg = MQTTClient_message_initializer;

    MQTTClient_deliveryToken token;

    msg.payload = (void *)data;
    msg.payloadlen = strlen(data);
    msg.qos = 1;
    msg.retained = 0;

    int ret = MQTTClient_publishMessage(client, topic, &msg, &token);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        return -1;
    }

    MQTTClient_waitForCompletion(client, token, 1000);
    return 0;
}

int my_mqtt_deinit(void)
{
    int ret;
    for (uint32_t i = 0; i < sizeof(sub_topics) / sizeof(sub_topics[0]); i++)
    {
        ret = MQTTClient_unsubscribe(client, sub_topics[i]);
        if (ret != MQTTCLIENT_SUCCESS)
        {
            printf("unsubscribe[%d] failed:%d\n", i, ret);
        }
    }

    ret = MQTTClient_disconnect(client, 1000);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        printf("disconnect failed:%d\n", ret);
    }

    MQTTClient_destroy(&client);
    printf("mqtt disconnected\n");
    return 0;
}