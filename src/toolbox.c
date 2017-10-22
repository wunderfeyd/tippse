// Tippse - Toolbox - Command interface

#include "toolbox.h"

struct toolbox_command toolbox_commands[] = {
  {"Close", toolbox_command_close},
  {"Quit", toolbox_command_quit},
  {"Save", toolbox_command_save},
  {"Save all", toolbox_command_save_all},
  {NULL,  NULL}
};


// Create toolbox
struct toolbox* toolbox_create() {
  struct toolbox* base = (struct toolbox*)malloc(sizeof(struct toolbox));
  return base;
}

// Destroy toolbox
void toolbox_destroy(struct toolbox* base) {
  free(base);
}

// Return list of toolbox elements into virtual file
void toolbox_list(struct toolbox* base, struct document_file* file) {
}

// Close current document
void toolbox_command_close(struct toolbox* base, struct document_file* file) {
}

// Exit to shell
void toolbox_command_quit(struct toolbox* base, struct document_file* file) {
}

// Save current document
void toolbox_command_save(struct toolbox* base, struct document_file* file) {
}

// Save all documents
void toolbox_command_save_all(struct toolbox* base, struct document_file* file) {
}
