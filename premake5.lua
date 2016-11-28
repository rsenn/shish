-- premake5.lua
workspace "shish"
   configurations { "Debug", "Release" }

project "shish"
   kind "ConsoleApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files { "*/*/*.c", "*/*.h" }
   includedirs { "lib", "src" }
   defines { 'PACKAGE_NAME="shish"', 'PACKAGE_VERSION="0.8"' }

   filter "configurations:Debug"
      defines { "DEBUG" }
      flags { "Symbols" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
