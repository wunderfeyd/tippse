#ifndef __TIPPSE_VISUALINFO__
#define __TIPPSE_VISUALINFO__

#include <stdlib.h>
#include "types.h"

// Flags for visual details (TODO: Rename me VISUAL_INFO->VISUAL_DETAIL)
#define VISUAL_INFO_COMMENT0 1
#define VISUAL_INFO_COMMENT1 2
#define VISUAL_INFO_COMMENT2 4
#define VISUAL_INFO_COMMENT3 8
#define VISUAL_INFO_STRING0 16
#define VISUAL_INFO_STRING1 32
#define VISUAL_INFO_STRING2 64
#define VISUAL_INFO_STRING3 128
#define VISUAL_INFO_STRINGESCAPE 256
#define VISUAL_INFO_INDENTATION 512
#define VISUAL_INFO_NEWLINE 1024
#define VISUAL_INFO_WORD 2048
#define VISUAL_INFO_PREPROCESSOR 4096
#define VISUAL_INFO_WHITESPACED_COMPLETE 8192
#define VISUAL_INFO_WHITESPACED_START 16384
#define VISUAL_INFO_CONTROLCHARACTER 32768

// Flags for page dirtiness
#define VISUAL_DIRTY_UPDATE 1
#define VISUAL_DIRTY_LASTSPLIT 2
#define VISUAL_DIRTY_SPLITTED 4
#define VISUAL_DIRTY_LEFT 8

// Return flags for document renderer
#define VISUAL_FLAG_COLOR_STRING 1
#define VISUAL_FLAG_COLOR_TYPE 2
#define VISUAL_FLAG_COLOR_KEYWORD 3
#define VISUAL_FLAG_COLOR_PREPROCESSOR 4
#define VISUAL_FLAG_COLOR_LINECOMMENT 5
#define VISUAL_FLAG_COLOR_BLOCKCOMMENT 6

// Flags for page finding processes
#define VISUAL_SEEK_NONE 0
#define VISUAL_SEEK_OFFSET 1
#define VISUAL_SEEK_X_Y 2
#define VISUAL_SEEK_LINE_COLUMN 3

// Block visualisation hints per page
struct visual_info {
  file_offset_t characters; // Characters (with current encoding)
  file_offset_t columns;    // Characters from last line in page
  file_offset_t lines;      // Lines in page
  file_offset_t xs;         // Size of last screen row in page
  file_offset_t ys;         // Screen rows in page
  int indentation;          // Common indentation of last screen row in page
  int indentation_extra;    // Extra indentation of last screen row in page (for indentation marker)
  int detail_before;        // Visual details after last page
  int detail_after;         // Visual details after current page
  int keyword_color;        // Color for current active keyword
  int keyword_length;       // Length of current active keyword
  file_offset_t displacement; // Offset to begin of first character
  file_offset_t displacement_rewind; // Relative offset (backwards) to begin of the last code point
  int dirty;                // Mark page as dirty (not completely rendered yet)
};

void visual_info_clear(struct visual_info* visuals);
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right);

#endif /* #ifndef __TIPPSE_VISUALINFO__ */
