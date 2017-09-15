/* Tippse - Document Text View - Cursor and display operations for 2d text mode */

#include "document_text.h"

struct document* document_text_create() {
  struct document_text* document = (struct document_text*)malloc(sizeof(struct document_text));
  document->keep_status = 0;
  document->show_scrollbar = 0;
  document->vtbl.reset = document_text_reset;
  document->vtbl.draw = document_text_draw;
  document->vtbl.keypress = document_text_keypress;
  document->vtbl.incremental_update = document_text_incremental_update;

  return (struct document*)document;
}

void document_text_destroy(struct document* base) {
  free(base);
}

// Called after a new document was assigned
void document_text_reset(struct document* base, struct splitter* splitter) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (file->buffer) {
    struct range_tree_node* node = range_tree_first(file->buffer);
    if (node) {
      range_tree_shrink(node);
    }
  }

  struct document_text_position in;
  struct document_text_position out;
  in.type = VISUAL_SEEK_OFFSET;
  in.offset = view->offset;
  in.clip = 0;

  view->offset = document_text_cursor_position(splitter, &in, &out, 0, 1);
}

// Goto specified location, apply cursor clipping/wrapping and render dirty pages as necessary
file_offset_t document_text_cursor_position(struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  struct document_text_render_info render_info;

  document_text_render_clear(&render_info, splitter->client_width);
  while (1) {
    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, in);
      if (document_text_render_span(&render_info, NULL, NULL, view, file, in, out, ~0, cancel)) {
        break;
      }
    }

    if (in->type==VISUAL_SEEK_OFFSET) {
      break;
    }

    int* x;
    int* y;
    if (in->type==VISUAL_SEEK_X_Y) {
      x = &in->x;
      y = &in->y;
    } else {
      x = &in->column;
      y = &in->line;
    }

    if (!out->y_drawn) {
      if (*y<0) {
        *y = 0;
        *x = 0;
        continue;
      } else if (*y>0) {
        if (in->type==VISUAL_SEEK_X_Y) {
          *y = file->buffer?file->buffer->visuals.ys:0;
        } else {
          *y = file->buffer?file->buffer->visuals.lines:0;
        }

        *x = 100000000;
        continue;
      }
    } else {
      if (*x>out->x_max) {
        if (wrap) {
          *y = (*y)+1;
          *x = 0;
          wrap = 0;
          continue;
        } else {
          *x = out->x_max;
          continue;
        }
      } else if (*x<out->x_min) {
        if (*y>0 && wrap) {
          *y = (*y)-1;
          *x = 100000000;
          wrap = 0;
          continue;
        } else {
          *x = out->x_min;
          continue;
        }
      }
    }

    break;
  }

  return out->offset;
}

// Clear renderer state to ensure a restart at next seek
void document_text_render_clear(struct document_text_render_info* render_info, int width) {
  memset(render_info, 0, sizeof(struct document_text_render_info));
  render_info->buffer = NULL;
  render_info->width = width;
  render_info->offset = 0;
}

// Update renderer state to restart at the given position or to continue if possible
void document_text_render_seek(struct document_text_render_info* render_info, struct range_tree_node* buffer, struct encoding* encoding, struct document_text_position* in) {
  file_offset_t offset_new = 0;
  int x_new = 0;
  int y_new = 0;
  int lines_new = 0;
  int columns_new = 0;
  int indentation_new = 0;
  int indentation_extra_new = 0;
  file_offset_t characters_new = 0;
  struct range_tree_node* buffer_new;

  int type = in->type;
  if (type==VISUAL_SEEK_BRACKET_NEXT) {
    type = VISUAL_SEEK_OFFSET;
  } else if (type==VISUAL_SEEK_BRACKET_PREV) {
    type = VISUAL_SEEK_OFFSET;
  }

  buffer_new = range_tree_find_visual(buffer, type, in->offset, in->x, in->y, in->line, in->column, &offset_new, &x_new, &y_new, &lines_new, &columns_new, &indentation_new, &indentation_extra_new, &characters_new);

  if (buffer_new /*&& (render_info->buffer!=buffer_new || (in->type==VISUAL_SEEK_OFFSET && render_info->offset!=in->offset) || (in->type==VISUAL_SEEK_X_Y && render_info->x!=in->x && render_info->y!=in->y) || (in->type==VISUAL_SEEK_LINE_COLUMN && render_info->line!=in->line && render_info->column!=in->column))*/) {
    render_info->visual_detail = buffer_new->visuals.detail_before;
    render_info->offset_sync = offset_new-buffer_new->visuals.rewind;
    render_info->displacement = buffer_new->visuals.displacement;
    render_info->offset = offset_new+buffer_new->visuals.displacement;
    if (offset_new==0) {
      render_info->visual_detail = VISUAL_INFO_NEWLINE;
    }

    render_info->whitespaced = range_tree_find_whitespaced(buffer_new);

    render_info->visual_detail |= VISUAL_INFO_WHITESPACED_COMPLETE;
    render_info->visual_detail &= ~VISUAL_INFO_WHITESPACED_START;

    render_info->indentation_extra = indentation_extra_new;
    render_info->indentation = indentation_new;
    render_info->x = x_new+render_info->indentation+render_info->indentation_extra;
    render_info->y_view = y_new;
    render_info->line = lines_new;
    render_info->column = columns_new;
    render_info->character = characters_new;

    if (render_info->indentation+render_info->indentation_extra>0 && x_new==0 && !(render_info->visual_detail&VISUAL_INFO_INDENTATION)) {
      render_info->draw_indentation = 1;
    } else {
      render_info->draw_indentation = 0;
    }

    render_info->xs = 0;
    render_info->ys = 0;
    render_info->lines = 0;
    render_info->columns = 0;
    render_info->characters = 0;
    render_info->indentations = 0;
    render_info->indentations_extra = 0;
    render_info->buffer = buffer_new;
    render_info->keyword_color = buffer_new->visuals.keyword_color;
    render_info->keyword_length = buffer_new->visuals.keyword_length;
    for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
      render_info->depth_new[n] = range_tree_find_bracket(buffer_new, n);
      render_info->depth_old[n] = render_info->depth_new[n];
      render_info->brackets[n].diff = 0;
      render_info->brackets[n].min = 0;
      render_info->brackets[n].max = 0;
    }

    encoding_stream_from_page(&render_info->stream, render_info->buffer, render_info->displacement);
    encoding_cache_clear(&render_info->cache, encoding, &render_info->stream);
  } else {
    encoding_stream_from_page(&render_info->stream, NULL, 0);
    encoding_cache_clear(&render_info->cache, encoding, &render_info->stream);
  }

  if (in->type==VISUAL_SEEK_X_Y) {
    render_info->y = in->y;
  } else {
    render_info->y = y_new;
  }
}

// Get word length from specified position
int document_text_render_lookahead_word_wrap(struct document_file* file, struct encoding_cache* cache, int max) {
  int count = 0;
  int advanced = 0;
  while (count<max) {
    int codepoints[8];
    size_t advance = 1;
    size_t length = 1;
    unicode_read_combined_sequence(cache, advanced, &codepoints[0], 8, &advance, &length);

    if (codepoints[0]<=' ') {
      break;
    }

    count++;
    advanced += advance;
  }

  return count;
}

// Render some pages until the position is found or pages are no longer dirty
// TODO: find better visualization for debugging, find unnecessary render iterations and then optimize (as soon the code is "feature complete")
int document_text_render_span(struct document_text_render_info* render_info, struct screen* screen, struct splitter* splitter, struct document_view* view, struct document_file* file, struct document_text_position* in, struct document_text_position* out, int dirty_pages, int cancel) {
  // TODO: Following initializations shouldn't be needed since the caller should check the type / verify
  if (out) {
    out->type = VISUAL_SEEK_NONE;
    out->offset = 0;
    out->x = 0;
    out->y = 0;
    out->x_min = 0;
    out->x_max = 0;
    out->y_drawn = 0;
    out->line = 0;
    out->column = 0;
    out->character = 0;
    out->buffer = NULL;
    out->displacement = 0;
  }

  int rendered = 1;
  int stop = render_info->buffer?0:1;
  int page_count = 0;
  int page_dirty = (render_info->buffer && render_info->buffer->visuals.dirty)?1:0;

  render_info->visual_detail |= (view->wrapping)?VISUAL_INFO_WRAPPING:0;
  render_info->visual_detail |= (view->showall)?VISUAL_INFO_SHOWALL:0;
  render_info->visual_detail |= (view->continuous)?VISUAL_INFO_CONTINUOUS:0;

  int newline_cp1 = (file->newline==TIPPSE_NEWLINE_CR)?'\r':'\n';
  int newline_cp2 = (file->newline==TIPPSE_NEWLINE_CRLF)?'\r':0;

  while (1) {
    int boundary = 0;
    while (render_info->buffer && render_info->displacement>=render_info->buffer->length) {
      boundary = 1;
      int dirty = (render_info->buffer->visuals.xs!=render_info->xs || render_info->buffer->visuals.ys!=render_info->ys || render_info->buffer->visuals.lines!=render_info->lines ||  render_info->buffer->visuals.columns!=render_info->columns || render_info->buffer->visuals.indentation!=render_info->indentations || render_info->buffer->visuals.indentation_extra!=render_info->indentations_extra || render_info->buffer->visuals.detail_after!=render_info->visual_detail || (render_info->ys==0 && render_info->buffer->visuals.dirty && (view->wrapping || view->continuous)))?1:0;

      if (render_info->buffer->visuals.dirty || dirty) {
        render_info->buffer->visuals.dirty = 0;
        render_info->buffer->visuals.xs = render_info->xs;
        render_info->buffer->visuals.ys = render_info->ys;
        render_info->buffer->visuals.lines = render_info->lines;
        render_info->buffer->visuals.columns = render_info->columns;
        render_info->buffer->visuals.characters = render_info->characters;
        render_info->buffer->visuals.indentation = render_info->indentations;
        render_info->buffer->visuals.indentation_extra = render_info->indentations_extra;
        render_info->buffer->visuals.detail_after = render_info->visual_detail;
        for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
          render_info->buffer->visuals.brackets[n] = render_info->brackets[n];
        }

        range_tree_update_calc_all(render_info->buffer);
      }

      render_info->displacement -= render_info->buffer->length;
      file_offset_t rewind = render_info->offset-render_info->offset_sync-render_info->displacement;
      if (render_info->displacement==0 && render_info->keyword_length==0) { // TODO: this is somehow not as thought, at best this condition wouldn't be necessary
        rewind = 0;
      }

      render_info->buffer = range_tree_next(render_info->buffer);

      if (render_info->buffer) {
        if (render_info->visual_detail!=render_info->buffer->visuals.detail_before || render_info->keyword_length!=render_info->buffer->visuals.keyword_length || (render_info->keyword_color!=render_info->buffer->visuals.keyword_color && render_info->keyword_length>0) || render_info->displacement!=render_info->buffer->visuals.displacement || rewind!=render_info->buffer->visuals.rewind || dirty) {
          render_info->buffer->visuals.keyword_length = render_info->keyword_length;
          render_info->buffer->visuals.keyword_color = render_info->keyword_color;
          render_info->buffer->visuals.detail_before = render_info->visual_detail;
          render_info->buffer->visuals.displacement = render_info->displacement;
          render_info->buffer->visuals.rewind = rewind;
          render_info->buffer->visuals.dirty |= VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
          range_tree_update_calc_all(render_info->buffer);
        }

        if (render_info->buffer->visuals.dirty) {
          if (dirty_pages!=~0) {
            dirty_pages--;
            if (dirty_pages==0 && stop==0) {
              stop = 2;
            }
          }

          page_count = 0;
        } else {
          if (dirty_pages==~0) {
            page_count++;
          }
        }
      } else {
        if (stop==0) {
          stop = 2;
        }
      }

      render_info->indentations = 0;
      render_info->indentations_extra = 0;
      render_info->xs = 0;
      render_info->ys = 0;
      render_info->columns = 0;
      render_info->lines = 0;
      render_info->characters = 0;
      render_info->visual_detail |= VISUAL_INFO_WHITESPACED_COMPLETE;
      render_info->visual_detail &= ~VISUAL_INFO_WHITESPACED_START;
      for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
        render_info->brackets[n].diff = 0;
        render_info->brackets[n].min = 0;
        render_info->brackets[n].max = 0;
        render_info->depth_old[n] = render_info->depth_new[n];
      }

      page_dirty = (render_info->buffer && render_info->buffer->visuals.dirty)?1:0;
    }

    int codepoints[8];
    size_t advance = 1;
    size_t length = 1;
    size_t read = unicode_read_combined_sequence(&render_info->cache, 0, &codepoints[0], 8, &advance, &length);

    int cp = codepoints[0];

//    size_t length = encoding_cache_find_length(&render_info->cache, 0);
//    int cp = encoding_cache_find_codepoint(&render_info->cache, 0);

//    printf("%d %d %d\r\n", (int)length, cp, (int)render_info->offset);
//    size_t length = 0;
//    int cp = (*file->encoding->decode)(file->encoding, &render_info->stream, ~0, &length);
//    size_t length = 1;
//    int cp = ' ';

    if (cp!=0xfeff && (cp!=newline_cp1 || newline_cp2==0)) {
      render_info->visual_detail &= ~VISUAL_INFO_CONTROLCHARACTER;
    }

    if (cp==0xfeff) {
      render_info->visual_detail |= VISUAL_INFO_CONTROLCHARACTER;
    }

    if (cp=='}') {
      render_info->depth_new[0]--;
      int min = render_info->depth_old[0]-render_info->depth_new[0];
      if (min>render_info->brackets[0].min) {
        render_info->brackets[0].min = min;
      }
      render_info->brackets[0].diff--;
    }

    // Character bounds / location based stops
    // bibber *brr*
    // values 0 = below; 1 = on line (below); 3 = on line (above); 4 = above
    // bitset 0 = on line; 1 = above (line); 2 = above

    int drawn = 0;
    if (in->type==VISUAL_SEEK_X_Y) {
      if (render_info->y_view==in->y) {
        drawn = (render_info->x>=in->x)?(2|1):1;
      } else if (render_info->y_view>in->y) {
        drawn = 4;
      }
    } else if (in->type==VISUAL_SEEK_LINE_COLUMN) {
      if (render_info->line==in->line) {
        drawn = (render_info->column>=in->column)?(2|1):1;
      } else if (render_info->line>in->line) {
        drawn = 4;
      }
    } else if (in->type==VISUAL_SEEK_OFFSET) {
      drawn = (render_info->offset>=in->offset)?(4|2|1):1;
    } else if (in->type==VISUAL_SEEK_BRACKET_NEXT) {
      drawn = (render_info->offset>=in->offset && render_info->depth_new[in->bracket]==in->bracket_search)?(4|2|1):0;
      rendered = (render_info->offset>=in->offset && (out->type!=VISUAL_SEEK_NONE || (drawn&4)))?1:-1;
    } else if (in->type==VISUAL_SEEK_BRACKET_PREV) {
      drawn = (render_info->depth_new[in->bracket]==in->bracket_search && render_info->offset<in->offset)?1:0;
      if (render_info->offset>=in->offset) {
        rendered = (out->type!=VISUAL_SEEK_NONE || drawn)?1:-1;
      } else {
        rendered = 0;
      }
    }

    if (out && stop!=1) {
      out->y_drawn |= (drawn&1);

      if (!(drawn&1) && out->y_drawn && in->type!=VISUAL_SEEK_BRACKET_PREV && in->type!=VISUAL_SEEK_BRACKET_NEXT) {
        stop = 1;
      }

      if (drawn&1) {
        if (in->type==VISUAL_SEEK_X_Y) {
          out->x_max = render_info->x;
          out->x_min = (render_info->visual_detail&VISUAL_INFO_WRAPPED)?render_info->indentation+render_info->indentation_extra:0;
        } else if (in->type==VISUAL_SEEK_LINE_COLUMN) {
          out->x_max = render_info->column;
          out->x_min = 0;
        }
      }

      if ((drawn&1) && (!(render_info->visual_detail&VISUAL_INFO_CONTROLCHARACTER) || view->showall)) {
        int set = 1;
        if (drawn&2) {
          if (cancel && out->type!=VISUAL_SEEK_NONE) {
            if ((in->type==VISUAL_SEEK_OFFSET && render_info->offset>in->offset) || (in->type==VISUAL_SEEK_X_Y && render_info->x>in->x) || (in->type==VISUAL_SEEK_LINE_COLUMN && render_info->column>in->column)) {
              set = 0;
            }
          }

          stop = 1;
        }

        if (set) {
          out->type = in->type;
          out->x = render_info->x;
          out->y = render_info->y_view;
          out->offset = render_info->offset;
          out->line = render_info->line;
          out->column = render_info->column;
          out->buffer = render_info->buffer;
          out->displacement = render_info->displacement;
          out->character = render_info->character;
          for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
            out->depth[n] = render_info->depth_new[n];
          }
        }
      }
    } else {
      if (drawn>=2 && !in->clip) {
        stop = 1;
      }
    }

    if (boundary && page_count>1 && drawn<2 && !stop) {
      if (in->type==VISUAL_SEEK_BRACKET_NEXT) {
      } else if (in->type==VISUAL_SEEK_BRACKET_PREV) {
      } else {
        rendered = 0;
      }

      stop = 1;
    }

    if (cp==newline_cp2 && newline_cp2!=0) {
      render_info->visual_detail |= VISUAL_INFO_CONTROLCHARACTER;
    }

    if (cp==newline_cp1) {
      render_info->visual_detail &= ~VISUAL_INFO_CONTROLCHARACTER;
    }

    if (!view->continuous && render_info->draw_indentation) {
      if (screen && render_info->y_view==render_info->y) {
        int cp = 0x21aa;
        splitter_drawchar(screen, splitter, render_info->x-file->tabstop_width-view->scroll_x, render_info->y-view->scroll_y, &cp, 1, file->defaults.colors[VISUAL_FLAG_COLOR_STATUS], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
      }
    }

    if ((boundary || !page_dirty) && stop) {
      break;
    }

    render_info->draw_indentation = 0;

    if (render_info->keyword_length==0) {
      render_info->offset_sync = render_info->offset;
    }

    int show;
    int color;
    int background;

    if (screen) {
      show = -1;
      color = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT]; //render_info->buffer->visuals.dirty?2:15; //((int)render_info->buffer&0xff); //
      background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];

      // Whitespace look ahead in current buffer
      // TODO: Hack for testing / speed me up
      if ((cp==' ' || cp=='\t' || cp==newline_cp2) && !render_info->whitespaced) {
        file_offset_t displacement = render_info->displacement;
        size_t pos = 0;
        while (1) {
          if (displacement>=render_info->buffer->length) {
            struct range_tree_node* buffer_new = range_tree_next(render_info->buffer);
            render_info->whitespaced = buffer_new?range_tree_find_whitespaced(buffer_new):1;
            break;
          }

          displacement += encoding_cache_find_length(&render_info->cache, pos);
          int cp = encoding_cache_find_codepoint(&render_info->cache, pos);
          if (cp==newline_cp1 || cp==0) {
            if (pos>0) {
              render_info->whitespaced = 1;
              break;
            }
          }

          if (cp!=' ' && cp!='\t' && cp!=newline_cp2) {
            break;
          }

          pos++;
        }
      }

      if (render_info->whitespaced && cp!=newline_cp1) {
        background = 1;
      }
    }

    if (!(render_info->buffer->inserter&TIPPSE_INSERTER_READONLY)) {
      if (render_info->keyword_length==0) {
        int visual_flag = 0;

        (*file->type->mark)(file->type, &render_info->visual_detail, &render_info->cache, (render_info->y_view==render_info->y)?1:0, &render_info->keyword_length, &visual_flag);

        render_info->keyword_color = file->defaults.colors[visual_flag];
      }

      if (render_info->keyword_length>0) {
        render_info->keyword_length--;
        color = render_info->keyword_color;
      }
    }

    int fill = 0;
    if (((((cp!=newline_cp2 || newline_cp2==0) && cp!=0xfeff) || view->showall) && cp!='\t') || view->continuous) {
      fill = 1;
    } else if (cp=='\t') {
      fill = file->tabstop_width-(render_info->x%file->tabstop_width);
    }

    if (screen && render_info->y_view==render_info->y) {
      if (cp<0x20 && cp!=newline_cp1) {
        show = 0xfffd;
      }

      if (cp=='\n') {
        show = view->showall?0x21a7:' ';
      } else if (cp=='\r') {
        show = view->showall?0x21a4:' ';
      } else if (cp=='\t') {
        show = view->showall?0x21a6:' ';
      } else if (cp==0x7f) {
        show = view->showall?0x21a2:' ';
      } else if (cp==0xfeff) {
        show = view->showall?0x66d:' ';
      } else if (cp<0x20) {
        show = cp+0x2400;
      }

      int sel = (render_info->offset>=view->selection_low && render_info->offset<view->selection_high)?1:0;
      if (sel) {
        int swap = color;
        color = background;
        background = swap;
      }

      if (read==1 && show==-1) {
        show = cp;
      }

      int x = render_info->x-view->scroll_x;
      int y = render_info->y-view->scroll_y;
      if (show!=-1) {
        splitter_drawchar(screen, splitter, x++, y, &show, 1, color, background);
      } else {
        splitter_drawchar(screen, splitter, x++, y, &codepoints[0], read, color, background);
      }

      show = ' ';
      int pos;
      for (pos = 1; pos<fill; pos++) {
        splitter_drawchar(screen, splitter, x++, y, &show, 1, color, background);
      }
    }

    render_info->x += fill;
    if (render_info->visual_detail&VISUAL_INFO_INDENTATION && !view->continuous && render_info->indentation<render_info->width/2) {
      render_info->indentation += fill;
      render_info->indentations += fill;
    } else {
      render_info->xs += fill;
    }

    render_info->column++;
    render_info->columns++;
    render_info->character++;
    render_info->characters++;
    encoding_cache_advance(&render_info->cache, advance);

    render_info->displacement += length;
    render_info->offset += length;

    int word_length = 0;
    if (cp<=' ' && view->wrapping) {
      int row_width = render_info->width-render_info->indentation-file->tabstop_width;
      word_length = document_text_render_lookahead_word_wrap(file, &render_info->cache, row_width+1);
      if (word_length>row_width) {
        word_length = 0;
      }
    }

    if ((!view->continuous && cp==newline_cp1) || (render_info->x+word_length>=render_info->width && view->wrapping) || (render_info->x+1>=render_info->width && view->continuous)) {
      if (cp==newline_cp1) {
        render_info->indentations = 0;
        render_info->indentations_extra = 0;
        render_info->indentation = 0;
        render_info->indentation_extra = 0;
        render_info->draw_indentation = 0;
      } else {
        render_info->draw_indentation = 1;

        if (render_info->indentation_extra==0 && !view->continuous) {
          render_info->indentations_extra = file->tabstop_width;
          render_info->indentation_extra = file->tabstop_width;
        }

        render_info->visual_detail |= VISUAL_INFO_WRAPPED;
      }

      render_info->ys++;
      render_info->y_view++;
      render_info->x = render_info->indentation+render_info->indentation_extra;
      render_info->xs = 0;
    }

    if (cp==newline_cp1) {
      if (render_info->visual_detail&VISUAL_INFO_WHITESPACED_COMPLETE) {
        render_info->visual_detail |= VISUAL_INFO_WHITESPACED_START;
      }

      render_info->visual_detail &= ~VISUAL_INFO_WRAPPED;

      render_info->whitespaced = 0;

      render_info->line++;
      render_info->lines++;
      render_info->column = 0;
      render_info->columns = 0;
    }

    if (cp!='\t' && cp!=' ' && cp!=newline_cp2) {
      render_info->visual_detail &= ~VISUAL_INFO_WHITESPACED_COMPLETE;
    }

    if (cp=='{') {
      render_info->depth_new[0]++;
      int max = render_info->depth_new[0]-render_info->depth_old[0];
      if (max>render_info->brackets[0].max) {
        render_info->brackets[0].max = max;
      }
      render_info->brackets[0].diff++;
    }

    if (in->clip && render_info->buffer) {
      // TODO: render_info->buffer->visuals.ys==render_info->ys is hacked since some lines didn't wrap during rendering (which is wrong anyway, retest if span render buffer is available)
      if ((render_info->y_view>render_info->y && render_info->buffer->visuals.ys==render_info->ys) || (render_info->y_view==render_info->y && render_info->x-view->scroll_x>render_info->width)) {
        stop = 1;
      }
    }
  }

  return rendered;
}

// Find next dirty pages and rerender them (background task)
int document_text_incremental_update(struct document* base, struct splitter* splitter) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  struct document_text_position in;
  in.type = VISUAL_SEEK_OFFSET;
  in.offset = file->buffer?file->buffer->length:0;
  in.clip = 0;

  struct document_text_render_info render_info;
  document_text_render_clear(&render_info, splitter->client_width);
  document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
  document_text_render_span(&render_info, NULL, NULL, view, file, &in, NULL, 16, 1);

  return file->buffer?file->buffer->visuals.dirty:0;
}

// Draw entire visible screen space
void document_text_draw(struct document* base, struct screen* screen, struct splitter* splitter) {
  struct document_text* document = (struct document_text*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (splitter->content && file->buffer) {
    if (file->buffer->length!=view->selection->length) {
      printf("\r\nSelection area and file length differ %d<>%d\r\n", (int)file->buffer->length, (int)view->selection->length);
      exit(0);
    }
  }

  struct document_text_position cursor;
  struct document_text_position in;

  int prerender = 0;
  in.type = VISUAL_SEEK_OFFSET;
  in.clip = 0;
  in.offset = view->offset;

  document_text_cursor_position(splitter, &in, &cursor, 0, 1);

  struct document_text_render_info render_info;
  while (1) {
    int scroll_x = view->scroll_x;
    int scroll_y = view->scroll_y;
    if (cursor.x<view->scroll_x) {
      view->scroll_x = cursor.x;
    }
    if (cursor.x>=view->scroll_x+splitter->client_width-1) {
      view->scroll_x = cursor.x-(splitter->client_width-1);
    }
    if (cursor.y<view->scroll_y) {
      view->scroll_y = cursor.y;
    }
    if (cursor.y>=view->scroll_y+splitter->client_height-1) {
      view->scroll_y = cursor.y-(splitter->client_height-1);
    }
    if (view->scroll_y+splitter->client_height>(file->buffer?file->buffer->visuals.ys+1:0) && (file->buffer?file->buffer->visuals.dirty:0)==0) {
      view->scroll_y = (file->buffer?file->buffer->visuals.ys+1:0)-(splitter->client_height);
    }
    if (view->scroll_y<0) {
      view->scroll_y = 0;
    }
    if (view->scroll_x<0) {
      view->scroll_x = 0;
    }

    if (scroll_x==view->scroll_x && scroll_y==view->scroll_y && prerender>0) {
      break;
    }

    prerender++;

    document_text_render_clear(&render_info, splitter->client_width);
    in.type = VISUAL_SEEK_X_Y;
    in.clip = 0;
    in.x = view->scroll_x+splitter->client_width;
    in.y = view->scroll_y+splitter->client_height-1;

    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      if (document_text_render_span(&render_info, NULL, splitter, view, file, &in, NULL, ~0, 1)) {
        break;
      }
    }
  }

  if (view->scroll_x_old!=view->scroll_x || view->scroll_y_old!=view->scroll_y) {
    view->scroll_x_old = view->scroll_x;
    view->scroll_y_old = view->scroll_y;
    document->show_scrollbar = 1;
  }

  document_text_render_clear(&render_info, splitter->client_width);
  in.type = VISUAL_SEEK_X_Y;
  in.clip = 1;
  in.x = view->scroll_x;
  int y;
  for (y=0; y<splitter->client_height; y++) {
    in.y = y+view->scroll_y;

    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      if (document_text_render_span(&render_info, screen, splitter, view, file, &in, NULL, ~0, 1)) {
        break;
      }
    }

    // Bookmark test
    int marked = 0;
    if (splitter->content) {
      struct document_text_position out;
      struct document_text_position in_x_y;
      in_x_y.type = VISUAL_SEEK_X_Y;
      in_x_y.clip = 0;

      struct document_text_position in_line_column;
      in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
      in_line_column.clip = 0;

      in_x_y.x = in.x;
      in_x_y.y = in.y;
      document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);

      in_line_column.line = out.line;
      in_line_column.column = 0;
      document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
      file_offset_t start = out.offset;
      in_line_column.column = 100000000;
      document_text_cursor_position(splitter, &in_line_column, &out, 1, 1);
      file_offset_t end = out.offset;

      marked = range_tree_marked(file->bookmarks, start, end-start, TIPPSE_INSERTER_MARK);
    }

    if (marked) {
      splitter_cursor(screen, splitter, in.x-view->scroll_x, in.y-view->scroll_y);
    }
  }

  if (!screen) {
    return;
  }

  size_t name_length = strlen(file->filename);
  char* title = malloc((name_length+file->modified*2+1)*sizeof(char));
  memcpy(title, file->filename, name_length);
  if (file->modified) {
    memcpy(title+name_length, " *\0", 3);
  } else {
    title[name_length] = '\0';
  }
  splitter_name(splitter, title);
  free(title);

  splitter_cursor(screen, splitter, cursor.x-view->scroll_x, cursor.y-view->scroll_y);

  // Test
  int test_bl = 0;
  int test_br = 0;
  int test_c = 0;
  if (cursor.depth[0]>0) {
    struct document_text_position out;
    in.type = VISUAL_SEEK_BRACKET_NEXT;
    in.clip = 0;
    in.offset = view->offset;
    in.bracket = 0;
    in.bracket_search = cursor.depth[0]-1;

    document_text_render_clear(&render_info, splitter->client_width);
    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      int rendered = document_text_render_span(&render_info, NULL, splitter, view, file, &in, &out, ~0, 1);
      if (rendered==1) {
        break;
      }

      if (rendered==-1) {
        test_c++;
        document_text_render_clear(&render_info, splitter->client_width);
        document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
        struct range_tree_node* node = range_tree_find_bracket_forward(render_info.buffer, 0, in.bracket_search);
        if (!node) {
          node = range_tree_last(file->buffer);
          if (render_info.buffer==node) {
            break;
          }
        }

        in.offset = range_tree_offset(node);
      }
    }

    test_br = (int)out.offset;
    splitter_hilight(screen, splitter, out.x-view->scroll_x, out.y-view->scroll_y);
  }

  if (cursor.depth[0]>0) {
    struct document_text_position out;
    in.type = VISUAL_SEEK_BRACKET_PREV;
    in.clip = 0;
    in.offset = view->offset;
    in.bracket = 0;
    in.bracket_search = cursor.depth[0]-1;

    document_text_render_clear(&render_info, splitter->client_width);
    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      int rendered = document_text_render_span(&render_info, NULL, splitter, view, file, &in, &out, ~0, 0);
      if (rendered==1) {
        break;
      }

      if (rendered==-1) {
        test_c++;
        document_text_render_clear(&render_info, splitter->client_width);
        document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
        struct range_tree_node* node = range_tree_find_bracket_backward(render_info.buffer, 0, in.bracket_search);
        if (!node) {
          node = range_tree_first(file->buffer);
        }

        if (render_info.buffer==range_tree_first(file->buffer)) {
          break;
        }

        in.offset = range_tree_offset(node)+node->length-1;
      }
    }

    test_bl = (int)out.offset;
    splitter_hilight(screen, splitter, out.x-view->scroll_x, out.y-view->scroll_y);
  }

  if (!document->keep_status) {
    const char* newline[TIPPSE_NEWLINE_MAX] = {"Auto", "Lf", "Cr", "CrLf"};
    const char* tabstop[TIPPSE_TABSTOP_MAX] = {"Auto", "Tab", "Space"};
    char status[1024];
    sprintf(&status[0], "%s%s%d/%d:%d - %d/%d byte - %s*%d %s %s %s - %d(%d-%d.%d)", (file->buffer?file->buffer->visuals.dirty:0)?"? ":"", (file->buffer?(file->buffer->inserter&TIPPSE_INSERTER_FILE):0)?"File ":"", (int)(file->buffer?file->buffer->visuals.lines+1:0), cursor.line+1, cursor.column+1, (int)view->offset, file->buffer?(int)file->buffer->length:0, tabstop[file->tabstop], file->tabstop_width, newline[file->newline], (*file->type->name)(), (*file->encoding->name)(), cursor.depth[0], test_bl, test_br, test_c);
    splitter_status(splitter, &status[0], 0);
  }

  document->keep_status = 0;

  if (document->show_scrollbar) {
    file_offset_t start = view->scroll_y;
    file_offset_t end = view->scroll_y+splitter->client_height;

    file_offset_t length = file->buffer?file->buffer->visuals.ys:0;
    if (end==~0) {
      end = length+1;
    }

    int pos_start = (start*(file_offset_t)splitter->client_height)/(length+1);
    int pos_end = (end*(file_offset_t)splitter->client_height)/(length+1);
    int cp = 0x20;
    for (y = 0; y<splitter->client_height; y++) {
      if (y>=pos_start && y<=pos_end) {
        splitter_drawchar(screen, splitter, splitter->client_width-1, y, &cp, 1, 17, 231);
      } else {
        splitter_drawchar(screen, splitter, splitter->client_width-1, y, &cp, 1, 17, 102);
      }
    }
  }

  document->show_scrollbar = 0;
}

void document_text_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document_text* document = (struct document_text*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  struct document_text_position out;
  struct document_text_position in_offset;
  in_offset.type = VISUAL_SEEK_OFFSET;
  in_offset.clip = 0;

  struct document_text_position in_x_y;
  in_x_y.type = VISUAL_SEEK_X_Y;
  in_x_y.clip = 0;

  struct document_text_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
  in_line_column.clip = 0;

  //range_tree_check(document->buffer);
  int seek = 0;

  int reset_selection = 1;
  if (modifier&TIPPSE_KEY_MOD_SHIFT) {
    if (view->selection_start==~0) {
      view->selection_start = view->offset;
    }

    reset_selection = 0;
  }

  if (cp!=TIPPSE_KEY_UP && cp!=TIPPSE_KEY_DOWN && cp!=TIPPSE_KEY_PAGEDOWN && cp!=TIPPSE_KEY_PAGEUP) {
    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  }

  in_x_y.x = view->cursor_x;
  in_x_y.y = view->cursor_y;

  if (cp==TIPPSE_KEY_UP) {
    in_x_y.y--;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_DOWN) {
    in_x_y.y++;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_LEFT) {
    in_x_y.x--;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (cp==TIPPSE_KEY_RIGHT) {
    in_x_y.x++;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 0);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    int page = ((splitter->height-2)/2)+1;
    in_x_y.y += page;
    view->scroll_y += page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    int page = ((splitter->height-2)/2)+1;
    in_x_y.y -= page;
    view->scroll_y -= page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    if (!document_file_delete_selection(splitter->file, splitter->view)) {
      in_x_y.x--;
      file_offset_t start = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      document_file_delete(splitter->file, start, view->offset-start);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_DELETE) {
    if (!document_file_delete_selection(splitter->file, splitter->view)) {
      in_x_y.x++;
      file_offset_t end = document_text_cursor_position(splitter, &in_x_y, &out, 1, 0);
      document_file_delete(splitter->file, view->offset, end-view->offset);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_FIRST) {
    in_x_y.x = 0;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_LAST) {
    in_x_y.x = 1000000000;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_HOME) {
    in_x_y.x = 0;
    in_x_y.y = 0;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_END) {
    in_x_y.x = 1000000000;
    in_x_y.y = 1000000000;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    document->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_UNDO) {
    document_undo_execute(file, view, file->undos, file->redos);
    while (document_undo_execute(file, view, file->undos, file->redos)) {}
    return;
  } else if (cp==TIPPSE_KEY_REDO) {
    document_undo_execute(file, view, file->redos, file->undos);
    while (document_undo_execute(file, view, file->redos, file->undos)) {}
    return;
  } else if (cp==TIPPSE_KEY_BOOKMARK) {
    document_text_toggle_bookmark(base, splitter, view->offset);
    return;
  } else if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      in_x_y.x = x-splitter->x+view->scroll_x;
      in_x_y.y = y-splitter->y+view->scroll_y;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
      if (view->selection_start==~0) {
        view->selection_start = view->offset;
      }
      view->selection_end = view->offset;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y += page;
      view->scroll_y += page;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
      document->show_scrollbar = 1;
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y -= page;
      view->scroll_y -= page;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
      document->show_scrollbar = 1;
    }

    reset_selection = (!(button_old&TIPPSE_MOUSE_LBUTTON) && (button&TIPPSE_MOUSE_LBUTTON))?1:0;
  } else if (cp==TIPPSE_KEY_TAB) {
    document_undo_chain(file);
    if (view->selection_low==~0) {
      uint8_t utf8[8];
      file_offset_t size;
      if (file->tabstop==TIPPSE_TABSTOP_SPACE) {
        size = file->tabstop_width-(view->cursor_x%file->tabstop_width);
        file_offset_t spaces;
        for (spaces = 0; spaces<size; spaces++) {
          utf8[spaces] = ' ';
        }
      } else {
        size = 1;
        utf8[0] = '\t';
      }

      document_file_insert(splitter->file, view->offset, &utf8[0], size);
    } else {
      struct document_text_position out_start;
      struct document_text_position out_end;

      in_offset.offset = view->selection_low;
      document_text_cursor_position(splitter, &in_offset, &out_start, 0, 1);
      in_offset.offset = view->selection_high-1;
      document_text_cursor_position(splitter, &in_offset, &out_end, 0, 1);

      reset_selection = 0;
      while(out_end.line>=out_start.line) {
        in_line_column.column = 0;
        in_line_column.line = out_end.line;
        document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);

        struct range_tree_node* buffer = out.buffer;
        file_offset_t displacement = out.displacement;
        file_offset_t offset = out.offset;

        uint8_t utf8[8];
        file_offset_t size;
        if (file->tabstop==TIPPSE_TABSTOP_SPACE) {
          size = file->tabstop_width;
          file_offset_t spaces;
          for (spaces = 0; spaces<size; spaces++) {
            utf8[spaces] = ' ';
          }
        } else {
          size = 1;
          utf8[0] = '\t';
        }

        const uint8_t* text = buffer->buffer->buffer+buffer->offset+displacement;
        if (*text!='\r' && *text!='\n') {
          document_file_insert(splitter->file, offset, &utf8[0], size);
        }

        out_end.line--;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_UNTAB) {
    document_undo_chain(file);
    if (view->selection_low!=~0) {
      struct document_text_position out_start;
      struct document_text_position out_end;

      in_offset.offset = view->selection_low;
      document_text_cursor_position(splitter, &in_offset, &out_start, 0, 1);
      in_offset.offset = view->selection_high-1;
      document_text_cursor_position(splitter, &in_offset, &out_end, 0, 1);

      reset_selection = 0;
      while(out_end.line>=out_start.line) {
        in_line_column.column = 0;
        in_line_column.line = out_end.line;
        document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);

        struct range_tree_node* buffer = out.buffer;
        file_offset_t displacement = out.displacement;
        file_offset_t offset = out.offset;

        file_offset_t length = 0;
        while (buffer) {
          if (displacement>=buffer->length || !buffer->buffer) {
            buffer = range_tree_next(buffer);
            displacement = 0;
            continue;
          }

          const uint8_t* text = buffer->buffer->buffer+buffer->offset+displacement;
          if (*text!='\t' && *text!=' ') {
            break;
          }

          length++;
          if (*text=='\t' || length>=file->tabstop_width) {
            break;
          }

          displacement++;
        }

        if (length>0) {
          document_file_delete(splitter->file, offset, length);
        }

        out_end.line--;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_COPY || cp==TIPPSE_KEY_CUT) {
    document_undo_chain(file);
    if (view->selection_low!=~0) {
      clipboard_set(range_tree_copy(file->buffer, view->selection_low, view->selection_high-view->selection_low));
      if (cp==TIPPSE_KEY_CUT) {
        document_file_delete_selection(splitter->file, splitter->view);
        reset_selection = 1;
      } else {
        reset_selection = 0;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_PASTE || cp==TIPPSE_KEY_BRACKET_PASTE_START) {
    document_undo_chain(file);
    document_file_delete_selection(splitter->file, splitter->view);
    if (clipboard_get()) {
      document_file_insert_buffer(splitter->file, view->offset, clipboard_get());
    }

    document_undo_chain(file);
    reset_selection = 1;
    seek = 1;
  } else if (cp>=0) {
    document_file_delete_selection(splitter->file, splitter->view);
    uint8_t utf8[8];
    size_t size;
    if (cp=='\n') {
      if (file->newline==TIPPSE_NEWLINE_CRLF) {
        utf8[0] = '\r';
        utf8[1] = '\n';
        size = 2;
      } else if (file->newline==TIPPSE_NEWLINE_CR) {
        utf8[0] = '\r';
        size = 1;
      } else {
        utf8[0] = '\n';
        size = 1;
      }
    } else {
      size = encoding_utf8_encode(NULL, cp, &utf8[0], 8);
    }

    document_file_insert(splitter->file, view->offset, &utf8[0], size);
    seek = 1;
  }

  if (seek) {
    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
    view->offset = out.offset;
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  }

  if (reset_selection) {
    view->selection_start = ~0;
    view->selection_end = ~0;
  }

  if (modifier&TIPPSE_KEY_MOD_SHIFT) {
    view->selection_end = view->offset;
  }

  if (view->selection_start<view->selection_end) {
    view->selection_low = view->selection_start;
    view->selection_high = view->selection_end;
  } else {
    view->selection_low = view->selection_end;
    view->selection_high = view->selection_start;
  }

  if (view->line_select) {
    in_offset.offset = view->offset;
    in_offset.clip = 0;

    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 0, 1);

    in_line_column.line = out.line;
    in_line_column.column = 0;
    document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->selection_low = view->selection_start = out.offset;
    in_line_column.column = 100000000;
    document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->selection_high = view->selection_end = out.offset;
  }
}

void document_text_toggle_bookmark(struct document* base, struct splitter* splitter, file_offset_t offset) {
  struct document_text* document = (struct document_text*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  struct document_text_position out;
  struct document_text_position in_offset;
  in_offset.type = VISUAL_SEEK_OFFSET;
  in_offset.clip = 0;

  struct document_text_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
  in_line_column.clip = 0;

  in_offset.offset = view->offset;
  document_text_cursor_position(splitter, &in_offset, &out, 0, 1);

  in_line_column.line = out.line;
  in_line_column.column = 0;
  document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
  file_offset_t start = out.offset;
  in_line_column.column = 100000000;
  document_text_cursor_position(splitter, &in_line_column, &out, 1, 1);
  file_offset_t end = out.offset;

  int marked = range_tree_marked(file->bookmarks, start, end-start, TIPPSE_INSERTER_MARK);
  file->bookmarks = range_tree_mark(file->bookmarks, start, end-start, marked?0:TIPPSE_INSERTER_MARK);
  // range_tree_print(file->bookmarks, 0, 0);
}
