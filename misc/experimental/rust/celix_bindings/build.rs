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

use std::env;
use std::error::Error;
use std::fs::File;
use std::io::{self, BufRead};
use std::path::PathBuf;

fn open_include_paths_file() -> Result<File, Box<dyn Error>> {
    let file: File;

    let corrosion_build_dir = env::var("CORROSION_BUILD_DIR");
    if corrosion_build_dir.is_ok() {
        let build_dir = PathBuf::from(corrosion_build_dir.unwrap());
        let include_path_file = build_dir.join("include_paths.txt");
        file = File::open(&include_path_file)?;
    } else {
        println!("include_paths.txt not found in CORROSION_BUILD_DIR. Failing back to CELIX_RUST_INCLUDE_PATHS_FILE env value");
        let include_path_file = env::var("CELIX_RUST_INCLUDE_PATHS_FILE")?;
        file = File::open(&include_path_file)?;
    }

    Ok(file)
}

fn print_include_paths() -> Result<Vec<String>, Box<dyn Error>> {
    let file = open_include_paths_file()?;
    let reader = io::BufReader::new(file);
    let mut include_paths = Vec::new();
    let line = reader
        .lines()
        .next()
        .ok_or("Expected at least one line")??;
    for path in line.split(';') {
        include_paths.push(path.to_string());
    }
    Ok(include_paths)
}

fn main() {
    println!("cargo:info=Start build.rs for celix_bindings");
    let include_paths = print_include_paths().unwrap();

    let mut builder = bindgen::Builder::default().header("src/celix_bindings.h");

    // Add framework and utils include paths
    for path in &include_paths {
        builder = builder.clang_arg(format!("-I{}", path));
    }

    // Gen bindings
    let bindings = builder.generate().expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("celix_bindings.rs"))
        .expect("Couldn't write Apache Celix bindings!");
}
