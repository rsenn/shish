#include "../arena_internal.h"

arena_pos
arena_tell(const arena* a) {
  arena_pos pos;

  pos.chunk = a->chunk;
  pos.beg = a->beg;
  return pos;
}
