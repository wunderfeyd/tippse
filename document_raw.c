/* Tippse - Document Raw View - Cursor and display operations for raw 1d display */

#include "document_raw.h"

struct document* document_raw_create() {
  struct document_raw* document = (struct document_raw*)malloc(sizeof(struct document_raw));
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
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!screen) {
    return;
  }

  file_offset_t file_pos = view->offset;
  file_offset_t file_size = file->buffer?file->buffer->length:0;
  file_offset_t displacement;
  struct range_tree_node* buffer = range_tree_find_offset(file->buffer, view->offset, &displacement);
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
  //document_raw_cursor_update(base, splitter);	TODO: needed for screen resize?
  file_offset_t cursor_pos = view->offset+view->cursor_x+view->cursor_y*16;
  sprintf(&status[0], "%lu/%lu bytes - %08lx - Hex ASCII", (unsigned long)cursor_pos, (unsigned long)file_size, (unsigned long)cursor_pos);
  splitter_status(splitter, &status[0], 0);
  int foreground = file->defaults.colors[VISUAL_FLAG_COLOR_TEXT];
  int background = file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  int x, y;
  for (y = 0; y<splitter->client_height; y++) {
    file_offset_t line_pos = file_pos;
    char line[1024];
    uint8_t data[16+1];
    int data_size = file_size-file_pos>16?16:file_size-file_pos;
	for (x = 0; x<data_size; x++,file_pos++) {
	  data[x] = encoding_stream_peek(&stream, 0);
	  encoding_stream_forward(&stream, 1);
	}
    data[x] = 0;
    sprintf(&line[0], "%08lx  ", (unsigned long)line_pos);
	for (x = 0; x<16; x++) {
	  if (x<data_size) {
	    sprintf(&line[10+(3*x)], "%02x ", data[x]);
	  } else {
	    strcpy(&line[10+(3*x)], "   ");
	  }
	}
    for (x = 0; x<data_size; x++) {		//TODO: maybe handle other text formats than ASCII later
	  if (data[x]<32 || data[x]>126) {
	    data[x] = '.';
	  }
	}
	sprintf(&line[58], " %s", &data[0]);
    splitter_drawtext(screen, splitter, 0, y, line, strlen(line), foreground, background);
    if (file_pos>=file_size) break;
  }
  splitter_cursor(screen, splitter, 10+(3*view->cursor_x), view->cursor_y);
  //splitter_scrollbar(splitter, view);  TODO: show scrollbar when implemented in splitter?
}

// Handle key press
void document_raw_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  file_offset_t offset_old = view->offset;
  file_offset_t offset_max = ((file_size/16)-splitter->client_height+1)*16;
  if (offset_max>file_size) offset_max = 0;
  int cursor_x_max = 16-1;
  int cursor_y_max = file_size/16<splitter->client_height-1?file_size/16:splitter->client_height-1;

  if (cp==TIPPSE_KEY_UP) {
    view->cursor_y--;
    if (view->cursor_y<0) {
      view->cursor_y = 0;
      view->offset = view->offset>16?view->offset-16:0;
      if (offset_old==0) view->cursor_x = 0;
    }
  } else if (cp==TIPPSE_KEY_DOWN) {
    view->cursor_y++;
    if (view->cursor_y>cursor_y_max) {
      view->cursor_y = cursor_y_max;
      view->offset = view->offset+16<offset_max?view->offset+16:offset_max;
      if (offset_old==offset_max) view->cursor_x = cursor_x_max;
    }
  } else if (cp==TIPPSE_KEY_LEFT) {
    view->cursor_x--;
    if (view->cursor_x<0) {
      view->cursor_x = cursor_x_max;
      view->cursor_y--;
      if (view->cursor_y<0) {
        view->cursor_y = 0;
        view->offset = view->offset>16?view->offset-16:0;
        if (offset_old==0) view->cursor_x = 0;
      }
    }
  } else if (cp==TIPPSE_KEY_RIGHT) {
    view->cursor_x++;
    if (view->cursor_x>cursor_x_max) {
      view->cursor_x = 0;
      view->cursor_y++;
      if (view->cursor_y>cursor_y_max) {
        view->cursor_y = cursor_y_max;
        view->offset = view->offset+16<offset_max?view->offset+16:offset_max;
        if (offset_old==offset_max) view->cursor_x = cursor_x_max;
      }
    }
  } else if (cp==TIPPSE_KEY_PAGEUP) {
    view->offset -= splitter->client_height*16;
    if (view->offset>file_size) view->offset = 0;
  } else if (cp==TIPPSE_KEY_PAGEDOWN) {
    view->offset += splitter->client_height*16;
    if (view->offset>offset_max) view->offset = offset_max;
  } else if (cp==TIPPSE_KEY_FIRST) {
    view->cursor_x = 0;
  } else if (cp==TIPPSE_KEY_LAST) {
    view->cursor_x = cursor_x_max;
  } else if (cp==TIPPSE_KEY_HOME) {
    view->offset = 0;
    view->cursor_x = 0;
    view->cursor_y = 0;
  } else if (cp==TIPPSE_KEY_END) {
    view->offset = offset_max;
    view->cursor_x = cursor_x_max;
    view->cursor_y = cursor_y_max;
  } else if (cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
    if (button&TIPPSE_MOUSE_LBUTTON) {
      document_raw_cursor_from_point(base, splitter, x, y, &view->cursor_x, &view->cursor_y);
    } else if (button&TIPPSE_MOUSE_WHEEL_UP) {
      view->offset -= (splitter->client_height/3)*16;
      if (view->offset>file_size) view->offset = 0;
    } else if (button&TIPPSE_MOUSE_WHEEL_DOWN) {
      view->offset += (splitter->client_height/3)*16;
      if (view->offset>offset_max) view->offset = offset_max;
    }
  }
  document_raw_cursor_update(base, splitter);
}

// Return cursor position from point
void document_raw_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, int* cursor_x, int* cursor_y)
{
  struct document_file* file = splitter->file;

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  int y_min = 1;
  int y_max = 1+(file_size/16<splitter->client_height-1?file_size/16:splitter->client_height-1);
  if (y>=y_min && y<=y_max)
  {
    *cursor_y = y-y_min;
    if (x<10) *cursor_x=0;
    if (x>=10 && x<58) *cursor_x = (x-10)/3;
    if (x>58) *cursor_x = 16-1;
  }
}

// Update cursor position
void document_raw_cursor_update(struct document* base, struct splitter* splitter) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t file_size = file->buffer?file->buffer->length:0;
  int cursor_x_max = 16-1;
  int cursor_y_max = splitter->client_height-1;
  if (view->offset+view->cursor_x+view->cursor_y*16>file_size) {
    cursor_x_max = file_size%16;
    cursor_y_max = file_size/16;
  }
  if (view->cursor_x<0) view->cursor_x = 0;
  if (view->cursor_x>cursor_x_max) view->cursor_x = cursor_x_max;
  if (view->cursor_y<0) view->cursor_y = 0;
  if (view->cursor_y>cursor_y_max) view->cursor_y = cursor_y_max;
}
