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
 * exports.h
 *
 *  \date       Jun 16, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EXPORTS_H_
#define EXPORTS_H_

/* Cmake will define utils_EXPORTS on Windows when it
configures to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define utils_EXPORTS when
building a DLL on windows.

We are using the Visual Studio Compiler and building Shared libraries
*/

#if defined (_WIN32)
  #if defined(celix_utils_EXPORTS)
    #define  UTILS_EXPORT __declspec(dllexport)
  #else
    #define  UTILS_EXPORT __declspec(dllimport)
  #endif /* celix_utils_EXPORTS */
#else /* defined (_WIN32) */
  #define UTILS_EXPORT __attribute__((visibility("default")))
#endif

#endif /* EXPORTS_H_ */
