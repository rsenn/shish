#include "../shell.h"
#include "../str.h"
#include "../stralloc.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#define GHN_BUFSIZE MAX_COMPUTERNAME_LENGTH
#define gethostname(x, n) get_host_name(x, n)
static int
get_host_name(char* x, size_t n) {
  DWORD len = n;
  if(GetComputerNameA(x, &len))
    return 0;
  return -1;
}
#else
#define GHN_BUFSIZE 64
#include <unistd.h>
#endif

char*
shell_gethostname(stralloc* sa) {
  unsigned long n = 0;

  do {
    n += 64;
    stralloc_ready(sa, n + 1);
  } while(gethostname(sa->s, sa->a));

  if(sa->s) {
    sa->s[sa->a - 1] = '\0';
    sa->len = str_len(sa->s);
  }

  return sa->s;
}
