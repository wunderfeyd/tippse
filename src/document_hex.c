/* Tippse - Document Hex View - Cursor and display operations for hex mode */

#include "document_hex.h"

struct document* document_hex_create(void) {
  struct document_hex* document = (struct document_hex*)malloc(sizeof(struct document_hex));
  document->cp_first = 0;
  document->vtbl.reset = document_hex_reset;
  document->vtbl.draw = document_hex_draw;
  document->vtbl.keypress = document_hex_keypress;
  document->vtbl.incremental_update = document_hex_incremental_update;

  return (struct document*)document;
}

void document_hex_destroy(struct document* base) {
  free(base);
}

// Called after a new document was assigned
void document_hex_reset(struct document* base, struct splitter* splitter) {
}

// Find next dirty pages and rerender them (background task)
int document_hex_incremental_update(struct document* base, struct splitter* splitter) {
  return 0;
}

// Draw entire visible screen space
void document_hex_draw(struct document* base, struct screen* screen, struct splitter* splitter) {
  struct document_hex* document = (struct document_hex*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!screen) {
    return;
  }

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  view->cursor_x = view->offset%16;
  view->cursor_y = (position_t)(view->offset/16);
  if (view->cursor_y>=view->scroll_y+splitter->client_height) {
    view->scroll_y = view->cursor_y-splitter->client_height+1;
  }
  view->scroll_y_max = (int)((file_size+16)/16);
  if (view->cursor_y<view->scroll_y) {
    view->scroll_y = view->cursor_y;
  }
  int max_height = (int)((file_size+16)/16)-splitter->client_height;
  if (view->scroll_y>max_height) {
    view->scroll_y = max_height;
  }
  if (view->scroll_y<0) {
    view->scroll_y = 0;
  }

  if (view->scroll_x_old!=view->scroll_x || view->scroll_y_old!=view->scroll_y) {
    view->scroll_x_old = view->scroll_x;
    view->scroll_y_old = view->scroll_y;
    view->show_scrollbar = 1;
  }

  file_offset_t offset = (file_offset_t)view->scroll_y*16;
  file_offset_t displacement;
  struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
  struct encoding_stream byte_stream;
  encoding_stream_from_page(&byte_stream, buffer, displacement);

  struct encoding_stream text_stream;
  encoding_stream_from_page(&text_stream, buffer, displacement);
  struct encoding_cache text_cache;
  encoding_cache_clear(&text_cache, file->encoding, &text_stream);

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

  char status[1024];
  sprintf(&status[0], "%lu/%lu bytes - %08lx - Hex %s", (unsigned long)view->offset, (unsigned long)file_size, (unsigned long)view->offset, (*file->encoding->name)());
  splitter_status(splitter, &status[0], 0);

  size_t char_size = 1;
  int x, y;
  for (y = 0; y<splitter->client_height; y++) {
    uint8_t data[16];
    struct document_hex_char chars[16];
    size_t data_size = file_size-offset>16?16:file_size-offset;
    for (x = 0; x<(int)data_size; x++) {
      char_size--;
      if (char_size==0) {
        size_t advance = 1;
        chars[x].length = unicode_read_combined_sequence(&text_cache, 0, chars[x].codepoints, 8, &advance, &char_size);
        encoding_cache_advance(&text_cache, advance);
      } else {
        chars[x].codepoints[0] = 0;
        chars[x].length = 1;
      }

      data[x] = encoding_stream_peek(&byte_stream, 0);
      encoding_stream_forward(&byte_stream, 1);
    }

    document_hex_render(base, screen, splitter, offset, y, data, data_size, chars);
    offset += data_size;
    if (offset>=file_size) break;
  }
  if (file_size && file_size%16==0) document_hex_render(base, screen, splitter, offset, y+1, NULL, 0, NULL);
  if (view->selection_low!=view->selection_high) {
    splitter_cursor(screen, splitter, -1, -1);
  } else {
    splitter_cursor(screen, splitter, (int)(10+(3*view->cursor_x)+(document->cp_first!=0)), (int)(view->cursor_y-view->scroll_y));
  }
  splitter_scrollbar(screen, splitter);
}

// Render one line of data
void document_hex_render(struct document* base, struct screen* screen, struct splitter* splitter, file_offset_t offset, position_t y, const uint8_t* data, size_t data_size, struct document_hex_char* chars) {
  struct document_hex* document = (struct document_hex*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  int foreground = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT];
  int background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  int selection = file->defaults.colors[VISUAL_FLAG_COLOR_SELECTION];
  int linenumber = file->defaults.colors[VISUAL_FLAG_COLOR_LINENUMBER];
  int x = 0;
  char line[1024];
  sprintf(line, "%08lx", (unsigned long)offset);
  splitter_drawtext(screen, splitter, x, (int)y, line, 8, linenumber, background);
  x = 10;
  size_t data_pos;
  for (data_pos = 0; data_pos<data_size; data_pos++) {
    sprintf(line, "%02x ", data[data_pos]);
    if (offset+data_pos<view->selection_low || offset+data_pos>=view->selection_high) {
      splitter_drawtext(screen, splitter, x, (int)y, line, 2, foreground, background);
    } else {
      splitter_drawtext(screen, splitter, x, (int)y, line, data_pos==15?2:3, foreground, selection);
    }
    x += 3;
  }
  if (document->cp_first!=0 && y==view->cursor_y-view->scroll_y) {
    splitter_drawchar(screen, splitter, (int)(10+(3*view->cursor_x)), (int)y, &document->cp_first, 1, foreground, background);
  }

  x = 59;
  for (data_pos = 0; data_pos<data_size; data_pos++) {
    for (size_t n = 0; n<chars[data_pos].length; n++) {
      chars[data_pos].visuals[n] = (file->encoding->visual)(file->encoding, chars[data_pos].codepoints[n]);
    }
    document_hex_convert(&chars[data_pos], view->show_invisibles, '.');
    if (offset+data_pos<view->selection_low || offset+data_pos>=view->selection_high) {
      splitter_drawchar(screen, splitter, x, (int)y, chars[data_pos].visuals, chars[data_pos].length, foreground, background);
    } else {
      splitter_drawchar(screen, splitter, x, (int)y, chars[data_pos].visuals, chars[data_pos].length, foreground, selection);
    }
    x++;
  }
}

// Handle key press
void document_hex_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document_hex* document = (struct document_hex*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  file_offset_t offset_old = view->offset;
  int selection_keep = 0;

  if (cp==TIPPSE_KEY_UP) {
    view->offset-=16;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_DOWN) {
    view->offset+=16;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_LEFT) {
    view->offset--;
  } else if (cp==TIPPSE_KEY_RIGHT) {
    view->offset++;
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    view->offset -= (file_offset_t)(splitter->client_height*16);
    view->scroll_y -= splitter->client_height;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    view->offset += (file_offset_t)(splitter->client_height*16);
    view->scroll_y += splitter->client_height;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_FIRST) {
    view->offset -= view->offset%16;
  } else if (cp==TIPPSE_KEY_LAST) {
    view->offset += 15-(view->offset%16);
  } else if (cp==TIPPSE_KEY_HOME) {
    view->offset = 0;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_END) {
    view->offset = file_size;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      document_hex_cursor_from_point(base, splitter, x, y, &view->offset);
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      view->offset -= (file_offset_t)((splitter->client_height/3)*16);
      view->scroll_y -= splitter->client_height/3;
      view->show_scrollbar = 1;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      view->offset += (file_offset_t)((splitter->client_height/3)*16);
      view->scroll_y += splitter->client_height/3;
      view->show_scrollbar = 1;
    }
  } else if (cp==TIPPSE_KEY_SELECT_ALL) {
    view->selection_start = 0;
    view->selection_end = file_size;
    selection_keep = 1;
  } else if (cp==TIPPSE_KEY_COPY || cp==TIPPSE_KEY_CUT) {
    if (view->selection_low!=FILE_OFFSET_T_MAX) {
      clipboard_set(range_tree_copy(file->buffer, view->selection_low, view->selection_high-view->selection_low), 1);
      if (cp==TIPPSE_KEY_CUT) {
        document_undo_chain(file, file->undos);
        document_file_delete_selection(splitter->file, splitter->view);
      } else {
        selection_keep = 1;
      }
    }
  } else if (cp==TIPPSE_KEY_PASTE) {
    document_undo_chain(file, file->undos);
    document_file_delete_selection(splitter->file, splitter->view);

    struct range_tree_node* buffer = clipboard_get();
    if (buffer) {
      document_file_insert_buffer(splitter->file, view->offset, buffer);
    }

    document_undo_chain(file, file->undos);
  } else if (cp==TIPPSE_KEY_UNDO) {
    document_undo_execute_chain(file, view, file->undos, file->redos, 0);
  } else if (cp==TIPPSE_KEY_REDO) {
    document_undo_execute_chain(file, view, file->redos, file->undos, 1);
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    if (view->selection_low!=FILE_OFFSET_T_MAX || document->cp_first!=0) {
      document_file_delete_selection(file, view);
    } else {
      view->offset--;
      document_file_delete(file, view->offset, 1);
    }
  } else if (cp==TIPPSE_KEY_DELETE) {
    if (view->selection_low!=FILE_OFFSET_T_MAX || document->cp_first!=0) {
      document_file_delete_selection(file, view);
    } else {
      document_file_delete(file, view->offset, 1);
    }
  } else if (cp==TIPPSE_KEY_RETURN) {
    uint8_t text = file->binary?0:32;
    document_file_delete_selection(file, view);
    document_file_insert(file, view->offset, &text, 1);
    view->offset--;
  }

  if ((cp>='0' && cp<='9') || (cp>='a' && cp<='f') || (cp>='A' && cp<='F')){
    if (document->cp_first!=0) {
      uint8_t text = (uint8_t)((document_hex_value(document->cp_first)<<4)+document_hex_value(cp));
      document_file_insert(file, view->offset, &text, 1);
      document_file_delete(file, view->offset, 1);
      document->cp_first = 0;
    } else {
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        uint8_t text = file->binary?0:32;
        document_file_delete_selection(file, view);
        document_file_insert(file, view->offset, &text, 1);
        view->offset--;
      }
      document->cp_first = cp;
    }
  } else if (cp!=TIPPSE_KEY_TIPPSE_MOUSE_INPUT || button!=0) {
    document->cp_first = 0;
  }

  file_size = file->buffer?file->buffer->length:0;
  if (view->offset>((file_offset_t)1<<(sizeof(file_offset_t)*8-1))) {
    view->offset = 0;
  } else if (view->offset>file_size) {
    view->offset = file_size;
  }

  int selection_reset = 0;
  if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      if (!(button_old&TIPPSE_MOUSE_LBUTTON) && !(modifier&TIPPSE_KEY_MOD_SHIFT)) view->selection_start = FILE_OFFSET_T_MAX;
      if (view->selection_start==FILE_OFFSET_T_MAX) view->selection_start = view->offset;
      view->selection_end = view->offset;
    }
  } else {
    if (modifier&TIPPSE_KEY_MOD_SHIFT) {
      if (view->selection_start==FILE_OFFSET_T_MAX) view->selection_start = offset_old;
      view->selection_end = view->offset;
    } else {
      selection_reset = selection_keep ? 0 : 1;
    }
  }
  if (selection_reset) {
    view->selection_start = FILE_OFFSET_T_MAX;
    view->selection_end = FILE_OFFSET_T_MAX;
  }
  if (view->selection_start<view->selection_end) {
    view->selection_low = view->selection_start;
    view->selection_high = view->selection_end;
  } else {
    view->selection_low = view->selection_end;
    view->selection_high = view->selection_start;
  }
}

// Return cursor position from point
void document_hex_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, file_offset_t* offset) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  x -= splitter->x;
  y -= splitter->y;
  file_offset_t file_size = file->buffer?file->buffer->length:0;
  if (y<0) *offset = 0;
  if (y>=0 && y<splitter->client_height) {
    if (x>=8 && x<10) *offset = (file_offset_t)((view->scroll_y+y)*16);
    if (x>=10 && x<58) *offset = (file_offset_t)(((view->scroll_y+y)*16)+((x-10)/3));
    if (x>=58 && x<59) *offset = (file_offset_t)(((view->scroll_y+y)*16)+16);
    if (x>=59 && x<75) *offset = (file_offset_t)(((view->scroll_y+y)*16)+x-59);
    if (x>=76) *offset = (file_offset_t)(((view->scroll_y+y)*16)+16);
    if (*offset>file_size) *offset = file_size;
  }
}

// Return value from hex character
uint8_t document_hex_value(int cp) {
  uint8_t value = 0;
  if (cp>='0' && cp<='9') {
    value = (uint8_t)cp-'0';
  } else if (cp>='a' && cp<='f') {
    value = (uint8_t)cp-'a'+10;
  } else if (cp>='A' && cp<='F') {
    value = (uint8_t)cp-'A'+10;
  }
  return value;
}

// Return value from hex string
uint8_t document_hex_value_from_string(const char* text, size_t length) {
  uint8_t value = 0;
  for (size_t pos=0; pos<length; pos++) {
    char cp = *(text+pos);
    if (cp>='0' && cp<='9') {
      value = value*16+(uint8_t)cp-'0';
    } else if (cp>='a' && cp<='f') {
      value = value*16+(uint8_t)cp-'a'+10;
    } else if (cp>='A' && cp<='F') {
      value = value*16+(uint8_t)cp-'A'+10;
    } else {
      break;
    }
  }
  return value;
}

// Convert invisible characters
void document_hex_convert(struct document_hex_char* cps, int show_invisibles, int cp_default) {
  int cp = cps->codepoints[0];
  int show = -1;
  if (cp=='\n') {
    show = show_invisibles?0x00ac:cp_default;
  } else if (cp=='\r') {
    show = show_invisibles?0x00ac:cp_default;
  } else if (cp=='\t') {
    show = show_invisibles?0x00bb:cp_default;
  } else if (cp==' ') {
    show = show_invisibles?0x22c5:' ';
  } else if (cp<32 || cps->visuals[0]==0xfffd) {
    show = cp_default;
  }

  if (show!=-1) {
    cps->visuals[0] = show;
    cps->length = 1;
  }
}