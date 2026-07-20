#ifndef HISTORY_H
#define HISTORY_H

#include "../lib/typedefs.h"
#include "../lib/stralloc.h"

#define DEFAULT_HISTSIZE 128

/* interactive command history.
 *
 * Two independent stores back this:
 *
 *  - an in-memory ring of the commands entered THIS session, bounded by
 *    $HISTSIZE. It is always fully resident; it's small.
 *
 *  - the on-disk history file inherited from previous sessions. This is
 *    only ever mmap()'d (an O(1) syscall -- pages are faulted in by the
 *    kernel on demand, nothing is actually read off disk up front) and
 *    then walked with line iterators, one entry at a time, exactly as
 *    far as the user actually scrolls or searches. A shell started
 *    against a multi-gigabyte history file costs the same at startup
 *    as one started against no history file at all: nothing is parsed
 *    until an up-arrow or `history` asks for it.
 *
 * on disk, one history entry is always exactly one line: a command that
 * spans multiple lines has its embedded newlines escaped (history_encode)
 * when appended, and unescaped again (history_decode) when read back, so
 * the line iterators never need to understand shell quoting to find
 * entry boundaries -- they just look for '\n'.
 *
 * new commands are appended to the file immediately (history_add), never
 * batched and rewritten wholesale at exit: that way a crash doesn't lose
 * history, and several concurrently running shells append their own
 * entries without clobbering each other's.
 */

/* ---- in-memory ring of this session's commands (newest at head-1) ---- */
extern char** history_session;
extern unsigned int history_session_cap;
extern unsigned int history_session_count;
extern unsigned int history_session_head; /* index of the oldest live entry */

/* ---- on-disk file, mmap'd once and never fully scanned ---- */
extern const char* history_file_map;
extern size_t history_file_len;
extern char history_file_path[]; /* resolved once by history_init */

/* lazy backward-scan cache of file entries already visited this run,
 * newest-first (index 0 = most recent line in the file). Grows one
 * entry at a time, only as far as history_file_entry() is asked to go. */
extern size_t* history_cache_off;
extern size_t* history_cache_len;
extern unsigned int history_cache_count;
extern unsigned int history_cache_cap;
extern size_t history_cache_scanpos; /* where the next backward step resumes */
extern int history_cache_exhausted;  /* backward scan reached start of file */

/* ---- arrow-key browsing state ---- */
extern unsigned int history_cursor; /* 0 = live edit line, 1.. = older entries */
extern stralloc history_pending;    /* in-progress edit line, stashed while browsing */

void history_init(void);
void history_shutdown(void);

void history_session_resize(void);

void history_add(const char* s, size_t len);

void history_clear(void);
void history_print(void);

void history_prev(void);
void history_next(void);

/* raw line access, shared by history_print/history_prev/history_next */
int history_file_entry(unsigned int index, const char** line, size_t* len);
int history_file_next(size_t* pos, const char** line, size_t* len);

/* puts the entry <cursor> positions back (1 = newest) on the edit
 * line; shared by history_prev/history_next */
void history_show(unsigned int cursor);

void history_encode(const char* s, size_t len, stralloc* out);
void history_decode(const char* s, size_t len, stralloc* out);

#endif /* HISTORY_H */
