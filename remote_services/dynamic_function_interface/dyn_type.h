/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */

#ifndef _DYN_TYPE_H_
#define _DYN_TYPE_H_

#include <stdio.h>
#include <sys/queue.h>
#include <stdbool.h>

#include <ffi.h>

#include "dfi_log_util.h"

#if defined(BSD) || defined(__APPLE__) 
#include "open_memstream.h"
#include "fmemopen.h"
#endif

/* Description string
 *
 * Type = [TypeDef]* (SimpleType | ComplexType | SequenceType | TypedPointer | PointerReference ) [TypeDef]*
 * Name = alpha[(alpha|numeric)*]
 * SPACE = ' ' 
 *
 * SimplesTypes (based on java bytecode method signatures)
 * //Java based:
 * B char
 * C (not supported)
 * D double
 * F float
 * I int //TODO use int32_t instead?
 * J long //TODO use int64_t intead?
 * S short //TODO use int16_t instead?
 * V void
 * Z boolean
 * //Extended
 * b unsigned char
 * i unsigned int (see I)
 * j unsigned long (see J)
 * s unsigned short (see S)
 * P pointer
 * t char* string
 * N native int
 *
 *
 * ComplexTypes (Struct)
 * {[Type]+ [(Name)(SPACE)]+}
 *
 * PointerReference
 * L(Name);
 *
 * ReferenceByValue
 * l(name);
 *
 * TypeDef 
 * T(Name)=Type;
 *
 * SequenceType
 * [(Type)
 *
 * examples
 * "{DDII a b c d}" -> struct { double a; double b; int c; int d; }; 
 * "{DD{FF c1 c2} a b c" -> struct { double a; double b; struct c { float c1; float c2; }; }; 
 *
 *
 */

#define DYN_TYPE_INVALID 0
#define DYN_TYPE_SIMPLE 1
#define DYN_TYPE_COMPLEX 2
#define DYN_TYPE_SEQUENCE 3
#define DYN_TYPE_TYPED_POINTER 4
#define DYN_TYPE_REF 5

typedef struct _dyn_type dyn_type;

struct _dyn_type {
    char *name;
    char descriptor;
    int type;
    ffi_type *ffiType;
    dyn_type *parent;
    TAILQ_HEAD(, nested_entry) nestedTypesHead;
    union {
        struct {
            TAILQ_HEAD(, complex_type_entry) entriesHead;
            ffi_type structType; //dyn_type.ffiType points to this
            dyn_type **types; //based on entriesHead for fast access
        } complex;
        struct {
            ffi_type seqType; //dyn_type.ffiType points to this
            dyn_type *itemType; 
        } sequence;
        struct {
            dyn_type *typedType;
        } typedPointer;
        struct {
            dyn_type *ref;
        } ref;
    };
};

struct complex_type_entry {
    dyn_type type;
    char *name;
    TAILQ_ENTRY(complex_type_entry) entries;
};

struct nested_entry {
    dyn_type type;
    TAILQ_ENTRY(nested_entry) entries;
};


//logging
DFI_SETUP_LOG_HEADER(dynType);

//generic
int dynType_create(const char *descriptor, dyn_type **type);
void dynType_destroy(dyn_type *type);
int dynType_alloc(dyn_type *type, void **bufLoc);
void dynType_print(dyn_type *type);
size_t dynType_size(dyn_type *type);
int dynType_type(dyn_type *type);
int dynType_descriptorType(dyn_type *type);

//complexType
int dynType_complex_indexForName(dyn_type *type, const char *name);
char dynType_complex_descriptorTypeAt(dyn_type *type, int index);
int dynType_complex_dynTypeAt(dyn_type *type, int index, dyn_type **subType);
int dynType_complex_setValueAt(dyn_type *type, int index, void *inst, void *in);
int dynType_complex_valLocAt(dyn_type *type, int index, void *inst, void **valLoc);

//sequence
int dynType_sequence_alloc(dyn_type *type, void *inst, int cap, void **buf);
int dynType_sequence_append(dyn_type *type, void *seq, void *in);
dyn_type * dynType_sequence_itemType(dyn_type *type);

//simple
void dynType_simple_setValue(dyn_type *type, void *inst, void *in);

#endif
