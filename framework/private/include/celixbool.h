#ifndef CELIXBOOL_H_
#define CELIXBOOL_H_


#if defined(__STDC__)
# define C89
# if defined(__STDC_VERSION__)
#  define C90
#  if (__STDC_VERSION__ >= 199409L)
#   define C94
#  endif
#  if (__STDC_VERSION__ >= 199901L)
#   define C99
#  endif
# endif
#endif

#ifndef C99

typedef int _Bool;

#define bool _Bool
#define false 0 
#define true 1


#else

#include <stdbool.h>

#endif

#endif /* CELIXBOOL_H_ */
