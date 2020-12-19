#include <stdio.h>
#include <stdlib.h>

#include "../library/types.h"
#include "../library/list.h"
#include "../library/stream.h"
#include "../library/file.h"
#include "../library/search.h"
#include "../library/encoding.h"
#include "../library/trie.h"

struct dedup_file {
  char* filename;
  uint8_t* buffer;
  file_offset_t length;
};

struct dedup_block {
  file_offset_t start;
  file_offset_t end;
  uint8_t* reduced;
  file_offset_t size;
  int line;
  uint8_t* buffer;
  file_offset_t length;
  const char* filename;
};

struct dedup_remover {
  struct search* from;
  char* to;
};

struct dedup_transform {
  struct search* valid;
  struct search* invalid;
  struct search* skip;
  struct list* removers;
  struct list* blocks;
  struct list* files;
};

file_offset_t find_line(uint8_t* buffer, file_offset_t start, file_offset_t end) {
  if (start==end) {
    return 0;
  }

  while (start<end) {
    if (buffer[start]=='\n') {
      return start+1;
    }
    start++;
  }

  return start;
}

struct search* search_build(const char* text) {
  struct stream stream;
  stream_from_plain(&stream, (uint8_t*)text, strlen(text));
  struct search* search = search_create_regex(0, 0, &stream, encoding_utf8_static(), encoding_utf8_static());
  stream_destroy(&stream);
  return search;
}

void transform_block_destroy_inplace(struct dedup_block* base) {
  free(base->reduced);
}

struct dedup_transform* transform_create() {
  struct dedup_transform* base = malloc(sizeof(struct dedup_transform));
  base->valid = NULL;
  base->invalid = NULL;
  base->skip = NULL;
  base->removers = list_create(sizeof(struct dedup_remover));
  base->blocks = list_create(sizeof(struct dedup_block));
  base->files = list_create(sizeof(struct dedup_file));
  return base;
}

void transform_create_remover(struct dedup_transform* base, const char* from, const char* to) {
  struct dedup_remover remover;
  remover.from = search_build(from);
  remover.to = strdup(to);
  list_insert(base->removers, NULL, &remover);
}

void transform_destroy(struct dedup_transform* base) {
  while (base->removers->first) {
    struct dedup_remover* remover = (struct dedup_remover*)list_object(base->removers->first);
    free(remover->to);
    search_destroy(remover->from);
    list_remove(base->removers, base->removers->first);
  }
  list_destroy(base->removers);

  while (base->blocks->first) {
    struct dedup_block* block = (struct dedup_block*)list_object(base->blocks->first);
    transform_block_destroy_inplace(block);
    list_remove(base->blocks, base->blocks->first);
  }
  list_destroy(base->blocks);

  while (base->files->first) {
    struct dedup_file* file = (struct dedup_file*)list_object(base->files->first);
    free(file->filename);
    free(file->buffer);
    list_remove(base->files, base->files->first);
  }
  list_destroy(base->files);

  search_destroy(base->skip);
  search_destroy(base->valid);
  search_destroy(base->invalid);
  free(base);
}

void transform_block(struct dedup_transform* base, struct dedup_block* block, uint8_t* buffer, file_offset_t length, file_offset_t start, file_offset_t end, int line, const char* filename) {

  uint8_t* clean = (uint8_t*)malloc(sizeof(uint8_t)*(end-start));
  file_offset_t size = 0;
  {
    struct stream stream;
    stream_from_plain(&stream, (uint8_t*)buffer+start, end-start);
    while (!stream_end(&stream)) {
      int found = search_find_check(base->skip, &stream);
      if (!found) {
        found = 0;
        struct list_node* it = base->removers->first;
        while (it) {
          struct dedup_remover* remover = (struct dedup_remover*)list_object(it);
          found = search_find_check(remover->from, &stream);
          if (found) {
            file_offset_t length = stream_offset(&remover->from->hit_end)-stream_offset(&remover->from->hit_start);
            const char* copy = remover->to;
            while (*copy) {
              clean[size++] = (uint8_t)*copy;
              copy++;
            }

            stream_forward(&stream, length);
            break;
          }

          it = it->next;
        }

        if (!found) {
          clean[size++] = (uint8_t)stream_read_forward(&stream);
        }
      } else {
        file_offset_t length = stream_offset(&base->skip->hit_end)-stream_offset(&base->skip->hit_start);
        while (length>0) {
          length--;
          clean[size++] = (uint8_t)stream_read_forward(&stream);
        }
      }
    }
    stream_destroy(&stream);
  }

  uint8_t* fill = (uint8_t*)malloc(sizeof(uint8_t)*(end-start)*8);
  file_offset_t fill_size = 0;
  {
    struct trie* indexes = trie_create(sizeof(int));
    int index = 0;
    struct stream stream;
    stream_from_plain(&stream, (uint8_t*)clean, size);
    while (!stream_end(&stream)) {
      int found = search_find_check(base->invalid, &stream);
      if (!found) {
        int found = search_find_check(base->valid, &stream);
        if (found) {
          file_offset_t length = stream_offset(&base->valid->hit_end)-stream_offset(&base->valid->hit_start);
          struct trie_node* node = NULL;
          while (length>0) {
            length--;
            node = trie_append_codepoint(indexes, node, stream_read_forward(&stream), 0);
          }

          int indexed = 0;
          if (!node->end) {
            node->end = 1;
            index++;
            *((int*)trie_object(node)) = index;
          } else {
            index = *((int*)trie_object(node));
          }

          fill_size += sprintf(&fill[fill_size], "<%d>", index);
        } else {
          fill[fill_size++] = stream_read_forward(&stream);
        }
      } else {
        file_offset_t length = stream_offset(&base->invalid->hit_end)-stream_offset(&base->invalid->hit_start);
        while (length>0) {
          length--;
          fill[fill_size++] = stream_read_forward(&stream);
        }
      }
    }
    stream_destroy(&stream);
    trie_destroy(indexes);
  }

  uint8_t* reduced = malloc(sizeof(uint8_t)*fill_size);
  for (size_t n = 0; n<fill_size; n++) {
    reduced[n] = fill[n];
  }

  block->start = start;
  block->end = end;
  block->buffer = buffer;
  block->length = length;
  block->line = line;
  block->filename = filename;
  block->reduced = reduced;
  block->size = fill_size;

  free(fill);
  free(clean);
}

void transform_duplicates(struct dedup_transform* base) {
  struct trie* indexes = trie_create(sizeof(struct dedup_block*));

  uint8_t* buffer_old = NULL;
  file_offset_t end_old = 0;

  struct list_node* it = base->blocks->first;
  while (it) {
    struct dedup_block* block = (struct dedup_block*)list_object(it);
    file_offset_t length = block->size;
    uint8_t* copy = block->reduced;
    struct trie_node* node = NULL;
    while (length>0) {
      length--;
      node = trie_append_codepoint(indexes, node, *copy++, 0);
    }

    if (node->end) {
      struct dedup_block* other = *((struct dedup_block**)trie_object(node));
      if ((other->buffer!=block->buffer || (other->start>=block->end || other->end<=block->start)) && (buffer_old!=block->buffer || end_old<=block->start)) {

        file_offset_t block_start = block->start;
        file_offset_t block_end = block->end;
        file_offset_t other_start = other->start;
        file_offset_t other_end = other->end;

        while (1) {
          file_offset_t block_end_new = find_line(block->buffer, block_end, block->length);
          if (block_end_new==0) {
            break;
          }
          struct dedup_block block_in;
          transform_block(base, &block_in, block->buffer, block->length, block_start, block_end_new, 0, "");

          file_offset_t other_end_new = find_line(other->buffer, other_end, other->length);
          if (other_end_new==0) {
            transform_block_destroy_inplace(&block_in);
            break;
          }
          struct dedup_block block_other;
          transform_block(base, &block_other, other->buffer, other->length, other_start, other_end_new, 0, "");

          if (block_in.size==0 || block_other.size==0 || block_in.size!=block_other.size) {
            transform_block_destroy_inplace(&block_in);
            transform_block_destroy_inplace(&block_other);
            break;
          }

          if (block_other.buffer==block_in.buffer && ( (block_other.start<block_in.end && block_other.end>=block_in.start))) {
            transform_block_destroy_inplace(&block_in);
            transform_block_destroy_inplace(&block_other);
            break;
          }

          if (memcmp(block_in.buffer, block_other.buffer, block_in.size)!=0) {
            transform_block_destroy_inplace(&block_in);
            transform_block_destroy_inplace(&block_other);
            break;
          }

          block_start = block_in.start;
          block_end = block_in.end;
          other_start = block_other.start;
          other_end = block_other.end;

          transform_block_destroy_inplace(&block_in);
          transform_block_destroy_inplace(&block_other);
        }

        printf("Duplication @ %s:%d <- %s:%d\r\n", block->filename, block->line, other->filename, other->line);
        printf("+++ %s:%d\r\n", block->filename, block->line);
        for (file_offset_t pos = block_start; pos<block_end; pos++) {
          printf("%c", block->buffer[pos]);
        }
        printf("--- %s:%d\r\n", other->filename, other->line);
        for (file_offset_t pos = other_start; pos<other_end; pos++) {
          printf("%c", other->buffer[pos]);
        }
        printf("\r\n");

        buffer_old = block->buffer;
        end_old = block_end;
      }
    } else {
      node->end = 1;
      *((struct dedup_block**)trie_object(node)) = block;
    }

    it = it->next;
  }

  trie_destroy(indexes);
}

void transform_load(struct dedup_transform* base, const char* filename) {

  printf("load %s...\r\n", filename);
  
  uint8_t* buffer = NULL;
  file_offset_t length = 0;
  struct file* file = file_create(filename, TIPPSE_FILE_READ);
  if (!file) {
    printf("File '%s' access not granted\r\n", filename);
    return;
  } else {
    length = file_seek(file, 0, TIPPSE_SEEK_END);
    file_seek(file, 0, TIPPSE_SEEK_START);
    buffer = (uint8_t*)malloc(length);
    file_read(file, buffer, length);
    file_destroy(file);
  }

  if (!buffer || length==0) {
    free(buffer);
    printf("File '%s' has no content\r\n", filename);
    return;
  }

  struct dedup_file entry;
  entry.filename = strdup(filename);
  entry.buffer = buffer;
  entry.length = length;
  list_insert(base->files, NULL, &entry);

  int lines_prefetch = 5;
  file_offset_t next = 0;
  file_offset_t prev = 0;
  for (int lines = 0; lines<lines_prefetch; lines++) {
    next = find_line(buffer, next, length);
    if (!next) {
      break;
    }
  }

  if (!next) {
    printf("File '%s' has not enough lines\r\n", filename);
    return;
  }

  int line = 0;
  while (1) {
    line++;
    struct dedup_block block;
    transform_block(base, &block, buffer, length, prev, next, line, entry.filename);
    if (block.size>0) {
      list_insert(base->blocks, base->blocks->last, &block);
    } else {
      transform_block_destroy_inplace(&block);
    }

    prev = find_line(buffer, prev, length);
    next = find_line(buffer, next, length);
    if (!next) {
      break;
    }
  }
}

int main(int argc, const char** argv) {
  if (argc<2) {
    printf("Usage: dedup <file...>\r\n");
    return 1;
  }

  encoding_init();
  unicode_init();

  struct dedup_transform* transform = transform_create();
  transform->valid = search_build("[\\w\\_\\d\\$]+");
  transform->invalid = search_build("\\\"(\\\\.|[^\\\\])*\\\"|//[^\\n]+|[\\w\\_\\d]+\\(|(size_t|const|char|short|int|long|uint\\d+_t|int\\d+_t|file_offset_t|ssize_t|struct [\\w\\_\\d]+|while|if|for|else|switch|case|return|inline|void)[^\\w]|\\-\\>[\\w\\_\\d]+|\\.[\\w\\_\\d]+");
  transform->skip = search_build("\\\"(\\\\.|[^\\\\])*\\\"|[\\w\\_\\d]\\s");
  transform_create_remover(transform, "\\s|\\r|\\n", "");
  transform_create_remover(transform, "//[^\\n]+", "");

  for (int n = 1; n<argc; n++) {
    transform_load(transform, argv[n]);
  }

  transform_duplicates(transform);
  transform_destroy(transform);

  unicode_free();
  encoding_free();
  return 0;
}
