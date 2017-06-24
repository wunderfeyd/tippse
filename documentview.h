#ifndef __TIPPSE_DOCUMENTVIEW__
#define __TIPPSE_DOCUMENTVIEW__

#include <stdlib.h>
#include "rangetree.h"

struct document_view {
  file_offset_t offset;
  
  int cursor_x;
  int cursor_y;

  file_offset_t selection_start;
  file_offset_t selection_end;

  file_offset_t selection_low;
  file_offset_t selection_high;

  int scroll_x;
  int scroll_y;
  
  int showall;
};

#endif /* #ifndef __TIPPSE_DOCUMENTVIEW__ */
