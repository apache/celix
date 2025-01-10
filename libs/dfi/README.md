---
title: Apache Celix Dynamic Function Interface
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

## Apache Celix Dynamic Function Interface

Dynamic Function Interface (DFI) is a dynamic interface type implementation based on [libffi](https://en.wikipedia.org/wiki/Libffi).
It can generate dynamic interface types according to the interface description file, and can convert the corresponding 
dynamic interface call into a [JSON representation](https://amdatu.atlassian.net/wiki/spaces/AMDATUDEV/pages/21954571/Amdatu+Remote#AmdatuRemote-AdminHTTP%2FJson). It can also convert the JSON representation into a dynamic interface call.

### Conan Option
    build_celix_dfi=True   Default is False

### CMake Option
    CELIX_DFI=ON           Default is ON

### Interface Descriptor

In `libdfi`, we have defined a set of simple interface description languages. Users can use this language to describe interface types, and at runtime, libdfi converts the interface description into a dynamic interface.

Before introducing the interface description language, let's look at an example of interface description.

~~~
:header
type=interface
name=calculator
version=1.0.0
:annotations
classname=org.example.Calculator
:types
StatsResult={DDD[D average min max input}
:methods
add(DD)D=add(#am=handle;PDD#am=pre;*D)N
~~~

As above, the interface description has four sections: the header section, annotations section, types section, and methods section. The format of each section is as follows:

~~~
 ':header\n' 'Name=Value\n'...
 ':annotations\n' 'Name=Value\n'...
 ':types\n' 'TypeId=Value\n'...
 ':methods\n' 'MethodId=Value\n'...
~~~

Among them, the legal characters that can be used in the “name” and “TypeId” include [a-zA-Z0-9_], the legal characters in “MethodId” include [a-zA-Z0-9_] and ".();[{}/". Besides [a-zA-Z0-9], the legal characters in “value” also include ".<>{}[]?;:~!@#$%^&*()_+-=,./'". It's worth noting that there should not be spaces on either side of '=', and each statement must end with a newline(‘\n’).

For the interface description, its header section must include three elements: type, name, version. The value of "type" should be "interface", "name" is the interface name (service name), the value of "version" should coincide with the version number in the actually used interface header file, and it should conform to [semantic versioning requirements](https://semver.org/).

#### The Data Types In The Interface Descriptor

The data types supported by the interface description include:

- **Simple Types**

  *Type schema*:
  
  |**Identifier**|B  |D     |F    |I      |J      |S      |V   |Z             |b    | i      | j      | s      |P     | t                |N  | p                   | a                   |
  |---------|---|------|-----|-------|-------|-------|----|--------------|-----|--------|--------|--------|------|------------------|---|---------------------|---------------------|
  |**Types**|char|double|float|int32_t|int64_t|int16_t|void|boolean(uint8)|uchar|uint32_t|uint64_t|uint16_t|void *| char *(C string) |int| celix_properties_t* | celix_array_list_t* |


- **Complex Types(Struct)**

  *Type schema*:
  ~~~
  {[Type]+ [(Name)(SPACE)]+}
  ~~~
  *Example*:
  ~~~
  {DDII a b c d}
  ~~~
  *To C language*:
  ~~~
  struct { double a; double b; int c; int d; };
  ~~~

- **Sequence Type**

  *Type schema*:
  ~~~
  [(type)
  ~~~
  *Example*:
  ~~~
  [D
  ~~~
  *To C language*:
  ~~~
  struct {
    uint32_t cap;
    uint32_t len;
    duoble *buf;
  };
  ~~~
  
- **Typed Pointer**

  *Type schema*:
  ~~~
  *(Type)
  ~~~
  *Example*:
  ~~~
  *D
  ~~~
  *To C language*:
  ~~~
  duoble *d;
  ~~~

> **NOTES**: "*B" indicates a pointer to a char, "t" indicates a text type, which is C string.

- **Reference By Value**

  *Type schema*:
  ~~~
  l(name);
  ~~~
  *Example*:
  ~~~
  MySubType={jDD time d1 d2}
  MyType={DDlMySubType; d11 d12 subTypeVal}
  ~~~
  *To C language*:
  ~~~
  struct MySubType{
    uint64_t time;
    double d1;
    double d2;
  };
  struct MyType {
    double d11;
    double d12;
    struct MySubType subTypeVal;
  };
  ~~~

- **Pointer Reference**

  *Type schema*:
  ~~~
  L(name);//shortcut for *l(name);
  ~~~
  *Example*:
  ~~~
  MySubType={jDD time d1 d2}
  MyType={DDLMySubType; d11 d12 subTypePtr}
  ~~~
  *To C language*:
  ~~~
  struct MySubType{
    uint64_t time;
    double d1;
    double d2;
  };
  struct MyType {
    double d11;
    double d12;
    struct MySubType *subTypePtr;
  };
  ~~~

- **Type Alias（typedef）**

  *Type schema*:
  ~~~
  T(Name)=Type;
  ~~~
  *Example*:
  ~~~
  Ttype={DD val1 val2};{ltype;D typeVal a}
  ~~~
  *To C language*:
  ~~~
  struct {
    typedef struct {
      double val1;
      double val2;
    }type;
    type typeVal;
    double a;
  }
  ~~~
  
- **Meta-Information**  

  *Type schema*:
  ~~~
  #Name=Value;
  ~~~
  *Example*:
  ~~~
  #a=hello;
  ~~~

- **Enumeration**

  *Type schema*:
  ~~~
  #EnumName=value;E
  ~~~
  *Example*:
  ~~~
  #v1=0;#v2=1;E
  ~~~
  *To C language*:
  ~~~
    enum {
      v1=0;
      v2=1;
    };
  ~~~
  
- **Method/Function**

  *Type schema*:
  ~~~
  (Name)([Type]*)Type
  ~~~
  In order to represent the properties of function parameters (eg: in, out...), function parameters support the following metadata annotations:

  |Meta-info| Description                                                                                                                                                                                                                                                                                                                                      |
  |---------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
  |am=handle| void pointer for the handle.                                                                                                                                                                                                                                                                                                                     |
  |am=pre   | output pointer with memory pre-allocated, it should be pointer to [trivially copyable type](#notion-definitions).                                                                                                                                                                                                                                |
  |am=out   | output pointer, the caller should use `free` to release the memory, and it should be pointer to text(t) or double pointer to [serializable types](#notion-definitions).                                                                                                                                                                          |
  |const=true| text argument(t) and `celix_properties_t*`(p) and `celix_array_list_t*`(a) can use it, Normally a text argument will be handled as char*, meaning that the callee is expected to take of ownership.If a const=true annotation is used the text argument will be handled as a const char*, meaning that the caller keeps ownership of the string. |

  If there is no metadata annotation, the default is standard argument(input parameter). And it can be any serializable type.

  *Example*:
  ~~~
  add(#am=handle;PDD#am=pre;*D)N
  ~~~
  *To C language*:
  ~~~
  int add(void* handle,double a, double b, double *ret);
  ~~~

##### Notion Definitions

- **trivially copyable type**: A trivially copyable type is a type that can be copied with a simple memcpy without the usual danger of shallow copying.
- **serializable type**: All types except types involving untyped pointer or double pointer (pointer to pointer) are serializable. For example, complex types consisting of non-pointer fields are serializable while complex type containing a untyped pointer field is not serializable; [I is serializable while [P and [**D are not serializable.

##### RSA Interface Convention Enforcement:
  - For RSA interface, the return type of methods must be N, because remote service calls usually return error codes.
  - The first parameter of a method must be `handle`, and`am=handle` can appear exactly once.
  - If exists, output parameter (either `am=pre` or `am=out`) is only allowed as the last one. Therefore, there is at most one output parameter.

#### Interface Description File

An interface description file is that the interface file written using the interface description language, and its file suffix is ".descriptor". Generally, to associate the remote service instance with the interface description file, the interface description filename should be consistent with the remote service name.

The interface description file should exist in the bundle where the interface consumer or provider is located, and the description information should be consistent with the interface header file in use. When generating a bundle, we usually store the interface description file in the following paths of the bundle: "META-INF/descriptors/", "META-INF/descriptors/services/ ".