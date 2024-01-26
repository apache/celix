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

#include "dyn_type.h"
#include "dyn_type_common.h"
#include "dyn_common.h"
#include "celix_err.h"
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ffi.h>

static const int OK = 0;
static const int ERROR = 1;
static const int MEM_ERROR = 2;
static const int PARSE_ERROR = 3;

static int dynType_parseWithStream(FILE* stream, const char* name, dyn_type* parent, const struct types_head* refTypes, dyn_type** result);
static int dynType_parseWithStreamOfName(FILE* stream, char* name, dyn_type* parent, const struct types_head* refTypes, dyn_type** result, int (*)(FILE*, dyn_type*));
static void dynType_clear(dyn_type* type);
static void dynType_clearComplex(dyn_type* type);
static void dynType_clearSequence(dyn_type* type);
static void dynType_clearTypedPointer(dyn_type* type);

static ffi_type* dynType_ffiTypeFor(int c);
static int dynType_parseAny(FILE* stream, dyn_type* type);
static int dynType_parseComplex(FILE* stream, dyn_type* type);
static int dynType_parseNestedType(FILE* stream, dyn_type* type);
static int dynType_parseReference(FILE* stream, dyn_type* type);
static int dynType_parseRefByValue(FILE* stream, dyn_type* type);
static int dynType_parseSequence(FILE* stream, dyn_type* type);
static int dynType_parseSimple(int c, dyn_type* type);
static int dynType_parseTypedPointer(FILE* stream, dyn_type* type);
static unsigned short dynType_getOffset(const dyn_type* type, int index);

static void dynType_printAny(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printComplex(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printSequence(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printSimple(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printEnum(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printTypedPointer(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printText(const char* name, const dyn_type* type, int depth, FILE* stream);
static void dynType_printDepth(int depth, FILE* stream);

static void dynType_printTypes(const dyn_type* type, FILE* stream);
static void dynType_printComplexType(const dyn_type* type, FILE* stream);
static void dynType_printSimpleType(const dyn_type* type, FILE* stream);

static int dynType_parseText(FILE* stream, dyn_type* type);
static int dynType_parseEnum(FILE* stream, dyn_type* type);
static void dynType_freeComplexType(const dyn_type* type, void* loc);
static void dynType_deepFree(const dyn_type* type, void* loc, bool alsoDeleteSelf);
static void dynType_freeSequenceType(const dyn_type* type, void* seqLoc);

static int dynType_parseMetaInfo(FILE* stream, dyn_type* type);

struct generic_sequence {
    uint32_t cap;
    uint32_t len;
    void* buf;
};

int dynType_parse(FILE* descriptorStream, const char* name, const struct types_head* refTypes, dyn_type** type) {
    return dynType_parseWithStream(descriptorStream, name, NULL, refTypes, type);
}


int dynType_parseOfName(FILE* descriptorStream, char* name, const struct types_head* refTypes, dyn_type** type) {
    return dynType_parseWithStreamOfName(descriptorStream, name, NULL, refTypes, type, dynType_parseAny);
}

int dynType_parseWithStr(const char* descriptor, const char* name, const struct types_head* refTypes, dyn_type** type) {
    int status;
    celix_autoptr(FILE) stream = fmemopen((char *)descriptor, strlen(descriptor), "r");
    if (stream == NULL) {
        celix_err_pushf("Error creating mem stream for descriptor string. %s", strerror(errno));
        return ERROR;
    }
    celix_autoptr(dyn_type) result = NULL;
    if ((status = dynType_parseWithStream(stream, name, NULL, refTypes, &result)) != OK) {
        return status;
    }
    if (dynCommon_eatChar(stream, EOF) != 0) {
        return PARSE_ERROR;
    }
    *type = celix_steal_ptr(result);
    return OK;
}

static int dynType_parseWithStream(FILE* stream, const char* name, dyn_type* parent, const struct types_head* refTypes, dyn_type** result) {
    char* typeName = NULL;
    if (name != NULL) {
        typeName = strdup(name);
        if (typeName == NULL) {
            celix_err_pushf("Error strdup'ing name '%s'", name);
            return MEM_ERROR;
        }
    }
    return dynType_parseWithStreamOfName(stream, typeName, parent, refTypes, result, dynType_parseAny);
}

static int dynType_parseWithStreamOfName(FILE* stream, char* name, dyn_type* parent, const struct types_head* refTypes,
                                         dyn_type** result, int (*parse)(FILE*, dyn_type*)) {
    int status;
    celix_autofree char* typeName = name;
    celix_autoptr(dyn_type) type = calloc(1, sizeof(*type));
    if (type == NULL) {
        celix_err_push("Error allocating memory for type");
        return MEM_ERROR;
    }
    type->parent = parent;
    type->type = DYN_TYPE_INVALID;
    type->referenceTypes = refTypes;
    TAILQ_INIT(&type->nestedTypesHead);
    TAILQ_INIT(&type->metaProperties);
    type->name = celix_steal_ptr(typeName);
    if ((status = parse(stream, type)) != OK) {
        return status;
    }
    *result = celix_steal_ptr(type);
    return OK;
}

static int dynType_parseAny(FILE* stream, dyn_type* type) {
    int status = OK;

    int c = fgetc(stream);
    switch(c) {
        case 'T' :
            status = dynType_parseNestedType(stream, type);
            if (status == OK) {
                status = dynType_parseAny(stream, type);
            } 
            break;
        case 'L' :
            status = dynType_parseReference(stream, type);
            break;
        case 'l' :
            status = dynType_parseRefByValue(stream, type);
            break;
        case '{' :
            status = dynType_parseComplex(stream, type);
            break;
        case '[' :
            status = dynType_parseSequence(stream, type);
            break;
        case '*' :
            status = dynType_parseTypedPointer(stream, type);
            break;
        case 't' :
            status = dynType_parseText(stream, type);
            break;
        case 'E' :
            status = dynType_parseEnum(stream, type);
            break;
        case '#' :
            status = dynType_parseMetaInfo(stream, type);
            if (status == OK) {
                status = dynType_parseAny(stream, type);
            }
            break;
        default :
            status = dynType_parseSimple(c, type);
            break;
    }

    return status;
}

static int dynType_parseMetaInfo(FILE* stream, dyn_type* type) {
    int status = OK;
    celix_autofree char* name = NULL;
    celix_autofree char* value = NULL;

    if (dynCommon_parseName(stream, &name) != OK) {
        status = PARSE_ERROR;
        goto bail_out;
    }
    if (dynCommon_eatChar(stream, '=') != OK) {
        status = PARSE_ERROR;
        goto bail_out;
    }
    if (dynCommon_parseName(stream, &value) != OK) {
        status = PARSE_ERROR;
        goto bail_out;
    }
    if (dynCommon_eatChar(stream, ';') != OK) {
        status = PARSE_ERROR;
        goto bail_out;
    }
    struct meta_entry *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        status = MEM_ERROR;
        goto bail_out;
    }
    entry->name = celix_steal_ptr(name);
    entry->value = celix_steal_ptr(value);
    TAILQ_INSERT_TAIL(&type->metaProperties, entry, entries);
    return OK;

bail_out:
    celix_err_push("Failed to parse meta properties");
    return status;
}

static int dynType_parseText(FILE* stream, dyn_type* type) {
    type->type = DYN_TYPE_TEXT;
    type->descriptor = 't';
    type->trivial = false;
    type->ffiType = &ffi_type_pointer;
    return OK;
}

static int dynType_parseEnum(FILE* stream, dyn_type* type) {
    type->ffiType = &ffi_type_sint32;
    type->descriptor = 'E';
    type->trivial = true;
    type->type = DYN_TYPE_SIMPLE;
    return OK;
}

static int dynType_parseComplex(FILE* stream, dyn_type* type) {
    size_t nbEntries = 0;
    int status = OK;
    type->type = DYN_TYPE_COMPLEX;
    type->descriptor = '{';
    type->trivial = true;
    type->ffiType = &type->complex.structType;
    TAILQ_INIT(&type->complex.entriesHead);

    int c = fgetc(stream);
    struct complex_type_entry* entry = NULL;
    while (c != ' ' && c != '}') {
        ungetc(c,stream);
        celix_autoptr(dyn_type) subType = NULL;
        status = dynType_parseWithStreamOfName(stream, NULL, type, NULL, &subType, dynType_parseAny);
        if (status != OK) {
            return status;
        }
        entry = calloc(1, sizeof(*entry));
        if (entry == NULL) {
            celix_err_push("Error allocating memory for complex_type_entry");
            return MEM_ERROR;
        }
        entry->type = celix_steal_ptr(subType);
        if (!dynType_isTrivial(entry->type)) {
            type->trivial = false;
        }
        TAILQ_INSERT_TAIL(&type->complex.entriesHead, entry, entries);
        nbEntries += 1;
        c = fgetc(stream);
    }

    // loop over names
    entry = TAILQ_FIRST(&type->complex.entriesHead);
    char* name = NULL;
    // the current implementation permits trailing unnamed fields, i.e. number of names is less than number of fields
    while (c == ' ' && entry != NULL) {
        if ((status = dynCommon_parseName(stream, &name)) != OK) {
            return status;
        }
        entry->name = name;
        entry = TAILQ_NEXT(entry, entries);
        c = getc(stream);
    }
    if (c != '}') {
        celix_err_push("Error parsing complex type, expected '}'");
        return PARSE_ERROR;
    }

    type->complex.structType.type =  FFI_TYPE_STRUCT;
    type->complex.structType.elements = calloc(nbEntries + 1, sizeof(ffi_type*));
    if (type->complex.structType.elements == NULL) {
        celix_err_push("Error allocating memory for ffi_type elements");
        return MEM_ERROR;
    }
    type->complex.structType.elements[nbEntries] = NULL;
    int index = 0;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        type->complex.structType.elements[index++] = dynType_ffiType(entry->type);
    }

    type->complex.types = calloc(nbEntries, sizeof(dyn_type *));
    if (type->complex.types == NULL) {
        celix_err_pushf("Error allocating memory for complex types");
        return MEM_ERROR;
    }
    index = 0;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        type->complex.types[index++] = entry->type;
    }

    (void)ffi_get_struct_offsets(FFI_DEFAULT_ABI, type->ffiType, NULL);

    return status;
}

static int dynType_parseNestedType(FILE* stream, dyn_type* type) {
    int status = OK;
    celix_autofree char* name = NULL;
    celix_autoptr(dyn_type) subType = NULL;
    if ((status = dynCommon_parseName(stream, &name)) != OK) {
        return status;
    }
    if (dynCommon_eatChar(stream, '=') != OK) {
        return PARSE_ERROR;
    }
    if ((status = dynType_parseWithStreamOfName(stream, celix_steal_ptr(name), type, NULL, &subType, dynType_parseAny)) != OK) {
        return status;
    }
    if (dynCommon_eatChar(stream, ';') != OK) {
        return PARSE_ERROR;
    }
    struct type_entry* entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_err_pushf("Error allocating entry");
        return MEM_ERROR;
    }
    entry->type = celix_steal_ptr(subType);
    TAILQ_INSERT_TAIL(&type->nestedTypesHead, entry, entries);
    return OK;
}

static int dynType_parseReference(FILE* stream, dyn_type* type) {
    int status;
    type->type = DYN_TYPE_TYPED_POINTER;
    type->descriptor = '*';
    type->trivial = false;
    type->ffiType = &ffi_type_pointer;
    type->typedPointer.typedType =  NULL;

    if ((status = dynType_parseWithStreamOfName(stream, NULL, type, NULL,
                                                &type->typedPointer.typedType, dynType_parseRefByValue)) != OK) {
        return status;
    }
    return OK;
}

static int dynType_parseRefByValue(FILE* stream, dyn_type* type) {
    int status = OK;
    type->type = DYN_TYPE_REF;
    type->descriptor = 'l';

    celix_autofree char* name = NULL;
    if ((status = dynCommon_parseName(stream, &name)) != OK) {
        return status;
    }
    dyn_type* ref = dynType_findType(type, name);
    if (ref == NULL) {
        celix_err_pushf("Error cannot find type '%s'", name);
        return PARSE_ERROR;
    }
    type->ref.ref = ref;
    if (dynCommon_eatChar(stream, ';') != OK) {
        return PARSE_ERROR;
    }
    return OK;
}

static ffi_type* seq_types[] = {&ffi_type_uint32, &ffi_type_uint32, &ffi_type_pointer, NULL};

static int dynType_parseSequence(FILE* stream, dyn_type* type) {
    int status;
    type->type = DYN_TYPE_SEQUENCE;
    type->descriptor = '[';

    type->sequence.seqType.elements = seq_types;
    type->sequence.seqType.type = FFI_TYPE_STRUCT;
    type->sequence.seqType.size = 0;
    type->sequence.seqType.alignment = 0;

    status = dynType_parseWithStreamOfName(stream, NULL, type, NULL, &type->sequence.itemType, dynType_parseAny);
    if (status != OK) {
        return status;
    }

    type->ffiType = &type->sequence.seqType;
    (void)ffi_get_struct_offsets(FFI_DEFAULT_ABI, type->ffiType, NULL);
    return OK;
}

static int dynType_parseSimple(int c, dyn_type* type) {
    ffi_type* ffiType = dynType_ffiTypeFor(c);
    if (ffiType == NULL) {
        celix_err_pushf("Error unsupported type '%c'", c);
        return PARSE_ERROR;
    }
    type->type = DYN_TYPE_SIMPLE;
    type->descriptor = c;
    type->trivial = c != 'P';
    type->ffiType = ffiType;
    return OK;
}

static int dynType_parseTypedPointer(FILE* stream, dyn_type* type) {
    int status = OK;
    type->type = DYN_TYPE_TYPED_POINTER;
    type->descriptor = '*';
    type->trivial = false;
    type->ffiType = &ffi_type_pointer;

    status = dynType_parseWithStreamOfName(stream, NULL, type, NULL, &type->typedPointer.typedType, dynType_parseAny);

    return status;
}

void dynType_destroy(dyn_type* type) {
    if (type != NULL) {          
        dynType_clear(type);
        free(type);
    }
}

static void dynType_clear(dyn_type* type) {
    struct type_entry* entry = TAILQ_FIRST(&type->nestedTypesHead);
    struct type_entry* tmp = NULL;
    while (entry != NULL) {
        tmp = entry;
        entry = TAILQ_NEXT(entry, entries);
        dynType_destroy(tmp->type);
        free(tmp);
    }

    struct meta_entry* mEntry = TAILQ_FIRST(&type->metaProperties);;
    struct meta_entry* next = NULL;
    while (mEntry != NULL) {
        next = TAILQ_NEXT(mEntry, entries);
        if (mEntry != NULL) {
            free(mEntry->name);
            free(mEntry->value);
            free(mEntry);
        }
        mEntry = next;
    }

    switch (type->type) {
        case DYN_TYPE_COMPLEX :
            dynType_clearComplex(type);
            break;
        case DYN_TYPE_SEQUENCE :
            dynType_clearSequence(type);
            break;
        case DYN_TYPE_TYPED_POINTER :
            dynType_clearTypedPointer(type);
            break;
    } 

    if (type->name != NULL) {
        free(type->name);
    }
}

static void dynType_clearComplex(dyn_type* type) {
    assert(type->type == DYN_TYPE_COMPLEX);
    struct complex_type_entry* entry = TAILQ_FIRST(&type->complex.entriesHead);
    struct complex_type_entry* tmp = NULL;
    while (entry != NULL) {
        dynType_destroy(entry->type);
        if (entry->name != NULL) {
            free(entry->name);
        }
        tmp = entry;
        entry = TAILQ_NEXT(entry, entries);
        free(tmp);
    }
    if (type->complex.types != NULL) {
        free(type->complex.types);
    }
    if (type->complex.structType.elements != NULL) {
        free(type->complex.structType.elements);
    }
}

static void dynType_clearSequence(dyn_type* type) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    if (type->sequence.itemType != NULL) {
        dynType_destroy(type->sequence.itemType);
    }
}

static void dynType_clearTypedPointer(dyn_type* type) {
    assert(type->type == DYN_TYPE_TYPED_POINTER);
    if (type->typedPointer.typedType != NULL) {
        dynType_destroy(type->typedPointer.typedType);
    }
}

int dynType_alloc(const dyn_type* type, void** bufLoc) {
    const dyn_type* current = dynType_realType(type);

    void* inst = calloc(1, current->ffiType->size);
    if (inst == NULL) {
        celix_err_pushf("Error allocating memory for type '%c'", current->descriptor);
        return MEM_ERROR;
    }
    *bufLoc = inst;

    return OK;
}


int dynType_complex_indexForName(const dyn_type* type, const char* name) {
    assert(type->type == DYN_TYPE_COMPLEX);
    if (name == NULL) {
        return -1;
    }
    int i = 0;
    int index = -1;
    struct complex_type_entry* entry = NULL;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        if (entry->name != NULL && strcmp(name, entry->name) == 0) {
            index = i;
            break;
        }
        i +=1;
    }
    return index;
}

const dyn_type* dynType_complex_dynTypeAt(const dyn_type* type, int index) {
    assert(type->type == DYN_TYPE_COMPLEX);
    assert(index >= 0);
    return dynType_realType(type->complex.types[index]);
}

int dynType_complex_setValueAt(const dyn_type* type, int index, void* start, const void* in) {
    assert(type->type == DYN_TYPE_COMPLEX);
    char* loc = ((char*)start) + dynType_getOffset(type, index);
    size_t size = type->complex.structType.elements[index]->size;
    memcpy(loc, in, size);
    return 0;
}

void* dynType_complex_valLocAt(const dyn_type* type, int index, void* inst) {
    assert(type->type == DYN_TYPE_COMPLEX);
    char* l = (char*)inst;
    return (void* )(l + dynType_getOffset(type, index));
}

size_t dynType_complex_nrOfEntries(const dyn_type* type) {
    size_t count = 0;
    struct complex_type_entry* entry = NULL;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        ++count;
    }
    return count;
}

const struct complex_type_entries_head* dynType_complex_entries(const dyn_type* type) {
    assert(type->type == DYN_TYPE_COMPLEX);
    return &type->complex.entriesHead;
}

//sequence

void dynType_sequence_init(const dyn_type* type, void* inst) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    struct generic_sequence* seq = inst;
    seq->buf = NULL;
    seq->cap = 0;
    seq->len = 0;
}

int dynType_sequence_alloc(const dyn_type* type, void* inst, uint32_t cap) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    struct generic_sequence* seq = inst;
    if (seq == NULL) {
        celix_err_pushf("Error null sequence");
        return ERROR;
    }
    size_t size = dynType_size(type->sequence.itemType);
    seq->buf = calloc(cap, size);
    if (seq->buf == NULL) {
        seq->cap = 0;
        celix_err_pushf("Error allocating memory for seq buf");
        return MEM_ERROR;
    }
    seq->cap = cap;
    seq->len = 0;
    return OK;
}

int dynType_sequence_reserve(const dyn_type* type, void* inst, uint32_t cap) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    int status = OK;
    struct generic_sequence* seq = inst;
    if (seq == NULL) {
        celix_err_pushf("Error null sequence");
        return ERROR;
    }
    if (seq->cap >= cap) {
        return OK;
    }
    size_t size = dynType_size(type->sequence.itemType);
    seq->buf = realloc(seq->buf, (size_t)(cap * size));
    if (seq->buf == NULL) {
        seq->cap = 0;
        celix_err_pushf("Error allocating memory for seq buf");
        return MEM_ERROR;
    }
    memset(seq->buf+seq->cap*size, 0, (cap-seq->cap)*size);
    seq->cap = cap;
    return status;
}

void dynType_free(const dyn_type* type, void* loc) {
    dynType_deepFree(type, loc, true);
}

static void dynType_deepFree(const dyn_type* type, void* loc, bool alsoDeleteSelf) {
    if (loc != NULL) {
        const dyn_type* subType = NULL;
        char* text = NULL;
        type = dynType_realType(type);
        switch (type->type) {
            case DYN_TYPE_COMPLEX :
                dynType_freeComplexType(type, loc);
                break;
            case DYN_TYPE_SEQUENCE :
                dynType_freeSequenceType(type, loc);
                break;
            case DYN_TYPE_TYPED_POINTER:
                subType = dynType_typedPointer_getTypedType(type);
                void *ptrToType = *(void**)loc;
                dynType_deepFree(subType, ptrToType, true);
                break;
            case DYN_TYPE_TEXT :
                text = *(char**)loc;
                free(text);
                break;
            case DYN_TYPE_SIMPLE:
                //nop
                break;
//LCOV_EXCL_START
            default:
                assert(0 && "Unexpected switch case. cannot free dyn type");
                break;
//LCOV_EXCL_STOP
        }

        if (alsoDeleteSelf) {
            free(loc);
        }
    }
}

static void dynType_freeSequenceType(const dyn_type* type, void* seqLoc) {
    struct generic_sequence* seq = seqLoc;
    const dyn_type* itemType = dynType_sequence_itemType(type);
    void* itemLoc = NULL;
    uint32_t i;
    for (i = 0; i < seq->len; ++i) {
        dynType_sequence_locForIndex(type, seqLoc, i, &itemLoc);
        dynType_deepFree(itemType, itemLoc, false);
    }
    free(seq->buf);
}

static void dynType_freeComplexType(const dyn_type* type, void* loc) {
    struct complex_type_entry* entry = NULL;
    int index = 0;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        dynType_deepFree(entry->type, dynType_complex_valLocAt(type, index++, loc), false);
    }
}


uint32_t dynType_sequence_length(const void *seqLoc) {
    const struct generic_sequence* seq = seqLoc;
    return seq->len;
}

int dynType_sequence_locForIndex(const dyn_type* type, const void* seqLoc, uint32_t index, void** out) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    const struct generic_sequence* seq = seqLoc;

    size_t itemSize = dynType_size(type->sequence.itemType);

    if (index >= seq->cap) {
        celix_err_pushf("Requested index (%u) is greater than capacity (%u) of sequence", index, seq->cap);
        return ERROR;
    }

    if (index >= seq->len) {
        celix_err_pushf("Requesting index (%u) outsize defined length (%u) but within capacity", index, seq->len);
        return ERROR;
    }

    char* valLoc = seq->buf + (index * itemSize);
    (*out) = valLoc;

    return OK;
}

int dynType_sequence_increaseLengthAndReturnLastLoc(const dyn_type* type, void* seqLoc, void** valLoc) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    struct generic_sequence* seq = seqLoc;

    uint32_t lastIndex = seq->len;
    if (seq->len >= seq->cap) {
        celix_err_pushf("Cannot increase sequence length beyond capacity (%u)", seq->cap);
        return ERROR;
    }
    seq->len += 1;
    return dynType_sequence_locForIndex(type, seqLoc, lastIndex, valLoc);
}

const dyn_type* dynType_sequence_itemType(const dyn_type* type) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    return dynType_realType(type->sequence.itemType);
}

void dynType_simple_setValue(const dyn_type *type, void *inst, const void *in) {
    size_t size = dynType_size(type);
    memcpy(inst, in, size);
}

char dynType_descriptorType(const dyn_type* type) {
    return type->descriptor;
}

const char* dynType_getMetaInfo(const dyn_type* type, const char* name) {
    const char* result = NULL;
    struct meta_entry* entry = NULL;
    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        if (strcmp(entry->name, name) == 0) {
            result = entry->value;
            break;
        }
    }
    return result;
}

const struct meta_properties_head* dynType_metaEntries(const dyn_type* type) {
    return &type->metaProperties;
}

const char* dynType_getName(const dyn_type* type) {
    return type->name;
}

static ffi_type* dynType_ffiTypeFor(int c) {
    ffi_type* type = NULL;
    switch (c) {
        case 'Z' :
            type = &ffi_type_uint8;
            break;
        case 'F' :
            type = &ffi_type_float;
            break;
        case 'D' :
            type = &ffi_type_double;
            break;
        case 'B' :
            type = &ffi_type_sint8;
            break;
        case 'b' :
            type = &ffi_type_uint8;
            break;
        case 'S' :
            type = &ffi_type_sint16;
            break;
        case 's' :
            type = &ffi_type_uint16;
            break;
        case 'I' :
            type = &ffi_type_sint32;
            break;
        case 'i' :
            type = &ffi_type_uint32;
            break;
        case 'J' :
            type = &ffi_type_sint64;
            break;
        case 'j' :
            type = &ffi_type_uint64;
            break;
        case 'N' :
            type = &ffi_type_sint;
            break;
        case 'P' :
            type = &ffi_type_pointer;
            break;
        case 'V' :
            type = &ffi_type_void;
            break;
    }
    return type;
}

static unsigned short dynType_getOffset(const dyn_type* type, int index) {
    assert(type->type == DYN_TYPE_COMPLEX);
    unsigned short offset = 0;

    const ffi_type* ffiType = &type->complex.structType;
    int i;
    for (i = 0;  i <= index && ffiType->elements[i] != NULL; i += 1) {
        size_t size = ffiType->elements[i]->size;
        unsigned short alignment = ffiType->elements[i]->alignment;
        int alignment_diff = offset % alignment;
        if (alignment_diff > 0) {
            offset += (alignment - alignment_diff);
        }
        if (i < index) {
            offset += size;
        }
    }

    return offset;
}

size_t dynType_size(const dyn_type* type) {
    const dyn_type* rType = dynType_realType(type);
    return rType->ffiType->size;
}

int dynType_type(const dyn_type* type) {
    return type->type;
}

const dyn_type* dynType_realType(const dyn_type* type) {
    const dyn_type* real = type;
    while (real->type == DYN_TYPE_REF) {
        real= real->ref.ref;
    }
    return real;

}

const dyn_type* dynType_typedPointer_getTypedType(const dyn_type* type) {
    assert(type->type == DYN_TYPE_TYPED_POINTER);
    return dynType_realType(type->typedPointer.typedType);
}


int dynType_text_allocAndInit(const dyn_type* type, void* textLoc, const char* value) {
    assert(type->type == DYN_TYPE_TEXT);
    const char* str = strdup(value);
    char const** loc = textLoc;
    if (str == NULL) {
        celix_err_pushf("Cannot allocate memory for string");
        return MEM_ERROR;
    }
    *loc = str;
    return OK;
}

void dynType_print(const dyn_type *type, FILE *stream) {
    if (type != NULL) {
        dynType_printTypes(type, stream);

        fprintf(stream, "main type:\n");
        dynType_printAny("root", type, 0, stream);
    } else {
        fprintf(stream, "invalid type\n");
    }
}

static void dynType_printDepth(int depth, FILE *stream) {
    int i;
    for (i = 0; i < depth; i +=1 ) {
        fprintf(stream, "\t");
    }
}

static void dynType_printAny(const char* name, const dyn_type* type, int depth, FILE *stream) {
    const dyn_type* toPrint = dynType_realType(type);
    name = (name != NULL) ? name : "(unnamed)";
    switch(toPrint->type) {
        case DYN_TYPE_COMPLEX :
            dynType_printComplex(name, toPrint, depth, stream);
            break;
        case DYN_TYPE_SIMPLE :
            dynType_printSimple(name, toPrint, depth, stream);
            break;
        case DYN_TYPE_SEQUENCE :
            dynType_printSequence(name, toPrint, depth, stream);
            break;
        case DYN_TYPE_TYPED_POINTER :
            dynType_printTypedPointer(name, toPrint, depth, stream);
            break;
        case DYN_TYPE_TEXT:
            dynType_printText(name, toPrint, depth, stream);
            break;
//LCOV_EXCL_START
        default :
            assert(0 && "Unexpected switch case. cannot print dyn type");
            break;
//LCOV_EXCL_STOP
    }
}

static void dynType_printComplex(const char* name, const dyn_type* type, int depth, FILE *stream) {
    if (type->name == NULL) {
        dynType_printDepth(depth, stream);
        fprintf(stream, "%s: complex type (anon), size is %zu, alignment is %i, descriptor is '%c'. fields:\n",
                name,  type->ffiType->size, type->ffiType->alignment, type->descriptor);

        struct complex_type_entry* entry = NULL;
        TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
            dynType_printAny(entry->name, entry->type, depth + 1, stream);
        }

        dynType_printDepth(depth, stream);
        fprintf(stream, "}\n");
    } else {
        dynType_printDepth(depth, stream);
        fprintf(stream, "%s: complex type ('%s'), size is %zu, alignment is %i, descriptor is '%c'.\n",
                name, type->name, type->ffiType->size, type->ffiType->alignment, type->descriptor);
    }
}

static void dynType_printSequence(const char* name, const dyn_type* type, int depth, FILE* stream) {
    dynType_printDepth(depth, stream);
    fprintf(stream, "sequence, size is %zu, alignment is %i, descriptor is '%c'. fields:\n",
            type->ffiType->size, type->ffiType->alignment, type->descriptor);

    dynType_printDepth(depth + 1, stream);
    fprintf(stream, "cap: simple type, size is %zu, alignment is %i.\n",
            type->sequence.seqType.elements[0]->size, type->sequence.seqType.elements[0]->alignment);

    dynType_printDepth(depth + 1, stream);
    fprintf(stream, "len: simple type, size is %zu, alignment is %i.\n",
            type->sequence.seqType.elements[1]->size, type->sequence.seqType.elements[1]->alignment);

    dynType_printDepth(depth + 1, stream);
    fprintf(stream, "buf: array, size is %zu, alignment is %i. points to ->\n",
            type->sequence.seqType.elements[2]->size, type->sequence.seqType.elements[2]->alignment);
    dynType_printAny("element", type->sequence.itemType, depth + 1, stream);
}

static void dynType_printSimple(const char* name, const dyn_type* type, int depth, FILE* stream) {
    if (type->descriptor != 'E') {
        dynType_printDepth(depth, stream);
        fprintf(stream, "%s: simple type, size is %zu, alignment is %i, descriptor is '%c'.\n",
                name, type->ffiType->size, type->ffiType->alignment, type->descriptor);
    }
    else {
        dynType_printEnum(name, type, depth, stream);
    }
}

static void dynType_printEnum(const char* name, const dyn_type* type, int depth, FILE* stream) {
    dynType_printDepth(depth, stream);
    fprintf(stream, "%s: enum type, size is %zu, alignment is %i, descriptor is '%c'. values:",
            name, type->ffiType->size, type->ffiType->alignment, type->descriptor);
    struct meta_entry* m_entry;
    TAILQ_FOREACH(m_entry, &type->metaProperties, entries) {
        fprintf(stream, " (\"%s\":\"%s\")", m_entry->name, m_entry->value);
    }
    fprintf(stream, "\n");
}

static void dynType_printTypedPointer(const char* name, const dyn_type* type, int depth, FILE* stream) {
    dynType_printDepth(depth, stream);
    fprintf(stream, "%s: typed pointer, size is %zu, alignment is %i, points to ->\n",
            name, type->ffiType->size, type->ffiType->alignment);
    char* subName = NULL;
    char buf[128];
    memset(buf,0,128);
    if (name != NULL) {
        snprintf(buf, 128, "*%s", name);
        subName = buf;
    }
    dynType_printAny(subName, type->typedPointer.typedType, depth + 1, stream);
}

static void dynType_printText(const char* name, const dyn_type* type, int depth, FILE* stream) {
    dynType_printDepth(depth, stream);
    fprintf(stream, "%s: text type, size is %zu, alignment is %i, descriptor is '%c'.\n",
            name, type->ffiType->size, type->ffiType->alignment, type->descriptor);
}

static void dynType_printTypes(const dyn_type* type, FILE* stream) {
    if (type->type == DYN_TYPE_REF) {
        const dyn_type* real = dynType_realType(type->ref.ref);
        dyn_type* parent = type->parent;
        struct type_entry* pentry = NULL;
        while (parent != NULL) {
            TAILQ_FOREACH(pentry, &parent->nestedTypesHead, entries) {
                if (pentry->type == real) {
                    return;
                }
            }
            parent = parent->parent;
        }
    }

    struct type_entry* entry = NULL;
    TAILQ_FOREACH(entry, &type->nestedTypesHead, entries) {
        const dyn_type* toPrint = dynType_realType(entry->type);
        switch(toPrint->type) {
            case DYN_TYPE_COMPLEX :
                dynType_printComplexType(toPrint, stream);
                break;
            case DYN_TYPE_SIMPLE :
                dynType_printSimpleType(toPrint, stream);
                break;
            default :
                printf("TODO Print Type %d\n", toPrint->type);
                break;
        }
    }


    struct complex_type_entry* centry = NULL;
    switch(type->type) {
        case DYN_TYPE_COMPLEX :
            TAILQ_FOREACH(centry, &type->complex.entriesHead, entries) {
                dynType_printTypes(centry->type, stream);
            }
            break;
        case DYN_TYPE_SEQUENCE :
            dynType_printTypes(type->sequence.itemType, stream);
            break;
        case DYN_TYPE_TYPED_POINTER :
            dynType_printTypes(type->typedPointer.typedType, stream);
            break;
    }
}

static void dynType_printComplexType(const dyn_type *type, FILE *stream) {
    fprintf(stream, "type '%s': complex type, size is %zu, alignment is %i, descriptor is '%c'. fields:\n", type->name,  type->ffiType->size, type->ffiType->alignment, type->descriptor);

    struct complex_type_entry *entry = NULL;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        dynType_printAny(entry->name, entry->type, 2, stream);
    }

    fprintf(stream, "}\n");
}

static void dynType_printSimpleType(const dyn_type *type, FILE *stream) {
    fprintf(stream, "type '%s': simple type, size is %zu, alignment is %i, descriptor is '%c'\n", type->name, type->ffiType->size, type->ffiType->alignment, type->descriptor);
}

bool dynType_isTrivial(const dyn_type* type) {
    return dynType_realType(type)->trivial;
}

