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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef CELIX_CELIX_STRING_FORMAT_CHECK_H
#define CELIX_CELIX_STRING_FORMAT_CHECK_H

#include "celix_jansson_schema.h"

/** RFC 3339 date-time validator. Returns CELIX_JANSSON_SCHEMA_OK or error code. */
int celix_jansson_check_date_time(const char* value);

/** RFC 3339 date validator. */
int celix_jansson_check_date(const char* value);

/** RFC 3339 time validator. */
int celix_jansson_check_time(const char* value);

/** RFC 5321 email address validator. Returns CELIX_JANSSON_SCHEMA_OK or error code. */
int celix_jansson_check_email(const char* value);

/** RFC 6531 international email address validator (no ASCII restriction). */
int celix_jansson_check_idn_email(const char* value);

/** Hostname (DNS label) validator per RFC 3986 Appendix A. */
int celix_jansson_check_hostname(const char* value);

/** IPv4 address validator. */
int celix_jansson_check_ipv4(const char* value);

/** IPv6 address validator. */
int celix_jansson_check_ipv6(const char* value);

/** Absolute URI validator per RFC 3986. */
int celix_jansson_check_uri(const char* value);

/** UUID validator per RFC 4122 (8-4-4-4-12 hex pattern). */
int celix_jansson_check_uuid(const char* value);

/** ECMAScript regex validator — attempts to compile the string as a regex. */
int celix_jansson_check_regex(const char* value);

#endif /* CELIX_CELIX_STRING_FORMAT_CHECK_H */
