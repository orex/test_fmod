/* Author: Kirill Okhotnikov <kirill.okhotnikov@gmail.com>
 * 
 * Background:
 *   Like the previous algorithm written by Sun Microsystems, the code also
 *   splits double value to integers with exponential (ix, iy) 
 *   and mantissa part (hx, hy). The variables names are kept common with previous code 
 *   in order to simplify understanding. The part of checking special cases, numbers NaN, INF etc
 *   is copied from Ulrich Drepper <drepper@gmail.com> previous code.
 *   The rest of the code uses different algorithms for fmod calculation.
 *   An input x, y in the algorithm is represented (mathematically) like h(x|y) * 2^i(x|y).
 *   There is an unambiguous number representation. For example h * 2^i = (2 * h) * 2^(i-1).
 *   The previous algorithm removes such unambiguous, "aligning" bits in hx, and hy. But for this code
 *   such unambiguity is essential.
 * 
 * Mathematics:
 *   The algorithm uses the fact that
 *   r = a % b = (a % (N * b)) % b, where N is positive integer number. a and b - positive.
 *   Let's adopt the formula for representation above.
 *   a = hx * 2^ix, b = hy * 2^iy, N = 2^k
 *   r(k) = a % b = (hx * 2^ix) % (2 ^k * hy * 2^iy) = 2^(iy + k) * (hx * 2^(ix - iy - k) % hy)
 *   r(k) = hr * 2^ir = (hx % hy) * 2^(iy + k) = (2^p * (hx % hy) * 2^(iy + k - p))
 *      hr = 2^p * (hx % hy)
 *      ir = iy + k - p  
 *    
 *     
 * Algorithm description:
 *   The examples below use byte (not uint64_t) for simplicity.
 *   1) Shift hy maximum to right without losing bits and increase iy value
 *      hy = 0b00101100 iy = 20 after shift hy = 0b00001011 iy = 22.
 *   2) hx = hx % hy.
 *   3) Move hx maximum to left. Note that after (hx = hx% hy) CLZ in hx is not lower than CLZ in hy.
 *      hx=0b00001001 ix = 100, hx=0b10010000, ix = 100-4 = 96.
 *   4) Repeat (2) until ix == iy. 
 * 
 * Complexity analysis:
 *   Converting x, y to (hx, ix), (hy, iy): CTZ/shift/AND/OR/if.
 *   Loop count: (ix - iy) / (64 - "length of hy"). max("length of hy") = 53, max(ix - iy) = 2048
 *                Maximum operation is 186. For rare "unrealistic" cases. 
 *                Previous algorithm had ix - iy complexity. 11 times more iterations for worst case. 
 *   
 *   Converting hx, ix back to double: CLZ/shift/AND/OR/if
 *   Previous algorithm uses loops for CLZ when converting from/to double.
 * 
 * Special cases:
 *   Supposing that case where |y| > 1e-292 and |x/y|<2000 is very common special processing is implemented.
 *   No hy alignment, no loop: result = (hx * 2^(ix - iy)) % hy.
 *   When x and y are both subnormal (rare case but...) the result = hx % hy. Simplified conversion back to double.
 * 
 * Performance: 
 *   Various tests on AArch64 and x86_64 shows that the current algorithm is faster than the previous one by factor 1.5-10 depend
 *   on input.
 * 
 */

/*
 * __ieee754_fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: divide and shift
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

#define __glibc_unlikely(cond)	__builtin_expect ((cond), 0)
#define __glibc_likely(cond)	__builtin_expect ((cond), 1)

static const double one = 1.0;

#define SIGN_POS_MASK UINT64_C(0x8000000000000000)
#define SIGN_NEG_MASK UINT64_C(0x7fffffffffffffff)
#define DOUBLE_INF_NAN UINT64_C(0x7ff0000000000000)
#define LAST_EXP_BIT UINT64_C(0x0010000000000000)
#define SET_NORMAL_VALUE(a) a = UINT64_C(0x0010000000000000)|(UINT64_C(0x000fffffffffffff)&(a))
#define CLZ(a) __builtin_clzl(a)
#define CTZ(a) __builtin_ctzl(a)

/* Convert back to double */
inline
double make_double(ieee_double_shape_type sx, uint64_t num, int32_t ep) {
    int32_t lz;
    lz = CLZ(num) - 11;
    num <<= lz;
    ep -= lz;

  if (__glibc_likely(ep >= 0))
    {
        num = ((num - LAST_EXP_BIT) | ((uint64_t) (ep + 1) << 52));
    }
  else
    {
        num >>= -ep;
    }
    sx.word += num;

    sx.value *= one;  /* Optimized out? May be math_check_force_underflow? */
    return sx.value;
}

double
my_fmod (double x, double y)
{
	int32_t n, ix, iy, lzhy, tzhy, hyltzeroes;
	uint64_t hx, hy, d;
    ieee_double_shape_type sx;

  EXTRACT_WORDS64 (hx, x);
  EXTRACT_WORDS64 (hy, y);
  sx.word = hx & SIGN_POS_MASK;    /* sign of x */
  hx ^= sx.word;            /* |x| */
  hy &= SIGN_NEG_MASK;    /* |y| */

    /* purge off exception values */
  if (__glibc_unlikely (hy == 0
			    || hx >= DOUBLE_INF_NAN
                        || hy > DOUBLE_INF_NAN))
	  /* y=0,or x not finite or y is NaN */
    return (x * y) / (x * y);
  if (__glibc_likely(hx <= hy))
    {
      if (hx < hy) return x;    /* |x|<|y| return x */
      return sx.value;    /* |x|=|y| return x*0*/
	}
	
    ix = hx >> 52;
    iy = hy >> 52;

    /* Most common case where |y| > 1e-292 and |x/y|<2000 */
  if (__glibc_likely(ix >= 53 && ix - iy <= 11))
    {
        SET_NORMAL_VALUE(hx);
        SET_NORMAL_VALUE(hy);
        d = (ix == iy) ? (hx - hy) : (hx << (ix - iy)) % hy;
      if (d == 0)
            return sx.value;
      return make_double (sx, d, iy - 1); /* iy - 1 because of "zero power" for number with power 1 */
    }

    /* Both subnormal special case. */ 
  if (__glibc_unlikely (ix == 0 && iy == 0))
    {
        uint64_t d = hx % hy;
        sx.word += d;
        sx.value *= one;  /* Optimized out? need for math signals */
        return sx.value;
    }

    /* Assume that hx is not subnormal by conditions above. */
    SET_NORMAL_VALUE(hx);
    ix--;
  if (__builtin_expect (iy > 0, 1))
    {
      SET_NORMAL_VALUE(hy);
      lzhy = 11;
      iy--;
    }
  else
    {
      lzhy = CLZ(hy);
    }

    /* Assume hy != 0 */
    tzhy = CTZ(hy);
    hyltzeroes = lzhy + tzhy;
    n = ix - iy;

    /* Shift hy right until the end or n = 0 */
  if (n > tzhy)
    {
      hy >>= tzhy;
      n -= tzhy;
      iy += tzhy;
    }
  else
    {
      hy >>= n;
      iy += n;
      n = 0;
   }

    /* Shift hx left until the end or n = 0 */
  if (n > 11)
    {
        hx <<= 11;
        n -= 11;
    }
  else
    {
        hx <<= n;
        n = 0;
    }
    hx %= hy;
  if (hx == 0)
      return sx.value;

  if (n == 0)
    return make_double (sx, hx, iy);

    /* hx in next code can become 0, because hx < hy, hy % 2 == 1 hx * 2^i % hy != 0 */

  while (n > hyltzeroes)
    {
      n -= hyltzeroes;
       hx <<= hyltzeroes;
       hx %= hy;
    }

    hx <<= n;
    hx %= hy;

  return make_double (sx, hx, iy);
}
