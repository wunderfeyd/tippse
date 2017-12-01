// Tippse - Document Text View - Cursor and display operations for 2d text mode

#include "document_text.h"

// Temporary debug visualisation
#define DEBUG_NONE 0
#define DEBUG_RERENDERDISPLAY 1
#define DEBUG_ALWAYSRERENDER 2
#define DEBUG_PAGERENDERDISPLAY 4

int debug = DEBUG_NONE;
int debug_draw = 0;
struct screen* debug_screen = NULL;
struct splitter* debug_splitter = NULL;

// Create document
struct document* document_text_create(void) {
  struct document_text* document = (struct document_text*)malloc(sizeof(struct document_text));
  document->keep_status = 0;
  document->vtbl.reset = document_text_reset;
  document->vtbl.draw = document_text_draw;
  document->vtbl.keypress = document_text_keypress;
  document->vtbl.incremental_update = document_text_incremental_update;

  return (struct document*)document;
}

// Destroy document
void document_text_destroy(struct document* base) {
  free(base);
}

// Called after a new document was assigned (correct to next character)
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
file_offset_t document_text_cursor_position_partial(struct document_text_render_info* render_info, struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  while (1) {
    while (1) {
      document_text_render_seek(render_info, file->buffer, file->encoding, in);
      if (document_text_render_span(render_info, NULL, NULL, view, file, in, out, ~0, cancel)) {
        break;
      }
    }

    if (in->type==VISUAL_SEEK_OFFSET) {
      break;
    }

    position_t* x;
    position_t* y;
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

        *x = POSITION_T_MAX;
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
          *x = POSITION_T_MAX;
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

// Goto specified position
file_offset_t document_text_cursor_position(struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel) {
  struct document_text_render_info render_info;
  document_text_render_clear(&render_info, splitter->client_width-splitter->view->address_width);
  return document_text_cursor_position_partial(&render_info, splitter, in, out, wrap, cancel);
}

// Clear renderer state to ensure a restart at next seek
void document_text_render_clear(struct document_text_render_info* render_info, position_t width) {
  memset(render_info, 0, sizeof(struct document_text_render_info));
  render_info->width = width;
}

// Update renderer state to restart at the given position or to continue if possible
void document_text_render_seek(struct document_text_render_info* render_info, struct range_tree_node* buffer, struct encoding* encoding, struct document_text_position* in) {
  file_offset_t offset_new = 0;
  position_t x_new = 0;
  position_t y_new = 0;
  position_t lines_new = 0;
  position_t columns_new = 0;
  int indentation_new = 0;
  int indentation_extra_new = 0;
  file_offset_t characters_new = 0;
  struct range_tree_node* buffer_new;

  int type = in->type;
  if (type==VISUAL_SEEK_BRACKET_NEXT || type==VISUAL_SEEK_BRACKET_PREV || type==VISUAL_SEEK_INDENTATION_LAST) {
    type = VISUAL_SEEK_OFFSET;
  }

  buffer_new = range_tree_find_visual(buffer, type, in->offset, in->x, in->y, in->line, in->column, &offset_new, &x_new, &y_new, &lines_new, &columns_new, &indentation_new, &indentation_extra_new, &characters_new, 0, in->offset);

  // TODO: Combine into single statement (if correctness is confirmed)
  int rerender = (debug&DEBUG_ALWAYSRERENDER)?1:0;

  if (render_info->append==0) {
    rerender = 1;
  }

  if (type==VISUAL_SEEK_OFFSET && render_info->offset>in->offset) {
    rerender = 1;
  }

  if (type==VISUAL_SEEK_X_Y && (render_info->y_view>in->y || (render_info->y_view==in->y && render_info->x>in->x && render_info->xs>0))) {
    rerender = 1;
  }

  if (type==VISUAL_SEEK_LINE_COLUMN && (render_info->line>in->line || (render_info->line==in->line && render_info->column>in->column))) {
    rerender = 1;
  }

  int dirty = 0;
  if (buffer_new) {
    if (range_tree_next(buffer_new)) {
      dirty = range_tree_next(buffer_new)->visuals.dirty;
    }
  }

  if (dirty && (!buffer_new || render_info->buffer!=range_tree_next(buffer_new)) && render_info->buffer!=buffer_new) {
    rerender = 1;
  } else if (!dirty && render_info->buffer!=buffer_new) {
    rerender = 1;
  }

  if (rerender) {
    debug_draw++;
    render_info->append = 1;
    if (buffer_new) {
      render_info->visual_detail = buffer_new->visuals.detail_before;
      render_info->offset_sync = offset_new-buffer_new->visuals.rewind;
      render_info->displacement = buffer_new->visuals.displacement;
      render_info->offset = offset_new+buffer_new->visuals.displacement;
      if (offset_new==0) {
        render_info->visual_detail = VISUAL_DETAIL_NEWLINE;
      }

      render_info->indented = range_tree_find_indentation(buffer_new);
      render_info->whitespaced = range_tree_find_whitespaced(buffer_new);

      render_info->visual_detail |= VISUAL_DETAIL_WHITESPACED_COMPLETE;
      render_info->visual_detail &= ~(VISUAL_DETAIL_WHITESPACED_START|VISUAL_DETAIL_STOPPED_INDENTATION);

      render_info->indentation_extra = indentation_extra_new;
      render_info->indentation = indentation_new;
      render_info->x = x_new+render_info->indentation+render_info->indentation_extra;
      render_info->y_view = y_new;
      render_info->line = lines_new;
      render_info->column = columns_new;
      render_info->character = characters_new;

      if (render_info->indentation+render_info->indentation_extra>0 && x_new==0 && !render_info->indented) {
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
        render_info->depth_line[n] = render_info->depth_new[n];
        render_info->brackets[n].diff = 0;
        render_info->brackets[n].min = 0;
        render_info->brackets[n].max = 0;
        render_info->brackets_line[n].diff = 0;
        render_info->brackets_line[n].min = 0;
        render_info->brackets_line[n].max = 0;
      }

      encoding_stream_from_page(&render_info->stream, render_info->buffer, render_info->displacement);
    } else {
      encoding_stream_from_page(&render_info->stream, NULL, 0);
    }

    encoding_cache_clear(&render_info->cache, encoding, &render_info->stream);
  }

  if (in->type==VISUAL_SEEK_X_Y) {
    render_info->y = in->y;
  } else {
    render_info->y = render_info->y_view;
  }
}

// Get word length from specified position
position_t document_text_render_lookahead_word_wrap(struct document_file* file, struct encoding_cache* cache, position_t max) {
  position_t count = 0;
  size_t advanced = 0;
  while (count<max) {
    codepoint_t codepoints[8];
    size_t advance = 1;
    size_t length = 1;
    size_t read = unicode_read_combined_sequence(cache, advanced, &codepoints[0], 8, &advance, &length);

    if (codepoints[0]<=' ') {
      break;
    }

    count += unicode_width(&codepoints[0], read);
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
    out->bracket_match = 0;
    for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
      out->depth[n] = 0;
      out->min_line[n] = 0;
    }
  }

  int rendered = 1;
  int stop = render_info->buffer?0:1;
  int page_count = 0;
  int page_dirty = (render_info->buffer && render_info->buffer->visuals.dirty)?1:0;

  render_info->visual_detail |= (view->wrapping)?VISUAL_DETAIL_WRAPPING:0;
  render_info->visual_detail |= (view->show_invisibles)?VISUAL_DETAIL_SHOW_INVISIBLES:0;
  render_info->visual_detail |= (view->continuous)?VISUAL_DETAIL_CONTINUOUS:0;

  codepoint_t newline_cp1 = (file->newline==TIPPSE_NEWLINE_CR)?'\r':'\n';
  codepoint_t newline_cp2 = (file->newline==TIPPSE_NEWLINE_CRLF)?'\r':0;

  while (1) {
    int boundary = 0;
    while (render_info->buffer && render_info->displacement>=render_info->buffer->length) {
      boundary = 1;
      int dirty = (render_info->buffer->visuals.xs!=render_info->xs || render_info->buffer->visuals.ys!=render_info->ys || render_info->buffer->visuals.lines!=render_info->lines ||  render_info->buffer->visuals.columns!=render_info->columns || render_info->buffer->visuals.indentation!=render_info->indentations || render_info->buffer->visuals.indentation_extra!=render_info->indentations_extra || render_info->buffer->visuals.detail_after!=render_info->visual_detail ||  (render_info->ys==0 && render_info->buffer->visuals.dirty && (view->wrapping || view->continuous)))?1:0;
      for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
        if (render_info->buffer->visuals.brackets[n].diff!=render_info->brackets[n].diff || render_info->buffer->visuals.brackets[n].min!=render_info->brackets[n].min || render_info->buffer->visuals.brackets[n].max!=render_info->brackets[n].max || render_info->buffer->visuals.brackets_line[n].diff!=render_info->brackets_line[n].diff || render_info->buffer->visuals.brackets_line[n].min!=render_info->brackets_line[n].min || render_info->buffer->visuals.brackets_line[n].max!=render_info->brackets_line[n].max) {
          dirty = 1;
          break;
        }
      }

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
          render_info->buffer->visuals.brackets_line[n] = render_info->brackets_line[n];
        }

        range_tree_update_calc_all(render_info->buffer);
      }

      render_info->displacement -= render_info->buffer->length;
      file_offset_t rewind = render_info->offset-render_info->offset_sync-render_info->displacement;
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
      render_info->visual_detail |= VISUAL_DETAIL_WHITESPACED_COMPLETE;
      render_info->visual_detail &= ~(VISUAL_DETAIL_WHITESPACED_START|VISUAL_DETAIL_STOPPED_INDENTATION);
      for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
        render_info->brackets[n].diff = 0;
        render_info->brackets[n].min = 0;
        render_info->brackets[n].max = 0;
        render_info->brackets_line[n].diff = 0;
        render_info->brackets_line[n].min = 0;
        render_info->brackets_line[n].max = 0;
        render_info->depth_old[n] = render_info->depth_new[n];
        render_info->depth_line[n] = render_info->depth_new[n];
      }

      page_dirty = (render_info->buffer && render_info->buffer->visuals.dirty)?1:0;
    }

    codepoint_t codepoints[8];
    size_t advance = 1;
    size_t length = 1;
    size_t read = unicode_read_combined_sequence(&render_info->cache, 0, &codepoints[0], 8, &advance, &length);

    codepoint_t cp = codepoints[0];

//    size_t length = encoding_cache_find_length(&render_info->cache, 0);
//    codepoint_t cp = encoding_cache_find_codepoint(&render_info->cache, 0);

//    printf("%d %d %d\r\n", (int)length, cp, (int)render_info->offset);
//    size_t length = 0;
//    codepoint_t cp = (*file->encoding->decode)(file->encoding, &render_info->stream, ~0, &length);
//    size_t length = 1;
//    codepoint_t cp = ' ';

    if (cp!=0xfeff && (cp!=newline_cp1 || newline_cp2==0)) {
      render_info->visual_detail &= ~VISUAL_DETAIL_CONTROLCHARACTER;
    }

    if (cp==0xfeff) {
      render_info->visual_detail |= VISUAL_DETAIL_CONTROLCHARACTER;
    }

    codepoint_t show = -1;
    int color = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT];
    int background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];

    if (render_info->buffer) {
      if (screen) {
        if ((debug&DEBUG_PAGERENDERDISPLAY)) {
          color = (intptr_t)render_info->buffer&0xff;
        }

        if ((debug&DEBUG_RERENDERDISPLAY)) {
          background = (debug_draw%22)*6+21;
        }

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
            codepoint_t cp = encoding_cache_find_codepoint(&render_info->cache, pos);
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

        if (render_info->whitespaced && cp!=newline_cp1 && cp!=newline_cp2) {
          background = 1;
        }
      }

      if (!(render_info->buffer->inserter&TIPPSE_INSERTER_READONLY)) {
        if (render_info->keyword_length==0) {
          int visual_flag = 0;

          (*file->type->mark)(file->type, &render_info->visual_detail, &render_info->cache, (render_info->y_view==render_info->y)?1:0, &render_info->keyword_length, &visual_flag);

          render_info->keyword_color = file->defaults.colors[visual_flag];
        }
      } else {
        color = file->defaults.colors[VISUAL_FLAG_COLOR_READONLY];
      }
    }

    int bracket_match = (*file->type->bracket_match)(render_info->visual_detail, &codepoints[0], read);

    // Character bounds / location based stops
    // bibber *brr*
    // values 0 = below; 1 = on line (below); 3 = on line (above); 4 = above
    // bitset 0 = on line; 1 = above (line); 2 = above

    int drawn = 0;
    if (!(render_info->visual_detail&VISUAL_DETAIL_CONTROLCHARACTER) || view->show_invisibles) {
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
        int bracket_correction = ((bracket_match&VISUAL_BRACKET_CLOSE) && ((bracket_match&VISUAL_BRACKET_MASK)==in->bracket))?1:0;
        drawn = (render_info->offset>=in->offset && render_info->depth_new[in->bracket]-bracket_correction==in->bracket_search)?(4|2|1):0;
        rendered = (render_info->offset>=in->offset && (out->type!=VISUAL_SEEK_NONE || (drawn&4)))?1:-1;
      } else if (in->type==VISUAL_SEEK_BRACKET_PREV) {
        int bracket_correction = ((bracket_match&VISUAL_BRACKET_CLOSE) && ((bracket_match&VISUAL_BRACKET_MASK)==in->bracket))?1:0;
        drawn = (render_info->depth_new[in->bracket]-bracket_correction==in->bracket_search && render_info->offset<=in->offset)?1:0;
        if (render_info->offset>=in->offset) {
          rendered = (out->type!=VISUAL_SEEK_NONE || drawn)?1:-1;
        } else {
          rendered = 0;
        }
      } else if (in->type==VISUAL_SEEK_INDENTATION_LAST) {
        if (render_info->line==in->line) {
          drawn = ((render_info->visual_detail&VISUAL_DETAIL_NEWLINE) || (render_info->indented))?1:0;
        } else if (render_info->line>in->line) {
          drawn = 4;
        }
      }

      if (out && stop!=1) {
        out->y_drawn |= (drawn&1);

        if (!(drawn&1) && out->y_drawn && in->type!=VISUAL_SEEK_BRACKET_PREV && in->type!=VISUAL_SEEK_BRACKET_NEXT && in->type!=VISUAL_SEEK_INDENTATION_LAST) {
          stop = 1;
        }

        if (drawn&1) {
          if (in->type==VISUAL_SEEK_X_Y) {
            out->x_max = render_info->x;
            out->x_min = (render_info->visual_detail&VISUAL_DETAIL_WRAPPED)?render_info->indentation+render_info->indentation_extra:0;
          } else if (in->type==VISUAL_SEEK_LINE_COLUMN) {
            out->x_max = render_info->column;
            out->x_min = 0;
          }
        }

        if (drawn&1) {
          int set = 1;
          if (drawn&2) {
            if (cancel && out->type!=VISUAL_SEEK_NONE) {
              if ((in->type==VISUAL_SEEK_OFFSET && render_info->offset>in->offset) || (in->type==VISUAL_SEEK_X_Y && render_info->x>in->x) || (in->type==VISUAL_SEEK_LINE_COLUMN && render_info->column>in->column) || (in->type==VISUAL_SEEK_INDENTATION_LAST)) {
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
            out->bracket_match = bracket_match;
            out->visual_detail = render_info->visual_detail;
            out->lines = render_info->lines;
            out->indented = render_info->indented;
            for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
              out->depth[n] = render_info->depth_new[n];
              out->min_line[n] = render_info->brackets_line[n].min;
            }
          }
        }
      }

      if (drawn>=2 && !in->clip) {
        stop = 1;
      }

      if (boundary && page_count>1 && drawn<2 && !stop) {
        if (in->type==VISUAL_SEEK_BRACKET_NEXT) {
        } else if (in->type==VISUAL_SEEK_BRACKET_PREV) {
        } else if (in->type==VISUAL_SEEK_INDENTATION_LAST) {
        } else {
          rendered = 0;
        }

        stop = 1;
      }
    }

    if (cp==newline_cp2 && newline_cp2!=0) {
      render_info->visual_detail |= VISUAL_DETAIL_CONTROLCHARACTER;
    }

    if (cp==newline_cp1) {
      render_info->visual_detail &= ~VISUAL_DETAIL_CONTROLCHARACTER;
    }

    if (!view->continuous && render_info->draw_indentation && view->show_invisibles) {
      if (screen && render_info->y_view==render_info->y) {
        codepoint_t cp = 0x21aa;
        position_t x = render_info->x-file->tabstop_width-view->scroll_x+view->address_width;
        if (x>=view->address_width) {
          splitter_drawchar(splitter, screen, (int)x, (int)(render_info->y-view->scroll_y), &cp, 1, file->defaults.colors[VISUAL_FLAG_COLOR_STATUS], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
        }
      }
    }

    if ((boundary || !page_dirty || in->clip) && stop) {
      break;
    }

    render_info->draw_indentation = 0;

    if (render_info->keyword_length>0) {
      render_info->keyword_length--;
      color = render_info->keyword_color;
    }

    int fill = 0;
    codepoint_t fill_code = -1;
    if (((((cp!=newline_cp2 || newline_cp2==0) && cp!=0xfeff) || view->show_invisibles) && cp!='\t') || view->continuous) {
      fill = unicode_width(&codepoints[0], read);
      if (view->show_invisibles && fill<=0) {
        fill = 1;
      }
    } else if (cp=='\t') {
      fill = file->tabstop_width-(render_info->x%file->tabstop_width);
      fill_code = ' ';
    }

    if (screen && render_info->y_view==render_info->y) {
      if (cp==newline_cp1) {
        show = view->show_invisibles?0x00ac:' ';
      } else if (cp==newline_cp2 && newline_cp2!=0) {
        show = view->show_invisibles?0x00ac:-1;
      } else if (cp=='\t') {
        show = view->show_invisibles?0x00bb:' ';
      } else if (cp==' ') {
        show = view->show_invisibles?0x22c5:' ';
      } else if (cp==0x7f) {
        show = view->show_invisibles?0xfffd:show;
      } else if (cp==0xfeff) {
        show = view->show_invisibles?0x2433:show;
      } else if (cp<0) {
        show = 0xfffd;
      }

      if (show!=-1 && cp!='\t') {
        fill = 1;
      }

      if (fill>0) {
        if (render_info->offset>=view->selection_low && render_info->offset<view->selection_high) {
          background = file->defaults.colors[VISUAL_FLAG_COLOR_SELECTION];
        }

        codepoint_t codepoints_visual[8];
        for (size_t code = 0; code<read; code++) {
          codepoints_visual[code] = (file->encoding->visual)(file->encoding, codepoints[code]);
        }

        position_t x = render_info->x-view->scroll_x+view->address_width;
        position_t y = render_info->y-view->scroll_y;
        if (x>=view->address_width) {
          if (show!=-1) {
            splitter_drawchar(splitter, screen, (int)x++, (int)y, &show, 1, color, background);
          } else {
            splitter_drawchar(splitter, screen, (int)x++, (int)y, &codepoints_visual[0], read, color, background);
          }
        }

        for (int pos = 1; pos<fill; pos++) {
          splitter_drawchar(splitter, screen, (int)x++, (int)y, &fill_code, 1, color, background);
        }
      }
    }

    render_info->x += fill;
    if (render_info->visual_detail&VISUAL_DETAIL_NEWLINE) {
      render_info->visual_detail &= ~VISUAL_DETAIL_NEWLINE;
      render_info->indented = 1;
    }

    if (!(render_info->visual_detail&VISUAL_DETAIL_INDENTATION)) {
      render_info->visual_detail |= VISUAL_DETAIL_STOPPED_INDENTATION;
      render_info->indented = 0;
    }

    if (render_info->indented && !view->continuous && render_info->indentation<render_info->width/2) {
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

    if (render_info->keyword_length==0) {
      if (!(render_info->visual_detail&VISUAL_DETAIL_CONTROLCHARACTER)) {
        render_info->offset_sync = render_info->offset;
      }
    }

    position_t word_length = 0;
    if (cp<=' ' && view->wrapping) {
      position_t row_width = render_info->width-render_info->indentation-file->tabstop_width;
      word_length = document_text_render_lookahead_word_wrap(file, &render_info->cache, row_width+1);
      if (word_length>row_width) {
        word_length = 0;
      }
    }

    if ((!view->continuous && cp==newline_cp1) || (render_info->x+word_length>=render_info->width && view->wrapping) || (render_info->x+fill>=render_info->width && view->continuous)) {
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

        render_info->visual_detail |= VISUAL_DETAIL_WRAPPED;
      }

      render_info->ys++;
      render_info->y_view++;
      render_info->x = render_info->indentation+render_info->indentation_extra;
      render_info->xs = 0;
    }

    if (cp==newline_cp1) {
      if (render_info->visual_detail&VISUAL_DETAIL_WHITESPACED_COMPLETE) {
        render_info->visual_detail |= VISUAL_DETAIL_WHITESPACED_START;
      }

      render_info->visual_detail |= VISUAL_DETAIL_NEWLINE;

      render_info->visual_detail &= ~(VISUAL_DETAIL_WRAPPED|VISUAL_DETAIL_STOPPED_INDENTATION);

      render_info->whitespaced = 0;
      render_info->indented = 0;

      render_info->line++;
      render_info->lines++;
      render_info->column = 0;
      render_info->columns = 0;

      for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
        render_info->brackets_line[n].diff = 0;
        render_info->brackets_line[n].min = 0;
        render_info->brackets_line[n].max = 0;
        render_info->depth_line[n] = render_info->depth_new[n];
      }
    }

    if (cp!='\t' && cp!=' ' && cp!=newline_cp2) {
      render_info->visual_detail &= ~VISUAL_DETAIL_WHITESPACED_COMPLETE;
    }

    if (bracket_match&VISUAL_BRACKET_CLOSE) {
      int bracket = bracket_match&VISUAL_BRACKET_MASK;
      render_info->depth_new[bracket]--;
      int min = render_info->depth_old[bracket]-render_info->depth_new[bracket];
      if (min>render_info->brackets[bracket].min) {
        render_info->brackets[bracket].min = min;
      }

      render_info->brackets[bracket].diff--;

      int min_line = render_info->depth_line[bracket]-render_info->depth_new[bracket];
      if (min_line>render_info->brackets_line[bracket].min) {
        render_info->brackets_line[bracket].min = min_line;
      }

      render_info->brackets_line[bracket].diff--;
    }

    if (bracket_match&VISUAL_BRACKET_OPEN) {
      int bracket = bracket_match&VISUAL_BRACKET_MASK;
      render_info->depth_new[bracket]++;
      int max = render_info->depth_new[bracket]-render_info->depth_old[bracket];
      if (max>render_info->brackets[bracket].max) {
        render_info->brackets[bracket].max = max;
      }

      render_info->brackets[bracket].diff++;

      int max_line = render_info->depth_new[bracket]-render_info->depth_line[bracket];
      if (max_line>render_info->brackets_line[bracket].max) {
        render_info->brackets_line[bracket].max = max_line;
      }

      render_info->brackets_line[bracket].diff++;
    }

    if (in->clip && render_info->buffer) {
      // TODO: render_info->buffer->visuals.ys==render_info->ys is hacked since some lines didn't wrap during rendering (which is wrong anyway, retest if span render buffer is available)
      if ((render_info->y_view>render_info->y && render_info->buffer->visuals.ys>=render_info->ys) || (render_info->y_view==render_info->y && render_info->x-view->scroll_x>render_info->width)) {
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
  document_text_render_clear(&render_info, splitter->client_width-view->address_width);
  document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
  document_text_render_span(&render_info, NULL, NULL, view, file, &in, NULL, 16, 1);

  return file->buffer?file->buffer->visuals.dirty:0;
}

// Draw entire visible screen space
void document_text_draw(struct document* base, struct screen* screen, struct splitter* splitter) {
  debug_screen = screen;
  debug_splitter = splitter;
  struct document_text* document = (struct document_text*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (file->save && file->buffer) {
    if (file->buffer->length!=view->selection->length) {
      printf("\r\nSelection area and file length differ %d<>%d\r\n", (int)file->buffer->length, (int)view->selection->length);
      exit(0);
    }
  }

  view->address_width = 6;

  struct document_text_position cursor;
  struct document_text_position in;

  int prerender = 0;
  in.type = VISUAL_SEEK_OFFSET;
  in.clip = 0;
  in.offset = view->offset;

  document_text_cursor_position(splitter, &in, &cursor, 0, 1);

  struct document_text_render_info render_info;
  while (1) {
    position_t scroll_x = view->scroll_x;
    position_t scroll_y = view->scroll_y;
    if (cursor.x<view->scroll_x) {
      view->scroll_x = cursor.x;
    }
    if (cursor.x>=view->scroll_x+splitter->client_width-1-view->address_width) {
      view->scroll_x = cursor.x-(splitter->client_width-1-view->address_width);
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

    document_text_render_clear(&render_info, splitter->client_width-view->address_width);
    in.type = VISUAL_SEEK_X_Y;
    in.clip = 0;
    in.x = view->scroll_x+splitter->client_width;
    in.y = view->scroll_y+splitter->client_height;

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
    view->show_scrollbar = 1;
  }

  document_text_render_clear(&render_info, splitter->client_width-view->address_width);
  in.type = VISUAL_SEEK_X_Y;
  in.clip = 1;
  in.x = view->scroll_x;
  position_t last_line = -1;
  for (position_t y = 0; y<splitter->client_height+1; y++) {
    in.y = y+view->scroll_y;

    // Get render start offset (for bookmark detection)
    struct document_text_position out;
    struct document_text_position in_x_y;
    in_x_y.type = VISUAL_SEEK_X_Y;
    in_x_y.clip = 0;
    in_x_y.x = in.x;
    in_x_y.y = in.y;

    document_text_cursor_position_partial(&render_info, splitter, &in_x_y, &out, 0, 1);

    // Actual rendering
    while (1) {
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      if (document_text_render_span(&render_info, screen, splitter, view, file, &in, NULL, ~0, 1)) {
        break;
      }
    }

    // Bookmark detection
    int marked = range_tree_marked(file->bookmarks, out.offset, render_info.offset-out.offset, TIPPSE_INSERTER_MARK);

    if (in.y<=(file->buffer?file->buffer->visuals.ys:0)) {
      char line[1024];
      int size;
      if (out.line!=last_line) {
        last_line = out.line;
        size = sprintf(line, "%5d", (int)(out.line+1));
      } else {
        size = sprintf(line, "     ");
      }

      splitter_drawtext(splitter, screen, 0, (int)y, line, (size_t)size, file->defaults.colors[VISUAL_FLAG_COLOR_LINENUMBER], file->defaults.colors[marked?VISUAL_FLAG_COLOR_PREPROCESSOR:VISUAL_FLAG_COLOR_BACKGROUND]);
    }
  }

  if (!screen) {
    return;
  }

  size_t name_length = strlen(file->filename);
  char* title = malloc((name_length+(size_t)document_undo_modified(file)*2+1)*sizeof(char));
  memcpy(title, file->filename, name_length);
  if (document_undo_modified(file)) {
    memcpy(title+name_length, " *\0", 3);
  } else {
    title[name_length] = '\0';
  }
  splitter_name(splitter, title);
  free(title);

  if (view->selection_low!=view->selection_high) {
    splitter_cursor(splitter, screen, -1, -1);
  } else {
    splitter_cursor(splitter, screen, (int)(cursor.x-view->scroll_x+view->address_width), (int)(cursor.y-view->scroll_y));
  }

  int mark1 = document_text_mark_brackets(base, screen, splitter, &cursor);
  if (mark1<=0) {
    int mark2 = -1;
    struct document_text_position left;
    if (cursor.column>0 && mark1==-1) {
      in.type = VISUAL_SEEK_LINE_COLUMN;
      in.clip = 0;
      in.line = cursor.line;
      in.column = cursor.column-1;

      document_text_cursor_position(splitter, &in, &left, 0, 1);
      mark2 = document_text_mark_brackets(base, screen, splitter, &left);
    }

    if (mark1==0) {
      splitter_hilight(splitter, screen, (int)(cursor.x-view->scroll_x+view->address_width), (int)(cursor.y-view->scroll_y), file->defaults.colors[VISUAL_FLAG_COLOR_BRACKETERROR]);
    } else if (mark2==0) {
      splitter_hilight(splitter, screen, (int)(left.x-view->scroll_x+view->address_width), (int)(left.y-view->scroll_y), file->defaults.colors[VISUAL_FLAG_COLOR_BRACKETERROR]);
    }
  }

  if (!document->keep_status) {
    const char* newline[TIPPSE_NEWLINE_MAX] = {"Auto", "Lf", "Cr", "CrLf"};
    const char* tabstop[TIPPSE_TABSTOP_MAX] = {"Auto", "Tab", "Space"};
    char status[1024];
    sprintf(&status[0], "%s%s%d/%d:%d - %d/%d byte - %s*%d %s %s %s", (file->buffer?file->buffer->visuals.dirty:0)?"? ":"", (file->buffer?(file->buffer->inserter&TIPPSE_INSERTER_FILE):0)?"File ":"", (int)(file->buffer?file->buffer->visuals.lines+1:0), (int)(cursor.line+1), (int)(cursor.column+1), (int)view->offset, file->buffer?(int)file->buffer->length:0, tabstop[file->tabstop], file->tabstop_width, newline[file->newline], (*file->type->name)(), (*file->encoding->name)());
    splitter_status(splitter, &status[0], 0);
  }

  document->keep_status = 0;
  view->scroll_y_max = file->buffer?file->buffer->visuals.ys:0;
  splitter_scrollbar(splitter, screen);
}

// Handle key press
void document_text_keypress(struct document* base, struct splitter* splitter, int command, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (view->line_select) {
    document_text_keypress_line_select(base, splitter, command, key, cp, button, button_old, x, y);
    return;
  }

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
  file_offset_t file_size = file->buffer?file->buffer->length:0;
  file_offset_t offset_old = view->offset;
  int seek = 0;
  int selection_keep = 0;

  if (command!=TIPPSE_CMD_UP && command!=TIPPSE_CMD_DOWN && command!=TIPPSE_CMD_PAGEDOWN && command!=TIPPSE_CMD_PAGEUP && command!=TIPPSE_CMD_SELECT_UP && command!=TIPPSE_CMD_SELECT_DOWN && command!=TIPPSE_CMD_SELECT_PAGEDOWN && command!=TIPPSE_CMD_SELECT_PAGEUP) {
    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  }

  in_x_y.x = view->cursor_x;
  in_x_y.y = view->cursor_y;

  if (command==TIPPSE_CMD_UP || command==TIPPSE_CMD_SELECT_UP) {
    in_x_y.y--;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_DOWN || command==TIPPSE_CMD_SELECT_DOWN) {
    in_x_y.y++;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_LEFT || command==TIPPSE_CMD_SELECT_LEFT) {
    in_x_y.x--;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (command==TIPPSE_CMD_RIGHT || command==TIPPSE_CMD_SELECT_RIGHT) {
    in_x_y.x++;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 0);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (command==TIPPSE_CMD_PAGEDOWN || command==TIPPSE_CMD_SELECT_PAGEDOWN) {
    int page = splitter->client_height;
    in_x_y.y += page;
    view->scroll_y += page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_PAGEUP || command==TIPPSE_CMD_SELECT_PAGEUP) {
    int page = splitter->client_height;
    in_x_y.y -= page;
    view->scroll_y -= page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_BACKSPACE) {
    if (!document_file_delete_selection(splitter->file, splitter->view)) {
      in_x_y.x--;
      file_offset_t start = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      document_file_delete(splitter->file, start, view->offset-start);
    }
    seek = 1;
  } else if (command==TIPPSE_CMD_DELETE) {
    if (!document_file_delete_selection(splitter->file, splitter->view)) {
      in_x_y.x++;
      file_offset_t end = document_text_cursor_position(splitter, &in_x_y, &out, 1, 0);
      document_file_delete(splitter->file, view->offset, end-view->offset);
    }
    seek = 1;
  } else if (command==TIPPSE_CMD_FIRST || command==TIPPSE_CMD_SELECT_FIRST) {
    struct range_tree_node* first = range_tree_find_indentation_last(out.buffer, out.lines, out.buffer?out.buffer:range_tree_last(file->buffer));
    int seek_first = 1;

    if (first) {
      struct document_text_position in;
      in.type = VISUAL_SEEK_INDENTATION_LAST;
      in.clip = 0;
      in.offset = range_tree_offset(first);
      in.line = out.line;

      struct document_text_position out_first;
      struct document_text_render_info render_info;
      document_text_render_clear(&render_info, splitter->client_width-view->address_width);
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      document_text_render_span(&render_info, NULL, splitter, view, file, &in, &out_first, ~0, 1);

      if (out_first.type!=VISUAL_SEEK_NONE) {
        view->cursor_x = out_first.x;
        view->cursor_y = out_first.y;
        view->offset = out_first.offset;
        seek_first = (out_first.offset==out.offset)?1:0;
      }
    }

    if (seek_first) {
      in_line_column.line = out.line;
      in_line_column.column = 0;
      view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
    }
  } else if (command==TIPPSE_CMD_LAST || command==TIPPSE_CMD_SELECT_LAST) {
    in_line_column.line = out.line;
    in_line_column.column = POSITION_T_MAX;
    view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (command==TIPPSE_CMD_HOME || command==TIPPSE_CMD_SELECT_HOME) {
    in_x_y.x = 0;
    in_x_y.y = 0;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_END || command==TIPPSE_CMD_SELECT_END) {
    in_x_y.x = POSITION_T_MAX;
    in_x_y.y = POSITION_T_MAX;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_UNDO) {
    document_undo_execute_chain(file, view, file->undos, file->redos, 0);
    return;
  } else if (command==TIPPSE_CMD_REDO) {
    document_undo_execute_chain(file, view, file->redos, file->undos, 1);
    return;
  } else if (command==TIPPSE_CMD_BOOKMARK) {
    document_text_toggle_bookmark(base, splitter, view->offset);
    return;
  } else if (command==TIPPSE_CMD_MOUSE) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      in_x_y.x = x+view->scroll_x-view->address_width;
      in_x_y.y = y+view->scroll_y;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
      if (view->selection_start==FILE_OFFSET_T_MAX) {
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
      view->show_scrollbar = 1;
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y -= page;
      view->scroll_y -= page;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
      view->show_scrollbar = 1;
    }
  } else if (command==TIPPSE_CMD_TAB) {
    document_undo_chain(file, file->undos);
    if (view->selection_low==FILE_OFFSET_T_MAX) {
      uint8_t utf8[8];
      file_offset_t size;
      if (file->tabstop==TIPPSE_TABSTOP_SPACE) {
        size = (file_offset_t)(file->tabstop_width-(view->cursor_x%file->tabstop_width));
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
      document_text_raise_indentation(base, splitter, view->selection_low, view->selection_high-1, 1);
    }
    document_undo_chain(file, file->undos);
    seek = 1;
    selection_keep = 1;
  } else if (command==TIPPSE_CMD_UNTAB) {
    document_undo_chain(file, file->undos);
    if (view->selection_low!=FILE_OFFSET_T_MAX) {
      document_text_lower_indentation(base, splitter, view->selection_low, view->selection_high-1);
    }
    document_undo_chain(file, file->undos);
    seek = 1;
    selection_keep = 1;
  } else if (command==TIPPSE_CMD_SELECT_ALL) {
    view->selection_start = 0;
    view->selection_end = file_size;
    selection_keep = 1;
  } else if (command==TIPPSE_CMD_COPY || command==TIPPSE_CMD_CUT) {
    document_undo_chain(file, file->undos);
    if (view->selection_low!=FILE_OFFSET_T_MAX) {
      clipboard_set(range_tree_copy(file->buffer, view->selection_low, view->selection_high-view->selection_low), file->binary);
      if (command==TIPPSE_CMD_CUT) {
        document_file_delete_selection(splitter->file, splitter->view);
      } else {
        selection_keep = 1;
      }
    }
    document_undo_chain(file, file->undos);
    seek = 1;
  } else if (command==TIPPSE_CMD_PASTE) {
    document_undo_chain(file, file->undos);
    document_file_delete_selection(splitter->file, splitter->view);

    struct range_tree_node* buffer = clipboard_get();
    if (buffer) {
      document_file_insert_buffer(splitter->file, view->offset, buffer);
    }

    document_undo_chain(file, file->undos);
    seek = 1;
  } else if (command==TIPPSE_CMD_SHOW_INVISIBLES) {
    view->show_invisibles ^= 1;
    (*base->reset)(base, splitter);
  } else if (command==TIPPSE_CMD_WORDWRAP) {
    view->wrapping ^= 1;
    (*base->reset)(base, splitter);
  } else if (command==TIPPSE_CMD_RETURN) {
    if (out.lines==0) {
      range_tree_find_bracket_lowest(out.buffer, out.min_line, out.buffer?out.buffer:range_tree_last(file->buffer));
    }

    struct document_text_position out_indentation_copy;
    if (out.column!=0) {
      in_line_column.line = out.line;
      in_line_column.column = 0;
      document_text_cursor_position(splitter, &in_line_column, &out_indentation_copy, 0, 1);
    } else {
      out_indentation_copy = out;
    }

    document_file_delete_selection(splitter->file, splitter->view);

    if (file->newline==TIPPSE_NEWLINE_CRLF) {
      document_file_insert(splitter->file, view->offset, (uint8_t*)"\r\n", 2);
    } else if (file->newline==TIPPSE_NEWLINE_CR) {
      document_file_insert(splitter->file, view->offset, (uint8_t*)"\r", 1);
    } else {
      document_file_insert(splitter->file, view->offset, (uint8_t*)"\n", 1);
    }

    // --- Begin test auto indentation ... opening bracket
    // Build a binary copy of the previous indentation (one could insert the default indentation style as alternative... to discuss)
    // TODO: Simplify me
    file_offset_t offset = out_indentation_copy.offset;
    file_offset_t displacement;
    struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
    file_offset_t displacement_start = displacement;
    while (buffer && offset!=out.offset) {
      if (displacement>=buffer->length || !buffer->buffer) {
        if (displacement!=displacement_start) {
          document_file_insert(splitter->file, view->offset, buffer->buffer->buffer+buffer->offset+displacement_start, displacement-displacement_start);
        }

        // Brrr... reresearch next node since it could have been fused by the last insert
        buffer = range_tree_find_offset(file->buffer, offset, &displacement);
        displacement_start = displacement;
        continue;
      }

      const uint8_t* text = buffer->buffer->buffer+buffer->offset+displacement;
      if (*text!='\t' && *text!=' ') {
        break;
      }

      displacement++;
      offset++;
    }

    if (buffer && displacement!=displacement_start) {
      document_file_insert(splitter->file, view->offset, buffer->buffer->buffer+buffer->offset+displacement_start, displacement-displacement_start);
    }

    int diff = 0;
    for (size_t bracket = 0; bracket<VISUAL_BRACKET_MAX; bracket++) {
      int add = out.depth[bracket]-out_indentation_copy.depth[bracket];
      if (add>0) {
        diff += add;
      }

      if (add==0 && out.min_line[bracket]>0) {
        diff++;
      }
    }

    if (diff>0) {
      document_text_raise_indentation(base, splitter, view->offset, view->offset, 0);
    }
    // --- End test auto indentation ... opening bracket

    seek = 1;
  } else if (command==TIPPSE_CMD_CHARACTER) {
    document_file_delete_selection(splitter->file, splitter->view);
    uint8_t coded[8];
    size_t size = splitter->file->encoding->encode(splitter->file->encoding, cp, &coded[0], 8);
    document_file_insert(splitter->file, view->offset, &coded[0], size);

    // --- Begin test auto indentation ... closing bracket
    struct document_text_position out_bracket;
    in_line_column.line = out.line;
    in_line_column.column = out.column;
    document_text_cursor_position(splitter, &in_line_column, &out_bracket, 0, 1);

    if ((out_bracket.bracket_match&VISUAL_BRACKET_CLOSE) && out_bracket.indented) {
      document_text_lower_indentation(base, splitter, view->offset, view->offset);
    }
    // --- End test auto indentation ... closing bracket

    seek = 1;
  }

  if (seek) {
    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
    view->offset = out.offset;
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  }

  int selection_reset = 0;
  if (command==TIPPSE_CMD_MOUSE) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      if (!(button_old&TIPPSE_MOUSE_LBUTTON) && !(key&TIPPSE_KEY_MOD_SHIFT)) view->selection_start = FILE_OFFSET_T_MAX;
      if (view->selection_start==FILE_OFFSET_T_MAX) view->selection_start = view->offset;
      view->selection_end = view->offset;
    }
  } else {
    if (command==TIPPSE_CMD_SELECT_UP || command==TIPPSE_CMD_SELECT_DOWN || command==TIPPSE_CMD_SELECT_LEFT || command==TIPPSE_CMD_SELECT_RIGHT || command==TIPPSE_CMD_SELECT_PAGEUP || command==TIPPSE_CMD_SELECT_PAGEDOWN || command==TIPPSE_CMD_SELECT_FIRST || command==TIPPSE_CMD_SELECT_LAST || command==TIPPSE_CMD_SELECT_HOME || command==TIPPSE_CMD_SELECT_END) {
      if (view->selection_start==FILE_OFFSET_T_MAX) view->selection_start = offset_old;
      view->selection_end = view->offset;
    } else {
      selection_reset = selection_keep?0:1;
    }
  }
  if (selection_reset) {
    view->selection_start = FILE_OFFSET_T_MAX;
    view->selection_end = FILE_OFFSET_T_MAX;
  }

  if (view->selection_start!=view->selection_end) {
    if (view->selection_start<view->selection_end) {
      view->selection_low = view->selection_start;
      view->selection_high = view->selection_end;
    } else {
      view->selection_low = view->selection_end;
      view->selection_high = view->selection_start;
    }
  } else {
    view->selection_low = FILE_OFFSET_T_MAX;
    view->selection_high = FILE_OFFSET_T_MAX;
  }
}

// In line select mode the keyboard is used in a different way since the display shows a list of options one can't edit
void document_text_keypress_line_select(struct document* base, struct splitter* splitter, int command, int key, codepoint_t cp, int button, int button_old, int x, int y) {
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

  in_offset.offset = view->offset;
  document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
  in_line_column.line = out.line;
  in_line_column.column = 0;
  in_x_y.x = 0;
  in_x_y.y = out.y;

  if (command==TIPPSE_CMD_UP) {
    in_line_column.line--;
    view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_DOWN) {
    in_line_column.line++;
    view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_PAGEDOWN) {
    int page = splitter->client_height;
    in_x_y.y += page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    if (out.line==in_line_column.line) {
      in_line_column.line++;
      view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    }
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_PAGEUP) {
    int page = splitter->client_height;
    in_x_y.y -= page;
    view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    if (out.line==in_line_column.line) {
      in_line_column.line--;
      view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    }
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_HOME) {
    in_line_column.line = 0;
    view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_END) {
    in_line_column.line = POSITION_T_MAX;
    view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_MOUSE) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      in_x_y.x = x+view->scroll_x-view->address_width;
      in_x_y.y = y+view->scroll_y;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 0, 1);
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y += page;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      if (out.line==in_line_column.line) {
        in_line_column.line++;
        view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
      }
      view->show_scrollbar = 1;
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y -= page;
      view->offset = document_text_cursor_position(splitter, &in_x_y, &out, 1, 1);
      if (out.line==in_line_column.line) {
        in_line_column.line--;
        view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
      }
      view->show_scrollbar = 1;
    }
  } else if (command==TIPPSE_CMD_RETURN) {
  }

  in_offset.offset = view->offset;
  document_text_cursor_position(splitter, &in_offset, &out, 1, 1);
  view->offset = out.offset;
  view->cursor_x = 0;
  view->cursor_y = out.y;

  if (view->line_select) {
    in_offset.offset = view->offset;
    in_offset.clip = 0;

    in_offset.offset = view->offset;
    document_text_cursor_position(splitter, &in_offset, &out, 0, 1);

    in_line_column.line = out.line;
    in_line_column.column = 0;
    document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->selection_low = view->selection_start = out.offset;
    in_line_column.column = POSITION_T_MAX;
    document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
    view->selection_high = view->selection_end = out.offset;
  }
}

// Set or clear bookmark range
void document_text_toggle_bookmark(struct document* base, struct splitter* splitter, file_offset_t offset) {
  //struct document_text* document = (struct document_text*)base;
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
  in_line_column.column = POSITION_T_MAX;
  document_text_cursor_position(splitter, &in_line_column, &out, 1, 1);
  file_offset_t end = out.offset;

  int marked = range_tree_marked(file->bookmarks, start, end-start, TIPPSE_INSERTER_MARK);
  file->bookmarks = range_tree_mark(file->bookmarks, start, end-start, marked?0:TIPPSE_INSERTER_MARK);
  // range_tree_print(file->bookmarks, 0, 0);
}

// Lower indentation for selected range (if possible)
void document_text_lower_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high) {
  struct document_file* file = splitter->file;

  struct document_text_position in_offset;
  in_offset.type = VISUAL_SEEK_OFFSET;
  in_offset.clip = 0;

  struct document_text_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
  in_line_column.clip = 0;

  struct document_text_position out;
  struct document_text_position out_start;
  struct document_text_position out_end;

  in_offset.offset = low;
  document_text_cursor_position(splitter, &in_offset, &out_start, 0, 1);
  in_offset.offset = high;
  document_text_cursor_position(splitter, &in_offset, &out_end, 0, 1);

  while (out_end.line>=out_start.line) {
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
      if (*text=='\t' || (int)length>=file->tabstop_width) {
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

// Raise indentation for selected range
void document_text_raise_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high, int empty_lines) {
  struct document_file* file = splitter->file;

  struct document_text_position in_offset;
  in_offset.type = VISUAL_SEEK_OFFSET;
  in_offset.clip = 0;

  struct document_text_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
  in_line_column.clip = 0;

  struct document_text_position out;
  struct document_text_position out_start;
  struct document_text_position out_end;

  in_offset.offset = low;
  document_text_cursor_position(splitter, &in_offset, &out_start, 0, 1);
  in_offset.offset = high;
  document_text_cursor_position(splitter, &in_offset, &out_end, 0, 1);

  while (out_end.line>=out_start.line) {
    in_line_column.column = 0;
    in_line_column.line = out_end.line;
    document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);

    struct range_tree_node* buffer = out.buffer;
    file_offset_t displacement = out.displacement;
    file_offset_t offset = out.offset;

    uint8_t utf8[8];
    file_offset_t size;
    if (file->tabstop==TIPPSE_TABSTOP_SPACE) {
      size = (file_offset_t)file->tabstop_width;
      for (file_offset_t spaces = 0; spaces<size; spaces++) {
        utf8[spaces] = ' ';
      }
    } else {
      size = 1;
      utf8[0] = '\t';
    }

    const uint8_t* text = buffer?buffer->buffer->buffer+buffer->offset+displacement:NULL;
    if (!buffer || (*text!='\r' && *text!='\n') || !empty_lines) {
      document_file_insert(splitter->file, offset, &utf8[0], size);
    }

    out_end.line--;
  }
}

// Mark the bracket and its partner below the cursor position and return whether something was marked
int document_text_mark_brackets(struct document* base, struct screen* screen, struct splitter* splitter, struct document_text_position* cursor) {
  if (!(cursor->bracket_match&(VISUAL_BRACKET_OPEN|VISUAL_BRACKET_CLOSE))) {
    return -1;
  }

  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  struct document_text_position in;
  struct document_text_position out;
  struct document_text_render_info render_info;

  in.clip = 0;
  in.bracket = cursor->bracket_match&VISUAL_BRACKET_MASK;
  in.bracket_search = cursor->depth[in.bracket];
  if (cursor->bracket_match&VISUAL_BRACKET_OPEN) {
    in.type = VISUAL_SEEK_BRACKET_NEXT;
    in.offset = cursor->offset+1;
  } else {
    in.type = VISUAL_SEEK_BRACKET_PREV;
    in.offset = (cursor->offset>0)?cursor->offset-1:0;
    size_t bracket = cursor->bracket_match&VISUAL_BRACKET_MASK;
    if (bracket==in.bracket) {
      in.bracket_search--;
    }
  }

  document_text_render_clear(&render_info, splitter->client_width-view->address_width);
  while (1) {
    document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
    int rendered = document_text_render_span(&render_info, NULL, splitter, view, file, &in, &out, ~0, 1);
    if (rendered==1) {
      break;
    }

    if (rendered==-1) {
      document_text_render_clear(&render_info, splitter->client_width-view->address_width);
      document_text_render_seek(&render_info, file->buffer, file->encoding, &in);
      if (cursor->bracket_match&VISUAL_BRACKET_OPEN) {
        struct range_tree_node* node = range_tree_find_bracket_forward(render_info.buffer, in.bracket, in.bracket_search);
        if (!node) {
          node = range_tree_last(file->buffer);
          if (render_info.buffer==node) {
            break;
          }
        }

        in.offset = range_tree_offset(node)+node->visuals.displacement;
      } else {
        struct range_tree_node* node = range_tree_find_bracket_backward(render_info.buffer, in.bracket, in.bracket_search);
        if (!node) {
          node = range_tree_first(file->buffer);
        }

        if (render_info.buffer==range_tree_first(file->buffer)) {
          break;
        }

        in.offset = range_tree_offset(node)+node->length-1-node->visuals.rewind;
      }
    }
  }

  if (((cursor->bracket_match&VISUAL_BRACKET_OPEN) && out.offset>cursor->offset) || ((cursor->bracket_match&VISUAL_BRACKET_CLOSE) && out.offset<cursor->offset && out.type!=VISUAL_SEEK_NONE)) {
    splitter_hilight(splitter, screen, (int)(cursor->x-view->scroll_x+view->address_width), (int)(cursor->y-view->scroll_y), file->defaults.colors[VISUAL_FLAG_COLOR_BRACKET]);
    splitter_hilight(splitter, screen, (int)(out.x-view->scroll_x+view->address_width), (int)(out.y-view->scroll_y), file->defaults.colors[VISUAL_FLAG_COLOR_BRACKET]);
    return 1;
  }

  return 0;
}

// Jump to specified line
void document_text_goto(struct document* base, struct splitter* splitter, position_t line) {
  struct document_view* view = splitter->view;

  struct document_text_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;
  in_line_column.clip = 0;
  in_line_column.column = 0;
  in_line_column.line = line;

  struct document_text_position out;

  view->offset = document_text_cursor_position(splitter, &in_line_column, &out, 0, 1);
  view->cursor_x = out.x;
  view->cursor_y = out.y;
  view->show_scrollbar = 1;
}
