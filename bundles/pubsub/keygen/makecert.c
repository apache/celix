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
/**
 * makecert.c
 *
 *  \date       Dec 2, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <string.h>

#include "czmq.h"

int main (int argc, const char * argv[]) {

    const char * cert_name_public = "certificate.pub";
    const char * cert_name_secret = "certificate.key";
    if (argc == 3 && strcmp(argv[1], argv[2]) != 0) {
        cert_name_public = argv[1];
        cert_name_secret = argv[2];
    }

    zcert_t * cert = zcert_new();

    char *timestr = zclock_timestr ();
    zcert_set_meta (cert, "date-created", timestr);
    free (timestr);

    zcert_save_public(cert, cert_name_public);
    zcert_save_secret(cert, cert_name_secret);
    zcert_print (cert);
    printf("\n");
    printf("I: CURVE certificate created in %s and %s\n", cert_name_public, cert_name_secret);
    zcert_destroy (&cert);

    return 0;
}
