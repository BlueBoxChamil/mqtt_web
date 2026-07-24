import paho.mqtt.client as mqtt

# 1. 服务器与认证配置
BROKER = "192.168.12.171"
PORT = 1883
USERNAME = "user1"
PASSWORD = "1"

# 2. 主题配置
TOPIC_CMD = "device/1/cmd"
TOPIC_STATE = "device/1/state"

# 3. 遗嘱配置（新增）
WILL_TOPIC = "device/1/lwt"        # 遗嘱主题，建议独立命名
WILL_PAYLOAD = "{\"id\":1,\"online\":0}"          # 遗嘱消息内容
WILL_QOS = 1                       # 服务质量（可选 0/1/2）
WILL_RETAIN = False                # 是否保留遗嘱消息（可选）


# 连接成功时的回调函数
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("成功连接到 MQTT 服务器！")
        client.subscribe(TOPIC_CMD)
        print(f"已成功订阅主题: {TOPIC_CMD}")
    else:
        print(f"连接失败，错误码: {rc}")


# 收到消息时的回调函数
def on_message(client, userdata, msg):
    payload = msg.payload
    try:
        msg_str = payload.decode("utf-8")
    except UnicodeDecodeError:
        msg_str = f"[Binary data, length: {len(payload)} bytes]"

    print(f"收到来自 [{msg.topic}] 的消息: {msg_str}")

    # 原样转发到 state 主题
    client.publish(TOPIC_STATE, payload)
    print(f"已原样转发到 [{TOPIC_STATE}]")


def main():
    # 初始化客户端
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

    # 设置账号密码
    client.username_pw_set(USERNAME, PASSWORD)

    # ---------- 设置遗嘱消息（新增） ----------
    client.will_set(
        topic=WILL_TOPIC,
        payload=WILL_PAYLOAD,
        qos=WILL_QOS,
        retain=WILL_RETAIN
    )
    print(f"遗嘱已设置：主题 [{WILL_TOPIC}]，内容 [{WILL_PAYLOAD}]")

    # 绑定回调事件
    client.on_connect = on_connect
    client.on_message = on_message

    # 连接到服务器
    print(f"正在连接到 MQTT 服务器 {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, 60)

    # 保持循环监听（阻塞模式）
    client.loop_forever()


if __name__ == "__main__":
    main()