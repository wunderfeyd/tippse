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

file_offset_t document_cursor_position(struct document* document, file_offset_t offset_search, int* cur_x, int* cur_y, int seek, int wrap, int cancel, int showall) {
  struct document_file* file = document->file;
  int x = 0;
  int y;
  int xv = 0;
  int sx = *cur_x;
  int sy = *cur_y;
  int next = wrap;

  if (seek) {
    y = sy;
    if (sx<0 && wrap) {
      y--;
      next = 0;
    }

    if (y<0) {
      y = 0;
      sx = 0;
    }

    if (file->buffer) {
      if (y>=file->buffer->lines+1) {
        y = file->buffer->lines;
        //if (wrap) {
          sx = 10000000;
        //}
      }
    }
  } else {
    y = range_tree_find_line_offset(file->buffer, offset_search);
  }

  file_offset_t offset = 0;
  file_offset_t buffer_pos = 0;
  struct range_tree_node* buffer = range_tree_find_line_start(file->buffer, y, x, &buffer_pos, &offset, NULL);
  file_offset_t offset_copy = offset;
  file_offset_t offset_last = offset;
  while (buffer) {
    if (buffer_pos>=buffer->length || !buffer->buffer) {
      buffer = range_tree_next(buffer);
      buffer_pos = 0;
      continue;
    }

    const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
    const char* end = buffer->buffer->buffer+buffer->offset+buffer->length;

    if (offset==offset_search && !seek) {
      break;
    }

    const char* copy = text;
    int cp = -1;
    text = utf8_decode(&cp, text, end-text, 1);

    if (sx<=x && seek && sx>=0 && cp!=0xfeff) {
      if (sx<x && cancel) {
        offset = offset_last;
      }
      break;
    }

    offset_last = offset;
    offset += text-copy;
    buffer_pos += text-copy;

    if (cp=='\n') {
      if (seek && !next) {
        offset = offset_copy;
      }
      break;
    }

    if (cp=='\t') {
      x += xv;
      xv = 0;
      x += TAB_WIDTH-x%TAB_WIDTH;
      offset_copy = offset;
    } else if (cp!='\r' && cp!=0xfeff) {
      x += xv;
      xv = 0;
      x++;
      offset_copy = offset;
    } else if (showall) {
      xv++;
    }
  }

  *cur_x = x+xv;
  *cur_y = y;

  return offset;
}

void document_render(struct screen* screen, struct splitter* splitter, struct range_tree_node* buffer, int* x) {
}

void document_draw(struct screen* screen, struct splitter* splitter) {
  struct document* document = &splitter->document;
  struct document_file* file = document->file;
  struct document_view* view = &document->view;
  
  int cursor_x;
  int cursor_y;
  
  document_cursor_position(document, view->offset, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
  int x = 0;
  int y = 0;
  int y_view = 0;
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
  if (view->scroll_y+splitter->client_height>(file->buffer?file->buffer->lines+1:0)) {
    view->scroll_y = (file->buffer?file->buffer->lines+1:0)-(splitter->client_height);
  }
  if (view->scroll_y<0) {
    view->scroll_y = 0;
  }
  if (view->scroll_x<0) {
    view->scroll_x = 0;
  }

  struct range_tree_node* prev_buffer = NULL;
  struct range_tree_node* buffer = NULL;
  file_offset_t buffer_pos = 0;
  file_offset_t offset = 0;
  int rows = 0;
  int columns = 0;
  int lines = 0;
  int stop = 2;
  for (y=0; y<splitter->client_height; y++) {
    int y_real = y+view->scroll_y;

    int keyword_length = 0;
    int keyword_color = 0;

    int visual_detail;
    file_offset_t offset_new = 0;
    int x_new = 0;
    int y_new = 0;
    int lines_new = 0;
    struct range_tree_node* buffer_new = range_tree_find_row_start(file->buffer, y_real, view->scroll_x, &offset_new, &x_new, &y_new, &lines_new);
    if (buffer_new && prev_buffer!=buffer_new && stop==2) {
      visual_detail = buffer_new->visuals.detail_before;
      offset = offset_new;
      x = x_new;
      y_view = y_new;
      lines = lines_new;
      buffer_pos = 0;
      rows = 0;
      columns = 0;
      buffer = buffer_new;
    }

/*    struct range_tree_node* prev = buffer?range_tree_prev(buffer):NULL;
    char text[1024];
    sprintf(&text[0], "%p - %d - %d - %d %d %s", buffer_new, x_new, y_new, prev?prev->visuals.dirty:0, buffer_new?buffer_new->visuals.detail_before:0, prev_buffer!=buffer_new?"R":" ");
    splitter_drawtext(screen, splitter, 70, y, &text[0], strlen(&text[0]), 2, 0);*/

    stop = 0;
    while (buffer) {
      if (buffer_pos>=buffer->length || !buffer->buffer) {
        if (buffer->visuals.dirty) {
          buffer->visuals.dirty = 0;
          buffer->columns = columns;
          buffer->rows = rows;
          range_tree_update_calc_all(buffer);
        }

        rows = 0;
        columns = 0;

        buffer = range_tree_next(buffer);
        buffer_pos = 0;
        if (buffer) {
          if (visual_detail!=buffer->visuals.detail_before) {
            buffer->visuals.detail_before = visual_detail;
            buffer->visuals.dirty |= VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
            range_tree_update_calc_all(buffer);
          }
        }
        continue;
      }

      if (stop==1) {
        break;
      } else if (stop==2) {
        buffer = NULL;
        break;
      }

      const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
      const char* end = buffer->buffer->buffer+buffer->offset+buffer->length;
      int sel = (offset>=view->selection_low && offset<view->selection_high)?1:0;
      int cp = -1;
      const char* copy = text;
      text = utf8_decode(&cp, text, end-text, 1);
      int show = cp;

      int color = splitter->foreground;
      int background = splitter->background;
      if (!(buffer->inserter&TIPPSE_INSERTER_READONLY)) {
        if (keyword_length==0) {
          int visual_flag = 0;
          (*file->type->mark)(file->type, &visual_detail, buffer, buffer_pos, (y_view==y_real)?1:0, &keyword_length, &visual_flag);

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

      buffer_pos += text-copy;
      offset += text-copy;

      if (((cp!='\r' && cp!=0xfeff) || view->showall) && cp!='\t') {
        if (y_view==y_real) {
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
            splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, color, background);
          } else {
            splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, background, color);
          }
        }
        x++;
        columns++;
      }

      if (cp=='\t') {
        int tabbing = TAB_WIDTH-(x%TAB_WIDTH);
        show = view->showall?0x21a6:' ';
        while (tabbing-->0) {
          if (y_view==y_real) {
            if (!sel) {
              splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, splitter->foreground, background);
            } else {
              splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, background, splitter->foreground);
            }
            show = ' ';
          }
          x++;
          columns++;
        }
      }

      // TODO: uncomment for line wrapping test / results in invalid cursor positions because the repositioning should be done by the renderer too
      if (cp=='\n' /*|| x>=splitter->client_width*/) {
        if (cp=='\n') {
          lines++;
        }

        rows++;
        y_view++;
        x = 0;
        columns = 0;

        if (y_view>y_real) {
          stop = 1;
        }
      }

      // TODO: for very long lines the draw should abort if the buffer can't reach the display area / speed improvement
      if (stop==0 && ((y_view>y_real) || (y_view==y_real && (x-view->scroll_x>=splitter->client_width /*|| x-view->scroll_x<-TREE_BLOCK_LENGTH_MAX*/)))) {
        stop = 2;
      }
    }

    prev_buffer = buffer;
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

  splitter_cursor(screen, splitter, cursor_x-view->scroll_x, cursor_y-view->scroll_y);

  if (!splitter->document.keep_status) {
    char status[1024];
    sprintf(&status[0], "%d/%d:%d - %d/%d byte - %d chars", (int)(file->buffer?file->buffer->lines+1:0), cursor_y+1, cursor_x+1, (int)view->offset, file->buffer?(int)file->buffer->length:0, file->buffer?(int)file->buffer->characters:0);
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
    document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 0, 1, 1, view->showall);
  }

  int cursor_x_copy = view->cursor_x;
  if (cp==TIPPSE_KEY_UP) {
    view->cursor_y--;
    view->offset = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_DOWN) {
    view->cursor_y++;
    view->offset = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_LEFT) {
    view->cursor_x--;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
    seek = 1;
  } else if (cp==TIPPSE_KEY_RIGHT) {
    view->cursor_x++;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 0, view->showall);
    seek = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    int page = ((splitter->height-2)/2)+1;
    view->cursor_y += page;
    view->scroll_y += page;
    view->offset = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    int page = ((splitter->height-2)/2)+1;
    view->cursor_y -= page;
    view->scroll_y -= page;
    view->offset = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    if (!document_delete_selection(document)) {
      cursor_x_copy--;
      file_offset_t start = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
      document_delete(document, start, view->offset-start);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_DELETE) {
    if (!document_delete_selection(document)) {
      cursor_x_copy++;
      file_offset_t end = document_cursor_position(document, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
      document_delete(document, view->offset, end-view->offset);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_FIRST) {
    view->cursor_x = 0;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_LAST) {
    view->cursor_x = 1000000000;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_HOME) {
    view->cursor_x = 0;
    view->cursor_y = 0;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_END) {
    view->cursor_x = 1000000000;
    view->cursor_y = 1000000000;
    view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
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
      view->cursor_x = x-splitter->x-1+view->scroll_x;
      view->cursor_y = y-splitter->y-1+view->scroll_y;
      view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
      if (view->selection_start==~0) {
        view->selection_start = view->offset;
      }
      view->selection_end = view->offset;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      int page = ((splitter->height-2)/3)+1;
      view->cursor_y += page;
      view->scroll_y += page;
      view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);      
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      view->cursor_y -= page;
      view->scroll_y -= page;
      view->offset = document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
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
      int cursor_x = view->cursor_x;
      int cursor_y = view->cursor_y;
      int cursor_y_start;
      document_cursor_position(document, view->selection_low, &cursor_x, &cursor_y_start, 0, 1, 1, view->showall);
      document_cursor_position(document, view->selection_high-1, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
      
      cursor_x = 0;
      reset_selection = 0;
      while(cursor_y>=cursor_y_start) {
        file_offset_t buffer_pos = 0;
        file_offset_t offset = 0;
        struct range_tree_node* buffer = range_tree_find_line_start(file->buffer, cursor_y, cursor_x, &buffer_pos, &offset, NULL);
        
        char utf8[8];
        file_offset_t size = TAB_WIDTH-(cursor_x%TAB_WIDTH);
        file_offset_t spaces;
        for (spaces = 0; spaces<size; spaces++) {
          utf8[spaces] = ' ';
        }
        
        const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
        if (*text!='\r' && *text!='\n') {
          document_insert(document, offset, &utf8[0], size);
        }
        cursor_y--;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_UNTAB) {
    document_undo_chain(file);
    if (view->selection_low==~0) {
    } else {
      int cursor_x = view->cursor_x;
      int cursor_y = view->cursor_y;
      int cursor_y_start;
      document_cursor_position(document, view->selection_low, &cursor_x, &cursor_y_start, 0, 1, 1, view->showall);
      document_cursor_position(document, view->selection_high-1, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
      
      cursor_x = 0;
      reset_selection = 0;
      while(cursor_y>=cursor_y_start) {
        file_offset_t buffer_pos = 0;
        file_offset_t offset = 0;
        struct range_tree_node* buffer = range_tree_find_line_start(file->buffer, cursor_y, cursor_x, &buffer_pos, &offset, NULL);
        
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
        cursor_y--;
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
    document_cursor_position(document, view->offset, &view->cursor_x, &view->cursor_y, 0, 1, 1, view->showall);
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
