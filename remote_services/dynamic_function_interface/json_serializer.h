/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __JSON_SERIALIZER_H_
#define __JSON_SERIALIZER_H_

#include "dyn_type.h"

int json_deserialize(dyn_type *type, const char *input, void **result);
int json_serialize(dyn_type *type, void *input, char **output, size_t *size);

#endif
