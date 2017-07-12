/* Tippse - Document - For mostly combined document file and view operations */

#include "document.h"

int document_compare(struct range_tree_node* left, file_offset_t buffer_pos_left, struct range_tree_node* right_root, file_offset_t length) {
  struct range_tree_node* right = right_root; //range_tree_first(right_root);
  size_t buffer_pos_right = 0;

  while (left && right) {
    if (buffer_pos_left>=left->length || !left->buffer) {
      left = range_tree_next(left);
      buffer_pos_left = 0;
      continue;
    }

    if (buffer_pos_right>=right->length || !right->buffer) {
      right = range_tree_next(right);
      buffer_pos_right = 0;
      continue;
    }

    const char* text_left = left->buffer->buffer+left->offset+buffer_pos_left;
    const char* text_right = right->buffer->buffer+right->offset+buffer_pos_right;
    file_offset_t max = length;
    if (left->length-buffer_pos_left<max) {
      max = left->length-buffer_pos_left;
    }

    if (right->length-buffer_pos_right<max) {
      max = right->length-buffer_pos_right;
    }

    if (strncmp(text_left, text_right, max)!=0) {
      return 0;
    }

    buffer_pos_left += max;
    buffer_pos_right += max;
    length-= max;
    if (length==0) {
      return 1;
    }
  }

  return 0;
}

void document_search(struct splitter* splitter, struct document* document, struct range_tree_node* text, file_offset_t length, int forward) {
  struct document_file* file = document->file;
  struct document_view* view = &document->view;
  if (!text || !file->buffer || file->buffer->length==0) {
    splitter->document.keep_status = 1;
    splitter_status(splitter, "No text to search for!", 1);
    return;
  }

  if (length>file->buffer->length) {
    splitter->document.keep_status = 1;
    splitter_status(splitter, "Not found!", 1);
    return;
  }

  file_offset_t offset = view->offset;
  if (forward) {
    if (view->selection_low!=~0) {
      offset = view->selection_high;
    }
  } else {
    offset--;
    if (view->selection_low!=~0) {
      offset = view->selection_low-1;
    }
    if (offset==~0) {
      offset = file->buffer->length-1;
    }
  }

  file_offset_t pos = offset;
  file_offset_t buffer_pos;
  struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &buffer_pos);
  int wrap = 0;
  if (forward) {
    while (pos<offset || !wrap) {
      if (buffer_pos>=buffer->length || !buffer->buffer) {
        buffer = range_tree_next(buffer);
        if (!buffer) {
          buffer = range_tree_first(file->buffer);
          pos = 0;
          wrap = 1;
        }

        buffer_pos = 0;
        continue;
      }

      if (document_compare(buffer, buffer_pos, text, length)) {
        view->offset = pos;
        view->selection_low = pos;
        view->selection_high = pos+length;
        view->selection_start = pos;
        view->selection_end = pos+length;
        return;
      }

      buffer_pos++;
      pos++;
    }
  } else {
    while (pos>offset || !wrap) {
      if (buffer_pos==~0 || !buffer->buffer) {
        buffer = range_tree_prev(buffer);
        if (!buffer) {
          buffer = range_tree_last(file->buffer);
          pos = file->buffer->length-1;
          wrap = 1;
        }

        buffer_pos = buffer->length-1;
        continue;
      }

      if (document_compare(buffer, buffer_pos, text, length)) {
        view->offset = pos;
        view->selection_low = pos;
        view->selection_high = pos+length;
        view->selection_start = pos;
        view->selection_end = pos+length;
        return;
      }

      buffer_pos--;
      pos--;
    }
  }

  splitter->document.keep_status = 1;
  splitter_status(splitter, "Not found!", 1);
}

file_offset_t document_cursor_position(struct splitter* splitter, struct document_render_info_position* in, struct document_render_info_position* out, int wrap, int cancel) {
  struct document* document = &splitter->document;
  struct document_file* file = document->file;
  struct document_view* view = &document->view;

  struct document_render_info render_info;

  while (1) {
    document_render_info_clear(&render_info, splitter->client_width);
    document_render_info_seek(&render_info, file->buffer, in);
    int rendered = document_render_info_span(&render_info, NULL, NULL, view, file, in, out);

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

    if (!rendered) {
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
    } else if (out->offset==~0) {
      if (*x>out->x_min) {
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

void document_render_info_clear(struct document_render_info* render_info, int width) {
  memset(render_info, 0, sizeof(struct document_render_info));
  render_info->stop = 2;
  render_info->buffer = NULL;
  render_info->width = width;
}

void document_render_info_seek(struct document_render_info* render_info, struct range_tree_node* buffer, struct document_render_info_position* in) {
  file_offset_t offset_new = 0;
  int x_new = 0;
  int y_new = 0;
  int lines_new = 0;
  int columns_new = 0;
  int indentation_new = 0;
  int indentation_extra_new = 0;
  struct range_tree_node* buffer_new;

  buffer_new = range_tree_find_visual(buffer, in->type, in->offset, in->x, in->y, in->line, in->column, &offset_new, &x_new, &y_new, &lines_new, &columns_new, &indentation_new, &indentation_extra_new);

  if (buffer_new && render_info->buffer!=buffer_new && render_info->stop==2) {
    render_info->visual_detail = buffer_new->visuals.detail_before;
    render_info->offset = offset_new;
    render_info->indentation_extra = indentation_extra_new;
    render_info->indentation = indentation_new;
    render_info->x = x_new+render_info->indentation+render_info->indentation_extra;
    render_info->y_view = y_new;
    render_info->line = lines_new;
    render_info->column = columns_new;

    if (render_info->indentation+render_info->indentation_extra>0 && x_new==0 && !(render_info->visual_detail&VISUAL_INFO_INDENTATION)) {
      render_info->draw_indentation = 1;
    } else {
      render_info->draw_indentation = 0;
    }

    render_info->draw_indentation = 0;

    render_info->buffer_pos = 0;
    render_info->xs = 0;
    render_info->ys = 0;
    render_info->lines = 0;
    render_info->columns = 0;
    render_info->indentations = 0;
    render_info->indentations_extra = 0;
    render_info->buffer = buffer_new;
  }

  if (in->type==VISUAL_SEEK_X_Y) {
    render_info->y = in->y;
  } else {
    render_info->y = y_new;
  } 
}

int document_render_lookahead_word_wrap(struct range_tree_node* buffer, file_offset_t buffer_pos, int max) {
  int count = 0;
  while (buffer && count<max) {
    if (buffer_pos>=buffer->length || !buffer->buffer) {
      buffer = range_tree_next(buffer);
      buffer_pos = 0;
      continue;
    }

    int cp = -1;
    const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
    const char* end = buffer->buffer->buffer+buffer->offset+buffer->length;
    const char* copy = text;
    text = utf8_decode(&cp, text, end-text, 1);
    if (cp<=' ') {
      break;
    }

    buffer_pos += text-copy;
    count++;
  }

  return count;
}

int document_render_info_span(struct document_render_info* render_info, struct screen* screen, struct splitter* splitter, struct document_view* view, struct document_file* file, struct document_render_info_position* in, struct document_render_info_position* out) {

  // TODO: Following initializations shouldn't be needed since the caller should check the type / verify
  if (out) {
    out->type = VISUAL_SEEK_NONE;
    out->offset = ~0;
    out->x_min = 0;
    out->x_max = 0;
    out->line = 0;
    out->column = 0;
    out->buffer = NULL;
    out->buffer_pos = 0;
  }

  int keyword_length = 0;
  int keyword_color = 0;

  int found = (in->type==VISUAL_SEEK_OFFSET)?0:1;
  int rendered = 0;
  render_info->stop = 0;

  while (1) {
    if (render_info->buffer && (render_info->buffer_pos>=render_info->buffer->length || !render_info->buffer->buffer)) {
      int dirty = (render_info->buffer->visuals.xs!=render_info->xs || render_info->buffer->visuals.ys!=render_info->ys || render_info->buffer->visuals.columns!=render_info->columns || render_info->buffer->visuals.indentation!=render_info->indentations || render_info->buffer->visuals.indentation_extra!=render_info->indentations_extra || (render_info->ys==0 && render_info->buffer->visuals.dirty))?1:0;

      if (render_info->buffer->visuals.dirty || dirty) {
        render_info->buffer->visuals.dirty = 0;
        render_info->buffer->visuals.xs = render_info->xs;
        render_info->buffer->visuals.ys = render_info->ys;
        render_info->buffer->visuals.columns = render_info->columns;
        render_info->buffer->visuals.indentation = render_info->indentations;
        render_info->buffer->visuals.indentation_extra = render_info->indentations_extra;
        range_tree_update_calc_all(render_info->buffer);
      }

      render_info->indentations = 0;
      render_info->indentations_extra = 0;
      render_info->xs = 0;
      render_info->ys = 0;
      render_info->columns = 0;
      render_info->lines = 0;

      render_info->buffer = range_tree_next(render_info->buffer);
      render_info->buffer_pos = 0;
      if (render_info->buffer) {
        if (render_info->visual_detail!=render_info->buffer->visuals.detail_before || dirty) {
          render_info->buffer->visuals.detail_before = render_info->visual_detail;
          render_info->buffer->visuals.dirty |= VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
          range_tree_update_calc_all(render_info->buffer);
        }
      }
      continue;
    }

    int cp = -1;
    size_t length = 0;

    int sel = (render_info->offset>=view->selection_low && render_info->offset<view->selection_high)?1:0;
    if (render_info->buffer) {
      const char* text = render_info->buffer->buffer->buffer+render_info->buffer->offset+render_info->buffer_pos;
      const char* end = render_info->buffer->buffer->buffer+render_info->buffer->offset+render_info->buffer->length;
      const char* copy = text;
      text = utf8_decode(&cp, text, end-text, 1);
      length = text-copy;
    } else {
      cp = ' ';
    }

    int show = cp;

    if (out && render_info->y_view==render_info->y && in->type==VISUAL_SEEK_X_Y) {
      out->x_max = render_info->x;
      out->x_min = render_info->indentation+render_info->indentation_extra;
      rendered = 1;
    }

    if (out && render_info->line==in->line && in->type==VISUAL_SEEK_LINE_COLUMN) {
      rendered = 1;
    }

    if (out && ((in->type==VISUAL_SEEK_OFFSET && render_info->offset==in->offset) || (in->type==VISUAL_SEEK_X_Y && render_info->y_view==in->y && render_info->x==in->x) || (in->type==VISUAL_SEEK_LINE_COLUMN && render_info->line==in->line && render_info->column==in->column))) {
      out->x = render_info->x;
      out->y = render_info->y_view;
      out->offset = render_info->offset;
      out->line = render_info->line;
      out->column = render_info->column;
      out->buffer = render_info->buffer;
      out->buffer_pos = render_info->buffer_pos;
      found = 1;
    }

     if (render_info->draw_indentation) {
      if (screen && render_info->y_view==render_info->y) {
        splitter_drawchar(screen, splitter, render_info->x-2-view->scroll_x, render_info->y-view->scroll_y, 0x21aa, splitter->foreground, splitter->background);
      }
    }

    if (render_info->stop==1) {
      break;
    } else if (render_info->stop==2) {
      render_info->buffer = NULL;
      break;
    } else if (!render_info->buffer) {
      break;
    }

    render_info->draw_indentation = 0;

    int color = splitter?splitter->foreground:0; //render_info->buffer->visuals.dirty?2:15; //((int)render_info->buffer&0xff); //
    int background = splitter?splitter->background:0;
    if (render_info->buffer) {
      if (!(render_info->buffer->inserter&TIPPSE_INSERTER_READONLY)) {
        if (keyword_length==0) {
          int visual_flag = 0;

          (*file->type->mark)(file->type, &render_info->visual_detail, render_info->buffer, render_info->buffer_pos, (render_info->y_view==render_info->y)?1:0, &keyword_length, &visual_flag);

          if (visual_flag==VISUAL_FLAG_COLOR_BLOCKCOMMENT) {
            keyword_color = 102;
          } else if (visual_flag==VISUAL_FLAG_COLOR_LINECOMMENT) {
            keyword_color = 102;
          } else if (visual_flag==VISUAL_FLAG_COLOR_STRING) {
            keyword_color = 226;
          } else if (visual_flag==VISUAL_FLAG_COLOR_TYPE) {
            keyword_color = 120;
          } else if (visual_flag==VISUAL_FLAG_COLOR_KEYWORD) {
            keyword_color = 210;
          } else if (visual_flag==VISUAL_FLAG_COLOR_PREPROCESSOR) {
            keyword_color = 103;
          }
        }

        if (keyword_length>0) {
          keyword_length--;
          color = keyword_color;
        }
      } else {
        color = 102;
      }
    } else {
      color = 102;
    }

    render_info->buffer_pos += length;
    render_info->offset += length;

    if (((cp!='\r' && cp!=0xfeff) || view->showall) && cp!='\t') {
      if (screen && render_info->y_view==render_info->y) {
        if (cp<0x20 && cp!='\n') {
          show = 0xfffd;
        }

        if (cp=='\n') {
          show = view->showall?0x21a7:' ';
        } else if (cp=='\r') {
          show = view->showall?0x21a4:' ';
        } else if (cp==0xfeff) {
          show = view->showall?0x66d:' ';
        }

        if (!sel) {
          splitter_drawchar(screen, splitter, render_info->x-view->scroll_x, render_info->y-view->scroll_y, show, color, background);
        } else {
          splitter_drawchar(screen, splitter, render_info->x-view->scroll_x, render_info->y-view->scroll_y, show, background, color);
        }
      }

      render_info->x++;
      if (render_info->visual_detail&VISUAL_INFO_INDENTATION) {
        render_info->indentation++;
        render_info->indentations++;
      } else {
        render_info->xs++;
      }
    }

    if (cp=='\t') {
      int tabbing = TAB_WIDTH-(render_info->x%TAB_WIDTH);
      show = view->showall?0x21a6:' ';
      while (tabbing-->0) {
        if (screen && render_info->y_view==render_info->y) {
          if (!sel) {
            splitter_drawchar(screen, splitter, render_info->x-view->scroll_x, render_info->y-view->scroll_y, show, splitter->foreground, background);
          } else {
            splitter_drawchar(screen, splitter, render_info->x-view->scroll_x, render_info->y-view->scroll_y, show, background, splitter->foreground);
          }

          show = ' ';
        }

        render_info->x++;
        if (render_info->visual_detail&VISUAL_INFO_INDENTATION) {
          render_info->indentation++;
          render_info->indentations++;
        } else {
          render_info->xs++;
        }
      }
    }

    render_info->column++;
    render_info->columns++;

    int word_length = 0;
    if ((cp<=' ') && !(render_info->visual_detail&VISUAL_INFO_INDENTATION) && view->wrapping) {
      word_length = document_render_lookahead_word_wrap(render_info->buffer, render_info->buffer_pos, render_info->width-render_info->x);
    }

    if (cp=='\n' || (render_info->x+word_length>=render_info->width && view->wrapping)) {
      if (cp=='\n') {
        render_info->line++;
        render_info->lines++;
        render_info->column = 0;
        render_info->columns = 0;
        render_info->indentations = 0;
        render_info->indentations_extra = 0;
        render_info->indentation = 0;
        render_info->indentation_extra = 0;
        render_info->draw_indentation = 0;
      } else {
        render_info->draw_indentation = 1;

        if (render_info->indentation_extra==0) {
          render_info->indentations_extra = 2;
          render_info->indentation_extra = 2;
        }
      }

      render_info->ys++;
      render_info->y_view++;
      render_info->x = render_info->indentation+render_info->indentation_extra;
      render_info->xs = 0;

      // TODO: Argh ... deduplicate me / test hack
      if (found && in->type==VISUAL_SEEK_X_Y && render_info->y_view>render_info->y) {
        render_info->stop = 1;
      }
      if (found && in->type==VISUAL_SEEK_OFFSET && render_info->offset>in->offset) {
        render_info->stop = 1;
      }
      if (found && in->type==VISUAL_SEEK_LINE_COLUMN && render_info->line>in->line) {
        render_info->stop = 1;
      }
    }

    // TODO: for very long lines the draw should abort if the buffer can't reach the display area / speed improvement
    // TODO: check for other seek types too
    if (found && in->type==VISUAL_SEEK_X_Y && render_info->stop==0 && ((render_info->y_view>render_info->y) || (render_info->y_view==render_info->y && (render_info->x-view->scroll_x>=render_info->width /*|| render_info->x-view->scroll_x<-TREE_BLOCK_LENGTH_MAX*/)))) {
      render_info->stop = 2;
    }
  }

  return rendered;
}

void document_draw(struct screen* screen, struct splitter* splitter) {
  struct document* document = &splitter->document;
  struct document_file* file = document->file;
  struct document_view* view = &document->view;
  struct document_render_info render_info;
  document_render_info_clear(&render_info, splitter->client_width);

  struct document_render_info_position out;
  struct document_render_info_position in;
  in.type = VISUAL_SEEK_OFFSET;
  in.offset = view->offset;

  document_cursor_position(splitter, &in, &out, 1, 1);

  int cursor_x = out.x;
  int cursor_y = out.y;
  
  if (cursor_x<view->scroll_x) {
    view->scroll_x = cursor_x;
  }
  if (cursor_x>=view->scroll_x+splitter->client_width-1) {
    view->scroll_x = cursor_x-(splitter->client_width-1);
  }
  if (cursor_y<view->scroll_y) {
    view->scroll_y = cursor_y;
  }
  if (cursor_y>=view->scroll_y+splitter->client_height-1) {
    view->scroll_y = cursor_y-(splitter->client_height-1);
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

  int y;

  in.type = VISUAL_SEEK_X_Y;
  in.x = view->scroll_x;

  for (y=0; y<splitter->client_height; y++) {
    in.y = y+view->scroll_y;

    document_render_info_seek(&render_info, file->buffer, &in);
    document_render_info_span(&render_info, screen, splitter, view, file, &in, &out);
  }

  document_render_info_clear(&render_info, splitter->client_width);
  in.type = VISUAL_SEEK_OFFSET;
  in.offset = view->offset;
  document_render_info_seek(&render_info, file->buffer, &in);
  document_render_info_span(&render_info, screen, splitter, view, file, &in, &out);

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

  splitter_cursor(screen, splitter, cursor_x-view->scroll_x, cursor_y-view->scroll_y);

  if (!splitter->document.keep_status) {
    char status[1024];
    sprintf(&status[0], "%d %d/%d:%d - %d/%d byte - %d chars", file->buffer?file->buffer->visuals.dirty:0, (int)(file->buffer?file->buffer->visuals.lines+1:0), out.line+1, out.column+1, (int)view->offset, file->buffer?(int)file->buffer->length:0, file->buffer?(int)file->buffer->visuals.characters:0);
    splitter_status(splitter, &status[0], 0);
  }

  splitter->document.keep_status = 0;
}

void document_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=~0) {
    *pos+=length;
  }
}

void document_insert(struct document* document, file_offset_t offset, const char* text, size_t length) {
  struct document_file* file = document->file;
  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_insert_split(file->buffer, file->type, offset, text, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, NULL);
  length = (file->buffer?file->buffer->length:0)-old_length;
  if (length<=0) {
    return;
  }

  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_expand(&view->selection_end, offset, length);
    document_expand(&view->selection_start, offset, length);
    document_expand(&view->selection_low, offset, length);
    document_expand(&view->selection_high, offset, length);
    document_expand(&view->offset, offset, length);
    
    views = views->next;
  }

  document_undo_empty(file->redos);
  file->modified = 1;
}

void document_insert_buffer(struct document* document, file_offset_t offset, struct range_tree_node* buffer) {
  struct document_file* file = document->file;
  if (!buffer) {
    return;
  }
  
  file_offset_t length = buffer->length;
  
  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file->buffer = range_tree_paste(file->buffer, file->type, buffer, offset);
  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_expand(&view->selection_end, offset, length);
    document_expand(&view->selection_start, offset, length);
    document_expand(&view->selection_low, offset, length);
    document_expand(&view->selection_high, offset, length);
    document_expand(&view->offset, offset, length);
    
    views = views->next;
  }

  document_undo_empty(file->redos);
  file->modified = 1;
}

void document_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=~0) {
    if ((*pos-offset)>=length) {
      *pos-=length;
    } else {
      *pos = offset;
    }
  }
}

void document_delete(struct document* document, file_offset_t offset, file_offset_t length) {
  struct document_file* file = document->file;
  if (!file->buffer || offset>=file->buffer->length) {
    return;
  }
  
  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_DELETE);

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_delete(file->buffer, file->type, offset, length, 0);
  length = old_length-(file->buffer?file->buffer->length:0);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_reduce(&view->selection_end, offset, length);
    document_reduce(&view->selection_start, offset, length);
    document_reduce(&view->selection_low, offset, length);
    document_reduce(&view->selection_high, offset, length);
    document_reduce(&view->offset, offset, length);
    
    views = views->next;
  }
  
  document_undo_empty(file->redos);
  file->modified = 1;
}

int document_delete_selection(struct document* document) {
  struct document_file* file = document->file;
  struct document_view* view = &document->view;
  if (!file->buffer) {
    return 0;
  }
  
  file_offset_t low = view->selection_low;
  if (low<0) {
    low = 0;
  }
  if (low>file->buffer->length) {
    low = file->buffer->length;
  }

  file_offset_t high = view->selection_high;
  if (high<0) {
    high = 0;
  }
  if (high>file->buffer->length) {
    high = file->buffer->length;
  }
  
  if (high-low==0) {
    return 0;
  }

  document_delete(document, low, high-low);
  view->offset = low;
  view->selection_low = ~0;
  view->selection_high = ~0;
  return 1;
}

void document_keypress(struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document* document = &splitter->document;
  struct document_file* file = document->file;
  struct document_view* view = &document->view;

  struct document_render_info_position out;
  struct document_render_info_position in_offset;
  in_offset.type = VISUAL_SEEK_OFFSET;

  struct document_render_info_position in_x_y;
  in_x_y.type = VISUAL_SEEK_X_Y;

  struct document_render_info_position in_line_column;
  in_line_column.type = VISUAL_SEEK_LINE_COLUMN;

  //range_tree_check(document->buffer);
  int seek = 0;

  int reset_selection = 1;
  if (modifier&TIPPSE_KEY_MOD_SHIFT) {
    if (view->selection_start==~0) {
      view->selection_start = view->offset;
    }
    reset_selection = 0;
  }
  
  if ((cp!=TIPPSE_KEY_UP && cp!=TIPPSE_KEY_DOWN && cp!=TIPPSE_KEY_PAGEDOWN && cp!=TIPPSE_KEY_PAGEUP) || (modifier&TIPPSE_KEY_MOD_SHIFT)) {
    in_offset.offset = view->offset;
    document_cursor_position(splitter, &in_offset, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  }

  in_x_y.x = view->cursor_x;
  in_x_y.y = view->cursor_y;
  if (cp==TIPPSE_KEY_UP) {
    in_x_y.y--;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_DOWN) {
    in_x_y.y++;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_LEFT) {
    in_x_y.x--;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (cp==TIPPSE_KEY_RIGHT) {
    in_x_y.x++;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 0);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
    seek = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    int page = ((splitter->height-2)/2)+1;
    in_x_y.y += page;
    view->scroll_y += page;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    int page = ((splitter->height-2)/2)+1;
    in_x_y.y -= page;
    view->scroll_y -= page;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    if (!document_delete_selection(document)) {
      in_x_y.x--;
      file_offset_t start = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
      document_delete(document, start, view->offset-start);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_DELETE) {
    if (!document_delete_selection(document)) {
      in_x_y.x++;
      file_offset_t end = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
      document_delete(document, view->offset, end-view->offset);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_FIRST) {
    in_x_y.x = 0;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_LAST) {
    in_x_y.x = 1000000000;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_HOME) {
    in_x_y.x = 0;
    in_x_y.y = 0;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_END) {
    in_x_y.x = 1000000000;
    in_x_y.y = 1000000000;
    view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
    view->cursor_x = out.x;
    view->cursor_y = out.y;
  } else if (cp==TIPPSE_KEY_UNDO) {
    document_undo_execute(file, view, file->undos, file->redos);
    while (document_undo_execute(file, view, file->undos, file->redos)) {}
    seek = 1;
    return;
  } else if (cp==TIPPSE_KEY_REDO) {
    document_undo_execute(file, view, file->redos, file->undos);
    while (document_undo_execute(file, view, file->redos, file->undos)) {}
    seek = 1;
    return;
  } else if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      in_x_y.x = x-splitter->x-1+view->scroll_x;
      in_x_y.y = y-splitter->y-1+view->scroll_y;
      view->offset = document_cursor_position(splitter, &in_x_y, &out, 0, 1);
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
      view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);      
      view->cursor_x = out.x;
      view->cursor_y = out.y;
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      in_x_y.y -= page;
      view->scroll_y -= page;
      view->offset = document_cursor_position(splitter, &in_x_y, &out, 1, 1);
      view->cursor_x = out.x;
      view->cursor_y = out.y;
    }
    reset_selection = (!(button_old&TIPPSE_MOUSE_LBUTTON) && (button&TIPPSE_MOUSE_LBUTTON))?1:0;
  } else if (cp==TIPPSE_KEY_TAB) {
    document_undo_chain(file);
    if (view->selection_low==~0) {
      char utf8[8];
      file_offset_t size = TAB_WIDTH-(view->cursor_x%TAB_WIDTH);
      file_offset_t spaces;
      for (spaces = 0; spaces<size; spaces++) {
        utf8[spaces] = ' ';
      }
      file_offset_t offset = view->offset;
      document_insert(document, offset, &utf8[0], size);
    } else {
      struct document_render_info_position out_start;
      struct document_render_info_position out_end;

      in_offset.offset = view->selection_low;
      document_cursor_position(splitter, &in_offset, &out_start, 0, 1);
      in_offset.offset = view->selection_high;
      document_cursor_position(splitter, &in_offset, &out_end, 0, 1);
      
      reset_selection = 0;
      while(out_end.line>=out_start.line) {
        in_line_column.column = 0;
        in_line_column.line = out_end.line;
        document_cursor_position(splitter, &in_line_column, &out, 0, 1);

        struct range_tree_node* buffer = out.buffer;
        file_offset_t buffer_pos = out.buffer_pos;
        file_offset_t offset = out.offset;

        char utf8[8];
        file_offset_t size = TAB_WIDTH-(out.column%TAB_WIDTH);
        file_offset_t spaces;
        for (spaces = 0; spaces<size; spaces++) {
          utf8[spaces] = ' ';
        }
        
        const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
        if (*text!='\r' && *text!='\n') {
          document_insert(document, offset, &utf8[0], size);
        }

        out_end.line--;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_UNTAB) {
    document_undo_chain(file);
    if (view->selection_low==~0) {
    } else {
      struct document_render_info_position out_start;
      struct document_render_info_position out_end;

      in_offset.offset = view->selection_low;
      document_cursor_position(splitter, &in_offset, &out_start, 0, 1);
      in_offset.offset = view->selection_high;
      document_cursor_position(splitter, &in_offset, &out_end, 0, 1);
      
      reset_selection = 0;
      while(out_end.line>=out_start.line) {
        in_line_column.column = 0;
        in_line_column.line = out_end.line;
        document_cursor_position(splitter, &in_line_column, &out, 0, 1);

        struct range_tree_node* buffer = out.buffer;
        file_offset_t buffer_pos = out.buffer_pos;
        file_offset_t offset = out.offset;

        file_offset_t length = 0;
        while (buffer) {
          if (buffer_pos>=buffer->length || !buffer->buffer) {
            buffer = range_tree_next(buffer);
            buffer_pos = 0;
            continue;
          }

          const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
          if (*text!='\t' && *text!=' ') {
            break;
          }
          
          length++;
          if (*text=='\t' || length>=TAB_WIDTH) {
            break;
          }

          buffer_pos++;
        }
        
        if (length>0) {
          document_delete(document, offset, length);
        }

        out_end.line--;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_COPY || cp==TIPPSE_KEY_CUT) {
    document_undo_chain(file);
    if (view->selection_low==~0) {
    } else {
      clipboard_set(range_tree_copy(file->buffer, file->type, view->selection_low, view->selection_high-view->selection_low));
      if (cp==TIPPSE_KEY_CUT) {
        document_delete_selection(document);
        reset_selection = 1;
      } else {
        reset_selection = 0;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_PASTE || cp==TIPPSE_KEY_BRACKET_PASTE_START) {
    document_undo_chain(file);
    document_delete_selection(document);
    if (clipboard_get()) {
      document_insert_buffer(document, view->offset, clipboard_get());
    }
    document_undo_chain(file);
    reset_selection = 1;
    seek = 1;
  } else if (cp>=0) {
    document_delete_selection(document);
    char utf8[8];
    size_t size = utf8_encode(cp, &utf8[0], 8)-&utf8[0];
    document_insert(document, view->offset, &utf8[0], size);
    seek = 1;
  }

  if (seek) {
    in_offset.offset = view->offset;
    document_cursor_position(splitter, &in_offset, &out, 1, 1);
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
}

void document_directory(struct document* document) {
  struct document_file* file = document->file;
  DIR* directory = opendir(file->filename);
  if (directory) {
    struct list* files = list_create();
    while (1) {
      struct dirent* entry = readdir(directory);
      if (!entry) {
        break;
      }
      
      list_insert(files, NULL, strdup(&entry->d_name[0]));
    }

    closedir(directory);

    char** sort1 = malloc(sizeof(char*)*files->count);
    char** sort2 = malloc(sizeof(char*)*files->count);
    struct list_node* name = files->first;
    size_t n;
    for (n = 0; n<files->count && name; n++) {
      sort1[n] = (char*)name->object;
      name = name->next;
    }
    
    char** sort = merge_sort(sort1, sort2, files->count);

    file->buffer = range_tree_delete(file->buffer, file->type, 0, file->buffer?file->buffer->length:0, TIPPSE_INSERTER_AUTO);

    for (n = 0; n<files->count; n++) {
      file->buffer = range_tree_insert_split(file->buffer, file->type, file->buffer?file->buffer->length:0, sort[n], strlen(sort[n]), TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      file->buffer = range_tree_insert_split(file->buffer, file->type, file->buffer?file->buffer->length:0, "\n", 1, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      
      free(sort[n]);
    }
    
    free(sort2);
    free(sort1);
  }
}
