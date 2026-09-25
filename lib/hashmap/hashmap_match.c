#include "../hashmap.h"
#include "../byte.h"

int
hashmap_match(hashentry* ent, const char* key, size_t keylen) {
  return ent->key && ent->key != HASHMAP_TOMBSTONE && ent->keylen == keylen &&
         byte_equal(ent->key, keylen, key);
}
