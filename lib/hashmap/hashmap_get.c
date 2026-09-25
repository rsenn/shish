#include "../hashmap.h"
#include "../str.h"

void*
hashmap_get(hashmap* map, const char* key) {
  return hashmap_get2(map, key, str_len(key));
}
