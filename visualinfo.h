#ifndef __TIPPSE_VISUALINFO__
#define __TIPPSE_VISUALINFO__

#include <stdlib.h>

#define VISUAL_INFO_COMMENT0 0
#define VISUAL_INFO_COMMENT1 1
#define VISUAL_INFO_COMMENT2 2
#define VISUAL_INFO_COMMENT3 3
#define VISUAL_INFO_STRING0 4
#define VISUAL_INFO_STRING1 5
#define VISUAL_INFO_STRING2 6
#define VISUAL_INFO_STRING3 7
#define VISUAL_INFO_BRACKET0 8
#define VISUAL_INFO_BRACKET1 9
#define VISUAL_INFO_BRACKET2 10
#define VISUAL_INFO_BRACKET3 11
#define VISUAL_INFO_MAX 12

struct visual_info_detail {
  int in;
  int out;
};

struct visual_info {
  struct visual_info_detail detail[VISUAL_INFO_MAX];
};

void visual_info_clear(struct visual_info* visuals);
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right);

#endif /* #ifndef __TIPPSE_VISUALINFO__ */
