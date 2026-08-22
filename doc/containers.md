# shish in containers

A container image usually carries a shell it barely uses: `/bin/sh`, plus
the C library it links against, plus the coreutils binaries the entrypoint
script calls. That is a few megabytes and a few hundred files of attack
surface for the sake of `mkdir -p`, `cat` and a `for` loop.

shish collapses that to one file.

## A shell layer that is one file

```sh
. ./cfg.sh
cfg-musl -DLINK_STATIC=ON -DENABLE_ALL_BUILTINS=ON
cmake --build build/x86_64-linux-musl -j
strip build/x86_64-linux-musl/shish
```

```dockerfile
FROM scratch
COPY shish /bin/sh
COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]
```

The entrypoint can do real work with nothing else in the image:

```sh
#!/bin/sh
PATH=                       # nothing to find, and nothing to hijack
mkdir -p /run/app
[ -f /config/app.conf ] || { echo "no config" >&2; exit 1; }
cat /config/app.conf > /run/app/app.conf
exec /app/server
```

`mkdir`, `cat`, `[` and `echo` are all inside the binary — see
[Builtins](builtins.md) for the full list and how to select it.

## Why this is worth doing

- **Fewer files to audit and to patch.** A distroless image with a shell
  in it usually means a libc, a dynamic loader, and every coreutils
  binary an entrypoint might touch. This is one static executable.
- **`PATH=` is a real defence.** If a script never needs an external
  program, an empty `PATH` costs nothing and removes a whole class of
  binary-planting and PATH-injection tricks.
- **Smaller layers, faster pulls.** 185 KB dynamic, ~1.15 MB static with
  every builtin compiled in. bash is 1.4 MB before its libc, busybox is
  2.0 MB.
- **Reproducible.** No startup file is read unless you point `$ENV` at
  one, and `-p` (privileged mode) makes the shell ignore `$ENV`
  altogether.

## Where busybox is still the answer

busybox brings ~400 applets: `sed`, `awk`, `tar`, `wget`, `ps`, an init.
shish brings a shell and about a dozen file utilities. If your entrypoint
pipes through `awk`, use busybox — or use both, and let shish be the
`/bin/sh` that busybox's `ash` would otherwise be.

## Caveat

shish is alpha (see [Conformance](conformance.md)). It runs its own test
suite and 5270 cases of yash's POSIX suite, not your distribution's
`/etc/init.d`. Test your entrypoint against it before you ship it —
`shish -n script.sh` parses without executing, which is a cheap first
check.
