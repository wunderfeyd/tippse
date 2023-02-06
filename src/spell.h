#ifndef TIPPSE_SPELL_H
#define TIPPSE_SPELL_H

#include "types.h"
#include "documentfile.h"
#include "library/trie.h"

struct spell {
	struct document_file* file;
	struct trie* cache;
	int connected;
#ifdef _ANSI_POSIX
  int pipein[2];
  int pipeout[2];
#endif
};

struct spell* spell_create(struct document_file* file);
void spell_destroy(struct spell* base);
int spell_check(struct spell* base, const char* utf8);
int spell_check_cache(struct spell* base, const uint8_t* utf8);
void spell_update_cache(struct spell* base, const uint8_t* utf8, int result);

#ifdef _ANSI_POSIX
void spell_connect(struct spell* base);
void spell_disconnect(struct spell* base);
void spell_reconnect(struct spell* base);
#endif

#endif /* #ifndef TIPPSE_SPELL_H */
