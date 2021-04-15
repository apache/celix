//
// Created by oipo on 01-03-21.
//

#include "HardcodedExampleSerializer.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

AddArgsSerializer::AddArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng) : _mng(std::move(mng)) {
    _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
        rapidjson::Document d{};
        d.Parse(static_cast<char*>(input->iov_base));

        if(d.HasParseError()) {
            return CELIX_ILLEGAL_ARGUMENT;
        }

        int id = d["id"].GetInt();
        int a = d["a"].GetInt();
        int b = d["b"].GetInt();
        decltype(AddArgs::ret) ret = d.HasMember("ret") ? d["ret"].GetInt() : decltype(AddArgs::ret){};

        *out = new AddArgs{id, a, b, ret};

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

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();

        writer.String("id");
        writer.Int(args->id);

        writer.String("a");
        writer.Int(args->a);

        writer.String("b");
        writer.Int(args->b);

        if(args->ret) {
            writer.String("ret");
            writer.Int(args->ret.value());
        }

        writer.EndObject();

        (*output)->iov_base = strndup(sb.GetString(), sb.GetSize() + 1);
        (*output)->iov_len = sb.GetSize() + 1;

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

SubtractArgsSerializer::SubtractArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng) : _mng(std::move(mng)) {
    _svc.handle = this;
    _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
        rapidjson::Document d{};
        d.Parse(static_cast<char*>(input->iov_base));

        if(d.HasParseError()) {
            return CELIX_ILLEGAL_ARGUMENT;
        }

        int id = d["id"].GetInt();
        int a = d["a"].GetInt();
        int b = d["b"].GetInt();
        decltype(SubtractArgs::ret) ret = d.HasMember("ret") ? d["ret"].GetInt() : decltype(SubtractArgs::ret){};

        *out = new SubtractArgs{id, a, b, ret};

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

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();

        writer.String("id");
        writer.Int(args->id);

        writer.String("a");
        writer.Int(args->a);

        writer.String("b");
        writer.Int(args->b);

        if(args->ret) {
            writer.String("ret");
            writer.Int(args->ret.value());
        }

        writer.EndObject();

        (*output)->iov_base = strndup(sb.GetString(), sb.GetSize() + 1);
        (*output)->iov_len = sb.GetSize() + 1;

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

ToStringArgsSerializer::ToStringArgsSerializer(std::shared_ptr<celix::dm::DependencyManager> mng) : _mng(std::move(mng)) {
    _svc.handle = this;
    _svc.deserialize = [](void*, const struct iovec* input, size_t, void** out) -> celix_status_t {
        rapidjson::Document d{};
        d.Parse(static_cast<char*>(input->iov_base));

        if(d.HasParseError()) {
            return CELIX_ILLEGAL_ARGUMENT;
        }

        int id = d["id"].GetInt();
        int a = d["a"].GetInt();
        decltype(ToStringArgs::ret) ret = d.HasMember("ret") ? d["ret"].GetString() : decltype(ToStringArgs::ret){};

        *out = new ToStringArgs{id, a, ret};

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

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();

        writer.String("id");
        writer.Int(args->id);

        writer.String("a");
        writer.Int(args->a);

        if(args->ret) {
            writer.String("ret");
            writer.String(args->ret.value().c_str(), args->ret.value().size());
        }

        writer.EndObject();

        (*output)->iov_base = strndup(sb.GetString(), sb.GetSize() + 1);
        (*output)->iov_len = sb.GetSize() + 1;

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