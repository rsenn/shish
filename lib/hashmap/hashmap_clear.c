#include "../hashmap.h"
#include "../alloc.h"

void
hashmap_clear(hashmap* map) {
  size_t i;

  for(i = 0; i < map->capacity; i++) {
    hashentry* ent = &map->buckets[i];

    if(ent->key && ent->key != HASHMAP_TOMBSTONE)
      alloc_free(ent->key);

    ent->key = NULL;
    ent->val = NULL;
  }

  map->used = 0;
}
