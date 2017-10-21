#ifndef TIPPSE_DOCUMENTFILE_H
#define TIPPSE_DOCUMENTFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

struct document_file;

#define TIPPSE_DOCUMENT_MEMORY_LOADMAX 1024*1024

#define TIPPSE_TABSTOP_AUTO 0
#define TIPPSE_TABSTOP_TAB 1
#define TIPPSE_TABSTOP_SPACE 2
#define TIPPSE_TABSTOP_MAX 3

#define TIPPSE_NEWLINE_AUTO 0
#define TIPPSE_NEWLINE_LF 1
#define TIPPSE_NEWLINE_CR 2
#define TIPPSE_NEWLINE_CRLF 3
#define TIPPSE_NEWLINE_MAX 4

#include "misc.h"
#include "list.h"
#include "rangetree.h"
#include "documentundo.h"
#include "documentview.h"
#include "filetype.h"
#include "filetype/c.h"
#include "filetype/cpp.h"
#include "filetype/sql.h"
#include "filetype/text.h"
#include "filetype/js.h"
#include "filetype/lua.h"
#include "filetype/patch.h"
#include "filetype/php.h"
#include "filetype/xml.h"
#include "encoding.h"
#include "encoding/utf8.h"
#include "encoding/cp850.h"
#include "encoding/ascii.h"
#include "config.h"

struct document_file_defaults {
  int colors[VISUAL_FLAG_COLOR_MAX];    // color values

  int invisibles;                       // show invisibles?
  int wrapping;                         // show line wrapping?
  int continuous;                       // ignore line wrapping?

  int tabstop;                          // type of tabstop
  int tabstop_width;                    // number of spaces per tab
  int newline;                          // type of newline, e.g. Unix or DOS
};

struct document_file_type {
  const char* extension;                // file extension
  struct file_type* (*constructor)(void); // file type
};

struct document_file {
  struct range_tree_node* buffer;       // access to document buffer, root page
  struct range_tree_node* bookmarks;    // access to bookmarks
  struct list* undos;                   // undo information
  struct list* redos;                   // redo information
  struct file_type* type;               // file type
  struct encoding* encoding;            // file encoding
  struct config* config;                // configuration
  int tabstop;                          // type of tabstop
  int tabstop_width;                    // number of spaces per tab
  int newline;                          // type of newline, e.g. Unix or DOS
  int binary;                           // binary file?

  char* filename;                       // file name
  int save;                             // file can be saved, is real file?
  size_t undo_save_point;               // last save point in undo information

  struct document_file_defaults defaults; // configuration
  struct document_view* view;           // last used view
  struct list* views;                   // available views
};

struct document_file* document_file_create(int save);
void document_file_clear(struct document_file* file);
void document_file_destroy(struct document_file* file);
void document_file_name(struct document_file* file, const char* filename);
void document_file_encoding(struct document_file* file, struct encoding* encoding);
void document_file_load(struct document_file* file, const char* filename);
void document_file_load_memory(struct document_file* file, const uint8_t* buffer, size_t length);
int document_file_save_plain(struct document_file* file, const char* filename);
void document_file_save(struct document_file* file, const char* filename);

void document_file_detect_properties(struct document_file* file);

void document_file_reload_config(struct document_file* file);

void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_insert(struct document_file* file, file_offset_t offset, const uint8_t* text, size_t length);
void document_file_insert_buffer(struct document_file* file, file_offset_t offset, struct range_tree_node* buffer);

void document_file_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_file_delete(struct document_file* file, file_offset_t offset, file_offset_t length);
int document_file_delete_selection(struct document_file* file, struct document_view* view);
void document_file_manualchange(struct document_file* file);

#endif /* #ifndef TIPPSE_DOCUMENTFILE_H */
