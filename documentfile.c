/* Tippse - Document - File operations and file setup */

#include "documentfile.h"

// TODO: this has to be covered by the settings subsystem in future
struct document_file_type document_file_types[] = {
  {"c",  file_type_c_create},
  {"h",  file_type_c_create},
  {"cpp",  file_type_cpp_create},
  {"hpp",  file_type_cpp_create},
  {"cc",  file_type_cpp_create},
  {"cxx",  file_type_cpp_create},
  {"sql",  file_type_sql_create},
  {"lua",  file_type_lua_create},
  {"php",  file_type_php_create},
  {"xml",  file_type_xml_create},
  {NULL,  NULL}
};

struct document_file* document_file_create(int save) {
  struct document_file* file = (struct document_file*)malloc(sizeof(struct document_file));
  file->buffer = NULL;
  file->undos = list_create();
  file->redos = list_create();
  file->filename = strdup("");
  file->views = list_create();
  file->modified = 0;
  file->save = save;
  file->type = file_type_text_create();
  file->encoding = encoding_utf8_create();
  return file;
}

void document_file_clear(struct document_file* file) {
  if (file->buffer) {
    range_tree_destroy(file->buffer);
    file->buffer = NULL;
  }
}

void document_file_destroy(struct document_file* file) {
  document_file_clear(file);
  list_destroy(file->undos);
  list_destroy(file->redos);
  list_destroy(file->views);
  free(file->filename);
  (*file->type->destroy)(file->type);
  (*file->encoding->destroy)(file->encoding);
  free(file);
}

void document_file_name(struct document_file* file, const char* filename) {
  free(file->filename);
  file->filename = strdup(filename);

  const char* search = filename;
  const char* last = filename;
  while (*search) {
    if (*search=='.') {
      last = search+1;
    }

    search++;
  }

  if (last==filename) {
    last = search;
  }

  size_t n;
  for (n = 0; document_file_types[n].extension; n++) {
    if (strcasecmp(document_file_types[n].extension, last)==0) {
      (*file->type->destroy)(file->type);
      file->type = (*document_file_types[n].constructor)();
      break;
    }
  }
}

void document_file_load(struct document_file* file, const char* filename) {
  document_file_clear(file);
  int f = open(filename, O_RDONLY);
  if (f!=-1) {
    file_offset_t offset = 0;
    while (1) {
      uint8_t* copy = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MAX);
      int got = read(f, copy, TREE_BLOCK_LENGTH_MAX);
      if (got<=0) {
        free(copy);
        break;
      }

      struct fragment* buffer = fragment_create_memory(copy, got);
      file->buffer = range_tree_insert(file->buffer, file->type, offset, buffer, 0, buffer->length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER);

      offset += got;
      if (got<TREE_BLOCK_LENGTH_MAX) {
        break;
      }
    }

/*    struct file_cache* cache = file_cache_create(filename);

    file_offset_t length = (file_offset_t)lseek(f, 0, SEEK_END);
    file_offset_t offset = 0;
    while (offset<length) {
      file_offset_t block = length-offset;
      if (block>TREE_BLOCK_LENGTH_MAX) {
        block = TREE_BLOCK_LENGTH_MAX;
      }

      struct fragment* buffer = fragment_create_file(cache, offset, (size_t)block);
      file->buffer = range_tree_insert(file->buffer, file->type, offset, buffer, 0, buffer->length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER);
      offset += block;
    }

    file_cache_dereference(cache);*/

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
