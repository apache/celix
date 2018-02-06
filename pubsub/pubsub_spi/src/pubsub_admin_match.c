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


#include <string.h>
#include "service_reference.h"

#include "pubsub_admin.h"

#include "pubsub_admin_match.h"

#define KNOWN_PUBSUB_ADMIN_NUM	2
#define KNOWN_SERIALIZER_NUM	2

static char* qos_sample_pubsub_admin_prio_list[KNOWN_PUBSUB_ADMIN_NUM] = {"udp_mc","zmq"};
static char* qos_sample_serializer_prio_list[KNOWN_SERIALIZER_NUM] = {"json","void"};

static char* qos_control_pubsub_admin_prio_list[KNOWN_PUBSUB_ADMIN_NUM] = {"zmq","udp_mc"};
static char* qos_control_serializer_prio_list[KNOWN_SERIALIZER_NUM] = {"json","void"};

static double qos_pubsub_admin_score[KNOWN_PUBSUB_ADMIN_NUM] = {100.0F,75.0F};
static double qos_serializer_score[KNOWN_SERIALIZER_NUM] = {30.0F,20.0F};

static void get_serializer_type(service_reference_pt svcRef, char **serializerType);
static void manage_service_from_reference(service_reference_pt svcRef, void **svc, bool getService);

celix_status_t pubsub_admin_match(properties_pt endpoint_props, const char *pubsub_admin_type, array_list_pt serializerList, double *score){

	celix_status_t status = CELIX_SUCCESS;
	double final_score = 0;
	int i = 0, j = 0;

	const char *requested_admin_type 		= NULL;
	const char *requested_serializer_type 	= NULL;
	const char *requested_qos_type			= NULL;

	if(endpoint_props!=NULL){
		requested_admin_type 		= properties_get(endpoint_props,PUBSUB_ADMIN_TYPE_KEY);
		requested_serializer_type 	= properties_get(endpoint_props,PUBSUB_SERIALIZER_TYPE_KEY);
		requested_qos_type			= properties_get(endpoint_props,QOS_ATTRIBUTE_KEY);
	}

	/* Analyze the pubsub_admin */
	if(requested_admin_type != NULL){ /* We got precise specification on the pubsub_admin we want */
		if(strncmp(requested_admin_type,pubsub_admin_type,strlen(pubsub_admin_type))==0){ //Full match
			final_score += PUBSUB_ADMIN_FULL_MATCH_SCORE;
		}
	}
	else if(requested_qos_type != NULL){ /* We got QoS specification that will determine the selected PSA */
		if(strncmp(requested_qos_type,QOS_TYPE_SAMPLE,strlen(QOS_TYPE_SAMPLE))==0){
			for(i=0;i<KNOWN_PUBSUB_ADMIN_NUM;i++){
				if(strncmp(qos_sample_pubsub_admin_prio_list[i],pubsub_admin_type,strlen(pubsub_admin_type))==0){
					final_score += qos_pubsub_admin_score[i];
					break;
				}
			}
		}
		else if(strncmp(requested_qos_type,QOS_TYPE_CONTROL,strlen(QOS_TYPE_CONTROL))==0){
			for(i=0;i<KNOWN_PUBSUB_ADMIN_NUM;i++){
				if(strncmp(qos_control_pubsub_admin_prio_list[i],pubsub_admin_type,strlen(pubsub_admin_type))==0){
					final_score += qos_pubsub_admin_score[i];
					break;
				}
			}
		}
		else{
			printf("Unknown QoS type '%s'\n",requested_qos_type);
			status = CELIX_ILLEGAL_ARGUMENT;
		}
	}
	else{ /* We got no specification: fallback to Qos=Sample, but count half the score */
		for(i=0;i<KNOWN_PUBSUB_ADMIN_NUM;i++){
			if(strncmp(qos_sample_pubsub_admin_prio_list[i],pubsub_admin_type,strlen(pubsub_admin_type))==0){
				final_score += (qos_pubsub_admin_score[i]/2);
				break;
			}
		}
	}

	char *serializer_type = NULL;
	/* Analyze the serializers */
	if(requested_serializer_type != NULL){ /* We got precise specification on the serializer we want */
		for(i=0;i<arrayList_size(serializerList);i++){
			service_reference_pt svcRef = (service_reference_pt)arrayList_get(serializerList,i);
			get_serializer_type(svcRef, &serializer_type);
			if(serializer_type != NULL){
				if(strncmp(requested_serializer_type,serializer_type,strlen(serializer_type))==0){
					final_score += SERIALIZER_FULL_MATCH_SCORE;
					break;
				}
			}
		}
	}
	else if(requested_qos_type != NULL){ /* We got QoS specification that will determine the selected serializer */
		if(strncmp(requested_qos_type,QOS_TYPE_SAMPLE,strlen(QOS_TYPE_SAMPLE))==0){
			bool ser_found = false;
			for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
				for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
					service_reference_pt svcRef = (service_reference_pt)arrayList_get(serializerList,j);
					get_serializer_type(svcRef, &serializer_type);
					if(serializer_type != NULL){
						if(strncmp(qos_sample_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
							ser_found = true;
						}
					}
				}
				if(ser_found){
					final_score += qos_serializer_score[i];
				}
			}
		}
		else if(strncmp(requested_qos_type,QOS_TYPE_CONTROL,strlen(QOS_TYPE_CONTROL))==0){
			bool ser_found = false;
			for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
				for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
					service_reference_pt svcRef = (service_reference_pt)arrayList_get(serializerList,j);
					get_serializer_type(svcRef, &serializer_type);
					if(serializer_type != NULL){
						if(strncmp(qos_control_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
							ser_found = true;
						}
					}
				}
				if(ser_found){
					final_score += qos_serializer_score[i];
				}
			}
		}
		else{
			printf("Unknown QoS type '%s'\n",requested_qos_type);
			status = CELIX_ILLEGAL_ARGUMENT;
		}
	}
	else{ /* We got no specification: fallback to Qos=Sample, but count half the score */
		bool ser_found = false;
		for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
			for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
				service_reference_pt svcRef = (service_reference_pt)arrayList_get(serializerList,j);
				get_serializer_type(svcRef, &serializer_type);
				if(serializer_type != NULL){
					if(strncmp(qos_sample_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
						ser_found = true;
					}
				}
			}
			if(ser_found){
				final_score += (qos_serializer_score[i]/2);
			}
		}
	}

	*score = final_score;

	printf("Score for pair <%s,%s> = %f\n",pubsub_admin_type,serializer_type,final_score);

	return status;
}

celix_status_t pubsub_admin_get_best_serializer(properties_pt endpoint_props, array_list_pt serializerList, pubsub_serializer_service_t **serSvc){
	celix_status_t status = CELIX_SUCCESS;

	int i = 0, j = 0;

	const char *requested_serializer_type = NULL;
	const char *requested_qos_type = NULL;

	if (endpoint_props != NULL){
		requested_serializer_type = properties_get(endpoint_props,PUBSUB_SERIALIZER_TYPE_KEY);
		requested_qos_type = properties_get(endpoint_props,QOS_ATTRIBUTE_KEY);
	}

	service_reference_pt svcRef = NULL;
	void *svc = NULL;

	/* Analyze the serializers */
	if (arrayList_size(serializerList) == 1) {
		// Only one serializer, use this one
		svcRef = (service_reference_pt)arrayList_get(serializerList,0);
		manage_service_from_reference(svcRef, &svc, true);
		*serSvc = svc;
		char *serializer_type = NULL;
		get_serializer_type(svcRef, &serializer_type);
		printf("Selected the only serializer available. Type = %s\n", serializer_type);

	}
	else if(requested_serializer_type != NULL){ /* We got precise specification on the serializer we want */
		for(i=0;i<arrayList_size(serializerList);i++){
			svcRef = (service_reference_pt)arrayList_get(serializerList,i);
			char *serializer_type = NULL;
			get_serializer_type(svcRef, &serializer_type);
			if(serializer_type != NULL){
				if(strncmp(requested_serializer_type,serializer_type,strlen(serializer_type))==0){
					manage_service_from_reference(svcRef, &svc,true);
					if(svc==NULL){
						printf("Cannot get pubsub_serializer_service from serviceReference %p\n",svcRef);
						status = CELIX_SERVICE_EXCEPTION;
					}
					*serSvc = svc;
					break;
				}
			}
		}
	}
	else if(requested_qos_type != NULL){ /* We got QoS specification that will determine the selected serializer */
		if(strncmp(requested_qos_type,QOS_TYPE_SAMPLE,strlen(QOS_TYPE_SAMPLE))==0){
			bool ser_found = false;
			for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
				for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
					svcRef = (service_reference_pt)arrayList_get(serializerList,j);
					char *serializer_type = NULL;
					get_serializer_type(svcRef, &serializer_type);
					if(serializer_type != NULL){
						if(strncmp(qos_sample_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
							manage_service_from_reference(svcRef, &svc,true);
							if(svc==NULL){
								printf("Cannot get pubsub_serializer_service from serviceReference %p\n",svcRef);
								status = CELIX_SERVICE_EXCEPTION;
							}
							else{
								*serSvc = svc;
								ser_found = true;
								printf("Selected %s serializer as best for QoS=%s\n",qos_sample_serializer_prio_list[i],QOS_TYPE_SAMPLE);
							}
						}
					}
				}
			}
		}
		else if(strncmp(requested_qos_type,QOS_TYPE_CONTROL,strlen(QOS_TYPE_CONTROL))==0){
			bool ser_found = false;
			for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
				for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
					svcRef = (service_reference_pt)arrayList_get(serializerList,j);
					char *serializer_type = NULL;
					get_serializer_type(svcRef, &serializer_type);
					if(serializer_type != NULL){
						if(strncmp(qos_control_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
							manage_service_from_reference(svcRef, &svc,true);
							if(svc==NULL){
								printf("Cannot get pubsub_serializer_service from serviceReference %p\n",svcRef);
								status = CELIX_SERVICE_EXCEPTION;
							}
							else{
								*serSvc = svc;
								ser_found = true;
								printf("Selected %s serializer as best for QoS=%s\n",qos_control_serializer_prio_list[i],QOS_TYPE_CONTROL);
							}
						}
					}
				}
			}
		}
		else{
			printf("Unknown QoS type '%s'\n",requested_qos_type);
			status = CELIX_ILLEGAL_ARGUMENT;
		}
	}
	else{ /* We got no specification: fallback to Qos=Sample, but count half the score */
		bool ser_found = false;
		for(i=0;i<KNOWN_SERIALIZER_NUM && !ser_found;i++){
			for(j=0;j<arrayList_size(serializerList) && !ser_found;j++){
				svcRef = (service_reference_pt)arrayList_get(serializerList,j);
				char *serializer_type = NULL;
				get_serializer_type(svcRef, &serializer_type);
				if(serializer_type != NULL){
					if(strncmp(qos_sample_serializer_prio_list[i],serializer_type,strlen(serializer_type))==0){
						manage_service_from_reference(svcRef, &svc,true);
						if(svc==NULL){
							printf("Cannot get pubsub_serializer_service from serviceReference %p\n",svcRef);
							status = CELIX_SERVICE_EXCEPTION;
						}
						else{
							*serSvc = svc;
							ser_found = true;
							printf("Selected %s serializer as best without any specification\n",qos_sample_serializer_prio_list[i]);
						}
					}
				}
			}
		}
	}

	if(svc!=NULL && svcRef!=NULL){
		manage_service_from_reference(svcRef, svc, false);
	}

	return status;
}

static void get_serializer_type(service_reference_pt svcRef, char **serializerType){

	const char *serType = NULL;
	serviceReference_getProperty(svcRef, PUBSUB_SERIALIZER_TYPE_KEY,&serType);
	if(serType != NULL){
		*serializerType = (char*)serType;
	}
	else{
		printf("Serializer serviceReference %p has no pubsub_serializer.type property specified\n",svcRef);
		*serializerType = NULL;
	}
}

static void manage_service_from_reference(service_reference_pt svcRef, void **svc, bool getService){
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	serviceReference_getBundle(svcRef, &bundle);
	bundle_getContext(bundle, &context);
	if(getService){
		bundleContext_getService(context, svcRef, svc);
	}
	else{
		bundleContext_ungetService(context, svcRef, NULL);
	}
}
