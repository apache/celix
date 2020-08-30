/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _DFI_LOG_UTIL_H_
#define _DFI_LOG_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif


#endif
