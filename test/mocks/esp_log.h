#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_LOG_NONE,       /*!< No log output */
    ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    ESP_LOG_WARN,       /*!< Error conditions from which recovery might be possible */
    ESP_LOG_INFO,       /*!< Informational messages which describe normal flow of events */
    ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    ESP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} esp_log_level_t;

// Define the macros to use printf
#define ESP_LOG_FORMAT(letter, format)  "V (" #letter ") %s: " format "\n"
#define ESP_LOG_LEVEL(level, tag, format, ...) do { if (level==ESP_LOG_ERROR) { printf(ESP_LOG_FORMAT(E, format), tag, ##__VA_ARGS__); } else if (level==ESP_LOG_WARN) { printf(ESP_LOG_FORMAT(W, format), tag, ##__VA_ARGS__); } else if (level==ESP_LOG_DEBUG) { printf(ESP_LOG_FORMAT(D, format), tag, ##__VA_ARGS__); } else { printf(ESP_LOG_FORMAT(I, format), tag, ##__VA_ARGS__); } } while(0)

#define ESP_LOGE(tag, format, ...) ESP_LOG_LEVEL(ESP_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) ESP_LOG_LEVEL(ESP_LOG_WARN,    tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) ESP_LOG_LEVEL(ESP_LOG_INFO,    tag, format, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) ESP_LOG_LEVEL(ESP_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) ESP_LOG_LEVEL(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif
