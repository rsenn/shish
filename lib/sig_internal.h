#include "sig.h"

#ifndef SIG_INTERNAL
#define SIG_INTERNAL

typedef struct sigtable_s sigtable_t, *sigtable_t_ref;
struct sigtable_s {
  int number;
  const char* name;
} __attribute__((packed));

extern sigtable_t const sigtable[];

#endif
