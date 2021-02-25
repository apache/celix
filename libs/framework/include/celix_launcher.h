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


#ifndef CELIX_LAUNCHER_H
#define CELIX_LAUNCHER_H

#include <stdio.h>
#include "celix_framework.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Launched a celix framework and wait (block) till the framework is stopped.
 * The function will also destroy the framework when it has been stopped.
 *
 * The launcher will also setup signal handlers for SIGINT, SIGTERM, SIGUSR1 and SIGUSR2 and initialize libcurl.
 * For SIGINT and SIGTERM the framework will be stopped
 * SIGUSR1 and SIGUSR2 will be ignored.
 *
 *
 * @param argc argc as provided in a main function.
 * @param argv argv as provided in a main function.
 * @param embeddedConfig The optional embedded config, will be overridden with the config.properties if found.
 * @return CELIX_SUCCESS if successful.
 */
int celixLauncher_launchAndWaitForShutdown(int argc, char *argv[], celix_properties_t *embeddedConfig);


/**
 * Launches the a celix framework and returns the framework.
 *
 * The launcher will also setup signal handlers for SIGINT, SIGTERM, SIGUSR1 and SIGUSR2 and initialize libcurl.
 * For SIGINT and SIGTERM the framework will be stopped
 * SIGUSR1 and SIGUSR2 will be ignored.
 *
 * @param configFile Path to the config file (config.properties)
 * @param framework Output parameter for the framework.
 * @return CELIX_SUCCESS if successful. 
 */
int celixLauncher_launch(const char *configFile, celix_framework_t **framework);

/**
 * Launches the a celix framework and returns the framework.
 *
 * The launcher will also setup signal handlers for SIGINT, SIGTERM, SIGUSR1 and SIGUSR2 and initialize libcurl.
 * For SIGINT and SIGTERM the framework will be stopped
 * SIGUSR1 and SIGUSR2 will be ignored.
 *
 * @param config FILE* to the config file (config.properties)
 * @param framework Output parameter for the framework.
 * @return CELIX_SUCCESS if successful.
 */
int celixLauncher_launchWithStream(FILE *config, celix_framework_t **framework);

/**
 * Launches the a celix framework and returns the framework.
 *
 * The launcher will also setup signal handlers for SIGINT, SIGTERM, SIGUSR1 and SIGUSR2 and initialize libcurl.
 * For SIGINT and SIGTERM the framework will be stopped
 * SIGUSR1 and SIGUSR2 will be ignored.
 *
 * @param config the config properties.
 * @param framework Output parameter for the framework.
 * @return CELIX_SUCCESS if successful.
 */
int celixLauncher_launchWithProperties(celix_properties_t *config, celix_framework_t **framework);

/**
 * Wait (blocks) for the shutdown of the provided celix framework.
 * @param framework The framework to wait for.
 */
void celixLauncher_waitForShutdown(celix_framework_t *framework);

/**
 * Stop the provided celix framework.
 * @param framework The framework to stop.
 */
void celixLauncher_stop(celix_framework_t *framework);

/**
 * Destroys the provided framework and if needed stops it first.
 * @param framework The framework to stop.
 */
void celixLauncher_destroy(celix_framework_t *framework);

#ifdef __cplusplus
}
#endif

#endif //CELIX_LAUNCHER_H
