#include "../buffer.h"
#include "../scan.h"
#include "../stralloc.h"
#include "../windoze.h"
#include "../str.h"

#if WINDOWS_NATIVE
#include <windows.h>
#define BUFSIZE 512
#else
#include <limits.h>
#include <unistd.h>
#endif
#include <fcntl.h>

int
mmap_filename(void* map, stralloc* sa) {
#if WINDOWS_NATIVE
  typedef DWORD(WINAPI get_mmaped_filename_fn)(HANDLE, LPVOID, LPSTR, DWORD);
  static get_mmaped_filename_fn* get_mmaped_filename;

  if(get_mmaped_filename == 0) {
    HINSTANCE psapi = LoadLibraryA("psapi.dll");
    if((get_mmaped_filename = (get_mmaped_filename_fn*)GetProcAddress(psapi, "GetMappedFileNameA")) == 0)
      return 0;
  }

  stralloc_ready(sa, MAX_PATH + 1);
  if((sa->len = (size_t)(*get_mmaped_filename)(GetCurrentProcess(), map, sa->s, sa->a))) {

    /* Translate path with device name to drive letters. */
    char szTemp[BUFSIZE];
    szTemp[0] = '\0';

    if(GetLogicalDriveStringsA(BUFSIZE - 1, szTemp)) {
      char szName[MAX_PATH];
      char szDrive[3] = " :";
      BOOL bFound = FALSE;
      char* p = szTemp;

      do {
        /* Copy the drive letter to the template string */
        *szDrive = *p;

        /* Look up each device name */
        if(QueryDosDevice(szDrive, szName, MAX_PATH)) {
          size_t uNameLen = str_len(szName);

          if(uNameLen < MAX_PATH) {
            bFound = strnicmp(sa->s, szName, uNameLen) == 0 && *(sa->s + uNameLen) == '\\';

            if(bFound) {
              /* Reconstruct sa->s using szTempFile
                 Replace device path with DOS path */
              stralloc_remove(sa, 0, uNameLen);
              stralloc_prepends(sa, szDrive);
              break;
            }
          }
        }

        /* Go to the next NULL character. */
        while(*p++)
          ;
      } while(!bFound && *p); /* end of string */
    }
  }
  return sa->len > 0;
#else
  char buf[1024];
  buffer b = BUFFER_INIT(read, open("/proc/self/maps", O_RDONLY), buf, sizeof(buf));
  char line[73 + PATH_MAX + 1];
  ssize_t n;
  int ret = 0;

  while((n = buffer_getline(&b, line, sizeof(line))) > 0) {
    char* p = line;
    uint64 start, end;

    p += scan_xlonglong(p, &start);
    if(*p == '-') {
      char* e = line + n - 1;
      int i = 4;

      ++p;
      p += scan_xlonglong(p, &end);

      while(i--) {
        p += scan_whitenskip(p, e - p);
        p += scan_nonwhitenskip(p, e - p);
      }
      p += scan_whitenskip(p, e - p);

      if((uint64)map >= start && (uint64)map < end) {
        stralloc_copyb(sa, p, e - p);
        ret = 1;
        break;
      }
    }
  }

  buffer_close(&b);
  return ret;
#endif
}
