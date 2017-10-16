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

// Skip to next line
void next_line(uint8_t* buffer, size_t size, size_t* offset) {
  while (*offset<size && buffer[*offset]!='\n') {
    (*offset)++;
  }

  (*offset)++;
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
    size_t runs;
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
  write_rle(to, "widths", output, output_size);
  free(output);
}

// Generic method for parsing (param1 is unicode range, param3 is compared parameter)
void convert_range_param3(const char* from, const char* to, const char* name, const char* cmp0, const char* cmp1) {
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
    char* param3 = read_param(buffer, size, &offset);

    int from;
    int to;
    code_range(param1, &from, &to);
    if (from!=-1) {
      if ((cmp0 && strcmp(param3, cmp0)==0) || (cmp1 && strcmp(param3, cmp1)==0)) {
        for (int n = from; n<to && n<UNICODE_MAX_CODEPOINT; n++) {
          output[n] = 1;
        }
      }
    }

    free(param3);
    free(param2);
    free(param1);
    next_line(buffer, size, &offset);
  }

  free(buffer);
  write_rle(to, name, output, output_size);
  free(output);
}

int main(int argc, const char** argv) {
  convert_widths("download/EastAsianWidth.txt", "output/unicode_widths.h");
  convert_range_param3("download/UnicodeData.txt", "output/unicode_invisibles.h", "invisibles", "Cc", "Cf");
  convert_range_param3("download/UnicodeData.txt", "output/unicode_nonspacing_marks.h", "nonspacing_marks", "Mn", "Me");
  convert_range_param3("download/UnicodeData.txt", "output/unicode_spacing_marks.h", "spacing_marks", "Mc", NULL);
  return 0;
}

