// Tippse - Config - Configuration loader and combiner

#include "config.h"

const char* config_filename = ".tippse";

// Default configuration
const char* config_default =
  "{"
    "colors:{"
      "background:background,"
      "text:foreground,"
      "selection:foreground,"
      "status:grey,"
      "frame:foreground,"
      "type:green,"
      "keyword:yellow,"
      "linecomment:blue,"
      "blockcomment:blue,"
      "string:yellow,"
      "preprocessor:aqua,"
      "bracket:blue,"
      "plus:lime,"
      "minus:red,"
      "linenumber:grey,"
      "bracketerror:red,"
    "},"
    "wrapping:1,"
    "invisibles:0,"
    "keys:{"
      "up:up,"
      "down:down,"
      "left:left,"
      "right:right,"
      "shift+up:selectup,"
      "shift+down:selectdown,"
      "shift+left:selectleft,"
      "shift+right:selectright,"
      "ctrl+q:quit,"
      "pageup:pageup,"
      "pagedown:pagedown,"
      "shift+pageup:selectpageup,"
      "shift+pagedown:selectpagedown,"
      "backspace:backspace,"
      "delete:delete,"
      "insert:insert,"
      "shift+delete:cut,"
      "shift+insert:paste,"
      "first:first,"
      "last:last,"
      "shift+first:selectfirst,"
      "shift+last:selectlast,"
      "ctrl+first:home,"
      "ctrl+last:end,"
      "shift+ctrl+first:selecthome,"
      "shift+ctrl+last:selectend,"
      "ctrl+f:search,"
      "f3:searchnext,"
      "ctrl+z:undo,"
      "ctrl+y:redo,"
      "ctrl+x:cut,"
      "ctrl+c:copy,"
      "ctrl+v:paste,"
      "tab:tab,"
      "shift+tab:untab,"
      "ctrl+s:saveall,"
      "mouse:mouse,"
      "shift+mouse:mouse,"
      "shift+f3:searchprevious,"
      "ctrl+o:open,"
      "f4:split,"
      "shift+f4:unsplit,"
      "ctrl+b:browser,"
      "ctrl+u:switch,"
      "f2:bookmark,"
      "ctrl+w:close,"
      "ctrl+d:documents,"
      "return:return,"
      "ctrl+a:selectall,"
      "pastestart:paste,"
      "f5:compile,"
      "ctrl+h:replace,"
      "f6:replacenext,"
      "shift+f6:replaceprevious,"
      "f7:replaceall,"
      "ctrl+g:goto,"
      "ctrl+r:reload,"
      "ctrl+p:commands,"
    "},"
    "filetypes:{"
      "C:{"
        "colors:{"
          "keywords:{"
            "int:type,"
            "unsigned:type,"
            "signed:type,"
            "char:type,"
            "short:type,"
            "long:type,"
            "void:type,"
            "bool:type,"
            "float:type,"
            "double:type,"
            "size_t:type,"
            "ssize_t:type,"
            "int8_t:type,"
            "uint8_t:type,"
            "int16_t:type,"
            "uint16_t:type,"
            "int32_t:type,"
            "uint32_t:type,"
            "int64_t:type,"
            "uint64_t:type,"
            "nullptr:type,"
            "const:type,"
            "struct:type,"
            "static:type,"
            "inline:type,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "return:keyword,"
            "break:keyword,"
            "continue:keyword,"
            "sizeof:keyword,"
            "NULL:keyword,"
          "},"
          "preprocessor:{"
            "include:preprocessor,"
            "define:preprocessor,"
            "if:preprocessor,"
            "ifdef:preprocessor,"
            "ifndef:preprocessor,"
            "else:preprocessor,"
            "elif:preprocessor,"
            "endif:preprocessor,"
          "}"
        "}"
      "},"
      "C++:{"
        "colors:{"
          "keywords:{"
            "int:type,"
            "unsigned:type,"
            "signed:type,"
            "char:type,"
            "short:type,"
            "long:type,"
            "void:type,"
            "bool:type,"
            "float:type,"
            "double:type,"
            "size_t:type,"
            "ssize_t:type,"
            "int8_t:type,"
            "uint8_t:type,"
            "int16_t:type,"
            "uint16_t:type,"
            "int32_t:type,"
            "uint32_t:type,"
            "int64_t:type,"
            "uint64_t:type,"
            "nullptr:type,"
            "const:type,"
            "struct:type,"
            "class:type,"
            "public:type,"
            "private:type,"
            "protected:type,"
            "virtual:type,"
            "template:type,"
            "static:type,"
            "inline:type,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "return:keyword,"
            "break:keyword,"
            "continue:keyword,"
            "using:keyword,"
            "namespace:keyword,"
            "new:keyword,"
            "delete:keyword,"
            "sizeof:keyword,"
            "NULL:keyword,"
          "},"
          "preprocessor:{"
            "include:preprocessor,"
            "define:preprocessor,"
            "if:preprocessor,"
            "ifdef:preprocessor,"
            "ifndef:preprocessor,"
            "else:preprocessor,"
            "elif:preprocessor,"
            "endif:preprocessor,"
          "}"
        "}"
      "},"
      "JS:{"
        "colors:{"
          "keywords:{"
            "this:type,"
            "var:type,"
            "const:type,"
            "true:type,"
            "false:type,"
            "null:type,"
            "in:keyword,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "return:keyword,"
            "break:keyword,"
            "continue:keyword,"
            "function:keyword,"
            "switch:keyword,"
            "case:keyword,"
            "default:keyword,"
            "new:keyword,"
            "let:keyword,"
          "}"
        "}"
      "},"
      "Lua:{"
        "colors:{"
          "keywords:{"
            "nil:type,"
            "self:type,"
            "true:type,"
            "false:type,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "elseif:keyword,"
            "end:keyword,"
            "then:keyword,"
            "break:keyword,"
            "function:keyword,"
            "local:keyword,"
            "return:keyword,"
            "dofile:keyword,"
            "and:keyword,"
            "or:keyword,"
            "not:keyword,"
          "}"
        "}"
      "},"
      "PHP:{"
        "colors:{"
          "keywords:{"
            "$this:type,"
            "const:type,"
            "var:type,"
            "true:type,"
            "false:type,"
            "max:type,"
            "min:type,"
            "is_set:type,"
            "is_null:type,"
            "empty:keyword,"
            "as:keyword,"
            "foreach:keyword,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "return:keyword,"
            "break:keyword,"
            "continue:keyword,"
            "function:keyword,"
            "defined:keyword,"
            "echo:keyword,"
            "class:keyword,"
            "global:keyword,"
            "include:keyword,"
            "require:keyword,"
            "require_once:keyword,"
            "switch:keyword,"
            "case:keyword,"
            "default:keyword,"
          "}"
        "}"
      "},"
      "SQL:{"
        "colors:{"
          "keywords:{"
            "create:keyword,"
            "database:keyword,"
            "use:keyword,"
            "from:keyword,"
            "to:keyword,"
            "into:keyword,"
            "join:keyword,"
            "where:keyword,"
            "order:keyword,"
            "group:keyword,"
            "by:keyword,"
            "on:keyword,"
            "as:keyword,"
            "is:keyword,"
            "in:keyword,"
            "out:keyword,"
            "begin:keyword,"
            "end:keyword,"
            "delimiter:keyword,"
            "sql:keyword,"
            "data:keyword,"
            "insert:keyword,"
            "select:keyword,"
            "update:keyword,"
            "delete:keyword,"
            "replace:keyword,"
            "drop:keyword,"
            "duplicate:keyword,"
            "temporary:keyword,"
            "modifies:keyword,"
            "reads:keyword,"
            "returns:keyword,"
            "exists:keyword,"
            "return:keyword,"
            "leave:keyword,"
            "call:keyword,"
            "execute:keyword,"
            "show:keyword,"
            "function:keyword,"
            "procedure:keyword,"
            "declare:keyword,"
            "if:keyword,"
            "then:keyword,"
            "elseif:keyword,"
            "else:keyword,"
            "not:keyword,"
            "table:keyword,"
            "values:keyword,"
            "key:keyword,"
            "unique:keyword,"
            "primary:keyword,"
            "foreign:keyword,"
            "constraint:keyword,"
            "references:keyword,"
            "cascade:keyword,"
            "default:keyword,"
            "engine:keyword,"
            "charset:keyword,"
            "auto_increment:keyword,"
            "definer:keyword,"
            "and:keyword,"
            "or:keyword,"
            "set:keyword,"
            "count:keyword,"
            "limit:keyword,"
            "now:type,"
            "max:type,"
            "min:type,"
            "sum:type,"
            "null:type,"
            "bit:type,"
            "int:type,"
            "bigint:type,"
            "char:type,"
            "varchar:type,"
            "string:type,"
            "boolean:type,"
            "short:type,"
            "date:type,"
            "time:type,"
            "datetime:type,"
            "decimal:type,"
            "true:type,"
            "false:type,"
          "}"
        "}"
      "}"
    "}"
  "}"
;

// Create configuration entry assigned value
void config_value_create(struct config_value* base, codepoint_t* value_codepoints, size_t value_length) {
  base->cached = 0;
  base->codepoints = malloc(sizeof(codepoint_t)*value_length);
  memcpy(base->codepoints, value_codepoints, sizeof(codepoint_t)*value_length);
}

void config_value_destroy(struct config_value* base) {
  free(base->codepoints);
}

// Create configuration
struct config* config_create(void) {
  struct config* base = malloc(sizeof(struct config));
  base->keywords = trie_create(sizeof(struct list_node*));
  base->values = list_create(sizeof(struct config_value));
  return base;
}

// Destroy configuration
void config_destroy(struct config* base) {
  config_clear(base);
  trie_destroy(base->keywords);
  list_destroy(base->values);
  free(base);
}

// Clear configuration
void config_clear(struct config* base) {
  trie_clear(base->keywords);
  while (base->values->first) {
    config_value_destroy((struct config_value*)list_object(base->values->first));
    list_remove(base->values, base->values->first);
  }
}

// Load configuration file from disk or memory
void config_load(struct config* base, const char* filename) {
  struct document_file* file = document_file_create(0, 0);
  if (filename) {
    document_file_load(file, filename, 0);
  } else {
    document_file_load_memory(file, (const uint8_t*)config_default, strlen(config_default));
  }

  if (file->buffer) {
    struct encoding_stream stream;
    encoding_stream_from_page(&stream, range_tree_first(file->buffer), 0);
    codepoint_t keyword_codepoints[1024];
    size_t keyword_length = 0;
    codepoint_t value_codepoints[1024];
    size_t value_length = 0;
    size_t bracket_positions[1024];
    size_t brackets = 0;

    int value = 0;
    int escape = 0;
    int string = 0;
    while (1) {
      size_t length;
      codepoint_t cp = (*file->encoding->decode)(file->encoding, &stream, &length);
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
            config_update(base, &keyword_codepoints[0], keyword_length, &value_codepoints[0], value_length+1);
          }

          if (cp=='{') {
            if (keyword_length<sizeof(keyword_codepoints)/sizeof(codepoint_t)) {
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
          if (keyword_length<sizeof(keyword_codepoints)/sizeof(codepoint_t)) {
            keyword_codepoints[keyword_length++] = cp;
          }
        } else {
          if (value_length<(sizeof(value_codepoints)/sizeof(codepoint_t))-1) {
            value_codepoints[value_length++] = cp;
          }
        }
      }
    }
  }

  document_file_destroy(file);
}

// Search all paths from filename to root
void config_loadpaths(struct config* base, const char* filename, int strip) {
  config_load(base, NULL);

  char* home = home_path();
  char* home_name = combine_path_file(home, config_filename);
  char* home_name2 = combine_path_file(home_name, config_filename);
  free(home);
  config_load(base, home_name2);
  free(home_name2);
  free(home_name);

  char* stripped = strip?strip_file_name(filename):strdup(filename);
  char* corrected = correct_path(stripped);
  free(stripped);

  int c = 0;
  while (1) {
    char* name = combine_path_file(corrected, config_filename);
    config_load(base, name);
    free(name);
    char* up = combine_path_file(corrected, "..");
    char* relative = correct_path(up);
    free(up);
    if (strcmp(relative, corrected)==0) {
      free(relative);
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
void config_update(struct config* base, codepoint_t* keyword_codepoints, size_t keyword_length, codepoint_t* value_codepoints, size_t value_length) {
  if (keyword_length==0) {
    return;
  }

  struct list_node* node = list_insert_empty(base->values, NULL);
  config_value_create((struct config_value*)list_object(node), value_codepoints, value_length);

  struct trie_node* parent = NULL;
  while (keyword_length-->0) {
    parent = trie_append_codepoint(base->keywords, parent, *keyword_codepoints, 0);
    if (keyword_length==0) {
      if (parent->end) {
        config_value_destroy((struct config_value*)list_object(*(struct list_node**)trie_object(parent)));
        list_remove(base->values, *(struct list_node**)trie_object(parent));
      }

      parent->end = 1;
      *(struct list_node**)trie_object(parent) = node;
    }

    keyword_codepoints++;
  }
}

// Find entry by code point list
struct trie_node* config_advance_codepoints(struct config* base, struct trie_node* parent, codepoint_t* keyword_codepoints, size_t keyword_length) {
  while (keyword_length-->0) {
    parent = trie_find_codepoint(base->keywords, parent, *keyword_codepoints);
    if (!parent) {
      return NULL;
    }

    keyword_codepoints++;
  }

  return parent;
}

// Find entry by code point list from root
struct trie_node* config_find_codepoints(struct config* base, codepoint_t* keyword_codepoints, size_t keyword_length) {
  return config_advance_codepoints(base, NULL, keyword_codepoints, keyword_length);
}

// Find keyword by ASCII string
struct trie_node* config_advance_ascii(struct config* base, struct trie_node* parent, const char* keyword) {
  while (*keyword) {
    parent = trie_find_codepoint(base->keywords, parent, *(uint8_t*)keyword);
    if (!parent) {
      return NULL;
    }

    keyword++;
  }

  return parent;
}

// Find keyword by ASCII string from root
struct trie_node* config_find_ascii(struct config* base, const char* keyword) {
  return config_advance_ascii(base, NULL, keyword);
}

// Get value at found position
codepoint_t* config_value(struct trie_node* parent) {
  if (parent && parent->end) {
    return ((struct config_value*)list_object(*(struct list_node**)trie_object(parent)))->codepoints;
  }

  return NULL;
}

// Convert code points to ASCII
char* config_convert_ascii_plain(codepoint_t* codepoints) {
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

// Convert value to ASCII
char* config_convert_ascii(struct trie_node* parent) {
  return config_convert_ascii_plain(config_value(parent));
}

// Convert code points to integer
int64_t config_convert_int64_plain(codepoint_t* codepoints) {
  if (!codepoints) {
    return 0;
  }

  int negate = 0;
  int64_t value = 0;
  while (*codepoints) {
    codepoint_t cp = *codepoints++;
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

// Convert value to integer
int64_t config_convert_int64(struct trie_node* parent) {
  return config_convert_int64_plain(config_value(parent));
}

// Convert value to integer and cache keywords
int64_t config_convert_int64_cache(struct trie_node* parent, struct config_cache* cache) {
  if (parent && parent->end) {
    struct config_value* value = (struct config_value*)list_object(*(struct list_node**)trie_object(parent));
    if (value->cached) {
      return value->value;
    }

    value->cached = 1;
    while (cache->text) {
      codepoint_t* left = value->codepoints;
      const char* right = cache->text;
      while ((*left && (codepoint_t)*right) && (*left==(codepoint_t)*right)) {
        left++;
        right++;
      }

      if (!*left && !*right) {
        value->value = cache->value;
        return value->value;
      }
      cache++;
    }

    value->value = config_convert_int64_plain(value->codepoints);
    return value->value;
  }

  return 0;
}
