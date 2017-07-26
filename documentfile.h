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

#include "list.h"
#include "rangetree.h"
#include "filetype.h"
#include "filetype/c.h"
#include "filetype/cpp.h"
#include "filetype/sql.h"
#include "filetype/text.h"
#include "filetype/lua.h"
#include "filetype/php.h"
#include "filetype/xml.h"
#include "encoding_utf8.h"

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
void document_file_save(struct document_file* file, const char* filename);

#endif /* #ifndef __TIPPSE_DOCUMENTFILE__ */
