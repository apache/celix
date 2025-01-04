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

#include <string.h>
#include <stdlib.h>

#include "celix_array_list_type.h"
#include "celix_array_list.h"
#include "celix_bundle.h"
#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_framework.h"
#include "celix_shell_constants.h"
#include "celix_utils.h"
#include "std_commands.h"

struct query_options {
    bool useColors;
    bool verbose;
    bool queryProvided;
    bool queryRequested;
    long bndId; // -1L if no bundle is selected
    celix_array_list_t *nameQueries; //entry is char*
    celix_array_list_t *filterQueries; //enry if celix_filter_t
};

struct bundle_callback_data {
    const struct query_options *opts;
    FILE *sout;
    size_t nrOfProvidedServicesFound;
    size_t nrOfRequestedServicesFound;
};

static bool queryCommand_printProvidedService(const struct query_options *opts, celix_bundle_service_list_entry_t *entry) {
    bool print = celix_arrayList_size(opts->nameQueries) == 0 && celix_arrayList_size(opts->filterQueries) == 0; //no queries, always print
    for (int i = 0; i < celix_arrayList_size(opts->nameQueries) && !print; ++i) {
        const char *qry = celix_arrayList_get(opts->nameQueries, i);
        print = strcasestr(entry->serviceName, qry) != NULL;
    }
    for (int i = 0; i < celix_arrayList_size(opts->filterQueries) && !print; ++i) {
        celix_filter_t *filter = celix_arrayList_get(opts->filterQueries, i);
        print = celix_filter_match(filter, entry->serviceProperties);
    }
    return print;
}

static bool queryCommand_printRequestedService(const struct query_options *opts, celix_bundle_service_tracker_list_entry_t *entry) {
    bool print = celix_arrayList_size(opts->nameQueries) == 0 && celix_arrayList_size(opts->filterQueries) == 0; //no queries, always print
    for (int i = 0; i < celix_arrayList_size(opts->nameQueries) && !print; ++i) {
        const char *qry = celix_arrayList_get(opts->nameQueries, i);
        print = strcasestr(entry->serviceName, qry) != NULL;
    }
    for (int i = 0; i < celix_arrayList_size(opts->filterQueries) && !print; ++i) {
        //in case of filter try to match with literal filter string
        celix_filter_t *filter = celix_arrayList_get(opts->filterQueries, i);
        const char *f = celix_filter_getFilterString(filter);
        print = strstr(entry->filter, f) != NULL;
    }
    return print;
}

/**
 * print bundle header (only for first time)
 */
static void queryCommand_printBundleHeader(FILE *sout, const celix_bundle_t *bnd, bool *called) {
    if (called != NULL && !(*called)) {
        fprintf(sout, "Bundle %li [%s]:\n", celix_bundle_getId(bnd), celix_bundle_getSymbolicName(bnd));
        *called = true;
    }
}

static void queryCommand_callback(void *handle, const celix_bundle_t *bnd) {
    struct bundle_callback_data *data = handle;
    bool printBundleCalled = false;
    if (data->opts->queryProvided) {
        celix_array_list_t *services = celix_bundle_listRegisteredServices(bnd);
        for (int i = 0; i < celix_arrayList_size(services); ++i) {
            celix_bundle_service_list_entry_t *entry = celix_arrayList_get(services, i);
            if (queryCommand_printProvidedService(data->opts, entry)) {
                data->nrOfProvidedServicesFound += 1;
                queryCommand_printBundleHeader(data->sout, bnd, &printBundleCalled);
                fprintf(data->sout, "|- Provided service '%s' [id = %li]\n", entry->serviceName, entry->serviceId);
                if (data->opts->verbose) {
                    fprintf(data->sout, "   |- Is factory: %s\n", entry->factory ? "true" : "false");
                    fprintf(data->sout, "   |- Properties:\n");
                    CELIX_PROPERTIES_ITERATE(entry->serviceProperties, iter) {
                        fprintf(data->sout, "      |- %20s = %s\n", iter.key, iter.entry.value);
                    }
                }
            }
        }
        celix_bundle_destroyRegisteredServicesList(services);
    }
    if (data->opts->queryRequested) {
        celix_array_list_t *trackers = celix_bundle_listServiceTrackers(bnd);
        for (int i = 0; i < celix_arrayList_size(trackers); ++i) {
            celix_bundle_service_tracker_list_entry_t *entry = celix_arrayList_get(trackers, i);
            if (queryCommand_printRequestedService(data->opts, entry)) {
                data->nrOfRequestedServicesFound += 1;
                queryCommand_printBundleHeader(data->sout, bnd, &printBundleCalled);
                fprintf(data->sout, "|- Service tracker '%s'\n", entry->filter);
                if (data->opts->verbose) {
                    fprintf(data->sout,"   |- nr of tracked services %lu\n", (long unsigned int) entry->nrOfTrackedServices);
                }
            }
        }
        celix_arrayList_destroy(trackers);
    }

    if (printBundleCalled) {
        fprintf(data->sout, "\n");
    }
}


static void queryCommand_listServicesForBundle(celix_bundle_context_t *ctx, long bndId, struct bundle_callback_data *data, const struct query_options *opts, FILE *sout, FILE *serr) {
    bool called = celix_framework_useBundle(celix_bundleContext_getFramework(ctx), true, bndId, data, queryCommand_callback);
    if (!called) {
        fprintf(serr, "Bundle %li not installed!", bndId);
    }
}

static void queryCommand_listServices(celix_bundle_context_t *ctx, const struct query_options *opts, FILE *sout, FILE *serr) {
    struct bundle_callback_data data;
    data.opts = opts;
    data.sout = sout;
    data.nrOfProvidedServicesFound = 0;
    data.nrOfRequestedServicesFound = 0;

    if (opts->bndId >= CELIX_FRAMEWORK_BUNDLE_ID) {
        queryCommand_listServicesForBundle(ctx, opts->bndId, &data, opts, sout, serr);
    } else {
        //query framework bundle
        queryCommand_listServicesForBundle(ctx, CELIX_FRAMEWORK_BUNDLE_ID, &data, opts, sout, serr);

        //query rest of the bundles
        celix_array_list_t *bundleIds = celix_bundleContext_listBundles(ctx);
        for (int i = 0; i < celix_arrayList_size(bundleIds); ++i) {
            long bndId = celix_arrayList_getLong(bundleIds, i);
            queryCommand_listServicesForBundle(ctx, bndId, &data, opts, sout, serr);
        }
        celix_arrayList_destroy(bundleIds);
    }

    if (data.nrOfRequestedServicesFound == 0 && data.nrOfProvidedServicesFound == 0) {
        fprintf(sout, "No results\n");
    } else {
        fprintf(sout, "Query result:\n");
        fprintf(sout, "|- Provided services found %lu\n", (long unsigned int) data.nrOfProvidedServicesFound);
        fprintf(sout, "|- Requested services found %lu\n", (long unsigned int) data.nrOfRequestedServicesFound);
        fprintf(sout, "\n");
    }
}


bool queryCommand_execute(void *_ptr, const char *command_line_str, FILE *sout, FILE *serr) {
    bundle_context_t* ctx = _ptr;

    char *commandLine = celix_utils_strdup(command_line_str); //note command_line_str should be treated as const.

    struct query_options opts;
    memset(&opts, 0, sizeof(opts));
    opts.bndId = -1L;
    opts.queryProvided = true;
    opts.queryRequested = true;

    opts.nameQueries = celix_arrayList_createPointerArray();
    opts.filterQueries = celix_arrayList_createPointerArray();
    opts.useColors = celix_bundleContext_getPropertyAsBool(ctx, CELIX_SHELL_USE_ANSI_COLORS, CELIX_SHELL_USE_ANSI_COLORS_DEFAULT_VALUE);

    bool validCommand = true;
    char *sub_str = NULL;
    char *save_ptr = NULL;

    strtok_r(commandLine, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    sub_str = strtok_r(NULL, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    while (sub_str != NULL) {
        if (strcmp(sub_str, "-v") == 0) {
            opts.verbose = true;
        } else if (strcmp(sub_str, "-p") == 0) {
            opts.queryProvided = true;
            opts.queryRequested = false;
        } else if (strcmp(sub_str, "-r") == 0) {
            opts.queryProvided = false;
            opts.queryRequested = true;
        } else {
            //check if its a number (bundle id)
            errno = 0;
            long bndId = strtol(sub_str, NULL, 10);
            if (bndId >= CELIX_FRAMEWORK_BUNDLE_ID && errno == 0 /*not EINVAL*/) {
                opts.bndId = bndId;
            } else {
                //not option and not a bundle id -> query
                if (strnlen(sub_str, 16) > 1 && sub_str[0] == '(') {
                    //assume this is a filter.
                    celix_filter_t *filter = celix_filter_create(sub_str);
                    if (filter != NULL) {
                        celix_arrayList_add(opts.filterQueries, filter);
                    } else {
                        validCommand = false;
                        fprintf(serr, "Cannot parse provided filter '%s'!\n", sub_str);
                        break;
                    }
                } else {
                    celix_arrayList_add(opts.nameQueries, celix_utils_strdup(sub_str));
                }

            }
        }
        sub_str = strtok_r(NULL, CELIX_SHELL_COMMAND_SEPARATOR, &save_ptr);
    }

    free(commandLine);

    if (validCommand) {
        queryCommand_listServices(ctx, &opts, sout, serr);
    }

    for (int i = 0; i < celix_arrayList_size(opts.nameQueries); ++i) {
        char *name = celix_arrayList_get(opts.nameQueries, i);
        free(name);
    }
    celix_arrayList_destroy(opts.nameQueries);

    for (int i = 0; i < celix_arrayList_size(opts.filterQueries); ++i) {
        celix_filter_t *filter = celix_arrayList_get(opts.filterQueries, i);
        celix_filter_destroy(filter);
    }
    celix_arrayList_destroy(opts.filterQueries);

    return validCommand;
}
