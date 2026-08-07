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
#include "celix_string_format_check.h"
#include "celix_smtp_address_validator.h"
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── RFC 3339 Date-Time ────────────────────────────────────────────────── */

static int is_leap_year(int year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }

static int days_in_month(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    /* Caller (check_date_part) guarantees 1 <= month <= 12 */
    assert(month >= 1 && month <= 12);
    if (month == 2 && is_leap_year(year))
        return 29;
    return days[month - 1];
}

static int check_date_part(const char* s) {
    /* YYYY-MM-DD — must be exactly 10 chars with zero-padded month/day */
    if (!s || strlen(s) != 10)
        return -1;
    if (s[4] != '-' || s[7] != '-')
        return -1;
    /* Verify all other chars are digits */
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7)
            continue;
        if (!isdigit((unsigned char)s[i]))
            return -1;
    }
    int year, month, day;
    /* All non-separator chars were validated as digits above, so the parse always succeeds */
    int n = sscanf(s, "%4d-%2d-%2d", &year, &month, &day);
    assert(n == 3);
    (void)n;
    if (month < 1 || month > 12)
        return -1;
    if (day < 1 || day > days_in_month(year, month))
        return -1;
    return 0;
}

static int parse_time_part(const char* s, int* out_hour, int* out_min, int* out_sec) {
    /* HH:MM:SS[.fraction] — parse components, return 0 on success */
    int hour, min, sec;
    double frac = 0.0;
    int n = 0;

    if (strchr(s, '.')) {
        n = sscanf(s, "%2d:%2d:%2d.%lf", &hour, &min, &sec, &frac);
    } else {
        n = sscanf(s, "%2d:%2d:%2d", &hour, &min, &sec);
    }

    if (n < 3)
        return -1;
    if (hour < 0 || hour > 23)
        return -1;
    if (min < 0 || min > 59)
        return -1;
    if (sec < 0 || sec > 60)
        return -1;

    *out_hour = hour;
    *out_min = min;
    *out_sec = sec;
    return 0;
}

static int check_leap_second(int hour, int min, int sec, int offset_hour, int offset_min) {
    /* Per RFC 3339, a leap second (sec == 60) is only valid when the
     * UTC-normalized time equals 23:59:60.  Normalize local time to UTC
     * day-minutes and check. */
    if (sec != 60)
        return 0;

    int day_minutes = hour * 60 + min - (offset_hour * 60 + offset_min);
    /* Wrap into [0, 1440) */
    while (day_minutes < 0)
        day_minutes += 24 * 60;
    day_minutes %= (24 * 60);

    int utc_hour = day_minutes / 60;
    int utc_min = day_minutes % 60;
    return (utc_hour == 23 && utc_min == 59) ? 0 : -1;
}

static int parse_timezone(const char* s, int* out_hh, int* out_mm) {
    /* Callers pass the result of strpbrk(s, "+-Zz"), so s is non-NULL and non-empty */
    assert(s != NULL && *s != '\0');
    if (*s == 'Z' || *s == 'z') {
        *out_hh = 0;
        *out_mm = 0;
        return 0;
    }
    /* ±HH:MM — s[0] is '+' or '-' (Z/z handled above) */
    int hh, mm;
    char sign;
    if (sscanf(s, "%c%2d:%2d", &sign, &hh, &mm) != 3)
        return -1;
    assert(sign == '+' || sign == '-');
    if (hh < 0 || hh > 23)
        return -1;
    if (mm < 0 || mm > 59)
        return -1;
    if (sign == '-') {
        hh = -hh;
        mm = -mm;
    }
    *out_hh = hh;
    *out_mm = mm;
    return 0;
}

int celix_jansson_check_date_time(const char* value) {
    /* Format: YYYY-MM-DD T HH:MM:SS[.fraction] (Z|±HH:MM) */
    const char* t = strpbrk(value, "Tt");
    if (!t)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    size_t date_len = (size_t)(t - value);
    if (date_len >= 16)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    /* Check date part */
    char date_part[16] = {0};
    memcpy(date_part, value, date_len);
    if (check_date_part(date_part) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    /* Check time part + timezone */
    const char* time_str = t + 1;
    const char* plus = strpbrk(time_str, "+-Zz");
    if (!plus) {
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    size_t time_only_len = (size_t)(plus - time_str);
    if (time_only_len == 0 || time_only_len >= 32)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    char time_part[33] = {0};
    memcpy(time_part, time_str, time_only_len);

    int h, m, s;
    if (parse_time_part(time_part, &h, &m, &s) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    int off_h = 0, off_m = 0;
    if (parse_timezone(plus, &off_h, &off_m) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    /* Reject trailing garbage after timezone */
    if (*plus == 'Z' || *plus == 'z') {
        if (plus[1] != '\0') return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    } else {
        if (plus[6] != '\0') return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    if (check_leap_second(h, m, s, off_h, off_m) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    return CELIX_JANSSON_SCHEMA_OK;
}

int celix_jansson_check_date(const char* value) {
    /* YYYY-MM-DD */
    if (strlen(value) < 10)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    return check_date_part(value) == 0 ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
}

int celix_jansson_check_time(const char* value) {
    /* HH:MM:SS[.fraction](Z|±HH:MM) */
    const char* plus = strpbrk(value, "+-Zz");
    size_t time_len = plus ? (size_t)(plus - value) : strlen(value);

    if (time_len == 0 || time_len > 32)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    char time_part[33] = {0};
    memcpy(time_part, value, time_len);

    int h, m, s;
    if (parse_time_part(time_part, &h, &m, &s) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    int off_h = 0, off_m = 0;
    if (plus) {
        if (parse_timezone(plus, &off_h, &off_m) != 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        /* Reject trailing garbage after timezone (e.g., "Z+00:30") */
        if (*plus == 'Z' || *plus == 'z') {
            if (plus[1] != '\0')
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        } else {
            /* ±HH:MM — 6 chars total */
            if (plus[6] != '\0')
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        }
    } else {
        /* No timezone — invalid per RFC 3339 */
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    if (check_leap_second(h, m, s, off_h, off_m) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Email ─────────────────────────────────────────────────────────────── */

static bool is_ascii(const char* s) {
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        if (*p > 127)
            return false;
    }
    return true;
}

int celix_jansson_check_email(const char* value) {
    if (!is_ascii(value))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    if (!celix_jansson_smtp_is_address(value, value + strlen(value)))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    return CELIX_JANSSON_SCHEMA_OK;
}

int celix_jansson_check_idn_email(const char* value) {
    if (!celix_jansson_smtp_is_address(value, value + strlen(value)))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Hostname ──────────────────────────────────────────────────────────── */

int celix_jansson_check_hostname(const char* value) {
    if (!value || *value == '\0')
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    const char* label_start = value;
    size_t total_len = strlen(value);

    if (total_len > 253)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    while (*label_start) {
        /* Find end of label */
        const char* dot = strchr(label_start, '.');
        if (!dot)
            dot = label_start + strlen(label_start);
        size_t label_len = (size_t)(dot - label_start);

        if (label_len == 0 || label_len > 63)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

        /* First char must be alphanumeric, last must be alphanumeric */
        if (!isalnum((unsigned char)label_start[0]))
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        if (!isalnum((unsigned char)dot[-1]))
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

        /* Middle chars: alphanumeric or hyphen */
        for (const char* p = label_start + 1; p < dot - 1; p++) {
            if (!isalnum((unsigned char)*p) && *p != '-')
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        }

        if (*dot == '\0')
            break;
        label_start = dot + 1;
    }

    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── IPv4 ──────────────────────────────────────────────────────────────── */

int celix_jansson_check_ipv4(const char* value) {
    unsigned int a, b, c, d;
    char tail;
    if (sscanf(value, "%3u.%3u.%3u.%3u%c", &a, &b, &c, &d, &tail) != 4)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    if (a > 255 || b > 255 || c > 255 || d > 255)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    /* Reject leading zeros (e.g., "192.168.01.001") */
    char octet[16];
    /* a,b,c,d <= 255 was validated above, so the formatted string always fits in octet */
    int n = snprintf(octet, sizeof(octet), "%u.%u.%u.%u", a, b, c, d);
    assert(n < (int)sizeof(octet));
    (void)n;
    if (strcmp(value, octet) != 0)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── IPv6 ──────────────────────────────────────────────────────────────── */

int celix_jansson_check_ipv6(const char* value) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, value, &addr) != 1)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    /* Reject zone-id forms (%eth0 etc.) */
    if (strchr(value, '%'))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── URI (absolute) ────────────────────────────────────────────────────── */

/* RFC 3986 §2.3: unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~" */
static bool uri_is_unreserved(int c) {
    return isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/* RFC 3986 §2.2: sub-delims = "!" / "$" / "&" / "'" / "(" / ")"
 *                           / "*" / "+" / "," / ";" / "=" */
static bool uri_is_sub_delims(int c) {
    return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
           c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=';
}

/* Validate percent-encoded octet: % HEXDIG HEXDIG.  Returns 3 (bytes
 * consumed) on success, -1 on failure. */
static int uri_check_pct_encoded(const char* p) {
    if (p[0] != '%' || !isxdigit((unsigned char)p[1]) || !isxdigit((unsigned char)p[2]))
        return -1;
    return 3;
}

/* Validate a path/query/fragment segment character-by-character.
 * @p extra_chars  additional allowed characters beyond pchar (e.g., "/?" for query).
 * Returns 0 if all characters up to @p stop_chars are valid, -1 on invalid char. */
static int uri_validate_segment(const char** pp, const char* stop_chars, const char* extra_chars) {
    const char* p = *pp;
    while (*p && !strchr(stop_chars, *p)) {
        if (*p == '%') {
            int n = uri_check_pct_encoded(p);
            if (n < 0) return -1;
            p += n; continue;
        }
        if (uri_is_unreserved(*p) || uri_is_sub_delims(*p) ||
            *p == ':' || *p == '@' || *p == '/')
            { p++; continue; }
        /* Check extra chars (e.g., '?' for query/fragment) */
        if (extra_chars && strchr(extra_chars, *p))
            { p++; continue; }
        return -1;
    }
    *pp = p;
    return 0;
}

int celix_jansson_check_uri(const char* value) {
    /* Hand-rolled RFC 3986 absolute URI parser */
    if (!value || *value == '\0')
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    const char* p = value;

    /* Scheme: ALPHA *(ALPHA / DIGIT / "+" / "-" / ".") */
    if (!isalpha((unsigned char)*p))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    while (isalnum((unsigned char)*p) || *p == '+' || *p == '-' || *p == '.')
        p++;

    /* Must have ':' after scheme */
    if (p[0] != ':')
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    p++; /* skip ':' */

    /* hier-part: "//" authority path-abempty / path-* */
    bool has_authority = (p[0] == '/' && p[1] == '/');
    const char* auth_end = p;

    if (has_authority) {
        p += 2; /* skip "//" */
        auth_end = strpbrk(p, "/?#");
        if (!auth_end)
            auth_end = p + strlen(p);

        /* Basic authority validation */
        if (auth_end > p) {
            /* Host part is present */
            const char* at = memchr(p, '@', (size_t)(auth_end - p));
            if (at) {
                /* Skip userinfo */
                p = at + 1;
            }

            size_t host_len = (size_t)(auth_end - p);
            /* Allow IPv6 literal [...] */
            if (*p == '[') {
                const char* bracket = memchr(p, ']', host_len);
                if (!bracket)
                    return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
                /* Validate inner IPv6 */
                size_t ip6_len = (size_t)(bracket - p - 1);
                if (ip6_len > 0) {
                    /* Basic check — skip full validation */
                }
                p = bracket + 1;
                /* Optional port */
                if (p < auth_end && *p == ':')
                    p = auth_end;
            } else {
                /* reg-name or IPv4: validate characters */
                const char* colon = memchr(p, ':', host_len);
                const char* host_part_end = colon ? colon : auth_end;
                for (const char* c = p; c < host_part_end; c++) {
                    if (*c == '%') {
                        int n = uri_check_pct_encoded(c);
                        if (n < 0) return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
                        c += 2; continue;
                    }
                    if (uri_is_unreserved(*c) || uri_is_sub_delims(*c))
                        continue;
                    return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
                }
                p = host_part_end;
                if (colon) {
                    /* Port must be digits */
                    for (const char* c = colon + 1; c < auth_end; c++) {
                        if (!isdigit((unsigned char)*c))
                            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
                    }
                    p = auth_end;
                }
            }
        }
        p = auth_end;
    }

    /* Path (optional): segment *( "/" segment )
     * For authority-based URIs this starts with '/'.
     * For scheme-only URIs (mailto:, tel:, news:) the rest is a path-rootless. */
    if (*p == '/' || (!has_authority && *p != '?' && *p != '#' && *p != '\0')) {
        if (uri_validate_segment(&p, "?#", NULL) != 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    /* Query (optional): ? *( pchar / "/" / "?" ) */
    if (*p == '?') {
        p++;
        if (uri_validate_segment(&p, "#", "?") != 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    /* Fragment (optional): # *( pchar / "/" / "?" ) */
    if (*p == '#') {
        p++;
        if (uri_validate_segment(&p, "", "?") != 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── UUID ──────────────────────────────────────────────────────────────── */

int celix_jansson_check_uuid(const char* value) {
    /* 8-4-4-4-12 hex pattern: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    if (strlen(value) != 36)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (value[i] != '-')
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        } else {
            if (!isxdigit((unsigned char)value[i]))
                return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        }
    }
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Regex ─────────────────────────────────────────────────────────────── */

int celix_jansson_check_regex(const char* value) {
    regex_t re;
    int rc = regcomp(&re, value, REG_EXTENDED | REG_NOSUB);
    if (rc != 0) {
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }
    regfree(&re);
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Main dispatch ─────────────────────────────────────────────────────── */

int celix_jansson_schema_default_format_check(const char* format, const char* value, void* user_data) {
    (void)user_data;
    if (!format || !value)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;

    if (strcmp(format, "date-time") == 0)
        return celix_jansson_check_date_time(value);
    if (strcmp(format, "date") == 0)
        return celix_jansson_check_date(value);
    if (strcmp(format, "time") == 0)
        return celix_jansson_check_time(value);
    if (strcmp(format, "email") == 0)
        return celix_jansson_check_email(value);
    if (strcmp(format, "idn-email") == 0)
        return celix_jansson_check_idn_email(value);
    if (strcmp(format, "hostname") == 0)
        return celix_jansson_check_hostname(value);
    if (strcmp(format, "ipv4") == 0)
        return celix_jansson_check_ipv4(value);
    if (strcmp(format, "ipv6") == 0)
        return celix_jansson_check_ipv6(value);
    if (strcmp(format, "uri") == 0)
        return celix_jansson_check_uri(value);
    if (strcmp(format, "uuid") == 0)
        return celix_jansson_check_uuid(value);
    if (strcmp(format, "regex") == 0)
        return celix_jansson_check_regex(value);

    /* Known but unsupported draft-7 formats */
    static const char* unsupported[] = {"idn-hostname",
                                        "uri-reference",
                                        "iri",
                                        "iri-reference",
                                        "uri-template",
                                        "json-pointer",
                                        "relative-json-pointer",
                                        NULL};
    for (const char** u = unsupported; *u; u++) {
        if (strcmp(format, *u) == 0)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    /* Unknown format */
    return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
}
