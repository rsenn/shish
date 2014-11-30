#ifndef SIG_H
#define SIG_H

#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif

void sig_block(void);

#endif /* SIG_H */
