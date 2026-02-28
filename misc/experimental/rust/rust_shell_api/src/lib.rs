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

use celix::Error;
pub const COMMAND_NAME: &str = "command.name";
pub const COMMAND_USAGE: &str = "command.usage";
pub const COMMAND_DESCRIPTION: &str = "command.description";

#[doc = "A trait to implement a Celix Shell Command"]
pub trait RustShellCommand {
    fn execute_command(&self, command_line: &str) -> Result<(), Error>;
}
