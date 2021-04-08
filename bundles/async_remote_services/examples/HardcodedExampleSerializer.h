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

#include <celix/dm/DependencyManager.h>
#include <pubsub_message_serialization_service.h>
#include <sys/uio.h>
#include <pubsub_constants.h>

#include <utility>

struct AddArgs {
    int id{};
    int a{};
    int b{};
    std::optional<int> ret{};
};

struct SubtractArgs {
    int id{};
    int a{};
    int b{};
    std::optional<int> ret{};
};

struct ToStringArgs {
    int id{};
    int a{};
    std::optional<std::string> ret{};
};

struct AddArgsSerializer {
    explicit AddArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng);

    ~AddArgsSerializer() {
        celix_bundleContext_unregisterService(_mng->bundleContext(), _svcId);
    }

private:
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    pubsub_message_serialization_service _svc{};
    long _svcId{};
};

struct SubtractArgsSerializer {
    explicit SubtractArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng);

    ~SubtractArgsSerializer() {
        celix_bundleContext_unregisterService(_mng->bundleContext(), _svcId);
    }

private:
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    pubsub_message_serialization_service _svc{};
    long _svcId{};
};

struct ToStringArgsSerializer {
    explicit ToStringArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng);

    ToStringArgsSerializer(ToStringArgsSerializer const &) = delete;
    ToStringArgsSerializer(ToStringArgsSerializer&&) = default;
    ToStringArgsSerializer& operator=(ToStringArgsSerializer const &) = delete;
    ToStringArgsSerializer& operator=(ToStringArgsSerializer &&) = default;

    ~ToStringArgsSerializer() {
        celix_bundleContext_unregisterService(_mng->bundleContext(), _svcId);
    }

private:
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    pubsub_message_serialization_service _svc{};
    long _svcId{};
};