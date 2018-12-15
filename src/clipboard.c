// Tippse - Clipboard - Holds and sets clipboard data (interfaces to the system if possible)

#include "clipboard.h"

#include "document_hex.h"
#include "fragment.h"
#include "rangetree.h"
#include "stream.h"

static struct range_tree_node* clipboard = NULL;

// Free clipboard data
void clipboard_free(void) {
  range_tree_destroy(clipboard, NULL);
}

// Write text to clipboard
void clipboard_set(struct range_tree_node* data, int binary) {
#ifdef __APPLE__
  clipboard_command_set(data, binary, "pbcopy");
#elif __linux__
  clipboard_command_set(data, binary, "xsel -i -p 2>/dev/null");
  clipboard_command_set(data, binary, "xsel -i -b 2>/dev/null");
#elif _WIN32
  clipboard_windows_set(data, binary);
#endif
  if (clipboard) range_tree_destroy(clipboard, NULL);
  clipboard = data;
}

// Write text to system clipboard
void clipboard_command_set(struct range_tree_node* data, int binary, const char* command) {
  FILE* pipe = popen(command, "w");
  if (pipe) {
    if (binary) {
      fwrite("hexdump plain/text\n", 1, 19, pipe);
      struct stream stream;
      stream_from_page(&stream, range_tree_first(data), 0);
      while (!stream_end(&stream)) {
        size_t length = stream_cache_length(&stream)-stream_displacement(&stream);
        char* buffer = (char*)malloc(length*3+1);
        for (file_offset_t pos = 0; pos<length; pos++) sprintf(buffer+3*pos, "%02x ", *(stream_buffer(&stream)+pos));
        fwrite(buffer, 1, length*3, pipe);
        free(buffer);
        stream_next(&stream);
      }
      stream_destroy(&stream);
    } else {
      struct stream stream;
      stream_from_page(&stream, range_tree_first(data), 0);
      while (!stream_end(&stream)) {
        size_t length = stream_cache_length(&stream)-stream_displacement(&stream);
        fwrite(stream_buffer(&stream), 1, length, pipe);
        stream_next(&stream);
      }
      stream_destroy(&stream);
    }
    pclose(pipe);
  }
}

#ifdef _WINDOWS
void clipboard_windows_set(struct range_tree_node* data, int binary) {
  //TODO: implement clipboard for Windows
}
#endif

// Get text from clipboard
struct range_tree_node* clipboard_get(void) {
  struct range_tree_node* data = NULL;
#ifdef __APPLE__
  data = clipboard_command_get("pbpaste");
#elif __linux__
  data = clipboard_command_get("xsel -o 2>/dev/null");
#elif _WIN32
  data = clipboard_windows_get();
#endif
  if (data) {
    if (clipboard) range_tree_destroy(clipboard, NULL);
    clipboard = data;
  }

  return clipboard;
}

// Get text from system clipboard
struct range_tree_node* clipboard_command_get(const char* command) {
  struct range_tree_node* data = NULL;
  FILE* pipe = popen(command, "r");
  if (pipe) {
    uint8_t* buffer = (uint8_t*)malloc(19);
    file_offset_t length = fread(buffer, 1, 19, pipe);
    if (length==19 && memcmp(buffer, "hexdump plain/text\n", 19)==0) {
      free(buffer);
      while (!feof(pipe)) {
        uint8_t* buffer = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MIN*3);
        file_offset_t length = fread(buffer, 1, TREE_BLOCK_LENGTH_MIN*3, pipe);
        if (length) {
          file_offset_t offset = range_tree_length(data);
          for (file_offset_t pos = 0; pos<length/3; pos++) *(buffer+pos) = document_hex_value_from_string((const char*)buffer+pos*3, 3);
          struct fragment* fragment = fragment_create_memory(buffer, length/3);
          data = range_tree_insert(data, offset, fragment, 0, length/3, 0, 0, NULL, NULL);
          fragment_dereference(fragment, NULL);
        } else {
          free(buffer);
        }
      }
    } else {
      if (length) {
        struct fragment* fragment = fragment_create_memory(buffer, length);
        data = range_tree_insert(data, 0, fragment, 0, length, 0, 0, NULL, NULL);
        fragment_dereference(fragment, NULL);
      } else {
        free(buffer);
      }
      while (!feof(pipe)) {
        uint8_t* buffer = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MIN);
        file_offset_t length = fread(buffer, 1, TREE_BLOCK_LENGTH_MIN, pipe);
        if (length) {
          file_offset_t offset = range_tree_length(data);
          struct fragment* fragment = fragment_create_memory(buffer, length);
          data = range_tree_insert(data, offset, fragment, 0, length, 0, 0, NULL, NULL);
          fragment_dereference(fragment, NULL);
        } else {
          free(buffer);
        }
      }
    }
    pclose(pipe);
  }
  return data;
}

#ifdef _WINDOWS
struct range_tree_node* clipboard_windows_get(void) {
  //TODO: implement clipboard for Windows
  return NULL;
}
#endif
