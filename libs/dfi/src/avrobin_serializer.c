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
#include "avrobin_serializer.h"
#include "dyn_type_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <jansson.h>

#define MAX_VARINT_BUF_SIZE 10

static int generate_sync(uint8_t **result);
static int generate_record_name(char **result);

static int avrobin_read_boolean(FILE *stream,bool *val);
static int avrobin_read_int(FILE *stream,int32_t *val);
static int avrobin_read_long(FILE *stream,int64_t *val);
static int avrobin_read_float(FILE *stream,float *val);
static int avrobin_read_double(FILE *stream,double *val);
static int avrobin_read_string(FILE *stream,char **val);

static int avrobin_write_boolean(FILE *stream,bool val);
static int avrobin_write_int(FILE *stream,int32_t val);
static int avrobin_write_long(FILE *stream,int64_t val);
static int avrobin_write_float(FILE *stream,float val);
static int avrobin_write_double(FILE *stream,double val);
static int avrobin_write_string(FILE *stream,const char *val);

static int avrobin_schema_primitive(const char *tname, json_t **output);

static int avrobinSerializer_createType(dyn_type *type, FILE *stream, void **result);
static int avrobinSerializer_parseAny(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_parseComplex(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_parseSequence(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_parseEnum(dyn_type *type, void *loc, FILE *stream);

static int avrobinSerializer_writeAny(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_writeComplex(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_writeSequence(dyn_type *type, void *loc, FILE *stream);
static int avrobinSerializer_writeEnum(dyn_type *type, void *loc, FILE *stream);

static int avrobinSerializer_generateAny(dyn_type *type, json_t **output);
static int avrobinSerializer_generateComplex(dyn_type *type, json_t **output);
static int avrobinSerializer_generateSequence(dyn_type *type, json_t **output);
static int avrobinSerializer_generateEnum(dyn_type *type, json_t **output);

static const int OK = 0;
static const int ERROR = 1;

static uint32_t record_name_counter;

DFI_SETUP_LOG(avrobinSerializer);

int avrobinSerializer_deserialize(dyn_type *type, const uint8_t *input, size_t inlen, void **result) {
    int status = OK;

    FILE *stream = fmemopen((void*)input, inlen, "rb");

    if (stream != NULL) {
        status = avrobinSerializer_createType(type, stream, result);

        fclose(stream);

        if (status != OK) {
            LOG_ERROR("Error cannot deserialize avrobin.");
        }
    } else {
        status = ERROR;
        LOG_ERROR("Error initializing memory stream for reading. Length was %d.", inlen);
    }

    return status;
}

int avrobinSerializer_serialize(dyn_type *type, const void *input, uint8_t **output, size_t *outlen) {
    int status = OK;

    FILE *stream = open_memstream((char**)output, outlen);

    if (stream != NULL) {
        status = avrobinSerializer_writeAny(type, (void*)input, stream);

        fclose(stream);

        if (status != OK) {
            free(*output);

            LOG_ERROR("Error cannot serialize avrobin.");
        }
    } else {
        status = ERROR;
        LOG_ERROR("Error initializing memory stream for writing.");
    }

    return status;
}

int avrobinSerializer_generateSchema(dyn_type *type, char **output) {
    int status = OK;

    record_name_counter = 1;

    json_t *root = NULL;
    status = avrobinSerializer_generateAny(type, &root);

    if (status == OK) {
        *output = json_dumps(root, JSON_COMPACT | JSON_PRESERVE_ORDER);

        if (*output == NULL) {
            status = ERROR;
        }

        json_decref(root);
    }

    return status;
}

int avrobinSerializer_saveFile(const char *filename, const char *schema, const uint8_t *serdata, size_t serdatalen) {
    int status = OK;

    FILE *file = fopen(filename, "wb");

    if (file != NULL) {
        if (status == OK) {
            if (fputc('O', file) == EOF) {
                status = ERROR;
            }
        }
        if (status == OK) {
            if (fputc('b', file) == EOF) {
                status = ERROR;
            }
        }
        if (status == OK) {
            if (fputc('j', file) == EOF) {
                status = ERROR;
            }
        }
        if (status == OK) {
            if (fputc(1, file) == EOF) {
                status = ERROR;
            }
        }
        if (status == OK) {
            status = avrobin_write_long(file, 1);
        }
        if (status == OK) {
            status = avrobin_write_string(file, "avro.schema");
        }
        if (status == OK) {
            status = avrobin_write_string(file, schema);
        }
        if (status == OK) {
            status = avrobin_write_long(file, 0);
        }
        if (status == OK) {
            uint8_t *sync = NULL;
            status = generate_sync(&sync);
            if (status == OK) {
                for (int i=0; i<16; i++) {
                    if (status == OK) {
                        if (fputc(sync[i], file) == EOF) {
                            status = ERROR;
                        }
                    }
                    if (status != OK) break;
                }
                if (status == OK) {
                    status = avrobin_write_long(file, 1);
                }
                if (status == OK) {
                    status = avrobin_write_long(file, serdatalen);
                }
                if (status == OK) {
                    for (int i=0; i<serdatalen; i++) {
                        if (status == OK) {
                            if (fputc(serdata[i], file) == EOF) {
                                status = ERROR;
                            }
                        }
                        if (status != OK) break;
                    }
                }
                for (int i=0; i<16; i++) {
                    if (status == OK) {
                        if (fputc(sync[i], file) == EOF) {
                            status = ERROR;
                        }
                    }
                    if (status != OK) break;
                }

                free(sync);
            }
        }

        fclose(file);
    } else {
        status = ERROR;
    }

    return status;
}

static int avrobinSerializer_createType(dyn_type *type, FILE *stream, void **result) {
    int status = OK;
    void *inst = NULL;

    status = dynType_alloc(type, &inst);

    if (status == OK) {
        assert(inst != NULL);
        status = avrobinSerializer_parseAny(type, inst, stream);

        if (status == OK) {
            *result = inst;
        }
        else {
            dynType_free(type, inst);
        }
    }

    return status;
}

static int avrobinSerializer_parseAny(dyn_type *type, void *loc, FILE *stream) {
    int status = OK;

    dyn_type *subType = NULL;
    char c = dynType_descriptorType(type);

    bool *z;            //Z
    float *f;           //F
    double *d;          //D
    char *b;            //B
    int *n;             //N
    int16_t *s;         //S
    int32_t *i;         //I
    int64_t *l;         //J
    uint8_t   *ub;      //b
    uint16_t  *us;      //s
    uint32_t  *ui;      //i
    uint64_t  *ul;      //j

    bool avro_boolean;
    int32_t avro_int;
    int64_t avro_long;
    float avro_float;
    double avro_double;
    char *avro_string;

    switch (c) {
        case 'Z' :
            z = loc;
            status = avrobin_read_boolean(stream,&avro_boolean);
            if (status == OK) {
                *z = avro_boolean;
            }
            break;
        case 'F' :
            f = loc;
            status = avrobin_read_float(stream,&avro_float);
            if (status == OK) {
                *f = avro_float;
            }
            break;
        case 'D' :
            d = loc;
            status = avrobin_read_double(stream,&avro_double);
            if (status == OK) {
                *d = avro_double;
            }
            break;
        case 'N' :
            n = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *n = (int)avro_int;
            }
            break;
        case 'B' :
            b = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *b = (char)avro_int;
            }
            break;
        case 'S' :
            s = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *s = (int16_t)avro_int;
            }
            break;
        case 'I' :
            i = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *i = avro_int;
            }
            break;
        case 'J' :
            l = loc;
            status = avrobin_read_long(stream,&avro_long);
            if (status == OK) {
                *l = avro_long;
            }
            break;
        case 'b' :
            ub = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *ub = (uint8_t)avro_int;
            }
            break;
        case 's' :
            us = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *us = (uint16_t)avro_int;
            }
            break;
        case 'i' :
            ui = loc;
            status = avrobin_read_int(stream,&avro_int);
            if (status == OK) {
                *ui = (uint32_t)avro_int;
            }
            break;
        case 'j' :
            ul = loc;
            status = avrobin_read_long(stream,&avro_long);
            if (status == OK) {
                *ul = (uint64_t)avro_long;
            }
            break;
        case 't' :
            status = avrobin_read_string(stream,&avro_string);
            if (status == OK) {
                status = dynType_text_allocAndInit(type, loc, avro_string);
                free(avro_string);
            }
            break;
        case '[' :
            if (status == OK) {
                status = avrobinSerializer_parseSequence(type, loc, stream);
            }
            break;
        case '{' :
            if (status == OK) {
                status = avrobinSerializer_parseComplex(type, loc, stream);
            }
            break;
        case '*' :
            status = dynType_typedPointer_getTypedType(type, &subType);
            if (status == OK) {
                status = avrobinSerializer_createType(subType, stream, (void**)loc);
            }
            break;
        case 'E' :
            if (status == OK) {
                status = avrobinSerializer_parseEnum(type, loc, stream);
            }
            break;
        case 'l':
            status = avrobinSerializer_parseAny(type->ref.ref, loc, stream);
            break;
        case 'P' :
            status = ERROR;
            LOG_WARNING("Untyped pointers are not supported for serialization.");
            break;
        default :
            status = ERROR;
            LOG_ERROR("Error provided type '%c' not supported for AVRO.", dynType_descriptorType(type));
            break;
    }

    return status;
}

static int avrobinSerializer_parseComplex(dyn_type *type, void *loc, FILE *stream) {
    int status = OK;

    struct complex_type_entry *entry = NULL;
    struct complex_type_entries_head *entries = NULL;
    dyn_type *subType = NULL;
    void *subLoc = NULL;
    int index = -1;

    status = dynType_complex_entries(type, &entries);

    if (status == OK) {
        TAILQ_FOREACH(entry, entries, entries) {
            index = dynType_complex_indexForName(type, entry->name);

            if (index < 0) {
                status = ERROR;
                LOG_ERROR("Cannot find index for member '%s'.", entry->name);
            }

            if (status == OK) {
                status = dynType_complex_dynTypeAt(type, index, &subType);
            }

            if (status == OK) {
                status = dynType_complex_valLocAt(type, index, loc, &subLoc);
            }

            if (status == OK) {
                status = avrobinSerializer_parseAny(subType, subLoc, stream);
            }

            if (status != OK) {
                break;
            }
        }
    }

    return status;
}

static int avrobinSerializer_parseSequence(dyn_type *type, void *loc, FILE *stream) {
    /* Avro 1.8.1 Specification
     * Arrays
     * Arrays are encoded as a series of blocks. Each block consists of a long count value, followed by that many array items. A block with count zero indicates the end of the array. Each item is encoded per the array's item schema.
     * If a block's count is negative, its absolute value is used, and the count is followed immediately by a long block size indicating the number of bytes in the block. This block size permits fast skipping through data, e.g., when projecting a record to a subset of its fields.
     * For example, the array schema
     * {"type": "array", "items": "long"}
     * an array containing the items 3 and 27 could be encoded as the long value 2 (encoded as hex 04) followed by long values 3 and 27 (encoded as hex 06 36) terminated by zero:
     * 04 06 36 00
     * The blocked representation permits one to read and write arrays larger than can be buffered in memory, since one can start writing items without knowing the full length of the array.
     */

    dynType_sequence_init(type, loc);
    int status = 0;
    dyn_type *itemType = dynType_sequence_itemType(type);
    size_t itemSize = (int64_t)dynType_size(itemType);
    uint32_t cap = 0;

    int64_t blockCount = 0;
    int64_t blockSize = 0;

    do {
        status = avrobin_read_long(stream, &blockCount);
        if (status != OK) {
            break;
        } else if (blockCount < 0) {
            blockSize = blockCount * -1; //found a block of blockSize bytes
            blockCount = blockSize / itemSize;
            int64_t rest = blockSize % itemSize;
            if (rest != 0) {
                LOG_ERROR("Found block size (%li) is not a multitude of the item size (%li)", blockSize, itemSize);
                status = ERROR;
                break;
            }
        }
        if (blockCount > 0) {
            LOG_DEBUG("Parsing block count of %li", blockCount);
            cap += blockCount;
            dynType_sequence_reserve(type, loc, cap);
            for (int64_t i = 0; i < blockCount; ++i) {
                void* itemLoc = NULL;
                status = dynType_sequence_increaseLengthAndReturnLastLoc(type, loc, &itemLoc);
                if (status != OK) {
                    break;
                }
                avrobinSerializer_parseAny(itemType, itemLoc, stream);
            }
            if (status != OK) {
                break;
            }
        }
    } while (blockCount != 0);

    if (status != OK) {
        dynType_free(type, loc);
    }
    return status;
}

static int avrobinSerializer_parseEnum(dyn_type *type, void *loc, FILE *stream) {
    int32_t index;
    if (avrobin_read_int(stream, &index) != OK) {
        return ERROR;
    }
    if (index < 0) {
        return ERROR;
    }

    struct meta_entry *entry = NULL;
    int32_t curr_index = 0;

    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        if (curr_index == index) {
            *(int32_t*)loc = atoi(entry->value);
            return OK;
        }
        curr_index++;
    }

    return ERROR;
}

static int avrobinSerializer_writeAny(dyn_type *type, void *loc, FILE *stream) {
    int status = OK;

    int descriptor = dynType_descriptorType(type);
    dyn_type *subType = NULL;

    bool *z;            //Z
    float *f;           //F
    double *d;          //D
    char *b;            //B
    int *n;             //N
    int16_t *s;         //S
    int32_t *i;         //I
    int64_t *l;         //J
    uint8_t   *ub;      //b
    uint16_t  *us;      //s
    uint32_t  *ui;      //i
    uint64_t  *ul;      //j

    switch (descriptor) {
        case 'Z' :
            z = loc;
            status = avrobin_write_boolean(stream,*z);
            break;
        case 'B' :
            b = loc;
            status = avrobin_write_int(stream,(int32_t)*b);
            break;
        case 'S' :
            s = loc;
            status = avrobin_write_int(stream,(int32_t)*s);
            break;
        case 'I' :
            i = loc;
            status = avrobin_write_int(stream,*i);
            break;
        case 'J' :
            l = loc;
            status = avrobin_write_long(stream,*l);
            break;
        case 'b' :
            ub = loc;
            status = avrobin_write_int(stream,(int32_t)*ub);
            break;
        case 's' :
            us = loc;
            status = avrobin_write_int(stream,(int32_t)*us);
            break;
        case 'i' :
            ui = loc;
            status = avrobin_write_int(stream,(int32_t)*ui);
            break;
        case 'j' :
            ul = loc;
            status = avrobin_write_long(stream,(int64_t)*ul);
            break;
        case 'N' :
            n = loc;
            status = avrobin_write_int(stream,(int32_t)*n);
            break;
        case 'F' :
            f = loc;
            status = avrobin_write_float(stream,*f);
            break;
        case 'D' :
            d = loc;
            status = avrobin_write_double(stream,*d);
            break;
        case 't' :
            status = avrobin_write_string(stream,*(const char**)loc);
            break;
        case '*' :
            status = dynType_typedPointer_getTypedType(type, &subType);
            if (status == OK) {
                status = avrobinSerializer_writeAny(subType, *(void**)loc, stream);
            }
            break;
        case '{' :
            status = avrobinSerializer_writeComplex(type, loc, stream);
            break;
        case '[' :
            status = avrobinSerializer_writeSequence(type, loc, stream);
            break;
        case 'E' :
            status = avrobinSerializer_writeEnum(type, loc, stream);
            break;
        case 'l':
            status = avrobinSerializer_writeAny(type->ref.ref, loc, stream);
            break;
        case 'P' :
            status = ERROR;
            LOG_WARNING("Untyped pointers are not supported for serialization.");
            break;
        default :
            status = ERROR;
            LOG_ERROR("Unsupported descriptor '%c'.", descriptor);
            break;
    }

    return status;
}

static int avrobinSerializer_writeComplex(dyn_type *type, void *loc, FILE *stream) {
    int status = OK;

    struct complex_type_entry *entry = NULL;
    struct complex_type_entries_head *entries = NULL;
    dyn_type *subType = NULL;
    void *subLoc = NULL;
    int index = -1;

    status = dynType_complex_entries(type, &entries);

    if (status == OK) {
        TAILQ_FOREACH(entry, entries, entries) {
            index = dynType_complex_indexForName(type, entry->name);

            if (index < 0) {
                status = ERROR;
                LOG_ERROR("Cannot find index for member '%s'.", entry->name);
            }

            if (status == OK) {
                status = dynType_complex_dynTypeAt(type, index, &subType);
            }

            if (status == OK) {
                status = dynType_complex_valLocAt(type, index, loc, &subLoc);
            }

            if (status == OK) {
                status = avrobinSerializer_writeAny(subType, subLoc, stream);
            }

            if (status != OK) {
                break;
            }
        }
    }

    return status;
}

static int avrobinSerializer_writeSequence(dyn_type *type, void *loc, FILE *stream) {
    uint32_t arrayLen = dynType_sequence_length(loc);

    dyn_type *itemType = dynType_sequence_itemType(type);
    void *itemLoc = NULL;

    if (avrobin_write_long(stream, arrayLen) != OK) {
        LOG_ERROR("Failed to write array block count.");
        return ERROR;
    }

    for (int i=0; i<arrayLen; i++) {
        if (dynType_sequence_locForIndex(type, loc, i, &itemLoc)) {
            return ERROR;
        }
        if (avrobinSerializer_writeAny(itemType, itemLoc, stream) != OK) {
            return ERROR;
        }
    }

    if (avrobin_write_long(stream, 0) != OK) {
        LOG_ERROR("Failed to write array block count.");
        return ERROR;
    }

    return OK;
}

static int avrobinSerializer_writeEnum(dyn_type *type, void *loc, FILE *stream) {
    char enum_value_str[16];
    if (sprintf(enum_value_str, "%d", *(int32_t*)loc) < 0) {
        return ERROR;
    }

    struct meta_entry *entry = NULL;
    int32_t index = 0;

    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        if (0 == strcmp(enum_value_str, entry->value)) {
            return avrobin_write_int(stream, index);
        }
        index++;
    }

    LOG_ERROR("Could not find Enum value %s in enum type.", enum_value_str);
    return ERROR;
}

static int avrobinSerializer_generateAny(dyn_type *type, json_t **output) {
    int status = OK;

    int descriptor = dynType_descriptorType(type);
    dyn_type *subType = NULL;
    json_t *schema = NULL;

    switch (descriptor) {
        case 'Z' :
            status = avrobin_schema_primitive("boolean", &schema);
            break;
        case 'B' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'S' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'I' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'J' :
            status = avrobin_schema_primitive("long", &schema);
            break;
        case 'b' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 's' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'i' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'j' :
            status = avrobin_schema_primitive("long", &schema);
            break;
        case 'N' :
            status = avrobin_schema_primitive("int", &schema);
            break;
        case 'F' :
            status = avrobin_schema_primitive("float", &schema);
            break;
        case 'D' :
            status = avrobin_schema_primitive("double", &schema);
            break;
        case 't' :
            status = avrobin_schema_primitive("string", &schema);
            break;
        case '*' :
            status = dynType_typedPointer_getTypedType(type, &subType);
            if (status == OK) {
                status = avrobinSerializer_generateAny(subType, &schema);
            }
            break;
        case '{' :
            status = avrobinSerializer_generateComplex(type, &schema);
            break;
        case '[' :
            status = avrobinSerializer_generateSequence(type, &schema);
            break;
        case 'E' :
            status = avrobinSerializer_generateEnum(type, &schema);
            break;
        case 'P' :
            status = ERROR;
            LOG_WARNING("Untyped pointers are not supported for serialization.");
            break;
        default :
            status = ERROR;
            LOG_ERROR("Unsupported descriptor '%c'.", descriptor);
            break;
    }

    if (status == OK && schema != NULL) {
        *output = schema;
    }

    return status;
}

static int avrobinSerializer_generateComplex(dyn_type *type, json_t **output) {
    int status = OK;

    struct complex_type_entry *entry = NULL;
    struct complex_type_entries_head *entries = NULL;
    dyn_type *subType = NULL;
    int index = -1;

    json_t *record_object = json_object();
    if (record_object == NULL) {
        return ERROR;
    }

    json_t *record_string = json_string_nocheck("record");
    if (record_string == NULL) {
        json_decref(record_object);
        return ERROR;
    }

    char *gen_str = NULL;
    if (generate_record_name(&gen_str) != OK) {
        json_decref(record_object);
        json_decref(record_string);
        return ERROR;
    }

    json_t *name_string = json_string(gen_str);
    free(gen_str);
    if (name_string == NULL) {
        json_decref(record_object);
        json_decref(record_string);
        return ERROR;
    }

    json_t *record_fields = json_array();
    if (record_fields == NULL) {
        json_decref(record_object);
        json_decref(record_string);
        json_decref(name_string);
        return ERROR;
    }

    json_t *field_object = NULL;
    json_t *field_name = NULL;
    json_t *field_schema = NULL;

    status = dynType_complex_entries(type, &entries);

    if (status == OK) {
        TAILQ_FOREACH(entry, entries, entries) {
            index = dynType_complex_indexForName(type, entry->name);

            if (index < 0) {
                status = ERROR;
                LOG_ERROR("Cannot find index for member '%s'.", entry->name);
            }

            if (status == OK) {
                status = dynType_complex_dynTypeAt(type, index, &subType);
            }

            if (status == OK) {
                field_object = json_object();
                if (field_object == NULL) {
                    status = ERROR;
                }
            }

            if (status == OK) {
                field_name = json_string(entry->name);
                if (field_name == NULL) {
                    status = ERROR;
                }
            }

            if (status == OK) {
                status = avrobinSerializer_generateAny(subType, &field_schema);
            }

            if (status == OK) {
                if (json_object_set_new_nocheck(field_object, "name", field_name) == 0) {
                    field_name = NULL;
                } else {
                    status = ERROR;
                }
            }

            if (status == OK) {
                if (json_object_set_new_nocheck(field_object, "type", field_schema) == 0) {
                    field_schema = NULL;
                } else {
                    status = ERROR;
                }
            }

            if (status == OK) {
                if (json_array_append_new(record_fields, field_object) == 0) {
                    field_object = NULL;
                } else {
                    status = ERROR;
                }
            }

            if (field_object != NULL) {
                json_decref(field_object);
                field_object = NULL;
            }

            if (field_name != NULL) {
                json_decref(field_name);
                field_name = NULL;
            }

            if (field_schema != NULL) {
                json_decref(field_schema);
                field_schema = NULL;
            }

            if (status != OK) {
                break;
            }
        }
    }

    if (status == OK) {
        if (json_object_set_new_nocheck(record_object, "type", record_string) == 0) {
            record_string = NULL;
        } else {
            status = ERROR;
        }
    }

    if (status == OK) {
        if (json_object_set_new_nocheck(record_object, "name", name_string) == 0) {
            name_string = NULL;
        } else {
            status = ERROR;
        }
    }

    if (status == OK) {
        if (json_object_set_new_nocheck(record_object, "fields", record_fields) == 0) {
            record_fields = NULL;
        } else {
            status = ERROR;
        }
    }

    if (record_string != NULL) {
        json_decref(record_string);
        record_string = NULL;
    }

    if (name_string != NULL) {
        json_decref(name_string);
        name_string = NULL;
    }

    if (record_fields != NULL) {
        json_decref(record_fields);
        record_fields = NULL;
    }

    if (status == OK) {
        *output = record_object;
    } else {
        json_decref(record_object);
        record_object = NULL;
    }

    return status;
}

static int avrobinSerializer_generateSequence(dyn_type *type, json_t **output) {
    dyn_type *itemType = dynType_sequence_itemType(type);

    json_t *array_object = json_object();
    if (array_object == NULL) {
        return ERROR;
    }

    json_t *array_string = json_string_nocheck("array");
    if (array_string == NULL) {
        json_decref(array_object);
        return ERROR;
    }

    json_t *item_schema = NULL;
    if (avrobinSerializer_generateAny(itemType, &item_schema) != OK) {
        json_decref(array_object);
        json_decref(array_string);
        return ERROR;
    }

    if (json_object_set_new_nocheck(array_object, "type", array_string) == 0) {
        array_string = NULL;
    } else {
        json_decref(array_object);
        json_decref(array_string);
        json_decref(item_schema);
        return ERROR;
    }

    if (json_object_set_new_nocheck(array_object, "items", item_schema) == 0) {
        item_schema = NULL;
    } else {
        json_decref(array_object);
        json_decref(item_schema);
        return ERROR;
    }

    *output = array_object;

    return OK;
}

static int avrobinSerializer_generateEnum(dyn_type *type, json_t **output) {
    json_t *enum_object = json_object();
    if (enum_object == NULL) {
        return ERROR;
    }

    json_t *enum_string = json_string_nocheck("enum");
    if (enum_string == NULL) {
        json_decref(enum_object);
        return ERROR;
    }

    char *gen_str = NULL;
    if (generate_record_name(&gen_str) != OK) {
        json_decref(enum_object);
        json_decref(enum_string);
        return ERROR;
    }

    json_t *name_string = json_string(gen_str);
    free(gen_str);
    if (name_string == NULL) {
        json_decref(enum_object);
        json_decref(enum_string);
        return ERROR;
    }

    json_t *symbols_array = json_array();
    if (symbols_array == NULL) {
        json_decref(enum_object);
        json_decref(enum_string);
        json_decref(name_string);
        return ERROR;
    }

    struct meta_entry *entry = NULL;
    json_t *symbol_string = NULL;

    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        symbol_string = json_string(entry->name);
        if (symbol_string == NULL) {
            json_decref(enum_object);
            json_decref(enum_string);
            json_decref(name_string);
            json_decref(symbols_array);
            return ERROR;
        }
        if (json_array_append_new(symbols_array, symbol_string) != 0) {
            json_decref(enum_object);
            json_decref(enum_string);
            json_decref(name_string);
            json_decref(symbol_string);
            json_decref(symbols_array);
            return ERROR;
        }
    }

    if (json_object_set_new_nocheck(enum_object, "type", enum_string) != 0) {
        json_decref(enum_object);
        json_decref(enum_string);
        json_decref(name_string);
        json_decref(symbols_array);
        return ERROR;
    }

    if  (json_object_set_new_nocheck(enum_object, "name", name_string) != 0) {
        json_decref(enum_object);
        json_decref(name_string);
        json_decref(symbols_array);
        return ERROR;
    }

    if (json_object_set_new_nocheck(enum_object, "symbols", symbols_array) != 0) {
        json_decref(enum_object);
        json_decref(symbols_array);
        return ERROR;
    }

    *output = enum_object;

    return OK;
}

static int generate_sync(uint8_t **result) {
    *result = (uint8_t*)malloc(sizeof(uint8_t) * 16);
    if (*result == NULL) {
        return ERROR;
    }
    for (int i=0; i<16; i++) {
        (*result)[i] = (uint8_t)(((double)rand() / (double)RAND_MAX) * 255);
    }
    return OK;
}

static int generate_record_name(char **result) {
    char num_str[16];
    if (sprintf(num_str, "R%u", record_name_counter) < 0) {
        return ERROR;
    }
    size_t len = strlen(num_str);
    *result = (char*)malloc(sizeof(char) * (len + 1));
    if (*result == NULL) {
        return ERROR;
    }
    for (int i=0; i<len; i++) {
        (*result)[i] = num_str[i];
    }
    (*result)[len] = '\0';
    record_name_counter++;
    return OK;
}

static int avrobin_read_boolean(FILE *stream,bool *val) {
    int c = fgetc(stream);
    if (c == EOF) {
        LOG_ERROR("Unexpected end of file.");
        return ERROR;
    }
    if (c!=0 && c!=1) {
        LOG_ERROR("Unexpected value for boolean.");
        return ERROR;
    }
    if (c == 0) {
        *val = false;
    } else {
        *val = true;
    }
    return OK;
}

static int avrobin_read_int(FILE *stream,int32_t *val) {
    int64_t lval;
    int status = avrobin_read_long(stream,&lval);
    //TODO Do range check.
    *val = (int32_t)lval;
    return status;
}

static int avrobin_read_long(FILE *stream,int64_t *val) {
    uint64_t uval = 0;
    uint8_t b;
    int c;
    int offset = 0;
    do {
        if (offset == MAX_VARINT_BUF_SIZE) {
            LOG_ERROR("Varint too long.");
            return ERROR;
        }
        c = fgetc(stream);
        if (c == EOF) {
            LOG_ERROR("Unexpected end of file.");
            return ERROR;
        }
        b = (uint8_t)c;
        uval |= (uint64_t) (b & 0x7F) << (7 * offset);
        ++offset;
    }
    while (b & 0x80);
    *val = ((uval >> 1) ^ -(uval & 1));
    return OK;
}

static int avrobin_read_float(FILE *stream,float *val) {
    uint8_t b[4];
    int c;
    for (int i=0;i<4;i++) {
        c = fgetc(stream);
        if (c == EOF) {
            LOG_ERROR("Unexpected end of file.");
            return ERROR;
        }
        b[i] = (uint8_t)c;
    }
    union {
        float f;
        uint32_t i;
    } v;
    v.i =
    ((uint32_t)b[0] << 0) |
    ((uint32_t)b[1] << 8) |
    ((uint32_t)b[2] << 16) |
    ((uint32_t)b[3] << 24);
    *val = v.f;
    return OK;
}

static int avrobin_read_double(FILE *stream,double *val) {
    uint8_t b[8];
    int c;
    for (int i=0;i<8;i++) {
        c = fgetc(stream);
        if (c == EOF) {
            LOG_ERROR("Unexpected end of file.");
            return ERROR;
        }
        b[i] = (uint8_t)c;
    }
    union {
        double d;
        uint64_t i;
    } v;
    v.i =
    ((uint64_t)b[0] << 0) |
    ((uint64_t)b[1] << 8) |
    ((uint64_t)b[2] << 16) |
    ((uint64_t)b[3] << 24) |
    ((uint64_t)b[4] << 32) |
    ((uint64_t)b[5] << 40) |
    ((uint64_t)b[6] << 48) |
    ((uint64_t)b[7] << 56);
    *val = v.d;
    return OK;
}

static int avrobin_read_string(FILE *stream,char **val) {
    int64_t len;
    if (avrobin_read_long(stream,&len) != OK) {
        LOG_ERROR("Failed to read string length.");
        return ERROR;
    }
    if (len < 0) {
        LOG_ERROR("Negative string length.");
        return ERROR;
    }
    *val = (char*)malloc(sizeof(char) * (len+1));
    if (*val == NULL) {
        LOG_ERROR("Failed to allocate memory for avro string.");
        return ERROR;
    }
    int c;
    for (int64_t i=0;i<len;i++) {
        c = fgetc(stream);
        if (c == EOF) {
            LOG_ERROR("Unexpected end of file.");
            free(*val);
            return ERROR;
        }
        (*val)[i] = (char)c;
    }
    (*val)[len] = '\0';
    return OK;
}

static int avrobin_write_boolean(FILE *stream,bool val) {
    int r;
    if (val) {
        r = fputc(1, stream);
    } else {
        r = fputc(0, stream);
    }
    if (r == EOF) {
        LOG_ERROR("Write error.");
        return ERROR;
    }
    return OK;
}

static int avrobin_write_int(FILE *stream,int32_t val) {
    int64_t lval = val;
    return avrobin_write_long(stream,lval);
}

static int avrobin_write_long(FILE *stream,int64_t val) {
    uint64_t uval = (val << 1) ^ (val >> 63);
    uint8_t b[MAX_VARINT_BUF_SIZE];
    int bytes_written = 0;
    while (uval & ~0x7F) {
        b[bytes_written++] = (uint8_t)((uval & 0x7F) | 0x80);
        uval >>= 7;
    }
    b[bytes_written++] = (uint8_t)uval;
    for (int i=0;i<bytes_written;i++) {
        if (fputc(b[i],stream) == EOF) {
            LOG_ERROR("Write error.");
            return ERROR;
        }
    }
    return OK;
}

static int avrobin_write_float(FILE *stream,float val) {
    uint8_t b[4];
    union {
        float f;
        uint32_t i;
    } v;
    v.f = val;
    b[0] = (uint8_t)((v.i & 0x000000FF) >> 0);
    b[1] = (uint8_t)((v.i & 0x0000FF00) >> 8);
    b[2] = (uint8_t)((v.i & 0x00FF0000) >> 16);
    b[3] = (uint8_t)((v.i & 0xFF000000) >> 24);
    for (int i=0;i<4;i++) {
        if (fputc(b[i],stream) == EOF) {
            LOG_ERROR("Write error.");
            return ERROR;
        }
    }
    return OK;
}

static int avrobin_write_double(FILE *stream,double val) {
    uint8_t b[8];
    union {
        double d;
        uint64_t i;
    } v;
    v.d = val;
    b[0] = (uint8_t)((v.i & 0x00000000000000FF) >> 0);
    b[1] = (uint8_t)((v.i & 0x000000000000FF00) >> 8);
    b[2] = (uint8_t)((v.i & 0x0000000000FF0000) >> 16);
    b[3] = (uint8_t)((v.i & 0x00000000FF000000) >> 24);
    b[4] = (uint8_t)((v.i & 0x000000FF00000000) >> 32);
    b[5] = (uint8_t)((v.i & 0x0000FF0000000000) >> 40);
    b[6] = (uint8_t)((v.i & 0x00FF000000000000) >> 48);
    b[7] = (uint8_t)((v.i & 0xFF00000000000000) >> 56);
    for (int i=0;i<8;i++) {
        if (fputc(b[i],stream) == EOF) {
            LOG_ERROR("Write error.");
            return ERROR;
        }
    }
    return OK;
}

static int avrobin_write_string(FILE *stream,const char *val) {
    assert(val != NULL);
    size_t len = strlen(val);
    if (avrobin_write_long(stream, (int64_t)len) != OK) {
        LOG_ERROR("Failed to write string length.");
        return ERROR;
    }
    for (size_t i=0;i<len;i++) {
        if (fputc((unsigned char)(val[i]),stream) == EOF) {
            LOG_ERROR("Write error.");
            return ERROR;
        }
    }
    return OK;
}

static int avrobin_schema_primitive(const char *tname, json_t **output) {
    json_t *jo = json_object();
    if (jo == NULL) {
        return ERROR;
    }
    json_t *js = json_string(tname);
    if (js == NULL) {
        json_decref(jo);
        return ERROR;
    }
    if (json_object_set_new_nocheck(jo, "type", js) != 0) {
        json_decref(jo);
        json_decref(js);
        return ERROR;
    }
    *output = jo;
    return OK;
}
