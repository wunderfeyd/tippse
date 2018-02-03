// Tippse - Visual information store - Holds information about variable length or long visuals like comments/strings etc.

#include "visualinfo.h"

// color entry names
struct config_cache visual_color_codes[VISUAL_FLAG_COLOR_MAX+1] = {
  {"background", VISUAL_FLAG_COLOR_BACKGROUND, NULL},
  {"text", VISUAL_FLAG_COLOR_TEXT, NULL},
  {"selection", VISUAL_FLAG_COLOR_SELECTION, NULL},
  {"status", VISUAL_FLAG_COLOR_STATUS, NULL},
  {"frame", VISUAL_FLAG_COLOR_FRAME, NULL},
  {"string", VISUAL_FLAG_COLOR_STRING, NULL},
  {"type", VISUAL_FLAG_COLOR_TYPE, NULL},
  {"keyword", VISUAL_FLAG_COLOR_KEYWORD, NULL},
  {"preprocessor", VISUAL_FLAG_COLOR_PREPROCESSOR, NULL},
  {"linecomment", VISUAL_FLAG_COLOR_LINECOMMENT, NULL},
  {"blockcomment", VISUAL_FLAG_COLOR_BLOCKCOMMENT, NULL},
  {"plus", VISUAL_FLAG_COLOR_PLUS, NULL},
  {"minus", VISUAL_FLAG_COLOR_MINUS, NULL},
  {"bracket", VISUAL_FLAG_COLOR_BRACKET, NULL},
  {"linenumber", VISUAL_FLAG_COLOR_LINENUMBER, NULL},
  {"bracketerror", VISUAL_FLAG_COLOR_BRACKETERROR, NULL},
  {"consolenormal", VISUAL_FLAG_COLOR_CONSOLENORMAL, NULL},
  {"consolewarning", VISUAL_FLAG_COLOR_CONSOLEWARNING, NULL},
  {"consoleerror", VISUAL_FLAG_COLOR_CONSOLEERROR, NULL},
  {"bookmark", VISUAL_FLAG_COLOR_BOOKMARK, NULL},
  {NULL, 0, NULL}
};

// Reset structure to known state
void visual_info_clear(struct visual_info* visuals) {
  memset(visuals, 0, sizeof(struct visual_info));
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
  visuals->detail_after = ((left->detail_after&right->detail_after)&VISUAL_DETAIL_WHITESPACED_COMPLETE)|((left->detail_after|right->detail_after)&(VISUAL_DETAIL_WHITESPACED_START|VISUAL_DETAIL_STOPPED_INDENTATION|VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_NEWLINE));

  int dirty = (left->dirty|right->dirty)&VISUAL_DIRTY_UPDATE;
  dirty |= (left->dirty)&(VISUAL_DIRTY_SPLITTED|VISUAL_DIRTY_LEFT);

  if (!(left->dirty&VISUAL_DIRTY_UPDATE) && (right->dirty&VISUAL_DIRTY_LEFT) && !(dirty&VISUAL_DIRTY_SPLITTED)) {
    dirty |= VISUAL_DIRTY_LASTSPLIT|VISUAL_DIRTY_SPLITTED;
  }

  visuals->dirty = dirty;

  for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
    visuals->brackets[n].diff = right->brackets[n].diff+left->brackets[n].diff;

    visuals->brackets[n].max = right->brackets[n].max+left->brackets[n].diff;
    if (visuals->brackets[n].max<left->brackets[n].max) {
      visuals->brackets[n].max = left->brackets[n].max;
    }

    visuals->brackets[n].min = right->brackets[n].min-left->brackets[n].diff;
    if (visuals->brackets[n].min<left->brackets[n].min) {
      visuals->brackets[n].min = left->brackets[n].min;
    }

    visuals->brackets_line[n].diff = right->brackets_line[n].diff+left->brackets_line[n].diff;

    visuals->brackets_line[n].max = right->brackets_line[n].max+left->brackets_line[n].diff;
    if (visuals->brackets_line[n].max<left->brackets_line[n].max) {
      visuals->brackets_line[n].max = left->brackets_line[n].max;
    }

    visuals->brackets_line[n].min = right->brackets_line[n].min-left->brackets_line[n].diff;
    if (visuals->brackets_line[n].min<left->brackets_line[n].min) {
      visuals->brackets_line[n].min = left->brackets_line[n].min;
    }
  }
}
