#include "../hashmap.h"

void*
hashmap_get2(hashmap* map, const char* key, size_t keylen) {
  hashentry* ent = hashmap_get_entry(map, key, keylen);

  return ent ? ent->val : NULL;
}
