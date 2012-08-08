/*
 * miniunz.h
 *
 *  Created on: Aug 8, 2012
 *      Author: alexander
 */

#ifndef MINIUNZ_H_
#define MINIUNZ_H_

#include "celix_errno.h"

celix_status_t unzip_extractDeploymentPackage(char * packageName, char * destination);

#endif /* MINIUNZ_H_ */
