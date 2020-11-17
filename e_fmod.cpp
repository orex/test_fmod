/* Rewritten for 64-bit machines by Ulrich Drepper <drepper@gmail.com>.  */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * my_fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

#include <math.h>
#include <stdint.h>

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

static const double one = 1.0;

#define SIGN_POS_MASK UINT64_C(0x8000000000000000)
#define SIGN_NEG_MASK UINT64_C(0x7fffffffffffffff)
#define DOUBLE_INF_NAN UINT64_C(0x7ff0000000000000)
#define LAST_EXP_BIT UINT64_C(0x0010000000000000)
#define SET_NORMAL_VALUE(a) a = UINT64_C(0x0010000000000000)|(UINT64_C(0x000fffffffffffff)&(a))
#define CLZ(a) __builtin_clzl(a)
#define CTZ(a) __builtin_ctzl(a)

inline
double make_double(ieee_double_shape_type sx, uint64_t num, int32_t ep) {
    int32_t lz;
    lz = CLZ(num) - 11;
    num <<= lz;
    ep -= lz;

    if (__builtin_expect(ep >= 0, 1)) {
        num = ((num - LAST_EXP_BIT) | ((uint64_t) (ep + 1) << 52));
    } else {
        num >>= -ep;
    }
    sx.word += num;

    sx.value *= one;  // Optimized out? May be math_check_force_underflow?
    return sx.value;
}


double
my_fmod (double x, double y)
{
	int32_t n, ix, iy, lzhy, tzhy, hyltzeroes;
	uint64_t hx, hy, d;
    ieee_double_shape_type sx;

	EXTRACT_WORDS64(hx, x);
	EXTRACT_WORDS64(hy, y);
	sx.word = hx & SIGN_POS_MASK;	/* sign of x */
	hx ^= sx.word;			/* |x| */
	hy &= SIGN_NEG_MASK;	/* |y| */

    /* purge off exception values */
	if(__builtin_expect(hy==0
			    || hx >= DOUBLE_INF_NAN
			    || hy > DOUBLE_INF_NAN, 0))
	  /* y=0,or x not finite or y is NaN */
	    return (x*y)/(x*y);
	if(__builtin_expect(hx<=hy, 0)) {
	    if(hx<hy) return x;	/* |x|<|y| return x */
	    return sx.value;	/* |x|=|y| return x*0*/
	}
    ix = hx >> 52;
    iy = hy >> 52;

    if( __builtin_expect(ix >= 53 && ix - iy <= 11, 1) ) {
        SET_NORMAL_VALUE(hx);
        SET_NORMAL_VALUE(hy);
        d = (ix == iy) ? (hx - hy) : (hx << (ix - iy)) % hy;
        if( d == 0 )
            return sx.value;
        return make_double(sx, d, iy - 1); // iy - 1 because of "zero power" for number with power 1
    }

    if( __builtin_expect(ix == 0 && iy == 0, 0) ) {
        uint64_t d = hx % hy;
        sx.word += d;
        sx.value *= one;  // Optimized out? need for math signals
        return sx.value;
    }

    // Assume that hx is not subnormal by conditions above.
    SET_NORMAL_VALUE(hx);
    ix--;
    if( __builtin_expect(iy > 0, 1)  ) {
      SET_NORMAL_VALUE(hy);
      lzhy = 11;
      iy--;
    } else {
      lzhy = CLZ(hy);
    }

    //Assume hy != 0
    tzhy = CTZ(hy);
    hyltzeroes = lzhy + tzhy;
    n = ix - iy;

    // Shift hy right until the end or n = 0
    if( n > tzhy ) {
      hy >>= tzhy;
      n -= tzhy;
      iy += tzhy;
    } else {
      hy >>= n;
      iy += n;
      n = 0;
   }

    //Shift hx left until the end or n = 0
    if( n > 11 ) {
        hx <<= 11;
        n -= 11;
    } else {
        hx <<= n;
        n = 0;
    }
    hx %= hy;
    if( hx == 0 )
      return sx.value;

    while( n != 0 ) {
      if( n > hyltzeroes ) {
          n -=  hyltzeroes;
          hx <<= hyltzeroes;
      } else {
          hx <<= n;
          n = 0;
      }
      hx %= hy;
      if( hx == 0 )
        return sx.value;
    }

    return make_double(sx, hx, iy);
}
