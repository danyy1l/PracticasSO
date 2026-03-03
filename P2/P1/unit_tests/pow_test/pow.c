#include "pow.h"

#define PRIME POW_LIMIT
#define BIG_X 435679812
#define BIG_Y 100001819

u64 pow_hash(u64 x) {
  u64 result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}
