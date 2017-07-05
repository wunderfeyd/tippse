/* Tippse - Visual information store - Holds information about variable length or long visuals like comments/strings etc.  */

#include "visualinfo.h"

void visual_info_clear(struct visual_info* visuals) {
  visuals->columns = 0;
  visuals->rows = 0;
  visuals->indentation = 0;
  visuals->detail_after = 0;
  visuals->detail_before = 0;
  visuals->dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
}

void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right) {
  visuals->rows = left->rows+right->rows;
  if (right->rows!=0) {
    visuals->columns = right->columns;
    visuals->indentation = right->indentation;
  } else {
    visuals->columns = left->columns+right->columns;
    visuals->indentation = left->indentation+right->indentation;
  }

  visuals->detail_before = left->detail_before;
  visuals->detail_after = right->detail_after;

  int dirty = (left->dirty|right->dirty)&VISUAL_DIRTY_UPDATE;
  dirty |= (left->dirty)&(VISUAL_DIRTY_SPLITTED|VISUAL_DIRTY_LEFT);

  if (!(left->dirty&VISUAL_DIRTY_UPDATE) && (right->dirty&VISUAL_DIRTY_LEFT) && !(dirty&VISUAL_DIRTY_SPLITTED)) {
    dirty |= VISUAL_DIRTY_LASTSPLIT|VISUAL_DIRTY_SPLITTED;
  }

  visuals->dirty = dirty;
}
