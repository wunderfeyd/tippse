#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define UNICODE_MAX_CODEPOINT 0x110000

// Simple variable length encoding
size_t var_encode(int cp, uint8_t* text) {
  size_t length = 0;
  while (1) {
    *text++ = (cp&0x7f)|(cp>=0x80?0x80:0x00);
    length++;
    cp>>=7;
    if (cp==0) {
      break;
    }
  }

  return length;
}

// Simple variable length check
size_t var_length(int cp) {
  size_t length = 0;
  while (1) {
    length++;
    cp>>=7;
    if (cp==0) {
      break;
    }
  }

  return length;
}

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
        fprintf(file, "\n ");
      }
      runs++;
      fprintf(file, " 0x%02x,", *output);
      output++;
    }
    fprintf(file, " 0};\n");
    fclose(file);
  } else {
    printf("Can't create file '%s'\r\n", to);
  }
}

// Strip unwanted characters
void reduce_file(uint8_t* buffer, size_t* size) {
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
  uint8_t* reduced = (uint8_t*)malloc(sizeof(uint8_t*)*(output_size*2)+256);
  uint8_t* write = reduced;
  int run = 0;
  size_t runs = 0;
  uint8_t old = output[0];
  size_t n = 0;
  while (n<=output_size) {
    if (n==output_size || old!=output[n]) {
      runs++;
      write += var_encode((run<<1)|old, write);

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

  write_file_bytes(to, name, reduced, write-reduced);
  free(reduced);
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
  write_rle(to, "unicode_widths_rle", output, output_size);
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

void append_delta(int* from, int froms, int* before, size_t before_pos, uint32_t* sequences, size_t* sequences_pos) {

  for (int n = 0; n<froms; n++) {
    int diff = from[n]-before[(before_pos+n)&31];
    if (diff<0) {
      sequences[(*sequences_pos)++] = (-diff)<<1;
    } else {
      sequences[(*sequences_pos)++] = ((diff+1)<<1)|1;
    }
  }
  sequences[(*sequences_pos)++] = 1;

  for (int n = 0; n<froms; n++) {
    before[((before_pos)+n)&31] = from[n];
  }
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

  uint32_t* sequences = (uint32_t*)malloc(128000*sizeof(uint32_t));
  size_t sequences_pos = 0;

  int before[256];
  for (size_t n = 0; n<256; n++) {
    before[n] = 0;
  }

  size_t before_pos = 0;

  int codes = 0;
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
          codes++;
          if (froms==8 || tos==8) {
            printf("umm very long %s - %s\r\n", param[left], param[right]);
          }

          /*for (int n = 0; n<froms; n++) {
            printf("%x ", from[n]);
          }
          printf("-> ");
          for (int n = 0; n<tos; n++) {
            printf("%x ", to[n]);
          }
          printf("\r\n");*/

          append_delta(&from[0], froms, &before[0], 0, &sequences[0], &sequences_pos);
          append_delta(&to[0], tos, &before[0], 8, &sequences[0], &sequences_pos);
        }
      }
    }

    for (size_t n = 0; n<8; n++) {
      free(param[n]);
    }
    next_line(buffer, size, &offset);
  }

  uint32_t* compresses = (uint32_t*)malloc(128000*sizeof(uint32_t));
  size_t compresses_pos = 0;

  sequences[sequences_pos++] = 0;

  int lazy = 0;
  for (size_t n = 0; n<sequences_pos;) {
    size_t best_l2 = 0;
    size_t best_m2 = 0;
    if (n+1<sequences_pos) {
      for (size_t m = 0; m<n+1; m++) {
        size_t l;
        for (l = 0; l<((n+1)-m) && (l+n+1)<sequences_pos; l++) {
          if (sequences[n+l+1]!=sequences[m+l]) {
            break;
          }
        }

        if (l>best_l2) {
          best_l2 = l;
          best_m2 = m;
        }
      }
    }

    size_t best_l = 0;
    size_t best_m = 0;
    for (size_t m = 0; m<n; m++) {
      size_t l;
      for (l = 0; l<(n-m) && (l+n)<sequences_pos; l++) {
        if (sequences[n+l]!=sequences[m+l]) {
          break;
        }
      }

      if (l>best_l) {
        best_l = l;
        best_m = m;
      }
    }

    int link_length = var_length((best_l<<1)|1)+var_length((n-best_m));
    int orig_length = 0;
    for (size_t l = 0; l<best_l; l++) {
      orig_length += var_length(sequences[n+l]<<1);
    }

    int link = (link_length<orig_length && best_l>0)?1:0;
    if (link && (best_l2<=best_l || lazy)) {
      // printf("%d %d %d\r\n", (int)best_l, (int)n, (int)(n-best_m));
      compresses[compresses_pos++] = (best_l<<1)|1;
      compresses[compresses_pos++] = (n-best_m);
      n += best_l;
    } else {
      compresses[compresses_pos++] = sequences[n]<<1;
      n++;
    }

    if ((best_l2>best_l && link) && !lazy) {
      lazy = 1;
    } else {
      lazy = 0;
    }
  }

  for (size_t n = 0; n<compresses_pos; n++) {
    write += var_encode(compresses[n], write);
  }

  free(buffer);
  write_file_bytes(to, name, output, (size_t)(write-output));
  printf("%d -> %d (%d bytes/%d codes)\r\n", (int)sequences_pos, (int)compresses_pos, (int)(write-output), codes);

  free(output);
  free(compresses);
  free(sequences);
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
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_invisibles.h", "unicode_invisibles_rle", "Cc", "Cf", NULL, NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_marks.h", "unicode_marks_rle", "Mc", "Mn", "Me", NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_digits.h", "unicode_digits_rle", "Nd", NULL, NULL, NULL);
    convert_range_param(3, "download/UnicodeData.txt", "output/unicode_whitespace.h", "unicode_whitespace_rle", "Zs", NULL, NULL, NULL);
    convert_range_param(5, "download/UnicodeData.txt", "output/unicode_letters.h", "unicode_letters_rle", "L", NULL, NULL, NULL);
    convert_transform("download/CaseFolding.txt", "output/unicode_case_folding.h", "unicode_case_folding", "C", "S", 1, 0, 2);
    convert_transform("download/NormalizationTest.txt", "output/unicode_normalization.h", "unicode_normalization", NULL, NULL, 0, 1, 2);
  } else if (argc>=6 && strcmp(argv[1], "--bin2c")==0) {
    build_manual(argv[2], argv[3], argv[4], (strcmp(argv[5], "reduce")==0)?1:0);
  }

  return 0;
}
