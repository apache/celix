---
title: Apache Celix Properties & Filter
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

# Apache Celix Properties
The Apache Celix Utils library provides a properties implementation which is used in several places in Apache Celix.
The properties are a key-value map with string keys and values with can be of a type:
- String (char*)
- Long (long)
- Double (double)
- Bool (bool)
- Version (celix_version_t*)
- String Array (celix_array_list_t*)
- Long Array (celix_array_list_t*)
- Double Array (celix_array_list_t*)
- Bool Array (celix_array_list_t*)
- Version Array (celix_array_list_t*)

## Configuration Properties
Properties can be used - and are used in the Apache Celix framework - for runtime configuration and metadata.
In this use case the actual underlying value types are not important and the properties values can be accessed and
automatically converted to the requested type.

In the aforementioned use case the properties value are accessed using the "getAs" functions, for example:
```c
celix_properties_t* props = celix_properties_create();
celix_properties_set(props, "myLong", "10");
long value = celix_properties_getAsLong(props, "myLong", 0L);
assert(value == 10L);
```
In the above example the value of the property with key "myLong" is returned as a long and if the property is not
available or cannot be converted to a long the default value (0L) is returned.

## Service Properties
Properties are also be used as metadata for services. In this use case the actual underlying value types are important
, because they are used to filter services. 

The only access properties values if the property value type is the expected type the "get" functions should be used,
for example:
```c
celix_properties_t* props = celix_properties_create();
celix_properties_setLong(props, "myLong", 10);
long value = celix_properties_getLong(props, "myLong", 0L);
assert(value == 10L);
```
In the above example the value of the property with key "myLong" is returned if the property is available and the
property value type is a long. 

## Apache Celix Properties C++ API
The Apache Celix Properties C++ API is a header only API which can be used to create and access properties.
The concept of C++ Properties is similar to the C Properties.

Example:
```cpp
celix::Properties props{};
props.set("myLong", 10L);
long value = props.getLong("myLong", 0L); 
assert(value == 10L);
```

## More Information
See the `celix_properites.h` C header and `celix/Properties.h` C++ header for more information.

## Apache Celix Filter
Apache Celix also provides a filter implementation which can be used to filter properties based on their values.
In the Apache Celix Framework the filter implementation is used to filter services based on their properties.

Filters are created using a LDAP like syntax, for example:
```c
celix_filter_t* filter = celix_filter_create("(myLong>5)");
```

Filters can be used to match properties, for example:
```c
celix_properties_t* props = celix_properties_create();
celix_properties_setLong(props, "myLong", 10);
bool match = celix_filter_match(filter, props);
assert(match);
```

When matching properties with a filter, the property value types determined the matching rules. This means
that a `=` (equal) match for a long property will yield a different result than a `=` (equal) match for a string.

For example, if a properties set contains a property "myLong" with string value "10" and a filter "(myLong>5)" is 
used to match the properties set, the filter will not match. This is because the filter matching uses lexical 
comparison and in that case "10" is not greater than "5". 
When the same filter is used to match a properties set with a property "myLong" with long value 10, the filter will 
match.

## Apache Celix Filter C++ API
The Apache Celix Filter C++ API is a header only API which can be used to create and match filters.
The concept of C++ Filters is similar to the C Filters.

Example:
```cpp
celix::Filter filter{"(myLong>5)"};
celix::Properties props{};
props.set("myLong", 10L);
bool match = filter.match(props);
assert(match);
```

## More Information
See the `celix_filter.h` C header and `celix/Filter.h` C++ header for more information.
