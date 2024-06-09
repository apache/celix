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

#ifndef CELIX_MDNSRESPONDER_EI_H
#define CELIX_MDNSRESPONDER_EI_H
#ifdef __cplusplus
extern "C" {
#endif
#include "dns_sd.h"
#include "celix_error_injector.h"

CELIX_EI_DECLARE(DNSServiceCreateConnection, DNSServiceErrorType);
CELIX_EI_DECLARE(DNSServiceProcessResult, DNSServiceErrorType);
CELIX_EI_DECLARE(DNSServiceRegister, DNSServiceErrorType);
CELIX_EI_DECLARE(TXTRecordSetValue, DNSServiceErrorType);
CELIX_EI_DECLARE(DNSServiceBrowse, DNSServiceErrorType);
CELIX_EI_DECLARE(DNSServiceResolve, DNSServiceErrorType);
CELIX_EI_DECLARE(DNSServiceGetAddrInfo, DNSServiceErrorType);

#ifdef __cplusplus
}
#endif

#endif //CELIX_MDNSRESPONDER_EI_H
