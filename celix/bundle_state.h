/*
 * bundle_state.h
 *
 *  Created on: Sep 27, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_STATE_H_
#define BUNDLE_STATE_H_

enum bundleState
{
	BUNDLE_UNINSTALLED = 0x00000001,
	BUNDLE_INSTALLED = 0x00000002,
	BUNDLE_RESOLVED = 0x00000004,
	BUNDLE_STARTING = 0x00000008,
	BUNDLE_STOPPING = 0x00000010,
	BUNDLE_ACTIVE = 0x00000020,
};

typedef enum bundleState BUNDLE_STATE;

#endif /* BUNDLE_STATE_H_ */
