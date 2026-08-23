#include "../windoze.h"

#if WINDOWS_NATIVE || defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__wasi__)

#include "../unix.h"
#include "../byte.h"
#include "../str.h"
#include "../fmt.h"

#if WINDOWS_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* undocumented but stable since XP; queried via GetProcAddress so we
 * don't need to link against ntdll.lib for a handful of fields. */
typedef struct {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  WCHAR szCSDVersion[128];
} RTL_OSVERSIONINFOW_;

typedef LONG(WINAPI* RtlGetVersionFn)(RTL_OSVERSIONINFOW_*);

static const char*
machine_name(void) {
  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);

  switch(si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
    case PROCESSOR_ARCHITECTURE_ARM64: return "aarch64";
    case PROCESSOR_ARCHITECTURE_ARM: return "arm";
    case PROCESSOR_ARCHITECTURE_INTEL: return "i686";
    default: return "unknown";
  }
}

int
uname(struct utsname* buf) {
  RTL_OSVERSIONINFOW_ vi;
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  RtlGetVersionFn RtlGetVersion = ntdll ? (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion") : 0;
  DWORD len = sizeof(buf->nodename);
  char* p;

  byte_zero(&vi, sizeof(vi));
  vi.dwOSVersionInfoSize = sizeof(vi);

  if(RtlGetVersion)
    RtlGetVersion(&vi);

  str_copy(buf->sysname, "Windows_NT");

  if(!GetComputerNameA(buf->nodename, &len))
    str_copy(buf->nodename, "localhost");

  p = buf->release;
  p += fmt_ulong(p, vi.dwMajorVersion);
  *p++ = '.';
  p += fmt_ulong(p, vi.dwMinorVersion);
  *p = 0;

  buf->version[fmt_ulong(buf->version, vi.dwBuildNumber)] = 0;

  str_copy(buf->machine, machine_name());

  return 0;
}
#else /* Emscripten / WASI / bare wasm: no host to introspect */
int
uname(struct utsname* buf) {
  str_copy(buf->sysname,
#if defined(__EMSCRIPTEN__)
           "Emscripten"
#elif defined(__wasi__)
           "WASI"
#else
           "WASM"
#endif
  );
  str_copy(buf->nodename, "localhost");
  str_copy(buf->release, "0");
  str_copy(buf->version, "0");
  str_copy(buf->machine, sizeof(void*) == 8 ? "wasm64" : "wasm32");

  return 0;
}
#endif

#endif
