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
 * topic_subscription.c
 *
 *  \date       Oct 2, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"
#include "celix_errno.h"
#include "constants.h"
#include "version.h"

#include "topic_subscription.h"
#include "topic_publication.h"
#include "pubsub/subscriber.h"
#include "pubsub/publisher.h"
#include "large_udp.h"

#include "pubsub_serializer.h"

#define MAX_EPOLL_EVENTS        10
#define RECV_THREAD_TIMEOUT     5
#define UDP_BUFFER_SIZE         65535
#define MAX_UDP_SESSIONS        16

struct topic_subscription{
	char* ifIpAddress;
	service_tracker_pt tracker;
	array_list_pt sub_ep_list;
	celix_thread_t recv_thread;
	bool running;
	celix_thread_mutex_t ts_lock;
	bundle_context_pt context;

	pubsub_serializer_service_t *serializer;

	int topicEpollFd; // EPOLL filedescriptor where the sockets are registered.
	hash_map_pt servicesMap; // key = service, value = msg types map
	hash_map_pt socketMap; // key = URL, value = listen-socket
	celix_thread_mutex_t socketMap_lock;

	celix_thread_mutex_t pendingConnections_lock;
	array_list_pt pendingConnections;

	array_list_pt pendingDisconnections;
	celix_thread_mutex_t pendingDisconnections_lock;

	//array_list_pt rawServices;
	unsigned int nrSubscribers;
	largeUdp_pt largeUdpHandle;
};

typedef struct msg_map_entry{
	bool retain;
	void* msgInst;
}* msg_map_entry_pt;

static celix_status_t topicsub_subscriberTracked(void * handle, service_reference_pt reference, void * service);
static celix_status_t topicsub_subscriberUntracked(void * handle, service_reference_pt reference, void * service);
static void* udp_recv_thread_func(void* arg);
static bool checkVersion(version_pt msgVersion,pubsub_msg_header_pt hdr);
static void sigusr1_sighandler(int signo);
static int pubsub_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
static void connectPendingPublishers(topic_subscription_pt sub);
static void disconnectPendingPublishers(topic_subscription_pt sub);


celix_status_t pubsub_topicSubscriptionCreate(bundle_context_pt bundle_context, char* ifIp,char* scope, char* topic ,pubsub_serializer_service_t *best_serializer, topic_subscription_pt* out){
	celix_status_t status = CELIX_SUCCESS;

	topic_subscription_pt ts = (topic_subscription_pt) calloc(1,sizeof(*ts));
	ts->context = bundle_context;
	ts->ifIpAddress = strdup(ifIp);
#if defined(__APPLE__) && defined(__MACH__)
	//TODO: Use kqueue for OSX
#else
	ts->topicEpollFd = epoll_create1(0);
#endif
	if(ts->topicEpollFd == -1) {
		status += CELIX_SERVICE_EXCEPTION;
	}

	ts->running = false;
	ts->nrSubscribers = 0;
	ts->serializer = best_serializer;

	celixThreadMutex_create(&ts->ts_lock,NULL);
	arrayList_create(&ts->sub_ep_list);
	ts->servicesMap = hashMap_create(NULL, NULL, NULL, NULL);
	ts->socketMap =  hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	arrayList_create(&ts->pendingConnections);
	arrayList_create(&ts->pendingDisconnections);
	celixThreadMutex_create(&ts->pendingConnections_lock, NULL);
	celixThreadMutex_create(&ts->pendingDisconnections_lock, NULL);
	celixThreadMutex_create(&ts->socketMap_lock, NULL);

	ts->largeUdpHandle = largeUdp_create(MAX_UDP_SESSIONS);

	char filter[128];
	memset(filter,0,128);
	if(strncmp(PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, scope, strlen(PUBSUB_SUBSCRIBER_SCOPE_DEFAULT)) == 0) {
		// default scope, means that subscriber has not defined a scope property
		snprintf(filter, 128, "(&(%s=%s)(%s=%s))",
				(char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_SUBSCRIBER_SERVICE_NAME,
				PUBSUB_SUBSCRIBER_TOPIC,topic);

	} else {
		snprintf(filter, 128, "(&(%s=%s)(%s=%s)(%s=%s))",
				(char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_SUBSCRIBER_SERVICE_NAME,
				PUBSUB_SUBSCRIBER_TOPIC,topic,
				PUBSUB_SUBSCRIBER_SCOPE,scope);
	}

	service_tracker_customizer_pt customizer = NULL;
	status += serviceTrackerCustomizer_create(ts,NULL,topicsub_subscriberTracked,NULL,topicsub_subscriberUntracked,&customizer);
	status += serviceTracker_createWithFilter(bundle_context, filter, customizer, &ts->tracker);

	struct sigaction actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sigusr1_sighandler;

	sigaction(SIGUSR1,&actions,NULL);

	if (status == CELIX_SUCCESS) {
		*out=ts;
	}

	return status;
}

celix_status_t pubsub_topicSubscriptionDestroy(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
	ts->running = false;
	free(ts->ifIpAddress);
	serviceTracker_destroy(ts->tracker);
	arrayList_clear(ts->sub_ep_list);
	arrayList_destroy(ts->sub_ep_list);
	hashMap_destroy(ts->servicesMap,false,false);

	celixThreadMutex_lock(&ts->socketMap_lock);
	hashMap_destroy(ts->socketMap,true,true);
	celixThreadMutex_unlock(&ts->socketMap_lock);
	celixThreadMutex_destroy(&ts->socketMap_lock);

	celixThreadMutex_lock(&ts->pendingConnections_lock);
	arrayList_destroy(ts->pendingConnections);
	celixThreadMutex_unlock(&ts->pendingConnections_lock);
	celixThreadMutex_destroy(&ts->pendingConnections_lock);

	celixThreadMutex_lock(&ts->pendingDisconnections_lock);
	arrayList_destroy(ts->pendingDisconnections);
	celixThreadMutex_unlock(&ts->pendingDisconnections_lock);
	celixThreadMutex_destroy(&ts->pendingDisconnections_lock);

	largeUdp_destroy(ts->largeUdpHandle);
#if defined(__APPLE__) && defined(__MACH__)
	//TODO: Use kqueue for OSX
#else
	close(ts->topicEpollFd);
#endif

	celixThreadMutex_unlock(&ts->ts_lock);

	celixThreadMutex_destroy(&ts->ts_lock);

	free(ts);

	return status;
}

celix_status_t pubsub_topicSubscriptionStart(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	status = serviceTracker_open(ts->tracker);

	ts->running = true;

	if(status==CELIX_SUCCESS){
		status=celixThread_create(&ts->recv_thread,NULL,udp_recv_thread_func,ts);
	}

	return status;
}

celix_status_t pubsub_topicSubscriptionStop(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));

	ts->running = false;

	pthread_kill(ts->recv_thread.thread,SIGUSR1);

	celixThread_join(ts->recv_thread,NULL);

	status = serviceTracker_close(ts->tracker);

	celixThreadMutex_lock(&ts->socketMap_lock);
	hash_map_iterator_pt it = hashMapIterator_create(ts->socketMap);
	while(hashMapIterator_hasNext(it)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(it);
		char *url = hashMapEntry_getKey(entry);
		int *s = hashMapEntry_getValue(entry);
		memset(&ev, 0, sizeof(ev));
		if(epoll_ctl(ts->topicEpollFd, EPOLL_CTL_DEL, *s, &ev) == -1) {
			printf("in if error()\n");
			perror("epoll_ctl() EPOLL_CTL_DEL");
			status += CELIX_SERVICE_EXCEPTION;
		}
		free(s);
		free(url);
		//hashMapIterator_remove(it);
	}
	hashMapIterator_destroy(it);
	hashMap_clear(ts->socketMap, false, false);
	celixThreadMutex_unlock(&ts->socketMap_lock);


	return status;
}

celix_status_t pubsub_topicSubscriptionConnectPublisher(topic_subscription_pt ts, char* pubURL) {

	printf("pubsub_topicSubscriptionConnectPublisher : pubURL = %s\n", pubURL);

	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->socketMap_lock);

	if(!hashMap_containsKey(ts->socketMap, pubURL)){

		int *recvSocket = calloc(sizeof(int), 1);
		*recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (*recvSocket < 0) {
			perror("pubsub_topicSubscriptionCreate:socket");
			status = CELIX_SERVICE_EXCEPTION;
		}

		if (status == CELIX_SUCCESS){
			int reuse = 1;
			if (setsockopt(*recvSocket, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse)) != 0) {
				perror("setsockopt() SO_REUSEADDR");
				status = CELIX_SERVICE_EXCEPTION;
			}
		}

		if(status == CELIX_SUCCESS){
			// TODO Check if there is a better way to parse the URL to IP/Portnr
			//replace ':' by spaces
			char *url = strdup(pubURL);
			char *pt = url;
			while((pt=strchr(pt, ':')) != NULL) {
				*pt = ' ';
			}
			char mcIp[100];
			unsigned short mcPort;
			sscanf(url, "udp //%s %hu", mcIp, &mcPort);
			free(url);

			printf("pubsub_topicSubscriptionConnectPublisher : IP = %s, Port = %hu\n", mcIp, mcPort);

			struct ip_mreq mc_addr;
			mc_addr.imr_multiaddr.s_addr = inet_addr(mcIp);
			mc_addr.imr_interface.s_addr = inet_addr(ts->ifIpAddress);
			printf("Adding MC %s at interface %s\n", mcIp, ts->ifIpAddress);
			if (setsockopt(*recvSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mc_addr, sizeof(mc_addr)) != 0) {
				perror("setsockopt() IP_ADD_MEMBERSHIP");
				status = CELIX_SERVICE_EXCEPTION;
			}

			if (status == CELIX_SUCCESS){
				struct sockaddr_in mcListenAddr;
				mcListenAddr.sin_family = AF_INET;
				mcListenAddr.sin_addr.s_addr = INADDR_ANY;
				mcListenAddr.sin_port = htons(mcPort);
				if(bind(*recvSocket, (struct sockaddr*)&mcListenAddr, sizeof(mcListenAddr)) != 0) {
					perror("bind()");
					status = CELIX_SERVICE_EXCEPTION;
				}
			}

			if (status == CELIX_SUCCESS){
#if defined(__APPLE__) && defined(__MACH__)
				//TODO: Use kqueue for OSX
#else
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN;
				ev.data.fd = *recvSocket;
				if(epoll_ctl(ts->topicEpollFd, EPOLL_CTL_ADD, *recvSocket, &ev) == -1) {
					perror("epoll_ctl() EPOLL_CTL_ADD");
					status = CELIX_SERVICE_EXCEPTION;
				}
#endif
			}

		}

		if (status == CELIX_SUCCESS){
			hashMap_put(ts->socketMap, strdup(pubURL), (void*)recvSocket);
		}
		else{
			free(recvSocket);
		}
	}

	celixThreadMutex_unlock(&ts->socketMap_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionAddConnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL) {
	celix_status_t status = CELIX_SUCCESS;
	char *url = strdup(pubURL);
	celixThreadMutex_lock(&ts->pendingConnections_lock);
	arrayList_add(ts->pendingConnections, url);
	celixThreadMutex_unlock(&ts->pendingConnections_lock);
	return status;
}

celix_status_t pubsub_topicSubscriptionAddDisconnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL) {
	celix_status_t status = CELIX_SUCCESS;
	char *url = strdup(pubURL);
	celixThreadMutex_lock(&ts->pendingDisconnections_lock);
	arrayList_add(ts->pendingDisconnections, url);
	celixThreadMutex_unlock(&ts->pendingDisconnections_lock);
	return status;
}

celix_status_t pubsub_topicSubscriptionDisconnectPublisher(topic_subscription_pt ts, char* pubURL){
	printf("pubsub_topicSubscriptionDisconnectPublisher : pubURL = %s\n", pubURL);
	celix_status_t status = CELIX_SUCCESS;
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));

	celixThreadMutex_lock(&ts->socketMap_lock);

	if (hashMap_containsKey(ts->socketMap, pubURL)){

#if defined(__APPLE__) && defined(__MACH__)
		//TODO: Use kqueue for OSX
#else
		int *s = hashMap_remove(ts->socketMap, pubURL);
		if(epoll_ctl(ts->topicEpollFd, EPOLL_CTL_DEL, *s, &ev) == -1) {
			printf("in if error()\n");
			perror("epoll_ctl() EPOLL_CTL_DEL");
			status = CELIX_SERVICE_EXCEPTION;
		}
		free(s);
#endif

	}

	celixThreadMutex_unlock(&ts->socketMap_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionAddSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
	arrayList_add(ts->sub_ep_list,subEP);
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;

}

celix_status_t pubsub_topicIncreaseNrSubscribers(topic_subscription_pt ts) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
	ts->nrSubscribers++;
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionRemoveSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
	arrayList_removeElement(ts->sub_ep_list,subEP);
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicDecreaseNrSubscribers(topic_subscription_pt ts) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
	ts->nrSubscribers--;
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

unsigned int pubsub_topicGetNrSubscribers(topic_subscription_pt ts) {
	return ts->nrSubscribers;
}

array_list_pt pubsub_topicSubscriptionGetSubscribersList(topic_subscription_pt sub){
	return sub->sub_ep_list;
}


static celix_status_t topicsub_subscriberTracked(void * handle, service_reference_pt reference, void * service){
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
	if (!hashMap_containsKey(ts->servicesMap, service)) {
		bundle_pt bundle = NULL;
		hash_map_pt msgTypes = NULL;

		serviceReference_getBundle(reference, &bundle);

		if(ts->serializer != NULL && bundle!=NULL){
			ts->serializer->createSerializerMap(ts->serializer->handle,bundle,&msgTypes);
			if(msgTypes != NULL){
				hashMap_put(ts->servicesMap, service, msgTypes);
				printf("PSA_UDP_MC_TS: New subscriber registered.\n");
			}
		}
		else{
			printf("PSA_UDP_MC_TS: Cannot register new subscriber.\n");
			status = CELIX_SERVICE_EXCEPTION;
		}
	}
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;

}

static celix_status_t topicsub_subscriberUntracked(void * handle, service_reference_pt reference, void * service){
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
	if (hashMap_containsKey(ts->servicesMap, service)) {
		hash_map_pt msgTypes = hashMap_remove(ts->servicesMap, service);
		if(msgTypes!=NULL && ts->serializer!=NULL){
			ts->serializer->destroySerializerMap(ts->serializer->handle,msgTypes);
			printf("PSA_ZMQ_TS: Subscriber unregistered.\n");
		}
		else{
			printf("PSA_ZMQ_TS: Cannot unregister subscriber.\n");
			status = CELIX_SERVICE_EXCEPTION;
		}
	}
	celixThreadMutex_unlock(&ts->ts_lock);

	printf("PSA_UDP_MC_TS: Subscriber unregistered.\n");
	return status;
}


static void process_msg(topic_subscription_pt sub,pubsub_udp_msg_t *msg){

	celixThreadMutex_lock(&sub->ts_lock);
	hash_map_iterator_pt iter = hashMapIterator_create(sub->servicesMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		pubsub_subscriber_pt subsvc = hashMapEntry_getKey(entry);
		hash_map_pt msgTypes = hashMapEntry_getValue(entry);

		pubsub_msg_serializer_t *msgSer = hashMap_get(msgTypes,(void*)(uintptr_t )msg->header.type);
		if (msgSer == NULL) {
			printf("PSA_UDP_MC_TS: Serializer not available for message %d.\n",msg->header.type);
		}
		else{
			void *msgInst = NULL;
			bool validVersion = checkVersion(msgSer->msgVersion,&msg->header);

			if(validVersion){

				celix_status_t status = msgSer->deserialize(msgSer, (const void *) msg->payload, 0, &msgInst);

				if (status == CELIX_SUCCESS) {
					bool release = true;
					pubsub_multipart_callbacks_t mp_callbacks;
					mp_callbacks.handle = sub;
					mp_callbacks.localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForMsgType;
					mp_callbacks.getMultipart = NULL;

					subsvc->receive(subsvc->handle, msgSer->msgName, msg->header.type, msgInst, &mp_callbacks, &release);

					if(release){
						msgSer->freeMsg(msgSer,msgInst);
					}
				}
				else{
					printf("PSA_UDP_MC_TS: Cannot deserialize msgType %s.\n",msgSer->msgName);
				}

			}
			else{
				int major=0,minor=0;
				version_getMajor(msgSer->msgVersion,&major);
				version_getMinor(msgSer->msgVersion,&minor);
				printf("PSA_UDP_MC_TS: Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
						msgSer->msgName,major,minor,msg->header.major,msg->header.minor);
			}

		}
	}
	hashMapIterator_destroy(iter);
	celixThreadMutex_unlock(&sub->ts_lock);
}

static void* udp_recv_thread_func(void * arg) {
	topic_subscription_pt sub = (topic_subscription_pt) arg;

#if defined(__APPLE__) && defined(__MACH__)
	//TODO: use kqueue for OSX
	//struct kevent events[MAX_EPOLL_EVENTS];
	while (sub->running) {
		int nfds = 0;
		if(nfds > 0) {
			pubsub_udp_msg_t* udpMsg = NULL;
			process_msg(sub, udpMsg);
		}
	}
#else
	struct epoll_event events[MAX_EPOLL_EVENTS];

	while (sub->running) {
		int nfds = epoll_wait(sub->topicEpollFd, events, MAX_EPOLL_EVENTS, RECV_THREAD_TIMEOUT * 1000);
		int i;
		for(i = 0; i < nfds; i++ ) {
			unsigned int index;
			unsigned int size;
			if(largeUdp_dataAvailable(sub->largeUdpHandle, events[i].data.fd, &index, &size) == true) {
				// Handle data
				pubsub_udp_msg_t *udpMsg = NULL;
				if(largeUdp_read(sub->largeUdpHandle, index, (void**)&udpMsg, size) != 0) {
					printf("PSA_UDP_MC_TS: ERROR largeUdp_read with index %d\n", index);
					continue;
				}

				process_msg(sub, udpMsg);

				free(udpMsg);
			}
		}
		connectPendingPublishers(sub);
		disconnectPendingPublishers(sub);
	}
#endif

	return NULL;
}

static void connectPendingPublishers(topic_subscription_pt sub) {
	celixThreadMutex_lock(&sub->pendingConnections_lock);
	while(!arrayList_isEmpty(sub->pendingConnections)) {
		char * pubEP = arrayList_remove(sub->pendingConnections, 0);
		pubsub_topicSubscriptionConnectPublisher(sub, pubEP);
		free(pubEP);
	}
	celixThreadMutex_unlock(&sub->pendingConnections_lock);
}

static void disconnectPendingPublishers(topic_subscription_pt sub) {
	celixThreadMutex_lock(&sub->pendingDisconnections_lock);
	while(!arrayList_isEmpty(sub->pendingDisconnections)) {
		char * pubEP = arrayList_remove(sub->pendingDisconnections, 0);
		pubsub_topicSubscriptionDisconnectPublisher(sub, pubEP);
		free(pubEP);
	}
	celixThreadMutex_unlock(&sub->pendingDisconnections_lock);
}

static void sigusr1_sighandler(int signo){
	printf("PSA_UDP_MC_TS: Topic subscription being shut down...\n");
	return;
}

static bool checkVersion(version_pt msgVersion,pubsub_msg_header_pt hdr){
	bool check=false;
	int major=0,minor=0;

	if(msgVersion!=NULL){
		version_getMajor(msgVersion,&major);
		version_getMinor(msgVersion,&minor);
		if(hdr->major==((unsigned char)major)){ /* Different major means incompatible */
			check = (hdr->minor>=((unsigned char)minor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
		}
	}

	return check;
}

static int pubsub_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId){
	*msgTypeId = utils_stringHash(msgType);
	return 0;
}
