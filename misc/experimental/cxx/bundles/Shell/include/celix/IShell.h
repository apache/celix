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

#ifndef CXX_CELIX_ISHELL_H
#define CXX_CELIX_ISHELL_H

#include <iostream>

namespace celix {
    class IShell {
    public:
        static constexpr const char * const SERVICE_FQN = "celix::IShell [Version 1]";

        virtual ~IShell() = default;

        virtual bool executeCommandLine(const std::string &commandLine, std::ostream &out, std::ostream &err) noexcept = 0;
    };
}

#endif //CXX_CELIX_ISHELL_H
