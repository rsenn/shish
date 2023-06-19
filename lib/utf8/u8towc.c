#include "../utf8.h"

int
u8towc(wchar_t* w, const char* u, size_t count) {
  int len;
  /* assert */

  if(NULL == w)
    return -1;

  len = u8len(u, 1);

  if(len < 1)
    return len;
  else if(1 == len) {
    w[0] = u[0] & 0x7f;
    return len;
  } else if(2 == len) {
    if((u[1] & 0xc0) != 0x80) /* error */
      return -1;
    w[0] = ((u[0] & 0x1f) << 6) | (u[1] & 0x3f);
    return 2;
  } else if(3 == len) {
    if((u[1] & 0xc0) != 0x80) /* error */
      return -1;
    if((u[2] & 0xc0) != 0x80) /* error */
      return -1;
    w[0] = ((u[0] & 0x0f) << 12) | ((u[1] & 0x3f) << 6) | (u[2] & 0x3f);
    return 3;
  } else if(4 == len) {
    if((u[1] & 0xc0) != 0x80) /* error */
      return -1;
    if((u[2] & 0xc0) != 0x80) /* error */
      return -1;
    if((u[3] & 0xc0) != 0x80) /* error */
      return -1;
    w[0] = ((u[0] & 0x07) << 18) | ((u[1] & 0x3f) << 12) |
           ((u[2] & 0x3f) << 6) | (u[3] & 0x3f);
    return 4;
  } else /* error */
    return -1;
}
