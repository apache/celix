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
 * framework_patch.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#include "framework_patch.h"

#include <stdlib.h>
#include <stdio.h>
/* celix.framework.public */
#include "celix_errno.h"
#include "bundle.h"
#include "bundle_archive.h"
#include "properties.h"
#include "hash_map.h"


celix_status_t bundle_getBundleLocation(bundle_pt bundle, char **location){

	celix_status_t status;

	bundle_archive_pt archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status != CELIX_SUCCESS){
		printf("[ ERROR ]: Bundle - getBundleLocation (BundleArchive) \n");
		return status;
	}

	status =  bundleArchive_getLocation(archive, location);
	if (status != CELIX_SUCCESS){
		printf("[ ERROR ]:  Bundle - getBundleLocation (BundleArchiveLocation) \n");
		return status;
	}

	return CELIX_SUCCESS;
}

