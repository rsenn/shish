#include "../hashmap.h"
#include "../alloc.h"
#include "../str.h"

static hashentry* get_or_insert_entry(hashmap* map, char* key, size_t keylen);

void
hashmap_put2(hashmap* map, const char* key, size_t keylen, void* val) {
  hashentry* ent;
  char* k = str_ndup(key, keylen);

  if(!k)
    return;

  if((ent = get_or_insert_entry(map, k, keylen))) {
    if(ent->key != k)
      alloc_free(k); /* an existing entry was reused; k was never stored */
    ent->val = val;
  } else {
    alloc_free(k);
  }
}

static hashentry*
get_or_insert_entry(hashmap* map, char* key, size_t keylen) {
  uint64 hash;
  size_t i;

  if(!map->buckets) {
    map->buckets = alloc_zero(HASHMAP_INIT_SIZE * sizeof(hashentry));

    if(!map->buckets)
      return NULL;

    map->capacity = HASHMAP_INIT_SIZE;
  } else if((map->used * 100) / map->capacity >= HASHMAP_HIGH_WATERMARK) {
    hashmap_rehash(map);
  }

  hash = hashmap_fnv_hash(key, keylen);

  for(i = 0; i < map->capacity; i++) {
    hashentry* ent = &map->buckets[(hash + i) % map->capacity];

    if(hashmap_match(ent, key, keylen))
      return ent;

    if(ent->key == HASHMAP_TOMBSTONE || ent->key == NULL) {
      if(ent->key == NULL)
        map->used++;

      ent->key = key;
      ent->keylen = keylen;
      return ent;
    }
  }

  return NULL;
}
