// Tippse - Clipboard - Holds and sets clipboard data (interfaces to the system if possible)

#include "clipboard.h"

#include "document_hex.h"
#include "fragment.h"
#include "rangetree.h"
#include "stream.h"
#include "encoding.h"

static struct range_tree* clipboard_data = NULL;
static struct encoding* clipboard_encoding = NULL;

// Free clipboard data
void clipboard_free(void) {
  if (clipboard_data) {
    range_tree_destroy(clipboard_data);
  }

  if (clipboard_encoding) {
    clipboard_encoding->destroy(clipboard_encoding);
  }
}

// Write text to clipboard
void clipboard_set(struct range_tree* data, int binary, struct encoding* encoding) {
#ifndef _TESTSUITE
#ifdef __APPLE__
  clipboard_command_set(data, binary, encoding, "pbcopy");
#elif __linux__
  clipboard_command_set(data, binary, encoding, "xsel -i -p 2>/dev/null");
  clipboard_command_set(data, binary, encoding, "xsel -i -b 2>/dev/null");
#elif _WIN32
  clipboard_windows_set(data, binary);
#endif
#endif
  if (clipboard_data) {
    range_tree_destroy(clipboard_data);
  }
  clipboard_data = data;

  if (clipboard_encoding) {
    clipboard_encoding->destroy(clipboard_encoding);
  }
  clipboard_encoding = encoding->create();
}

// Write text to system clipboard
void clipboard_command_set(struct range_tree* data, int binary, struct encoding* encoding, const char* command) {
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
      struct range_tree* transform = encoding_transform_page(data->root, 0, FILE_OFFSET_T_MAX, encoding, encoding_utf8_static());
      struct stream stream;
      stream_from_page(&stream, range_tree_first(transform), 0);
      while (!stream_end(&stream)) {
        size_t length = stream_cache_length(&stream)-stream_displacement(&stream);
        fwrite(stream_buffer(&stream), 1, length, pipe);
        stream_next(&stream);
      }
      stream_destroy(&stream);
      range_tree_destroy(transform);
    }
    pclose(pipe);
  }
}

#ifdef _WINDOWS
void clipboard_windows_set(struct range_tree* data, int binary) {
  //TODO: implement clipboard for Windows
}
#endif

// Get text from clipboard
struct range_tree* clipboard_get(struct encoding** encoding) {
  struct range_tree* data = NULL;
#ifndef _TESTSUITE
#ifdef __APPLE__
  data = clipboard_command_get(encoding, "pbpaste");
#elif __linux__
  data = clipboard_command_get(encoding, "xsel -o 2>/dev/null");
#elif _WIN32
  data = clipboard_windows_get();
#endif
#endif
  if (!data) {
    data = clipboard_data;
  }

  if (encoding && !*encoding) {
    *encoding = clipboard_encoding?clipboard_encoding:encoding_utf8_static();
  }

  return data;
}

// Get text from system clipboard
struct range_tree* clipboard_command_get(struct encoding** encoding, const char* command) {
  struct range_tree* data = range_tree_create(NULL, 0);
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
          range_tree_insert(data, offset, fragment, 0, length/3, 0, 0, NULL);
          fragment_dereference(fragment, NULL);
        } else {
          free(buffer);
        }
      }
    } else {
      if (length) {
        struct fragment* fragment = fragment_create_memory(buffer, length);
        range_tree_insert(data, 0, fragment, 0, length, 0, 0, NULL);
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
          range_tree_insert(data, offset, fragment, 0, length, 0, 0, NULL);
          fragment_dereference(fragment, NULL);
        } else {
          free(buffer);
        }
      }

      if (data->root && encoding && !*encoding) {
        *encoding = encoding_utf8_static();
      }
    }
    pclose(pipe);
  }
  return data;
}

#ifdef _WINDOWS
struct range_tree* clipboard_windows_get(void) {
  //TODO: implement clipboard for Windows
  return NULL;
}
#endif
