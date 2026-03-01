#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <sys/types.h>

typedef int32_t i32;  // 4 B [-2^31, 2^31 - 1]
typedef int64_t i64;  // 8 B [-2^63, 2^63 - 1]
typedef uint32_t u32; // 4 B [0, 2^32 - 1]
typedef uint64_t u64; // 8 B [0, 2^64 - 1]
typedef float f32;  // 4 B [+- 2^-126, +-(2-2^-23) * 2^127]   - 7 dig precision
typedef double f64; // 8 B [+- 2^-1022, +-(2-2^-52) * 2^1023] - 15 dig precision

typedef i32 b32;

#define ERR -1
#define OK !(ERR)

#define READ 0
#define WRITE 1

#define FOUND 1

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif
