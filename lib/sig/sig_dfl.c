#include "../sig.h"

#include <signal.h>

#undef sig_dfl
struct sigaction const sig_dfl = {.sa_handler = (void (*)(int))0,
                                  .sa_mask = {{0}},
                                  .sa_flags = 0,
                                  .sa_restorer = 0};
#undef sig_ign
struct sigaction const sig_ign = {.sa_handler = (void (*)(int))1,
                                  .sa_mask = {{0}},
                                  .sa_flags = 0,
                                  .sa_restorer = 0};
