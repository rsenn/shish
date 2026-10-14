# Path library: assessment and revised design

Scope: `shish/lib/path.h` + `lib/path/*.c` (the consolidated, authoritative copy), compared with
`c-utils/lib/path*` (the older, wider copy; tech debt, not a source) and `qjs-modules/{include/path.h,src/path.c,quickjs-path.c}`
(a third, DynBuf-based copy). All callers in the three projects were surveyed. Statements marked
**[run]** were reproduced with a test program; **[read]** were found by reading only.

Nothing in this document is implemented yet. It is the proposal.

---

## 0. Summary (read this first)

1. **One function family, one convention.** Every operation takes `(const char* p, size_t n)` views,
   never a `NUL`-terminated string and never a `stralloc`. Callers that have a C string pass
   `str_len(s)` (this is what `diet-coding` asks for anyway). All `_b`, `b`, `_s`, `_sa`, `1/2/3/4/5`
   suffix variants disappear: roughly 90 distinct c-utils/qjs-modules names collapse into ~25 functions.
2. **Readers return views, writers are `snprintf`-shaped.** A reader returns an offset or a length
   into the caller's memory (zero copy). A writer is `size_t f(char* out, size_t cap, ...)`: it returns
   the length it *needs* and writes only if it fits. `f(0, 0, ...)` is the "precalculate the shape"
   call the brief asks for; allocation is entirely the caller's (heap, stack, arena, `stralloc`,
   `DynBuf`, a JS string). Upper bounds are documented so most callers need no second call.
3. **`stralloc` survives in exactly four places**: `path_getcwd`, `path_readlink`, `path_resolve`
   (symlink-following realpath) and `path_gethome`-like lookups: the cases where the *output size is
   unknown until the OS answers*. Lexical operations (cat, canon, relative, absolute) never need it.
4. **Two iterators make the rest trivial**: a forward one (`path_next`: the prefix up to and
   including the next component) and a strictly-shrinking backward one (`path_parentlen`). Today every
   walker re-invents them: `mkdir -p`, `rmdir -p`, `realpath --relative-to`, `dirname`, `cd`, the
   module resolver in `qjsm`.
5. **Join semantics: "a absolute `b` wins"** (Python `os.path.join`, Rust `PathBuf::push`,
   Node `path.resolve` step). One total function, lexical, no normalisation; `canon` is a separate
   call. Section 5.4.
6. **Several real defects exist today** (section 2). The worst: `path_is_absolute()` is `rootlen & 1`,
   so `//x` is "relative" **[run]**; `path_commonlenb("/a","a")` returns `(1,0)` **[run]**;
   `path_dirname` in c-utils returned `""` for `usr`, `/usr`, `/` (fixed in the c-utils working tree on 2026-10-13, not committed).
7. **The private header becomes `lib/path/internal.h`** with short upper-case macros (section 9; migration plan: section 12).
   Today clients already reach into it (`builtin_realpath.c` uses `path_component`), which proves two of
   its primitives are public API in disguise.
8. **Delete or relocate** everything that is not about path *strings*: `path_exists`, `path_access`,
   `path_is_directory`, `path_is_root`, `path_find(dir,name)`, `path_quote_spaces`, `path_mmap_read`,
   `path_split(strlist)`. Section 8.

---

## 1. Inventory

### 1.1 Three copies, three naming schemes

| | shish `lib/path` | c-utils `lib/path` | qjs-modules `src/path.c` |
|---|---|---|---|
| size | 13 `.c`, ~1060 lines | ~95 `.c`, ~2150 lines | 1208 + 781 (binding) lines |
| buffer arg | `(p, n)`, suffix `b` | `(p, n)`, suffix `_b` / none (recently flipped) | `(p, n)`, digit = arg count (`path_dirlen2`) |
| NUL-terminated | `path_basename(p, &len)` only | `_s` suffix | `1` suffix |
| output | `stralloc` | `stralloc` | `DynBuf` |
| separator set | `/` (+`\` when `WINDOWS`) | same, via `PATHSEP_S_MIXED` | `/` and `\` on Windows, scanning with UTF-8 aware loops |
| private header | `lib/path_internal.h` (macros) | `lib/path_internal.h` (different macros) | none (all public in `include/path.h`) |

`SizePair`, `PATHSEP_C/S`, `PATHLISTSEP_*`, `PATH_FNM_*`, `path_component1/3`, `path_separator1/2`,
`path_skip1/2/3` are defined **three times** with slightly different bodies.

### 1.2 What each copy has that the others lack

* shish: lexical-only canonicalisation that is *correct* against `realpath -m -s` (below), a
  root-aware `dirlen`, `rootlenb` that understands drives, `fnmatch` on non-NUL strings,
  `basename` with POSIX trailing-separator rules.
* c-utils: concat/append/prepend/join, `collapse`, `normalize`, `relative_to`, `common_prefix`,
  `split`, `num`, `find`, `access`/`exists`/`is_*` predicates, `quote_spaces`, `basepos`, `trim`.
  Four different lexical normalisers (`canonical`, `canonicalize`, `collapse`, `normalize`) plus two more
  in `genmakefile/path.c` (`path_clean`, `path_normalize_sa`).
* qjs-modules: Node-shaped surface (`extname`, `parse`, `format`, `resolve`, `slice`, `at`, `isin`,
  `equal`, `search`), stat predicates, `gethome(user)`.

---

## 2. Defects found

Verified against `coreutils` and `node` where applicable. Test harness: a 34-path table plus a
3000-path random fuzz of `path_canonicalize` against `realpath -m -s`.

| # | where | defect | status |
|---|---|---|---|
| D1 | shish `path.h:path_is_absolute` | `(path_rootlenb(p,n) & 1)`: `"//"`, `"//a"`, `"//a//b//"` report **not absolute** (rootlen is 2). Works for `/` (1) and `C:/` (3) by accident of parity. Node: `isAbsolute("//")` is true. | **[run]** |
| D2 | shish `path_commonlenb` | `("/a","a")` returns `(1,0)` and `("a","/a")` returns `(0,1)`: the final `b += seplen(b)` / `t += seplen(t)` are unconditional, so one side consumes a root the other does not have. A common prefix of absolute and relative is `(0,0)`. | **[run]** |
| D3 | shish `path_commonlen` (string version) vs `commonlenb` | `("/usr/local/bin","//////usr//local//share")`: string version returns **1**, buffer version returns **(11,18)**. The string version advances both strings by one separator in lock-step and so cannot see through `//`. Same function name family, different answers. | **[run]** |
| D4 | shish `path_canonicalize` | Fuzz: 0 differences from `realpath -m -s` except for a **leading `//`**: `//a` stays `//a`, `///a` stays `///a` (3+ slashes are required by POSIX to mean one). 126/1709 inputs, all of them with a leading `//`. | **[run]** |
| D5 | c-utils `path_dirname` | (fixed in the c-utils working tree, not committed) `usr`, `/usr`, `/`, `usr/`, `.` gave `""`. Cause: `path_dirlen` was swapped from "rchr index, `len` if none" to a shrinking length while `path_dirname` still compared `path[i]=='\0'`. Same bug class as the `byte_chrs` OOM. | fixed, **[run]** |
| D6 | c-utils `path_internal.h:path_isname` | `#define path_isname(p) ((p)[u8s_chrs(...)] != '\0')` is documented "a bare name, no separator" but is true when the string **contains** a separator. qjs-modules' copy has the opposite (correct) polarity, so the same name means opposite things in two projects. | **[read]** |
| D7 | qjs-modules `path.h` | `#define path_isrelative(p) !path_isabsolute1(p)` is not parenthesised. | **[read]** |
| D8 | shish `path_gethome` (non-Windows) | returns from inside the loop without `mmap_unmap(map,size)`: leaks the mapping per successful call. The field walk (`&(":\n"[i == 2])` as the charset, so field 2 skips to end of line unless the uid matches) is opaque, but it returns correct homes for uids 0, 1, 33, 1000, 65534 and null for an unknown uid **[run]**; I have not shown it to be wrong on malformed lines. `id == uid` also compares `unsigned` with `int`. | **[read]** |
| D9 | shish `exec_path` | returns `str_dup(path)` from a `static char path[PATH_MAX]`, copying with `str_copyn` into a fixed buffer: the name-length check is implicit in `PATH_MAX - pi - 1`. | **[read]** |
| D10 | shish `builtin_ln` | uses libc `basename()` (GNU/POSIX variants differ on trailing `/`) while `cp` uses its own `cpmv_base` and `basename` builtin uses `path_basename`: **three** basename implementations in one directory. | **[read]** |
| D11 | shish `path_realpath` | copies `cwd + sep + path` into a `stralloc` `full`, then `path_canonicalize` copies it again into `rest`: 3 copies and 3 allocations for what is a purely lexical operation when `symbolic != 0`. | **[read]** |
| D12 | c-utils `path_skip_component` / `path_skip_separator` / `path_trim` | decode UTF-8 per byte (`u8_len`) to compare against an ASCII separator. A continuation byte can never equal `/` or `\`, so the decode is dead weight (and needed a special-case for invalid lead bytes). Only multi-byte *separators* (none exist) would justify it. | **[read]** |
| D13 | `path_dirlen` use as iterator | `dirlenb("/")==1`, so `while((n = dirlen(p,n)))` never terminates at the root. `builtin_rmdir.c` works around it with `len > 1 \|\| !path_is_absolute(...)`. A real reverse iterator must strictly shrink. | **[read]** |

Behavioural differences between the shish/POSIX reading and Node `path.posix` that the design must
state, not stumble over **[run, node]**:

| input | POSIX tools / shish | Node `path.posix` |
|---|---|---|
| `basename("//")` | `/` | `""` |
| `dirname("//a")` | `/` | `//` |
| `normalize("a/")` | `a` (trailing separator dropped) | `a/` (kept) |
| `normalize("//a//b")` | `//a/b` (shish) / `/a/b` (`realpath`) | `/a/b` |
| `relative("/a","/a")` | `.` | `""` |
| `join("a","")`, `join("","")` | n/a | `a`, `.` |
| `extname(".bashrc")`, `extname("a.")` | n/a | `""`, `.` |

Decision: the **C core follows POSIX utilities** (it serves a shell); the QuickJS binding adapts to
Node in its own thin layer (empty-basename, trailing-separator flag, `""` for equal relative). The
core offers the flags needed for that adaptation, and nothing Node-specific lives in the core.

---

## 3. Client survey

Hit counts are `grep` lines outside `lib/path/`, so they include a few comments; they show shape, not
exact numbers.

### 3.1 shish

| client | what it does by hand today | wants |
|---|---|---|
| `builtin_dirname.c` | three `while` loops over `/` | `path_dirname(p,n,&len)` (a view; `"."` fallback is a string literal, no copy) |
| `builtin_basename.c`, `find.c`, `sh_*.c` (7 sites) | `path_basename(p, NULL)` on NUL strings | `path_basename(p,n,&len)` |
| `builtin_ln.c` | libc `basename()` | same |
| `builtin_cp.c:85,651` | private `cpmv_base()`, then `copys + catc('/') + catb` | `path_basename` + `path_cat` |
| `builtin_mkdir.c` | NUL-patching loop skipping `/` runs, `path_len_s` | forward prefix iterator |
| `builtin_rmdir.c` | `path_lensa(path_dirlenb, sa)` plus root special case (D13) | backward iterator `path_parentlen` |
| `builtin_realpath.c:69-75` | uses **private** `path_component`; hand loop to emit `../` | `path_rel(...)` |
| `builtin_realpath.c`, `readlink.c` | walk back to the last `/` to get the parent directory (`while(n && s[n-1] != PATHSEP_C) n--`) | `path_dirlen` / `path_parentlen` |
| `builtin_cd.c` | splits `CDPATH` with `str_chr(cdpath, ':')`, manual `byte_copy` + `'/'` + `str_copy` into `PATH_MAX`, then `path_realpath` | list iterator + `path_cat` with cap check |
| `exec_path.c`, `builtin_source.c`, `builtin_command.c`, `exec_check.c`, `exec_hash.c`, `exec_type.c`, `eval_function.c` | `name[str_chr(name,'/')]` to ask "does this name have a slash?" (7 sites, three spellings); `exec_path` and `source_search_path` also split `PATH` themselves | `path_isname(p,n)`; list iterator |
| `builtin_command.c:44` | `command_abspath`: cwd + `'/'` + strip `./` | `path_abs(...)` |
| `builtin_mktemp.c` | `str_rchr(argv0,'/')`; `base + '/'` | `path_basename`, `path_cat` |
| `builtin_cp.c`, `chmod.c`, `find.c`, `ls.c`, `rm.c`, `ln.c` | **the walker idiom**, five times (`rm.c` also uses `readdir`, not read): save `dirlen`, check trailing separator, `catc('/')`, `cats(name)`, `nul`, recurse, restore `len` | `path_cat` in place; later one `dirwalk` |
| `term_complete.c` **[read: comment only]** | splits the word into `dir` / `base`, special-cases "`.` when the word had no `/`" | `path_dirlen` + `path_basename` split |
| `expand_tilde.c`, `sh_gethome.c` | `path_gethome(uid)` | keep, but fix D8 |

### 3.2 c-utils

| client | pattern |
|---|---|
| `genmakefile.c`, `pathtool.c`, `src/genmakefile/path.c` | `path_absolute_sa` → `path_canonical_sa` → `path_collapse_sa` on one `stralloc` (3 passes, 3 in-place mutators); `path_relative_to_s/_sa` ×12; own `path_prefix`, `path_extension`, `path_wildcard`, `path_clean`, `path_normalize_sa` (re-implementations) |
| `ar-wrap`, `cc-wrap`, `cmake-run`, `exe-wrap`, `msys-shell` | each has its **own** `path_lookup(cmd, out)` (five copies) plus `path_find(dir,name,out)` calls; each splits the `PATH` env by hand and scans for `/` **and** `\\` with two `byte_rchr` calls |
| `pkgcfg.c` (15), `ftpcopy.c` (4), `plsconv`, `logserial`, `redir`, `reg2cmd`, `mediathek-list`, `exec-intercept`, `compiler-wrapper` | `str_rchr(s,'/')` / `strrchr` to find a basename or dirname; `ftpcopy.c`, `uri.c` do it on **remote/URL paths that must always use `/`**, not the host separator |
| `lib/cpp/*` | `path_join(dir, file)` returning a heap string, repeated per include dir |

### 3.3 qjs-modules / qjsm

| client | pattern |
|---|---|
| `qjsm.c:1666-1692` (module resolver) | `path_dirlen1` → `path_append3` → `path_append2` → `path_skipdotslash2` → `path_normalize2` → `path_absolute1` → `path_normalize1`: seven steps for "resolve specifier against importing file" |
| `qjsm.c:262, 1490, 1500, 1686, 1830, 1875` | `!name[path_component1(name)]` = "is a bare name" (6 sites) and `path_isexplicit` / `path_isdotslash` / `path_isdotdot` = Node's module specifier classes |
| `child-process.c:266` | `path_isname` (the correctly-polarised copy) |
| `quickjs-path.c` | Node `path` surface. Every `*_dbuf` method allocates a `DynBuf`, calls a `path_*3` function and converts. With snprintf-style writers it can measure, `JS_NewStringLen`-style allocate once, write. |

### 3.4 Plain string operations on what is effectively a path (candidates)

* shish: `builtin_realpath.c` (parent dir by hand), `builtin_readlink.c:50`, `builtin_cd.c:100-115`,
  `exec_path.c`, `builtin_mktemp.c:69-77`, `term_complete.c:326`.
* c-utils: 118 grep hits for `'/'`-oriented `str_rchr`/`stralloc_catc('/')`/`PATHSEP` outside
  `lib/path/`; `pkgcfg.c` (15) and `genmakefile.c` (11) lead.
* qjs-modules: `src/pointer.c` handles JSON Pointer (`/a/b`), which is *not* a filesystem path and must
  stay separate (different escape rules, `~0`/`~1`). No change.

---

## 4. Design conventions

1. **Inputs are views `(p, n)`.** Never `NUL`-scanned inside the library. Library functions never read
   `p[n]`. A `NUL`-terminated caller writes `f(s, str_len(s))`.
2. **Returns are `size_t` offsets/lengths** wherever the answer is "a prefix, a suffix, or a span of the
   input". Pointers only where a span has a non-input fallback (`dirname` → `"."`).
3. **Writers are `snprintf`-shaped**: `size_t f(char* out, size_t cap, ...)` returns the length the
   result needs (excluding the `NUL`). **All or nothing**: if `cap > result` it writes result + `NUL`;
   otherwise `out` is not touched at all (no truncated prefix, no `NUL`), and the call cannot be
   resumed or continued, so the caller retries from scratch with a buffer of `return + 1`. So:
   ```c
   size_t need = path_cat(0, 0, a, an, b, bn);       /* precalculate */
   char* out = arena_alloc(arena, need + 1);
   path_cat(out, need + 1, a, an, b, bn);            /* fill */
   ```
   Fixed bounds are macros (`PATH_CAT_MAX(an,bn)` = `an+bn+1`, `PATH_CANON_MAX(n)` = `n+1`, …) so
   stack/`PATH_MAX` callers skip the first call.
4. **Aliasing is defined.** `out` may equal an input prefix for `canon` (output never longer than input +1
   for `""`→`.`) and `cat(out, …, out, len, name, nl)` (this *is* "push a component", see 5.5).
5. **Only structural failure is an error**; a too-small buffer is reported by the return value
   (`>= cap`) with `out` left unwritten, never as a silent truncation. The one writer that can be impossible is `path_rel` (mixed absolute/relative or different
   drives): it returns `(size_t)-1`.
6. **Separators.** Input accepts `PATHSEP_CHARS` (`/` everywhere; `\` too on `WINDOWS`). Output writes
   `PATHSEP_C` unless the caller then runs `path_setsep(p,n,'/')` (an in-place, orthogonal relabeller).
   A build-time `PATH_POSIX_ONLY` macro (or a second tiny header) gives URL/remote-path code
   (`ftpcopy`, `uri`) a `/`-only variant of the same functions without a runtime flag.
7. **Bytes, not code points.** No UTF-8 decoding anywhere (D12). Separators are ASCII; every UTF-8
   continuation byte is `>= 0x80`.
8. **Lexical vs. filesystem is a hard line.** `lib/path/*` string functions never touch the
   filesystem. The few that do (`getcwd`, `readlink`, `resolve`, `gethome`) are in their own files and
   are the only ones allowed `stralloc`.
9. **`.` and `..` are components**, not special in the scanners; only `canon`/`rel`/`isdot*` interpret them.
10. **Root is a prefix, not a component.** `rootlen("C:/x") = 3`, `rootlen("//x") = 2`, `rootlen("x") = 0`.
    `//` policy: POSIX-undecided, so `canon` collapses it to `/` on POSIX targets (matches `realpath`,
    fixes D4) and keeps exactly two leading separators on `WINDOWS`/MSYS (UNC and `//host` semantics).

---

## 5. Proposed API

Names are the proposal; section 8 lists old → new.

### 5.1 Classification (macros/inline, no allocation)

| name | meaning |
|---|---|
| `path_issep(c)` | `c` is a separator |
| `path_rootlen(p,n)` | length of the root prefix (drive + separators); 0 for relative. **Fixes** D1/D2 by being the only place that knows about roots |
| `path_isabs(p,n)` / `path_isrel(p,n)` | root contains a separator. `C:` and `C:x` are relative, `C:/x` absolute, `//x` absolute |
| `path_isname(p,n)` | contains **no** separator: a bare command/module name. Replaces ~13 hand-rolled sites; one polarity (fixes D6) |
| `path_isexplicit(p,n)` | absolute, or starts with `./` or `../` (or is `.` / `..`): Node's "explicit relative" and exec's "contains a slash" notion without the other cases |
| `path_isdot(p,n)`, `path_isdotdot(p,n)` | the whole view is `.` / `..` (for component views) |

### 5.2 Scanners (the four primitives, now public)

These already exist privately as `path_seplen`, `path_comlen`, `path_rseplen`, `path_rcomlen`
(thin wrappers over `scan_charsetnskip` and friends). They stay, become public, one line each:

```c
size_t path_seplen (const char* p, size_t n);   /* run of separators at p        */
size_t path_comlen (const char* p, size_t n);   /* run of non-separators at p    */
size_t path_rseplen(const char* p, size_t n);   /* run of separators ending at p+n */
size_t path_rcomlen(const char* p, size_t n);   /* run of non-separators ending at p+n */
```

Everything below is expressible from these plus `rootlen`.

### 5.3 Decomposition and iteration (zero copy, return offsets/views)

```c
const char* path_basename(const char* p, size_t n, size_t* len);   /* POSIX basename(1) as a view      */
size_t      path_dirlen  (const char* p, size_t n);                 /* POSIX dirname(1) length (root-aware) */
const char* path_dirname (const char* p, size_t n, size_t* len);   /* view; "." literal if no dir part  */
size_t      path_extoff  (const char* p, size_t n);                 /* offset of ".ext" in basename, or n */

size_t path_parentlen(const char* p, size_t n);   /* backward step: strictly < n, 0 at the end        */
size_t path_next     (const char* p, size_t n, size_t pos);  /* forward step: end of next component, 0 at the end */
size_t path_ncomp    (const char* p, size_t n);   /* number of components; qjs `length`               */
```

* `path_dirlen("usr")` = 0 and `path_dirlen("/usr")` = 1: this is `dirname(1)`, kept because
  `builtin_dirname` and the binding need exactly that.
* `path_parentlen` is the reverse iterator the brief calls for. Different from `dirlen` at the root:
  `parentlen("/") == 0`, `parentlen("/a") == 1`, `parentlen("a") == 0`, `parentlen("a//b") == 1`.
  Ascending idiom, terminating by construction:
  ```c
  for(k = path_parentlen(p, n); k; k = path_parentlen(p, k))
    rmdir_prefix(p, k);                     /* rmdir -p: "a/b/c" -> "a/b" -> "a" */
  ```
* `path_next` is the forward counterpart; `mkdir -p`:
  ```c
  for(k = 0; (k = path_next(p, n, k)); ) {  /* "a", "a/b", "a/b/c" (root first) */
    char c = buf[k]; buf[k] = '\0';
    mkdir(buf, mode);
    buf[k] = c;
  }
  ```
  A macro `PATH_FOREACH(p, n, comp, clen)` yields the components themselves for the cases that want the
  names instead of the prefixes (`relative`, `common`).
* `path_extoff` follows Node/dotfile rules (`.bashrc` has no extension, `a.` has `.`): replaces
  qjs-modules `path_extpos1/extlen1/extname1` and genmakefile's `path_extension`. Replacing an
  extension is then `cat`-free: `fwrite(p, extoff)`, `fwrite(newext)`.

`path_at(p,n,i,&len)` and `path_slice` (qjs binding) are *implemented in the binding* on top of
`path_next`; they do not need to live in the C core.

### 5.4 Comparison

```c
size_t path_common(const char* a, size_t an, const char* b, size_t bn, size_t* blen);
int    path_cmp   (const char* a, size_t an, const char* b, size_t bn);   /* component-wise, ignores separator count */
int    path_isin  (const char* p, size_t pn, const char* dir, size_t dn);  /* p == dir or p below dir */
```

* `path_common` returns `alen` and writes `*blen`: the lengths of the common *whole-component* prefix
  in each input, **excluding** trailing separators, so a caller can immediately `path_seplen` forward
  to the remainder. Rules (fixing D2/D3): different roots → `(0,0)`; the root itself is part of the
  prefix when both have the same root; `//` vs `/` runs compare equal; `/usr/local/bin` vs
  `//////usr//local//share` → `(10, 17)`.
  It is the tokenisation of two inputs at once that the brief describes; `cmp`, `isin`, `rel` all
  reduce to it. The string-only `path_commonlen` is removed.
* `path_cmp` / `path_isin` replace qjs `path_diff4`/`equal4`/`isin4` and `path_compare2` in `qjsm.c`.

### 5.5 Composition (snprintf-shaped writers)

```c
size_t path_cat  (char* out, size_t cap, const char* a, size_t an, const char* b, size_t bn);
size_t path_catm (char* out, size_t cap, const char* p, size_t n, ... /* pairs, NULL-terminated */);
size_t path_canon(char* out, size_t cap, const char* p, size_t n);
size_t path_abs  (char* out, size_t cap, const char* p, size_t n, const char* cwd, size_t cwdn);
size_t path_rel  (char* out, size_t cap, const char* p, size_t pn, const char* base, size_t bn);
void   path_setsep(char* p, size_t n, char sep);                /* in place relabel, both separators → sep */
```

**`path_cat(a, b)` semantics (the decision that matters most):**

| a | b | result |
|---|---|---|
| any | empty | `a` |
| empty | any | `b` |
| any | **has a root** (absolute / drive) | `b` (**absolute right operand wins**) |
| `x` | `y` | `x` + one separator + `y`; no separator added if `x` already ends in one |

* Purely lexical: does not touch `.`/`..`, does not strip `./`, does not collapse repeats inside `a`
  or `b`. That is what makes it exactly the zero-copy "plan" the brief describes (the result is
  `a[0..an']` + at most one separator + `b[…]`; both source views stay as they are). It is
  associative on relative operands and idempotent on empty ones, so folding works.
* Precedent: Python `os.path.join`, Rust `PathBuf::push`, Node `path.resolve` per step. Rejected
  alternatives: Node `path.join` (concatenates even absolute `b`; surprising for a shell, where
  `cd "$dir" && … "$file"` must not silently re-root) and c-utils `path_concat` (silently strips a
  leading `./` from the *result*, which makes it non-associative).
* In-place use: `len = path_cat(buf, cap, buf, len, name, nl)` appends a component to the
  caller's own buffer. This is the walker idiom (5.6), with the capacity check built in
  (`len >= cap` ⇒ `ENAMETOOLONG`; the former open-coded `n + len + 1 > PATH_MAX` in `builtin_cd`).
* `path_catm` covers three or more fragments in one pass and is the arena-friendly form of c-utils'
  `path_concat` chain. The "last fragment with a root wins" rule is applied across the list.

**`path_canon(p, flags)`** replaces `canonical`/`canonicalize(symbolic=1)`/`collapse`/`normalize*`/
`path_clean*`/`path_normalize_sa*` (six implementations → one, the shish one, which already agrees with
`realpath -m -s`). Output never exceeds `n + 1`. Flags: `PATH_KEEPTRAIL` (Node `normalize("a/")==="a/"`),
`PATH_KEEPUNC` (leave exactly two leading separators). `".."` above the root of an absolute path is
dropped; above a relative start it is kept.

**`path_abs(p, cwd)`** is `path_cat(cwd, p)` followed by `path_canon`, as one call: it replaces the
`absolute → canonical → collapse` triple that `genmakefile.c`, `pathtool.c` and `builtin_cd.c` each
spell out. `cwd` is passed in (no hidden `getcwd` call), so the function is pure and testable.

**`path_rel(p, base)`**: lexical relative path from `base` to `p`; both must already be canonical (document;
callers use `path_abs`). Equal paths give `.` (POSIX), the binding maps that to `""` for Node. Returns
`(size_t)-1` for mixed absolute/relative or different drives. Bound: `pn + 3 * ((bn+1)/2) + 1`.
Implementation is `path_common` + `PATH_FOREACH` over the rest of `base` emitting `../`; this is
precisely what `builtin_realpath.c:relative_to()` and c-utils `path_relative_to` do by hand today, in
two different ways.

### 5.6 The walker idiom and its replacement

`cp`, `chmod`, `find`, `ls` and `ln` (and, from its `readdir` use, probably `rm`; not read) repeat: save `dirlen` → ensure trailing separator → `catc`/`cats(d_name)` → `nul`
→ recurse → restore `len`. With the conventions above:

```c
size_t base = len;                               /* remember */
len = path_cat(buf, cap, buf, len, name, nl);    /* push     */
if(len >= cap) { errno = ENAMETOOLONG; … }
recurse(buf, len, path_basename_off(buf, len));  /* name is buf + off: zero copy */
buf[len = base] = '\0';                          /* pop      */
```

A future `dirwalk(root, flags, cb)` belongs in a directory module, not here. Its path contract is
the one above: **one caller-owned buffer, pushes and truncations, callback gets `(buf, len, nameoff)`**,
so `find -name` never re-scans for the basename (`builtin_find.c:117` currently does). `openat`/fd-relative
traversal needs only `nameoff`, not the full path.

### 5.7 Search lists (`PATH`, `CDPATH`, `MANPATH`, include dirs)

**Separator set** `PATHLISTSEP_CHARS` (input) and `PATHLISTSEP_C` (output, one character):

| target | input accepts | output writes |
|---|---|---|
| `WINDOWS_NATIVE` | `;` only (`:` is the drive colon) | `;` |
| everything else (incl. MSYS/Cygwin) | `:` and `;` | `:` |

```c
size_t path_listlen (const char* l, size_t n);              /* length of the first element           */
size_t path_listnext(const char* l, size_t n, size_t pos);  /* start of the next element, -1 at the end */
#define PATH_LIST_FOREACH(l, n, elem, elen) ...             /* every element, empty ones included     */

size_t path_search(char* out, size_t cap, const char* l, size_t ln,
                   const char* name, size_t nl, int (*ok)(const char*, void*), void* arg);
```

* An empty element is the current directory (POSIX): `"a::b:"` yields `a`, `""`, `b`, `""`, and `path_cat`
  with an empty `a` already returns `name`. An empty list is one empty element.
* `path_listnext(l, n, pos)` returns `(size_t)-1` after the last element, so a trailing separator
  leaves one more, empty, element. The macro is the only caller most code needs.
* `path_search` joins each element with `name` by `path_cat`, skips elements too long for `cap`, and
  returns the length of the first candidate `ok()` accepts (0 if none). The predicate carries the policy:
  `exec_path` (executable, not a directory), `.` (readable), `command -v`. Callers apply it to bare names only
  (`path_isname`).
* `cd` uses the macro directly because a too-long candidate is an error there, not a skip.

Replaces `exec_path`, `source_search_path`, `CDPATH` in `builtin_cd`, c-utils' five `path_lookup`s and its
`path_split(strlist)`, and qjs-modules' `path_search`.

### 5.8 Filesystem-touching (kept, but isolated)

```c
int  path_getcwd  (stralloc* sa);                                   /* unknown size                 */
int  path_readlink(const char* p, size_t n, stralloc* sa);          /* unknown size                 */
int  path_resolve (const char* p, size_t n, stralloc* sa, int symbolic, const char* cwd, size_t cwdn);
size_t path_homeof(const char* user, size_t un, char* out, size_t cap);   /* was gethome(uid)/(name)    */
```

* `path_resolve` = `path_realpath`/`path_canonicalize(symbolic=0)`. For `symbolic != 0` it simply calls
  `path_abs` and never allocates, which fixes D11. Without it the `stralloc rest/link/tmp` triple stays
  (symlink expansion really does grow the string).
* `path_homeof` writes into the caller's buffer (no static `home[PATH_MAX+1]`, thread-safe, no D8
  leak). Only the signature changes (caller buffer, no `static`, and the missing `mmap_unmap`); the field-walk logic is correct as is and is left alone.

### 5.9 Scope: what is public, what is private, what waits (YAGNI cut)

Sections 5.1–5.8 are the full design space. Most of it has **no client today**. Evidence below is the
number of distinct caller files outside `lib/path*` (grep for `name(`, summed over the `_s`/`_sa`/`_b`
spellings in c-utils); qjs-modules ships its own Node-`path` binding, so its demand is for the JS API, not the C core.

**Observed demand.**

| project | what the clients actually call |
|---|---|
| shish | `path_basename` 5, `path_fnmatch` 4, `path_realpath` 3, everything else 1–2; the real demand is the hand-rolled idioms of 3.1 |
| c-utils | dirname ~20, canonical/collapse/normalize/clean ~20, `relative_to` ~15, is_absolute ~14, absolute ~9, concat/append/join ~8, extension 5, lookup 5, find 5, getcwd 7, readlink 5 |
| qjs-modules | `ncomp`, `cmp`, `isin`, `isexplicit`, `at`, `slice`, `equal`, `diff`: only the Node binding; they stay in the binding |

**Tier 1: public in `lib/path.h` (about 12 functions, each its own source file).**

| function | evidence |
|---|---|
| `path_rootlen`, `path_isabs` | needed by every other function; 14+ files |
| `path_basename`, `path_dirlen` | 20+ files |
| `path_canon` | ~20 files, replaces six implementations |
| `path_abs` | ~9 files |
| `path_rel` (+ `path_common`) | ~15 files; `rel` is built on `common` |
| `path_cat` | ~8 files plus the walker idiom in 5 shish builtins |
| `path_extoff` | c-utils 5, qjs-modules 3 |
| `path_isname` | 7 shish sites plus 5 c-utils `path_lookup` copies |
| `path_listnext` (+ `PATH_LIST_FOREACH`) | same 12 sites |
| `getcwd`, `readlink`, `realpath`, `gethome`, `fnmatch` | exist already, signature clean-up only |

**Tier 2: stay private in `path_internal.h` (macros, not API).** `seplen`, `comlen`, `rseplen`, `rcomlen`,
`isdot*`, `path_next`, `PATH_FOREACH`. `rel` and `common` use them internally; clients do not.

**Tier 3: deferred until a second client asks (not in the header).**

| function | why waiting |
|---|---|
| `path_parentlen` | one client (`rmdir -p`) |
| `path_ncomp`, `path_cmp`, `path_isin`, `path_isexplicit`, `path_at`, `path_slice` | qjs binding only |
| `path_catm` | no variadic join caller; fold with `path_cat` |
| `path_setsep` | Windows-only relabelling, one site |
| `path_dirname` (view form) | `dirlen` + a literal `"."` covers it |
| `PATH_*_MAX` sizing macros | add with the first fixed-buffer caller |
| `path_inside` | hypothetical (jail); speculative, no client |

**Contract for tier 1** (unchanged from section 4): views `(p, n)`, writers all-or-nothing
(`ret >= cap` means `out` is untouched), only structural failure is an error.

**Rollout and status** (2026-10-14).

1. **c-utils prototype, batch 1** (`rootlen`, `basename`, `dirlen`, `canon`, `cat`): **done**, every c-utils client migrated
   (44 files), builds, unit tests green. See "Results" below.
2. **shish port of batch 1**: **done** for the five functions and their clients (`rm`, `chmod`, `find`, `ls`, `cp`, `rmdir`,
   `command`, `sh_*` mains, `basename`).
3. **c-utils prototype, batch 2** (`abs`, `rel` + `common`, `extoff`, `isname`): **done**; `path_listlen`/`PATH_LIST_FOREACH` **not
   prototyped**: c-utils has no client for it (its five `path_lookup` copies iterate an already split `strlist`). The clients are
   shish (4 sites) and qjs-modules `path_search` (see 5.10), so the iterator is prototyped in shish, not in c-utils.
4. **shish port of batch 2**: **done** (2026-10-14): `path_abs`, `path_rel`, `path_common`, `path_extoff`, `path_isname` and the `_sa` forms are in `lib/path/`; clients: `realpath --relative-to` (`path_rel_sa`, its own `relative_to` deleted), `path_isname` in `exec_hash`/`exec_check`/`exec_type`/`eval_function`/`command`/`source`, `path_cat` in `exec_path` and `cd` (CDPATH), `path_dirlen` in `readlink -f`/`realpath`, `path_basename` in `mktemp`. Also fixed on the way: `realpath //tmp///` kept the doubled root. The list functions (5.7) were written afterwards: `exec_path`, `source` and `cd` use them, and PATH/CDPATH accept `;` as well as `:`. `cp` and `ln` use `path_basename` (fixes/407); `path_commonlen`/`path_commonlenb`/`SizePair`/`path_component` removed. **qjs-modules**: lexical core migrated (`src/path-core.c`, `qjsm.c` uses `path_isname`, `path_search` uses `path_cat`); `quickjs-path.c` partly moved, uncommitted.

**Results of the c-utils verification** (worktree at HEAD plus the tree's two uncommitted path files; the real tree
does not build today because the untracked `lib/stralloc/readclose_append.c` redefines `readclose_append`).

| check | outcome |
|---|---|
| build | every program links (74 executables); `tests/test_path.c` was already broken before (`path_is_absolute` called with one argument) and is fixed |
| unit tests | 9 new path tests green (`basename`, `rootlen`, `cat`, `canon`, `dirlen`, `rel`, `abs`, `common`, `extoff`), including all-or-nothing, in-place and root cases; `all_tests`: 495 of 502 pass, the 7 failures are `fmt` and `io` (not path; not compared against a baseline, because `all_tests` did not build before) |
| differential `path_canon` vs old `canonical_sa` + `collapse_sa` | equal on every ordinary input; the old pair is wrong at the edges (see below) |
| `genmakefile` on a three-file sample, two layouts | output identical to the old binary (only the "Generated by" line differs) |
| `pathtool` on nine argument sets | identical, except `-f /usr/../etc`: old `etc`, new `/etc` (old lost the root) |
| size (`text`) | `genmakefile` 225739 → 224347 (-1392), `pkgcfg` 58052 → 57587 (-465), `pathtool` 28926 → 29422 (+496, it uses the most glue) |

**Bugs the old code had that the new functions fix** (found by the differential test, not planned):
`a/b/..` canonicalized to `a/` (trailing separator kept), `a/..` to the empty string, `/..` stayed `/..`, `./a` stayed `./a`,
`dirlen("/usr")` was 0 so popping a parent in `path_canonicalize` turned `/usr/../etc` into `etc` (root lost), and
`genmakefile -M` with a build directory under `/tmp` aborted (exit 134) or wrote `DISTDIR = tmp/...`.

**What the verification changed in the design** (the sketch above was wrong or incomplete in these places):

| finding | consequence |
|---|---|
| `path_canon` has no `flags`: Node's trailing-separator rule is two lines in the binding (append one if the input ended in a separator and the output does not), `KEEPUNC` has no client | signature is `path_canon(out, cap, p, n)`; 5.5 updated |
| exact output length of `canon` needs a second pass, because `..` reads back what was written | `canon` is all-or-nothing against the **bound**: `cap >= n + 2`, else returns `n + 1` and writes nothing; `abs` the same with `cwdn + n + 2` |
| every c-utils and shish client holds a `stralloc`, so each call site needs `stralloc_ready` + length bookkeeping | one glue function per writer in the same module, one file each: `path_cat_sa`, `path_canon_sa`, `path_abs_sa`, `path_rel_sa`. `path_cat_sa(sa, sa->s, dirlen, name, nl)` appends **in place** without a copy (the walker idiom), otherwise it builds aside so `out` may overlap `a` or `b`. The caller-buffer writers stay the core; the glue is `stralloc`-specific |
| `path_append` on an empty `stralloc` is a plain copy; the `path_append` + `path_append` chain collapses to one `path_cat_sa` | the `append` sites became `stralloc_copyb` (empty target) or one `path_cat_sa` call |
| 25 call sites want the basename as a C string | `path_basename_s(s)` macro (`path_basename(s, str_len(s), 0)`); the view form keeps `len` optional |
| 20 call sites want "dirname into a `stralloc`, `.` if none" | `path_dirname` (c-utils) stays as a convenience on top of `path_dirlen`; it is not part of the shish core |
| `path_join` (heap string) is used by the c-utils C preprocessor (3 sites) | kept, now `alloc(n + 2)` so `path_canon` can run in place; the lookup copies and `ar-wrap` etc. use `path_cat_sa` |
| the old `path_isname` macro had the opposite polarity (true when a separator was present) and no user | replaced by `path_isname(p, n)`, true when there is no separator |
| `path_relative_to` with one absolute and one relative argument returned garbage | `path_rel` returns `(size_t)-1`; `path_rel_sa` then leaves a copy of `p` in `out` and returns 0 |
| `rootlen("//x")` is 1 in c-utils and 2 in shish | `//` policy still undecided (convention 10). `canon` and `dirlen` follow their library's `rootlen` and agree everywhere else |

**Not verified:** no Windows build (`cfg-mingw*`) of either project, so drive-letter roots and `\\` separators are untested; the
shish binary size was not measured before and after; the shish `ctest` run matches the baseline (14 of 190 failing before and
after, plus the flaky `term-eof.sh`).

**Not in the header after all:** the `PATH_*_MAX` macros, `path_catm`, `path_setsep`, `path_parentlen`, `path_next`,
`path_inside`, `path_dirname` (view form). Nothing asked for them.

### 5.10 Assessment: should qjs-modules `src/path.c` move to this library?

**Verdict: yes, for the lexical core; no for the filesystem predicates and the component accessors.** Do it after shish
batch 2, in one pass, with `tests/unittests/test-path.js` as the safety net.

**What it is.** `src/path.c` (1208 lines) plus `include/path.h` (284) are a C library, not the JS module. The JS binding is
`quickjs-path.c` (781 lines), which does `JS_ToCStringLen()` on every argument and then calls the C functions, so everything
already has a `(p, n)` view in hand; the NUL-terminated variants (`path_extname1`, `path_basename1`, `path_dirlen1`,
`path_relative3(path, relative_to)`) re-scan with `strlen` for no reason. Output goes to a growable `DynBuf`. The same
library is used by `qjsm.c` (module resolution, about 25 calls), `child-process.c` (`path_isname`, `path_search`) and
`compat/glob.c` (`path_gethome2`). There are two copies of the tree (`qjs-modules/` and `quickjs/qjs-modules/`, identical `path.c`).

**Function by function.**

| qjs-modules | new library | note |
|---|---|---|
| `path_normalize3` / `_normalize1/2`, `path_normstr`, `path_normappend` | `path_canon` | Node's `normalizeString` state machine (about 100 lines, allocates a `DynBuf` in `normalize2` and `absolute3`) becomes the 40-line in-place `canon`. Trailing separator and `""` -> `.` are the binding's two lines |
| `path_absolute3/2/1` | `path_abs` | cwd passed in; `path_getcwd1` stays filesystem-side |
| `path_relative3/2/1/5`, `path_common4` | `path_rel`, `path_common` | the binding already makes both sides absolute first, which is the library's precondition |
| `path_basename1/3`, `path_dirlen1/2`, `path_dirname1`, `path_root2` | `path_basename`, `path_dirlen`, `path_rootlen` | **fixes BUGS `path-root-handling`**: `dirname('/x')` is `.`, `resolve('/')` is `<cwd>/\0`, `resolve('/a/..')` is `''`; `lib/module.js` carries its own posix helpers because of it |
| `path_extpos1`, `path_extlen1`, `path_extname1` | `path_extoff` | `extlen` is `n - off`; same dotfile rule |
| `path_append2/3`, `path_dup*` | `path_cat` + the binding's `DynBuf` glue | `js_path_join` must **not** use `path_cat` (Node `join('/a','/b')` is `/a/b`; `cat` lets the absolute right operand win). It keeps its own separator-joining loop and then calls `canon`. `js_path_resolve` is the right-wins case and maps to `cat`/`abs` |
| `path_isname` (macro), `path_isabs`, `path_isrel`, `path_isexplicit`, `path_isimplicit`, `path_isdot*` | `path_isname(p, n)`, `path_isabs`, `path_rootlen` | `isname` polarity matches (true = no separator); explicit/implicit stay in the binding |
| `path_search` (PATH loop in `child-process.c` and the binding) | `PATH_LIST_FOREACH` + `path_cat` | a hand-rolled copy of the loop the shish sites have |
| `path_fnmatch5` | `path_fnmatch` | same algorithm and flag values; delete the duplicate (the flags are even spelled `PATH_FNM_*` in both) |
| `path_getcwd1/0`, `path_gethome*`, `path_readlink2`, `path_realpath3`, `path_resolve3` | stay (filesystem group) | different growable type (`DynBuf` vs `stralloc`); sharing needs `(buf, cap)` signatures with retry, a separate decision |
| `path_exists1`, `path_isdir1`, `path_isfile1`, `path_ischardev1`..`path_issymlink1` | stay | `stat` predicates, not lexical, not in this library |
| `path_components3`, `path_at3/4`, `path_slice4`, `path_length1/2`, `path_right2`, `path_skip*`, `path_separator*`, `path_component*`, `path_isin4`, `path_equal4`, `path_diff4`, `path_compare2` | stay in qjs-modules for now | these are the JS methods that Node's `path` does not have. They need `seplen`/`comlen` and the component iterator, which are tier 2 (private). Promote `seplen`/`comlen`/`rseplen`/`rcomlen` and add `path_next` to the header **only if** the binding is moved over; then `at`/`slice`/`components` are 10 lines each in the binding |

**What it costs.**

- One growable-buffer glue per writer in the binding (`dbuf_realloc(db, need + 1)`, then call), about 12 lines in total, because
  there is one operation per JS call.
- The one-function-per-file layout: `src/path.c` (one file, one `.o`) becomes about 15 files under `src/path/`, or a vendored
  copy of `lib/path/` kept in step with the `cmake/` sync that the qjs-* projects already do. Linking `libpath` from c-utils is
  possible but adds a build-time dependency between two repositories.
- Behaviour differences to check with `test-path.js` before and after: `//` roots, drive letters on Windows builds,
  `..` above a relative start (kept in both), trailing separators (binding), and the NUL byte `resolve('/')` currently produces.

**What it gains.** The root bug above is fixed by construction; about 400 lines of C and the `DynBuf` temporaries in `normalize2`
and `absolute3` disappear (design metric 3, fewer allocations); `qjsm.c`'s module resolver shares the code the shells and tools
use (`path_component1` as `isname` is used five times there); and a fix in one library reaches all three projects.

**What not to do.** Do not move the filesystem predicates or the non-Node methods in the same change, and do not change
`js_path_join` to `path_cat`.

**Order of work.** (1) shish batch 2 and the list iterator, (2) vendor `lib/path/` into qjs-modules, (3) switch `qjsm.c` and
`child-process.c` (C-only, small), (4) switch `quickjs-path.c` function by function, running `test-path.js` after each.

---

## 6. How the clients simplify (before → after)

| client | before | after |
|---|---|---|
| `builtin_dirname` | 3 loops, special-cased `n==0` | `p = path_dirname(path, n, &len); buffer_put(out, p, len);` |
| `builtin_rmdir -p` | `path_lensa(path_dirlenb, sa)` + root special-case | `for(k = path_parentlen(sa->s, sa->len); k; k = path_parentlen(sa->s, k))` |
| `builtin_mkdir -p` | NUL-patch loop, own separator skipping | `path_next` loop (5.3) |
| `builtin_realpath --relative-to` | 20-line `relative_to()` using a private macro | `path_rel(...)` |
| `builtin_cd` CDPATH | manual `str_chr`, `byte_copy`, `'/'`, `PATH_MAX` check | `PATH_LIST_FOREACH` + `path_cat` |
| `exec_path`, `source_search_path` | duplicated PATH splitting, `static` result | `PATH_LIST_FOREACH`, caller buffer |
| 13 × `name[str_chr(name,'/')]` | three spellings | `path_isname(name, str_len(name))` |
| directory walkers (`cp`, `chmod`, `find`, `ls`, `ln`) | 5 × (save/ensure-sep/cat/restore) | in-place `path_cat` (then `dirwalk`) |
| c-utils `genmakefile` | abs → canonical → collapse | `path_abs` |
| c-utils five `path_lookup` | each splits `PATH`, scans for `/` and `\\` | `PATH_LIST_FOREACH` + `path_cat` |
| `qjsm.c:1666-1692` | 7 calls and a `DynBuf` | `d = path_dirname(importer, …); n = path_cat(buf, cap, d, dl, spec, sl); path_canon(buf, cap, buf, n, 0);` |
| `quickjs-path.c` | `DynBuf` per call | measure, allocate once, fill |

Code that gets **smaller as a side effect**: every `_b`/`_s`/`_sa` triple (three entry points per
operation) becomes one function; every `stralloc_nul` after a path call disappears (writers
terminate); `SizePair` is gone; `path_split(strlist)` goes away because `PATH_LIST_FOREACH` needs no list.

---

## 7. Orthogonality check: is everything composable?

| need | built from |
|---|---|
| dirname / basename / extname | `rootlen`, `dirlen`, `basename`, `extoff` |
| "is x inside dir y" | `path_isin` (= `common`) |
| `cd`-style `..` handling, `mkdir -p`, `rmdir -p`, tab completion | `next`, `parentlen`, `dirlen`+`basename` |
| `realpath -m`, `dirs`, `pwd -L` | `abs` / `canon` (pure) |
| `realpath` / `cd -P` | `resolve` |
| `--relative-to`, include-dir relativising | `rel` |
| join list, prefix, append, prepend | `cat` / `catm` (prepend = `cat(prefix, p)`) |
| `PATH`/`CDPATH` search | list iterator + `cat` |
| Node `parse`, `format`, `resolve`, `join` | binding layer on the above (`join` = `cat` after dropping `b`'s root, then `canon(KEEPTRAIL)`) |
| separator conversion for make/cmake output | `setsep` |
| `~user` expansion | `homeof` |
| recursive walk | in-place `cat` + `parentlen` truncation + a future `dirwalk` |
| collation / sorting | `path_cmp` |

Nothing in the surveyed clients needs more than this. Things that looked like gaps and are
**not** path-lib business: quoting (`path_quote_spaces`), existence/type tests, `find` in a directory
(`path_find(dir, name)`), `mmap` helpers.

---

## 8. Renames and removals

### 8.1 Removed / relocated (not path-string operations)

| c-utils / qjs-modules | action |
|---|---|
| `path_exists`, `path_access`, `path_is_directory`, `path_is_root`, `path_isfile1`, `path_isdir1`, `path_ischardev1`, `path_isblockdev1`, `path_isfifo1`, `path_issocket1`, `path_issymlink1`, `path_exists2` | move to an `fs_*` module (`fs_exists`, `fs_isdir`, `fs_type`); 45 callers of `path_exists` stay mechanical |
| `path_find(dir, name, out)` | `dir_find`; it is a directory scan, and the name collides conceptually with PATH lookup |
| `path_quote_spaces[_b/_s]` | `shell`/`quote` module |
| `path_mmap_read` (genmakefile) | `mmap_*` |
| `path_split(strlist)` | removed, use `PATH_LIST_FOREACH` |
| `path_is_root` (stat-based) | removed; `path_parentlen(p,n) == 0 && rootlen > 0` is the lexical test, `stat("/..")` can be inlined where truly needed |

### 8.2 Merged

| old | new |
|---|---|
| `path_canonical`, `path_canonical_sa`, `path_canonicalize(symbolic=1)`, `path_collapse`, `path_collapse_sa`, `path_normalize`, `path_normalize2`, `path_normalize_sa(_b)`, `path_clean_s`, `path_clean_b`, `path_normalize1/2/3` | `path_canon` |
| `path_canonicalize(symbolic=0)`, `path_realpath`, `path_resolve3`, `path_realpath3` | `path_resolve` |
| `path_absolute`, `path_absolute_s`, `path_absolute_sa`, `path_absolute1/2/3` | `path_abs` |
| `path_concat`, `path_concatb`, `path_concat_sa`, `path_append(s)`, `path_prepend(s)`, `path_join`, `path_append2/3` | `path_cat` / `path_catm` |
| `path_relative`, `_b`, `_s`, `_to`, `_to_b`, `_to_s`, `_to_sa`, `path_relative1..5` | `path_rel` |
| `path_dirlen`, `_b`, `_s`, `path_dirlen1/2`, `path_right`, `path_right2`, `path_dirname`, `_b`, `_s`, `_sa`, `path_dirname1`, `path_dirname_alloc` | `path_dirlen`, `path_dirname`, `path_parentlen` |
| `path_basename`, `basename2`, `basepos2`, `path_basename1/3`, `path_baselen`, `path_basepos` | `path_basename` (+ `path_baseoff` macro if needed) |
| `path_rootlen`, `_s`, `_b`, `path_root2`, `path_is_absolute(_s/_b/_sa)`, `path_isabs(b)`, `path_isabsolute1/2`, `path_isrel(b)`, `path_isrelative` | `path_rootlen`, `path_isabs`, `path_isrel` |
| `path_len`, `path_len_s`, `path_lenb`, `path_component1/2/3`, `path_separator1/2`, `path_skip`, `path_skips`, `path_skip1/2/3`, `path_skip_component`, `path_skip_separator`, `path_length1/2`, `path_num`, `path_num_sa` | the four scanners, `path_next`, `path_ncomp` |
| `path_common_prefix`, `path_commonlen`, `path_commonlenb`, `path_common4`, `path_diff4`, `path_equal4`, `path_isin4`, `path_compare2` | `path_common`, `path_cmp`, `path_isin` |
| `path_extname1`, `path_extpos1`, `path_extlen1`, genmakefile `path_extension` | `path_extoff` |
| `path_trim`, `path_trim_b`, `path_trimdotslash1`, `path_skipdotslash2` | `path_rseplen` / `path_seplen` + `path_canon` |
| `SizePair` | removed |

### 8.3 Kept, renamed only

| old | new | note |
|---|---|---|
| `path_fnmatch(pat,pl,str,sl,flags)` | `path_fnmatch` | already `(p,n)`, 46 callers, correct signature |
| `path_getcwd` | `path_getcwd` | stays `stralloc`, legit |
| `path_readlink` | `path_readlink(p,n,sa)` | takes a length |
| `path_gethome(uid)`, `gethome1/2` | `path_homeof(user,…)` | caller buffer; uid variant stays as `path_homeofuid` if `sh_gethome` needs it |
| `path_getsep` | removed | callers (`getcwd`, `collapse`) disappear with `setsep` |
| `PATHSEP_C/S`, `PATHLISTSEP_*`, `PATH_FNM_*`, `MAX_PATH`/`PATH_MAX` | unchanged, single definition in `path.h` | |

---

## 9. The private header

Status 2026-10-14: `lib/path_internal.h` still exists (lower-case, `…b` pairs, `stralloc` helpers). Target:
`lib/path/internal.h`, included as `#include "internal.h"` by every `lib/path/*.c` and by nothing else (the
rule "only `lib/path/*.c` include it" then holds by directory). Phase plan: section 12.

All macros are **view-based** `(p, n)`, upper-case, without the `path_` prefix (they cannot collide with
the public `path_*` namespace, and a reader sees at once that a name is private). Arguments are
evaluated more than once: pass plain lvalues, no side effects (stated in the header).

| macro | meaning | replaces |
|---|---|---|
| `SEP(c)` | `c` is a separator (`/`; `\` too on `WINDOWS`) | `path_issep` |
| `SEPCHARS`, `NSEP` | `"/"` or `"\\/"` and its length | `PATHSEP_CHARS`, `PATHSEP_LENGTH` |
| `DRIVE(p, n)` | 2 if `p` starts with `X:` (`WINDOWS` only), else 0; never reads past `n` | `path_isdriveb`, `path_isdrive` |
| `ISDOT(p, n)`, `ISDOTDOT(p, n)` | the whole view is `.` / `..` | `path_isdotb`, `path_isdotdotb`, `path_isdotdot` |
| `ISDOTSEP(p, n)`, `ISDOTDOTSEP(p, n)` | view starts with `.` / `..` followed by a separator or the end | `path_isdotsepb`, `path_isdotdotsepb` |
| `ISABS(p, n)`, `ISREL(p, n)` | root contains a separator (`C:/x`, `/x`, `//x`) / not | `path_isabsb`, `path_isrelb`, `path_isabs`, `path_isrel` |
| `SEPLEN(p, n)`, `COMLEN(p, n)` | run of separators / of non-separators at `p` | `path_seplen`, `path_comlen` |
| `RSEPLEN(p, n)`, `RCOMLEN(p, n)` | the same, ending at `p + n` | `path_rseplen`, `path_rcomlen` |

Dropped without replacement: the NUL-terminated forms (`path_isabs(p)`, `path_isdrive(p)`, `path_isdotdot(p)`;
`path_realpath` was the only caller and now computes `str_len` once), `path_len`, `path_plen`, `path_lensa`, `path_plensa`
(function-applying wrappers; the `stralloc` callers pass `sa->s, sa->len`), and the `PATHSEP_*` spellings.

**Decision (supersedes the 2026-10 draft of this section): the four scanners stay private.** The draft said clients
"already need them". The survey for section 12 shows none does: outside `lib/path/` no file in shish calls
`path_seplen`/`path_comlen`/`path_rseplen`/`path_rcomlen` (the one outside use, `builtin_realpath.c`'s
`path_component`, went with `path_rel`). What clients still need is the *iterators* built on them
(`path_next`, `path_parentlen`) and the classifiers (`path_isabs`, `path_isrel`); those are lifted into
`path.h` as functions in phases 2 and 3 and the scanners are their private implementation.

Files in `lib/path/` that are not path-string functions (`getcwd`, `readlink`, `realpath`, `canonicalize`,
`gethome`, `fnmatch`) include the same header: they use `SEP`, `SEPLEN`… for the path parts and bring their
own system headers.

---

## 10. Migration and tests

### 10.1 Sequence

1. Land the **defect fixes** (D1–D4, D6, D7, D8) on the current API, with tests (below).
2. Introduce the new API in `shish/lib/path/` (the authoritative copy). Old names that other
   projects still call stay for one step as thin deprecated wrappers.
3. Convert shish clients (the table in 3.1 is the work list); delete shish's now unused functions.
4. Port c-utils and qjs-modules to the shish library, in this order: c-utils `path_lookup` x5 (smallest,
   most duplicated), then `genmakefile`; then rewrite the `DynBuf`-based `src/path.c` and
   `quickjs-path.c` on the new writers, so the binding needs no `dbuf` and `qjsm.c`'s resolver becomes
   the 3-call form in section 6.
5. Delete c-utils' `lib/path*` and qjs-modules' `src/path.c` / `include/path.h` once `grep` shows no
   remaining users of the old names in the three trees.

Single source of truth: `shish/lib/path`. c-utils is the largest holder of tech debt here (six
normalisers, `_b/_s/_sa` triples, u8-per-byte scanners, filesystem predicates mixed into the path
namespace) and is a consumer, not a source: it takes the new library and drops its own copy. How
c-utils and qjs-modules consume it (vendored copy, git submodule, or a shared `lib/` checkout) is
open decision 7. `SizePair` and the triple macro definitions go away with the old copies.

### 10.2 Tests (the differential approach already worked once)

* `dirname`/`basename` against coreutils on the 34-path table plus a random fuzz (done once for
  `canon`, reuse the harness for the rest).
* `canon` against `realpath -m -s`, with the `//` policy (D4) as the only documented deviation.
* `rel` against `realpath --relative-to -m` and `python -c os.path.relpath` on canonical inputs.
* `cat` / `canon` / `extoff` against Node `path.posix` (a `@portable` test in qjs-modules, so it also
  runs under node/bun/deno and keeps the binding honest).
* `common` / `cmp` / `isin`: property tests (`common(a,a)==len`, symmetric, `cmp==0 ⇔ canon equal`).
* Every writer: `cap` sweep (0,1,need-1,need,need+1) to check the snprintf contract and the `NUL`.
* Aliasing: `canon` in place, `cat(out, …, out, …)`.
* Windows (`WINDOWS` macro) vectors compiled on the Linux host via `-DWINDOWS=1` for the pure functions.

---

## 11. Open decisions (need the author)

1. **Shipping `PATH_POSIX_ONLY`** for URL-like callers (`ftpcopy`, `uri`): compile-time macro (proposed)
   or a `flags` bit on every function? The macro keeps signatures clean.
2. **`//` root**: collapse on POSIX (proposed, matches `realpath` and fixes D4) or preserve exactly two
   leading separators everywhere (Node and POSIX-allowed "implementation-defined" both fine).
3. **Where `fs_*` lives** (exists/isdir/type predicates leaving the path lib): new `lib/fs/` or an existing
   module in c-utils?
4. **`path_cat` list form**: variadic `path_catm` (proposed, matches `stralloc_catm`) or an array of
   `(p,n)` for callers that build lists dynamically (the module-resolver case)?
5. **`dirwalk`**: confirm it is a separate module and take the `(buf, len, nameoff)` callback contract of
   5.6, since it dictates how `path_cat` aliasing must behave.
6. **Breaking `path_basename(p, NULL)`**: the 12 shish call sites change to the `(p, n, &len)` form. OK?
7. **How c-utils and qjs-modules consume `shish/lib/path`**: vendored copy, submodule, or a shared checkout?

---

## 12. Consolidation plan: `lib/path/internal.h` and the lifted public API

End state:

* `lib/path/internal.h` exists, holds exactly the macros of section 9, and is included by every `lib/path/*.c`
  as `#include "internal.h"`; `lib/path_internal.h` is gone.
* No `lib/path/*.c` spells a lower-case helper macro; every one uses the upper-case set.
* `lib/path.h` declares only the public functions of section 5.9 (tier 1) plus the iterators and classifiers
  lifted below; no macro of `path.h` reads `rootlen` or splits strings.
* Every client in shish calls the public functions only; nothing outside `lib/path/` can name a private macro.

Two kinds of step, never mixed in one phase:

* **refactor phases** (1, 4): behaviour must not change. Gate: the machine code of every `lib/path/*.o` is
  identical before and after (a snapshot script: `objdump -d -r` of each object with addresses
  stripped, hashed), plus a clean build and the `path_test` unit test.
* **lifting phases** (2, 3, 5): a function enters `path.h` with tests; then its clients migrate, one commit
  per client group; then the old spelling is deleted. Gate: `path_test`, the shish `ctest` compared with the
  baseline (14 of 190 failing, plus the flaky `term-eof.sh`), `size` of `shish` not larger.

Baseline taken 2026-10-14 before phase 1: `cmake --build build/x86_64-linux-gnu` clean, `path_test` all OK,
36 objects hashed in `snap.sh` output (`base.txt`).

### Phase 0: inventory (done)

Who uses what, outside `lib/path/` (grep, shish tree incl. `tests/`):

| private/legacy name | external users | fate |
|---|---|---|
| `path_issep`, `PATHSEP_CHARS`, `PATHSEP_LENGTH`, `path_isdrive[b]`, `path_isdot*`, `path_isabs[b]`, `path_isrel[b]`, `path_seplen`, `path_comlen`, `path_rseplen`, `path_rcomlen`, `path_len`, `path_plen`, `path_plensa` | none | private, renamed (phase 1) or deleted |
| `path_is_absolute` (`path.h` macro, `rootlen & 1`, defect D1) | `builtin_rmdir.c` | replaced by `path_isabs(p, n)` (phase 2) |
| `path_lensa` | `builtin_rmdir.c` | replaced by `path_parentlen` (phase 3) |
| `path_len_s` (`path.h` macro over `path_lenb`) | `builtin_mkdir.c` | replaced by `path_next` (phase 3) |
| `path_lenb` | `tests/lib/path_test.c` | deleted with its test rewritten (phase 3) |
| `path_getsep` | none outside (`path_getcwd.c` only) | made private/inlined, then deleted (phase 4) |
| `path_rootlen_s` | none | deleted (phase 4) |
| `path_basename_s` | 5 clients | kept (a macro over the public `path_basename`) |

### Phase 1: `internal.h` (private side, zero behaviour change): DONE 2026-10-14

Result: 21 files converted, `lib/path_internal.h` deleted. All object hashes identical except `path_realpath`
(the NUL-string `path_isabs`/`path_isdrive` became `ISABS`/`DRIVE` on `str_len(path)`, one extra length scan on a cold path).
`builtin_realpath.c` included the private header without needing it (now `path.h`); `builtin_rmdir.c` used
the private `path_lensa` (now `path_dirlen(sa->s, sa->len)`, replaced for good in phase 3). No `ADV` macro: the
"15 repeated pairs" lived in the deleted `commonlenb`; nothing in the current files advances a view.

1a. Create `lib/path/internal.h` with the section-9 macros **next to** the old ones: `lib/path_internal.h`
    becomes a three-line shim (`#include "path/internal.h"` plus the legacy lower-case aliases defined in
    terms of the new macros). Everything still compiles; snapshot must be unchanged.
1b. Convert the `lib/path/*.c` files, simplest first, snapshot after each batch:
    `path_isname`, `path_rootlen`, `path_dirlen`, `path_basename`, `path_extoff` (scanners and predicates only);
    `path_cat`, `path_cat_sa`, `path_canon`, `path_abs`, `path_abs_sa`, `path_common`, `path_rel`, `path_rel_sa`
    ; `path_canonicalize`, `path_realpath` (`ISABS`/`DRIVE` on a view, `str_len` at the entry);
    `path_getcwd`, `path_getsep`, `path_readlink`, `path_gethome`, `path_fnmatch`, `path_lenb`, `path_list*`, `path_search`
    (`path_list*` and `path_search` use only the public header and do not include `internal.h`).
1c. Delete the shim `lib/path_internal.h` and the legacy aliases; `grep -rn 'path_internal'` must be empty.
    Gate: object hashes identical for all objects whose source still exists.

### Phase 2: lift the classifiers

* Add `size_t`-free functions in their own files: `int path_isabs(const char*, size_t)` and
  `int path_isrel(const char*, size_t)` (one-liners over `ISABS`/`ISREL`). They fix D1: `//`, `//a`,
  `C:/` are absolute regardless of the parity of `rootlen`.
* Tests: the vectors of D1 plus `""`, `"a"`, `"/"`, `"C:"` (compiled `-DWINDOWS=1` for the drive cases).
* Migrate `builtin_rmdir.c` (`path_is_absolute`), then delete the `path_is_absolute` macro from `path.h`.

### Phase 3: lift the iterators

* `size_t path_next(const char* p, size_t n, size_t pos)`: forward step, end of the next component
  prefix (root first), 0 at the end. `size_t path_parentlen(const char* p, size_t n)`: strictly shrinking
  backward step, 0 at the end (section 5.3, fixes D13). One file each, built on `SEPLEN`/`COMLEN`/`RSEPLEN`/`RCOMLEN`.
* Tests: `"a/b/c"`, `"/a"`, `"/"`, `"a//b/"`, `"//a"`; the loops of 5.3 as the tests.
* Migrate: `builtin_mkdir.c` (the NUL-patching loop and `path_len_s`) to `path_next`; `builtin_rmdir.c`
  (`path_lensa(path_dirlenb, …)` and its root special case) to `path_parentlen`.
* Delete `path_lenb`, `path_len_s`, `path_lensa`, `path_plensa`; rewrite the `path_lenb` test as `path_next` tests.

### Phase 4: private clean-up

* `path_getsep` is used by `path_getcwd.c` only; inline the loop (`for(...) if(SEP(*p))`) and delete the file
  and the declaration. `path_rootlen_s` has no user: delete.
* Re-run the snapshot on the objects that were not touched (must be identical) and `size` on `shish`.

### Phase 5: `canonicalize`/`realpath` consolidation (optional, defect D11)

Replace `path_canonicalize` + `path_realpath` with `path_resolve` of 5.8: for `symbolic != 0` call `path_abs`
(no `stralloc` at all), otherwise keep the `rest/link/tmp` triple. Clients: `cd`, `realpath`, `readlink -f`, `pwd`.
Uses the same macros; separate because it changes behaviour (the `//tmp` root policy of D4 must be decided,
open decision 2).

### Phase 6: other projects consume the same library (decision 7, not started)

* **c-utils**: its `lib/path_internal.h` has different macros (`path_issep`, `PATHSEP_S_MIXED`, NUL forms) and
  49 `lib/path/*.c`. Work list: vendor the shish files (identical `path_cat/canon/common/rel/abs/extoff`
  already), drop the `_b/_s/_sa` families and filesystem predicates (section 8.1/8.2), migrate the clients of
  section 3.2. The macros of `internal.h` make the vendored files compile unchanged.
* **qjs-modules**: `src/path-core.c` carries its own scanner copies and `byte_copy`/`byte_diff` shims; once
  `internal.h` is stable it can be generated from the same files (`PATH_NO_LIBOWFAT` shim block in the header),
  then `quickjs-path.c` moves over function by function (5.10), `test-path.js` as the net.

### Order and checkpoints

0 → 1a → 1b → 1c (commit: "path: internal.h with upper-case macros") → 2 (commit) → 3 (commit per client) →
4 (commit) → 5 (own review) → 6 (per project). After every commit: `snap.sh` against the previous snapshot,
`path_test`, build of all targets; after phases 2, 3, 5 also the shish `ctest` and `size`.
