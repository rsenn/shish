#include "fd.h"
#include "vartab.h"

/* ----------------------------------------------------------------------- */
unsigned long vartab_hash(struct vartab *vartab, const char *v, 
                          struct search *context) {
  context->global = 0;
  context->name = v;
  context->hdist = (VAR_HASH)-1LL;
  context->bdist = (VAR_HASH)-1LL;
  context->exact = 0;
  context->closest = NULL;
  var_lexhash(v, &context->lexhash);
  context->len = var_rndhash(v, &context->rndhash);
  context->bucket = (unsigned long)((VAR_HASH)context->rndhash % (VAR_HASH)VARTAB_BUCKETS);
  
/*  buffer_puts(&fd_out->b, "bucket: ");
  buffer_putulong(&fd_out->b, context->bucket);
  buffer_putnlflush(&fd_out->b);*/
  
  return context->len;
}

