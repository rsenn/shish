#include "fmt.h"
#include "fd.h"
#include "var.h"

/* lexicographically hash a variable name
 * 
 * v is assumed to be a valid name (^[A-Z_a-z][0-9A-Z_a-z]*)
 * 
 * the character range has 63 chars (0-62), so its modulus is 63.
 * for simplicity again we'll use 64 (6 bits) so we can do the thing
 * using bit operations.
 * 
 * MSB 11111122 22223333 33444444 55555566 LSB
 * 
 * lexicographically hashing means that you can use the resulting 
 * for string comparision, so if hash1 < hash2 then var1 is lexico-
 * graphically smaller than var2 (which means that a var_diff() would 
 * return -1). given hash1 == hash2 doesn't need to be an exact match,
 * because the entropy is limited to the size of an unsigned long, 
 * which usually means 5 1/3 character (if its 32-bit), so this case
 * would need further comparision using var_diff().
 * ----------------------------------------------------------------------- */

/* this macro reduces 8-bit ascii to 
   the POSIX shell variable name charset [0-9A-Z_a-z]
   
   BIG WARNING ONCE AGAIN: supply only valid names!!!!!!!!!! */
#define reduce(c) ((c) <  'A' ? (c) - '0' : \
                  ((c) <= 'Z' ? (c) - 'A' + 10 : \
                  ((c) <  'a' ? (c) - '_' + 36 : \
                   (c) -  'a' + 37)))

unsigned long var_lexhash(const char *v, VAR_HASH *h) {
  VAR_HASH hash;
  register int shift;
  register VAR_HASH value;
  register const unsigned char *p = (const unsigned char *)v; 

  /* begin with the 6 most significant 
     bits, independent of word size */
  shift = VAR_BITS - 6;
  
  /* we subtract 10 from the topmost 6 bits because 
     there are no digits allowed */
  hash = -10LL << shift;
  
  do {
    /* map the character to a 1-63 range */
    value = 1LL + (uint64)reduce(*p);
    
    /* add another 6 hash bits after shifting them.
       when the shift count is negative we have to
       shift the other way round */
    hash += (uint64)(shift < 0 ?
                    (value >> (-shift)) :
                    (value << ( shift)));
    
    /* skip current char and exit if the variable name ends */
    p++;
    
    if(*p == '\0' || *p == '=') 
      break;
  }
  /* fill in next 6 bits on next iteration */
  while((shift -= 6) > -6);
  
  *h = (uint64)hash;
  
  return (unsigned long)p - (unsigned long)v;
}

