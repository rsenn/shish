#ifndef DATE_H
#define DATE_H

#include <time.h>

/* date_parse: the date strings `date -d` and `touch -d` accept; returns 0 and sets *out, or -1.
 *
 *   @SECONDS            seconds since the epoch (may be negative)
 *   now today yesterday tomorrow
 *   YYYY-MM-DD          midnight; with  [T ]hh:mm[:ss][Z|+hh:mm|-hh:mm]  the time of day
 *   hh:mm[:ss]          today at that time
 *   [+-]N UNIT [ago]    relative to the rest (or to now): second minute hour day week month year, plural ok
 *
 * Words are separated by blanks; `now` is the reference for everything relative. A string with no date and
 * no time of day means `now`. Dates and times without an offset are local time. */
int date_parse(const char* s, time_t now, time_t* out);

#endif
