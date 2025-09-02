#!/etc/nginx/fastcgi_scripts/pys/.venv/bin/python3
# -*- coding: utf-8 -*-

protocol = '''
[1 字节标志位] + [4 字节长度] + [图像数据]
其中 1 字节标志位格式：

bit7–bit6 （前两位）：是否 Base64
00 = 原始 JPEG
01 = Base64 
bit5–bit0（后 6 位）：填充 0
'''

import cgi, cgitb
from PIL import Image
import os
import io
import base64
import serial

resize_width = 240   # 调整后的图像宽
resize_height = 280  # 调整后的图像高
scale = 1
SERIAL_PORT = "/dev/ttyS5"  # 发送数据串口
BAUDRATE = 115200           # 波特率

cgitb.enable()  # 出错时显示调试信息

print("Content-Type: text/html;charset=utf-8\n")

form = cgi.FieldStorage()

def process_image(file_item, resize_width=800, resize_height=600, scale=1, isBase64=True):
    # 读取上传文件
    img = Image.open(file_item.file)

    # 判断是否是宽图，如果宽 > 高则旋转90度
    # if img.width > img.height:
    #   img = img.rotate(270, expand=True)

    # 缩放
    # img = img.resize((resize_width // scale, resize_height // scale))
    img = img.resize((resize_height // scale, resize_width // scale))

    # 转换颜色模式
    if img.mode != "RGB":
        img = img.convert("RGB")

    # 保存到 buffer
    buffer = io.BytesIO()
    img.save(buffer, format="JPEG", quality=70)
    
    if isBase64:
        return base64.b64encode(buffer.getvalue()).decode("utf-8")
    else:
        return buffer.getvalue()

if "file" not in form:
    # 上传页面（加开关：checkbox 控制是否启用 Base64）
    print("""
    <html>
    <body>
    <h2>上传图片</h2>
    <form enctype="multipart/form-data" method="post">
      <input type="file" name="file"><br><br>
      <label>
        <input type="checkbox" name="use_base64" value="1" checked>
        使用 Base64 编码
      </label><br><br>
      <input type="submit" value="上传">
    </form>
    </body>
    </html>
    """)
else:
    file_item = form['file']
    if file_item.filename:
        # 根据是否勾选 Base64 开关
        encodeBase64 = form.getvalue("use_base64") == "1"

        encoded_img = process_image(file_item, resize_width, resize_height, scale, encodeBase64)

        # 构造 1 字节标志位
        flag = 1 if encodeBase64 else 0
        try:
            with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1) as ser:
                ser.write(flag.to_bytes(1, "big"))      # 发送 1 字节标志
                if encodeBase64:           
                    ser.write((encoded_img + "\n").encode("utf-8"))
                else:
                    # 先发长度，再发数据，避免粘包
                    ser.write(len(encoded_img).to_bytes(4, "big"))
                    ser.write(encoded_img)
        except Exception as e:
            print(f"<h1>串口发送失败: {e}</h1>")

        # 输出 HTML 显示压缩后的图
        print(f"""
        <html>
        <body>
        <h2>上传成功！采样压缩后图片如下：</h2>
        <h3>Base64 模式: {encodeBase64}</h3>
        <img src="data:image/jpeg;base64,{encoded_img if encodeBase64 else base64.b64encode(encoded_img).decode("utf-8")}">
        </body>
        </html>
        """)
    else:
        print("<h2>没有选择文件</h2>")
