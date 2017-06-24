/* Tippse - Document - File operations and file setup */

#include "documentfile.h"

struct document_file* document_file_create(int save) {
  struct document_file* file = (struct document_file*)malloc(sizeof(struct document_file));
  file->buffer = NULL;
  file->undos = list_create();
  file->redos = list_create();
  file->filename = strdup("");
  file->views = list_create();
  file->modified = 0;
  file->save = save;
  return file;
}

void document_file_clear(struct document_file* file) {
  if (file->buffer) {
    range_tree_clear(file->buffer);
    file->buffer = NULL;
  }
}

void document_file_destroy(struct document_file* file) {
  document_file_clear(file);
  list_destroy(file->undos);
  list_destroy(file->redos);
  list_destroy(file->views);
  free(file->filename);
  free(file);
}

void document_file_name(struct document_file* file, const char* filename) {
  free(file->filename);
  file->filename = strdup(filename);
}

void document_file_load(struct document_file* file, const char* filename) {
  document_file_clear(file);
  int f = open(filename, O_RDONLY);
  if (f!=-1) {
    char in[1024];
    file_offset_t offset = 0;
    while (1) {
      int got = read(f, &in[0], 1024);
      if (got<=0) {
        break;
      }

      file->buffer = range_tree_insert_split(file->buffer, offset, &in[0], got, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, NULL);

      offset += got;
      if (got<1024) {
        break;
      }
    }
    close(f);
  }
  document_file_name(file, filename);
  file->modified = 0;
}

void document_file_save(struct document_file* file, const char* filename) {
  int f = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);
  if (f!=-1) {
    file->modified = 0;
    if (file->buffer) {
      struct range_tree_node* buffer = range_tree_first(file->buffer);
      while (buffer) {
        write(f, buffer->buffer->buffer+buffer->offset, buffer->length);
        buffer = range_tree_next(buffer);
      }
    }
    close(f);
  }

  //document->keep_status = 1;
  //splitter_status(splitter, "Saved!", 1);
  //splitter_name(splitter, filename);
}
