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

#ifndef _DYN_TYPE_H_
#define _DYN_TYPE_H_

#include <stdio.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>

#include "celix_dfi_export.h"
#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * dyn type (dynamic type) represent a structure in memory. It can be used to calculate the needed memory size, the size
 * and offset of struct members and can be used to dynamically alloc, initialize, dealloc, read & write structure
 * in memory. In other words it can be used for reflection.
 *
 * Dyn types can be created by parsing a dyn type descriptor string.
 * Dyn type descriptor strings are very compact description of a C structure.
 *
 * Descriptor strings grammar:
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
 * j uint64_t
 * s uint16_t
 * P untyped pointer (void *)
 * t char* string
 * N native int
 * E enum (int) + meta infos #EnumName=#EnumValue (e.g. #e1=v1;#e2=v2;E)
 *
 * < Array //TODO
 * <(size)[Type] //TODO 
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
 * #OMG style sequence. Struct with uint32_t cap, uint32_t len and a <itemtype> buf[] fields.
 * SequenceType
 * [(Type)
 *
 * TypedPointer
 * *(Type)
 *
 * MetaInfo
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
    dyn_type* type;
    TAILQ_ENTRY(type_entry) entries;
};

TAILQ_HEAD(complex_type_entries_head, complex_type_entry);
struct complex_type_entry {
    dyn_type* type;
    char* name;
    TAILQ_ENTRY(complex_type_entry) entries;
};

TAILQ_HEAD(meta_properties_head, meta_entry);
struct meta_entry {
    char* name;
    char* value;
    TAILQ_ENTRY(meta_entry) entries;
};

/**
 * Parses a descriptor stream and creates a dyn_type (dynamic type).
 * If successful the type output argument points to the newly created dyn type.
 * The caller is the owner of the dyn type and use dynType_destroy to deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param descriptorStream  Stream to the descriptor.
 * @param name              name for the dyn_type. This can be used in references to dyn types.
 * @param refTypes          A list of referable dyn types.
 * @param type              Output argument for the parsed dyn type.
 * @return                  0 if successful.
 */
CELIX_DFI_EXPORT int dynType_parse(FILE* descriptorStream, const char* name, const struct types_head* refTypes, dyn_type** type);

/**
 * @brief Parses a descriptor stream and creates a dyn_type (dynamic type) of a given name.
 * Similar to dynType_parse except that the ownership of the given name is taken by the dyn type.
 */
CELIX_DFI_EXPORT int dynType_parseOfName(FILE* descriptorStream, char* name, const struct types_head* refTypes, dyn_type** type);

/**
 * Parses a descriptor string and creates a dyn_type (dynamic type).
 * If successful the type output argument points to the newly created dyn type.
 * The caller is the owner of the dyn type and use dynType_destroy to deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param descriptor        The descriptor.
 * @param name              name for the dyn_type. This can be used in references to dyn types.
 * @param refTypes          A list if referable dyn types.
 * @param type              Output argument for the parsed dyn type.
 * @return                  0 if successful.
 */
CELIX_DFI_EXPORT int dynType_parseWithStr(const char* descriptor, const char* name, const struct types_head* refTypes, dyn_type** type);

/**
 * Destroy a dyn type and de-allocates the memory.
 * @param type      The dyn type to destroy.
 */
CELIX_DFI_EXPORT void dynType_destroy(dyn_type* type);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(dyn_type, dynType_destroy);

/**
 * Allocates memory for a type instance described by a dyn type. The memory will be set to zero (calloc).
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param type      The dyn type for which structure to allocate.
 * @param instance  The output argument for the allocated memory.
 * @return          0 on success.
 */
CELIX_DFI_EXPORT int dynType_alloc(const dyn_type* type, void** instance);

/**
 * free the memory for a type instance described by a dyn type.
 * This is a deep free.
 *
 * @param type        The dyn type for which structure to allocate.
 * @param instance    The memory location of the type instance.
 */
CELIX_DFI_EXPORT void dynType_free(const dyn_type* type, void* instance);

/**
 * Prints the dyn type information to the provided output stream.
 * @param type      The dyn type to print.
 * @param stream    The output stream (e.g. stdout).
 */
CELIX_DFI_EXPORT void dynType_print(const dyn_type* type, FILE* stream);

/**
 * Return the size of the structure described by the dyn type.
 * This does _not_ includes sizes of data which is only pointed to (i.e. content of sequences, strings, etc).
 *
 * @param type  The dyn type.
 * @return      The size of the type instance described by dyn type.
 */
CELIX_DFI_EXPORT size_t dynType_size(const dyn_type* type);

/**
 * The type of the dyn type
 * E.g. DYN_TYPE_SIMPLE, DYN_TYPE_COMPLEX, etc
 * @param type  The dyn type
 * @return      The type of the dyn type.
 */
CELIX_DFI_EXPORT int dynType_type(const dyn_type* type);

/**
 * Returns the real (non-reference) type of the dyn type.
 */
CELIX_DFI_EXPORT const dyn_type* dynType_realType(const dyn_type* type);

/**
 * Returns the char identifier of the dyn type type.
 * E.g. 'D' for a double, or '{' for a complex type.
 * @param type  The dyn type
 * @return      The descriptor of the dyn type.
 */
CELIX_DFI_EXPORT char dynType_descriptorType(const dyn_type* type);

/**
 * Get the dyn type meta information for the provided name.
 * @param type  The dyn type.
 * @param name  The name of the requested meta information.
 * @return      The meta information or NULL if the meta information is not present.
 */
CELIX_DFI_EXPORT const char* dynType_getMetaInfo(const dyn_type* type, const char* name);

/**
 * Returns the list of meta entries.
 * Note that dyn type is owner of the entries. Traversing the entries is not thread safe.
 * @param type      The dyn type.
 * @note It always returns valid list.
 */
CELIX_DFI_EXPORT const struct meta_properties_head* dynType_metaEntries(const dyn_type* type);

/**
 * Returns the name of the dyn type. Name can be NULL.
 */
CELIX_DFI_EXPORT const char* dynType_getName(const dyn_type* type);


/**
 * Returns the field index for a complex type.
 *
 * e.g. for a struct { int a; int b; }, a will have an index of 0 and b an index of 1.
 * @param type  The dyn type. Must be a complex type.
 * @param name  The field name.
 * @return      The field index or -1 if no field with the name was found.
 */
CELIX_DFI_EXPORT int dynType_complex_indexForName(const dyn_type* type, const char* name);

/**
 * Returns the field dyn type for a given complex type and a field index.
 *
 * @param type      The dyn type. Must be a complex type.
 * @param index     The field index for which field the dyn type should be returned. The field index must be correct.
 */
CELIX_DFI_EXPORT const dyn_type* dynType_complex_dynTypeAt(const dyn_type* type, int index);

CELIX_DFI_EXPORT int dynType_complex_setValueAt(const dyn_type* type, int index, void* inst, const void* in);
CELIX_DFI_EXPORT void* dynType_complex_valLocAt(const dyn_type* type, int index, void* inst);
CELIX_DFI_EXPORT const struct complex_type_entries_head* dynType_complex_entries(const dyn_type* type);
CELIX_DFI_EXPORT size_t dynType_complex_nrOfEntries(const dyn_type* type);

//sequence

/**
 * Initialize a sequence struct with a cap & len of 0 and the buf to NULL.
 */
CELIX_DFI_EXPORT void dynType_sequence_init(const dyn_type* type, void* inst);

/**
 * Allocates memory for a sequence with capacity cap.
 * Will not free if the existing buf.
 * Sets len to 0
 *
 * In case of an error, an error message is added to celix_err.
 *
 */
CELIX_DFI_EXPORT int dynType_sequence_alloc(const dyn_type* type, void* inst, uint32_t cap);

/**
 * Reserve a sequence capacity of cap
 * Using realloc of the requested capicity is not enough.
 * Keeps the len value.
 * Note will not decrease the allocated memory
 *
 * In case of an error, an error message is added to celix_err.
 *
 */
CELIX_DFI_EXPORT int dynType_sequence_reserve(const dyn_type* type, void* inst, uint32_t cap);

/**
 * @brief Gets the value location for a specific sequence index from a sequence instance.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dyn type. Must be a sequence type.
 * @param[in] seqLoc The sequence instance.
 * @param[in] index The index of the value to get.
 * @param[out] valLoc The value location as output.
 * @return 0 if successful.
 * @retval 1 if the index is out of bounds.
 */
CELIX_DFI_EXPORT int dynType_sequence_locForIndex(const dyn_type* type, const void* seqLoc, uint32_t index, void** valLoc);

/**
 * @brief Increase the length of the sequence by one and return the value location for the last element.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dyn type. Must be a sequence type.
 * @param[in] seqLoc The sequence instance.
 * @param[out] valLoc The last value location as output.
 * @return 0 if successful.
 * @retval 1 if the sequence length is already at the max.
 */
CELIX_DFI_EXPORT int dynType_sequence_increaseLengthAndReturnLastLoc(const dyn_type* type, void* seqLoc, void** valLoc);

/**
 * @brief Get the item type of a sequence.
 * @param[in] type The dyn type. Must be a sequence type.
 * @return The item type of the sequence.
 */
CELIX_DFI_EXPORT const dyn_type* dynType_sequence_itemType(const dyn_type* type);

/**
 * @brief Get the length of a sequence.
 * @param[in] seqLoc The sequence instance.
 * @return The length of the sequence.
 */
CELIX_DFI_EXPORT uint32_t dynType_sequence_length(const void* seqLoc);

/**
 * @brief Gets the typedType of a typedPointer type.
 */
CELIX_DFI_EXPORT const dyn_type* dynType_typedPointer_getTypedType(const dyn_type* type);

/**
 * @brief Allocates and initializes a string type.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] type The dyn type. Must be a string type.
 * @param[out] textLoc The string instance.
 * @param[in] value The value to initialize the string.
 * @return 0 if successful.
 * @retval 1 if Cannot allocate memory.
 */
CELIX_DFI_EXPORT int dynType_text_allocAndInit(const dyn_type* type, void* textLoc, const char* value);

/**
 * @brief Sets the value of a simple type.
 * @param[in] type The dyn type. Must be a simple type.
 * @param[out] inst The instance of the simple type.
 * @param[in] in   The value to set.
 */
CELIX_DFI_EXPORT void dynType_simple_setValue(const dyn_type* type, void* inst, const void* in);

#ifdef __cplusplus
}
#endif

#endif //_DYN_TYPE_H_
