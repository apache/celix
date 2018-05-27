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
