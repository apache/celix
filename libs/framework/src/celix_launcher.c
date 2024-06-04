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

#include "celix_launcher.h"
#include "celix_launcher_private.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#ifndef CELIX_NO_CURLINIT
#include <curl/curl.h>
#endif

#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_err.h"
#include "celix_file_utils.h"
#include "celix_framework_factory.h"
#include "celix_framework_utils.h"

#define DEFAULT_CONFIG_FILE "config.properties"

#define CELIX_LAUNCHER_OK_EXIT_CODE 0
#define CELIX_LAUNCHER_ERROR_EXIT_CODE 1

static framework_t* g_fw = NULL;

typedef struct {
    bool showHelp;
    bool showProps;
    bool createCache;
    const char* configFile;
} celix_launcher_options_t;

/**
 * @brief SIGUSR1 SIGUSR2 no-op callback handler.
 */
static void celix_launcher_noopSignalHandler(int signal);

/**
 * @brief SIGINT SIGTERM callback to shutdown the framework.
 */
static void celix_launcher_shutdownFrameworkSignalHandler(int signal);

/**
 * @brief Create a Celix framework instance with the given embedded and runtime properties.
 *
 * Also:
 *  - Set up signal handlers for SIGINT and SIGTERM to stop the framework.
 *  - Set up signal handlers for SIGUSR1 and SIGUSR2 to ignore the signals.
 *  - If `CELIX_NO_CURLINIT` was not defined during compilation, initialize Curl.
 */
static celix_status_t celix_launcher_createFramework(celix_properties_t* embeddedProps,
                                          const celix_properties_t* runtimeProps,
                                          celix_framework_t** frameworkOut);

/**
 * @brief Stop the global framework instance (if set).
 */
static celix_status_t celix_launcher_parseOptions(int argc, char* argv[], celix_launcher_options_t* opts);

/**
 * @brief Print the usage of the Celix launcher command arguments.
 */
static void celix_launcher_printUsage(char* progName);

/**
 * @brief Print the embedded, runtime and combined properties.
 */
static void celix_launcher_printProperties(const celix_properties_t* embeddedProps,
                                           const celix_properties_t* runtimeProps,
                                           const char* configFile);


/**
 * @brief Create the bundle cache using the given embedded and runtime properties.
 *
 * Will create a framework instance to create the bundle cache and destroy the framework instance afterwards.
 */
static celix_status_t celix_launcher_createBundleCache(celix_properties_t* embeddedProps, const celix_properties_t* runtimeProps);

/**
 * @brief Load the runtime properties from the given file.
 *
 * If no file is given, a check is done for the default file and if it exists, the default file is used.
 */
static celix_status_t celix_launcher_loadRuntimeProperties(const char* configFile, celix_properties_t** outConfigProperties);

/**
 * @brief Set the global framework instance.
 *
 * If a global framework instance is already set, the new framework instance will be destroyed and a
 * error code will be returned.
 */
static celix_status_t celix_launcher_setGlobalFramework(celix_framework_t* fw);

int celix_launcher_launchAndWait(int argc, char* argv[], const char* embeddedConfig) {
    celix_autoptr(celix_properties_t) embeddedProps = NULL;
    if (embeddedConfig) {
        celix_status_t status = celix_properties_loadFromString(embeddedConfig, 0, &embeddedProps);
        if (status != CELIX_SUCCESS) {
            celix_err_printErrors(stderr, "Error creating embedded properties: ", NULL);
            return CELIX_LAUNCHER_ERROR_EXIT_CODE;
        }
    } else {
        embeddedProps = celix_properties_create();
    }

    celix_launcher_options_t options;
    memset(&options, 0, sizeof(options));
    celix_status_t status = celix_launcher_parseOptions(argc, argv, &options);
    if (status != CELIX_SUCCESS) {
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }

    celix_autoptr(celix_properties_t) runtimeProps = NULL;
    status = celix_launcher_loadRuntimeProperties(options.configFile, &runtimeProps);
    if (status != CELIX_SUCCESS) {
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }

    if (options.showHelp) {
        celix_launcher_printUsage(argv[0]);
        return CELIX_LAUNCHER_OK_EXIT_CODE;
    } else if (options.showProps) {
        celix_launcher_printProperties(embeddedProps, runtimeProps, options.configFile);
        return CELIX_LAUNCHER_OK_EXIT_CODE;
    } else if (options.createCache) {
        status = celix_launcher_createBundleCache(celix_steal_ptr(embeddedProps), runtimeProps);
        return status == CELIX_SUCCESS ? CELIX_LAUNCHER_OK_EXIT_CODE : CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }

    celix_framework_t* framework = NULL;
    status = celix_launcher_createFramework(celix_steal_ptr(embeddedProps), runtimeProps, &framework);
    if (status == CELIX_SUCCESS && framework) {
        status = celix_launcher_setGlobalFramework(framework);
        if (status != CELIX_SUCCESS) {
            return CELIX_LAUNCHER_ERROR_EXIT_CODE;
        }
        celix_framework_waitForStop(framework);
        celix_frameworkFactory_destroyFramework(framework);
#ifndef CELIX_NO_CURLINIT
        // Cleanup Curl
        curl_global_cleanup();
#endif
    }
    return status == CELIX_SUCCESS ? CELIX_LAUNCHER_OK_EXIT_CODE : CELIX_LAUNCHER_ERROR_EXIT_CODE;
}

static celix_status_t celix_launcher_setGlobalFramework(celix_framework_t* fw) {
    //try to set the global framework instance, but fail if global instance is already set.
    celix_framework_t* expected = NULL;
    bool swapped = __atomic_compare_exchange_n(&g_fw, &expected, fw, false,  __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);
    if (!swapped) {
        celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(fw);
        celix_bundleContext_log(
            ctx, CELIX_LOG_LEVEL_ERROR, "Failed to set global framework instance, already set. Destroying framework");
        celix_frameworkFactory_destroyFramework(fw);
        return CELIX_ILLEGAL_STATE;
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_launcher_parseOptions(int argc, char* argv[], celix_launcher_options_t* opts) {
    // Perform some minimal command-line option parsing...
    int optCount = 0;
    for (int i = 1; i < argc; ++i) {
        const char* opt = argv[i];
        // Check whether the user wants some help...
        if (strncmp("-?", opt, sizeof("-?")) == 0 || strncmp("-h", opt, sizeof("-h")) == 0 ||
            strncmp("--help", opt, sizeof("--help")) == 0) {
            opts->showHelp = true;
            optCount++;
        } else if (strncmp("-p", opt, sizeof("-p")) == 0 || strncmp("--props", opt, sizeof("--props")) == 0) {
            opts->showProps = true;
            optCount++;
        } else if (strncmp("-c", opt, sizeof("-c")) == 0 ||
                   strncmp("--create-bundle-cache", opt, sizeof("--create-bundle-cache")) == 0) {
            opts->createCache = true;
            optCount++;
        } else {
            if (opts->configFile) {
                fprintf(stderr, "Error: multiple configuration files specified\n");
                return CELIX_ILLEGAL_ARGUMENT;
            }
            opts->configFile = opt;
        }
    }

    //check if not multiple options are set
    if (optCount > 1) {
        fprintf(stderr, "Error: multiple options specified\n");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    return CELIX_SUCCESS;
}

static celix_status_t celix_launcher_createFramework(celix_properties_t* embeddedProps,
                                          const celix_properties_t* runtimeProps,
                                          celix_framework_t** frameworkOut) {
    celix_status_t status = celix_launcher_combineProperties(embeddedProps, runtimeProps);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = celix_launcher_shutdownFrameworkSignalHandler;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = celix_launcher_noopSignalHandler;
    sigaction(SIGUSR1, &sigact, NULL);
    sigaction(SIGUSR2, &sigact, NULL);

#ifndef CELIX_NO_CURLINIT
    // Before doing anything else, let's setup Curl
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    *frameworkOut = celix_frameworkFactory_createFramework(embeddedProps);
    return *frameworkOut != NULL ? CELIX_SUCCESS : CELIX_FRAMEWORK_EXCEPTION;
}

/**
 * @brief SIGUSR1 SIGUSR2 no-op callback
 */
static void celix_launcher_noopSignalHandler(int signal __attribute__((unused))) {
    // ignoring signal SIGUSR1, SIGUSR2. Can be used on threads
}

static void celix_launcher_printUsage(char* progName) {
    printf("Usage:\n  %s [-h|-p] [path/to/runtime/config.properties]\n", basename(progName));
    printf("Options:\n");
    printf("\t-h | --help: Show this message.\n");
    printf("\t-p | --props: Show the embedded and runtime properties for this Celix container and exit.\n");
    printf("\t-c | --create-bundle-cache: Create the bundle cache for this Celix container and exit.\n");
    printf("\n");
}

static void celix_launcher_printProperties(const celix_properties_t* embeddedProps,
                                           const celix_properties_t* runtimeProps,
                                           const char* configFile) {
    celix_autoptr(celix_properties_t) keys = celix_properties_create(); // only to store the keys

    printf("Embedded properties:\n");
    if (embeddedProps == NULL || celix_properties_size(embeddedProps) == 0) {
        printf("|- Empty!\n");
    } else {
        CELIX_PROPERTIES_ITERATE(embeddedProps, visit) {
            printf("|- %s=%s\n", visit.key, visit.entry.value);
            celix_properties_set(keys, visit.key, NULL);
        }
    }
    printf("\n");


    printf("Runtime properties (input %s):\n", configFile ? configFile : "none");
    if (runtimeProps == NULL || celix_properties_size(runtimeProps) == 0) {
        printf("|- Empty!\n");
    } else {
        CELIX_PROPERTIES_ITERATE(runtimeProps, visit) {
            printf("|- %s=%s\n", visit.key, visit.entry.value);
            celix_properties_set(keys, visit.key, NULL);
        }
    }
    printf("\n");

    // combined result
    printf("Resolved (env, runtime and embedded) properties:\n");
    if (celix_properties_size(keys) == 0) {
        printf("|- Empty!\n");
    } else {
        CELIX_PROPERTIES_ITERATE(keys, visit) {
            const char* valEm = celix_properties_get(embeddedProps, visit.key, NULL);
            const char* valRt = celix_properties_get(runtimeProps, visit.key, NULL);
            const char* envVal = getenv(visit.key);
            const char* val = envVal != NULL ? envVal : valRt != NULL ? valRt : valEm;
            const char* source = envVal != NULL ? "environment" : valRt != NULL ? "runtime" : "embedded";
            printf("|- %s=%s (source %s)\n", visit.key, val, source);
        }
    }
    printf("\n");
}

static celix_status_t celix_launcher_createBundleCache(celix_properties_t* embeddedProps,
                                                       const celix_properties_t* runtimeProps) {
    celix_status_t status = celix_launcher_combineProperties(embeddedProps, runtimeProps);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    celix_framework_t* fw = celix_frameworkFactory_createFramework(embeddedProps);
    if (!fw) {
        fprintf(stderr, "Failed to create framework for bundle cache creation\n");
        return CELIX_FRAMEWORK_EXCEPTION;
    }

    status = celix_framework_utils_createBundleArchivesCache(fw);
    if (status != CELIX_SUCCESS) {
        celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(fw);
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Failed to create bundle cache");
        return status;
    }
    celix_frameworkFactory_destroyFramework(fw);
    return CELIX_SUCCESS;
}

celix_status_t celix_launcher_combineProperties(celix_properties_t* embeddedProps,
                                                       const celix_properties_t* runtimeProps) {
    if (embeddedProps && runtimeProps) {
        CELIX_PROPERTIES_ITERATE(runtimeProps, visit) {
            celix_status_t status = celix_properties_setEntry(embeddedProps, visit.key, &visit.entry);
            if (status != CELIX_SUCCESS) {
                celix_properties_destroy(embeddedProps);
                return status;
            }
        }
    }
    return CELIX_SUCCESS;
}

/**
 * @brief Load the config properties from the given file.
 * If configFile is NULL, config properties will only be loaded if DEFAULT_CONFIG_FILE exists.
 */
static celix_status_t celix_launcher_loadRuntimeProperties(const char* configFile, celix_properties_t** outConfigProperties) {
    *outConfigProperties = NULL;
    bool loadConfig = configFile != NULL;
    if (!loadConfig) {
        loadConfig = celix_utils_fileExists(DEFAULT_CONFIG_FILE);
        configFile = DEFAULT_CONFIG_FILE;
    }
    if (loadConfig) {
        celix_status_t status = celix_properties_load(configFile, 0, outConfigProperties);
        if (status != CELIX_SUCCESS) {
            celix_err_printErrors(stderr, "Error creating runtime properties: ", NULL);
            return status;
        }
    }
    return CELIX_SUCCESS;
}

static void celix_launcher_shutdownFrameworkSignalHandler(int signal) {
    celix_launcher_stopInternal(&signal);
}

void celix_launcher_stopInternal(const int* signal) {
    celix_framework_t* fw = __atomic_exchange_n(&g_fw, NULL, __ATOMIC_SEQ_CST);
    if (fw) {
        if (signal) {
            celix_bundle_context_t* ctx = celix_framework_getFrameworkContext(fw);
            celix_bundleContext_log(
                ctx, CELIX_LOG_LEVEL_INFO, "Stopping Celix framework due to signal %s", strsignal(*signal));
        }
        celix_framework_stopBundle(fw, CELIX_FRAMEWORK_BUNDLE_ID);
    } else {
        fprintf(stderr, "No global framework instance to stop\n");
    }
}

void celix_launcher_triggerStop() {
    celix_launcher_stopInternal(NULL);
}
