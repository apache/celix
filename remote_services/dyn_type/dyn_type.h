#ifndef __DYN_TYPE_H_
#define __DYN_TYPE_H_

#include <ffi.h>


/*Schema description
 *
 * Type = SimpleType | ComplexType | ArrayType
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
 * T char* string
 *
 *
 * ComplexTypes
 * {[Type]+ [(Name)(SPACE)]+}
 *
 * NOTE MAYBE SUPPORT STRUCT BY VALUE ->
 * <[Type]+ [(Name)(SPACE)]+>
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

#define DYN_TYPE_SIMPLE 1
#define DYN_TYPE_COMPLEX 2
#define DYN_TYPE_SEQUENCE 3

typedef struct _dyn_type dyn_type;

/* TODO MOVE TO PRIVATE HEADER */
struct _dyn_type {
    int type;
    union {
        struct {
            ffi_type *ffiType;
        } simple;

        struct {
            ffi_type ffiType;
            char **names; 
            dyn_type **nested_types; 
        } complex;

        struct {
            ffi_type seqStruct;
            dyn_type *dynType; //set if sequence is of a complex type
            ffi_type *simpleType; //set if sequence is of a simple type
        } sequence;
    };
};


int dynType_create(const char *schema, dyn_type **type);
int dynType_destroy(dyn_type *type);

//generic
void dynType_print(dyn_type *type);
size_t dynType_size(dyn_type *type);
int dynType_type(dyn_type *type);
void * dynType_alloc(dyn_type *type); 
//TODO void dynType_free(dyn_type *type, void *inst);

//complex
int dynType_complex_index_for_name(dyn_type *type, const char *name);
char dynType_complex_schemaType_at(dyn_type *type, int index);
dyn_type * dynType_complex_dynType_at(dyn_type *type, int index);
void * dynType_complex_val_loc_at(dyn_type *type, void *inst_loc, int index);
int dynType_complex_set_value_at(dyn_type *type, int index, void *start_loc_type, void *in);

//sequence
void * dynType_sequence_init(dyn_type *type, void *seqInst, size_t cap); //note assume seq struct at seqLoc. only allocates buf
char dynType_sequence_elementSchemaType(dyn_type *type);
int dynType_sequence_append(dyn_type *type, void *inst, void *in);

//simple
int dynType_simple_set_value(dyn_type *type, void *typeLoc, void *in);


#endif
