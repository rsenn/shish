#include "../hashmap.h"
#include "../str.h"

void
hashmap_delete(hashmap* map, const char* key) {
  hashmap_delete2(map, key, str_len(key));
}
