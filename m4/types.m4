
# check for socklen_t type
# ------------------------------------------------------------------
AC_DEFUN([AC_TYPE_SOCKLEN], 
[AC_CHECK_TYPE([socklen_t], [], 
[AC_DEFINE_UNQUOTED([socklen_t], [unsigned int],
[Define to 'unsigned int' if <sys/types.h> does not define.])],
[#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif])])
