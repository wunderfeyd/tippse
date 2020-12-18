// Tippse - Clipboard - Holds and sets clipboard data (interfaces to the system if possible)

#include "clipboard.h"

#include "document_hex.h"
#include "library/fragment.h"
#include "library/rangetree.h"
#include "library/stream.h"
#include "library/encoding.h"
#include "library/encoding/utf8.h"
#include "library/encoding/utf16le.h"
#include "library/unicode.h"
#include "library/misc.h"

static struct range_tree* clipboard_data = NULL;
static struct encoding* clipboard_encoding = NULL;
const char* binary_marker = "hexdump plain/text\n";

#define TIPPSE_BINARY_LINE_LENGTH 128

// Free clipboard data
void clipboard_free(void) {
  if (clipboard_data) {
    range_tree_destroy(clipboard_data);
  }

  if (clipboard_encoding) {
    clipboard_encoding->destroy(clipboard_encoding);
  }
}

// File cache was invalidated, check files and copy
void clipboard_cache_invalidate(struct file_cache* cache) {
  range_tree_cache_invalidate(clipboard_data, cache);
}

// Write text to clipboard
void clipboard_set(struct range_tree* data, int binary, struct encoding* encoding) {
  struct range_tree* external_data = data;
#ifndef _TESTSUITE
  struct encoding* external_encoding = encoding;
#endif
  if (binary) {
#ifndef _TESTSUITE
    external_encoding = encoding_utf8_static();
#endif
    external_data = range_tree_create(NULL, 0);
    range_tree_insert_split(external_data, 0, (const uint8_t*)binary_marker, strlen(binary_marker), 0);

    char* buffer = NULL;
    size_t pos = 0;
    struct stream stream;
    stream_from_page(&stream, range_tree_first(data), 0);
    while (1) {
      int end = stream_end(&stream);
      if (pos==TIPPSE_BINARY_LINE_LENGTH*3 || end) {
        if (pos>0) {
          buffer[pos-1] = '\n';
          struct fragment* fragment = fragment_create_memory((uint8_t*)buffer, pos);
          range_tree_insert(external_data, range_tree_length(external_data), fragment, 0, pos, 0, 0, NULL);
          fragment_dereference(fragment, NULL);
        }

        if (end) {
          break;
        }
        pos = 0;
      }

      if (pos==0) {
        buffer = malloc(sizeof(char)*TIPPSE_BINARY_LINE_LENGTH*3);
      }

      sprintf(&buffer[pos], "%02x ", stream_read_forward(&stream));
      pos+=3;
    }
    stream_destroy(&stream);
  }

#ifndef _TESTSUITE
#ifdef __APPLE__
  clipboard_command_set(external_data, external_encoding, "pbcopy");
#elif __linux__
  clipboard_command_set(external_data, external_encoding, "((xclip -i --selection primary) || (xsel -i -p)) 2>/dev/null");
  clipboard_command_set(external_data, external_encoding, "((xclip -i --selection clipboard) || (xsel -i -b)) 2>/dev/null");
#elif _WINDOWS
  clipboard_command_set(external_data, external_encoding);
#endif
#endif

  if (external_data!=data) {
    range_tree_destroy(external_data);
  }

  if (clipboard_data) {
    range_tree_destroy(clipboard_data);
  }
  clipboard_data = data;

  if (clipboard_encoding) {
    clipboard_encoding->destroy(clipboard_encoding);
  }
  clipboard_encoding = encoding->create();
}

#ifdef _WINDOWS
void clipboard_command_set(struct range_tree* data, struct encoding* encoding) {
  struct encoding* utf16 = encoding_utf16le_create();
  struct range_tree* transform = encoding_transform_page(data->root, 0, FILE_OFFSET_T_MAX, encoding, utf16);
  utf16->destroy(utf16);

  if (transform) {
    if (OpenClipboard(NULL)) {
      wchar_t zero = 0;
      range_tree_insert_split(transform, range_tree_length(transform), (const uint8_t*)&zero, sizeof(zero), 0);

      file_offset_t length = range_tree_length(transform);
      HANDLE clipboard = GlobalAlloc(GMEM_MOVEABLE, length);
      if (clipboard) {
        WCHAR* data = (WCHAR*)GlobalLock(clipboard);
        uint8_t* raw = (uint8_t*)range_tree_raw(transform, 0, length);
        memcpy(data, raw, (size_t)length);
        free(raw);
        GlobalUnlock(clipboard);
        SetClipboardData(CF_UNICODETEXT, clipboard);
      }
    }

    CloseClipboard();
    range_tree_destroy(transform);
  }
}
#else
// Write text to system clipboard
void clipboard_command_set(struct range_tree* data, struct encoding* encoding, const char* command) {
  FILE* pipe = popen(command, "w");
  if (pipe) {
    struct range_tree* transform = encoding_transform_page(data->root, 0, FILE_OFFSET_T_MAX, encoding, encoding_utf8_static());
    if (transform) {
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
#endif

// Get text from clipboard
struct range_tree* clipboard_get(struct encoding** encoding, int* binary) {
  struct range_tree* data = NULL;
#ifndef _TESTSUITE
#ifdef __APPLE__
  data = clipboard_command_get(encoding, "pbpaste");
#elif __linux__
  data = clipboard_command_get(encoding, "((xclip -o --selection clipboard) || (xsel -o -b)) 2>/dev/null");
  /*if (!data) {
    data = clipboard_command_get(encoding, "((xclip -o --selection primary) || (xsel -o -p)) 2>/dev/null");
  }*/
#elif _WINDOWS
  data = clipboard_command_get(encoding);
#endif
#endif

  if (encoding && *encoding && data && data->root && binary) {
    size_t marker_length = strlen(binary_marker);
    char* marker = strdup(binary_marker);
    size_t pos = 0;
    struct stream stream;
    stream_from_page(&stream, range_tree_first(data), 0);
    struct unicode_sequencer sequencer;
    unicode_sequencer_clear(&sequencer, *encoding, &stream);
    while (!stream_end(&stream) && pos<marker_length) {
      codepoint_t cp = unicode_sequencer_find(&sequencer, 0)->cp[0];
      if (cp>=128) {
        break;
      }

      marker[pos] = (char)cp;
      unicode_sequencer_advance(&sequencer, 1);
      pos++;
    }

    marker[pos] = 0;
    if (pos==marker_length && memcmp(marker, binary_marker, marker_length)==0) {
      *binary = 1;
      pos = 0;
      struct range_tree* external_data = range_tree_create(NULL, 0);
      uint8_t* buffer = NULL;
      while (1) {
        int end = stream_end(&stream);
        if (end || pos==TREE_BLOCK_LENGTH_MIN) {
          struct fragment* fragment = fragment_create_memory((uint8_t*)buffer, pos);
          range_tree_insert(external_data, range_tree_length(external_data), fragment, 0, pos, 0, 0, NULL);
          fragment_dereference(fragment, NULL);

          if (end) {
            break;
          }
          pos = 0;
        }

        size_t offset = 0;
        uint64_t byte = decode_based_unsigned_offset(&sequencer, 16, &offset, 2);
        unicode_sequencer_find(&sequencer, offset);
        unicode_sequencer_advance(&sequencer, offset+1);

        if (pos==0) {
          buffer = malloc(sizeof(uint8_t)*TREE_BLOCK_LENGTH_MIN);
        }

        buffer[pos] = (uint8_t)byte;
        pos++;
      }

      stream_destroy(&stream);
      range_tree_destroy(data);
      data = external_data;
    }

    free(marker);
  }

  if (!data) {
    data = range_tree_copy(clipboard_data, 0, range_tree_length(clipboard_data), NULL);
  }

  if (encoding && !*encoding) {
    *encoding = (clipboard_encoding?clipboard_encoding:encoding_utf8_static())->create();
  }

  return data;
}

#ifdef _WINDOWS
struct range_tree* clipboard_command_get(struct encoding** encoding) {
  struct range_tree* converted = NULL;
  if (OpenClipboard(NULL)) {
    HANDLE clipboard = GetClipboardData(CF_UNICODETEXT);
    if (clipboard) {
      WCHAR* data = (WCHAR*)GlobalLock(clipboard);
      if (data) {
        size_t size = GlobalSize(clipboard);
        if (size>0) {
          size = (size-1)&~(size_t)1;
        }

        converted = range_tree_create(NULL, 0);
        range_tree_insert_split(converted, 0, (const uint8_t*)data, size, 0);

        if (encoding && !*encoding) {
          *encoding = encoding_utf16le_create();
        }
      }

      GlobalUnlock(clipboard);
    }

    CloseClipboard();
  }

  return converted;
}
#else
// Get text from system clipboard
struct range_tree* clipboard_command_get(struct encoding** encoding, const char* command) {
  struct range_tree* data = NULL;
  FILE* pipe = popen(command, "r");
  if (pipe) {
    data = range_tree_create(NULL, 0);
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

    if (encoding && !*encoding) {
      *encoding = encoding_utf8_create();
    }
    pclose(pipe);
  }
  return data;
}
#endif
