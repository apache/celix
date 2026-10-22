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

use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_DEBUG as CELIX_LOG_LEVEL_DEBUG;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_DISABLED as CELIX_LOG_LEVEL_DISABLED;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_ERROR as CELIX_LOG_LEVEL_ERROR;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_FATAL as CELIX_LOG_LEVEL_FATAL;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_INFO as CELIX_LOG_LEVEL_INFO;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_TRACE as CELIX_LOG_LEVEL_TRACE;
use celix_bindings::celix_log_level_CELIX_LOG_LEVEL_WARNING as CELIX_LOG_LEVEL_WARNING;
use celix_bindings::celix_log_level_e;

pub enum LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Disabled,
}
impl From<celix_log_level_e> for LogLevel {
    fn from(level: celix_log_level_e) -> Self {
        match level {
            CELIX_LOG_LEVEL_TRACE => LogLevel::Trace,
            CELIX_LOG_LEVEL_DEBUG => LogLevel::Debug,
            CELIX_LOG_LEVEL_INFO => LogLevel::Info,
            CELIX_LOG_LEVEL_WARNING => LogLevel::Warning,
            CELIX_LOG_LEVEL_ERROR => LogLevel::Error,
            CELIX_LOG_LEVEL_FATAL => LogLevel::Fatal,
            _ => LogLevel::Disabled,
        }
    }
}

impl Into<celix_log_level_e> for LogLevel {
    fn into(self) -> celix_log_level_e {
        match self {
            LogLevel::Trace => CELIX_LOG_LEVEL_TRACE,
            LogLevel::Debug => CELIX_LOG_LEVEL_DEBUG,
            LogLevel::Info => CELIX_LOG_LEVEL_INFO,
            LogLevel::Warning => CELIX_LOG_LEVEL_WARNING,
            LogLevel::Error => CELIX_LOG_LEVEL_ERROR,
            LogLevel::Fatal => CELIX_LOG_LEVEL_FATAL,
            _ => CELIX_LOG_LEVEL_DISABLED,
        }
    }
}
