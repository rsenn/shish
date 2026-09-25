#include "../hashmap.h"

hashentry*
hashmap_next(hashmap* map, size_t* i) {
  for(; *i < map->capacity; (*i)++) {
    hashentry* ent = &map->buckets[*i];

    if(ent->key && ent->key != HASHMAP_TOMBSTONE) {
      (*i)++;
      return ent;
    }
  }

  return NULL;
}
