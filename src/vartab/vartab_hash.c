#include "../fd.h"
#include "../vartab.h"

/* ----------------------------------------------------------------------- */
size_t
vartab_hash(const char* v, struct search* context) {
  context->global = 0;
  context->name = v;
  context->hdist = (VAR_HASH)-1;
  context->bdist = (VAR_HASH)-1;
  context->exact = 0;
  context->closest = NULL;
  var_lexhash(v, &context->lexhash);
  context->len = var_rndhash(v, &context->rndhash);
  context->bucket =
      (size_t)((VAR_HASH)context->rndhash % (VAR_HASH)VARTAB_BUCKETS);

  /*  buffer_puts(&fd_out->b, "bucket: ");
    buffer_putulong(&fd_out->b, context->bucket);
    buffer_putnlflush(&fd_out->b);*/

  return context->len;
}
