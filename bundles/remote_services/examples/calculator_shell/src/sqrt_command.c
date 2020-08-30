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


#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "celix_api.h"
#include "celix_utils.h"
#include "sqrt_command.h"
#include "calculator_service.h"

static celix_status_t sqrtCommand_isNumeric(char *number, bool *ret);

struct calc_callback_data {
    double a;
    double result;
    int rc;
};

static void calcCallback(void *handle, void *svc) {
    struct calc_callback_data *data = handle;
    calculator_service_t *calc = svc;
    data->rc = calc->sqrt(calc->handle, data->a, &data->result);
}

bool sqrtCommand_execute(void *handle, const char *const_line, FILE *out, FILE *err) {
    celix_bundle_context_t *context = handle;
    bool ok = true;
    char *line = celix_utils_strdup(const_line);


    char *token = line;
    strtok_r(line, " ", &token);
    char *aStr = strtok_r(NULL, " ", &token);
    if(aStr != NULL) {
        bool numeric;
        sqrtCommand_isNumeric(aStr, &numeric);
        if (numeric) {
            struct calc_callback_data data;

            data.a = atof(aStr);
            data.result = 0;
            data.rc = 0;
            bool called = celix_bundleContext_useService(context, CALCULATOR_SERVICE, &data, calcCallback);
            if (called && data.rc == 0) {
                fprintf(out, "CALCULATOR_SHELL: Sqrt: %f = %f\n", data.a, data.result);
            } else if (!called) {
                fprintf(err, "ADD: calculator service not available\n");
                ok = false;
            } else {
                fprintf(err, "ADD: Unexpected exception in Calc service\n");
                ok = false;
            }
        } else {
            fprintf(err, "SQRT: Requires 1 numerical parameter\n");
            ok = false;
        }
    } else {
        fprintf(err, "SQRT: Requires 1 numerical parameter\n");
        ok = false;
    }

    free(line);

    return ok;
}

static celix_status_t sqrtCommand_isNumeric(char *number, bool *ret) {
    celix_status_t status = CELIX_SUCCESS;
    *ret = true;
    while(*number) {
        if(!isdigit(*number) && *number != '.') {
            *ret = false;
            break;
        }
        number++;
    }
    return status;
}
