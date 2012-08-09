# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

In the log server a listener registers when the log_service bundle is stopped and 
started by means of the serviceChanged() routine. This routine maintains the set of log
services in the variable m_logServiceRefs. The function getLogService() returns the
current LOG_SERVICE, if it is active or NULL.
Starting and stopping the log_service bundle shows that the logServiceTest thread 
correctly recognizes the non-availability of the log_services, when stopped.
