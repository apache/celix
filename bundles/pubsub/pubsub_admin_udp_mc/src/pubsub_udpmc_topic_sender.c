#include <pubsub_serializer.h>
#include <stdlib.h>
#include <memory.h>
#include <pubsub_constants.h>
#include "pubsub_udpmc_topic_sender.h"
#include "pubsub_psa_udpmc_constants.h"

static void pubsub_udpmcTopicSender_setSerializer(void *handle, void *svc, const celix_properties_t *props);

struct pubsub_updmc_topic_sender {
    celix_bundle_context_t *ctx;
    char *scope;
    char *topic;
    char *socketAddress;

    long serTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        pubsub_serializer_service_t *svc;
        const celix_properties_t *props;
    } serializer;
};

pubsub_updmc_topic_sender_t* pubsub_udpmcTopicSender_create(celix_bundle_context_t *ctx, const char *scope, const char *topic, long serializerSvcId) {
    pubsub_updmc_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->scope = strndup(scope, 1024 * 1024);
    sender->topic = strndup(topic, 1024 * 1024);

    celixThreadMutex_create(&sender->serializer.mutex, NULL);

    char filter[64];
    snprintf(filter, 64, "(service.id=%li)", serializerSvcId);

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
    opts.filter.filter = filter;
    opts.filter.ignoreServiceLanguage = true;
    opts.callbackHandle = sender;
    opts.setWithProperties = pubsub_udpmcTopicSender_setSerializer;
    sender->serTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

    return sender;
}

void pubsub_udpmcTopicSender_destroy(pubsub_updmc_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_stopTracker(sender->ctx, sender->serTrackerId);

        celixThreadMutex_destroy(&sender->serializer.mutex);

        free(sender->scope);
        free(sender->topic);
        free(sender);
    }
}

const char* pubsub_udpmcTopicSender_psaType(pubsub_updmc_topic_sender_t *sender __attribute__((unused))) {
    return PSA_UDPMC_PUBSUB_ADMIN_TYPE;
}

const char* pubsub_udpmcTopicSender_serializerType(pubsub_updmc_topic_sender_t *sender) {
    const char *result = NULL;
    celixThreadMutex_lock(&sender->serializer.mutex);
    if (sender->serializer.props != NULL) {
        result = celix_properties_get(sender->serializer.props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    }
    celixThreadMutex_unlock(&sender->serializer.mutex);
    return result;
}

const char* pubsub_udpmcTopicSender_scope(pubsub_updmc_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_udpmcTopicSender_topic(pubsub_updmc_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_udpmcTopicSender_socketAddress(pubsub_updmc_topic_sender_t *sender) {
    return sender->socketAddress;
}

void pubsub_udpmcTopicSender_connectTo(pubsub_updmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

void pubsub_udpmcTopicSender_disconnectFrom(pubsub_updmc_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

static void pubsub_udpmcTopicSender_setSerializer(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_updmc_topic_sender_t *sender = handle;
    pubsub_serializer_service_t *ser = svc;
    celixThreadMutex_lock(&sender->serializer.mutex);
    sender->serializer.svc = ser;
    sender->serializer.props = props;
    celixThreadMutex_unlock(&sender->serializer.mutex);
}