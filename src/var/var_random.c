#include "../var.h"
#include "../../lib/scan.h"
#include "../../lib/uint16.h"
#include "../../lib/uint32.h"

/* $RANDOM: bash/yash-compatible special-variable behavior.
 *
 * Active (the default): every read of $RANDOM produces the next value of
 * a pseudo-random sequence instead of an ordinary variable lookup.
 * Assigning an integer to RANDOM reseeds that sequence deterministically
 * (two shells seeded with the same integer must produce the same
 * sequence); assigning anything else (including empty) permanently turns
 * the magic off, at which point RANDOM behaves like any other variable --
 * `unset RANDOM` also permanently turns it off, per bash's documented
 * quirk that RANDOM never regains its special behavior once unset.
 * ----------------------------------------------------------------------- */

int var_random_active = 1;

static uint32 rnd_state;
static int rnd_seeded = 0;

static uint32
rnd_next(void) {
  uint32 x = rnd_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return (rnd_state = x);
}

void
var_random_assign(const char* value, size_t len) {
  signed int n;

  if(!var_random_active)
    return;

  if(len && scan_int(value, &n) == len) {
    rnd_state = ((uint32)n * 2654435761u) + 1;
    rnd_seeded = 1;
  } else {
    var_random_active = 0;
  }
}

void
var_random_unset(void) {
  var_random_active = 0;
}

uint16
var_random_next(void) {
  if(!rnd_seeded) {
    rnd_state = uint32_random() | 1;
    rnd_seeded = 1;
  }

  return (uint16)rnd_next();
}
