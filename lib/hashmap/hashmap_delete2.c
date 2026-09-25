#include "../hashmap.h"
#include "../alloc.h"

void
hashmap_delete2(hashmap* map, const char* key, size_t keylen) {
  hashentry* ent;

  if((ent = hashmap_get_entry(map, key, keylen))) {
    alloc_free(ent->key);
    ent->key = HASHMAP_TOMBSTONE;
  }
}
