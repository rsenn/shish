#include "sh.h"
#include "fd.h"
#include "vartab.h"

/* try to find a var on a table
 * ----------------------------------------------------------------------- */
struct var *vartab_search(struct vartab *vartab, const char *v, 
                          struct search *context) {
  /* FIXME: we do not need to go so paranoid and horny after the closest 
            match, it will be ok having just some close match and a clear
            mismatch! and we will need to loop outside of bsearch/hsearch
            to support searching the global list */
  
  
  VAR_HASH dist;
  
  /* do a context inside here if none was supplied */
  if(context == NULL) {
    /* FIXME: exact match should be assumed from here because there
       is no way to evaluate the search context */
    struct search ctx;
    vartab_hash(sh->varstack, v, &ctx);
    return vartab_search(vartab, v, &ctx);
  }
  
  /* set position to the current bucket and return if its empty */
  context->pos = vartab->pos = &vartab->table[context->bucket];
  
  if(*vartab->pos == NULL)
    return NULL;
  
  /* check for a hashed search match and return if we already have
     a closer match */
  dist = var_hsearch(context);

  if(dist == 0LL) {
    /* if the searched vars len is smaller than hashed chars
       we can safely exit, because every 6-bit pair has a value
       of 1-63 only if it is set */
    if(VAR_CHARS > context->len)
      return *context->pos;
    
    /* if both lens are smaller or equal to hashed char count
       its a match too :) */
    if(VAR_CHARS >= context->len && 
       VAR_CHARS >= (*context->pos)->len)
      return *context->pos;
  }
  /* return if exact match is wanted */
  else if(context->exact) {
    return NULL;
  }
    
  if(dist <= context->hdist) {
    /* here its already clear that we have a new closest match,
       but the binary search could come a bit closer or find a
       full match :) */
    if(dist < context->hdist) {
      context->hdist = dist;
      context->bdist = (VAR_HASH)-1LL;
      context->closest = context->pos;
    }
  
/*  buffer_puts(&fd_out->w, "bsearch: ");
  buffer_puts(&fd_out->w, context->name);
  buffer_putnlflush(&fd_out->w);*/
  
    /* check for a binary search match and return if we already have
       a closer match */
    dist = var_bsearch(context);
    
    if(dist == 0LL)
      return *context->pos;
  
    /* set new closest match */
    if(dist <= context->bdist) {
      context->bdist = dist;
      context->closest = context->pos;
    }
  }
  
  vartab->pos = context->pos;
  
  return NULL;
}

