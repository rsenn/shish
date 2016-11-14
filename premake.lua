#!/usr/bin/env lua

package.name     = "shish"
package.language = "c"
package.kind     = "exe"
package.target   = "shish"

-- Include paths
  package.includepaths = { "lib", "src" }
  
-- Defines
  package.defines = { 
    "NDEBUG=1",
    "PACKAGE_NAME=\\\"shish\\\"",
    "PACKAGE_VERSION=\\\"0.8\\\""
  }
  
-- Files
  package.files =
  {
		  matchfiles("lib/*.h", "src/*.h", "lib/buffer/*.c", "lib/byte/*.c", "lib/fmt/*.c", "lib/mmap/*.c", "lib/open/*.c", "lib/scan/*.c", "lib/shell/*.c", "lib/str/*.c", "lib/stralloc/*.c", "lib/uint32/*.c", "src/builtin/*.c", "src/debug/*.c", "src/eval/*.c", "src/exec/*.c", "src/expand/*.c", "src/fd/*.c", "src/fdstack/*.c", "src/fdtable/*.c", "src/history/*.c", "src/job/*.c", "src/parse/*.c", "src/prompt/*.c", "src/redir/*.c", "src/sh/*.c", "src/sig/*.c", "src/source/*.c", "src/term/*.c", "src/tree/*.c", "src/var/*.c", "src/vartab/*.c")
  }
  
  