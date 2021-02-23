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

# Intro

The main way to use Celix is through the bundle context of a bundle.
When a bundle is started the bundle context will be injected in the bundle activator.

Apache Celix is a C framework with a C and C++ (C++11) API. 

## C Bundle Activator
```
#include <celix_api.h>

typedef struct activator_data {
    /*intentional empty*/
} activator_data_t;

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    //use the bundle context ctx to activate the bundle
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    //use the bundle context ctx to cleanup
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
```

## C++ Bundle Activator
```
#include "celix/BundleActivator.h"

class MyBundleActivator {
public:
    explicit MyBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        //use the bundle context ctx to activate the bundle
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(MyBundleActivator)
```

# C API

See include/celix_bundle_context.h as a starting point for the C API.

# C++ API

See celix::BundleContext as a starting point for the C++ API. 
