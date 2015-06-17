#include "dyn_type.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>


//place in dyn_type.h?
struct generic_sequence {
    uint32_t _cap;
    uint32_t _len;
    void *buf;
};


static char dynType_schemaType(dyn_type *dynType, ffi_type *ffiType);

static void dynType_print_sequence(dyn_type *type, int depth);
static void dynType_print_complex(dyn_type *type, int depth);
static void dynType_print_any(dyn_type *type, int depth);

static unsigned short get_offset_at(ffi_type *type, int index);
static void dynType_trim(const char *str, size_t len, int *start, int *end);

static int dynType_complex_create(char *schema, dyn_type **result);
static int dynType_sequence_create(char *schema, dyn_type **result); 
static int dynType_simple_create(char t, dyn_type **result);

static ffi_type* dynType_ffiType_for(char c);

int dynType_create(const char *schema, dyn_type **result) {
    //trim white space
    int status = 0;
    int offset, len;
    dynType_trim(schema, strlen(schema), &offset, &len); 

    /*
       char test[len+1];
       memcpy(test, schema + offset, len);
       test[len] = '\0';
       printf("trimmed schema is %s\n", test);
       */

    if (schema[offset] == '{' && schema[offset + len -1] == '}') {
        //complex

        //removing prefix { and postfix }  
        offset += 1; 
        len -= 2; 

        int offsetType;
        int lenType;
        //trim again
        dynType_trim(schema + offset, len , &offsetType, &lenType);

        char type[lenType + 1];
        memcpy(type, schema + offset + offsetType, lenType);
        type[lenType] = '\0';
        status = dynType_complex_create(type, result);
    } else if (schema[offset] == '[') {
        //sequence
        //fixme assuming simple type
        offset += 1; //remove [
        len -= 1;
        char type[len + 1];
        memcpy(type, schema + offset, len);
        type[len] = '\0';
        status = dynType_sequence_create(type, result);
    } else if (len == 1){
        //simple
        status = dynType_simple_create(schema[offset], result);
    } else {
        printf("invalid schema '%s'\n", schema);
    }

    return status;
}

static int dynType_simple_create(char t, dyn_type **result) {
    int status = 0;
    ffi_type *ffiType = dynType_ffiType_for(t); 
    dyn_type *simple = NULL;
    if (ffiType != NULL) {
        simple = calloc(1,sizeof(*simple));
        simple->type = DYN_TYPE_SIMPLE;
        simple->simple.ffiType = ffiType;
    }
    if (ffiType != NULL && simple != NULL) {
        *result = simple;
    } else {
        printf("Error creating simple dyn type for '%c'\n", t);
        status = 1;
    }
    return status;
}

static int dynType_sequence_create(char *schema, dyn_type **result) {
    int status = 0;
    dyn_type *type = calloc(1, sizeof(*type));

    type->type = DYN_TYPE_SEQUENCE;
    type->sequence.seqStruct.elements = calloc(4, sizeof(ffi_type));
    type->sequence.seqStruct.elements[0] =  &ffi_type_uint32; //_cap
    type->sequence.seqStruct.elements[1] =  &ffi_type_uint32; //_len
    type->sequence.seqStruct.elements[2] =  &ffi_type_pointer; 
    type->sequence.seqStruct.elements[3] =  NULL;

    if (schema[0] == '{') { //complex type
        int len = strlen(schema) -2;
        char complexSchema[len + 1];
        memcpy(complexSchema, schema + 1, len);
        complexSchema[len] = '\0';
        status = dynType_complex_create(complexSchema, &type->sequence.dynType);
    } else { //simple type
        type->sequence.simpleType = dynType_ffiType_for(schema[0]);
    } 
    //TODO sequence of sequence

    if (status == 0) {
        //dummy cif prep to ensure size for struct ffi_type is set
        ffi_cif cif;
        ffi_type *args[1];
        args[0] = &type->sequence.seqStruct;
        ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args);

        *result = type;
        return 0;
    } else {
        printf("DYN_TYPE: invalid type schema\n");
        return 1;
    }

    return status;
} 

static int dynType_complex_create(char *schema, dyn_type **result) {
    bool valid = true;
    size_t len = strlen(schema);
    int i = 0;

    dyn_type *type = calloc(1, sizeof(*type));
    type->type = DYN_TYPE_COMPLEX;
    if (type == NULL) {
        return -1;
    }

    //count elements
    int k;
    size_t count = 0;
    for (k=i; k < len && !isspace(schema[k]); k += 1) {
        if (schema[k] == '[') {
            k += 1;
        } else if (schema[k] == '{') {
            for (; schema[k] != '}'; k += 1) ;
        }
        count += 1;
    }

    type->complex.ffiType.elements = calloc(count + 1, sizeof(ffi_type));
    type->complex.nested_types = calloc(count + 1, sizeof(dyn_type *));
    int index = 0;
    for (k=0; k+i < len && !isspace(schema[k+i]); k += 1, index += 1) {
        if (schema[k+i] == '[') { //sequence
            //FIXME assume simple type
            dyn_type *seqType;
            char seq[2];
            seq[0] = schema[k+i+1];
            seq[1] = '\0';
            dynType_sequence_create(seq, &seqType);

            type->complex.nested_types[index] = seqType;
            type->complex.ffiType.elements[index] = &seqType->sequence.seqStruct;
            k += 1;
        } else if (schema[k+i] == '{') { //nested complex
            int start = k+i;
            int end;
            for (; schema[k+i] != '}' && k+i < len; k +=1) ;
            if (schema[k+i] == '}') {
                end = k+i;
                size_t len = end-start + 1 -2; //without {}
                char nested[len+1];
                memcpy(nested, schema + start + 1, len);
                nested[len] = '\0';
                dyn_type *nestedType = NULL;
                //TODO catch nested problem
                dynType_complex_create(nested, &nestedType); //TODO use status result
                type->complex.nested_types[index] = nestedType;
                type->complex.ffiType.elements[index] = &ffi_type_pointer;
            } else {
                printf("invalid schema. object not closed '%s'\n", schema);
            }
        } else {
            ffi_type *elType = NULL;
            elType = dynType_ffiType_for(schema[k+i]);
            if (elType != NULL) {
                type->complex.ffiType.elements[index] = elType;
            } else {
                valid=false;
            }
        }
    }


    //eat { and }
    int j;
    int nrOfBraces = 0;
    for (j = 0; j < len; j += 1) { //TODO make safer
        if (isspace(schema[j]) && nrOfBraces == 0) {
                break;
        }
        if (schema[j] == '{') {
            nrOfBraces += 1;
        } else if (schema[j] == '}') {
            nrOfBraces -=1 ;
        }
    }

    char *names = schema + j +1;

    type->complex.names = calloc(count, sizeof(char *));
    k = 0;
    char *savePtr;
    char *token = NULL;
    token = strtok_r(names, " ", &savePtr); 
    while (token != NULL && k < count) {
        type->complex.names[k] = strdup(token);
        k += 1;
        token = strtok_r(NULL, " ", &savePtr);
    }
    if ( k != count || token != NULL) {
        valid =false;
    }

    if (valid) {
        //dummy cif prep to ensure size for struct ffi_type is set
        ffi_cif cif;
        ffi_type *args[1];
        args[0] = &type->complex.ffiType;
        ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args);

        *result = type;
        return 0;
    } else {
        printf("DYN_TYPE: invalid type schema '%s'\n", schema);
        return 1;
    }
}

int dynType_destroy(dyn_type *type) {
    //TODO
    return 0;
}

void * dynType_alloc(dyn_type *type) {
    assert(type != NULL);
    void *result = NULL;
    switch(type->type) {
        case DYN_TYPE_COMPLEX :
            result = calloc(1, type->complex.ffiType.size);
            break;
        case DYN_TYPE_SEQUENCE :
            result = calloc(1, type->sequence.seqStruct.size);
            break;
        case DYN_TYPE_SIMPLE :
            result = calloc(1, type->simple.ffiType->size);
            break;
        default :
            printf("DYN_TYPE: unsupported type %i\n", type->type);
            break;
    }
    return result;
}

static ffi_type* dynType_ffiType_for(char c) {
    ffi_type *type = NULL;
    switch (c) {
        case 'F' :
            type = &ffi_type_float;
            break;
        case 'D' :
            type = &ffi_type_double;
            break;
        case 'B' :
            type = &ffi_type_sint8;
            break;
        case 'S' :
            type = &ffi_type_sint16;
            break;
        case 'I' :
            type = &ffi_type_sint32;
            break;
        case 'J' :
            type = &ffi_type_sint64;
            break;
        default:
            printf("unsupported type %c\n", c);
    }
    return type;
}

static unsigned short get_offset_at(ffi_type *type, int index) {
    unsigned short offset = 0;

    int i;
    for (i = 0;  i <= index && type->elements[i] != NULL; i += 1) {
        size_t size = type->elements[i]->size;
        unsigned short alignment = type->elements[i]->alignment;
        int alignment_diff = offset % alignment;
        if (alignment_diff > 0) {
            offset += (alignment - alignment_diff);
        }
        if (i < index) {
            offset += size;
        }
    }

    //TODO check if last element is not reached

    return offset;
}

int dynType_complex_set_value_at(dyn_type *type, int index, void *start, void *in) {
    assert(type->type == DYN_TYPE_COMPLEX);
    char *loc = ((char *)start) + get_offset_at(&type->complex.ffiType, index);
    size_t size = type->complex.ffiType.elements[index]->size;
    memcpy(loc, in, size);
    return 0;
}

int dynType_complex_index_for_name(dyn_type *type, const char *name) {
    assert(type->type == DYN_TYPE_COMPLEX);
    int i;
    int index = -1;
    for (i = 0; type->complex.ffiType.elements[i] != NULL; i += 1) {
        if (strcmp(type->complex.names[i], name) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

char dynType_complex_schemaType_at(dyn_type *complexType, int index) {
    assert(complexType->type == DYN_TYPE_COMPLEX);

    ffi_type *type = complexType->complex.ffiType.elements[index];
    dyn_type *nestedType = complexType->complex.nested_types[index];

    return dynType_schemaType(nestedType, type);
}

static char dynType_schemaType(dyn_type *dynType, ffi_type *ffiType) {
    char result = '\0';
    if (dynType != NULL) {
        switch(dynType->type) {
            case DYN_TYPE_SIMPLE :
                result = dynType_schemaType(NULL, dynType->simple.ffiType);
                break;
            case DYN_TYPE_COMPLEX :
                result = '{';
                break;
            case DYN_TYPE_SEQUENCE :
                result = '[';
                break;
        }
    } else if (ffiType != NULL) {
        switch(ffiType->type) {
            case FFI_TYPE_FLOAT :
                result = 'F';
                break;
            case FFI_TYPE_DOUBLE :
                result = 'D';
                break;
            case FFI_TYPE_SINT8 :
                result = 'B';
                break;
            case FFI_TYPE_SINT16 :
                result = 'S';
                break;
            case FFI_TYPE_SINT32 :
                result = 'I';
                break;
            case FFI_TYPE_SINT64 :
                result = 'J';
                break;
            default :
                printf("Unsurported ffi type encounted. ffi type is %u\n", ffiType->type);
        }
    } 
    return result;
}

dyn_type * dynType_complex_dynType_at(dyn_type *type, int index) {
    assert(type->type == DYN_TYPE_COMPLEX);
    return type->complex.nested_types[index];
}

void * dynType_complex_val_loc_at(dyn_type *type, void *inst_loc, int index) {
    assert(type->type == DYN_TYPE_COMPLEX);
    char *l = inst_loc;
    return (void *)(l + get_offset_at(&type->complex.ffiType, index));
}

int dynType_simple_set_value(dyn_type *type, void *typeLoc, void *in) {
    //TODO
    return 0;
}

static void dynType_print_sequence(dyn_type *type, int depth) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    static const char const *SEQ_NAMES[] = {"_cap", "_len", "buf"};

    int i;
    for (i = 0; i < 3 ; i += 1) {
        int k;
        for (k = 0; k < depth; k += 1) { 
            printf("\t");
        }
        printf("element %i with name '%s': size is %zu, allignment is %i and offset is %i\n", i, 
                SEQ_NAMES[i], type->sequence.seqStruct.elements[i]->size, type->sequence.seqStruct.elements[i]->alignment, get_offset_at(&type->sequence.seqStruct, i));
    }
}

static void dynType_print_complex(dyn_type *type, int depth) {
    assert(type->type == DYN_TYPE_COMPLEX);
    int i, k;
    for (k = 0; k < depth; k += 1) { 
        printf("\t");
    }
    printf("printing ffi type. size is %zu, allignment is %i :\n", type->complex.ffiType.size, type->complex.ffiType.alignment);
    for (i = 0; type->complex.ffiType.elements[i] != NULL; i += 1) {
        for (k = 0; k < depth; k += 1) { 
            printf("\t");
        }
        printf(" element %i with name '%s': size is %zu, allignment is %i and offset is %i\n", i, 
                type->complex.names[i], type->complex.ffiType.elements[i]->size, type->complex.ffiType.elements[i]->alignment, get_offset_at(&type->complex.ffiType, i));
        if (type->complex.nested_types[i] != NULL) { 
            dynType_print_any(type->complex.nested_types[i], depth + 1);
        }
    }
}

static void dynType_print_any(dyn_type *type, int depth) {
    switch (type->type) {
        case DYN_TYPE_COMPLEX :
            dynType_print_complex(type, depth);
            break;
        case DYN_TYPE_SEQUENCE :
            dynType_print_sequence(type, depth);
            break;
        default :
            printf("Unsurported dyn type %i\n", type->type);
    }
}

void dynType_print(dyn_type *type) {
    dynType_print_any(type, 0);
}

size_t dynType_size(dyn_type *type) {
    assert(type != NULL);
    size_t size = 0;
    switch (type->type) {
        case DYN_TYPE_COMPLEX :
            size = type->complex.ffiType.size;
            break;
        case DYN_TYPE_SEQUENCE :
            size = type->sequence.seqStruct.size;
            break;
        case DYN_TYPE_SIMPLE :
            size = type->simple.ffiType->size;
            break;
        default :
            printf("Error unsupported type %i\n", type->type);
            break;
    }
    return size;
}

int dynType_type(dyn_type *type) {
    return type->type;
}

static void dynType_trim(const char *str, size_t strLen, int *offset, int *len) {
    int i;
    for (i = 0; i < strLen && isspace(str[i]); i += 1) {
        ;
    }
    *offset = i;

    for (i = strLen-1; i >= 0 && isspace(str[i]); i -= 1) {
        ;
    }
    *len = i -  (*offset) + 1;
}


void* dynType_sequence_init(dyn_type *type, void *inst, size_t cap) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    char *result;
    struct generic_sequence *seq = inst;
    if (seq != NULL) {
        size_t size = type->sequence.simpleType != NULL ? type->sequence.simpleType->size : dynType_size(type->sequence.dynType);
        seq->buf = calloc(cap, size);
        seq->_cap = cap;
        result = seq->buf;
    }
    return result;
}

char dynType_sequence_elementSchemaType(dyn_type *type) {
    assert(type->type == DYN_TYPE_SEQUENCE);
    return dynType_schemaType(type->sequence.dynType, type->sequence.simpleType);
}

int dynType_sequence_append(dyn_type *type, void *inst, void *in) {
    int index = -1;
    struct generic_sequence *seq = inst;
    if (seq->_len + 1 <= seq->_cap) {
        index = seq->_len;
        seq->_len += 1;
        char *buf = seq->buf;
        size_t elSize = type->sequence.dynType != NULL ? dynType_size(type->sequence.dynType) : type->sequence.simpleType->size;
        size_t offset = (elSize * index);
        memcpy(buf + offset, in, elSize);
    } else {
        printf("DYN_TYPE: sequence out of capacity\n");
        //todo resize ?
    }
    return index;
}
