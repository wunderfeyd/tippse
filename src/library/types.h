#ifndef TIPPSE_LIBRARY_TYPES_H
#define TIPPSE_LIBRARY_TYPES_H

#include <stdlib.h>
#include <stdint.h>

#ifdef _MSC_VER
#include <stdio.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#endif

// This one limits the maximum file offset
typedef uint64_t file_offset_t;

// And this one limits the maximum cursor position etc. (signed type needed)
typedef int64_t position_t;

// Type for code points
typedef unsigned long codepoint_t;

// Type for unicode bit tables
typedef uint16_t codepoint_table_t;

// Type for boolean values (stdbool.h seems to degrade the performance ... TODO: check why)
typedef long bool_t;

// Max and min
#define FILE_OFFSET_T_MAX (~(file_offset_t)0)
#define POSITION_T_MAX ((position_t)((~(uint64_t)0)>>1))
#define POSITION_T_MIN ((position_t)(~(uint64_t)POSITION_T_MAX))
#define SIZE_T_MAX (~(size_t)0)
#define SIZE_T_HALF (((~(size_t)0)>>1)+1)

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)
#else
#define LIKELY(x) x
#define UNLIKELY(x) x
#endif

#ifdef __GNUC__
#define PREFETCH(x, rw, locality) __builtin_prefetch(x, rw, locality)
#else
#define PREFETCH(x, rw, locality)
#endif

#ifndef __SMALLEST__
//#define TIPPSE_INLINE inline __attribute__((always_inline))
#ifndef _MSC_VER
#define TIPPSE_INLINE static inline
#else
#define TIPPSE_INLINE static __inline
#endif
#else
#ifndef _MSC_VER
#define TIPPSE_INLINE static inline
#else
#define TIPPSE_INLINE static __inline
#endif
#endif

// Unused results
TIPPSE_INLINE void unused_result(int result) {}
#define UNUSED(a) unused_result(a?1:0)

// Forward declarations
struct directory;
struct encoding;
struct file;
struct file_cache;
struct file_cache_node;
struct fragment;
struct list;
struct list_node;
struct mutex;
struct range_tree;
struct range_tree_node;
struct search;
struct stream;
struct thread;
struct trie;
struct trie_node;
struct unicode_sequence;
struct unicode_sequencer;

#endif /* #ifndef TIPPSE_LIBRARY_TYPES_H */
