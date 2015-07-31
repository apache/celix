/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef _DFI_LOG_UTIL_H_
#define _DFI_LOG_UTIL_H_

typedef void (*logf_ft)(void *handle, int level, const char *file, int line, const char *format, ...); 

#define DFI_SETUP_LOG_HEADER(cmp) \
    void cmp ## _logSetup(logf_ft logf, void *handle, int currentLogLevel);

#define DFI_SETUP_LOG(cmp) \
    static logf_ft g_logf = NULL; \
    static void *g_logHandle = NULL; \
    static int g_currentLogLevel = 1; \
    \
    void cmp ## _logSetup(logf_ft logf, void *handle, int currentLogLevel) { \
        g_currentLogLevel = currentLogLevel; \
        g_logHandle = handle; \
        g_logf = logf; \
    }  

#define LOG_LVL_ERROR    1
#define LOG_LVL_WARNING  2
#define LOG_LVL_INFO     3
#define LOG_LVL_DEBUG    4

#define LOG_ERROR(msg, ...) \
    if (g_logf != NULL && g_currentLogLevel >= LOG_LVL_ERROR) { \
        g_logf(g_logHandle, LOG_LVL_ERROR, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
    } 

#define LOG_WARNING(msg, ...) \
    if (g_logf != NULL && g_currentLogLevel >= LOG_LVL_WARNING) { \
        g_logf(g_logHandle, LOG_LVL_WARNING, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
    } 

#define LOG_INFO(msg, ...) \
    if (g_logf != NULL && g_currentLogLevel >= LOG_LVL_INFO) { \
        g_logf(g_logHandle, LOG_LVL_INFO, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
    } 

#define LOG_DEBUG(msg, ...) \
    if (g_logf != NULL && g_currentLogLevel >= LOG_LVL_DEBUG) { \
        g_logf(g_logHandle, LOG_LVL_DEBUG, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
    } 

#endif
