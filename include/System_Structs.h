#include <ArduinoJson.h>

struct CPUInfo {
    int cores;
    int usage;
};

struct MemoryInfo {
    int total;
    int used;
    int free;
};

struct DiskInfo {
    int total;
    int used;
    int free;
};

struct SystemInfo {
    CPUInfo cpu;
    MemoryInfo memory;
    DiskInfo disk;
};

void updateSystemInfo(SystemInfo &sys, const char* jsonStr) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    // 更新 CPU 信息
    JsonObject cpu = doc["cpu"];
    sys.cpu.cores = cpu["cores"];
    sys.cpu.usage = cpu["usage"];

    // 更新 Memory 信息
    JsonObject memory = doc["memory"];
    sys.memory.total = memory["total"];
    sys.memory.used = memory["used"];
    sys.memory.free = memory["free"];

    // 更新 Disk 信息
    JsonObject disk = doc["disk"];
    sys.disk.total = disk["total"];
    sys.disk.used = disk["used"];
    sys.disk.free = disk["free"];
}
