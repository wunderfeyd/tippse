/* Tippse - Visual information store - Holds information about variable length or long visuals like comments/strings etc. */

#include "visualinfo.h"

// Reset structure to known state
void visual_info_clear(struct visual_info* visuals) {
  visuals->characters = 0;
  visuals->columns = 0;
  visuals->lines = 0;
  visuals->xs = 0;
  visuals->ys = 0;
  visuals->indentation = 0;
  visuals->indentation_extra = 0;
  visuals->detail_after = 0;
  visuals->detail_before = 0;
  visuals->keyword_length = 0;
  visuals->keyword_color = 0;
  visuals->displacement = 0;
  visuals->dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
}

// Update page with informations about two other pages (usally its children)
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right) {
  visuals->characters = left->characters+right->characters;
  visuals->lines = left->lines+right->lines;
  visuals->ys = left->ys+right->ys;
  if (right->ys!=0) {
    visuals->xs = right->xs;
  } else {
    visuals->xs = left->xs+right->xs;
  }

  if (right->lines!=0) {
    visuals->columns = right->columns;
    visuals->indentation = right->indentation;
    visuals->indentation_extra = right->indentation_extra;
  } else {
    visuals->columns = left->columns+right->columns;
    visuals->indentation = left->indentation+right->indentation;
    visuals->indentation_extra = left->indentation_extra+right->indentation_extra;
  }

  visuals->detail_before = left->detail_before;
  visuals->detail_after = ((left->detail_after&right->detail_after)&VISUAL_INFO_WHITESPACED_COMPLETE)|((left->detail_after|right->detail_after)&VISUAL_INFO_WHITESPACED_START);

  int dirty = (left->dirty|right->dirty)&VISUAL_DIRTY_UPDATE;
  dirty |= (left->dirty)&(VISUAL_DIRTY_SPLITTED|VISUAL_DIRTY_LEFT);

  if (!(left->dirty&VISUAL_DIRTY_UPDATE) && (right->dirty&VISUAL_DIRTY_LEFT) && !(dirty&VISUAL_DIRTY_SPLITTED)) {
    dirty |= VISUAL_DIRTY_LASTSPLIT|VISUAL_DIRTY_SPLITTED;
  }

  visuals->dirty = dirty;
}
