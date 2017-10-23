// Tippse - Config - Configuration loader and combiner

#include "config.h"

const char* config_filename = ".tippse";

// Default configuration
const char* config_default =
  "{"
    "colors:{"
      "background:-2,"
      "text:-1,"
      "selection:-1,"
      "status:8,"
      "frame:-1,"
      "readonly:8,"
      "type:2,"
      "keyword:11,"
      "linecomment:12,"
      "blockcomment:12,"
      "string:11,"
      "preprocessor:14,"
      "bracket:12,"
      "plus:10,"
      "minus:9,"
      "linenumber:8,"
    "},"
    "wrapping:1,"
    "invisibles:0,"
    "continuous:0,"
  "}"
;

// Create configuration
struct config* config_create(void) {
  struct config* config = malloc(sizeof(struct config));
  config->keywords = trie_create();
  config->values = list_create();
  return config;
}

// Destroy configuration
void config_destroy(struct config* config) {
  config_clear(config);
  trie_destroy(config->keywords);
  list_destroy(config->values);
  free(config);
}

// Clear configuration
void config_clear(struct config* config) {
  trie_clear(config->keywords);
  while (config->values->first) {
    free(config->values->first->object);
    list_remove(config->values, config->values->first);
  }
}

// Load configuration file from disk or memory
void config_load(struct config* config, const char* filename) {
  struct document_file* file = document_file_create(0);
  if (filename) {
    document_file_load(file, filename);
  } else {
    document_file_load_memory(file, (const uint8_t*)config_default, strlen(config_default));
  }

  if (file->buffer) {
    struct encoding_stream stream;
    encoding_stream_from_page(&stream, range_tree_first(file->buffer), 0);
    int keyword_codepoints[1024];
    size_t keyword_length = 0;
    int value_codepoints[1024];
    size_t value_length = 0;
    size_t bracket_positions[1024];
    size_t brackets = 0;

    int value = 0;
    int escape = 0;
    int string = 0;
    while (1) {
      size_t length;
      int cp = (*file->encoding->decode)(file->encoding, &stream, &length);
      encoding_stream_forward(&stream, length);

      if (cp==0) {
        break;
      }

      if (escape>0) {
        escape--;
        continue;
      }

      int append = 0;
      if (!string) {
        if (cp==',' || cp=='{' || cp=='}') {
          size_t keyword = keyword_length-((brackets>0)?bracket_positions[brackets-1]:0);
          if (cp!='{' && keyword) {
            value_codepoints[value_length] = 0;
            config_update(config, &keyword_codepoints[0], keyword_length, &value_codepoints[0], value_length+1);
          }

          if (cp=='{') {
            if (keyword_length<sizeof(keyword_codepoints)/sizeof(int)) {
              keyword_codepoints[keyword_length++] = '/';
            }

            if (brackets<sizeof(bracket_positions)/sizeof(int)) {
              bracket_positions[brackets++] = keyword_length;
            }
          } else if (cp=='}' && brackets>0) {
            brackets--;
          }

          if (brackets>0) {
            keyword_length = bracket_positions[brackets-1];
          } else {
            keyword_length = 0;
          }

          value_length = 0;
          value = 0;
        } else if (cp=='{') {
        } else if (cp=='"') {
          string = 1;
        } else if (cp==':') {
          value = 1;
        } else if (cp=='\r' || cp=='\n' || cp==' ' || cp=='\t') {
        } else {
          append = 1;
        }
      } else {
        if (cp=='"' || cp=='\r' || cp=='\n') {
          string = 0;
        } else if (cp=='\\') {
          escape = 1;
        } else {
          append = 1;
        }
      }

      if (append) {
        if (!value) {
          if (keyword_length<sizeof(keyword_codepoints)/sizeof(int)) {
            keyword_codepoints[keyword_length++] = cp;
          }
        } else {
          if (value_length<(sizeof(value_codepoints)/sizeof(int))-1) {
            value_codepoints[value_length++] = cp;
          }
        }
      }
    }
  }

  document_file_destroy(file);
}

// Search all paths from filename to root
void config_loadpaths(struct config* config, const char* filename, int strip) {
  config_load(config, NULL);

  char* home = home_path();
  char* home_name = combine_path_file(home, config_filename);
  char* home_name2 = combine_path_file(home_name, config_filename);
  free(home);
  config_load(config, home_name2);
  free(home_name2);
  free(home_name);

  char* stripped = strip?strip_file_name(filename):strdup(filename);
  char* corrected = correct_path(stripped);
  free(stripped);

  int c = 0;
  while (1) {
    char* name = combine_path_file(corrected, config_filename);
    config_load(config, name);
    free(name);
    char* up = combine_path_file(corrected, "..");
    char* relative = correct_path(up);
    free(up);
    if (strcmp(relative, corrected)==0) {
      break;
    }

    free(corrected);
    corrected = relative;
    c++;
    if (c==20) {
      break;
    }
  }

  free(corrected);
}

// Update configuration keyword
void config_update(struct config* config, int* keyword_codepoints, size_t keyword_length, int* value_codepoints, size_t value_length) {
  int* values = malloc(sizeof(int)*value_length);
  memcpy(values, value_codepoints, sizeof(int)*value_length);
  struct list_node* node = list_insert(config->values, NULL, values);

  struct trie_node* parent = NULL;
  while (keyword_length-->0) {
    parent = trie_append_codepoint(config->keywords, parent, *keyword_codepoints, 0);
    if (keyword_length==0) {
      if (parent->type!=0) { // TODO: 0 != NULL
        free(((struct list_node*)parent->type)->object);
        list_remove(config->values, (struct list_node*)parent->type);
      }

      parent->type = (intptr_t)node;
    }

    keyword_codepoints++;
  }
}

// Find keyword by code point list
int* config_find_codepoints(struct config* config, int* keyword_codepoints, size_t keyword_length) {
  struct trie_node* parent = NULL;
  while (keyword_length-->0) {
    parent = trie_find_codepoint(config->keywords, parent, *keyword_codepoints);
    if (!parent) {
      return NULL;
    }

    if (parent->type!=0 && keyword_length==0) {
      return (int*)(((struct list_node*)parent->type)->object);
    }

    keyword_codepoints++;
  }

  return NULL;
}

// Find keyword by ASCII string
int* config_find_ascii(struct config* config, const char* keyword) {
  struct trie_node* parent = NULL;
  while (*keyword) {
    parent = trie_find_codepoint(config->keywords, parent, *(uint8_t*)keyword);
    if (!parent) {
      return NULL;
    }

    keyword++;

    if (parent->type!=0 && !*keyword) {
      return (int*)(((struct list_node*)parent->type)->object);
    }
  }

  return NULL;
}

// Convert code points to ASCII
char* config_convert_ascii(int* codepoints) {
  if (!codepoints) {
    return strdup("");
  }

  size_t length = 0;
  while (codepoints[length++]) {
  }

  char* string = malloc(sizeof(char)*length);
  length = 0;
  do {
    string[length] = (char)codepoints[length];
  } while (codepoints[length++]);

  return string;
}

// Convert code points to integer
int64_t config_convert_int64(int* codepoints) {
  if (!codepoints) {
    return 0;
  }

  int negate = 0;
  int64_t value = 0;
  while (*codepoints) {
    int cp = *codepoints++;
    if (cp=='-') {
      negate = 1;
    } else if (cp>='0' && cp<='9') {
      value *= 10;
      value += cp-'0';
    }
  }

  if (negate) {
    value = -value;
  }

  return value;
}
