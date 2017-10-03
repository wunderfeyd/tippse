/* Tippse - Clipboard - Holds and sets clipboard data (interfaces to the system if possible) */

#include "clipboard.h"

struct range_tree_node* clipboard = NULL;

// Write text to clipboard
void clipboard_set(struct range_tree_node* data) {
#ifdef __APPLE__
  clipboard_command_set(data, "pbcopy");
#elif __linux__
  clipboard_command_set(data, "xsel -i -p");
  clipboard_command_set(data, "xsel -i -b");
#elif _WIN32
  clipboard_windows_set(data);
#endif
  if (clipboard) range_tree_destroy(clipboard);
  clipboard = data;
}

// Write text to system clipboard
void clipboard_command_set(struct range_tree_node* data, const char* command) {
  FILE* pipe = popen(command, "w");
  if (pipe) {
    struct range_tree_node* node = range_tree_first(data);
    while (node) {
      fwrite(node->buffer->buffer+node->offset, 1, node->length, pipe);
      node = range_tree_next(node);
    }
    pclose(pipe);
  }
}

#ifdef _WIN32
void clipboard_windows_set(struct range_tree_node* data, const char* command) {
  //TODO: implement clipboard for Windows
}
#endif

// Get text from clipboard
struct range_tree_node* clipboard_get() {
  struct range_tree_node* data = NULL;
#ifdef __APPLE__
  data = clipboard_command_get("pbpaste");
#elif __linux__
  data = clipboard_command_get("xsel -o");
#elif _WIN32
  data = clipboard_windows_get();
#endif
  return data?data:clipboard;
}

// Get text from system clipboard
struct range_tree_node* clipboard_command_get(const char* command) {
  struct range_tree_node* data = NULL;
  FILE* pipe = popen(command, "r");
  if (pipe) {
    while (!feof(pipe)) {
      uint8_t* buffer = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MAX);
      file_offset_t length = fread(buffer, 1, TREE_BLOCK_LENGTH_MAX, pipe);
      if(length) {
        file_offset_t offset = data?data->length:0;
        struct fragment* fragment = fragment_create_memory(buffer, length);
        data = range_tree_insert(data, offset, fragment, 0, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER);
      } else {
        free(buffer);
      }
    }
    pclose(pipe);
  }
  return data;
}

#ifdef _WIN32
struct range_tree_node* clipboard_windows_get(const char* command) {
  //TODO: implement clipboard for Windows
  return NULL;
}
#endif
