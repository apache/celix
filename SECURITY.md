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

# Apache Celix Security

## Reporting a Vulnerability

The Apache Software Foundation takes an active stance in eliminating security problems and denial-of-service attacks
against its products. Report suspected vulnerabilities privately to the
[Apache Security Team](https://www.apache.org/security/) at `security@apache.org`.

Do not open a public GitHub issue, pull request, or mailing-list thread for an undisclosed vulnerability. Use the public
[Celix issue tracker](https://github.com/apache/celix/issues) only for ordinary bugs and hardening suggestions that do
not disclose a vulnerability. If in doubt, report the issue privately.

## Security Model

Apache Celix is a framework for dynamic, modular C and C++ applications. A Celix container runs native-code bundles in
one process, without sandboxing or security isolation. Celix trusts the host, runtime account, process environment,
configuration, bundle cache, and installed bundles. 

Install only trusted bundles, protect deployment files with OS permissions, and treat bundle lifecycle or shell access as 
administrative access to the process. 

Apache Celix does not authenticate bundle publishers or enforce bundle signing. Network-oriented bundles are intended 
for controlled embedded and cyber-physical systems, not direct Internet exposure.
Unless explicitly documented otherwise, the deployment must provide network isolation, peer authentication, authorization, 
and any required confidentiality and integrity. 

Examples and test configurations are not production security recommendations. 

Apache Celix still aims to handle malformed network and IPC data without memory corruption, code execution, process crashes, 
or disproportionate resource use; failures of this kind are potential vulnerabilities even on an isolated network. 

Apache Celix does not provide general secrets management; logs, shell output, property dumps, endpoint descriptions,
 bundle storage, and configuration may contain sensitive values and must be access-controlled.

### Bundle-Specific Security Model

| Bundle or bundle group | Intended use and security boundary |
|---|---|
| `remote_shell`, `shell_wui`, and `Bonjour` shell | Development and early-integration aids; not intended for production. Shell access is administrative access. The Telnet shell has no authentication or encryption, and the web shell adds no authentication or authorization. |
| `HTTP Admin` | May host production application endpoints, but provides routing rather than a security gateway. It does not provide authentication, authorization, rate limiting, or transport encryption. Applications or deployment infrastructure must protect its HTTP and WebSocket endpoints. |
| `Remote Services` | Intended only for a controlled system network. The topology policy imports and exports all eligible services and endpoints, so export only intended services and trust discovery data. The DFI implementation uses unauthenticated, unencrypted HTTP and binds to all interfaces by default. Configured, etcd, and Zeroconf discovery do not authenticate endpoint descriptions. Interceptors are extension hooks, not built-in security controls. |
| `Shared-memory Remote Service Admin` | Uses shared memory and Linux domain datagram sockets for same-host IPC. It does not isolate untrusted local processes. |
| `MQTT Event Admin remote provider` | Trusts the broker, broker discovery information, and participating Celix framework instances. The deployment must verify broker identity and provide network access control, authentication, authorization, and encryption as required. |
