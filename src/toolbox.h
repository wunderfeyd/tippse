#ifndef TIPPSE_TOOLBOX_H
#define TIPPSE_TOOLBOX_H

#include <stdlib.h>
#include <string.h>

#include "misc.h"
#include "documentfile.h"

struct toolbox {
};

struct toolbox_command {
  const char* text;                                                 // description of command
  void (*apply)(struct toolbox* base, struct document_file* file);  // destination function
};

struct toolbox* toolbox_create();
void toolbox_destroy(struct toolbox* base);
void toolbox_list(struct toolbox* base, struct document_file* file);

void toolbox_command_close(struct toolbox* base, struct document_file* file);
void toolbox_command_quit(struct toolbox* base, struct document_file* file);
void toolbox_command_save(struct toolbox* base, struct document_file* file);
void toolbox_command_save_all(struct toolbox* base, struct document_file* file);

#endif /* #ifndef TIPPSE_TOOLBOX_H */
