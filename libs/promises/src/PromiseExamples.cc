/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */


#include <iostream>
#include "celix/PromiseFactory.h"

static long calc_fib(long n) {
    long m = 1;
    long k = 0;
    for (long i = 0; i <= n; ++i) {
        int tmp = m + k;
        m = k;
        k = tmp;
    }
    return m;
}

celix::Promise<long> fib(celix::PromiseFactory& factory, long n) {
    return factory.deferredTask<long>([n](auto deferred) {
        deferred.resolve(calc_fib(n));
    });
}

int main() {
    celix::PromiseFactory factory{};

    fib(factory, 1000000000)
        .setTimeout(std::chrono::milliseconds {100})
        .onSuccess([](long val) {
            std::cout << "Success promise 1 : " << std::to_string(val) << std::endl;
        })
        .onFailure([](const std::exception& e) {
            std::cerr << "Failure promise 1 : " << e.what() << std::endl;
        });

    fib(factory, 39)
        .setTimeout(std::chrono::milliseconds{100})
        .onSuccess([](long val) {
            std::cout << "Success promise 2 : " << std::to_string(val) << std::endl;
        })
        .onFailure([](const std::exception& e) {
            std::cerr << "Failure promise 2 : " << e.what() << std::endl;
        });

    //NOTE the program can only exit if the executor in the PromiseFactory is done executing all tasks.
}
