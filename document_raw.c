/* Tippse - Document Raw View - Cursor and display operations for raw 1d display */

#include "document_raw.h"

struct document* document_raw_create() {
  struct document_raw* document = (struct document_raw*)malloc(sizeof(struct document_raw));
  document->cp_first = 0;
  document->vtbl.reset = document_raw_reset;
  document->vtbl.draw = document_raw_draw;
  document->vtbl.keypress = document_raw_keypress;
  document->vtbl.incremental_update = document_raw_incremental_update;

  return (struct document*)document;
}

void document_raw_destroy(struct document* base) {
  free(base);
}

// Called after a new document was assigned
void document_raw_reset(struct document* base, struct splitter* splitter) {
}

// Find next dirty pages and rerender them (background task)
int document_raw_incremental_update(struct document* base, struct splitter* splitter) {
  return 0;
}

// Draw entire visible screen space
void document_raw_draw(struct document* base, struct screen* screen, struct splitter* splitter) {
  struct document_raw* document = (struct document_raw*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!screen) {
    return;
  }

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  view->cursor_x = view->offset%16;
  view->cursor_y = view->offset/16;
  if (view->cursor_y>=view->scroll_y+splitter->client_height) {
    view->scroll_y = view->cursor_y-splitter->client_height+1;
  }
  view->scroll_y_max = (file_size+16)/16;
  if (view->cursor_y<view->scroll_y) {
    view->scroll_y = view->cursor_y;
  }
  int max_height = ((file_size+16)/16)-splitter->client_height;
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
  struct encoding_stream stream;
  encoding_stream_from_page(&stream, buffer, displacement);

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

  char status[1024];
  sprintf(&status[0], "%lu/%lu bytes - %08lx - Hex ASCII", (unsigned long)view->offset, (unsigned long)file_size, (unsigned long)view->offset);
  splitter_status(splitter, &status[0], 0);
  int foreground = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT];
  int background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  int x, y;
  for (y = 0; y<splitter->client_height; y++) {
    char line[1024];
    sprintf(&line[0], "%08lx  ", (unsigned long)offset);
    uint8_t data[16+1];
    int data_size = (int)file_size-offset>16?16:file_size-offset;
    for (x = 0; x<data_size; x++,offset++) {
      data[x] = encoding_stream_peek(&stream, 0);
      encoding_stream_forward(&stream, 1);
    }
    data[x] = 0;
    for (x = 0; x<16; x++) {
      if (x<data_size) {
        sprintf(&line[10+(3*x)], "%02x ", data[x]);
      } else {
        strcpy(&line[10+(3*x)], "   ");
      }
    }
    for (x = 0; x<data_size; x++) {   //TODO: maybe handle other text formats than ASCII later
      if (data[x]<32 || data[x]>126) {
        data[x] = '.';
      }
    }
    sprintf(&line[58], " %s", &data[0]);
    splitter_drawtext(screen, splitter, 0, y, line, strlen(line), foreground, background);
    if (offset>=file_size) break;
  }
  if (document->cp_first!=0) {
    uint8_t text = document->cp_first;
    splitter_drawtext(screen, splitter, 10+(3*view->cursor_x), view->cursor_y-view->scroll_y, (char*)&text, 1, foreground, background);
  }
  splitter_cursor(screen, splitter, 10+(3*view->cursor_x), view->cursor_y-view->scroll_y);
  splitter_scrollbar(screen, splitter);
}

// Handle key press
void document_raw_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document_raw* document = (struct document_raw*)base;
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = file->buffer?file->buffer->length:0;

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
    view->offset -= splitter->client_height*16;
    view->scroll_y -= splitter->client_height;
    view->show_scrollbar = 1;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    view->offset += splitter->client_height*16;
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
  } else if (cp==TIPPSE_KEY_UNDO) {
    document_undo_execute(file, view, file->undos, file->redos);
    while (document_undo_execute(file, view, file->undos, file->redos)) {}
  } else if (cp==TIPPSE_KEY_REDO) {
    document_undo_execute(file, view, file->redos, file->undos);
    while (document_undo_execute(file, view, file->redos, file->undos)) {}
  } else if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      document_raw_cursor_from_point(base, splitter, x, y, &view->offset);
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      view->offset -= (splitter->client_height/3)*16;
      view->scroll_y -= splitter->client_height/3;
      view->show_scrollbar = 1;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      view->offset += (splitter->client_height/3)*16;
      view->scroll_y += splitter->client_height/3;
      view->show_scrollbar = 1;
    }
  } else if (cp=='\n') {
    uint8_t text = file->binary?0:32;
    document_file_insert(splitter->file, view->offset, &text, 1);
    view->offset--;
  } else if (cp==TIPPSE_KEY_BACKSPACE) {
    view->offset--;
    document_file_delete(splitter->file, view->offset, 1);
  } else if (cp==TIPPSE_KEY_DELETE) {
    document_file_delete(splitter->file, view->offset, 1);
  }

  if (cp>=32 && cp<=126) {
    if (document->cp_first!=0) {
      uint8_t text = (document_raw_value(document->cp_first)<<4) + document_raw_value(cp);
      document_file_insert(splitter->file, view->offset, &text, 1);
      document_file_delete(splitter->file, view->offset, 1);
      document->cp_first = 0;
    } else {
      document->cp_first = cp;
    }
  } else {
    document->cp_first = 0;
  }

  file_size = file->buffer?file->buffer->length:0;
  if (view->offset>((file_offset_t)1<<(sizeof(file_offset_t)*8-1))) {
    view->offset = 0;
  } else if (view->offset>file_size) {
    view->offset = file_size;
  }
}

// Return cursor position from point
void document_raw_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, file_offset_t* offset) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  x -= splitter->x;
  y -= splitter->y;
  file_offset_t file_size = file->buffer?file->buffer->length:0;
  if (y>=0 && y<splitter->client_height) {
    if (x>=10 && x<10+16*3) *offset = ((view->scroll_y+y)*16)+((x-10)/3);
    if (x>=59 && x<59+16) *offset = ((view->scroll_y+y)*16)+x-59;
    if (*offset>file_size) *offset = file_size;
  }
}

// Return value from hex character
uint8_t document_raw_value(int cp) {
  uint8_t value = 0;
  if (cp>='0' && cp<='9') {
    value = cp-'0';
  } else if (cp>='a' && cp<='f') {
    value = cp-'a'+10;
  } else if (cp>='A' && cp<='F') {
    value = cp-'A'+10;
  }
  return value;
}
