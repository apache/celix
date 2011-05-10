/*
 * inputstream.h
 *
 *  Created on: Apr 13, 2011
 *      Author: alexanderb
 */

#ifndef INPUTSTREAM_H_
#define INPUTSTREAM_H_

#include "celix_errno.h"

struct inputStream {
	celix_status_t (*inputStream_read)(FILE *fp);
};


#endif /* INPUTSTREAM_H_ */
