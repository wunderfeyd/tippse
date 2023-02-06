#ifndef TIPPSE_DOCUMENTFILE_H
#define TIPPSE_DOCUMENTFILE_H

#include <stdlib.h>
#include <stdio.h>
#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "visualinfo.h"
#include "spell.h"
#include "library/rangetree.h"
#include "library/thread.h"
#include "library/mutex.h"
#define TIPPSE_RANGETREE_CAPS_VISUAL (TIPPSE_RANGETREE_CAPS_USER<<0)

struct range_tree_callback_hook {
  struct range_tree_callback callback;
  struct document_file* file;
};

#define TIPPSE_DOCUMENT_MEMORY_LOADMAX 1024*1024
#define TIPPSE_DOCUMENT_AUTO_LOADMAX 1024*16

#define TIPPSE_TABSTOP_AUTO 0
#define TIPPSE_TABSTOP_TAB 1
#define TIPPSE_TABSTOP_SPACE 2
#define TIPPSE_TABSTOP_MAX 3

#define TIPPSE_NEWLINE_AUTO 0
#define TIPPSE_NEWLINE_LF 1
#define TIPPSE_NEWLINE_CR 2
#define TIPPSE_NEWLINE_CRLF 3
#define TIPPSE_NEWLINE_MAX 4

#define TIPPSE_PIPE_DONE -1
#define TIPPSE_PIPE_FREE 0
#define TIPPSE_PIPE_ACTIVE 1

#define TIPPSE_PIPEOP_SEARCH 0
#define TIPPSE_PIPEOP_EXECUTE 1

struct document_file_pipe_operation {
  int operation;
  struct document_file_pipe_operation_search {
    int binary;
    int ignore_case;
    int regex;
    char* pattern_text;
    char* path;
    struct range_tree* buffer;
    struct encoding* encoding;
  } search;
  struct document_file_pipe_operation_execute {
    char* shell;
  } execute;
};

struct document_file_pipe_block {
  void* data;
  size_t length;
};

struct document_file_defaults {
  int colors[VISUAL_FLAG_COLOR_MAX];    // color values

  int invisibles;                       // show invisibles?
  int wrapping;                         // show line wrapping?
  int overwrite;                        // overwrite enabled?
  int spellcheck;                       // spell checking enabled?
  int address_width;                    // width of the position column in characters
  int line_width;                       // width of the file data in characters per line
  int hex_width;                        // width of hex data in bytes

  int tabstop;                          // type of tabstop
  int tabstop_width;                    // number of spaces per tab
  int newline;                          // type of newline, e.g. Unix or DOS
};

struct document_file_parser {
  const char* name;                      // parser name
  struct file_type* (*constructor)(struct config* config, const char* file_type); // file type
};

struct document_file_cache {
  int count;                            // count of used fragments
  struct file_cache* cache;             // reference to cache
};

struct document_file {
  struct range_tree buffer;             // access to document buffer, root page
  struct file_cache* cache;             // base file cache that points directly to the selected file
  struct range_tree bookmarks;          // access to bookmarks
  struct list* undos;                   // undo information
  struct list* redos;                   // redo information
  struct file_type* type;               // file type
  struct encoding* encoding;            // file encoding
  struct config* config;                // configuration
  struct editor* editor;                // The editor instance the file belongs to
  struct splitter* splitter;            // Preferred splitter to use
  struct spell* spellcheck;             // Spell checking
  int tabstop;                          // type of tabstop
  int tabstop_width;                    // number of spaces per tab
  int newline;                          // type of newline, e.g. Unix or DOS
  int binary;                           // binary file?
  int line_select;                      // Selection list?
  int draft;                            // file is draft / new and never saved
  int undo;                             // undo enabled

  char* filename;                       // file name
  int save;                             // is real file?
  int save_skip;                        // user doesn't permit save?
  size_t undo_save_point;               // last save point in undo information

  struct document_file_defaults defaults; // configuration
  int view_inactive;                    // last view in list is inactive
  struct list* views;                   // available views

  struct list* caches;                  // attached caches

  file_offset_t autocomplete_offset;    // current auto complete scan offset
  struct trie* autocomplete_last;       // last auto complete full tree
  struct trie* autocomplete_build;      // auto complete build tree
  int autocomplete_rescan;              // file was changed rescan entire document

  struct range_tree_callback_hook hook; // range tree hooks
  struct thread thread_pipe;            // thread executing asynchronous output tasks
  int piped;                            // async output task is active
  struct list* pipe_queue;              // incoming pipe data
  struct mutex pipe_mutex;              // pipe lock
  struct document_file_pipe_operation* pipe_operation; // operation to be made on pipe
};

struct document_file_pipe_operation* document_file_pipe_operation_create();
void document_file_pipe_operation_destroy(struct document_file_pipe_operation* base);

struct document_file* document_file_create(int save, int config, struct editor* editor);
void document_file_clear(struct document_file* base, int all);
void document_file_destroy(struct document_file* base);
void document_file_name(struct document_file* base, const char* filename);
void document_file_draft(struct document_file* base);
int document_file_drafted(struct document_file* base);
void document_file_encoding(struct document_file* base, struct encoding* encoding);
void document_file_create_pipe(struct document_file* base, struct document_file_pipe_operation* pipe_operation);
void document_file_fill_pipe(struct document_file* base, uint8_t* buffer, size_t length);
void document_file_flush_pipe(struct document_file* base);
void document_file_close_pipe(struct document_file* base);
void document_file_shutdown_pipe(struct document_file* base);
void document_file_kill_pipe(struct document_file* base);
void document_file_load(struct document_file* base, const char* filename, int reload, int reset);
void document_file_load_memory(struct document_file* base, const uint8_t* buffer, size_t length, const char* name);
int document_file_save_plain(struct document_file* base, const char* filename);
int document_file_save(struct document_file* base, const char* filename);
void document_file_save_skip(struct document_file* base);

void document_file_detect_properties(struct document_file* base);
void document_file_detect_properties_stream(struct document_file* base, struct stream* document_stream);

void document_file_reload_config(struct document_file* base);

void document_file_expand_all(struct document_file* base, file_offset_t offset, file_offset_t length);
void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_insert(struct document_file* base, file_offset_t offset, const uint8_t* text, size_t length, int inserter);
void document_file_insert_utf8(struct document_file* base, file_offset_t offset, const char* text, size_t length, int inserter);
void document_file_insert_buffer(struct document_file* base, file_offset_t offset, struct range_tree_node* buffer);

void document_file_reduce_all(struct document_file* base, file_offset_t offset, file_offset_t length);
void document_file_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_delete(struct document_file* base, file_offset_t offset, file_offset_t length);
void document_file_empty(struct document_file* base);

void document_file_relocate(file_offset_t* pos, file_offset_t from, file_offset_t to, file_offset_t length, int inclusive);
void document_file_move(struct document_file* base, file_offset_t from, file_offset_t to, file_offset_t length);

void document_file_change_views(struct document_file* base, int defaults);
void document_file_reset_views(struct document_file* base, int defaults);
void document_file_invalidate_view_node(struct document_file* base, struct range_tree_node* node, struct range_tree* tree);
void document_file_destroy_view_node(struct document_file* base, struct range_tree_node* node);
void document_file_combine_view_node(struct document_file* base, struct range_tree_node* node);

void document_file_reference_cache(struct document_file* base, struct file_cache* cache);
void document_file_dereference_cache(struct document_file* base, struct file_cache* cache);
int document_file_modified_cache(struct document_file* base);
void document_file_invalidate_cache(struct document_file* base, struct file_cache* cache);

void document_file_fragment_reference(struct range_tree_callback* base, struct fragment* fragment);
void document_file_fragment_dereference(struct range_tree_callback* base, struct fragment* fragment);
void document_file_node_combine(struct range_tree_callback* base, struct range_tree_node* node, struct range_tree* tree);
void document_file_node_invalidate(struct range_tree_callback* base, struct range_tree_node* node, struct range_tree* tree);
void document_file_node_destroy(struct range_tree_callback* base, struct range_tree_node* node, struct range_tree* tree);

void document_file_autocomplete_destroy(struct document_file* base);

#endif /* #ifndef TIPPSE_DOCUMENTFILE_H */
