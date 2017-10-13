#ifndef TIPPSE_TYPES_H
#define TIPPSE_TYPES_H

#include <stdlib.h>
#include <stdint.h>

// This one limits the maximum file offset
typedef uint64_t file_offset_t;

// And this one limits the maximum cursor position etc. (signed type needed)
typedef int64_t position_t;

#endif /* #ifndef TIPPSE_TYPES_H */
