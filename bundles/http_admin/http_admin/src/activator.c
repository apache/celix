/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * activator.c
 *
 *  \date       May 24, 2019
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

#include "celix_api.h"
#include "celix_types.h"

#include "http_admin.h"
#include "websocket_admin.h"

#include "http_admin/api.h"
#include "civetweb.h"


typedef struct http_admin_activator {
    http_admin_manager_t *httpManager;
    websocket_admin_manager_t *sockManager;

    long httpAdminSvcId;
    long sockAdminSvcId;

    celix_array_list_t *aliasList;      //Array list of http_alias_t
    bool useWebsockets;
    char *root;

    long bundleTrackerId;
} http_admin_activator_t;

typedef struct http_alias {
    char *alias_path;
    long bundle_id;
} http_alias_t;

//Local function prototypes
void createAliasesSymlink(const char *aliases, const char *admin_root, const char *bundle_root, long bundle_id, celix_array_list_t *alias_list);
bool aliasList_containsAlias(celix_array_list_t *alias_list, const char *alias);

static void http_admin_startBundle(void *data, const celix_bundle_t *bundle);
static void http_admin_stopBundle(void *data, const celix_bundle_t *bundle);

/**
 * Decode aliases from the given alias string and make a symbolic link for proper resource behavior.
 * Save the created aliases as pair with the corresponding bundle id in the given hash map.
 *
 * @param aliases                      String which contains the aliases to append to the alias map
 * @param admin_root                   Root path of bundle where the admin runs
 * @param bundle_root                  Bundle path of bundle where the resources remain
 * @param bundle_id                    Bundle ID to connect aliases to this bundle
 * @param alias_map                    Pointer to the alias map to which the created symlink is saved with the bundle id as key
 */
void createAliasesSymlink(const char *aliases, const char *admin_root, const char *bundle_root, long bundle_id, celix_array_list_t *alias_list) {
    char *token = NULL;
    char *sub_token = NULL;
    char *save_ptr = NULL;
    char *sub_save_ptr = NULL;
    char *alias_cpy;

    if(aliases != NULL && alias_list != NULL && admin_root != NULL && bundle_root != NULL) {
        token = alias_cpy = strdup(aliases);
        token = strtok_r(token, ",", &save_ptr);

        while(token != NULL) {
            int i;
            char *alias_path;
            char *bnd_resource_path;
            char *cwd = get_current_dir_name();

            while(isspace(*token)) token++; //skip spaces at beginning
            for (i = (int)(strlen(token) - 1); (isspace(token[i])); i--); //skip spaces at end
            token[i+1] = '\0';

            sub_token = strtok_r(token, ";", &sub_save_ptr);
            if(sub_token == NULL) {
                token = strtok_r(NULL, ",", &save_ptr);
                continue; //Invalid alias to path, continue to next entry
            }

            asprintf(&alias_path, "%s/%s%s", cwd, admin_root, sub_token);
            if(aliasList_containsAlias(alias_list, alias_path)) {
                free(alias_path);
                continue; //Alias already existing, Continue to next entry
            }

            sub_token = strtok_r(NULL, ";", &sub_save_ptr);
            if(sub_token == NULL) {
                free(alias_path);
                token = strtok_r(NULL, ",", &save_ptr);
                continue; //Invalid alias to path, continue to next entry
            }

            asprintf(&bnd_resource_path, "%s/%s/%s", cwd, bundle_root, sub_token);

            if(symlink(bnd_resource_path, alias_path) == 0) {
                http_alias_t *alias = calloc(1, sizeof(*alias));
                alias->alias_path = alias_path;
                alias->bundle_id = bundle_id;
                celix_arrayList_add(alias_list, alias);
            } else {
                free(alias_path);
            }

            free(bnd_resource_path);
            free(cwd);
            token = strtok_r(NULL, ",", &save_ptr);
        }
        free(alias_cpy);
    }
}

/**
 * Check if the given alias is already part of the given alias list
 *
 * @param alias_list                   The list to search for aliases
 * @param alias                        The alias to search for
 * @return true if alias is already in the list, false if not.
 */
bool aliasList_containsAlias(celix_array_list_t *alias_list, const char *alias) {
    unsigned int size = arrayList_size(alias_list);
    for(unsigned int i = 0; i < size; i++) {
        http_alias_t *http_alias = arrayList_get(alias_list, i);
        if(strcmp(http_alias->alias_path, alias) == 0) {
            return true;
        }
    }

    return false;
}

static void http_admin_startBundle(void *data, const celix_bundle_t *bundle) {
    bundle_archive_pt archive = NULL;
    bundle_revision_pt revision = NULL;
    manifest_pt manifest = NULL;

    http_admin_activator_t *act = data;

    // Retrieve manifest from current bundle revision (retrieve revision from bundle archive) to check for
    // Amdatu pattern manifest property X-Web-Resource. This property is used for aliases.
    celix_status_t status = bundle_getArchive((celix_bundle_t*)bundle, &archive);
    if(status == CELIX_SUCCESS && archive != NULL) {
        status = bundleArchive_getCurrentRevision(archive, &revision);
    }

    if(status == CELIX_SUCCESS && revision != NULL) {
        status = bundleRevision_getManifest(revision, &manifest);
    }

    if(status == CELIX_SUCCESS && manifest != NULL) {
        const char *aliases = NULL;
        const char *revision_root = NULL;
        long bnd_id;

        aliases = manifest_getValue(manifest, "X-Web-Resource");
        bnd_id = celix_bundle_getId(bundle);
        bundleRevision_getRoot(revision, &revision_root);
        createAliasesSymlink(aliases, act->root, revision_root, bnd_id, act->aliasList);
    }
}

static void http_admin_stopBundle(void *data, const celix_bundle_t *bundle) {
    http_admin_activator_t *act = data;
    long bundle_id = celix_bundle_getId(bundle);

    //Remove all aliases which are connected to this bundle
    unsigned int size = arrayList_size(act->aliasList);
    for(unsigned int i = (size - 1); (i >= 0 && i < size); i--) {
        http_alias_t *alias = arrayList_get(act->aliasList, i);
        if(alias->bundle_id == bundle_id) {
            remove(alias->alias_path); //Delete alias in cache directory
            free(alias->alias_path);
            free(alias);
            celix_arrayList_removeAt(act->aliasList, i);
        }
    }
}

static int http_admin_start(http_admin_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundle_t *bundle = celix_bundleContext_getBundle(ctx);
    act->root = celix_bundle_getEntry(bundle, "root");
    act->aliasList = celix_arrayList_create();

    {
        celix_bundle_tracking_options_t opts = CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS;
        opts.callbackHandle = act;
        opts.onStarted = http_admin_startBundle;
        opts.onStopped = http_admin_stopBundle;
        act->bundleTrackerId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
    }

    const char *prop_doc_root = act->root;
    bool prop_use_websockets = celix_bundleContext_getPropertyAsBool(ctx, "USE_WEBSOCKETS", false);
    const char *prop_port = celix_bundleContext_getProperty(ctx, "LISTENING_PORTS", "8080");
    const char *prop_timeout = celix_bundleContext_getProperty(ctx, "WEBSOCKET_TIMEOUT_MS", "3600000");
    long prop_port_min = celix_bundleContext_getPropertyAsLong(ctx, "PORT_RANGE_MIN", 8000);
    long prop_port_max = celix_bundleContext_getPropertyAsLong(ctx, "PORT_RANGE_MAX", 9000);

    act->useWebsockets = prop_use_websockets;

    const char *svr_opts[] = {
            "document_root", prop_doc_root,
            "listening_ports", prop_port,
            "websocket_timeout_ms", prop_timeout,
            "websocket_root", prop_doc_root,
            NULL
    };

    //Try the 'LISTENING_PORT' property first, if failing continue with the port range functionality
    act->httpManager = httpAdmin_create(ctx, svr_opts);

    for(long port = prop_port_min; act->httpManager == NULL && port <= prop_port_max; port++) {
        char *port_str;
        asprintf(&port_str, "%li", port);
        svr_opts[3] = port_str;
        act->httpManager = httpAdmin_create(ctx, svr_opts);
        free(port_str);
    }

    if (act->httpManager != NULL) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = act->httpManager;
        opts.addWithProperties = http_admin_addHttpService;
        opts.removeWithProperties = http_admin_removeHttpService;
        opts.filter.serviceName = HTTP_ADMIN_SERVICE_NAME;
        act->httpAdminSvcId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

        //Retrieve some data from the http admin and reuse it for the websocket admin
        struct mg_context *svr_ctx = httpAdmin_getServerContext(act->httpManager);

        //Websockets are dependent from the http admin, which starts the server.
        if(act->useWebsockets) {
            act->sockManager = websocketAdmin_create(ctx, svr_ctx);

            if (act->sockManager != NULL) {
                celix_service_tracking_options_t opts2 = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
                opts2.callbackHandle = act->sockManager;
                opts2.addWithProperties = websocket_admin_addWebsocketService;
                opts2.removeWithProperties = websocket_admin_removeWebsocketService;
                opts2.filter.serviceName = WEBSOCKET_ADMIN_SERVICE_NAME;
                act->sockAdminSvcId = celix_bundleContext_trackServicesWithOptions(ctx, &opts2);
            }
        }
    }

    return CELIX_SUCCESS;
}

static int http_admin_stop(http_admin_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(ctx, act->httpAdminSvcId);
    celix_bundleContext_stopTracker(ctx, act->sockAdminSvcId);
    celix_bundleContext_stopTracker(ctx, act->bundleTrackerId);
    httpAdmin_destroy(act->httpManager);

    if(act->useWebsockets && act->sockManager != NULL) {
        websocketAdmin_destroy(act->sockManager);
        act->useWebsockets = false;
    }

    //Destroy alias map by removing symbolic links first.
    unsigned int size = arrayList_size(act->aliasList);
    for(unsigned int i = (size - 1); (i >= 0 && i < size); i--) {
        http_alias_t *alias = arrayList_get(act->aliasList, i);
        remove(alias->alias_path); //Delete alias in cache directory
        free(alias->alias_path);
        free(alias);
        celix_arrayList_removeAt(act->aliasList, i);
    }
    arrayList_destroy(act->aliasList);

    free(act->root);

    return CELIX_SUCCESS;
}


CELIX_GEN_BUNDLE_ACTIVATOR(http_admin_activator_t, http_admin_start, http_admin_stop);