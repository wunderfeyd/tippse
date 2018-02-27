#ifndef TIPPSE_DOCUMENTFILE_H
#define TIPPSE_DOCUMENTFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

struct document_file;
struct document_view;
struct file_type;
struct range_tree_node;
struct list;
struct encoding;
struct config;
struct editor;

#include "visualinfo.h"

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

struct document_file_defaults {
  int colors[VISUAL_FLAG_COLOR_MAX];    // color values

  int invisibles;                       // show invisibles?
  int wrapping;                         // show line wrapping?

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
  struct range_tree_node* buffer;       // access to document buffer, root page
  struct file_cache* cache;             // base file cache that points directly to the selected file
  struct range_tree_node* bookmarks;    // access to bookmarks
  struct list* undos;                   // undo information
  struct list* redos;                   // redo information
  struct file_type* type;               // file type
  struct encoding* encoding;            // file encoding
  struct config* config;                // configuration
  struct editor* editor;                // The editor instance the file belongs to
  int tabstop;                          // type of tabstop
  int tabstop_width;                    // number of spaces per tab
  int newline;                          // type of newline, e.g. Unix or DOS
  int binary;                           // binary file?
  int line_select;                      // Selection list?
  int draft;                            // File is draft / new and never saved

  char* filename;                       // file name
  int save;                             // file can be saved, is real file?
  size_t undo_save_point;               // last save point in undo information

  struct document_file_defaults defaults; // configuration
  struct document_view* view;           // last used view
  struct list* views;                   // available views

  struct list* caches;                  // attached caches

  file_offset_t autocomplete_offset;    // current auto complete scan offset
  struct trie* autocomplete_last;       // last auto complete full tree
  struct trie* autocomplete_build;      // auto complete build tree
  int autocomplete_rescan;              // file was changed rescan entire document

  int pipefd[2];                        // Pipe to child process
  pid_t pid;                            // Child process id
};

#include "misc.h"
#include "list.h"
#include "rangetree.h"
#include "documentundo.h"
#include "documentview.h"
#include "config.h"
#include "filecache.h"
#include "filetype.h"
#include "filetype/c.h"
#include "filetype/sql.h"
#include "filetype/text.h"
#include "filetype/lua.h"
#include "filetype/patch.h"
#include "filetype/php.h"
#include "filetype/xml.h"
#include "encoding.h"
#include "encoding/utf8.h"
#include "encoding/utf16le.h"
#include "encoding/utf16be.h"
#include "encoding/cp850.h"
#include "encoding/ascii.h"
#include "editor.h"
#include "unicode.h"

struct document_file* document_file_create(int save, int config, struct editor* editor);
void document_file_clear(struct document_file* base, int all);
void document_file_destroy(struct document_file* base);
void document_file_name(struct document_file* base, const char* filename);
void document_file_draft(struct document_file* base);
int document_file_drafted(struct document_file* base);
void document_file_encoding(struct document_file* base, struct encoding* encoding);
void document_file_create_pipe(struct document_file* base);
void document_file_pipe(struct document_file* base, const char* command);
void document_file_fill_pipe(struct document_file* base, uint8_t* buffer, size_t length);
void document_file_close_pipe(struct document_file* base);
void document_file_load(struct document_file* base, const char* filename, int reload, int reset);
void document_file_load_memory(struct document_file* base, const uint8_t* buffer, size_t length);
int document_file_save_plain(struct document_file* base, const char* filename);
int document_file_save(struct document_file* base, const char* filename);

void document_file_detect_properties(struct document_file* base);
void document_file_detect_properties_stream(struct document_file* base, struct stream* document_stream);

void document_file_reload_config(struct document_file* base);

void document_file_expand_all(struct document_file* base, file_offset_t offset, file_offset_t length);
void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_insert(struct document_file* base, file_offset_t offset, const uint8_t* text, size_t length);
void document_file_insert_buffer(struct document_file* base, file_offset_t offset, struct range_tree_node* buffer);

void document_file_reduce_all(struct document_file* base, file_offset_t offset, file_offset_t length);
void document_file_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_delete(struct document_file* base, file_offset_t offset, file_offset_t length);

void document_file_relocate(file_offset_t* pos, file_offset_t from, file_offset_t to, file_offset_t length);
void document_file_move(struct document_file* base, file_offset_t from, file_offset_t to, file_offset_t length);

void document_file_change_views(struct document_file* base);
void document_file_reset_views(struct document_file* base);

void document_file_reference_cache(struct document_file* base, struct file_cache* cache);
void document_file_dereference_cache(struct document_file* base, struct file_cache* cache);
int document_file_modified_cache(struct document_file* base);
#endif /* #ifndef TIPPSE_DOCUMENTFILE_H */
