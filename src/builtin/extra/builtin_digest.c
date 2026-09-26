#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/open.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/* third_party/Hash-Algorithms (see cmake/Digest.cmake). Its sha384,
 * sha512-224 and sha512-256 headers are empty, so declare those here. */
#include "../../../third_party/Hash-Algorithms/md5.h"
#include "../../../third_party/Hash-Algorithms/sha1.h"
#include "../../../third_party/Hash-Algorithms/sha224.h"
#include "../../../third_party/Hash-Algorithms/sha256.h"
#include "../../../third_party/Hash-Algorithms/sha512.h"

void sha384_digest(uint8_t* message, size_t message_len, uint64_t digest[], bool debug);
void sha512_224_digest(uint8_t* message, size_t message_len, uint64_t digest[], bool debug);
void sha512_256_digest(uint8_t* message, size_t message_len, uint64_t digest[], bool debug);

const char help_digest[] =
    "    Print the message digest of each file.\n"
    "\n"
    "    The algorithm is chosen by the command name: md5sum, sha1sum,\n"
    "    sha224sum, sha256sum, sha384sum, sha512sum, sha512-224sum or\n"
    "    sha512-256sum. Output is \"<hex digest>  <file>\", like coreutils.\n"
    "\n"
    "    file            file to hash; '-' or omitted means stdin\n";

/* one algorithm: command name, and a function that hashes a whole message
 * into 'out' as big-endian bytes and returns the digest size in bytes.
 * ----------------------------------------------------------------------- */
struct digest_algo {
  const char* name;
  size_t (*hash)(uint8_t* msg, size_t len, uint8_t* out);
};

/* the library's state words -> the first 'n' digest bytes, big-endian
 *
 *   const void*  words  uint32_t or uint64_t array
 *   size_t       w      word size in bytes (4 or 8)
 * ----------------------------------------------------------------------- */
static void
digest_bytes(uint8_t* out, size_t n, const void* words, size_t w) {
  size_t i;

  for(i = 0; i < n; i++) {
    uint64_t v = w == 4 ? ((const uint32_t*)words)[i / w] : ((const uint64_t*)words)[i / w];

    out[i] = v >> (8 * (w - 1 - i % w));
  }
}

/* wraps <fn>_digest(), whose state words are 'word_t', for 'n' digest bytes
 * ----------------------------------------------------------------------- */
#define DIGEST_HASH(fn, word_t, n) \
  static size_t digest_##fn(uint8_t* msg, size_t len, uint8_t* out) { \
    word_t w[8]; \
    fn##_digest(msg, len, w, false); \
    digest_bytes(out, n, w, sizeof(word_t)); \
    return n; \
  }

DIGEST_HASH(md5, uint32_t, 16)
DIGEST_HASH(sha1, uint32_t, 20)
DIGEST_HASH(sha224, uint32_t, 28)
DIGEST_HASH(sha256, uint32_t, 32)
DIGEST_HASH(sha384, uint64_t, 48)
DIGEST_HASH(sha512_224, uint64_t, 28)
DIGEST_HASH(sha512_256, uint64_t, 32)

/* sha512 has its own object API instead of a sha512_digest() function
 * ----------------------------------------------------------------------- */
static size_t
digest_sha512(uint8_t* msg, size_t len, uint8_t* out) {
  sha512 h = SHA512();

  h.digest(&h, msg, len);
  digest_bytes(out, 64, h.digests, 8);
  return 64;
}

static const struct digest_algo digest_algos[] = {
    {"md5sum", digest_md5},
    {"sha1sum", digest_sha1},
    {"sha224sum", digest_sha224},
    {"sha256sum", digest_sha256},
    {"sha384sum", digest_sha384},
    {"sha512sum", digest_sha512},
    {"sha512-224sum", digest_sha512_224},
    {"sha512-256sum", digest_sha512_256},
    {NULL, NULL},
};

/* reads all of 'path' ("-" = stdin) into a malloc'd buffer; the library
 * hashes a whole message at once, so there is no streaming.
 * Returns 0 on success, -1 on open/read failure.
 *
 *   const char*  path  file to read
 *   uint8_t**    data  receives the buffer (caller frees; never NULL on success)
 *   size_t*      len   receives the byte count
 * ----------------------------------------------------------------------- */
static int
digest_read(const char* path, uint8_t** data, size_t* len) {
  buffer inb;
  buffer* in;
  char rbuf[4096];
  uint8_t* x = NULL;
  size_t n = 0, a = 0;
  ssize_t r;
  int rfd = -1;

  if(!str_diff(path, "-")) {
    in = fd_in->r;
  } else {
    if((rfd = open_read(path)) == -1)
      return -1;

    in = &inb;
    buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
  }

  while((r = buffer_feed(in)) > 0) {
    if(n + r > a) {
      a = (n + r) * 2;
      x = alloc_re(x, a);
    }

    byte_copy(x + n, r, buffer_PEEK(in));
    buffer_SEEK(in, r);
    n += r;
  }

  if(rfd != -1)
    close(rfd);

  if(r < 0) {
    alloc_free(x);
    return -1;
  }

  /* empty input: hand the library a valid pointer */
  if(!x)
    x = alloc(1);

  *data = x;
  *len = n;
  return 0;
}

int
builtin_digest(int argc, char* argv[]) {
  static const char hex[] = "0123456789abcdef";
  const struct digest_algo* algo;
  int i, nfiles, ret = 0;

  for(algo = digest_algos; algo->name; algo++)
    if(!str_diff(argv[0], algo->name))
      break;

  if(!algo->name)
    return builtin_error(argv, "unknown digest algorithm");

  /* no options: any that shell_getopt() returns is invalid */
  if(shell_getopt(argc, argv, "") > 0) {
    builtin_invopt(argv);
    return 1;
  }

  nfiles = argc - shell_optind;

  for(i = 0; i < (nfiles ? nfiles : 1); i++) {
    const char* path = nfiles ? argv[shell_optind + i] : "-";
    uint8_t *data, sum[64];
    size_t len, size, j;

    if(digest_read(path, &data, &len) == -1) {
      builtin_error(argv, (char*)path);
      ret = 1;
      continue;
    }

    size = algo->hash(data, len, sum);
    alloc_free(data);

    for(j = 0; j < size; j++) {
      buffer_putc(fd_out->w, hex[sum[j] >> 4]);
      buffer_putc(fd_out->w, hex[sum[j] & 15]);
    }

    buffer_puts(fd_out->w, "  ");
    buffer_puts(fd_out->w, path);
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}
