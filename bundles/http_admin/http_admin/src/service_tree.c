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

#include <stdlib.h>
#include <stdbool.h> //for `bool`
#include <stdio.h>   //for `asprintf`
#include <memory.h>  //for `strcmp` and `strtok_r`

#include "service_tree.h"

//Local function prototypes
static service_tree_node_t *createServiceNode(service_tree_node_t *parent, service_tree_node_t *children, service_tree_node_t *next, service_tree_node_t *prev, const char *uri, void *svc);
static size_t getUriTokenCount(const char *uri, const char *separator);



static service_tree_node_t *createServiceNode(service_tree_node_t *parent, service_tree_node_t *children, service_tree_node_t *next, service_tree_node_t *prev, const char *uri, void *svc) {
    service_tree_node_t *node = calloc(1, sizeof(service_tree_node_t));
    node->parent = parent;
    node->children = children;
    node->next = next;
    node->prev = prev;
    node->svc_data = calloc(1, sizeof(service_node_data_t));
    node->svc_data->sub_uri = (uri != NULL ? strdup(uri) : NULL);
    node->svc_data->service = svc;
    return node;
}

static size_t getUriTokenCount(const char *uri, const char *separator) {
    char *saveptr, *token;
    char *uri_cpy = strdup(uri);
    size_t tokenCount = 0;

    token = strtok_r(uri_cpy, separator, &saveptr);
    while(token != NULL) {
        tokenCount++;
        token = strtok_r(NULL, separator, &saveptr);
    }

    free(uri_cpy);
    return tokenCount;
}

bool addServiceNode(service_tree_t *svc_tree, const char *uri, void *svc) {
    char *save_ptr = NULL; //Used internally by strtok_r
    char *uri_cpy = NULL;
    char *req_uri = NULL;
    bool uri_exists = true;
    size_t tokenCount = getUriTokenCount(uri, "/");
    size_t subNodeCount = 1;

    if(svc_tree == NULL || uri == NULL){
        return false;
    }

    if(strcmp(uri, "/") == 0 ) {
        //Create the root URI
        if(svc_tree->root_node == NULL) { //No service yet added
            svc_tree->root_node = createServiceNode(NULL, NULL, NULL, NULL, "root", svc);
            svc_tree->tree_node_count = 1;
            uri_exists = false;
        } else if(strcmp(svc_tree->root_node->svc_data->sub_uri, "root") != 0) {
            service_tree_node_t *node = createServiceNode(NULL, svc_tree->root_node, NULL, NULL, "root", svc);
            //Set this new node as parent for the first line of nodes
            service_tree_node_t *tmp = svc_tree->root_node;
            while(tmp != NULL){
                tmp->parent = node;
                tmp = tmp->next;
            }
            svc_tree->root_node->parent = node;
            svc_tree->root_node = node; //Set new root
            svc_tree->tree_node_count++;
            uri_exists = false;
        }
        //Else root already exists
    } else if(svc_tree->root_node == NULL) { //No service yet added
        uri_cpy = strdup(uri);
        if (uri_cpy == NULL) { // Check for memory allocation failure
            return false;
        }
        req_uri = strtok_r(uri_cpy, "/", &save_ptr);
        svc_tree->root_node = createServiceNode(NULL, NULL, NULL, NULL, req_uri, (tokenCount == 1 ? svc : NULL));
        svc_tree->tree_node_count = 1;
        uri_exists = false;
    } else if(strcmp(svc_tree->root_node->svc_data->sub_uri, "root") == 0){
        if (asprintf(&uri_cpy, "%s%s", "root", uri)<0) {
            return false;
        }
        req_uri = strtok_r(uri_cpy, "/", &save_ptr);
    } else {
        uri_cpy = strdup(uri);
        if (uri_cpy == NULL) { // Check for memory allocation failure
            return false;
        }
        req_uri = strtok_r(uri_cpy, "/", &save_ptr);
    }

    service_tree_node_t *current = svc_tree->root_node;
    service_node_data_t *current_data = current->svc_data;
    while (req_uri != NULL && subNodeCount <= tokenCount) {
        bool is_last_entry = (tokenCount == subNodeCount);
        if (strcmp(current_data->sub_uri, req_uri) == 0) {
            if (is_last_entry) {
                //Entry already exists/added in tree
                if (current_data->service == NULL) {
                    //When no service added yet, add the current. Because there can already be sub URIs registered
                    //as children for this URI. No extra node is added, because the node already exists
                    current_data->service = svc;
                    uri_exists = false;
                }
                req_uri = strtok_r(NULL, "/", &save_ptr);
            } else if (current->children != NULL) {
                //Found the parent of the URI, keep searching in the children for sub URIs
                current = current->children;
                current_data = current->svc_data;
                req_uri = strtok_r(NULL, "/", &save_ptr);
            } else {
                //Parent has no sub URIs registered yet
                req_uri = strtok_r(NULL, "/", &save_ptr);
                //Create svc node without svc pointer, this will be added later if needed.
                service_tree_node_t *node = createServiceNode(current, NULL, NULL, NULL, req_uri, NULL);
                current->children = node;
                current = node;
                current_data = node->svc_data;
                svc_tree->tree_node_count++;
                uri_exists = false;
            }
            subNodeCount++;
        } else if (current->next != NULL) {
            current = current->next;
            current_data = current->svc_data;
        } else {
            //Not found, so create this URI (tree)
            service_tree_node_t *node = createServiceNode(current->parent, NULL, NULL, current,
                                                          req_uri, (is_last_entry ? svc : NULL));
            current->next = node;
            current = node;
            current_data = node->svc_data;
            svc_tree->tree_node_count++;
            uri_exists = false;
            subNodeCount++;
        }
    }

    //Increment tree service count if URI exists (only one service can be added at once)
    if(!uri_exists) {
        svc_tree->tree_svc_count++;
    }

    if(uri_cpy) {
        free(uri_cpy);
    }

    return !uri_exists;
}

void destroyChildrenFromServiceNode(service_tree_node_t *parent, int *tree_item_count, int *tree_svc_count) {
    if(parent != NULL && tree_item_count != NULL && tree_svc_count != NULL){
        service_tree_node_t *child = parent->children;
        while(child != NULL) {
            if (child->children != NULL) {
                //Delete children first
                destroyChildrenFromServiceNode(child, tree_item_count, tree_svc_count);

            } else {
                //Delete child first
                service_tree_node_t *next_child = child->next;

                if(child->svc_data != NULL) {
                    //Decrement service count if a service was present
                    if(child->svc_data->service != NULL) (*tree_svc_count)--;
                    free(child->svc_data->sub_uri);
                    free(child->svc_data);
                }
                free(child);
                (*tree_item_count)--;
                child = next_child;
            }
        }
        parent->children = NULL;
    }
}

void destroyServiceNode(service_tree_t *svc_tree, service_tree_node_t *node, int *tree_item_count, int *tree_svc_count) {
    if(node != NULL && tree_item_count != NULL && tree_svc_count != NULL) {
        bool has_underlaying_services = false;

        //If this service node has children then check if we need to destroy them first
        if(node->children != NULL) {
            service_tree_node_t *node_cpy = node;
            while(node_cpy->children != NULL && has_underlaying_services == false) {
                service_tree_node_t *node_cpy_horizontal = node_cpy->children;
                has_underlaying_services |= (node_cpy_horizontal->svc_data->service != NULL);
                while(node_cpy_horizontal->next != NULL && has_underlaying_services == false) {
                    has_underlaying_services |= (node_cpy_horizontal->svc_data->service != NULL);
                    node_cpy_horizontal = node_cpy_horizontal->next;
                }
                node_cpy = node_cpy->children;
            }

            //No underlaying services means we can delete all children
            if(!has_underlaying_services) {
                destroyChildrenFromServiceNode(node, tree_item_count, tree_svc_count);
            }
        }

        //Decrement service count if a service was present
        if(node->svc_data->service != NULL) {
            node->svc_data->service = NULL;
            (*tree_svc_count)--;
        }

        //When no underlaying services found, we have to take care of some pointers and memory...
        if(!has_underlaying_services){
            //Set new children pointer for parent when this is the first child in the tree.
            //When no next is present the children pointer should become NULL.
            if(node->parent != NULL && node->prev == NULL){
                node->parent->children = node->next;
            }

            //If current node to delete is the root node, set new root node pointer
            if(svc_tree->root_node == node) {
                svc_tree->root_node = node->next;
            }

            //Set new previous pointer for the next node if present.
            if(node->next != NULL) {
                node->next->prev = node->prev;
            }

            //Set new next pointer if a previous is present
            if(node->prev != NULL) {
                node->prev->next = node->next;
            }

            free(node->svc_data->sub_uri);
            free(node->svc_data);
            free(node);
            (*tree_item_count)--;
        }
    }
}

void destroyServiceTree(service_tree_t *svc_tree) {
    if(svc_tree != NULL) {
        if(svc_tree->tree_node_count > 0 && svc_tree->root_node != NULL) {
            service_tree_node_t *current = svc_tree->root_node;

            while(current != NULL) {
                destroyChildrenFromServiceNode(current, &svc_tree->tree_node_count, &svc_tree->tree_svc_count);

                service_tree_node_t *next = current->next;
                if(current->svc_data != NULL) {
                    if(current->svc_data->service != NULL) svc_tree->tree_svc_count--;
                    free(current->svc_data->sub_uri);
                    free(current->svc_data);
                }
                free(current);

                //Iterate to next node in similar level as root node
                current = next;
            }

            svc_tree->tree_node_count = 0;
            svc_tree->tree_svc_count = 0;
            svc_tree->root_node = NULL;
        }
    }
}


service_tree_node_t *findServiceNodeInTree(service_tree_t *svc_tree, const char *uri) {
    service_tree_node_t *found_node = NULL;
    service_tree_node_t *current = NULL;
    char *save_ptr; //Pointer used internally by strtok_r to point to next token
    char *uri_cpy;
    bool tree_not_empty = false;

    if (svc_tree != NULL && uri != NULL) {
        tree_not_empty = ((bool) (svc_tree->tree_svc_count | svc_tree->tree_node_count)
                               & (svc_tree->root_node != NULL));
        current = svc_tree->root_node;
    }

    if (tree_not_empty) {
        if(strcmp(current->svc_data->sub_uri, "root") == 0)
        {
            if (asprintf(&uri_cpy, "%s%s", "root", uri)<0)
            {
                return NULL;
            }
        } else {
            uri_cpy = strdup(uri);
            if (uri_cpy == NULL) {  // Check for strdup failure
                return NULL;
            }
        }

        char *uri_token = strtok_r(uri_cpy, "/", &save_ptr);
        //Check for horizontal matches for the first token
        while (current != NULL) {
            if (uri_token == NULL) {
                current = NULL;
            } else if (strcmp(current->svc_data->sub_uri, uri_token) == 0){
                //Save current node to comply with OSGI Http Whiteboard Specification
                if (current->svc_data->service != NULL) {
                    found_node = current;
                }
                break; //Break out of while loop, keep 'current' pointer
            } else {
                current = current->next;
            }
        }

        //Check also in vertical direction for the remaining tokens
        if (current != NULL) {
            uri_token = strtok_r(NULL, "/", &save_ptr);
            if (uri_token != NULL && current->children != NULL){
                current = current->children;
                while (current != NULL) {
                    //Match for current sub URI with URI token
                    if (current->svc_data->sub_uri != NULL && strcmp(current->svc_data->sub_uri, uri_token) == 0) {
                        //Save current node to comply with OSGI Http Whiteboard Specification
                        if (current->svc_data->service != NULL) {
                            found_node = current;
                        }

                        uri_token = strtok_r(NULL, "/", &save_ptr);
                        if (uri_token != NULL && current->children != NULL) {
                            current = current->children;
                        }
                        else {
                            break; //Break out of while loop since URI token can still have a value
                        }
                    } else {
                        current = current->next;
                    }
                }
                //No more tokens left, save this node when a service is present
            } else if (uri_token == NULL && current->svc_data->service != NULL) {
                found_node = current;
            }
            //Else we haven't found a node that complies with the requested URI...
        }

        free(uri_cpy);
    }

    return found_node;
}
