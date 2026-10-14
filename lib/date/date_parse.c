#include "../date.h"
#include "../scan.h"
#include "../str.h"
#include "../byte.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#define timegm _mkgmtime
#endif

/* "NN" at s, exactly two digits; -1 if not */
static int
two(const char* s) {
  return s[0] >= '0' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9' ? (s[0] - '0') * 10 + (s[1] - '0') : -1;
}

/* the next blank-separated word of *sp: sets *w to it, advances *sp past it, returns its length (0: no more) */
static size_t
nextword(const char** sp, const char** w) {
  const char* s = *sp;
  size_t n;

  while(*s == ' ' || *s == '\t')
    s++;

  *w = s;
  n = str_chrs(s, " \t", 2);
  *sp = s + n;
  return n;
}

/* hh:mm[:ss][Z|+hh:mm|-hh:mm], the whole word; returns 0 on success */
static int
parse_time(const char* s, size_t n, struct tm* tm, long* off, int* has_off) {
  int h, m, sec = 0;

  if(n < 5 || s[2] != ':' || (h = two(s)) < 0 || (m = two(s + 3)) < 0)
    return -1;

  s += 5, n -= 5;

  if(n >= 3 && s[0] == ':') {
    if((sec = two(s + 1)) < 0)
      return -1;

    s += 3, n -= 3;
  }

  if(n == 1 && *s == 'Z') {
    *has_off = 1, *off = 0;
    n = 0;
  } else if(n == 6 && (*s == '+' || *s == '-') && s[3] == ':') {
    int oh = two(s + 1), om = two(s + 4);

    if(oh < 0 || om < 0)
      return -1;

    *has_off = 1;
    *off = (long)(oh * 3600 + om * 60) * (*s == '-' ? -1 : 1);
    n = 0;
  }

  if(n || h > 23 || m > 59 || sec > 60)
    return -1;

  tm->tm_hour = h, tm->tm_min = m, tm->tm_sec = sec;
  return 0;
}

/* "second", "minute", ... or their plurals (also sec, min) -> 0..6; -1 if unknown */
static int
unit(const char* s, size_t n) {
  static const char* const names[] = {"second", "minute", "hour", "day", "week", "month", "year"};
  int i;

  for(i = 0; i < 7; i++) {
    size_t l = str_len(names[i]);

    if((n == l || (n == l + 1 && s[l] == 's')) && !str_diffn(s, names[i], l))
      return i;
  }

  if((n == 3 || (n == 4 && s[3] == 's')) && !str_diffn(s, "sec", 3))
    return 0;

  if((n == 3 || (n == 4 && s[3] == 's')) && !str_diffn(s, "min", 3))
    return 1;

  return -1;
}

int
date_parse(const char* s, time_t now, time_t* out) {
  struct tm tm, rel;
  long off = 0;
  int has_off = 0, have_date = 0, have_time = 0;
  const char* w;
  size_t n;
  time_t t;

  while(*s == ' ' || *s == '\t')
    s++;

  if(*s == '@') {
    int neg = s[1] == '-';
    const char* p = s + 1 + (neg || s[1] == '+');
    unsigned long u;
    size_t k;

    if(!(k = scan_ulong(p, &u)) || (p[k] && p[k] != ' ' && p[k] != '\t'))
      return -1;

    /* "@N" is the base for a relative tail: "@86400 +1 day" */
    now = neg ? -(time_t)u : (time_t)u;
    s = p + k;
  }

  t = now;
  tm = *localtime(&t);
  byte_zero(&rel, sizeof(rel));

  while((n = nextword(&s, &w))) {
    if(n == 3 && !str_diffn(w, "now", 3))
      continue;

    if(n == 5 && !str_diffn(w, "today", 5))
      continue;

    if(n == 9 && !str_diffn(w, "yesterday", 9)) {
      rel.tm_mday -= 1;
      continue;
    }

    if(n == 8 && !str_diffn(w, "tomorrow", 8)) {
      rel.tm_mday += 1;
      continue;
    }

    /* YYYY-MM-DD, optionally glued to the time with 'T' */
    if(n >= 10 && w[4] == '-' && w[7] == '-') {
      int y1 = two(w), y2 = two(w + 2), mo = two(w + 5), d = two(w + 8);

      if(have_date || y1 < 0 || y2 < 0 || mo < 1 || mo > 12 || d < 1 || d > 31)
        return -1;

      tm.tm_year = y1 * 100 + y2 - 1900, tm.tm_mon = mo - 1, tm.tm_mday = d;
      have_date = 1;

      if(n > 10) {
        if((w[10] != 'T' && w[10] != 't') || parse_time(w + 11, n - 11, &tm, &off, &has_off) < 0)
          return -1;

        have_time = 1;
      }

      continue;
    }

    if(n >= 5 && w[2] == ':') {
      if(have_time || parse_time(w, n, &tm, &off, &has_off) < 0)
        return -1;

      have_time = 1;
      continue;
    }

    /* [+-]N unit [ago], the unit in the next word or glued to the number ("2days") */
    {
      int neg = *w == '-';
      const char* p = w + (neg || *w == '+');
      const char* u0;
      const char* save;
      unsigned long u;
      size_t k, un;
      long v;
      int idx;

      if(!(k = scan_ulong(p, &u)))
        return -1;

      if(p + k < w + n) {
        u0 = p + k;
        un = (size_t)(w + n - u0);
      } else if(!(un = nextword(&s, &u0))) {
        return -1;
      }

      if((idx = unit(u0, un)) < 0)
        return -1;

      v = neg ? -(long)u : (long)u;
      save = s;

      if(nextword(&s, &w) == 3 && !str_diffn(w, "ago", 3))
        v = -v;
      else
        s = save;

      switch(idx) {
        case 0: rel.tm_sec += (int)v; break;
        case 1: rel.tm_min += (int)v; break;
        case 2: rel.tm_hour += (int)v; break;
        case 3: rel.tm_mday += (int)v; break;
        case 4: rel.tm_mday += (int)v * 7; break;
        case 5: rel.tm_mon += (int)v; break;
        default: rel.tm_year += (int)v; break;
      }
    }
  }

  /* a date alone means midnight */
  if(have_date && !have_time)
    tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

  tm.tm_sec += rel.tm_sec;
  tm.tm_min += rel.tm_min;
  tm.tm_hour += rel.tm_hour;
  tm.tm_mday += rel.tm_mday;
  tm.tm_mon += rel.tm_mon;
  tm.tm_year += rel.tm_year;
  tm.tm_isdst = -1;

  if(has_off) {
    tm.tm_isdst = 0;
    t = timegm(&tm) - off;
  } else {
    t = mktime(&tm);
  }

  if(t == (time_t)-1)
    return -1;

  *out = t;
  return 0;
}
