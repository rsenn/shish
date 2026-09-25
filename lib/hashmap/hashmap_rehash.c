#include "../hashmap.h"
#include "../alloc.h"

/* Make room for new entries in a given hashmap by removing
   tombstones and possibly extending the bucket size. */
void
hashmap_rehash(hashmap* map) {
  size_t nkeys = 0, cap, i;
  hashmap map2;

  for(i = 0; i < map->capacity; i++)
    if(map->buckets[i].key && map->buckets[i].key != HASHMAP_TOMBSTONE)
      nkeys++;

  cap = map->capacity ? map->capacity : HASHMAP_INIT_SIZE;

  while(cap == 0 || (nkeys * 100) / cap >= HASHMAP_LOW_WATERMARK)
    cap = cap * 2;

  map2.buckets = alloc_zero(cap * sizeof(hashentry));

  if(!map2.buckets)
    return; /* out of memory: leave the map as it was */

  map2.capacity = cap;
  map2.used = 0;

  for(i = 0; i < map->capacity; i++) {
    hashentry* ent = &map->buckets[i];

    if(ent->key && ent->key != HASHMAP_TOMBSTONE) {
      hashmap_put2(&map2, ent->key, ent->keylen, ent->val);
      alloc_free(ent->key);
    }
  }

  alloc_free(map->buckets);
  *map = map2;
}
