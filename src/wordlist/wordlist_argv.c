#include "../wordlist.h"

char**
wordlist_argv(wordlist* wl, int* argc) {
  if(argc)
    *argc = (int)wl->n;

  return wl->v;
}
