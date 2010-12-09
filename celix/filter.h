/*
 * filter.h
 *
 *  Created on: Apr 28, 2010
 *      Author: dk489
 */

#ifndef FILTER_H_
#define FILTER_H_

typedef struct filter * FILTER;

FILTER filter_create(char * filterString);
void filter_destroy(FILTER filter);

int filter_match(FILTER filter, HASHTABLE properties);

#endif /* FILTER_H_ */
