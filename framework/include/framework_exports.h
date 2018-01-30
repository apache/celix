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
 */

#ifndef FRAMEWORK_EXPORTS_H_
#define FRAMEWORK_EXPORTS_H_

/* Cmake will define celix_framework_EXPORTS on Windows when it
configures to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define celix_framework_EXPORTS when
building a DLL on windows.
*/
// We are using the Visual Studio Compiler and building Shared libraries

#if defined _WIN32 || defined __CYGWIN__
  #ifdef celix_framework_EXPORTS
    #ifdef __GNUC__
      #define FRAMEWORK_EXPORT __attribute__ ((dllexport))
      #define ACTIVATOR_EXPORT __attribute__ ((dllexport))
    #else
      #define ACTIVATOR_EXPORT __declspec(dllexport)
      #define FRAMEWORK_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define ACTIVATOR_EXPORT __attribute__ ((dllexport))
      #define FRAMEWORK_EXPORT __attribute__ ((dllimport))
    #else
      #define ACTIVATOR_EXPORT __declspec(dllexport)
      #define FRAMEWORK_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define ACTIVATOR_EXPORT __attribute__ ((visibility ("default")))
    #define FRAMEWORK_EXPORT __attribute__ ((visibility ("default")))
    #define FRAMEWORK_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define ACTIVATOR_EXPORT
    #define FRAMEWORK_EXPORT
    #define FRAMEWORK_LOCAL
  #endif
#endif

#endif /* FRAMEWORK_EXPORTS_H_ */
