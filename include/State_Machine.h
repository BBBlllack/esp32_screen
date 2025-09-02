#include "Arduino.h"

enum State
{
    STATE_IDLE, // 空闲，等待数据类型
    STATE_BIMG, // 正在接收二进制图片
    STATE_IMG,  // 正在接收Base64
    STATE_SYS   // 正在接收sys数据
};

int base64_decode(const char *input, uint8_t *output, size_t output_len);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);

bool process_bimg();

bool process_img();

bool process_sys();

void displayTempHumidity();

void displaySysInfo(SystemInfo sysinfo);

void updateSystemInfo(SystemInfo &sys, const char* jsonStr);
