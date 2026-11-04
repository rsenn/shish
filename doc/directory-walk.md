# Reading directories: `lib/dirlist` and `lib/walk`

No builtin calls `opendir()`/`readdir()` itself. A directory is read by `lib/dirlist` (one level), a tree is
walked by `lib/walk` (many levels, built on `dirlist`). Both are generic: they use only `lib/arena`, `byte`, `str`
and `utf8`, so they have no shell dependency and can be copied into another project whole.

| who | uses | why |
|---|---|---|
| `rm`, and `mv` across devices | `walk` | children first, then the directory; never follows a link |
| `chmod -R` | `walk` | directory first; a link met below the operand is left alone |
| `cp -R` | `walk` | directory created on the way in, mode and times set on the way out |
| `find` | `walk` | `-prune`, `-depth`, `-maxdepth`, `-xdev`, `-H`/`-L`, `-empty` |
| `ls`, `ls -R` | `dirlist` | the whole listing is sorted and measured before any subdirectory is entered |
| tab completion | `dirlist` | one level, no `stat` |

## `lib/dirlist`: one directory, read whole

```c
arena a;
struct dirlist d;
const char* p;

arena_init(&a, &arena_heap, 0);
d = dirlist_read(&a, "dir", DIRLIST_NOHIDDEN);
if(!d.s)
  return -1;                       /* errno says why */
dirlist_foreach(p, d)
  use(dirlist_type(p), dirlist_name(p));
arena_free(&a);                    /* or arena_rewind() to a position taken before the read */
```

The directory is read into one packed blob in the caller's arena, then closed. A record is a type byte, the name
and a NUL (2 bytes of overhead per entry); there is no per-entry allocation and nothing to free one by one.

- **Types** are `DL_REG`, `DL_DIR`, `DL_LNK`, `DL_OTHER`, or `DL_UNKNOWN` where the platform gives none. They come from
  `d_type` and are the entry itself, not its target (a symlink is `DL_LNK`).
- **Names** are UTF-8 on every platform (`FindFirstFileExW` plus `u8fromu16` on Windows).
- **Flags:** `DIRLIST_DOTS` keeps `.` and `..` (dropped by default); `DIRLIST_NOHIDDEN` drops every name starting
  with `.` (wins over `DOTS`); `DIRLIST_SORT` sorts by `str_diff()`. `dirlist_sort(a, &d, cmp)` sorts an existing list
  with another comparator. `ls` sorts its own entries (by time or size) and does not use the flag.
- **Failure:** `d.s == NULL` and `errno`. An empty directory is a valid empty list (`d.n == 0`).
- **Cost:** built in place in the arena, trimmed to its exact size at the end. A level is dropped by rewinding the arena
  to the `arena_tell()` taken before the read. 1M files cost about 24 MB; streaming would be smaller, but it
  would be a second code path and the time is the same (the syscall per entry dominates).

## `lib/walk`: one tree, one visit per call

```c
struct walk w;
struct walk_ent* e;

if(walk_open(&w, root, WALK_POST | WALK_ERR))
  return -1;                       /* ENOMEM only */
while((e = walk_read(&w))) {
  switch(e->phase) {
    case WALK_ERR:  /* e->err, e->path */ break;
    case WALK_POST: /* a directory, its children are done */ break;
    case WALK_FILE: /* everything else */ break;
  }
}
walk_close(&w);
```

The handle is the caller's `struct walk` (on the stack, no `malloc` for it), so walks nest: `find -exec rm -r` runs one
inside another. There is one root per walk; a builtin loops over its operands and opens one walk for each.

### What a visit is

`e->phase` is one of the bits below, and the same bits in `walk_open()`'s flags choose which visits are reported.
`WALK_FILE` is always reported and is the zero-flags default, so a file-only loop has no `switch`.

| bit | visit |
|---|---|
| `WALK_FILE` | anything that is not a directory: regular file, special file, a symlink (unless followed) |
| `WALK_PRE` | a directory, before its children. `walk_skip()` here leaves it unread; its `WALK_POST` is then not reported |
| `WALK_POST` | a directory, after its children. Not reported after an error or a skip |
| `WALK_ERR` | the root or a directory could not be examined or read; `e->err` is the `errno`. No `WALK_POST` follows |
| `WALK_CYCLE` | `WALK_LOGICAL`: the directory is already being walked above it. It is reported and not entered |

An error that was not asked for (`WALK_ERR` not in the flags) skips that directory silently. A file as the root is a
single `WALK_FILE` at depth 0.

### The entry

| field | meaning |
|---|---|
| `path`, `len` | the whole path, in the walker's one buffer: valid until the next `walk_read()`. Separators are `/` |
| `rel` | the path below the root, `""` for the root: `cp -R` builds `dst/rel` from it |
| `name` | the last component (the whole root string for the root) |
| `depth` | 0 for the root |
| `type` | `DL_*`, **never** `DL_UNKNOWN`: the walker `lstat()`s only when the directory gave no type |
| `islink` | the entry is a symlink; `type` is the target's when it was followed |
| `err` | `WALK_ERR`: the `errno` |
| `kept` | `WALK_POST`: `walk_keep()` was called somewhere below |

### Policy

| | |
|---|---|
| default | **physical**: a symlink is a `WALK_FILE` of type `DL_LNK` and is never entered |
| `WALK_COMFOLLOW` | follow the root if it is a symlink (`find -H`, `cp -H`) |
| `WALK_LOGICAL` | follow every symlink (`find -L`, `cp -L`); an ancestor `(dev, ino)` chain turns a loop into `WALK_CYCLE` |
| `WALK_XDEV` | a directory on another device is reported but not read (`find -xdev`) |
| `w.maxdepth` | set after `walk_open()`; `-1` is unlimited. Directories at that depth are reported, not read |

A root written `link/` is looked at as `link` unless a follow flag is set, so a trailing slash cannot make the
kernel resolve the link behind the caller's back. A builtin that wants the kernel's meaning (`chmod -R`, `find`,
`cp -R`) passes `WALK_COMFOLLOW` itself when the operand ends in `/`.

### The other calls

- `walk_stat(&w)`: the `stat` of the current entry as the policy says (`lstat`, or `stat` when followed), cached for this
  entry; `NULL` with `errno` on failure. `find` takes it only when a test needs it, so `find . -name x` does not
  `lstat` every entry.
- `walk_keep(&w)`: the current entry stays (refused, or its removal failed): its parent, and the parent's parents, are
  reported with `e->kept` at `WALK_POST`. `rm` uses it to leave a directory alone without a second error.
- `walk_skip(&w)`: only meaningful after a `WALK_PRE`.
- `walk_kids(&w)`: the current directory's entries *as they are now* (a `dirlist`), read when asked and valid until the
  next `walk_read()`. At `WALK_PRE` the walk then goes through that same list instead of reading the directory again; at
  `WALK_POST` it is read afresh, which is what `find -empty -delete` needs.
- `walk_close(&w)`: frees the three arenas (lists, path, frames).

### How it is built

- **Iterative.** An array of frames (one per open level, about 70 bytes) replaces the C stack; a 1500-level chain is
  walked in the unit test.
- **One descriptor at a time.** A directory is read whole and closed before its first child is visited, so the depth of
  the tree does not use descriptors.
- **Memory.** Three arenas: directory lists (a level is rewound when it is popped), the path (one growing contiguous
  string), the frames. They are plain `lib/arena`, no `stralloc` and no `alloc_re`.
- **The directory is looked at again right before it is opened.** If it has become something other than a directory
  since it was listed (replaced by a symlink), the walker reports `ENOTDIR` instead of entering it.

### Limits

- Between that second look and `opendir()` a directory can still be swapped for a symlink. Closing it needs
  `openat(O_NOFOLLOW)`; this is listed in `TODO.md`.
- Paths are strings, so the OS path limit applies (about 4096 bytes on Linux).
- POSIX only. On `WINDOWS_NATIVE` it compiles (`stat` stands in for `lstat`), has no cycle check, and its unit test is
  skipped; `dirlist` has a full Windows backend.

## Writing a client

- **Ask for the phases you handle.** Do not filter phases in the loop. `rm` asks for `WALK_POST | WALK_ERR`, `cp -R` for
  `WALK_PRE | WALK_POST | WALK_ERR | WALK_CYCLE`, `chmod -R` for `WALK_PRE | WALK_ERR`.
- **Keep the builtin's own wording and options in the loop.** Errors are visits, so the message and the `-f` handling stay
  in the builtin; there is no callback.
- **Do not follow links unless the utility's options say so**, and never in anything that deletes. `find` refuses `-delete`
  with `-H` and `-L` for that reason.
- **Use `rel` and `depth`, not string surgery on `path`.**
- **Test in a private directory** (`mktemp -d`) and check that what a link points at survives; see `tests/builtin-rm.sh`
  and `tests/lib/walk_test.c`.

## Tests

`tests/lib/dirlist_test.c` and `tests/lib/walk_test.c` build a tree under `$TMPDIR` and are registered with `ctest`
(`walk_test`: symlink loops, links to sibling directories and to `..`, `link/` roots, unreadable directories, `walk_skip`,
`walk_keep`, `maxdepth`, `WALK_XDEV`, two walks in step, 20000 entries, a 1500-level chain; clean under ASan/UBSan).
The builtins have their own: `tests/builtin-{rm,chmod,find,cp,ls}.sh` and `tests/term-complete.sh`.
