#ifndef __TIPPSE_DOCUMENTFILE__
#define __TIPPSE_DOCUMENTFILE__

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
#include "filetype.h"
#include "filetype/c.h"
#include "filetype/cpp.h"
#include "filetype/sql.h"
#include "filetype/text.h"
#include "filetype/lua.h"
#include "filetype/php.h"
#include "filetype/xml.h"
#include "encoding.h"
#include "encoding/utf8.h"

struct document_file_type {
  const char* extension;
  struct file_type* (*constructor)();
};

struct document_file {
  struct range_tree_node* buffer;
  struct list* undos;
  struct list* redos;
  struct file_type* type;
  struct encoding* encoding;
  int tabstop;
  int tabstop_width;
  int newline;

  char* filename;
  int modified;
  int save;

  struct list* views;
};

struct document_file* document_file_create(int save);
void document_file_clear(struct document_file* file);
void document_file_destroy(struct document_file* file);
void document_file_name(struct document_file* file, const char* filename);
void document_file_load(struct document_file* file, const char* filename);
int document_file_save_plain(struct document_file* file, const char* filename);
void document_file_save(struct document_file* file, const char* filename);

void document_file_detect_properties(struct document_file* file);

#endif /* #ifndef __TIPPSE_DOCUMENTFILE__ */
