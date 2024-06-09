---
title: Apache Celix Properties Encoding
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# Apache Celix Properties JSON Encoding

## Introduction

In Apache Celix, properties represent key-value pairs, often used for configuration. While these properties are not JSON
objects inherently, they can be encoded to and decoded from JSON for interoperability or storage. This page explains how
Apache Celix properties are encoded to and decoded from JSON.

### Encoding limitations

Except for empty arrays and the double values NaN, Infinity, and -Infinity, all Apache Celix properties types can
be encoded to JSON.

The reason for the empty array limitation is that for a properties array entry the array list element type is must be
known, this is not possible to infer from an empty JSON array. To ensure that everything this is encoded, can be decoded
again, a properties array entry with an empty array is not encoded to JSON.

The reason for the double values NaN, Infinity, and -Infinity limitation is that JSON does not support these values.

### Decoding limitations

When decoding JSON to Apache Celix properties, the following limitations apply:

- Mixed array types are not supported. For example, an array with both strings and longs cannot be decoded to a
  properties' entry.
- null values are not supported, because properties does not support a null value type.
- Empty arrays are not supported, because the array list element type must be known, this is not possible to infer from
  an empty JSON array.
- JSON keys that collide on the created properties' key level are not supported.
  See [Properties Decoding](##Properties Decoding) for more information.

## Properties Encoding

Apache Celix properties can be encoded to JSON using the `celix_properties_save`, `celix_properties_saveToStream`
and `celix_properties_saveToString` functions. These functions take a properties object and encode it to a JSON object
string. The encoding can be controlled using flags and can be done in a flat or nested structure.

### Properties Flat Encoding

By default, the encoding is done in a flat structure, because a flat structure ensures that all keys of the properties
can be represented in JSON format. When properties are encoded to JSON in a flat structure, the reverse operation,
decoding JSON that has been encoded from properties, will result in the same properties (except for the previously
mentioned limitations (empty arrays and the double values NaN, Infinity, and -Infinity)).

Flat Encoding example:

```C
#include <stdio.h>
#include <celix/properties.h>

int main() {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_properties_set(props, "single/strKey", "strValue");
    celix_properties_setLong(props, "single/longKey", 42);
    celix_properties_setDouble(props, "single/doubleKey", 2.0);
    celix_properties_setBool(props, "single/boolKey", true);
    celix_properties_assignVersion(props, "single/versionKey", celix_version_create(1, 2, 3, "qualifier"));

    celix_array_list_t* strArr = celix_arrayList_createStringArray();
    celix_arrayList_addString(strArr, "value1");
    celix_arrayList_addString(strArr, "value2");
    celix_properties_assignArrayList(props, "array/stringArr", strArr);

    celix_array_list_t* longArr = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longArr, 1);
    celix_arrayList_addLong(longArr, 2);
    celix_properties_assignArrayList(props, "array/longArr", longArr);

    celix_array_list_t* doubleArr = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleArr, 1.0);
    celix_arrayList_addDouble(doubleArr, 2.0);
    celix_properties_assignArrayList(props, "array/doubleArr", doubleArr);

    celix_array_list_t* boolArr = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolArr, true);
    celix_arrayList_addBool(boolArr, false);
    celix_properties_assignArrayList(props, "array/boolArr", boolArr);

    celix_array_list_t* versionArr = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(versionArr, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_assignVersion(versionArr, celix_version_create(4, 5, 6, "qualifier"));
    celix_properties_assignArrayList(props, "array/versionArr", versionArr);
  
    celix_properties_saveToStream(props, stdout, CELIX_PROPERTIES_ENCODE_PRETTY);
}
```

Will output the following JSON (order of keys can differ):

```JSON
{
  "array/doubleArr": [
    1.0,
    2.0
  ],
  "array/boolArr": [
    true,
    false
  ],
  "single/versionKey": "version<1.2.3.qualifier>",
  "array/longArr": [
    1,
    2
  ],
  "single/strKey": "strValue",
  "single/doubleKey": 2.0,
  "single/boolKey": true,
  "array/versionArr": [
    "version<1.2.3.qualifier>",
    "version<4.5.6.qualifier>"
  ],
  "array/stringArr": [
    "value1",
    "value2"
  ],
  "single/longKey": 42
}   
```

### Properties Nested Encoding

When properties are encoded to JSON in a nested structure, the keys of the properties are used to create a nested JSON
object. This is done by using the '/' character in the properties key to create a nested JSON objects. When encoding
properties using a nested structure, there is a risk of key collisions. To detect key collisions, the flag
`CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS` can be used.

Nested Encoding example:

```C
#include <stdio.h>
#include <celix/properties.h>

int main() {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_properties_set(props, "single/strKey", "strValue");
    celix_properties_setLong(props, "single/longKey", 42);
    celix_properties_setDouble(props, "single/doubleKey", 2.0);
    celix_properties_setBool(props, "single/boolKey", true);
    celix_properties_assignVersion(props, "single/versionKey", celix_version_create(1, 2, 3, "qualifier"));

    celix_array_list_t* strArr = celix_arrayList_createStringArray();
    celix_arrayList_addString(strArr, "value1");
    celix_arrayList_addString(strArr, "value2");
    celix_properties_assignArrayList(props, "array/stringArr", strArr);

    celix_array_list_t* longArr = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longArr, 1);
    celix_arrayList_addLong(longArr, 2);
    celix_properties_assignArrayList(props, "array/longArr", longArr);

    celix_array_list_t* doubleArr = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleArr, 1.0);
    celix_arrayList_addDouble(doubleArr, 2.0);
    celix_properties_assignArrayList(props, "array/doubleArr", doubleArr);

    celix_array_list_t* boolArr = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolArr, true);
    celix_arrayList_addBool(boolArr, false);
    celix_properties_assignArrayList(props, "array/boolArr", boolArr);

    celix_array_list_t* versionArr = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(versionArr, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_assignVersion(versionArr, celix_version_create(4, 5, 6, "qualifier"));
    celix_properties_assignArrayList(props, "array/versionArr", versionArr);
  
    celix_properties_saveToStream(props, stdout, CELIX_PROPERTIES_ENCODE_PRETTY | CELIX_PROPERTIES_ENCODE_NESTED_STYLE);
}
```

Will output the following JSON (order of keys can differ):

```JSON
{
  "array": {
    "doubleArr": [
      1.0,
      2.0
    ],
    "boolArr": [
      true,
      false
    ],
    "longArr": [
      1,
      2
    ],
    "versionArr": [
      "version<1.2.3.qualifier>",
      "version<4.5.6.qualifier>"
    ],
    "stringArr": [
      "value1",
      "value2"
    ]
  },
  "single": {
    "versionKey": "version<1.2.3.qualifier>",
    "strKey": "strValue",
    "doubleKey": 2.0,
    "boolKey": true,
    "longKey": 42
  }
}
```

### Encoding Flags

Properties encoding flags can be used control the behavior of the encoding. The following encoding flags can be used:

- `CELIX_PROPERTIES_ENCODE_PRETTY`: Flag to indicate that the encoded output should be pretty; e.g. encoded with
  additional whitespaces, newlines and indentation. If this flag is not set, the encoded output will compact; e.g.
  without additional whitespaces, newlines and indentation.

- `CELIX_PROPERTIES_ENCODE_FLAT_STYLE`: Flag to indicate that the encoded output should be flat; e.g. all properties
  entries are written as top level field entries.

- `CELIX_PROPERTIES_ENCODE_NESTED_STYLE`: Flag to indicate that the encoded output should be nested; e.g. properties
  entries are split on '/' and nested in JSON objects.

- `CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS`: Flag to indicate that the encoding should fail if the JSON
  representation will contain colliding keys. Note that colliding keys can only occur when using the nested encoding
  style.

- `CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS`: Flag to indicate that the encoding should fail if the JSON
  representation will contain empty arrays.

- `CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF`: Flag to indicate that the encoding should fail if the JSON representation
  will contain NaN or Inf values.

- `CELIX_PROPERTIES_ENCODE_STRICT`: Flag to indicate that all encode "error on" flags should be set.

## Properties Decoding

JSON can be decoded to an Apache Celix properties object using
the `celix_properties_load2`, `celix_properties_loadFromStream` and `celix_properties_loadFromString2` functions. These
functions take a JSON input and decode it to a properties object. Because properties use a flat key structure,
decoding a nested JSON object to properties results in combining JSON object keys to a flat key structure. This can
result in key collisions. 

By default, the decoding will not fail on empty arrays, null values, empty keys, or mixed arrays and instead these JSON
entries will be ignored. Also by default, if decoding results in a duplicate properties key, the last value will be used
and no error will be returned.

### Decoding example

Given a `example.json` file with the following content:

```JSON
{
  "counters": {
    "counter1": 1,
    "counter2": 2
  },
  "strings": {
    "string1": "value1",
    "string2": "value2"
  }
}
```

Combined with the following code:

```c
#include <stdio.h>

#include <celix/properties.h>

int main() {
    celix_autoptr(celix_properties_t) props;
    celix_status_t status = celix_properties_load2("example.json", 0, &props):
    (void)status; //for production code check status
    CELIX_PROPERTIES_ITERATE(props, iter) { 
        printf("key=%s, value=%s\n", celix_properties_key(iter.key), celix_properties_value(iter.entry.value));        
    }
}
```

Will output the following:

```
key=counters/counter1, value=1
key=counters/counter2, value=2
key=strings/string1, value=value1
key=strings/string2, value=value2
```

### Decoding Flags

Properties decoding behavior can be controlled using flags. The following decoding flags can be used:

- `CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES`: Flag to indicate that the decoding should fail if the input contains
  duplicate JSON keys.

- `CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS`: Flag to indicate that the decoding should fail if the input contains
  entry that collide on property keys.

- `CELIX_PROPERTIES_DECODE_ERROR_ON_NULL_VALUES`: Flag to indicate that the decoding should fail if the input contains
  null values.

- `CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS`: Flag to indicate that the decoding should fail if the input contains
  empty arrays.

- `CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_KEYS`: Flag to indicate that the decoding should fail if the input contains
  empty keys.

- `CELIX_PROPERTIES_DECODE_ERROR_ON_MIXED_ARRAYS`: Flag to indicate that the decoding should fail if the input contains
  mixed arrays.

- `CELIX_PROPERTIES_DECODE_STRICT`: Flag to indicate that the decoding should fail if the input contains any of the
  decode error flags.
