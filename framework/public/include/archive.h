/*
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
/**
 *
 * @defgroup Archive Archive
 * @ingroup framework
 * @{
 *
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	May 31, 2010
 *  \copyright	Apache License, Version 2.0
 */
#ifndef ARCHIVE_H_
#define ARCHIVE_H_

#include "celix_errno.h"

/**
 * Extracts the bundle pointed to by bundleName to the given root.
 *
 * @param bundleName location of the bundle to extract.
 * @param revisionRoot directory to where the bundle must be extracted.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_FILE_IO_EXCEPTION If the zip file cannot be extracted.
 */
celix_status_t extractBundle(char * bundleName, char * revisionRoot);

#endif /* ARCHIVE_H_ */

/**
 * @}
 */
