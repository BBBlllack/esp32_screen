
#define TFT_CS 7
#define TFT_RST 10
#define TFT_DC 6
#define WIDTH 280
#define HEIGHT 240

#define TFT_MOSI 3 // Data out
#define TFT_SCLK 2 // Clock out

// 触摸引脚
#define TP_SDA 4
#define TP_SCL 5
#define TP_RST 9
#define TP_IRQ 8

// DHT22 传感器
#define DHTTYPE DHT22
#define DHT22_DAT 13

#define MAX_BASE64_LEN (60 * 1024)        // 假设传输的图片最大 Base64 长度
#define MAX_BASE64_IMAGE_LEN (120 * 1024) // 解码后最大字节数
