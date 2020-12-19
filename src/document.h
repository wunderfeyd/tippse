#ifndef TIPPSE_DOCUMENT_H
#define TIPPSE_DOCUMENT_H

#include <stdlib.h>
#include <string.h>
#include "types.h"

struct document {
  void (*reset)(struct document* base, struct document_view* view, struct document_file* file);
  void (*draw)(struct document* base, struct screen* screen, struct splitter* splitter);
  void (*keypress)(struct document* base, struct document_view* view, struct document_file* file, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y);
  int (*incremental_update)(struct document* base, struct document_view* view, struct document_file* file);
};

int document_search(struct document_file* file, struct document_view* view, struct range_tree* search_text, struct encoding* search_encoding, struct range_tree* replace_text, struct encoding* replace_encoding, int reverse, int ignore_case, int regex, int all, int replace);
void document_search_directory(struct thread* thread, struct document_file* file, const char* path, struct range_tree* search_text, struct encoding* search_encoding, struct range_tree* replace_text, struct encoding* replace_encoding, int ignore_case, int regex, int replace, const char* pattern_text, struct encoding* pattern_encoding, int binary);
void document_directory(struct document_file* file, struct stream* filter_stream, struct encoding* filter_encoding, const char* predefined);
void document_insert_search(struct document_file* file, struct search* search, const char* output, size_t length, int inserter);

void document_select_all(struct document_file* file, struct document_view* view, int update_offset);
void document_select_nothing(struct document_file* file, struct document_view* view);
int document_select_delete(struct document_file* file, struct document_view* view);
void document_clipboard_copy(struct document_file* file, struct document_view* view);
void document_clipboard_paste(struct document_file* file, struct document_view* view);
void document_clipboard_extend(struct document_file* file, struct document_view* view, file_offset_t from, file_offset_t to, int extend);

void document_bookmark_toggle_range(struct document_file* file, file_offset_t low, file_offset_t high);
void document_bookmark_range(struct document_file* file, file_offset_t low, file_offset_t high, int marked);
void document_bookmark_selection(struct document_file* file, struct document_view* view, int marked);
void document_bookmark_toggle_selection(struct document_file* file, struct document_view* view);
void document_bookmark_find(struct document_file* file, struct document_view* view, int reverse);

int document_keypress(struct document* base, struct document_view* view, struct document_file* file, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, file_offset_t selection_low, file_offset_t selection_high, int* selection_keep, int* seek, file_offset_t file_size, file_offset_t* offset_old);
#endif /* #ifndef TIPPSE_DOCUMENT_H */
