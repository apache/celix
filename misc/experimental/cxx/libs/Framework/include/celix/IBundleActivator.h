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

#ifndef CXX_CELIX_IBUNDLEACTIVATOR_H
#define CXX_CELIX_IBUNDLEACTIVATOR_H

namespace celix {
    /**
     * The BundleActivator is a marker interface and contains no virtual methods.
     *
     * The Celix Framework will expect a constructor with a std::shared_ptr<celix:IBundleContext> argument for the
     * concrete bundle activator. RAII will be used to start (on ctor) and stop (on dtor) a bundle.
     */
    class IBundleActivator {
    public:
        virtual ~IBundleActivator() = default;
    };
}

#endif //CXX_CELIX_IBUNDLEACTIVATOR_H
