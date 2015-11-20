/*
 * benchmark_result.h
 *
 *  Created on: Feb 13, 2014
 *      Author: dl436
 */

#ifndef BENCHMARK_RESULT_H_
#define BENCHMARK_RESULT_H_

typedef struct benchmark_result {
		unsigned int nrOfThreads;
		unsigned int nrOfsamples;
		unsigned int requestedNrOfSamples;
		unsigned int result;
		unsigned int skips;
		double averageCallTimeInNanoseconds;
		double callFrequencyInMhz;
} benchmark_result_t;

#endif /* BENCHMARK_RESULT_H_ */
