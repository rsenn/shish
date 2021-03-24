solution "shish"
    configurations { "Debug", "Release" }

project "shish"
    targetname "shish"
    kind "ConsoleApp"
    language "C"
    files { "lib/**.h", "lib/**.c",
            "src/**.h", "src/**.c" }

    excludes { "src/sh/sh_fmt.c" }

    configuration "Debug"
      targetdir   "bin/debug"
      defines     { "_DEBUG", "DEBUG_OUTPUT" }
      flags       { "Symbols" }
      links       { "m" }

    configuration "Release"
      targetdir   "bin/release"
      defines     { "NDEBUG" }
      flags       { "OptimizeSize" }
      links       { "m" }
