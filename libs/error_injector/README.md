---
title: Error Injector
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

# Celix Error Injector

Error handling path is infamously difficult to test. 
To achieve high testing coverage, some extra mechanism is generally needed to emulate various error conditions in the testing environment.
Such mechanism, as [SQLite's test VFS](https://www.sqlite.org/src/doc/trunk/src/test_vfs.c), tends to be heavy-weight and requires a lot of efforts to implement. 

Celix Error Injector provides a lightweight alternative way.
As its name suggests, Celix Error Injector enables you to inject arbitrary errors into target function call very easily.
You only have to:

1. Implement a simple stub module for your target API under this folder.
2. Link it into your test executable.
3. Link the code under test into your test executable **statically**. Check `test_framework_with_ei` for a way of doing this with minimal CMake duplication.
4. Specify the target function call you want to injector specific error into with a single function call before the code under test runs.
5. Disable error injector during the TearDown phase of the test. Forgetting this step may interrupt other tests.

We have implemented several injectors. 

* `malloc`/`realloc`/`calloc`
* `celix_properties_create`

Have a look at them before implementing your own.

## Example Usage

```c++
#include <gtest/gtest.h>
#include <iostream>
#include <cstring>
#include "malloc_ei.h"

#include "pubsub_wire_protocol_common.h"

class WireProtocolCommonEiTest : public ::testing::Test {
public:
    WireProtocolCommonEiTest() = default;
    ~WireProtocolCommonEiTest() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    };
};

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_NotEnoughMemoryForMultipleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key2,6:value2,6:key111,8:value111,"); //note 3 entries
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 3);
    for (int i = 0; i < 6; ++i) {
        celix_ei_expect_calloc((void *)pubsubProtocol_decodeMetadata/* caller */, 0, nullptr, i+1/* ordinal */);
        auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

        EXPECT_EQ(status, CELIX_ENOMEM);
        EXPECT_EQ(nullptr, message.metadata.metadata);
    }
    free(data);
}
```

In the above test, `pubsubProtocol_decodeMetadata` makes six calls to `calloc`.
Failure in any of the six calls should return `CELIX_ENOMEM`.
`celix_ei_expect_calloc((void *)pubsubProtocol_decodeMetadata, 0, nullptr, i+1)` specifies only the (i+1)-th call made by `pubsubProtocol_decodeMetadata` should fail.


Note that `caller` does not always work, especially if the caller is a static function (not visible outside the translation unit).
In this case, you should specify `CELIX_EI_UNKNOWN_CALLER` for `caller`, and rely solely on the `ordinal` parameter to specify which call to injector error into.
Note also that `celix_ei_expect_calloc(nullptr, 0, nullptr)` is called to disable error injector for `calloc` in the destructor.