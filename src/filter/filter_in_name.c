#include "../filter.h"

/* return the string name of the file operand currently being read,
 * or "-" if reading from standard input/upstream.
 * ----------------------------------------------------------------------- */
const char*
filter_in_name(const struct filter_in* in) {
  return in->files && in->i > 0 ? in->files[in->i - 1] : "-";
}
