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
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

add_library(mdnsresponder_ei STATIC src/mdnsresponder_ei.cc)

find_package(DNSSD)

target_include_directories(mdnsresponder_ei PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(mdnsresponder_ei PUBLIC Celix::error_injector DNSSD::DNSSD)

target_link_options(mdnsresponder_ei INTERFACE LINKER:--wrap,DNSServiceCreateConnection LINKER:--wrap,DNSServiceProcessResult LINKER:--wrap,DNSServiceRegister LINKER:--wrap,TXTRecordSetValue LINKER:--wrap,DNSServiceBrowse LINKER:--wrap,DNSServiceResolve LINKER:--wrap,DNSServiceGetAddrInfo)
add_library(Celix::mdnsresponder_ei ALIAS mdnsresponder_ei)
