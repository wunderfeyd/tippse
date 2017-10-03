#ifndef __TIPPSE_CONFIG__
#define __TIPPSE_CONFIG__

#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "trie.h"
#include "rangetree.h"
#include "encoding.h"
#include "documentfile.h"

struct config {
  struct trie* keywords;
  struct list* values;
};

struct config* config_create();
void config_destroy(struct config* config);
void config_clear(struct config* config);

void config_load(struct config* config, const char* filename);
void config_loadpaths(struct config* config, const char* filename, int strip);

void config_update(struct config* config, int* keyword_codepoints, size_t keyword_length, int* value_codepoints, size_t value_length);
int* config_find_codepoints(struct config* config, int* keyword_codepoints, size_t keyword_length);
int* config_find_ascii(struct config* config, const char* keyword);

char* config_convert_ascii(int* codepoints);
int64_t config_convert_int64(int* codepoints);
#endif /* #ifndef __TIPPSE_CONFIG__ */
