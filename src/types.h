#ifndef TIPPSE_TYPES_H
#define TIPPSE_TYPES_H

#include <stdlib.h>
#include <stdint.h>

// This one limits the maximum file offset
typedef uint64_t file_offset_t;

// And this one limits the maximum cursor position etc. (signed type needed)
typedef int64_t position_t;

// Max and min
#define FILE_OFFSET_T_MAX (~(file_offset_t)0)
#define POSITION_T_MAX ((position_t)((~(uint64_t)0)>>1))
#define SIZE_T_MAX (~(size_t)0)

#endif /* #ifndef TIPPSE_TYPES_H */