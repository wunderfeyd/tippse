#ifndef __TIPPSE_CLIPBOARD__
#define __TIPPSE_CLIPBOARD__

#include <stdlib.h>
#include "rangetree.h"

void clipboard_set(struct range_tree_node* data);
struct range_tree_node* clipboard_get();

#endif  /* #ifndef __TIPPSE_UTF8__ */
