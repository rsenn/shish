/**
 * @defgroup   hashmap
 * @brief      HASHMAP module: an open-addressing string-keyed map.
 *
 * Ported from ../c-utils/lib/hashmap (open-addressing, FNV-1a hash,
 * tombstone deletes, grow-on-70%/shrink-to-50%-on-rehash) with two
 * small adaptations for this tree: `bool` -> `int` (no bool.h here)
 * and `str_ndup` now lives in lib/str.h instead of c-utils' own copy.
 * hashmap_next() (iteration) is new, not in the original.
 * @{
 */
#ifndef HASHMAP_H
#define HASHMAP_H

#include "typedefs.h"
#include "uint64.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char* key;
  size_t keylen;
  void* val;
} hashentry;

typedef struct {
  hashentry* buckets;
  size_t capacity, used;
} hashmap;

#define HASHMAP_INIT() (hashmap){0, 0, 0}

void hashmap_init(hashmap*);
void hashmap_delete2(hashmap*, const char*, size_t);
void hashmap_delete(hashmap*, const char*);
void* hashmap_get2(hashmap*, const char*, size_t);
void* hashmap_get(hashmap*, const char*);
hashentry* hashmap_get_entry(hashmap*, const char*, size_t);
int hashmap_match(hashentry*, const char*, size_t);
void hashmap_put2(hashmap*, const char*, size_t, void*);
void hashmap_put(hashmap*, const char*, void*);
void hashmap_rehash(hashmap*);

/* hashmap_next: iterates live entries in bucket order (POSIX leaves
 * `for (k in a)` order unspecified, so this is as good as any).
 * Start with *i = 0; each call returns the next live entry and
 * advances *i past it, or NULL once none remain. */
hashentry* hashmap_next(hashmap*, size_t* i);

/* hashmap_clear: deletes every entry (frees each key, not each val --
 * the caller owns what val points to) but keeps the buckets. */
void hashmap_clear(hashmap*);

/* Initial hash bucket size*/
#define HASHMAP_INIT_SIZE 16

/* Rehash if the usage exceeds 70%.*/
#define HASHMAP_HIGH_WATERMARK 70

/* We'll keep the usage below 50% after rehashing.*/
#define HASHMAP_LOW_WATERMARK 50

/* Represents a deleted hash entry*/
#define HASHMAP_TOMBSTONE ((void*)-1)

static inline uint64
hashmap_fnv_hash(const char* s, size_t len) {
  uint64 hash = 0xcbf29ce484222325ULL;
  size_t i;

  for(i = 0; i < len; i++) {
    hash *= 0x100000001b3ULL;
    hash ^= (unsigned char)s[i];
  }

  return hash;
}

#ifdef __cplusplus
}
#endif

#endif /* defined HASHMAP_H */
/** @} */
