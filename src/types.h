#ifndef TIPPSE_TYPES_H
#define TIPPSE_TYPES_H

#include <stdlib.h>
#include <stdint.h>

// This one limits the maximum file offset
typedef uint64_t file_offset_t;

// And this one limits the maximum cursor position etc. (signed type needed)
typedef int64_t position_t;

// Type for code points (signed type needed at the moment)
typedef int32_t codepoint_t;

// Type for unicode bit tables
typedef unsigned long codepoint_table_t;

// Type for boolean values
typedef int bool_t;

// Max and min
#define FILE_OFFSET_T_MAX (~(file_offset_t)0)
#define POSITION_T_MAX ((position_t)((~(uint64_t)0)>>1))
#define POSITION_T_MIN ((position_t)(~(uint64_t)POSITION_T_MAX))
#define SIZE_T_MAX (~(size_t)0)

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

// Unused results
inline void unused_result(int result) {}
#define UNUSED(a) unused_result(a?1:0)

#ifndef __SMALLEST__
#ifdef __GNUC__
//#define TIPPSE_INLINE inline __attribute__((always_inline))
#define TIPPSE_INLINE inline
#else
#define TIPPSE_INLINE inline
#endif
#else
#define TIPPSE_INLINE inline
#endif

// Forward declarations
struct config;
struct config_command;
struct document;
struct document_file;
struct document_hex;
struct document_text;
struct document_text_render_info;
struct document_undo;
struct document_view;
struct directory;
struct editor;
struct encoding;
struct encoding_cache;
struct encoding_cache_codepoint;
struct file;
struct file_type;
struct file_cache;
struct file_cache_node;
struct fragment;
struct list;
struct list_node;
struct range_tree_node;
struct screen;
struct screen_char;
struct search;
struct splitter;
struct stream;
struct trie;
struct trie_node;
struct unicode_sequence;
struct unicode_sequencer;

#endif /* #ifndef TIPPSE_TYPES_H */
