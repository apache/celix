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

#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef CELIX_NO_CURLINIT
#include <curl/curl.h>
#endif

#include "celix_constants.h"
#include "celix_framework_factory.h"
#include "celix_framework_utils.h"
#include "celix_err.h"

static void celixLauncher_shutdownFramework(int signal);
static void celixLauncher_ignore(int signal);
static int celixLauncher_launchWithConfigAndProps(const char* configFile,
                                                  celix_framework_t** framework,
                                                  celix_properties_t* packedConfig);
static void celixLauncher_combineProperties(celix_properties_t* original, const celix_properties_t* append);
static celix_properties_t* celixLauncher_createConfig(const char* configFile, celix_properties_t* embeddedProperties);

static void celixLauncher_printUsage(char* progName);
static void celixLauncher_printProperties(celix_properties_t* embeddedProps, const char* configFile);
static int celixLauncher_createBundleCache(celix_properties_t* embeddedProperties, const char* configFile);
static celix_status_t celixLauncher_loadConfigProperties(const char* configFile,
                                                         celix_properties_t** outConfigProperties);

#define DEFAULT_CONFIG_FILE "config.properties"

#define CELIX_LAUNCHER_OK_EXIT_CODE 0
#define CELIX_LAUNCHER_ERROR_EXIT_CODE 1

static framework_t* g_fw = NULL;

int celixLauncher_launchAndWaitForShutdown(int argc, char* argv[], celix_properties_t* packedConfig) {
    celix_framework_t* framework = NULL;
    int rc = celixLauncher_launchWithArgv(argc, argv, packedConfig, &framework);
    if (rc == 0 && framework != NULL) {
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);
    }
    return rc;
}

int celixLauncher_launchWithArgv(int argc,
                                 char* argv[],
                                 celix_properties_t* embeddedConfig,
                                 celix_framework_t** frameworkOut) {
    celix_framework_t* framework = NULL;

    // Perform some minimal command-line option parsing...
    char* opt = NULL;
    char* configFile = NULL;
    bool showProps = false;
    bool createCache = false;
    for (int i = 1; i < argc; ++i) {
        opt = argv[i];
        // Check whether the user wants some help...
        if (strncmp("-?", opt, sizeof("-?")) == 0 || strncmp("-h", opt, sizeof("-h")) == 0 ||
            strncmp("--help", opt, sizeof("--help")) == 0) {
            celixLauncher_printUsage(argv[0]);
            celix_properties_destroy(embeddedConfig);
            return CELIX_LAUNCHER_OK_EXIT_CODE;
        } else if (strncmp("-p", opt, sizeof("-p")) == 0 || strncmp("--props", opt, sizeof("--props")) == 0) {
            showProps = true;
        } else if (strncmp("-c", opt, sizeof("-c")) == 0 ||
                   strncmp("--create-bundle-cache", opt, sizeof("--create-bundle-cache")) == 0) {
            createCache = true;
        } else {
            configFile = opt;
        }
    }

    if (embeddedConfig == NULL) {
        embeddedConfig = celix_properties_create();
    }

    if (showProps) {
        celixLauncher_printProperties(embeddedConfig, configFile);
        celix_properties_destroy(embeddedConfig);
        return CELIX_LAUNCHER_OK_EXIT_CODE;
    }

    if (createCache) {
        return celixLauncher_createBundleCache(embeddedConfig, configFile);
    }

    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = celixLauncher_shutdownFramework;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = celixLauncher_ignore;
    sigaction(SIGUSR1, &sigact, NULL);
    sigaction(SIGUSR2, &sigact, NULL);

    int rc = celixLauncher_launchWithConfigAndProps(configFile, &framework, embeddedConfig);
    if (rc == CELIX_LAUNCHER_OK_EXIT_CODE) {
        g_fw = framework;
        *frameworkOut = framework;
    }
    return rc;
}

static void celixLauncher_shutdownFramework(int signal) {
    if (g_fw != NULL) {
        celixLauncher_stop(g_fw); // NOTE main thread will destroy
    }
}

static void celixLauncher_ignore(int signal) {
    // ignoring for signal SIGUSR1, SIGUSR2. Can be used on threads
}

int celixLauncher_launch(const char* configFile, celix_framework_t** framework) {
    return celixLauncher_launchWithConfigAndProps(configFile, framework, NULL);
}

static int celixLauncher_launchWithConfigAndProps(const char* configFile,
                                                  celix_framework_t** framework,
                                                  celix_properties_t* packedConfig) {
    celix_properties_t* config = celixLauncher_createConfig(configFile, packedConfig);
    if (!config) {
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }
    return celixLauncher_launchWithProperties(config, framework);
}

int celixLauncher_launchWithProperties(celix_properties_t* config, celix_framework_t** framework) {
#ifndef CELIX_NO_CURLINIT
    // Before doing anything else, let's setup Curl
    curl_global_init(CURL_GLOBAL_ALL);
#endif
    *framework = celix_frameworkFactory_createFramework(config);
    return *framework != NULL ? CELIX_LAUNCHER_OK_EXIT_CODE : CELIX_LAUNCHER_ERROR_EXIT_CODE;
}

void celixLauncher_waitForShutdown(celix_framework_t* framework) { celix_framework_waitForStop(framework); }

void celixLauncher_destroy(celix_framework_t* framework) {
    celix_frameworkFactory_destroyFramework(framework);
#ifndef CELIX_NO_CURLINIT
    // Cleanup Curl
    curl_global_cleanup();
#endif
}

void celixLauncher_stop(celix_framework_t* framework) {
    celix_framework_stopBundle(framework, CELIX_FRAMEWORK_BUNDLE_ID);
}

static void celixLauncher_printUsage(char* progName) {
    printf("Usage:\n  %s [-h|-p] [path/to/runtime/config.properties]\n", basename(progName));
    printf("Options:\n");
    printf("\t-h | --help: Show this message.\n");
    printf("\t-p | --props: Show the embedded and runtime properties for this Celix container and exit.\n");
    printf("\t-c | --create-bundle-cache: Create the bundle cache for this Celix container and exit.\n");
    printf("\t--embedded_bundles: Show the embedded bundles for this Celix container and exit.\n");
    printf("\n");
}

static void celixLauncher_printProperties(celix_properties_t* embeddedProps, const char* configFile) {
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

    celix_autoptr(celix_properties_t) runtimeProps;
    (void)celixLauncher_loadConfigProperties(configFile, &runtimeProps);
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

static int celixLauncher_createBundleCache(celix_properties_t* embeddedProperties, const char* configFile) {
    celix_properties_t* config = celixLauncher_createConfig(configFile, embeddedProperties);
    if (!config) {
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }
    celix_framework_t* fw = celix_frameworkFactory_createFramework(config);
    if (!fw) {
        fprintf(stderr, "Failed to create framework for bundle cache creation\n");
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }
    celix_status_t status = celix_framework_utils_createBundleArchivesCache(fw);
    celix_frameworkFactory_destroyFramework(fw);
    if (status != CELIX_SUCCESS) {
        fprintf(stderr, "Failed to create bundle cache\n");
        return CELIX_LAUNCHER_ERROR_EXIT_CODE;
    }
    return CELIX_LAUNCHER_OK_EXIT_CODE;
}

static void celixLauncher_combineProperties(celix_properties_t* original, const celix_properties_t* append) {
    if (original != NULL && append != NULL) {
        CELIX_PROPERTIES_ITERATE(append, visit) { celix_properties_setEntry(original, visit.key, &visit.entry); }
    }
}

static celix_properties_t* celixLauncher_createConfig(const char* configFile, celix_properties_t* embeddedProperties) {
    if (!embeddedProperties) {
        embeddedProperties = celix_properties_create();
    }
    celix_autoptr(celix_properties_t) configProperties;
    celix_status_t status = celixLauncher_loadConfigProperties(configFile, &configProperties);
    if (status != CELIX_SUCCESS) {
        celix_properties_destroy(embeddedProperties);
        return NULL;
    }
    celixLauncher_combineProperties(embeddedProperties, configProperties);
    return embeddedProperties;
}

/**
 * @brief Load the config properties from the given file.
 * If configFile is NULL, config properties will only be loaded if DEFAULT_CONFIG_FILE exists.
 */
static celix_status_t celixLauncher_loadConfigProperties(const char* configFile,
                                                         celix_properties_t** outConfigProperties) {
    celix_properties_t* configProperties = NULL;
    bool loadConfig = configFile != NULL;
    if (!loadConfig) {
        struct stat buffer;
        loadConfig = stat(DEFAULT_CONFIG_FILE, &buffer) == 0;
        configFile = DEFAULT_CONFIG_FILE;
    }
    if (loadConfig) {
        celix_status_t status = celix_properties_load2(configFile, 0, &configProperties);
        if (status != CELIX_SUCCESS) {
            celix_err_printErrors(stderr, "Error loading config file: ", NULL);
            *outConfigProperties = NULL;
            return status;
        }
    }
    *outConfigProperties = configProperties;
    return CELIX_SUCCESS;
}
