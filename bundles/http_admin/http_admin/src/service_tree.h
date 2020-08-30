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
 * service_tree.h
 *
 *  \date       Jun 19, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef SERVICE_TREE_H
#define SERVICE_TREE_H

//Type declarations
typedef struct service_tree_node service_tree_node_t;

typedef struct service_node_data {
    char *sub_uri;
    void *service;
} service_node_data_t;


struct service_tree_node {
    service_node_data_t *svc_data;
    service_tree_node_t *next;
    service_tree_node_t *prev;
    service_tree_node_t *parent;
    service_tree_node_t *children;
};

typedef struct service_tree {
    int tree_svc_count;             //Count for number of services in the tree (not number of nodes)
    int tree_node_count;            //Count for number of nodes (tree_node_count != tree_svc_count)
    service_tree_node_t *root_node; //Pointer to first item
} service_tree_t;

//Global function prototypes
bool addServiceNode(service_tree_t *svc_tree, const char *uri, void *svc);
void destroyChildrenFromServiceNode(service_tree_node_t *parent, int *tree_item_count, int *tree_svc_count);
void destroyServiceNode(service_tree_t *svc_tree, service_tree_node_t *node, int *tree_item_count, int *tree_svc_count);
void destroyServiceTree(service_tree_t *svc_tree);
service_tree_node_t *findServiceNodeInTree(service_tree_t *svc_tree, const char *uri);

#endif //SERVICE_TREE_H
