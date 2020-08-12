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
#define VISUAL_DIRTY_UPDATE 0x1
#define VISUAL_DIRTY_LASTSPLIT 0x2
#define VISUAL_DIRTY_SPLITTED 0x4
#define VISUAL_DIRTY_LEFT 0x8

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
#define VISUAL_FLAG_COLOR_DIRECTORY 20
#define VISUAL_FLAG_COLOR_MODIFIED 21
#define VISUAL_FLAG_COLOR_REMOVED 22
#define VISUAL_FLAG_COLOR_MAX 23

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
#define VISUAL_BRACKET_MAX 3

// Block structure for bracket matching and code folding
struct visual_bracket {
  int diff;                 // Difference level
  int min;                  // Lowest level
  int max;                  // Highest level
};

// Block visualisation hints per page
struct visual_info {
  file_offset_t characters; // Characters (with current encoding)
  position_t columns;       // Characters from last line in page
  position_t lines;         // Lines in page
  position_t xs;            // Size of last screen row in page
  position_t ys;            // Screen rows in page
  long indentation;         // Common indentation of last screen row in page
  long indentation_extra;   // Extra indentation of last screen row in page (for indentation marker)
  int detail_before;        // Visual details after last page
  int detail_after;         // Visual details after current page
  int keyword_color;        // Color for current active keyword
  long keyword_length;       // Length of current active keyword
  file_offset_t displacement; // Offset to begin of first character
  file_offset_t rewind;     // Relative offset (backwards) to begin of the last keyword/character
  int dirty;                // Mark page as dirty (not completely rendered yet)
  struct visual_bracket brackets[VISUAL_BRACKET_MAX]; // Bracket depth
  struct visual_bracket brackets_line[VISUAL_BRACKET_MAX]; // Bracket depth of line
};

void visual_info_clear(struct visual_info* visuals);
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right);
void visual_info_invalidate(struct range_tree_node* node, struct range_tree* tree);

struct range_tree_node* visual_info_find(struct range_tree_node* node, int find_type, file_offset_t find_offset, position_t find_x, position_t find_y, position_t find_line, position_t find_column, file_offset_t* offset, position_t* x, position_t* y, position_t* line, position_t* column, int* indentation, int* indentation_extra, file_offset_t* character, int retry, file_offset_t before);
int visual_info_find_bracket(struct range_tree_node* node, size_t bracket);
struct range_tree_node* visual_info_find_bracket_forward(struct range_tree_node* node, size_t bracket, int search);
struct range_tree_node* visual_info_find_bracket_backward(struct range_tree_node* node, size_t bracket, int search);
void visual_info_find_bracket_lowest(struct range_tree_node* node, int* mins, struct range_tree_node* last);
struct range_tree_node* visual_info_find_indentation_last(struct range_tree_node* node, position_t lines, struct range_tree_node* last);
int visual_info_find_indentation(struct range_tree_node* node);
int visual_info_find_whitespaced(struct range_tree_node* node);

struct visual_info* visual_info_create(struct visual_info** base);
void visual_info_destroy(struct visual_info** base);

#endif /* #ifndef TIPPSE_VISUALINFO_H */
