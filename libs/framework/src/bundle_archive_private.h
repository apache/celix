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


#ifndef BUNDLE_ARCHIVE_PRIVATE_H_
#define BUNDLE_ARCHIVE_PRIVATE_H_

#include "bundle_archive.h"

celix_status_t bundleArchive_create(celix_framework_t* fw, const char *archiveRoot, long id, const char *location, const char *inputFile,
                                    bundle_archive_pt *bundle_archive);

celix_status_t bundleArchive_createSystemBundleArchive(celix_framework_t* fw, bundle_archive_pt *bundle_archive);

celix_status_t bundleArchive_recreate(celix_framework_t* fw, const char *archiveRoot, bundle_archive_pt *bundle_archive);

celix_status_t bundleArchive_destroy(bundle_archive_pt archive);

#endif /* BUNDLE_ARCHIVE_PRIVATE_H_ */
