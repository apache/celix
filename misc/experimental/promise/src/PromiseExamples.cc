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
#include "celix/Deferred.h"

celix::Promise<long> fib(long n) {
    auto deferred = celix::Deferred<long>{};

    if (n <= 0) {
        deferred.fail(std::logic_error{"Requires more than 2"});
    } else {
        std::thread t{[deferred, n] () mutable {
            long m = 1;
            long k = 0;
            for (long i = 1; i <= n; ++i) {
                int tmp = m + k;
                m = k;
                k = tmp;
            }
            deferred.resolve(m);
        }};
        t.detach();
    }

    return deferred.getPromise();
}

int main() {
    auto p1 = fib(100);
    p1.timeout(std::chrono::seconds{5}).onSuccess([](long val) {
        std::cout << "Found result p1 " << val << std::endl;
    });
    auto p2 = fib(1000);
    p2.timeout(std::chrono::seconds{5}).onSuccess([](long val) {
        std::cout << "Found result p2 " << val << std::endl;
    });
    p1.wait();
    p2.wait();
}