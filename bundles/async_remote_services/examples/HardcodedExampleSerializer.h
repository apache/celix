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
#include <jansson.h>
#include <bits/types/struct_iovec.h>
#include <pubsub_constants.h>

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
    explicit AddArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> &mng) : _mng(mng) {
        _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
            json_error_t error;
            json_t *js_request = json_loads(static_cast<char*>(input->iov_base), 0, &error);

            if(!js_request) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            if(!json_is_object(js_request)) {
                json_decref(js_request);
                return CELIX_ILLEGAL_ARGUMENT;
            }

            auto idObj = json_object_get(js_request, "id");
            auto aObj = json_object_get(js_request, "a");
            auto bObj = json_object_get(js_request, "b");
            auto retObj = json_object_get(js_request, "ret");

            int id = json_integer_value(idObj);
            int a = json_integer_value(aObj);
            int b = json_integer_value(bObj);
            decltype(AddArgs::ret) ret = retObj != nullptr ? json_integer_value(retObj) : decltype(AddArgs::ret){};

            *out = new AddArgs{id, a, b, ret};
            json_decref(js_request);

            return CELIX_SUCCESS;
        };
        _svc.freeDeserializedMsg = [](void*, void* msg) {
            delete static_cast<AddArgs*>(msg);
        };

        _svc.serialize = [](void*, const void* input, struct iovec** output, size_t* outputIovLen) -> celix_status_t {
            auto *args = static_cast<AddArgs const *>(input);

            if (*output == nullptr) {
                *output = static_cast<iovec *>(calloc(1, sizeof(struct iovec)));
                if (outputIovLen){
                    *outputIovLen = 1;
                }
            }

            auto *js = json_object();
            json_object_set(js, "id", json_integer(args->id));
            json_object_set(js, "a", json_integer(args->a));
            json_object_set(js, "b", json_integer(args->b));
            if(args->ret) {
                json_object_set(js, "ret", json_integer(args->ret.value()));
            }

            (*output)->iov_base = json_dumps(js, JSON_DECODE_ANY);
            (*output)->iov_len = std::strlen(static_cast<const char *>((*output)->iov_base));

            json_decref(js);

            return CELIX_SUCCESS;
        };

        _svc.freeSerializedMsg = [](void*, struct iovec* input, size_t) {
            free(input->iov_base);
            free(input);
        };

        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, "AddArgs");
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "1.0.0");
        celix_properties_setLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 1LL);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, "json");

        celix_service_registration_options_t opts{};
        opts.svc = &_svc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        opts.properties = props;
        _svcId = celix_bundleContext_registerServiceWithOptions(_mng->bundleContext(), &opts);
    }

    ~AddArgsSerializer() {
        celix_bundleContext_unregisterService(_mng->bundleContext(), _svcId);
    }

private:
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    pubsub_message_serialization_service _svc{};
    long _svcId{};
};

struct SubtractArgsSerializer {
    explicit SubtractArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> &mng) : _mng(mng) {
        _svc.handle = this;
        _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
            json_error_t error;
            json_t *js_request = json_loads(static_cast<char*>(input->iov_base), 0, &error);

            if(!js_request) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            if(!json_is_object(js_request)) {
                json_decref(js_request);
                return CELIX_ILLEGAL_ARGUMENT;
            }

            auto idObj = json_object_get(js_request, "id");
            auto aObj = json_object_get(js_request, "a");
            auto bObj = json_object_get(js_request, "b");
            auto retObj = json_object_get(js_request, "ret");

            int id = json_integer_value(idObj);
            int a = json_integer_value(aObj);
            int b = json_integer_value(bObj);
            decltype(SubtractArgs::ret) ret = retObj != nullptr ? json_integer_value(retObj) : decltype(SubtractArgs::ret){};

            *out = new SubtractArgs{id, a, b, ret};
            json_decref(js_request);

            return CELIX_SUCCESS;
        };
        _svc.freeDeserializedMsg = [](void*, void* msg) {
            delete static_cast<SubtractArgs*>(msg);
        };

        _svc.serialize = [](void*, const void* input, struct iovec** output, size_t* outputIovLen) -> celix_status_t {
            auto *args = static_cast<SubtractArgs const *>(input);

            if (*output == nullptr) {
                *output = static_cast<iovec *>(calloc(1, sizeof(struct iovec)));
                if (outputIovLen){
                    *outputIovLen = 1;
                }
            }

            auto *js = json_object();
            json_object_set(js, "id", json_integer(args->id));
            json_object_set(js, "a", json_integer(args->a));
            json_object_set(js, "b", json_integer(args->b));
            if(args->ret) {
                json_object_set(js, "ret", json_integer(args->ret.value()));
            }

            (*output)->iov_base = json_dumps(js, JSON_DECODE_ANY);
            (*output)->iov_len = std::strlen(static_cast<const char *>((*output)->iov_base));

            json_decref(js);

            return CELIX_SUCCESS;
        };

        _svc.freeSerializedMsg = [](void*, struct iovec* input, size_t) {
            free(input->iov_base);
            free(input);
        };

        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, "SubtractArgs");
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "1.0.0");
        celix_properties_setLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 2LL);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, "json");

        celix_service_registration_options_t opts{};
        opts.svc = &_svc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        opts.properties = props;
        _svcId = celix_bundleContext_registerServiceWithOptions(_mng->bundleContext(), &opts);
    }

    ~SubtractArgsSerializer() {
        celix_bundleContext_unregisterService(_mng->bundleContext(), _svcId);
    }

private:
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    pubsub_message_serialization_service _svc{};
    long _svcId{};
};

struct ToStringArgsSerializer {
    explicit ToStringArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> &mng) : _mng(mng) {
        _svc.handle = this;
        _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
            json_error_t error;
            json_t *js_request = json_loads(static_cast<char*>(input->iov_base), 0, &error);

            if(!js_request) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            if(!json_is_object(js_request)) {
                json_decref(js_request);
                return CELIX_ILLEGAL_ARGUMENT;
            }

            auto idObj = json_object_get(js_request, "id");
            auto aObj = json_object_get(js_request, "a");
            auto retObj = json_object_get(js_request, "ret");

            int id = json_integer_value(idObj);
            int a = json_integer_value(aObj);
            decltype(ToStringArgs::ret) ret = retObj != nullptr ? json_string_value(retObj) : decltype(ToStringArgs::ret){};

            *out = new ToStringArgs{id, a, ret};
            json_decref(js_request);

            return CELIX_SUCCESS;
        };
        _svc.freeDeserializedMsg = [](void*, void* msg) {
            delete static_cast<ToStringArgs*>(msg);
        };

        _svc.serialize = [](void*, const void* input, struct iovec** output, size_t* outputIovLen) -> celix_status_t {
            auto *args = static_cast<ToStringArgs const *>(input);

            if (*output == nullptr) {
                *output = static_cast<iovec *>(calloc(1, sizeof(struct iovec)));
                if (outputIovLen){
                    *outputIovLen = 1;
                }
            }

            auto *js = json_object();
            json_object_set(js, "id", json_integer(args->id));
            json_object_set(js, "a", json_integer(args->a));
            if(args->ret) {
                json_object_set(js, "ret", json_string(args->ret.value().c_str()));
            }

            (*output)->iov_base = json_dumps(js, JSON_DECODE_ANY);
            (*output)->iov_len = std::strlen(static_cast<const char *>((*output)->iov_base));

            json_decref(js);

            return CELIX_SUCCESS;
        };

        _svc.freeSerializedMsg = [](void*, struct iovec* input, size_t) {
            free(input->iov_base);
            free(input);
        };

        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, "ToStringArgs");
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "1.0.0");
        celix_properties_setLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 3LL);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, "json");

        celix_service_registration_options_t opts{};
        opts.svc = &_svc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        opts.properties = props;
        _svcId = celix_bundleContext_registerServiceWithOptions(_mng->bundleContext(), &opts);
    }

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