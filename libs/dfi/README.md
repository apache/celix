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

  *Type representation*:
  
  |**Identifier**|B  |D     |F    |I      |J      |S      |V   |Z             |b    | i      | j      | s      |P     |t     |N  | 
  |---------|---|------|-----|-------|-------|-------|----|--------------|-----|--------|--------|--------|------|------|---|
  |**Types**|char|double|float|int32_t|int64_t|int16_t|void|boolean(uint8)|uchar|uint32_t|uint64_t|uint16_t|void *|char *|int|


- **Complex Types(Struct)**

  *Type representation*:
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

  *Type representation*:
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

  *Type representation*:
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

- **Reference By Value**

  *Type representation*:
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

  *Type representation*:
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

  *Type representation*:
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
    typedef {
      double val1;
      double val2;
    }type;
    type typeVal;
    double a;
  }
  ~~~
  
- **Meta-Information**  

  *Type representation*:
  ~~~
  #Name=Value;
  ~~~
  *Example*:
  ~~~
  #a=hello;
  ~~~

- **Enumeration**

  *Type representation*:
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

  *Type representation*:
  ~~~
  (Name)([Type]*)Type
  ~~~
  In order to represent the properties of function parameters (eg: in, out...), function parameters support the following metadata annotations:

  |Meta-info|Description|
  |---------|-----------|
  |am=handle| void pointer for the handle|
  |am=pre   | output pointer with memory pre-allocated|
  |am=out   | output pointer, the caller should use `free` to release the memory|
  |const=true| text argument(t) can use it, Normally a text argument will be handled as char*, meaning that the callee is expected to take of ownership.If a const=true annotation is used the text argument will be handled as a const char*, meaning that the caller keeps ownership of the string.|

  *Example*:
  ~~~
  add(#am=handle;PDD#am=pre;*D)N
  ~~~
  *To C language*:
  ~~~
  int add(void* handle,double a, double b, double *ret);
  ~~~
  *Notes*

  - The return type of the function must be N, because remote service calls usually return error codes.
  - Currently, the function only supports one output parameter, so the function cannot be defined in a multi-output parameter form.

#### Interface Description File

An interface description file is that the interface file written using the interface description language, and its file suffix is ".descriptor". Generally, for the association between the remote service instance and the interface description file, the interface filename tends to match the remote service name."

The interface description file should exist in the bundle where the interface user or provider is located, and the description information should be consistent with the interface header file in use. When generating a bundle, we usually store the interface description file in the following paths of the bundle: "META-INF/descriptors/", "META-INF/descriptors/services/ ".