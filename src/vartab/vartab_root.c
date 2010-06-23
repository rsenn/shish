#include "vartab.h"

struct vartab  vartab_root = { .pos = NULL, .parent = NULL, .level = 0 };
struct vartab *varstack = &vartab_root;
struct var    *var_list = NULL;

