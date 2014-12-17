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
 * dm_component_state_listener.h
 *
 *  \date       22 Feb 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef DM_COMPONENT_STATE_LISTENER_H_
#define DM_COMPONENT_STATE_LISTENER_H_

enum dm_component_state {
    DM_INACTIVE,
    DM_WAITING_FOR_REQUIRED,
    DM_INSTANTIATED_AND_WAITING_FOR_REQUIRED,
    DM_TRACKING_OPTIONAL,
};



#endif /* DM_COMPONENT_STATE_LISTENER_H_ */
