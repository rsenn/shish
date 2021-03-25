#include "../fd.h"

struct fd* fd_list[FD_MAX];
int fd_expected = 0; /* expected next effective fd */
int fd_top = 0;      /* highest effective fd we had under control */
int fd_lo = 0;       /* smallest current effective fd */
int fd_hi = 0;       /* biggest current effective fd + 1 */
