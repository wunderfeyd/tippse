#ifndef __TIPPSE_UTF8__
#define __TIPPSE_UTF8__

#include <stdlib.h>

const char* utf8_decode(int* cp, const char* text, size_t size, size_t skip);
char* utf8_encode(int cp, char* text, size_t size);
const char* utf8_next(const char* text, size_t size, size_t skip);
size_t utf8_strnlen(const char* text, size_t size);
size_t utf8_strlen(const char* text);
const char* utf8_seek(const char* text, size_t pos);

#endif  /* #ifndef __TIPPSE_UTF8__ */
