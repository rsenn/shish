#include "../fdtable.h"

static struct fd* fdtable_table[FDTABLE_SIZE + 1];
struct fd** fdtable_pos;
int fdtable_bottom = 0;
int fdtable_top = 0;
struct fd** const fdtable = &fdtable_table[1];
