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
 * array_list_private.h
 *
 *  \date       Aug 4, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef array_list_t_PRIVATE_H_
#define array_list_t_PRIVATE_H_

struct celix_array_list {
    celix_array_list_entry_t* elementData;
    size_t size;
    size_t capacity;
    unsigned int modCount;
    celix_arrayList_equals_fp  equals;
    void (*simpleRemovedCallback)(void* value);
    void* removedCallbackData;
    void (*removedCallback)(void* data, celix_array_list_entry_t entry);
};

#endif /* array_list_t_PRIVATE_H_ */
