#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define UNICODE_MAX_CODEPOINT 0x110000

// Read complete file into memory
uint8_t* read_file(const char* name, size_t* size) {
  FILE* file = fopen(name, "r");
  if (!file) {
    printf("File '%s' is not accessible\r\n", name);
    *size = 0;
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  *size = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);
  uint8_t* buffer = (uint8_t*)malloc(*size);
  fread(buffer, 1, *size, file);
  fclose(file);
  return buffer;
}

// Read complete file into memory
void write_file_bytes(const char* to, const char* name, uint8_t* output, size_t size) {
  FILE* file = fopen(to, "w");
  if (file) {
    fprintf(file, "uint8_t %s[] = {", name);
    int runs = 0;
    while (size--!=0) {
      if ((runs&31)==0) {
        fprintf(file, "\r\n ");
      }
      runs++;
      fprintf(file, " 0x%02x,", *output);
      output++;
    }
    fprintf(file, " 0};\r\n");
    fclose(file);
  } else {
    printf("Can't create file '%s'\r\n", to);
  }
}

// Strip unwanted characters
uint8_t* reduce_file(uint8_t* buffer, size_t* size) {
  size_t size_org = *size;
  int newline = 1;
  int escape = 0;
  int string = 0;
  size_t size_new = 0;
  uint8_t* buffer_in = buffer;
  uint8_t* buffer_out = buffer;
  while (size_org>0) {
    uint8_t c = *buffer_in++;
    if (!string && !escape && (c=='\r' || c=='\n' || ((c=='\t' || c==' ' || c==',') && newline))) {
      newline = 1;
    } else {
      if (c=='"' && !escape) {
        string = !string;
      }

      if (c=='\\') {
        escape = 1;
      } else {
        escape = 0;
      }

      newline = (c=='{' || c=='}')?1:0;
      *buffer_out++ = c;
      size_new++;
    }

    size_org--;
  }

  *size = size_new;
}

// Skip to next line
void next_line(uint8_t* buffer, size_t size, size_t* offset) {
  while (*offset<size && buffer[*offset]!='\n') {
    (*offset)++;
  }

  if (*offset<size) {
    (*offset)++;
  }
}

// Read parameter from cursor position and trim
char* read_param(uint8_t* buffer, size_t size, size_t* offset) {
  size_t begin = *offset;
  while (*offset<size && buffer[*offset]!=';' && buffer[*offset]!='#' && buffer[*offset]!='\n' && buffer[*offset]!='\r') {
    (*offset)++;
  }

  size_t end = (*offset);
  while (begin<end && buffer[begin]==' ') {
    begin++;
  }

  while (begin<end && buffer[end-1]==' ') {
    end--;
  }

  size_t length = end-begin;
  char* string = (char*)malloc(length+1);
  memcpy(string, &buffer[begin], length);
  string[length] = 0;

  if (*offset<size && buffer[*offset]==';') {
    (*offset)++;
  }

  return string;
}

// Return code range encoded in specified string
void code_range(char* param, int* from, int* to) {
  int n = sscanf(param, "%x..%x", from, to);
  if (n==1) {
    *to = *from;
  } else if (n<=0) {
    *from = -1;
    *to = -1;
  }
  (*to)++;
}

// Write RLE stream
void write_rle(const char* to, const char* name, uint8_t* output, size_t output_size) {
  FILE* file = fopen(to, "w");
  if (file) {
    fprintf(file, "uint16_t unicode_%s_rle[] = {", name);
    int run = 0;
    size_t runs = 0;
    uint8_t old = output[0];
    size_t n = 0;
    while (n<=output_size) {
      if (n==output_size || old!=output[n] || run==0x7fff) {
        if ((runs&15)==0) {
          fprintf(file, "\r\n ");
        }
        runs++;
        fprintf(file, " 0x%04x,", (run<<1)|old);

        run = 0;
        if (n==output_size) {
          n++;
        } else {
          old = output[n];
        }
      } else {
        run++;
        n++;
      }
    }
    fprintf(file, " 0};\r\n");
    fclose(file);
  } else {
    printf("Can't create file '%s'\r\n", to);
  }
}

// Extract full width and half width information from unicode database to build a bit array
void convert_widths(const char* from, const char* to) {
  size_t size;
  uint8_t* buffer = read_file(from, &size);
  if (!buffer) {
    return;
  }

  size_t output_size = UNICODE_MAX_CODEPOINT;
  uint8_t* output = (uint8_t*)malloc(output_size);
  memset(output, 0, output_size);

  size_t offset = 0;
  while (offset<size) {
    char* param1 = read_param(buffer, size, &offset);
    char* param2 = read_param(buffer, size, &offset);

    int from;
    int to;
    code_range(param1, &from, &to);
    if (from!=-1) {
      // printf("'%s'  '%s'  %d-%d\r\n", param1, param2, from, to);
      if (strcmp(param2, "F")==0 || strcmp(param2, "W")==0) {
        for (int n = from; n<to && n<UNICODE_MAX_CODEPOINT; n++) {
          output[n] = 1;
        }
      }
    }

    free(param2);
    free(param1);
    next_line(buffer, size, &offset);
  }

  free(buffer);
  write_rle(to, "unicode_widths", output, output_size);
  free(output);
}

// Generic method for parsing (param1 is unicode range, param is compared parameter)
void convert_range_param(size_t param, const char* from, const char* to, const char* name, const char* cmp0, const char* cmp1, const char* cmp2, const char* cmp3) {
  size_t size;
  uint8_t* buffer = read_file(from, &size);
  if (!buffer) {
    return;
  }

  size_t output_size = UNICODE_MAX_CODEPOINT;
  uint8_t* output = (uint8_t*)malloc(output_size);
  memset(output, 0, output_size);

  size_t offset = 0;
  char* params[1024];
  while (offset<size) {
    for (size_t n = 0; n<param; n++) {
      params[n] = read_param(buffer, size, &offset);
    }

    int from;
    int to;
    code_range(params[0], &from, &to);
    if (from!=-1) {
      if ((cmp0 && strcmp(params[param-1], cmp0)==0) || (cmp1 && strcmp(params[param-1], cmp1)==0) || (cmp2 && strcmp(params[param-1], cmp2)==0) || (cmp3 && strcmp(params[param-1], cmp3)==0)) {
        for (int n = from; n<to && n<UNICODE_MAX_CODEPOINT; n++) {
          output[n] = 1;
        }
      }
    }

    for (size_t n = 0; n<param; n++) {
      free(params[n]);
    }
    next_line(buffer, size, &offset);
  }

  free(buffer);
  write_rle(to, name, output, output_size);
  free(output);
}

// Simplified utf8 encoding
size_t utf8_encode(int cp, uint8_t* text) {
  if (cp<0x80) {
    *text++ = (uint8_t)cp;
    return 1;
  } else if (cp<0x800) {
    *text++ = 0xc0+(uint8_t)(cp>>6);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 2;
  } else if (cp<0x10000) {
    *text++ = 0xe0+(uint8_t)(cp>>12);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 3;
  } else if (cp<0x101000) {
    *text++ = 0xf0+(uint8_t)(cp>>18);
    *text++ = 0x80+(uint8_t)((cp>>12)&0x3f);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 4;
  }

  return 0;
}

// Create translation table (many to many codepoints) (see unicode_decode_transform for more details)
void convert_transform(const char* from, const char* to, const char* name, const char* cmp0, const char* cmp1, size_t compare, size_t left, size_t right) {
  size_t size;
  uint8_t* buffer = read_file(from, &size);
  if (!buffer) {
    return;
  }

  size_t output_size = UNICODE_MAX_CODEPOINT*16*4;
  uint8_t* output = (uint8_t*)malloc(output_size);
  uint8_t* write = output;

  int from_before[8];
  int to_before[8];
  for (size_t n = 0; n<8; n++) {
    from_before[n] = 0;
    to_before[n] = 0;
  }

  size_t copy[16];
  size_t copies = 0;
  for (size_t n = 0; n<16; n++) {
    copy[n] = ~(size_t)0;
  }

  int copied = 0;
  size_t offset = 0;
  while (offset<size) {
    char* param[8];
    for (size_t n = 0; n<8; n++) {
      param[n] = read_param(buffer, size, &offset);
    }

    if (!compare || (cmp0 && strcmp(param[compare], cmp0)==0) || (cmp1 && strcmp(param[compare], cmp1)==0)) {
      int from[8];
      int froms = sscanf(param[left], "%x %x %x %x %x %x %x %x", &from[0], &from[1], &from[2], &from[3], &from[4], &from[5], &from[6], &from[7]);
      int to[8];
      int tos = sscanf(param[right], "%x %x %x %x %x %x %x %x", &to[0], &to[1], &to[2], &to[3], &to[4], &to[5], &to[6], &to[7]);
      if (froms>0 && tos>0) {
        int add = 0;
        if (froms!=tos) {
          add = 1;
        } else {
          for (int n = 0; n<froms; n++) {
            if (from[n]!=to[n]) {
              add = 1;
            }
          }
        }
        if (add) {
          if (froms==8 || tos==8) {
            printf("umm very long %s - %s\r\n", param[left], param[right]);
          }
          uint8_t* start = write;
          *write++ = (uint8_t)(froms<<3|tos<<0);
          for (int n = 0; n<froms; n++) {
            int diff = from[n]-from_before[n]+0x10;
            if (diff<=0x1f && diff>=0) {
              write += utf8_encode(diff, write);
            } else {
              write += utf8_encode(from[n], write);
            }
            from_before[n] = from[n];
          }
          for (int n = 0; n<tos; n++) {
            int diff = to[n]-to_before[n]+0x10;
            if (diff<=0x1f && diff>=0) {
              write += utf8_encode(diff, write);
            } else {
              write += utf8_encode(to[n], write);
            }
            to_before[n] = to[n];
          }

          size_t l = (size_t)(write-start);
          size_t c;
          for (c = 0; c<16; c++) {
            if (copy[c]==~(size_t)0) {
              continue;
            }

            size_t m;
            for (m = 0; m<l; m++) {
              if (output[copy[c]+m]!=start[m]) {
                break;
              }
            }

            if (m==l) {
              uint8_t run = 0;
              write = start;
              if (copied && ((*(start-1))&0xf)==c && ((*(start-1))>>4)!=15) {
                run = (((*(start-1))>>4)&0x7)+1;
                write = start-1;
              }

              *write++ = 0x80|(uint8_t)c|(run<<4);

              copied = 1;
              break;
            }
          }
          if (c==16) {
            copy[copies] = (size_t)(start-output);
            copies++;
            copies&=15;
            copied = 0;
          }
        }
      }
    }

    for (size_t n = 0; n<8; n++) {
      free(param[n]);
    }
    next_line(buffer, size, &offset);
  }

  free(buffer);
  write_file_bytes(to, name, output, (size_t)(write-output));
  free(output);
}

void build_manual(const char* from, const char* to, const char* name, int reduce) {
  size_t size;
  uint8_t* buffer = read_file(from, &size);
  if (!buffer) {
    return;
  }

  if (reduce) {
    reduce_file(buffer, &size);
  }

  write_file_bytes(to, name, buffer, size);
  free(buffer);
}

int main(int argc, const char** argv) {
  if (argc>=2 && strcmp(argv[1], "--unicode")==0) {
    convert_widths("download/EastAsianWidth.txt", "output/unicode_widths.h");
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_invisibles.h", "unicode_invisibles", "Cc", "Cf", NULL, NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_marks.h", "unicode_marks", "Mc", "Mn", "Me", NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_digits.h", "unicode_digits", "Nd", NULL, NULL, NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_whitespace.h", "unicode_whitespace", "Zs", NULL, NULL, NULL);
    convert_range_param(5, "download/UnicodeData.txt", "output/unicode_letters.h", "unicode_letters", "L", NULL, NULL, NULL);
    convert_transform("download/CaseFolding.txt", "output/unicode_case_folding.h", "unicode_case_folding", "C", "F", 1, 0, 2);
    convert_transform("download/NormalizationTest.txt", "output/unicode_normalization.h", "unicode_normalization", NULL, NULL, 0, 1, 2);
  } else if (argc>=6 && strcmp(argv[1], "--bin2c")==0) {
    build_manual(argv[2], argv[3], argv[4], (strcmp(argv[5], "reduce")==0)?1:0);
  }

  return 0;
}
