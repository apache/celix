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


#include <thread>
#include <iostream>
#include "celix/Deferred.h"

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

celix::Promise<long> fib(long n) {
    auto deferred = celix::Deferred<long>{};

    if (n <= 0) {
        deferred.fail(std::logic_error{"argument must be positive"});
    } else if (n < 10 ) {
        deferred.resolve(calc_fib(n));
    } else {
        std::thread t{[deferred, n] () mutable {
            deferred.resolve(calc_fib(n));
        }};
        t.detach();
    }

    return deferred.getPromise();
}

int main() {
    auto p1 = fib(39);
    p1.timeout(std::chrono::milliseconds{100})
    .onSuccess([](long val) {
        std::cout << "Success p1 : " << val << std::endl;
    })
    .onFailure([](const std::exception& e) {
        std::cerr << "Failure p1 : " << e.what() << std::endl;
    });
    auto p2 = fib(1000000000);
    p2.timeout(std::chrono::milliseconds {100}).onSuccess([](long val) {
        std::cout << "Success p2 : " << std::to_string(val) << std::endl;
    })
    .onFailure([](const std::exception& e) {
        std::cerr << "Failure p2 : " << e.what() << std::endl;
    });
    p1.wait();
    p2.wait();
}
