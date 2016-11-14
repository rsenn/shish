#!/usr/bin/env lua

-- A solution contains projects, and defines the available configurations
solution "shish"
   configurations { "Debug", "Release" }

   -- A project defines one build target
   project "shish"
      kind "ConsoleApp"
      language "C"
      files { "*/*/*.c" }

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
