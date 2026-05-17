#pragma once
#include <stdio.h>
#define ESP_LOGI(tag, fmt, ...) printf("[I] " fmt "\n", ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "[E] " fmt "\n", ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[W] " fmt "\n", ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)
#define ESP_LOGV(tag, fmt, ...)
