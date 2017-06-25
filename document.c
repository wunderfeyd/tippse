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

void document_search(struct splitter* splitter, struct document_file* file, struct document_view* view, struct range_tree_node* text, file_offset_t length, int forward) {
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

file_offset_t document_cursor_position(struct document_file* file, file_offset_t offset_search, int* cur_x, int* cur_y, int seek, int wrap, int cancel, int showall) {
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

struct keywords {
  const char* text;
  size_t length;
  int color;
};

/* TODO: Replace with a trie */
int document_keyword(struct range_tree_node* buffer, file_offset_t buffer_pos, const char* keyword, size_t length, int whitespace) {
  while (buffer) {
    if (buffer_pos>=buffer->length || !buffer->buffer) {
      buffer = range_tree_next(buffer);
      buffer_pos = 0;
      continue;
    }

    const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
    file_offset_t max = length;
    if (buffer->length-buffer_pos<max) {
      max = buffer->length-buffer_pos;
    }

    if (length==0) {
      if (whitespace || ((*text<'a' || *text>'z') && (*text<'A' || *text>'Z') && (*text<'0' || *text>'9') && *text!='_')) {
        return 1;
      } else {
        return 0;
      }
    }

    if (strncmp(text, keyword, max)!=0) {
      return 0;
    }

    buffer_pos += max;
    keyword += max;
    length-= max;
  }

  return 0;
}

struct keywords keywords[] = {
  {"int", 3, 120},
  {"unsigned", 8, 120},
  {"signed", 6, 120},
  {"char", 4, 120},
  {"short", 5, 120},
  {"long", 4, 120},
  {"void", 4, 120},
  {"bool", 4, 120},
  {"size_t", 6, 120},
  {"int8_t", 6, 120},
  {"uint8_t", 7, 120},
  {"int16_t", 7, 120},
  {"uint16_t", 8, 120},
  {"int32_t", 7, 120},
  {"uint32_t", 8, 120},
  {"int64_t", 7, 120},
  {"uint64_t", 8, 120},
  {"nullptr", 7, 120},
  {"const", 5, 120},
  {"struct", 6, 120},
  {"class", 5, 120},
  {"public", 6, 120},
  {"private", 7, 120},
  {"protected", 9, 120},
  {"virtual", 7, 120},
  {"template", 8, 120},
  {"static", 6, 120},
  {"inline", 6, 120},
  {"for", 3, 210},
  {"do", 2, 210},
  {"while", 5, 210},
  {"if", 2, 210},
  {"else", 4, 210},
  {"return", 6, 210},
  {"break", 5, 210},
  {"continue", 8, 210},
  {"using", 5, 210},
  {"namespace", 9, 210},
  {"new", 3, 210},
  {"delete", 6, 210},
  {NULL, 0, 0}
};

void document_draw(struct screen* screen, struct splitter* splitter) {
  struct document_file* file = splitter->document.file;
  struct document_view* view = &splitter->document.view;
  
  int cursor_x;
  int cursor_y;
  
  document_cursor_position(file, view->offset, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
  int x = 0;
  int y = 0;
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

  for (y=0; y<splitter->client_height; y++) {
    x = 0;
    int quote = 0;
    int escape = 0;
    int comment = 0;
    int cp_last = ' ';

    file_offset_t buffer_pos = 0;
    file_offset_t offset = 0;
    size_t keyword_length = 0;
    int keyword_color = 0;
    struct range_tree_node* buffer = range_tree_find_line_start(file->buffer, y+view->scroll_y, view->scroll_x, &buffer_pos, &offset, &x);
    while (buffer) {
      if (buffer_pos>=buffer->length || !buffer->buffer) {
        if (buffer->column==-1) {
          buffer->column = x;
        }
        buffer = range_tree_next(buffer);
        buffer_pos = 0;
        continue;
      }

      const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
      const char* end = buffer->buffer->buffer+buffer->offset+buffer->length;
      int sel = (offset>=view->selection_low && offset<view->selection_high)?1:0;
      int cp = -1;
      const char* copy = text;
      text = utf8_decode(&cp, text, end-text, 1);
      int show = cp;

      int quote_old = quote;
      int escape_old = escape;
      escape = 0;
      int color = splitter->foreground;
      int background = splitter->background;
      if (!(buffer->inserter&TIPPSE_INSERTER_READONLY)) {
        if (quote==0 && comment==0 && document_keyword(buffer, buffer_pos, "//", 2, 1)) {
          comment = 1;
        }
        
        if (comment==0) {
          if ((cp=='\'' || cp=='\"') && escape_old==0 && (quote==0 || quote==cp)) {
            quote = (quote==0)?cp:0;
            color = 226;
          } else if (cp=='\\' && quote!=0 && escape_old==0) {
            escape = 1;
          }

          if (quote!=0) {
            color = 226;
          }

          if (keyword_length==0 && quote_old==0 && quote==0 && (cp_last<'a' || cp_last>'z') && (cp_last<'A' || cp_last>'Z') && (cp_last<'0' || cp_last>'9') && cp_last!='_' && ((cp>='a' && cp<='z') || (cp>='A' && cp<='Z') || (cp>='0' && cp<='9') || cp=='_')) {
            size_t keyword;
            for (keyword=0; keywords[keyword].text; keyword++) {
              if (document_keyword(buffer, buffer_pos, keywords[keyword].text, keywords[keyword].length, 0)) {
                keyword_length = keywords[keyword].length;
                keyword_color = keywords[keyword].color;
              }
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

      cp_last = cp;
      if (((cp!='\r' && cp!=0xfeff) || view->showall) && cp!='\t') {
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
        x++;
      }

      if (cp=='\t') {
        int tabbing = TAB_WIDTH-(x%TAB_WIDTH);
        show = view->showall?0x21a6:' ';
        while (tabbing-->0) {
          if (!sel) {
            splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, splitter->foreground, background);
          } else {
            splitter_drawchar(screen, splitter, x-view->scroll_x, y, show, background, splitter->foreground);
          }
          show = ' ';
          x++;
        }
      }

      if (cp=='\n' || x-view->scroll_x>=splitter->client_width) {
        break;
      }
      buffer_pos += text-copy;
      offset += text-copy;
    }
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

void document_insert(struct document_file* file, file_offset_t offset, const char* text, size_t length) {
  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_insert_split(file->buffer, offset, text, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, NULL);
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

void document_insert_buffer(struct document_file* file, file_offset_t offset, struct range_tree_node* buffer) {
  if (!buffer) {
    return;
  }
  
  file_offset_t length = buffer->length;
  
  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file->buffer = range_tree_paste(file->buffer, buffer, offset);
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

void document_delete(struct document_file* file, file_offset_t offset, file_offset_t length) {
  if (!file->buffer || offset>=file->buffer->length) {
    return;
  }
  
  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_DELETE);

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_delete(file->buffer, offset, length, 0);
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

int document_delete_selection(struct document_file* file, struct document_view* view) {
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

  document_delete(file, low, high-low);
  view->offset = low;
  view->selection_low = ~0;
  view->selection_high = ~0;
  return 1;
}

void document_keypress(struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document_file* file = splitter->document.file;
  struct document_view* view = &splitter->document.view;

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
    document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 0, 1, 1, view->showall);
  }

  int cursor_x_copy = view->cursor_x;
  if (cp==TIPPSE_KEY_UP) {
    view->cursor_y--;
    view->offset = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_DOWN) {
    view->cursor_y++;
    view->offset = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_LEFT) {
    view->cursor_x--;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
    seek = 1;
  } else if (cp==TIPPSE_KEY_RIGHT) {
    view->cursor_x++;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 0, view->showall);
    seek = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    int page = ((splitter->height-2)/2)+1;
    view->cursor_y += page;
    view->scroll_y += page;
    view->offset = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    int page = ((splitter->height-2)/2)+1;
    view->cursor_y -= page;
    view->scroll_y -= page;
    view->offset = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    if (!document_delete_selection(file, view)) {
      cursor_x_copy--;
      file_offset_t start = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
      document_delete(file, start, view->offset-start);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_DELETE) {
    if (!document_delete_selection(file, view)) {
      cursor_x_copy++;
      file_offset_t end = document_cursor_position(file, view->offset, &cursor_x_copy, &view->cursor_y, 1, 1, 1, view->showall);
      document_delete(file, view->offset, end-view->offset);
    }
    seek = 1;
  } else if (cp==TIPPSE_KEY_FIRST) {
    view->cursor_x = 0;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_LAST) {
    view->cursor_x = 1000000000;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
  } else if (cp==TIPPSE_KEY_HOME) {
    view->cursor_x = 0;
    view->cursor_y = 0;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
  } else if (cp==TIPPSE_KEY_END) {
    view->cursor_x = 1000000000;
    view->cursor_y = 1000000000;
    view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
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
      view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 0, 1, view->showall);
      if (view->selection_start==~0) {
        view->selection_start = view->offset;
      }
      view->selection_end = view->offset;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      int page = ((splitter->height-2)/3)+1;
      view->cursor_y += page;
      view->scroll_y += page;
      view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);      
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      int page = ((splitter->height-2)/3)+1;
      view->cursor_y -= page;
      view->scroll_y -= page;
      view->offset = document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 1, 1, 1, view->showall);
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
      document_insert(file, offset, &utf8[0], size);
    } else {
      int cursor_x = view->cursor_x;
      int cursor_y = view->cursor_y;
      int cursor_y_start;
      document_cursor_position(file, view->selection_low, &cursor_x, &cursor_y_start, 0, 1, 1, view->showall);
      document_cursor_position(file, view->selection_high-1, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
      
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
          document_insert(file, offset, &utf8[0], size);
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
      document_cursor_position(file, view->selection_low, &cursor_x, &cursor_y_start, 0, 1, 1, view->showall);
      document_cursor_position(file, view->selection_high-1, &cursor_x, &cursor_y, 0, 1, 1, view->showall);
      
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
          document_delete(file, offset, length);
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
      clipboard_set(range_tree_copy(file->buffer, view->selection_low, view->selection_high-view->selection_low));
      if (cp==TIPPSE_KEY_CUT) {
        document_delete_selection(file, view);
        reset_selection = 1;
      } else {
        reset_selection = 0;
      }
    }
    document_undo_chain(file);
    seek = 1;
  } else if (cp==TIPPSE_KEY_PASTE || cp==TIPPSE_KEY_BRACKET_PASTE_START) {
    document_undo_chain(file);
    document_delete_selection(file, view);
    if (clipboard_get()) {
      document_insert_buffer(file, view->offset, clipboard_get());
    }
    document_undo_chain(file);
    reset_selection = 1;
    seek = 1;
  } else if (cp>=0) {
    document_delete_selection(file, view);
    char utf8[8];
    size_t size = utf8_encode(cp, &utf8[0], 8)-&utf8[0];
    document_insert(file, view->offset, &utf8[0], size);
    seek = 1;
  }

  if (seek) {
    document_cursor_position(file, view->offset, &view->cursor_x, &view->cursor_y, 0, 1, 1, view->showall);
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

void document_directory(struct document_file* file) {
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
    for (size_t n = 0; n<files->count && name; n++) {
      sort1[n] = (char*)name->object;
      name = name->next;
    }
    
    char** sort = merge_sort(sort1, sort2, files->count);

    file->buffer = range_tree_delete(file->buffer, 0, file->buffer?file->buffer->length:0, TIPPSE_INSERTER_AUTO);

    for (size_t n = 0; n<files->count; n++) {
      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, sort[n], strlen(sort[n]), TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, "\n", 1, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      
      free(sort[n]);
    }
    
    free(sort2);
    free(sort1);
  }
}
