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

#include <exception>
#if __cplusplus >= 201703L //C++17 or higher
#include <string_view>
#endif

namespace celix {

    /**
     * @brief Celix runtime IO Exception
     */
    class IOException : public std::exception {
    public:
#if __cplusplus >= 201703L //C++17 or higher
        explicit IOException(std::string_view msg) : w{msg} {}
#else
        explicit IOException(std::string msg) : w{std::move(msg)} {}
#endif

        IOException(const IOException&) = default;
        IOException(IOException&&) = default;
        IOException& operator=(const IOException&) = default;
        IOException& operator=(IOException&&) = default;

        [[nodiscard]] const char* what() const noexcept override {
            return w.c_str();
        }
    private:
        std::string w;
    };

}
