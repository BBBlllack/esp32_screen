#!/etc/nginx/fastcgi_scripts/pys/.venv/bin/python3
# -*- coding: utf-8 -*-

import os
import io
import base64
import serial
import psutil
import json

'''
00000000 00000000 00000000 00000000 00000000 
数据类型
00 二进制图片 图片长度 二进制流
01 base64图片 \n
02 sys_info 内存CPU
'''

# 串口配置 (修改为你的 ESP32 串口号)
SERIAL_PORT = "/dev/ttyS5"
BAUDRATE = 115200

def imageFun():  
    dt = "img"
    coder = "utf-8"
    with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1) as ser:
        data = "abvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuuiabvetrhrtuyrtyutyuretrteyrttyrugffuui"
        num = '10'
        if dt == "bimg":
            f = 0
            ser.write(f.to_bytes(1,"big"))
            ser.write(len(data.encode(coder)).to_bytes(4,"big"))
            ser.write(data.encode(coder))
        if dt == "img":
            f = 1
            ser.write(f.to_bytes(1,"big"))
            ser.write((data + '\n').encode(coder))
        if dt == 'mem':
            f = 2
            ser.write(f.to_bytes(1,'big'))
            ser.write((num + '\n').encode(coder))

def sys_info():
    # CPU
    cpu_cores = psutil.cpu_count(logical=False)
    logical_cores = psutil.cpu_count(logical=True)
    cpu_usage = int(psutil.cpu_percent(interval=1))

    # 内存
    mem = psutil.virtual_memory()
    total_mem = mem.total // (1024**2)   # MB
    used_mem = mem.used // (1024**2)     # MB
    available_mem = mem.available // (1024**2) # MB

    # 磁盘
    disk = psutil.disk_usage('/')
    total_disk = disk.total // (1024**3) # GB
    used_disk = disk.used // (1024**3)   # GB
    free_disk = disk.free // (1024**3)   # GB

    # 输出
    print(f"CPU核心: {cpu_cores}, 逻辑核心: {logical_cores}, CPU使用率: {cpu_usage}%")
    print(f"内存: 总 {total_mem} MB, 已用 {used_mem} MB, 可用 {available_mem} MB")
    print(f"磁盘: 总 {total_disk} GB, 已用 {used_disk} GB, 可用 {free_disk} GB")

# 收集系统信息
data = {
    "cpu": {"cores": psutil.cpu_count(False), "usage": int(psutil.cpu_percent())},
    "memory": {
        "total": psutil.virtual_memory().total // (1024**2),
        "used": psutil.virtual_memory().used // (1024**2),
        "free": psutil.virtual_memory().available // (1024**2)
    },
    "disk": {
        "total": psutil.disk_usage("/").total // (1024**3),
        "used": psutil.disk_usage("/").used // (1024**3),
        "free": psutil.disk_usage("/").free // (1024**3)
    }
}

json_str = json.dumps(data)
json_bytes = json_str.encode("utf-8")
length = len(json_bytes)

# 拼接协议
packet = bytearray()
packet.append(0x02)               # 起始
packet += length.to_bytes(4, "big") # 长度
packet += json_bytes

# 发送
with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1) as ser:
    ser.write(packet)