/* Tippse - Visual information store - Holds information about variable length or long visuals like comments/strings etc.  */

#include "visualinfo.h"

void visual_info_clear(struct visual_info* visuals) {
  size_t n;
  for(n = 0; n<VISUAL_INFO_MAX; n++) {
    visuals->detail[n].in = 0;
    visuals->detail[n].out = 0;
  }
}

void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right) {
  size_t n;
  for(n = 0; n<VISUAL_INFO_MAX; n++) {
    visuals->detail[n].in = left->detail[n].in+right->detail[n].in;
    visuals->detail[n].out = left->detail[n].out+right->detail[n].out;
  }
}