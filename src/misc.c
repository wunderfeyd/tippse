// Tippse - Misc - Functions that fit nowhere at the moment

#include "misc.h"

#include "directory.h"
#include "encoding.h"
#include "encoding/native.h"
#include "encoding/utf8.h"
#include "encoding/utf16le.h"
#include "file.h"
#include "unicode.h"

// Sort an array with merge sort, second array is a temporary buffer
char** merge_sort(char** sort1, char** sort2, size_t count) {
  for (size_t m = 1; m<count; m<<=1) {
    for (size_t n = 0; n<count; n+=m*2) {
      size_t left = n;
      size_t left_end = n+m;
      if (left_end>count) {
        left_end = count;
      }

      size_t right = n+m;
      size_t right_end = n+m*2;
      if (right_end>count) {
        right_end = count;
      }

      size_t out = n;
      while (left<left_end && right<right_end) {
        if (strcasecmp(sort1[left], sort1[right])<=0) {
          sort2[out++] = sort1[left];
          left++;
        } else {
          sort2[out++] = sort1[right];
          right++;
        }
      }

      while (left<left_end) {
        sort2[out++] = sort1[left];
        left++;
      }

      while (right<right_end) {
        sort2[out++] = sort1[right];
        right++;
      }
    }

    char** sort = sort2;
    sort2 = sort1;
    sort1 = sort;
  }

  return sort1;
}

// Return file name without directory path
char* extract_file_name(const char* file) {
  const char* last = file;
  const char* search = file;
  while (*search) {
    if (*search=='/') {
      last = search+1;
    }

    search++;
  }

  return strdup(last);
}

// Return directory path without file name
char* strip_file_name(const char* file) {
  const char* last = file;
  const char* search = file;
  while (*search) {
    if (*search=='/') {
      last = search;
    }

    search++;
  }

  char* stripped = malloc(sizeof(char)*(size_t)((last-file)+1));
  memcpy(stripped, file, (size_t)(last-file));
  stripped[last-file] = '\0';

  return stripped;
}

// Return merged strings
char* combine_string(const char* string1, const char* string2) {
  size_t string1_length = strlen(string1);
  size_t string2_length = strlen(string2);
  char* combined = malloc(sizeof(char)*(string1_length+string2_length+1));
  memcpy(combined, string1, string1_length);
  memcpy(combined+string1_length, string2, string2_length);
  combined[string1_length+string2_length] = '\0';

  return combined;
}

TIPPSE_INLINE int directory_delimiter(char c) {
#ifdef _WINDOWS
  return (c=='\\' || c=='/')?1:0;
#else
  return (c=='/')?1:0;
#endif
}

// Returned merged directory path and file name
char* combine_path_file(const char* path, const char* file) {
  if (directory_delimiter(*file)) {
    return strdup(file);
  }

  size_t path_length = strlen(path);
  if (path_length==0) {
    return strdup(file);
  }

  if (path_length>0 && directory_delimiter(path[path_length-1])) {
    path_length--;
  }

  size_t file_length = strlen(file);
  char* combined = malloc(sizeof(char)*(path_length+file_length+2));
  memcpy(combined, path, path_length);
  combined[path_length] = '/';
  memcpy(combined+path_length+1, file, file_length);
  combined[path_length+file_length+1] = '\0';

  return combined;
}

// Return normalised directory path
char* correct_path(const char* path) {
  size_t path_length = strlen(path);
  char* real = malloc(sizeof(char)*(path_length+2));
  char* root = real;
  char* combined = real;

  int directories = 0;
  while (*path) {
    if (directory_delimiter(path[0])) {
      combined = root;
      directories = 0;

      *combined++ = '/';
      path++;
#ifdef _WINDOWS
    } else if (path[0]!='\0' && path[1]==':') {
      combined = real;
      *combined++ = toupper(path[0]);
      *combined++ = ':';
      root = combined;
      directories = 0;
      path+=2;
#endif
    } else if (path[0]=='.' && (directory_delimiter(path[1]) || path[1]==0)) {
      path+=(path[1]==0)?1:2;
    } else if (directories>0 && path[0]=='.' && path[1]=='.' && (directory_delimiter(path[2]) || path[2]==0)) {
      combined--;
      while (combined!=root && !directory_delimiter(*(combined-1))) {
        combined--;
      }
      directories--;
      path+=(path[2]==0)?2:3;
    } else if (combined!=root && directory_delimiter(*root) && path[0]=='.' && path[1]=='.' && (directory_delimiter(path[2]) || path[2]==0)) {
      path+=(path[2]==0)?2:3;
    } else {
      int level = (path[0]=='.' && path[1]=='.' && directory_delimiter(path[2]))?0:1;
      while (*path && !directory_delimiter(*path)) {
        *combined++ = *path++;
      }

      if (directory_delimiter(*path)) {
        *combined++ = '/';
        directories+=level;
        path++;
      }
    }
  }

  if (combined!=root && combined!=root+1) {
    if (directory_delimiter(*(combined-1))) {
      combined--;
    }
  }

  if (combined==real) {
    *combined++ = '.';
  }

  *combined = '\0';

  return real;
}

// Return relative directory path
char* relativate_path(const char* base, const char* path) {
  char* real = realpath(path, NULL);
  if (!real) {
    return strdup(path);
  }

  const char* search = real;
  while (*search==*base && *base!=0) {
    search++;
    base++;
  }

  if (*base==0) {
    if (directory_delimiter(*search)) {
      char* relative = strdup(search+1);
      free(real);
      return relative;
    } else if (*search==0) {
      char* relative = strdup(".");
      free(real);
      return relative;
    }
  }

  return real;
}

// Return user's home directory
char* home_path(void) {
#ifdef _WINDOWS
  wchar_t result[MAX_PATH];
  SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, &result[0]);
  char* convert = string_internal(&result[0]);
  char* output = correct_path(convert);
  free(convert);
  return output;
#else
  char* env = getenv("HOME");
  if (env) {
    return strdup(env);
  }

  struct passwd* pw = getpwuid(getuid());
  if (pw->pw_dir) {
    return strdup(pw->pw_dir);
  }

  return strdup("");
#endif
}

// Check for directory
int is_directory(const char* path) {
#ifdef _WINDOWS
  wchar_t* os = string_system(path);
  DWORD attributes = GetFileAttributesW(os);
  free(os);
  return (attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_DIRECTORY))?1:0;
#else
  struct stat statbuf;
  if (stat(path, &statbuf)!=0) {
    return 0;
  }

  return S_ISDIR(statbuf.st_mode);
#endif
}

// Check for file
int is_file(const char* path) {
#ifdef _WINDOWS
  wchar_t* os = string_system(path);
  DWORD attributes = GetFileAttributesW(os);
  free(os);
  return (attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY))?1:0;
#else
  struct stat statbuf;
  if (stat(path, &statbuf)!=0) {
    return 0;
  }

  return !S_ISDIR(statbuf.st_mode);
#endif
}

// Return tick counter (microseconds)
int64_t tick_count(void) {
#ifdef _WINDOWS
  // TODO: Check for wrapped counter
  return (int64_t)GetTickCount()*(int64_t)1000;
#else
  struct timeval t;
  gettimeofday(&t, NULL);
  return (int64_t)t.tv_sec*1000000+(int64_t)t.tv_usec;
#endif
}

// Return tick count from milliseconds
int64_t tick_ms(int64_t ms) {
  return ms*(int64_t)1000;
}

// Convert human readable number into internal representation
uint64_t decode_based_unsigned_offset(struct encoding_cache* cache, int base, size_t* offset, size_t count) {
  uint64_t output = 0;
  size_t advanced = *offset;
  while (count>0) {
    count--;
    struct unicode_transform_node transform;
    unicode_read_combined_sequence(cache, advanced, &transform);
    codepoint_t cp = transform.cp[0];
    if (cp==0) {
      break;
    }

    int value = 0;
    if (cp>='0' && cp<='9') {
      value = (int)cp-'0';
    } else if (cp>='A' && cp<='Z') {
      value = (int)cp-'A'+10;
    } else if (cp>='a' && cp<='z') {
      value = (int)cp-'a'+10;
    } else {
      break;
    }

    if (value>=base) {
      break;
    }

    output *= (uint64_t)base;
    output += (uint64_t)value;
    advanced += transform.advance;
  }
  *offset = advanced;

  return output;
}

uint64_t decode_based_unsigned(struct encoding_cache* cache, int base, size_t count) {
  size_t offset = 0;
  return decode_based_unsigned_offset(cache, base, &offset, count);
}

int64_t decode_based_signed_offset(struct encoding_cache* cache, int base, size_t* offset, size_t count) {
  int negate = 1;
  if (count>0) {
    struct unicode_transform_node transform;
    unicode_read_combined_sequence(cache, *offset, &transform);
    codepoint_t cp = transform.cp[0];
    *offset = (*offset)+transform.advance;
    count--;
    negate = (cp=='-')?1:0;
  }

  int64_t output;
  uint64_t abs = decode_based_unsigned_offset(cache, base, offset, count);
  uint64_t max = ((uint64_t)1)<<(8*sizeof(uint64_t)-1);
  if (negate) {
    output = abs>=max?((int64_t)max-1):(int64_t)abs;
  } else {
    output = abs>max?(-((int64_t)max-1))-1:-((int64_t)abs);
  }
  return output;
}

int64_t decode_based_signed(struct encoding_cache* cache, int base, size_t count) {
  size_t offset = 0;
  return decode_based_signed_offset(cache, base, &offset, count);
}

#ifdef _WINDOWS
// Return real path to specified folder/file
char* realpath(const char* path, char* resolved_path) {
  wchar_t result[MAX_PATH];
  wchar_t* os = string_system(path);
  GetFullPathNameW(os, MAX_PATH, result, NULL);
  free(os);
  char* convert = string_internal(&result[0]);
  char* output = correct_path(convert);
  free(convert);
  return output;
}

// Duplicate a string with given length and terminate with NUL
char* strndup(const char* src, size_t length) {
  char* dst = malloc(sizeof(char)*(length+1));
  memcpy(dst, src, length);
  dst[length] = '\0';
  return dst;
}

wchar_t* string_system(const char* convert) {
  struct encoding* utf16 = encoding_utf16le_create();

  const char* end = convert;
  while (*end!=0) {
    end++;
  }

  struct stream stream;
  stream_from_plain(&stream, (uint8_t*)convert, (size_t)((uint8_t*)end-(uint8_t*)convert));
  struct range_tree_node* root = encoding_transform_stream(&stream, encoding_utf8_static(), utf16);
  wchar_t null = 0;
  root = range_tree_insert_split(root, range_tree_length(root), (uint8_t*)&null, sizeof(null), 0);
  wchar_t* output = (wchar_t*)range_tree_raw(root, 0, range_tree_length(root));
  range_tree_destroy(root, NULL);

  utf16->destroy(utf16);
  stream_destroy(&stream);

  return output;
}

char* string_internal(const wchar_t* convert) {
  struct encoding* utf16 = encoding_utf16le_create();

  const wchar_t* end = convert;
  while (*end!=0) {
    end++;
  }

  struct stream stream;
  stream_from_plain(&stream, (uint8_t*)convert, (size_t)((uint8_t*)end-(uint8_t*)convert));
  struct range_tree_node* root = encoding_transform_stream(&stream, utf16, encoding_utf8_static());
  char null = 0;
  root = range_tree_insert_split(root, range_tree_length(root), (uint8_t*)&null, sizeof(null), 0);
  char* output = (char*)range_tree_raw(root, 0, range_tree_length(root));
  range_tree_destroy(root, NULL);

  utf16->destroy(utf16);
  stream_destroy(&stream);
  return output;
}
#endif
