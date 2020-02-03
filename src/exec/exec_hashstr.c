#include "exec.h"
#include "uint32.h"

uint32
exec_hashstr(const char* s) {
  uint32 hash = 0x7fedcb95; /* some prime number */
  while(*s) hash = (hash * 0x0123456b) ^ *s++;
  return hash;
}