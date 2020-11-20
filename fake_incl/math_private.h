#include <stdint.h>
#include <math.h>

typedef union
{
    double value;
    struct
    {
        uint32_t msw;
        uint32_t lsw;
    } parts;
    uint64_t word;
} ieee_double_shape_type;

/* Get all in one, efficient on 64-bit machines.  */
#ifndef EXTRACT_WORDS64
# define EXTRACT_WORDS64(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.word;						\
} while (0)
#endif

/* Set a double from two 32 bit ints.  */
#ifndef INSERT_WORDS
# define INSERT_WORDS(d,ix0,ix1)				\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)
#endif

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

#define __glibc_unlikely(cond)	__builtin_expect ((cond), 0)
#define __glibc_likely(cond)	__builtin_expect ((cond), 1)

/* Dirty hack */
#define __ieee754_fmod my_fmod