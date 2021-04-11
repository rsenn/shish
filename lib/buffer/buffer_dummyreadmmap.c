#include "../buffer.h"

ssize_t
buffer_dummyreadmmap(int fd, void* buf, size_t len, void* ptr) {
  return 0;
}
