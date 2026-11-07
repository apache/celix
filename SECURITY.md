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

Apache Celix is intended to be used as a framework inside a trusted system. Its network- and IPC-capable bundles are, by
design, not meant to be exposed directly to a public or untrusted environment; deployments are expected to provide the host
and network isolation and access controls that establish the trust boundary. Network- and IPC-capable bundles therefore expect 
their peers to be inside the trust boundary and trusted. 

Apache Celix does not authenticate bundle publishers or enforce bundle signing.

Examples and test configurations are not production security recommendations. 

Apache Celix still aims to handle malformed network and IPC data without memory corruption, code execution, process crashes, 
or disproportionate resource use; failures of this kind are potential vulnerabilities even on an isolated network. When the data
is supplied by a peer that is inside the trust boundary by the statements above (for example, a local process in the same network
namespace, or a trusted bundle), the defect is still real and may still constitute a vulnerability, but it is generally scored low
because a correct deployment already places it behind trust-establishing protections at the system boundary. Fixes for such cases
are treated as defense-in-depth improvements.

Apache Celix does not provide general secrets management; logs, shell output, property dumps, endpoint descriptions,
 bundle storage, and configuration may contain sensitive values and must be access-controlled.

### Bundle-Specific Security Model

| Bundle or bundle group | Intended use and security boundary |
|---|---|
| `remote_shell`, `shell_wui`, and `Bonjour` shell | Development and early-integration aids; not intended for production. Shell access is administrative access. The Telnet shell has no authentication or encryption, and the web shell adds no authentication or authorization. |
| `HTTP Admin` | May host production application endpoints, but provides routing rather than a security gateway. Assumes all HTTP and WebSocket requests originate from hosts inside the trust boundary and are trusted. Deployments must use network isolation and access controls to ensure no untrusted client can reach it. Defects in handling these inputs are thereby scored low as defense-in-depth. |
| `Remote Services` | Intended only for a controlled system network. The topology policy imports and exports all eligible services and endpoints, so export only intended services and trust discovery data. The DFI implementation uses unauthenticated, unencrypted HTTP and binds to all interfaces by default. Configured, etcd, and Zeroconf discovery do not authenticate endpoint descriptions. Interceptors are extension hooks, not built-in security controls. |
| `Shared-memory Remote Service Admin` | Same-host IPC via shared memory and Linux abstract-namespace domain datagram sockets. Assumes every process on the host (in the same network namespace) is inside the trust boundary and trusted; it does not authenticate, authorize, or isolate local peers. Deployments must ensure no untrusted code runs on the host or in the same network namespace. |
| `MQTT Event Admin remote provider` | Trusts the broker, broker discovery information, and participating Celix framework instances. The deployment must verify broker identity and provide network access control, authentication, authorization, and encryption as required. |

