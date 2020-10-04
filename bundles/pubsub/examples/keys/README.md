---
title: Pubsub Keys
---

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

Store the AES key for encrypting and decrypting the encoded secret keys safe in a file!
Default file is `/etc/pubsub.keys` with the following format:
```
aes_key:{32 character AES key here}
aes_iv:{16 character AES iv here}
```

Use the $PROJECT_BUILD/pubsub/keygen/makecert for generating keypairs
Use the $PROJECT_BUILD/pubsub/keygen/ed_file for encrypting and decrypting private keys

Public keys need to be stored in the 'public' folder having the following format:
- pub_{topic}.pub
- sub_{topic}.pub

Secret keys need to be stored in the 'private' folder having the following format:
- pub_{topic}.key.enc
- sub_{topic}.key.enc
These files need to be encrypted using the 'ed_file' executable.
