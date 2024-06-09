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
#include "mdnsresponder_ei.h"
#include "dns_sd.h"
#include "celix_error_injector.h"

extern "C" {
DNSServiceErrorType __real_DNSServiceCreateConnection(DNSServiceRef *__sdRef);
CELIX_EI_DEFINE(DNSServiceCreateConnection, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceCreateConnection(DNSServiceRef *__sdRef) {
    CELIX_EI_IMPL(DNSServiceCreateConnection);
    return __real_DNSServiceCreateConnection(__sdRef);
}

DNSServiceErrorType __real_DNSServiceProcessResult(DNSServiceRef __sdRef);
CELIX_EI_DEFINE(DNSServiceProcessResult, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceProcessResult(DNSServiceRef __sdRef) {
    CELIX_EI_IMPL(DNSServiceProcessResult);
    return __real_DNSServiceProcessResult(__sdRef);
}

DNSServiceErrorType __real_DNSServiceRegister(DNSServiceRef *__sdRef, DNSServiceFlags __flags, uint32_t __interfaceIndex, const char *__name, const char *__regtype, const char *__domain, const char *__host, uint16_t __port, uint16_t __txtLen, const void *__txtRecord, DNSServiceRegisterReply __callBack, void *__context);
CELIX_EI_DEFINE(DNSServiceRegister, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceRegister(DNSServiceRef *__sdRef, DNSServiceFlags __flags, uint32_t __interfaceIndex, const char *__name, const char *__regtype, const char *__domain, const char *__host, uint16_t __port, uint16_t __txtLen, const void *__txtRecord, DNSServiceRegisterReply __callBack, void *__context) {
    CELIX_EI_IMPL(DNSServiceRegister);
    return __real_DNSServiceRegister(__sdRef, __flags, __interfaceIndex, __name, __regtype, __domain, __host, __port, __txtLen, __txtRecord, __callBack, __context);
}

DNSServiceErrorType __real_TXTRecordSetValue(TXTRecordRef *__txtRec, const char *__key, uint8_t __valSize, const void *__val);
CELIX_EI_DEFINE(TXTRecordSetValue, DNSServiceErrorType)
DNSServiceErrorType __wrap_TXTRecordSetValue(TXTRecordRef *__txtRec, const char *__key, uint8_t __valSize, const void *__val) {
    CELIX_EI_IMPL(TXTRecordSetValue);
    return __real_TXTRecordSetValue(__txtRec, __key, __valSize, __val);
}

DNSServiceErrorType __real_DNSServiceBrowse(DNSServiceRef *__sdRef, DNSServiceFlags __flags, uint32_t __interfaceIndex,  const char *__regtype, const char *__domain, DNSServiceBrowseReply __callBack, void *__context);
CELIX_EI_DEFINE(DNSServiceBrowse, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceBrowse(DNSServiceRef *__sdRef, DNSServiceFlags __flags, uint32_t __interfaceIndex, const char *__regtype, const char *__domain, DNSServiceBrowseReply __callBack, void *__context) {
    CELIX_EI_IMPL(DNSServiceBrowse);
    return __real_DNSServiceBrowse(__sdRef, __flags, __interfaceIndex, __regtype, __domain, __callBack, __context);
}

DNSServiceErrorType __real_DNSServiceResolve(DNSServiceRef *sdRef,DNSServiceFlags flags,uint32_t interfaceIndex,const char *name,const char *regtype,const char *domain,DNSServiceResolveReply callBack,void *context);
CELIX_EI_DEFINE(DNSServiceResolve, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceResolve(DNSServiceRef *sdRef,DNSServiceFlags flags,uint32_t interfaceIndex,const char *name,const char *regtype,const char *domain,DNSServiceResolveReply callBack,void *context) {
    CELIX_EI_IMPL(DNSServiceResolve);
    return __real_DNSServiceResolve(sdRef, flags, interfaceIndex, name, regtype, domain, callBack, context);
}

DNSServiceErrorType __real_DNSServiceGetAddrInfo(DNSServiceRef *sdRef,DNSServiceFlags flags,uint32_t interfaceIndex,DNSServiceProtocol protocol,const char *hostname,DNSServiceGetAddrInfoReply callBack,void *context);
CELIX_EI_DEFINE(DNSServiceGetAddrInfo, DNSServiceErrorType)
DNSServiceErrorType __wrap_DNSServiceGetAddrInfo(DNSServiceRef *sdRef,DNSServiceFlags flags,uint32_t interfaceIndex,DNSServiceProtocol protocol,const char *hostname,DNSServiceGetAddrInfoReply callBack,void *context) {
    CELIX_EI_IMPL(DNSServiceGetAddrInfo);
    return __real_DNSServiceGetAddrInfo(sdRef, flags, interfaceIndex, protocol, hostname, callBack, context);
}


}