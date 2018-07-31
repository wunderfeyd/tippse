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
      "consolenormal:grey,"
      "consolewarning:yellow,"
      "consoleerror:red,"
      "bookmark:aqua,"
      "directory:aqua,"
      "modified:red,"
      "removed:red,"
    "},"
    "wrapping:1,"
    "invisibles:0,"
    "addresswidth:6,"
    "shell:{"
      "compile:\"./make.sh\","
    "},"
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
      "f5:shell compile,"
      "shift+f5:shellkill,"
      "ctrl+h:replace,"
      "f6:replacenext,"
      "shift+f6:replaceprevious,"
      "f7:replaceall,"
      "ctrl+g:goto,"
      "ctrl+r:reload,"
      "ctrl+p:commands,"
      "ctrl+k:cutline,"
      "ctrl+up:blockup,"
      "ctrl+down:blockdown,"
      "ctrl+pageup:bookmarkprev,"
      "ctrl+pagedown:bookmarknext,"
      "ctrl+left:wordprev,"
      "ctrl+right:wordnext,"
      "shift+ctrl+left:selectwordprev,"
      "shift+ctrl+right:selectwordnext,"
      "escape:escape,"
      "ctrl+f3:searchall,"
      "shift+ctrl+f3:searchdirectory,"
      "ctrl+t:new,"
      "ctrl+n:splitnext,"
    "},"
    "fileextensions:{"
      "c:C,"
      "h:C,"
      "cpp:C++,"
      "hpp:C++,"
      "cc:C++,"
      "cxx:C++,"
      "sql:SQL,"
      "lua:Lua,"
      "php:PHP,"
      "xml:XML,"
      "diff:Patch,"
      "patch:Patch,"
      "js:JS,"
    "},"
    "filetypes:{"
      "compiler_output:{"
        "parser:c,"
        "colors:{"
          "keywords:{"
            "\"error:\":consoleerror,"
            "\"warning:\":consolewarning,"
          "},"
          "preprocessor:{},"
        "},"
      "},"
      "C:{"
        "parser:c,"
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
            "extern:type,"
            "for:keyword,"
            "do:keyword,"
            "while:keyword,"
            "if:keyword,"
            "else:keyword,"
            "return:keyword,"
            "break:keyword,"
            "continue:keyword,"
            "sizeof:keyword,"
            "switch:keyword,"
            "case:keyword,"
            "default:keyword,"
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
        "parser:c,"
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
            "extern:type,"
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
            "switch:keyword,"
            "case:keyword,"
            "default:keyword,"
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
        "parser:c,"
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
        "parser:lua,"
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
        "parser:php,"
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
        "parser:sql,"
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
            "ignore:keyword,"
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
      "},"
      "Patch:{"
        "parser:patch,"
      "},"
      "XML:{"
        "parser:xml,"
      "},"
    "}"
  "}"
;

// Build command
void config_command_create_inplace(struct config_command* base) {
  base->cached = 0;
  base->arguments = NULL;
  base->length = 0;
}

void config_command_destroy_inplace(struct config_command* base) {
  for (size_t n = 0; n<base->length; n++) {
    free(base->arguments[n].codepoints);
  }
  free(base->arguments);
}

void config_command_destroy(struct config_command* base) {
  config_command_destroy_inplace(base);
  free(base);
}

struct config_command* config_command_clone(struct config_command* clone) {
  struct config_command* base = malloc(sizeof(struct config_command));
  base->cached = clone->cached;
  base->value = clone->value;
  base->length = clone->length;
  size_t length = sizeof(struct config_argument)*clone->length;
  base->arguments = (struct config_argument*)malloc(length);
  for (size_t n = 0; n<clone->length; n++) {
    base->arguments[n].length = clone->arguments[n].length;
    size_t length = sizeof(codepoint_t)*(clone->arguments[n].length+1);
    base->arguments[n].codepoints = (codepoint_t*)malloc(length);
    memcpy(base->arguments[n].codepoints, clone->arguments[n].codepoints, length);
  }
  return base;
}

// Create configuration entry assigned value
struct config_value* config_value_create(codepoint_t* value_codepoints, size_t value_length) {
  struct config_value* base = malloc(sizeof(struct config_value));
  config_value_create_inplace(base, value_codepoints, value_length);
  return base;
}

void config_value_create_inplace(struct config_value* base, codepoint_t* value_codepoints, size_t value_length) {
  base->cached = 0;
  base->parsed = 0;
  base->codepoints = malloc(sizeof(codepoint_t)*value_length);
  base->length = value_length;
  list_create_inplace(&base->commands, sizeof(struct config_command));
  memcpy(base->codepoints, value_codepoints, sizeof(codepoint_t)*value_length);
  config_value_parse_command(base);
}

// Create argument and command tables from command string
void config_value_parse_command(struct config_value* base) {
  codepoint_t* pos = base->codepoints;
  int string = 0;
  int escape = 0;
  size_t argument = 0;
  struct config_argument arguments[256];
  codepoint_t* tmp = (codepoint_t*)malloc(sizeof(codepoint_t)*base->length);
  codepoint_t* build = tmp;

  while (1) {
    codepoint_t cp = *pos++;
    if (cp==0 || ((cp==' ' || cp=='\t' || cp==';') && !string && !escape)) {
      if (build!=tmp && argument<sizeof(arguments)/sizeof(codepoint_t*)) {
        *build = 0;
        size_t length = sizeof(codepoint_t)*(size_t)(build-tmp+1);
        arguments[argument].codepoints = (codepoint_t*)malloc(length);
        memcpy(arguments[argument].codepoints, tmp, length);
        arguments[argument].length = (size_t)(build-tmp);
        argument++;
      }
      if (cp==';' || cp==0) {
        if (argument>0) {
          list_insert_empty(&base->commands, base->commands.last);
          config_command_create_inplace((struct config_command*)list_object(base->commands.last));
        }
        if (base->commands.last) {
          struct config_command* command = (struct config_command*)list_object(base->commands.last);
          command->length = argument;
          size_t length = sizeof(struct config_argument)*argument;
          command->arguments = malloc(length);
          memcpy(command->arguments, &arguments[0], length);
        }
      }
      if (cp==0) {
        break;
      }
      if (cp==';') {
        argument = 0;
      }
      build = tmp;
    } else if (cp=='\\' && !escape) {
      escape = 1;
    } else if (cp=='"' && !string && !escape) {
      string = 1;
    } else if (cp=='"' && string && !escape) {
      string = 0;
    } else {
      escape = 0;
      *build++ = cp;
    }
  }
  free(tmp);
}

void config_value_destroy(struct config_value* base) {
  config_value_destroy_inplace(base);
  free(base);
}

void config_value_destroy_inplace(struct config_value* base) {
  while (base->commands.first) {
    config_command_destroy_inplace((struct config_command*)list_object(base->commands.first));
    list_remove(&base->commands, base->commands.first);
  }
  list_destroy_inplace(&base->commands);
  free(base->codepoints);
}

// Duplicate node
struct config_value* config_value_clone(struct config_value* base) {
  return config_value_create(base->codepoints, base->length);
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
    config_value_destroy_inplace((struct config_value*)list_object(base->values->first));
    list_remove(base->values, base->values->first);
  }
}

// Load configuration file from disk or memory
void config_load(struct config* base, const char* filename) {
  struct document_file* file = document_file_create(0, 0, NULL);
  if (filename) {
    document_file_load(file, filename, 0, 0);
  } else {
    document_file_load_memory(file, (const uint8_t*)config_default, strlen(config_default));
  }

  if (file->buffer) {
    struct stream stream;
    stream_from_page(&stream, range_tree_first(file->buffer), 0);
    codepoint_t keyword_codepoints[1024];
    size_t keyword_length = 0;
    codepoint_t value_codepoints[1024];
    size_t value_length = 0;
    size_t bracket_positions[1024];
    size_t brackets = 0;

    int value = 0;
    int escape = 0;
    int string = 0;
    while (!stream_end(&stream)) {
      size_t length;
      codepoint_t cp = (*file->encoding->decode)(file->encoding, &stream, &length);

      int append = 0;
      if (!string && !escape) {
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
        } else if (cp=='"') {
          string = 1;
          if (value) {
            append = 1;
          }
        } else if (cp==':') {
          value = 1;
        } else if (cp=='\r' || cp=='\n' || ((cp=='\t' || cp==' ') && !value)) {
        } else {
          append = 1;
        }
      } else {
        if (!escape) {
          if (cp=='"' || cp=='\r' || cp=='\n') {
            string = 0;
          } else if (cp=='\\') {
            escape = 1;
          } else {
            append = 1;
          }
          if (value) {
            append = 1;
          }
        } else {
          escape = 0;
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
    stream_destroy(&stream);
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
  config_value_create_inplace((struct config_value*)list_object(node), value_codepoints, value_length);

  struct trie_node* parent = NULL;
  while (keyword_length-->0) {
    parent = trie_append_codepoint(base->keywords, parent, *keyword_codepoints, 0);
    if (keyword_length==0) {
      if (parent->end) {
        config_value_destroy_inplace((struct config_value*)list_object(*(struct list_node**)trie_object(parent)));
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
struct config_argument* config_value(struct trie_node* parent) {
  if (parent && parent->end) {
    struct config_value* value = (struct config_value*)list_object(*(struct list_node**)trie_object(parent));
    if (!value->commands.first) {
      return NULL;
    }
    struct config_command* command = (struct config_command*)list_object(value->commands.first);
    if (command->length==0) {
      return NULL;
    }
    return &command->arguments[0];
  }

  return NULL;
}

// Convert code points to encoding
uint8_t* config_convert_encoding_plain(struct config_argument* argument, struct encoding* encoding) {
  if (!argument) {
    return (uint8_t*)strdup("");
  }

  struct encoding* native = encoding_native_create();
  uint8_t* string = encoding_transform_plain((uint8_t*)argument->codepoints, argument->length*sizeof(codepoint_t), native, encoding);
  encoding_native_destroy(native);
  return string;
}

// Convert value to encoding
uint8_t* config_convert_encoding(struct trie_node* parent, struct encoding* encoding) {
  return config_convert_encoding_plain(config_value(parent), encoding);
}

// Convert code points to integer
int64_t config_convert_int64_plain(struct config_argument* argument) {
  if (!argument) {
    return 0;
  }

  int negate = 0;
  int64_t value = 0;
  codepoint_t* codepoints = argument->codepoints;
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
    struct config_command* command = (struct config_command*)list_object(value->commands.first);
    if (command->length==0) {
      value->value = 0;
      return value->value;
    }

    value->value = config_command_cache(command, cache);
    return value->value;
  }

  return 0;
}

// Assign keyword and cache
int64_t config_command_cache(struct config_command* base, struct config_cache* cache) {
  if (base->cached) {
    return base->value;
  }

  base->cached = 1;
  if (base->length==0) {
    base->value = 0;
    return base->value;
  }

  struct config_argument* argument = &base->arguments[0];

  while (cache->text) {
    codepoint_t* left = argument->codepoints;
    if (!left) {
      break;
    }
    const char* right = cache->text;
    while ((*left && (codepoint_t)*right) && (*left==(codepoint_t)*right)) {
      left++;
      right++;
    }

    if (!*left && !*right) {
      base->value = cache->value;
      return base->value;
    }
    cache++;
  }

  base->value = config_convert_int64_plain(argument);
  return base->value;
}
