#ifndef __TIPPSE_FILETYPE__
#define __TIPPSE_FILETYPE__

#include <stdlib.h>
#include "trie.h"

#define SYNTAX_TEXTBEGIN1 1
#define SYNTAX_TEXTEND1 2
#define SYNTAX_TEXTBEGIN2 3
#define SYNTAX_TEXTEND2 4
#define SYNTAX_TEXTBEGIN3 5
#define SYNTAX_TEXTEND3 6
#define SYNTAX_TEXTBEGIN4 7
#define SYNTAX_TEXTEND4 8
#define SYNTAX_BLOCKCOMMENTBEGIN1 9
#define SYNTAX_BLOCKCOMMENTEND1 10
#define SYNTAX_BLOCKCOMMENTBEGIN2 11
#define SYNTAX_BLOCKCOMMENTEND2 12
#define SYNTAX_BLOCKCOMMENTBEGIN3 13
#define SYNTAX_BLOCKCOMMENTEND3 14
#define SYNTAX_BLOCKCOMMENTBEGIN4 15
#define SYNTAX_BLOCKCOMMENTEND4 16
#define SYNTAX_LINECOMMENT 17
#define SYNTAX_MASK 0xff

#define SYNTAX_COLOR_STRING 0x100
#define SYNTAX_COLOR_TYPE 0x200
#define SYNTAX_COLOR_KEYWORD 0x300
#define SYNTAX_COLOR_PREPROCESSOR 0x400
#define SYNTAX_COLOR_MASK 0xf00

#define SYNTAX_CONDITION_NOWHITESPACE_BEFORE 0x10000
#define SYNTAX_CONDITION_NOWHITESPACE_AFTER 0x20000
#define SYNTAX_CONDITION_STOP 0x40000

struct file_type {
  struct file_type* (*create)();
  void (*destroy)(struct file_type*);

  struct trie* (*keywords)(struct file_type*);
};

#endif  /* #ifndef __TIPPSE_FILETYPE__ */