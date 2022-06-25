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
/**
 * bonjour_shell.c
 *
 *  \date       Oct 20, 2014
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "bonjour_shell.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>

#include <dns_sd.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <celixbool.h>
#include <shell.h>

#if defined(BSD) || defined(__APPLE__)  || defined(ANDROID)
#include "open_memstream.h"
#include "fmemopen.h"
#endif

#define MAX_BUFFER_SIZE 5120

//static xmlBufferPtr buf; //FOR DEBUG

struct bonjour_shell {
	char *id;
	volatile bool running;

	//service member and mutex
	pthread_mutex_t mutex;
	shell_service_pt service;

	//tcp socket members
	pthread_t listenThread;
	int listenSocket;
	uint16_t portInNetworkByteOrder;

	//dns_sd registration
	DNSServiceRef sdRef;
	TXTRecordRef txtRecord;

};

struct connection_context {
	bool gotCommand;
	bool running;
	int sockfd;
	bonjour_shell_pt shell;
	xmlTextWriterPtr writer;
	xmlTextReaderPtr reader;
	pthread_t sendThread;
	pthread_mutex_t mutex;
	pthread_cond_t dataAvailCond;
	array_list_pt dataList; //protected by mutex
	struct timeval lastUpdated; //protected by mutex
};

static struct connection_context *currentContext = NULL; //TODO update shell to accept void * data next to callback

static void bonjourShell_addDataToCurrentContext(const char* out, const char* err);
static void bonjourShell_sendData(struct connection_context *context);

static celix_status_t bonjourShell_register(bonjour_shell_pt shell);
static celix_status_t bonjourShell_listen(bonjour_shell_pt shell);

static void bonjourShell_acceptConnection(bonjour_shell_pt shell, int connectionFd);

static void bonjourShell_parse(bonjour_shell_pt shell, struct connection_context *context);
static void bonjourShell_parseXmlNode(bonjour_shell_pt shell, struct connection_context *context);

static void bonjourShell_parseStream(bonjour_shell_pt shell, struct connection_context *context);
static void bonjourShell_parseCommand(bonjour_shell_pt shell, struct connection_context *context);

celix_status_t bonjourShell_create(char *id, bonjour_shell_pt *result) {
	celix_status_t status = CELIX_SUCCESS;
	bonjour_shell_pt shell = (bonjour_shell_pt) calloc(1, sizeof(*shell));
	if (shell != NULL) {
		shell->id = strdup(id);
		shell->running = true;
		shell->listenSocket = 0;
		shell->service = NULL;

		pthread_mutex_init(&shell->mutex, NULL);

		pthread_create(&shell->listenThread, NULL, (void *)bonjourShell_listen, shell);

		(*result) = shell;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

static celix_status_t bonjourShell_register(bonjour_shell_pt shell) {
	celix_status_t status = CELIX_SUCCESS;

	uint16_t portInNetworkByteOrder = shell->portInNetworkByteOrder;
	char *srvName = shell->id;
	char portStr[64];
	sprintf(portStr, "%i", ntohs(portInNetworkByteOrder));

	TXTRecordCreate(&shell->txtRecord, 256, NULL);

	TXTRecordSetValue(&shell->txtRecord, "txtver", 1, "1");
	TXTRecordSetValue(&shell->txtRecord, "version", 1, "1");;
	TXTRecordSetValue(&shell->txtRecord, "1st", strlen(shell->id), shell->id);
	TXTRecordSetValue(&shell->txtRecord, "port.p2pj", 5, portStr);
	TXTRecordSetValue(&shell->txtRecord, "status", 5, "avail");

	DNSServiceRegister(&shell->sdRef, 0, 0,
	srvName, /* may be NULL */
	"_presence._tcp",
	NULL, /* may be NULL */
	NULL, /* may be NULL */
	portInNetworkByteOrder, /* In network byte order */
	TXTRecordGetLength(&shell->txtRecord), TXTRecordGetBytesPtr(&shell->txtRecord), /* may be NULL */
	NULL, /* may be NULL */
	NULL /* may be NULL */
	);

	//DNSServiceProcessResult(shell->sdRef);

	return status;
}

static celix_status_t bonjourShell_listen(bonjour_shell_pt shell) {
	celix_status_t status = CELIX_SUCCESS;


	shell->listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (shell->listenSocket < 0) {
		printf("error opening socket\n");
		return CELIX_START_ERROR;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));       /* Clear struct */
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Incoming addr */
	serv_addr.sin_family = AF_INET;                 /* Internet/IP */
	serv_addr.sin_port = 0;   			           /* server port, don't specify let os decide */

	if (bind(shell->listenSocket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) < 0) {
		printf("error binding\n");
		return CELIX_START_ERROR;
	}

	if (listen(shell->listenSocket, 1) < 0) {
		printf("error listening");
		return CELIX_START_ERROR;
	}

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(shell->listenSocket, (struct sockaddr *)&sin, &len) == -1) {
	    perror("getsockname");
		return CELIX_START_ERROR;
	} else {
		shell->portInNetworkByteOrder = sin.sin_port;
	}

	status = bonjourShell_register(shell);
	if (status != CELIX_SUCCESS) {
		return status;
	}

	struct sockaddr_in connect_addr;
	socklen_t slen = sizeof(struct sockaddr_in);
	while (shell->running) {
		int connectionSocket = accept(shell->listenSocket, (struct sockaddr *) &connect_addr, &slen);
		if (connectionSocket < 0) {
			printf("error accepting connection\n");
			return CELIX_START_ERROR;
		} else {
			bonjourShell_acceptConnection(shell, connectionSocket);
		}
	}

	return status;
}

static void bonjourShell_acceptConnection(bonjour_shell_pt shell, int connectionFd) {
	//printf("setting up parser\n");

	struct connection_context context;
	context.gotCommand = false;
	context.running = true;
	context.sockfd = connectionFd;
	context.shell = shell;

	context.reader = xmlReaderForFd(context.sockfd, NULL, NULL, 0);

	xmlOutputBufferPtr outputBuff = xmlOutputBufferCreateFd(context.sockfd,
			NULL);
	context.writer  = xmlNewTextWriter(outputBuff);

	//buf = xmlBufferCreate();
	//xmlTextWriterPtr writer = xmlNewTextWriterMemory(buf, 0);

	//init send thread and needed data types.
	arrayList_create(&context.dataList);
	pthread_cond_init(&context.dataAvailCond, NULL); //TODO destroy
	pthread_mutex_init(&context.mutex, NULL); //TODO destroy
	pthread_create(&context.sendThread, NULL, (void *)bonjourShell_sendData, &context);

	int sockStatus = 0;
	if (context.reader != NULL && context.writer != NULL) {
		while (sockStatus == 0 && shell->running && context.running) {
			bonjourShell_parse(shell, &context);

			//check if socket is closed
			int error = 0;
			socklen_t len = sizeof(error);
			sockStatus = getsockopt(context.sockfd, SOL_SOCKET, SO_ERROR,
					&error, &len);
			if (sockStatus != 0) {
				printf("Got error from socket error is %i", error);
			}
		}

		if (sockStatus == 0) { //shell stopped still connected
			usleep(1500); //wait until all data is send
			xmlTextWriterEndElement(context.writer); //end stream
			xmlTextWriterEndDocument(context.writer);
			close(context.sockfd);
			xmlFreeTextReader(context.reader);
			xmlFreeTextWriter(context.writer);
		}
		//printf("after close + free of xml parser & socker\n");

		context.running = false;
		pthread_cond_signal(&context.dataAvailCond);

		pthread_join(context.sendThread, NULL);
		pthread_mutex_destroy(&context.mutex);
		pthread_cond_destroy(&context.dataAvailCond);
	} else {
		if (context.reader != NULL) {
			xmlFreeTextReader(context.reader);
		}
		if (context.writer != NULL) {
			xmlFreeTextWriter(context.writer);
		}
	}
        arrayList_destroy(context.dataList);

}

static void bonjourShell_parse(bonjour_shell_pt shell, struct connection_context *context) {
	xmlTextReaderRead(context->reader);
	bonjourShell_parseXmlNode(shell, context);
}

static void bonjourShell_parseXmlNode(bonjour_shell_pt shell, struct connection_context *context) {
	   xmlChar *name;

	   int nodeType = xmlTextReaderNodeType(context->reader);

	   if (nodeType == XML_READER_TYPE_ELEMENT) {
		   name = xmlTextReaderLocalName(context->reader);
		   //printf("found element with name %s\n", name);
		   if (strcmp((char *)name, "stream") == 0) {
			   bonjourShell_parseStream(shell, context);
		   } else if (strcmp((char *)name, "body") == 0 && context->gotCommand == false) {
			   bonjourShell_parseCommand(shell, context); //assuming first body element is command
		   } else if (strcmp((char *)name, "message") == 0) {
			   context->gotCommand = false;
		   }
		   xmlFree(name);
	   } else if (nodeType == XML_READER_TYPE_END_ELEMENT /*end element*/ ) {
		   name = xmlTextReaderLocalName(context->reader);
		   //printf("found END element with name %s\n", name);
		   if (strcmp((char *)name, "stream") == 0) {
			  context->running = false;
		   }
		   xmlFree(name);
	   } else {
		   //printf("found node type %i\n", nodeType);
	   }
}

static void bonjourShell_parseStream(bonjour_shell_pt shell, struct connection_context *context) {
	xmlChar *to = xmlTextReaderGetAttribute(context->reader, (xmlChar *)"from");
	xmlChar *from = xmlTextReaderGetAttribute(context->reader, (xmlChar *)"to");

	xmlTextWriterStartDocument(context->writer, NULL, NULL, NULL);
	xmlTextWriterStartElementNS(context->writer, (xmlChar *)"stream", (xmlChar *)"stream", (xmlChar *)"http://etherx.jabber.org/streams");
	xmlTextWriterWriteAttribute(context->writer, (xmlChar *)"xmlns", (xmlChar *)"jabber:client"); //TODO should use namespace method/
	xmlTextWriterWriteAttribute(context->writer, (xmlChar *)"to", to);
	xmlTextWriterWriteAttribute(context->writer, (xmlChar *)"from", from);
	xmlTextWriterWriteAttribute(context->writer, (xmlChar *)"version", (xmlChar *)"1.0");

	xmlTextWriterWriteString(context->writer, (xmlChar *)"\n"); //Needed to flush to writer
	xmlTextWriterFlush(context->writer);
	//printf("current context buf: %s\n", (char *)buf->content);

	if (from != NULL) {
		xmlFree(from);
	}
	if (to != NULL) {
		xmlFree(to);
	}
}

static void bonjourShell_parseCommand(bonjour_shell_pt shell, struct connection_context *context)
{
        xmlChar *command = xmlTextReaderReadString(context->reader);

        if (command != NULL) {
                context->gotCommand = true;
                currentContext = context;
                pthread_mutex_lock(&shell->mutex);
                if (shell->service != NULL) {
                        char *outbuf;
                        size_t outsize;
                        char *errbuf;
                        size_t errsize;

                        FILE *out = open_memstream(&outbuf, &outsize);
                        FILE *err = open_memstream(&errbuf, &errsize);

                        shell->service->executeCommand(shell->service->handle, (char *) command, out, err);

                        fclose(out);
                        fclose(err);
                        bonjourShell_addDataToCurrentContext(outbuf, errbuf);
                        free(outbuf);
                        free(errbuf);
                }
                pthread_mutex_unlock(&shell->mutex);
        }

        if (command != NULL) {
                xmlFree(command);
        }
}

static void bonjourShell_addDataToCurrentContext(const char* out, const char* err) {
	pthread_mutex_lock(&currentContext->mutex);
    if (out != NULL) {
	    arrayList_add(currentContext->dataList, strdup(out));
    }
	if (err != NULL) {
        arrayList_add(currentContext->dataList, strdup(err));
    }
    gettimeofday(&currentContext->lastUpdated, NULL);
	pthread_mutex_unlock(&currentContext->mutex);
	pthread_cond_signal(&currentContext->dataAvailCond);
}

static void bonjourShell_sendData(struct connection_context *context) {
	while (context->running == true ) {
		pthread_mutex_lock(&context->mutex);
		pthread_cond_wait(&context->dataAvailCond, &context->mutex); //wait till some data is ready.

		struct timeval now;
		while (context->running) {
			gettimeofday(&now, NULL);
			long elapsed = (now.tv_sec * 1000000 + now.tv_usec) - (context->lastUpdated.tv_sec * 1000000 + context->lastUpdated.tv_usec);
			if (elapsed > 1000) { //usec passed without update of data.
				break;
			}
			pthread_mutex_unlock(&context->mutex);
			usleep(1000);
			pthread_mutex_lock(&context->mutex);
		}

		if (context->running) {
			xmlTextWriterStartElement(currentContext->writer, (xmlChar *)"message");
			xmlTextWriterWriteAttribute(currentContext->writer, (xmlChar *)"type", (xmlChar *)"chat");
			xmlTextWriterStartElement(currentContext->writer, (xmlChar *)"body");
			xmlTextWriterWriteString(currentContext->writer, (xmlChar *)"\n");
			int i;
			int size = arrayList_size(context->dataList);
			for ( i = 0 ; i < size ; i += 1) {
				char *entry = arrayList_get(context->dataList, i);
				xmlTextWriterWriteString(currentContext->writer, (xmlChar *)entry);
				//xmlTextWriterWriteString(currentContext->writer, (xmlChar *)"\r"); //needed for adium to create new line in UI
				free(entry);
			}
			arrayList_clear(context->dataList);
			xmlTextWriterEndElement(currentContext->writer); //end body
			xmlTextWriterEndElement(currentContext->writer); //end message
			xmlTextWriterWriteString(currentContext->writer, (xmlChar *)"\n"); //flush
			xmlTextWriterFlush(currentContext->writer);
		}
		pthread_mutex_unlock(&context->mutex);
	}
}


celix_status_t bonjourShell_destroy(bonjour_shell_pt shell) {
	DNSServiceRefDeallocate(shell->sdRef);
	TXTRecordDeallocate(&shell->txtRecord);

	close(shell->listenSocket);
	pthread_join(shell->listenThread, NULL);
        free(shell->id);
	free(shell);
	return CELIX_SUCCESS;
}

celix_status_t bonjourShell_addShellService(void * handle, service_reference_pt reference, void * service) {
	bonjour_shell_pt shell = handle;
	pthread_mutex_lock(&shell->mutex);
	shell->service = service;
	pthread_mutex_unlock(&shell->mutex);
	return CELIX_SUCCESS;
}

celix_status_t bonjourShell_removeShellService(void * handle, service_reference_pt reference, void * service) {
	bonjour_shell_pt shell = handle;
	pthread_mutex_lock(&shell->mutex);
	if (shell->service == service) {
		shell->service = NULL;
	}
	pthread_mutex_unlock(&shell->mutex);
	return CELIX_SUCCESS;
}
