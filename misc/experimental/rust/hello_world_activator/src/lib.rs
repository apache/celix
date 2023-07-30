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
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

use std::os::raw::c_void;

// TODO use celix_bundleActivator_create to create a bundle activator
// pub struct RustBundleActivator {
//     bundle_context: *mut c_void, /*TODO use bindgen to generate the correct ctx type*/
// }

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_start(_data: *mut c_void, _context: *mut c_void) -> i32 {
    println!("Rust Bundle started!");
    0
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_stop(_data: *mut c_void, _context: *mut c_void) -> i32 {
    println!("Rust Bundle stopped!");
    0
}
