#include "byte.h"
#include "var.h"

struct var *var_init(const char *v, struct var *var, struct search *context) {
  var->bnext = NULL;
  var->gnext = NULL;
  var->blink = NULL;
  var->glink = NULL;
  stralloc_init(&var->sa);
  var->offset = var->len = context->len;
  var->flags = 0;
  var->lexhash = context->lexhash;
  var->rndhash = context->rndhash;
  var->parent = NULL;
  var->child = NULL;
  var->table = NULL;
  
  return var;
}
