
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <TJpg_Decoder.h>
#include "mbedtls/base64.h"
#include "Pins.h"           // 引脚定义
#include "System_Structs.h" // 系统信息显示结构体定义
#include "State_Machine.h"  // 状态机信息及process函数定义
#include <ArduinoJson.h>
#include <Wire.h>
#include "cst816t.h" // capacitive touch
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

State currentState = STATE_IDLE; // 状态机判断数据是否传输完毕
unsigned long stateStartTime = 0;
const unsigned long TIMEOUT_MS = 8000; // 8秒超时
SystemInfo sys;                        // 系统信息展示结构体

DHT_Unified dht(DHT22_DAT, DHTTYPE);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
TwoWire Wire2(0);
cst816t touchpad(Wire2, TP_RST, TP_IRQ);

// 缓存区：Base64 字符串 & 解码后的 JPEG
char base64_buffer[MAX_BASE64_LEN];
uint8_t base64_image_buffer[MAX_BASE64_IMAGE_LEN];
// uint8_t bin_image_buffer[MAX_BASE64_IMAGE_LEN]; // 两个缓冲区会崩

void setup()
{
    Serial0.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 1, 0);

    tft.init(HEIGHT, WIDTH); // Init ST7789 280x240
    // tft.setSPISpeed(60000000);

    tft.fillScreen(ST77XX_BLACK);
    tft.setRotation(1); // 旋转屏幕 90°

    // 触摸驱动通信设置
    Wire2.begin(TP_SDA, TP_SCL);
    touchpad.begin(mode_motion);

    // DHT22 传感器
    dht.begin();

    TJpgDec.setCallback(tft_output);
    Serial0.println("System boot");
}

void loop()
{

    if (touchpad.available())
    {
        switch (touchpad.gesture_id)
        {
        case GESTURE_NONE:
            Serial0.println("NONE");
            break;
        case GESTURE_SWIPE_DOWN:
            Serial0.println("SWIPE DOWN");
            break;
        case GESTURE_SWIPE_UP:
            Serial0.println("SWIPE UP");
            displayTempHumidity();
            break;
        case GESTURE_SWIPE_LEFT:
            Serial0.println("SWIPE LEFT");
            break;
        case GESTURE_SWIPE_RIGHT:
            Serial0.println("SWIPE RIGHT");
            break;
        case GESTURE_SINGLE_CLICK:
            Serial0.println("SINGLE CLICK");
            break;
        case GESTURE_DOUBLE_CLICK:
            Serial0.println("DOUBLE CLICK");
            displaySysInfo(sys);
            break;
        case GESTURE_LONG_PRESS:
            Serial0.println("LONG PRESS");
            break;
        default:
            Serial0.println("?");
            break;
        }
        Serial0.printf("x: %d, y: %d \n", touchpad.x, touchpad.y);
    }

    switch (currentState)
    {
    case STATE_IDLE:
        if (Serial1.available())
        {
            uint8_t datatype = Serial1.read();
            if (datatype == 0x00)
            {
                currentState = STATE_BIMG;
            }
            else if (datatype == 0x01)
            {
                currentState = STATE_IMG;
            }
            else if (datatype == 0x02)
            {
                currentState = STATE_SYS;
            }
            stateStartTime = millis(); // 记录进入状态的时间
        }
        break;

    case STATE_BIMG:
        if (process_bimg())
        { // 返回true表示接收完了
            currentState = STATE_IDLE;
        }
        break;

    case STATE_IMG:
        if (process_img())
        { // 返回true表示接收完了
            currentState = STATE_IDLE;
        }
        break;

    case STATE_SYS:
        if (process_sys())
        { // 返回true表示接收完了
            currentState = STATE_IDLE;
        }
        break;
    }

    // 通用超时检测
    if (currentState != STATE_IDLE &&
        (millis() - stateStartTime > TIMEOUT_MS))
    {
        Serial0.println("Timeout, resetting state machine!");
        currentState = STATE_IDLE;
    }
}

int base64_decode(const char *input, uint8_t *output, size_t output_len)
{
    size_t out_len = 0;
    int ret = mbedtls_base64_decode(output, output_len, &out_len, (const unsigned char *)input, strlen(input));
    if (ret == 0)
    {
        return out_len; // 返回解码后的字节数
    }
    else
    {
        return -1; // 出错
    }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

bool process_bimg()
{
    static size_t received = 0;
    static size_t expected = 0;

    // 先接收4字节长度
    if (expected == 0 && Serial1.available() >= 4)
    {
        expected = ((uint32_t)Serial1.read() << 24) |
                   ((uint32_t)Serial1.read() << 16) |
                   ((uint32_t)Serial1.read() << 8) |
                   (uint32_t)Serial1.read();
        Serial0.printf("Expecting %d bytes JPEG\n", expected);
        received = 0;
    }

    // 接收 JPEG 数据
    while (expected > 0 && Serial1.available() && received < expected)
    {
        base64_image_buffer[received++] = Serial1.read();
    }

    // 收完后显示
    if (expected > 0 && received == expected)
    {
        Serial0.printf("JPEG received: %d bytes\n", received);

        // 显示
        TJpgDec.drawJpg(0, 0, base64_image_buffer, received);

        // 重置
        expected = 0;
        received = 0;
        return true;
    }
    return false;
}

bool process_img()
{
    static size_t len = 0;
    while (Serial1.available())
    {
        char c = Serial1.read();
        if (c == '\n')
        {
            base64_buffer[len] = '\0';
            // Serial0.printf("base64: %s %d \n", buffer, len);
            Serial0.println("Base64 received, decoding...");
            // 解码
            size_t out_len = 0;
            if (mbedtls_base64_decode(base64_image_buffer, MAX_BASE64_IMAGE_LEN, &out_len,
                                      (const unsigned char *)base64_buffer, strlen(base64_buffer)) == 0)
            {
                Serial0.printf("Decoded JPEG size: %d bytes\n", out_len);

                // 用 TJpg_Decoder 显示图像
                // tft.fillScreen(ST77XX_BLACK);
                // delay(1000);
                TJpgDec.drawJpg(0, 0, base64_image_buffer, out_len);
            }
            else
            {
                Serial0.println("Base64 decode failed!");
            }

            len = 0; // 重置缓冲
            return true;
        }
        else
        {
            if (len < MAX_BASE64_LEN - 1)
            {
                base64_buffer[len++] = c;
            }
            else
            {
                Serial0.println("base64 buffer overflow!");
                len = 0;
                return true;
            }
        }
    }
    return false;
}

bool process_sys()
{
    static size_t received = 0;
    static size_t expected = 0;

    // 先接收4字节长度
    if (expected == 0 && Serial1.available() >= 4)
    {
        expected = ((uint32_t)Serial1.read() << 24) |
                   ((uint32_t)Serial1.read() << 16) |
                   ((uint32_t)Serial1.read() << 8) |
                   (uint32_t)Serial1.read();
        Serial0.printf("Expecting %d bytes JSON\n", expected);
        received = 0;
    }

    // 接收 JSON 数据
    while (expected > 0 && Serial1.available() && received < expected)
    {
        base64_buffer[received++] = Serial1.read();
    }

    // 收完后显示
    if (expected > 0 && received == expected)
    {
        Serial0.printf("JSON received: %d bytes\n", received);
        base64_buffer[received] = '\0'; // 结尾

        // 显示
        Serial0.printf("%s\n", base64_buffer);
        updateSystemInfo(sys, base64_buffer);

        // 输出
        Serial.printf("CPU: %d cores, %d%% usage\n", sys.cpu.cores, sys.cpu.usage);
        Serial.printf("Memory: total=%d, used=%d, free=%d\n", sys.memory.total, sys.memory.used, sys.memory.free);
        Serial.printf("Disk: total=%d, used=%d, free=%d\n", sys.disk.total, sys.disk.used, sys.disk.free);

        // 重置
        expected = 0;
        received = 0;
        return true;
    }

    return false;
}

// 显示温湿度的函数
void displayTempHumidity()
{
    sensors_event_t tempEvent, humEvent;

    // 读取温湿度
    dht.temperature().getEvent(&tempEvent);
    dht.humidity().getEvent(&humEvent);

    float temperature = isnan(tempEvent.temperature) ? NAN : tempEvent.temperature;
    float humidity = isnan(humEvent.relative_humidity) ? NAN : humEvent.relative_humidity;

    // 清屏
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(true);
    tft.setTextSize(2);

    // 基准位置
    int baseX = 10;
    int baseY = 40; // 温度文字的起始 y 坐标
    int barHeight = 20;
    int barWidthMax = tft.width() - 2 * baseX;

    // ---------- 温度显示 ----------
    tft.setCursor(baseX, baseY);
    tft.setTextColor(ST77XX_RED);
    if (!isnan(temperature))
    {
        tft.print("Temp: ");
        tft.print(temperature);
        tft.println(" C");
    }
    else
    {
        tft.println("Temp: Error!");
    }

    // 温度条
    int tempBarWidth = isnan(temperature) ? 0 : map(temperature, 0, 50, 0, barWidthMax);
    int tempBarY = baseY + 30; // 相对基准位置
    tft.fillRect(baseX, tempBarY, tempBarWidth, barHeight, ST77XX_RED);
    tft.drawRect(baseX, tempBarY, barWidthMax, barHeight, ST77XX_WHITE);

    // ---------- 湿度显示 ----------
    int humTextY = tempBarY + barHeight + 20; // 湿度文字相对温度条
    tft.setCursor(baseX, humTextY);
    tft.setTextColor(ST77XX_CYAN);
    if (!isnan(humidity))
    {
        tft.print("Humidity: ");
        tft.print(humidity);
        tft.println(" %");
    }
    else
    {
        tft.println("Humidity: Error!");
    }

    // 湿度条
    int humBarWidth = isnan(humidity) ? 0 : map(humidity, 0, 100, 0, barWidthMax);
    int humBarY = humTextY + 30; // 湿度条相对湿度文字
    tft.fillRect(baseX, humBarY, humBarWidth, barHeight, ST77XX_BLUE);
    tft.drawRect(baseX, humBarY, barWidthMax, barHeight, ST77XX_WHITE);
}

void displaySysInfo(SystemInfo sysinfo)
{
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(true);

    // 设置文字大小
    tft.setTextSize(2);

    // 1. CPU 信息
    tft.setCursor(0, 20);
    tft.setTextColor(ST77XX_YELLOW);
    String cpuLine = "CPU: cores:" + String(sysinfo.cpu.cores) + " usage:" + String(sysinfo.cpu.usage) + "%";
    tft.println(cpuLine);

    // 绘制 CPU 使用率条
    int cpuBarWidth = map(sysinfo.cpu.usage, 0, 100, 0, tft.width() - 20);
    tft.fillRect(10, 50, cpuBarWidth, 20, ST77XX_RED);
    tft.drawRect(10, 50, tft.width() - 20, 20, ST77XX_WHITE);

    // 2. 内存信息
    tft.setCursor(0, 80);
    tft.setTextColor(ST77XX_CYAN);
    String memLine = "Mem: total:" + String(sysinfo.memory.total) +
                     " used:" + String(sysinfo.memory.used) +
                     " free:" + String(sysinfo.memory.free) + "(MB)";
    tft.println(memLine);

    // 绘制内存占用条
    int memBarWidth = map(sysinfo.memory.used, 0, sysinfo.memory.total, 0, tft.width() - 20);
    tft.fillRect(10, 110, memBarWidth, 20, ST77XX_BLUE);
    tft.drawRect(10, 110, tft.width() - 20, 20, ST77XX_WHITE);

    // 3. 磁盘信息
    tft.setCursor(0, 140);
    tft.setTextColor(ST77XX_GREEN);
    String diskLine = "Disk: total:" + String(sysinfo.disk.total) +
                      " used:" + String(sysinfo.disk.used) +
                      " free:" + String(sysinfo.disk.free) + "(GB)";
    tft.println(diskLine);

    // 绘制磁盘占用条
    int diskBarWidth = map(sysinfo.disk.used, 0, sysinfo.disk.total, 0, tft.width() - 20);
    tft.fillRect(10, 170, diskBarWidth, 20, ST77XX_ORANGE);
    tft.drawRect(10, 170, tft.width() - 20, 20, ST77XX_WHITE);
}
