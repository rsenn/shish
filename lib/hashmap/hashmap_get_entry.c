#include "../hashmap.h"

hashentry*
hashmap_get_entry(hashmap* map, const char* key, size_t keylen) {
  uint64 hash;
  size_t i;

  if(!map->buckets)
    return NULL;

  hash = hashmap_fnv_hash(key, keylen);

  for(i = 0; i < map->capacity; i++) {
    hashentry* ent = &map->buckets[(hash + i) % map->capacity];

    if(hashmap_match(ent, key, keylen))
      return ent;

    if(ent->key == NULL)
      return NULL;
  }

  return NULL;
}
