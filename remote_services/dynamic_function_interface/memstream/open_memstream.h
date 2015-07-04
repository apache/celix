/*
 * Licensed under Apache License v2. See LICENSE for more information.
 *
 */
#ifndef OPEN_MEMSTREAM_H_
#define OPEN_MEMSTREAM_H_

#ifdef __cplusplus
extern "C"
{
#endif

FILE *open_memstream(char **cp, size_t *lenp);

#ifdef __cplusplus
}
#endif

#endif // #ifndef FMEMOPEN_H_
