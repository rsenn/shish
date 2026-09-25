#include "../hashmap.h"
#include "../str.h"

void
hashmap_put(hashmap* map, const char* key, void* val) {
  hashmap_put2(map, key, str_len(key), val);
}
