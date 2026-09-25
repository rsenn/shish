#include "../hashmap.h"
#include "../byte.h"

void
hashmap_init(hashmap* map) {
  byte_zero(map, sizeof(hashmap));
}
