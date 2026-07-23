"""
Author: BlueboxChamil
Date: 2026-07-23 13:52:01
LastEditTime: 2026-07-23 13:53:17
FilePath: \11.py
Description:
Copyright (c) 2026 by BlueboxChamil, All Rights Reserved.
"""

import paho.mqtt.client as mqtt

# 1. 服务器与认证配置
BROKER = "192.168.12.171"
PORT = 1883
USERNAME = "user1"
PASSWORD = "1"

# 2. 主题配置
TOPIC_CMD = "device/1/cmd"
TOPIC_STATE = "device/1/state"


# 连接成功时的回调函数
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("成功连接到 MQTT 服务器！")
        # 订阅控制指令主题
        client.subscribe(TOPIC_CMD)
        print(f"已成功订阅主题: {TOPIC_CMD}")
    else:
        print(f"连接失败，错误码: {rc}")


# 收到消息时的回调函数
def on_message(client, userdata, msg):
    # 获取原始二进制 payload，避免强行 decode 导致非文本数据报错
    payload = msg.payload

    # 尝试安全打印文本，如果不是文本则打印二进制长度
    try:
        msg_str = payload.decode("utf-8")
    except UnicodeDecodeError:
        msg_str = f"[Binary data, length: {len(payload)} bytes]"

    print(f"收到来自 [{msg.topic}] 的消息: {msg_str}")

    # 使用 device/1/state 主题原样发送接收到的消息
    client.publish(TOPIC_STATE, payload)
    print(f"已原样转发到 [{TOPIC_STATE}]")


def main():
    # 初始化客户端（指定 CallbackAPIVersion.VERSION1 兼容标准回调签名）
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

    # 设置账号密码
    client.username_pw_set(USERNAME, PASSWORD)

    # 绑定回调事件
    client.on_connect = on_connect
    client.on_message = on_message

    # 连接到服务器
    print(f"正在连接到 MQTT 服务器 {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, 60)

    # 保持循环监听（阻塞模式，持续运行）
    client.loop_forever()


if __name__ == "__main__":
    main()
