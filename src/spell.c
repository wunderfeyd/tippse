// Tippse - Spell - Interface for spell checking

#include "spell.h"
#include "config.h"

// Create spell checker
struct spell* spell_create(struct document_file* file) {
  struct spell* base = (struct spell*)malloc(sizeof(struct spell));
  base->file = file;
  base->connected = 0;
  base->cache = trie_create(sizeof(int));
  return base;
}

// Destroy spell checker
void spell_destroy(struct spell* base) {
  spell_disconnect(base);
  trie_destroy(base->cache);
  free(base);
}

// Check cache for recent words added
int spell_check_cache(struct spell* base, const uint8_t* utf8) {
  struct trie_node* node = NULL;
  while (*utf8) {
    node = trie_find_codepoint(base->cache, node, *utf8);
    if (!node) {
      return -1;
    }

    utf8++;
  }

  return node->end?(*((int*)trie_object(node))):-1;
}

// Update cache with spell check result
void spell_update_cache(struct spell* base, const uint8_t* utf8, int result) {
  struct trie_node* node = NULL;
  struct trie_node* parent = NULL;
  while (*utf8) {
    parent = node;
    node = trie_find_codepoint(base->cache, node, *utf8);
    if (!node) {
      node = parent;
      break;
    }

    utf8++;
  }

  while (*utf8) {
    node = trie_append_codepoint(base->cache, node, *utf8, 0);
    //fprintf(stderr, "append %c\r\n", *utf8);
    if (!node) {
      break;
    }

    utf8++;
  }

  //fprintf(stderr, "append\r\n");
  node->end = 1;
  *((int*)trie_object(node)) = result;
}

// Check a word
int spell_check(struct spell* base, const char* utf8) {
  int result = spell_check_cache(base, (uint8_t*)utf8);
  if (result!=-1) {
    return result;
  }

  result = 0;
#ifdef _ANSI_POSIX
  spell_reconnect(base);
  write(base->pipeout[1], utf8, strlen(utf8));
  write(base->pipeout[1], "\n", 1);

  uint8_t line_type = 0;
  while (1) {
    fd_set set_read;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    FD_ZERO(&set_read);
    FD_SET(base->pipein[0], &set_read);
    int nfds = base->pipein[0];
    UNUSED(select(nfds+1, &set_read, NULL, NULL, &tv));

    uint8_t data[256];
    int in = read(base->pipein[0], &data[0], sizeof(data));
    if (in>0) {
      int done = 0;
      for (int n = 0; n<in; n++) {
        if (data[n]!='\n') {
          if (line_type==0) {
            line_type = data[n];
          }
        } else {
          if (line_type=='*' || line_type=='+' || line_type=='-') {
            result = 1;
          }

          if (line_type==0) {
            done = 1;
            break;
          }

          line_type = 0;
        }
      }

      if (done) {
        break;
      }
    } else if (in==0) {
      spell_disconnect(base);
      break;
    } else if (in<0) {
      break;
    }
  }
#endif

  spell_update_cache(base, (uint8_t*)utf8, result);
  return result;
}

#ifdef _ANSI_POSIX
// Connect spell checking provider
void spell_connect(struct spell* base) {
  UNUSED(pipe(base->pipein));
  UNUSED(pipe(base->pipeout));
  signal(SIGCHLD, SIG_IGN);
  pid_t pid = fork();
  struct trie_node* parent = config_find_ascii(base->file->config, "/shell/spell");
  char* shell = (char*)config_convert_encoding(parent, encoding_utf8_static(), NULL);
  if (pid==0) {
    dup2(base->pipeout[0], 0);
    dup2(base->pipein[1], 1);
    dup2(base->pipein[1], 2);
    close(base->pipein[0]);
    close(base->pipein[1]);
    close(base->pipeout[0]);
    close(base->pipeout[1]);

    const char* argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = shell;
    argv[3] = NULL;
    setpgid(0, 0);
    execv(argv[0], (char**)&argv[0]);
    exit(0);
  }

  close(base->pipeout[0]);
  close(base->pipein[1]);
  free(shell);

  int flags = fcntl(base->pipein[0], F_GETFL, 0);
  fcntl(base->pipein[0], F_SETFL, flags|O_NONBLOCK);
  base->connected = 1;
}

// Disconnect provider
void spell_disconnect(struct spell* base) {
  if (base->connected) {
    close(base->pipeout[0]);
    close(base->pipein[1]);
  }

  base->connected = 0;
}

// Reconnect in case a disconnect happend
void spell_reconnect(struct spell* base) {
  if (!base->connected) {
    spell_connect(base);
  }
}
#endif
