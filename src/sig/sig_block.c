#include "sig.h"

/* block SIGINT and SIGCHILD before forking a child 
 * ----------------------------------------------------------------------- */
void sig_block(void) {
  static sigset_t oldset;
  sigset_t newset;
  
  sigemptyset(&newset);
  sigemptyset(&oldset);
  sigaddset(&newset, SIGINT);
  sigaddset(&newset, SIGCHLD);
  sigprocmask(SIG_BLOCK, &newset, &oldset);
}
