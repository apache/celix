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

#ifndef _DYN_TYPE_H_
#define _DYN_TYPE_H_

#include <stdio.h>
#include <sys/queue.h>
#include <stdbool.h>

#include <stdint.h>

#include "dfi_log_util.h"

#if defined(BSD) || defined(__APPLE__) || defined(__ANDROID__)
#include "memstream/open_memstream.h"
#include "memstream/fmemopen.h"
#endif

/* Description string
 *
 * Type = [TypeDef]* (MetaInfo)* (SimpleType | ComplexType | SequenceType | TypedPointer | PointerReference ) [TypeDef]*
 * Name = alpha[(alpha|numeric)*]
 * SPACE = ' ' 
 *
 * SimplesTypes (based on java bytecode method signatures)
 * //Java based:
 * B char
 * C (not supported)
 * D double
 * F float
 * I int32_t 
 * J int64_t 
 * S int16_t 
 * V void
 * Z boolean
 * //Extended
 * b unsigned char
 * i uint32_t
 * j uint62_t
 * s uint64_t
 * P untyped pointer (void *)
 * t char* string
 * N native int
 *
 * ComplexTypes (Struct)
 * {[Type]+ [(Name)(SPACE)]+}
 *
 * ReferenceByValue
 * l(name);
 *
 * PointerReference -> note shortcut for *l(name);
 * L(Name);
 *
 * TypeDef 
 * T(Name)=Type;
 *
 * SequenceType
 * [(Type)
 *
 * TypedPointer
 * *(Type)
 *
 * MetaInfo TODO
 * #Name=Value;
 *
 *
 *
 * examples
 * "{DDII a b c d}" -> struct { double a; double b; int c; int d; }; 
 * "{DD{FF c1 c2} a b c}" -> struct { double a; double b; struct c { float c1; float c2; }; }; 
 *
 *
 */

#define DYN_TYPE_INVALID 0
#define DYN_TYPE_SIMPLE 1
#define DYN_TYPE_COMPLEX 2
#define DYN_TYPE_SEQUENCE 3
#define DYN_TYPE_TYPED_POINTER 4
#define DYN_TYPE_TEXT 5
#define DYN_TYPE_REF 6

typedef struct _dyn_type dyn_type;

TAILQ_HEAD(types_head, type_entry);
struct type_entry {
    dyn_type *type;
    TAILQ_ENTRY(type_entry) entries;
};

TAILQ_HEAD(complex_type_entries_head, complex_type_entry);
struct complex_type_entry {
    dyn_type *type;
    char *name;
    TAILQ_ENTRY(complex_type_entry) entries;
};

//logging
DFI_SETUP_LOG_HEADER(dynType);

//generic
int dynType_parse(FILE *descriptorStream, const char *name, struct types_head *refTypes, dyn_type **type);
int dynType_parseWithStr(const char *descriptor, const char *name, struct types_head *refTypes, dyn_type **type);
void dynType_destroy(dyn_type *type);

int dynType_alloc(dyn_type *type, void **bufLoc);
void dynType_free(dyn_type *type, void *loc);

void dynType_print(dyn_type *type, FILE *stream);
size_t dynType_size(dyn_type *type);
int dynType_type(dyn_type *type);
int dynType_descriptorType(dyn_type *type);
const char * dynType_getMetaInfo(dyn_type *type, const char *name);

//complexType
int dynType_complex_indexForName(dyn_type *type, const char *name);
int dynType_complex_dynTypeAt(dyn_type *type, int index, dyn_type **subType);
int dynType_complex_setValueAt(dyn_type *type, int index, void *inst, void *in);
int dynType_complex_valLocAt(dyn_type *type, int index, void *inst, void **valLoc);
int dynType_complex_entries(dyn_type *type, struct complex_type_entries_head **entries);

//sequence
int dynType_sequence_alloc(dyn_type *type, void *inst, uint32_t cap);
int dynType_sequence_locForIndex(dyn_type *type, void *seqLoc, int index, void **valLoc);
int dynType_sequence_increaseLengthAndReturnLastLoc(dyn_type *type, void *seqLoc, void **valLoc);
dyn_type * dynType_sequence_itemType(dyn_type *type);
uint32_t dynType_sequence_length(void *seqLoc);

//typed pointer
int dynType_typedPointer_getTypedType(dyn_type *type, dyn_type **typedType);

//text
int dynType_text_allocAndInit(dyn_type *type, void *textLoc, const char *value);

//simple
void dynType_simple_setValue(dyn_type *type, void *inst, void *in);

#endif
