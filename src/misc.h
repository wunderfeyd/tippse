#ifndef TIPPSE_MISC_H
#define TIPPSE_MISC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WINDOWS
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#endif
#include "types.h"
#include "encoding.h"
#include "encoding/native.h"
#include "encoding/utf8.h"
#include "encoding/utf16le.h"
#include "unicode.h"
#include "directory.h"
#include "file.h"

char** merge_sort(char** sort1, char** sort2, size_t count);

char* extract_file_name(const char* file);
char* strip_file_name(const char* file);
char* combine_string(const char* string1, const char* string2);
char* combine_path_file(const char* path, const char* file);
char* correct_path(const char* path);
char* relativate_path(const char* base, const char* path);
char* home_path(void);
int is_directory(const char* path);
int is_file(const char* path);

int64_t tick_count(void);
int64_t tick_ms(int64_t ms);

uint64_t decode_based_unsigned_offset(struct encoding_cache* cache, int base, size_t* offset, size_t count);
uint64_t decode_based_unsigned(struct encoding_cache* cache, int base, size_t count);

int64_t decode_based_signed_offset(struct encoding_cache* cache, int base, size_t* offset, size_t count);
int64_t decode_based_signed(struct encoding_cache* cache, int base, size_t count);

#ifdef _WINDOWS
char* realpath(const char* path, char* resolved_path);
char* strndup(const char* src, size_t length);

wchar_t* string_system(const char* convert);
char* string_internal(const wchar_t* convert);
#endif
#endif /* #ifndef TIPPSE_MISC_H */
