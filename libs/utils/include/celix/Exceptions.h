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
#pragma once

#include "celix_errno.h"

#include <stdexcept>
#include <cassert>

namespace celix {

    /**
     * @brief Celix Generic Exception
     */
    class Exception : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief Celix Illegal Argument Exception
     */
    class IllegalArgumentException final: public Exception {
    public:
        using Exception::Exception;
    };

    /**
     * @brief Celix IO Exception
     */
    class IOException final: public Exception {
    public:
        using Exception::Exception;
    };

    namespace impl {
        /**
        * @brief Utils function to throw a celix::Exception using the given celix_status_t and message.
        */
        void inline throwException(celix_status_t status, const std::string& message) {
            assert(status != CELIX_SUCCESS);
            const auto* statusMsg = celix_strerror(status);
            auto msg = std::string{message} + " (" + statusMsg + ")";
            switch (status) {
                case CELIX_ILLEGAL_ARGUMENT:
                    throw celix::IllegalArgumentException{msg};
                case CELIX_FILE_IO_EXCEPTION:
                    throw celix::IOException{msg};
                default:
                    throw celix::Exception{msg};
            }
        }
    }

}
