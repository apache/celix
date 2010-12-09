/*
 * publisher.h
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */

#ifndef PUBLISHER_H_
#define PUBLISHER_H_

#define PUBLISHER_NAME "Publisher"

typedef struct publisher * PUBLISHER;

struct publisherService {
	PUBLISHER publisher;
	void (*invoke)(PUBLISHER pub, char * text);
};

typedef struct publisherService * PUBLISHER_SERVICE;


#endif /* PUBLISHER_H_ */
