#ifndef __TIPPSE_VISUALINFO__
#define __TIPPSE_VISUALINFO__

#include <stdlib.h>

#define VISUAL_INFO_COMMENT0 1
#define VISUAL_INFO_COMMENT1 2
#define VISUAL_INFO_COMMENT2 4
#define VISUAL_INFO_COMMENT3 8
#define VISUAL_INFO_STRING0 16
#define VISUAL_INFO_STRING1 32
#define VISUAL_INFO_STRING2 64
#define VISUAL_INFO_STRING3 128
#define VISUAL_INFO_STRINGESCAPE 256

#define VISUAL_DIRTY_UPDATE 1
#define VISUAL_DIRTY_LASTSPLIT 2
#define VISUAL_DIRTY_SPLITTED 4
#define VISUAL_DIRTY_LEFT 8

#define VISUAL_FLAG_COLOR_STRING 1
#define VISUAL_FLAG_COLOR_TYPE 2
#define VISUAL_FLAG_COLOR_KEYWORD 3
#define VISUAL_FLAG_COLOR_PREPROCESSOR 4
#define VISUAL_FLAG_COLOR_LINECOMMENT 5
#define VISUAL_FLAG_COLOR_BLOCKCOMMENT 6

struct visual_info_detail {
  int in;
  int out;
};

struct visual_info {
  int detail_before;
  int detail_after;
  int dirty;
};

void visual_info_clear(struct visual_info* visuals);
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right);

#endif /* #ifndef __TIPPSE_VISUALINFO__ */
