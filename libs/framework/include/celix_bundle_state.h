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

#ifndef CELIX_BUNDLE_STATE_H_
#define CELIX_BUNDLE_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum celix_bundleState {
    CELIX_BUNDLE_STATE_UNKNOWN =        0x00000000,
    CELIX_BUNDLE_STATE_UNINSTALLED =    0x00000001,
    CELIX_BUNDLE_STATE_INSTALLED =      0x00000002,
    CELIX_BUNDLE_STATE_RESOLVED =       0x00000004,
    CELIX_BUNDLE_STATE_STARTING =       0x00000008,
    CELIX_BUNDLE_STATE_STOPPING =       0x00000010,
    CELIX_BUNDLE_STATE_ACTIVE =         0x00000020,

    /*
     * Note the OSGI_ bundle state symbols will be removed in the next major update.
     * Currently, they are equivalent to the CELIX_BUNDLE_STATE_ variants.
     */
    OSGI_FRAMEWORK_BUNDLE_UNKNOWN =     0x00000000,
    OSGI_FRAMEWORK_BUNDLE_UNINSTALLED = 0x00000001,
    OSGI_FRAMEWORK_BUNDLE_INSTALLED =   0x00000002,
    OSGI_FRAMEWORK_BUNDLE_RESOLVED =    0x00000004,
    OSGI_FRAMEWORK_BUNDLE_STARTING =    0x00000008,
    OSGI_FRAMEWORK_BUNDLE_STOPPING =    0x00000010,
    OSGI_FRAMEWORK_BUNDLE_ACTIVE =      0x00000020,
};

typedef enum celix_bundleState bundle_state_e;
typedef enum celix_bundleState celix_bundle_state_e;

const char* celix_bundleState_getName(celix_bundle_state_e state);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_STATE_H_ */

