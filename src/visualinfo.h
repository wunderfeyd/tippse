#ifndef TIPPSE_VISUALINFO_H
#define TIPPSE_VISUALINFO_H

#include <stdlib.h>
#include <string.h>
#include "types.h"

// Lower bits (until value VISUAL_DETAIL_STATEMASK) representing the parser internal state and could be used differently depending on parser
#define VISUAL_DETAIL_COMMENT0 0x1
#define VISUAL_DETAIL_COMMENT1 0x2
#define VISUAL_DETAIL_COMMENT2 0x4
#define VISUAL_DETAIL_COMMENT3 0x8
#define VISUAL_DETAIL_STRING0 0x10
#define VISUAL_DETAIL_STRING1 0x20
#define VISUAL_DETAIL_STRING2 0x40
#define VISUAL_DETAIL_STRING3 0x80
#define VISUAL_DETAIL_STRINGESCAPE 0x100
#define VISUAL_DETAIL_STATEMASK 0xffff
#define VISUAL_DETAIL_INDENTATION 0x10000
#define VISUAL_DETAIL_NEWLINE 0x20000
#define VISUAL_DETAIL_WORD 0x40000
#define VISUAL_DETAIL_PREPROCESSOR 0x80000
#define VISUAL_DETAIL_WHITESPACED_COMPLETE 0x100000
#define VISUAL_DETAIL_WHITESPACED_START 0x200000
#define VISUAL_DETAIL_CONTROLCHARACTER 0x400000
#define VISUAL_DETAIL_WRAPPING 0x1000000
#define VISUAL_DETAIL_SHOW_INVISIBLES 0x2000000
#define VISUAL_DETAIL_WRAPPED 0x8000000
#define VISUAL_DETAIL_STOPPED_INDENTATION 0x10000000

// Flags for page dirtiness
#define VISUAL_DIRTY_UPDATE 1
#define VISUAL_DIRTY_LASTSPLIT 2
#define VISUAL_DIRTY_SPLITTED 4
#define VISUAL_DIRTY_LEFT 8

// Return flags for document renderer
#define VISUAL_FLAG_COLOR_BACKGROUND 0
#define VISUAL_FLAG_COLOR_TEXT 1
#define VISUAL_FLAG_COLOR_SELECTION 2
#define VISUAL_FLAG_COLOR_STATUS 3
#define VISUAL_FLAG_COLOR_FRAME 4
#define VISUAL_FLAG_COLOR_STRING 5
#define VISUAL_FLAG_COLOR_TYPE 6
#define VISUAL_FLAG_COLOR_KEYWORD 7
#define VISUAL_FLAG_COLOR_PREPROCESSOR 8
#define VISUAL_FLAG_COLOR_LINECOMMENT 9
#define VISUAL_FLAG_COLOR_BLOCKCOMMENT 10
#define VISUAL_FLAG_COLOR_PLUS 11
#define VISUAL_FLAG_COLOR_MINUS 12
#define VISUAL_FLAG_COLOR_BRACKET 13
#define VISUAL_FLAG_COLOR_LINENUMBER 14
#define VISUAL_FLAG_COLOR_BRACKETERROR 15
#define VISUAL_FLAG_COLOR_CONSOLENORMAL 16
#define VISUAL_FLAG_COLOR_CONSOLEWARNING 17
#define VISUAL_FLAG_COLOR_CONSOLEERROR 18
#define VISUAL_FLAG_COLOR_BOOKMARK 19
#define VISUAL_FLAG_COLOR_MAX 20

// Flags for page finding processes
#define VISUAL_SEEK_NONE 0
#define VISUAL_SEEK_OFFSET 1
#define VISUAL_SEEK_X_Y 2
#define VISUAL_SEEK_LINE_COLUMN 3
#define VISUAL_SEEK_BRACKET_NEXT 4
#define VISUAL_SEEK_BRACKET_PREV 5
#define VISUAL_SEEK_INDENTATION_LAST 6
#define VISUAL_SEEK_WORD_TRANSITION_NEXT 7
#define VISUAL_SEEK_WORD_TRANSITION_PREV 8

// Bracket definitions
#define VISUAL_BRACKET_MASK 0xffff
#define VISUAL_BRACKET_OPEN 0x10000
#define VISUAL_BRACKET_CLOSE 0x20000
#define VISUAL_BRACKET_MAX 4

#define VISUAL_BRACKET_USED_LINE 0x100

// Block structure for bracket matching and code folding
struct visual_bracket {
  int diff;
  int min;
  int max;
};

// Block visualisation hints per page
struct visual_info {
  file_offset_t characters; // Characters (with current encoding)
  position_t columns;       // Characters from last line in page
  position_t lines;         // Lines in page
  position_t xs;            // Size of last screen row in page
  position_t ys;            // Screen rows in page
  int indentation;          // Common indentation of last screen row in page
  int indentation_extra;    // Extra indentation of last screen row in page (for indentation marker)
  int detail_before;        // Visual details after last page
  int detail_after;         // Visual details after current page
  int keyword_color;        // Color for current active keyword
  int keyword_length;       // Length of current active keyword
  file_offset_t displacement; // Offset to begin of first character
  file_offset_t rewind;     // Relative offset (backwards) to begin of the last keyword/character
  int dirty;                // Mark page as dirty (not completely rendered yet)
  struct visual_bracket brackets[VISUAL_BRACKET_MAX]; // Bracket depth
  struct visual_bracket brackets_line[VISUAL_BRACKET_MAX]; // Bracket depth of line
};

#include "config.h"

void visual_info_clear(struct visual_info* visuals);
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right);

#endif /* #ifndef TIPPSE_VISUALINFO_H */
