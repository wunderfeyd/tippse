// Tippse - Document Hex View - Cursor and display operations for hex mode

#include "document_hex.h"

// Create document
struct document* document_hex_create(void) {
  struct document_hex* document = (struct document_hex*)malloc(sizeof(struct document_hex));
  document->cp_first = 0;
  document->mode = DOCUMENT_HEX_MODE_BYTE;
  document->vtbl.reset = document_hex_reset;
  document->vtbl.draw = document_hex_draw;
  document->vtbl.keypress = document_hex_keypress;
  document->vtbl.incremental_update = document_hex_incremental_update;

  return (struct document*)document;
}

// Destroy document
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

  int foreground = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT];
  int background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  int selectionx = file->defaults.colors[VISUAL_FLAG_COLOR_SELECTION];
  int bookmarkx = file->defaults.colors[VISUAL_FLAG_COLOR_BOOKMARK];

  file_offset_t file_size = range_tree_length(file->buffer);
  view->cursor_x = (position_t)(view->offset%16);
  view->cursor_y = (position_t)(view->offset/16);
  if (view->cursor_y>=view->scroll_y+splitter->client_height) {
    view->scroll_y = view->cursor_y-splitter->client_height+1;
  }
  view->scroll_y_max = (position_t)(file_size/16)+1;
  if (view->cursor_y<view->scroll_y) {
    view->scroll_y = view->cursor_y;
  }
  position_t max_height = view->scroll_y_max-splitter->client_height;
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
  struct stream byte_stream;
  stream_from_page(&byte_stream, buffer, displacement);

  struct stream text_stream;
  stream_from_page(&text_stream, buffer, displacement);
  struct encoding_cache text_cache;
  encoding_cache_clear(&text_cache, file->encoding, &text_stream);

  file_offset_t selection_displacement;
  struct range_tree_node* selection = range_tree_find_offset(view->selection, offset, &selection_displacement);

  file_offset_t bookmark_displacement;
  struct range_tree_node* bookmark = range_tree_find_offset(file->bookmarks, offset, &bookmark_displacement);

  size_t name_length = strlen(file->filename);
  int modified = document_undo_modified(file);
  char* title = malloc((name_length+(size_t)modified*2+1)*sizeof(char));
  memcpy(title, file->filename, name_length);
  if (modified) {
    memcpy(title+name_length, " *\0", 3);
  } else {
    title[name_length] = '\0';
  }
  splitter_name(splitter, title);
  free(title);

  char status[1024];
  sprintf(&status[0], "%llu/%llu bytes - %llx - Hex %s", (long long unsigned int)view->offset, (long long unsigned int)file_size, (long long unsigned int)view->offset, (*file->encoding->name)());
  splitter_status(splitter, &status[0]);

  size_t char_size = 1;
  for (int y = 0; y<splitter->client_height; y++) {
    size_t data_size = 16;
    char line[1024];
    int size = sprintf(line, "%llx", (long long unsigned int)offset);

    if (view->address_width>0) {
      int x = 0;
      int start = size-(view->address_width-1);
      if (start<=0) {
        start = 0;
        x = (view->address_width-1)-size;
      } else {
        size = view->address_width+1;
        start--;
      }

      int marked = range_tree_marked(file->bookmarks, offset, data_size, TIPPSE_INSERTER_MARK);

      splitter_drawtext(splitter, screen, x, (int)y, line+start, (size_t)size, file->defaults.colors[marked?VISUAL_FLAG_COLOR_BOOKMARK:VISUAL_FLAG_COLOR_LINENUMBER], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
    }

    for (size_t delta = 0; delta<data_size; delta++) {
      int x_bytes = view->address_width+(int)(delta*3);
      int x_characters = view->address_width+(16*3)+(int)delta;

      if (offset<file_size) {
        size_t length;
        codepoint_t codepoints[8];
        char_size--;
        if (char_size==0) {
          size_t advance;
          length = unicode_read_combined_sequence(&text_cache, 0, &codepoints[0], 8, &advance, &char_size);
          encoding_cache_advance(&text_cache, advance);
        } else {
          codepoints[0] = 0;
          length = 1;
        }

        uint8_t byte = stream_read_forward(&byte_stream);
        selection_displacement++;
        while (selection && selection_displacement>selection->length) {
          selection_displacement -= selection->length;
          selection = selection->next;
        }
        int selected = (selection && (selection->inserter&TIPPSE_INSERTER_MARK))?1:0;

        bookmark_displacement++;
        while (bookmark && bookmark_displacement>bookmark->length) {
          bookmark_displacement -= bookmark->length;
          bookmark = bookmark->next;
        }
        int marked = (bookmark && (bookmark->inserter&TIPPSE_INSERTER_MARK))?1:0;

        int size = sprintf(line, (delta!=data_size)?"%02x ":"%02x", byte);
        if (!selected) {
          splitter_drawtext(splitter, screen, x_bytes, y, line, (size_t)size, marked?bookmarkx:foreground, background);
        } else {
          splitter_drawtext(splitter, screen, x_bytes, y, line, (size_t)size, foreground, selectionx);
        }

        codepoint_t visuals[8];
        for (size_t n = 0; n<length; n++) {
          visuals[n] = (file->encoding->visual)(file->encoding, codepoints[n]);
        }
        document_hex_convert(&codepoints[0], &length, &visuals[0], view->show_invisibles, '.');

        if (!selected) {
          splitter_drawchar(splitter, screen, x_characters, y, visuals, length, marked?bookmarkx:foreground, background);
        } else {
          splitter_drawchar(splitter, screen, x_characters, y, visuals, length, foreground, selectionx);
        }
      }

      if (view->offset==offset) {
        if (document->cp_first) {
          int size = sprintf(line, "%01x", document_hex_value(document->cp_first));
          splitter_drawtext(splitter, screen, x_bytes, y, line, (size_t)size, foreground, background);
        }

        if (document_view_select_active(view)) {
          splitter_cursor(splitter, screen, -1, -1);
        } else {
          if (document->mode==DOCUMENT_HEX_MODE_BYTE) {
            splitter_cursor(splitter, screen, x_bytes+(document->cp_first!=0), y);
          } else {
            splitter_cursor(splitter, screen, x_characters, y);
          }
        }
      }

      offset++;
    }

    if (offset>file_size) break;
  }

  stream_destroy(&byte_stream);
  stream_destroy(&text_stream);

  splitter_scrollbar(splitter, screen);
}

// Handle key press
void document_hex_keypress(struct document* base, struct splitter* splitter, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  struct document_hex* document = (struct document_hex*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = range_tree_length(file->buffer);
  file_offset_t offset_old = view->offset;
  int selection_keep = 0;
  file_offset_t selection_low;
  file_offset_t selection_high;
  document_view_select_next(view, 0, &selection_low, &selection_high);

  if (command==TIPPSE_CMD_UP || command==TIPPSE_CMD_SELECT_UP) {
    view->offset-=16;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_DOWN || command==TIPPSE_CMD_SELECT_DOWN) {
    view->offset+=16;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_LEFT || command==TIPPSE_CMD_SELECT_LEFT) {
    view->offset--;
  } else if (command==TIPPSE_CMD_RIGHT || command==TIPPSE_CMD_SELECT_RIGHT) {
    view->offset++;
  } else if (command==TIPPSE_CMD_PAGEUP || command==TIPPSE_CMD_SELECT_PAGEUP) {
    view->offset -= (file_offset_t)(splitter->client_height*16);
    view->scroll_y -= splitter->client_height;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_PAGEDOWN || command==TIPPSE_CMD_SELECT_PAGEDOWN) {
    view->offset += (file_offset_t)(splitter->client_height*16);
    view->scroll_y += splitter->client_height;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_FIRST || command==TIPPSE_CMD_SELECT_FIRST) {
    view->offset -= view->offset%16;
  } else if (command==TIPPSE_CMD_LAST || command==TIPPSE_CMD_SELECT_LAST) {
    view->offset += 15-(view->offset%16);
  } else if (command==TIPPSE_CMD_HOME || command==TIPPSE_CMD_SELECT_HOME) {
    view->offset = 0;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_END || command==TIPPSE_CMD_SELECT_END) {
    view->offset = file_size;
    view->show_scrollbar = 1;
  } else if (command==TIPPSE_CMD_MOUSE) {
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
  } else if (command==TIPPSE_CMD_SELECT_ALL) {
    offset_old = view->selection_start = 0;
    view->offset = view->selection_end = file_size;
  } else if (command==TIPPSE_CMD_COPY || command==TIPPSE_CMD_CUT) {
    document_undo_chain(file, file->undos);
    if (selection_low!=FILE_OFFSET_T_MAX) {
      document_clipboard_copy(splitter);
      if (command==TIPPSE_CMD_CUT) {
        document_select_delete(splitter);
      } else {
        selection_keep = 1;
      }
    }
    document_undo_chain(file, file->undos);
  } else if (command==TIPPSE_CMD_PASTE) {
    document_undo_chain(file, file->undos);
    document_select_delete(splitter);
    document_clipboard_paste(splitter);
    document_undo_chain(file, file->undos);
  } else if (command==TIPPSE_CMD_UNDO) {
    document_undo_execute_chain(file, view, file->undos, file->redos, 0);
  } else if (command==TIPPSE_CMD_REDO) {
    document_undo_execute_chain(file, view, file->redos, file->undos, 1);
  } else if (command==TIPPSE_CMD_BOOKMARK) {
    if (document_view_select_active(view)) {
      document_bookmark_toggle_selection(splitter);
      document_view_select_nothing(view, file);
    } else {
      if (view->offset<range_tree_length(file->buffer)) {
        document_bookmark_toggle_range(splitter, view->offset, view->offset+1);
      }
    }
  } else if (command==TIPPSE_CMD_BOOKMARK_NEXT) {
    document_bookmark_next(splitter);
  } else if (command==TIPPSE_CMD_BOOKMARK_PREV) {
    document_bookmark_prev(splitter);
  } else if (command==TIPPSE_CMD_BACKSPACE) {
    if (selection_low!=FILE_OFFSET_T_MAX || document->cp_first!=0) {
      document_select_delete(splitter);
    } else {
      view->offset--;
      document_file_delete(file, view->offset, 1);
    }
  } else if (command==TIPPSE_CMD_DELETE) {
    if (selection_low!=FILE_OFFSET_T_MAX || document->cp_first!=0) {
      document_select_delete(splitter);
    } else {
      document_file_delete(file, view->offset, 1);
    }
  } else if (command==TIPPSE_CMD_TAB) {
    document->mode++;
    document->mode %= DOCUMENT_HEX_MODE_MAX;
  } else if (command==TIPPSE_CMD_RETURN) {
    uint8_t text = file->binary?0:32;
    document_select_delete(splitter);
    document_file_insert(file, view->offset, &text, 1, 0);
    view->offset--;
  }

  if (command==TIPPSE_CMD_CHARACTER) {
    if (document->mode==DOCUMENT_HEX_MODE_BYTE) {
      if ((cp>='0' && cp<='9') || (cp>='a' && cp<='f') || (cp>='A' && cp<='F')) {
        if (document->cp_first!=0) {
          uint8_t text = (uint8_t)((document_hex_value(document->cp_first)<<4)+document_hex_value(cp));
          document_file_insert(file, view->offset, &text, 1, 0);
          document_file_delete(file, view->offset, 1);
          document->cp_first = 0;
        } else {
          if (selection_low!=FILE_OFFSET_T_MAX) {
            uint8_t text = file->binary?0:32;
            document_select_delete(splitter);
            document_file_insert(file, view->offset, &text, 1, 0);
            view->offset--;
          }
          document->cp_first = cp;
        }
      }
    } else if (document->mode==DOCUMENT_HEX_MODE_CHARACTER) {
      uint8_t coded[8];
      size_t size = file->encoding->encode(file->encoding, cp, &coded[0], 8);
      document_file_delete(file, view->offset, size);
      document_file_insert(file, view->offset, &coded[0], size, 0);
      document->cp_first = 0;
    }
  } else if (cp!=TIPPSE_CMD_MOUSE || button!=0) {
    document->cp_first = 0;
  }

  file_size = range_tree_length(file->buffer);
  if (view->offset>((file_offset_t)1<<(sizeof(file_offset_t)*8-1))) {
    view->offset = 0;
  } else if (view->offset>file_size) {
    view->offset = file_size;
  }

  int selection_reset = 0;
  if (command==TIPPSE_CMD_MOUSE) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      if (!(button_old&TIPPSE_MOUSE_LBUTTON) && !(key&TIPPSE_KEY_MOD_SHIFT)) view->selection_start = FILE_OFFSET_T_MAX;
      if (view->selection_start==FILE_OFFSET_T_MAX) {
        view->selection_reset = 1;
        view->selection_start = view->offset;
      }
      view->selection_end = view->offset;
    }
  } else {
    if (command==TIPPSE_CMD_SELECT_UP || command==TIPPSE_CMD_SELECT_DOWN || command==TIPPSE_CMD_SELECT_LEFT || command==TIPPSE_CMD_SELECT_RIGHT || command==TIPPSE_CMD_SELECT_PAGEUP || command==TIPPSE_CMD_SELECT_PAGEDOWN || command==TIPPSE_CMD_SELECT_FIRST || command==TIPPSE_CMD_SELECT_LAST || command==TIPPSE_CMD_SELECT_HOME || command==TIPPSE_CMD_SELECT_END || command==TIPPSE_CMD_SELECT_ALL) {
      if (view->selection_start==FILE_OFFSET_T_MAX) {
        view->selection_reset = 1;
        view->selection_start = offset_old;
      }
      view->selection_end = view->offset;
    } else {
      selection_reset = selection_keep?0:1;
    }
  }
  if (!selection_keep) {
    if (selection_reset) {
      view->selection_reset = 1;
      view->selection_start = FILE_OFFSET_T_MAX;
      view->selection_end = FILE_OFFSET_T_MAX;
    }

    document_view_select_nothing(view, file);
    document_view_select_range(view, view->selection_start, view->selection_end, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
  }
}

// Return cursor position from point
void document_hex_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, file_offset_t* offset) {
  struct document_hex* document = (struct document_hex*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = range_tree_length(file->buffer);
  if (y<0) *offset = 0;
  if (y>=0 && y<splitter->client_height) {
    int byte = 0;
    if (x>=view->address_width && x<view->address_width+(3*16)) {
      byte = (x-view->address_width)/3;
      document->mode = DOCUMENT_HEX_MODE_BYTE;
    } else if (x>=view->address_width+(3*16) && x<view->address_width+(3*16)+16) {
      byte = (x-(view->address_width+(3*16)));
      document->mode = DOCUMENT_HEX_MODE_CHARACTER;
    } else if (x>=view->address_width+(3*16)+16) {
      byte = 16;
    }

    *offset = (file_offset_t)(((view->scroll_y+y)*16)+byte);
    if (*offset>file_size) *offset = file_size;
  }
}

// Return value from hex character
uint8_t document_hex_value(codepoint_t cp) {
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
void document_hex_convert(codepoint_t* codepoints, size_t* length, codepoint_t* visuals, int show_invisibles, codepoint_t cp_default) {
  codepoint_t cp = codepoints[0];
  codepoint_t show = -1;
  if (cp=='\n') {
    show = show_invisibles?0x00ac:cp_default;
  } else if (cp=='\r') {
    show = show_invisibles?0x00ac:cp_default;
  } else if (cp=='\t') {
    show = show_invisibles?0x00bb:cp_default;
  } else if (cp==' ') {
    show = show_invisibles?0x22c5:' ';
  } else if (cp<32 || visuals[0]==0xfffd) {
    show = cp_default;
  }

  if (show!=-1) {
    visuals[0] = show;
    *length = 1;
  }
}
