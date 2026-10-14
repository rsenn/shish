/* lib/date date_parse unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../../lib/date.h"
#include <stdio.h>
#include <stdlib.h>

static int failed;

/* date_parse(s) at the fixed now 2020-03-15 12:00:00 UTC must give want, or fail when valid is 0 */
#define NOW 1584273600L

static void
t(const char* s, long want, int valid) {
  time_t r = 0;
  int rc = date_parse(s, NOW, &r), ok = valid ? rc == 0 && (long)r == want : rc == -1;

  printf("'%s': %s", s, ok ? "OK\n" : "FAIL");

  if(!ok)
    printf(" rc=%d got %ld want %ld\n", rc, (long)r, want);

  failed |= !ok;
}

int
main(void) {
  setenv("TZ", "UTC", 1);
  tzset();

  t("@0", 0, 1);
  t("@86400", 86400, 1);
  t("@-86400", -86400, 1);
  t("now", NOW, 1);
  t("2020-01-02", 1577923200L, 1);
  t("2020-01-02 03:04:05", 1577934245L, 1);
  t("2020-01-02T03:04:05", 1577934245L, 1);
  t("2020-01-02T03:04", 1577934240L, 1);
  t("2020-01-02T03:04:05Z", 1577934245L, 1);
  t("2020-01-02T03:04:05+01:00", 1577930645L, 1);
  t("2020-01-02T03:04:05-02:30", 1577943245L, 1);
  t("13:30", NOW + 5400, 1);
  t("today", NOW, 1);
  t("tomorrow", NOW + 86400, 1);
  t("yesterday", NOW - 86400, 1);
  t("1 day", NOW + 86400, 1);
  t("+2 hours", NOW + 7200, 1);
  t("2 days ago", NOW - 172800, 1);
  t("1 week", NOW + 604800, 1);
  t("1 minute 30 seconds", NOW + 90, 1);
  t("@86400 +1 day", 172800, 1);
  t("2020-01-02 -1 day", 1577836800L, 1);
  t("", NOW, 1);
  t("xyz", 0, 0);
  t("2020-13-01", 0, 0);
  t("2020-01-02 25:00", 0, 0);
  t("1 fortnight", 0, 0);
  t("@", 0, 0);
  return failed;
}
