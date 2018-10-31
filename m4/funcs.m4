# $Id: funcs.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Macros checking for system depedencies.
# Do so without preprocessor tests, because the preprocessor is almost
# never invoked separately
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for termios
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_TERMIOS],
[AC_SYS_POSIX_TERMIOS
if test "$ac_cv_sys_posix_termios" = "yes"; then
  AC_DEFINE_UNQUOTED([HAVE_TERMIOS], 1, 
  [Define this if you have the POSIX termios library])
fi
])

# check for if_indextoname
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_N2I],
[AC_MSG_CHECKING([for if_indextoname])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main() {
  static char ifname[IFNAMSIZ];
  char *tmp=if_indextoname(0,ifname);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_N2I], 1,
[Define this if you have the if_indextoname() call])], [AC_MSG_RESULT([no])])])

# check for IPv6 scope ids
# ------------------------------------------------------------------
AC_DEFUN([AC_SCOPE_ID],
[AC_MSG_CHECKING([for ipv6 scope ids])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
  sa.sin6_scope_id = 23;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SCOPE_ID], 1,
[Define this if your libc supports ipv6 scope ids])], [AC_MSG_RESULT([no])])])


# check for IPv6 support
# ------------------------------------------------------------------
AC_DEFUN([AC_IPV6],
[AC_MSG_CHECKING([for ipv6 support])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_IPV6], 1,
[Define this if your libc supports ipv6])
AC_DEFINE_UNQUOTED([LIBC_HAS_IP6], 1,
[Define this if your libc supports ipv6])], [AC_MSG_RESULT([no])])])


# check for alloca() function and header
# ------------------------------------------------------------------
AN_HEADER([alloca.h], [AC_FUNC_ALLOCA])
AC_DEFUN([AC_FUNC_ALLOCA],
[AC_MSG_CHECKING([for alloca])
AC_COMPILE_IFELSE([
#include <stdlib.h>
#include <alloca.h>
dnl
main() {
  char* c=alloca(23);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_ALLOCA], 1,
[Define this if your compiler supports alloca()])], [AC_MSG_RESULT([no])])])


# check for epoll() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_EPOLL],
[AC_MSG_CHECKING([for epoll])
AC_COMPILE_IFELSE([
  int efd=epoll_create(10);
  struct epoll_event x;
  if (efd==-1) return 111;
  x.events=EPOLLIN;
  x.data.fd=0;
  if (epoll_ctl(efd,EPOLL_CTL_ADD,0 /* fd */,&x)==-1) return 111;
  {
    int i,n;
    struct epoll_event y[100];
    if ((n=epoll_wait(efd,y,100,1000))==-1) return 111;
    if (n>0)
      printf("event %d on fd #%d\n",y[0].events,y[0].data.fd);
  }
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_EPOLL], 1,
[Define this if your OS supports epoll()])], [AC_MSG_RESULT([no])])])

# check for select() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SELECT],
[AC_MSG_CHECKING([for select])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/time.h>

#include <string.h>     /* BSD braindeadness */

#include <sys/select.h> /* SVR4 silliness */

int main()
{
  fd_set x;
  int ret;
  struct timeval tv;
  tv.tv_sec = 2; tv.tv_usec = 0;
  FD_ZERO(&x); FD_SET(22, &x);
  ret = select(23, &x, &x, NULL, &tv);
  if(FD_ISSET(22, &x)) FD_CLR(22, &x);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SELECT], 1,
[Define this if your OS supports select()])], [$2
AC_MSG_RESULT([no])])])

# check for poll() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_POLL],
[AC_MSG_CHECKING([for poll])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>

int main()
{
  struct pollfd x;

  x.fd = open("trypoll.c",O_RDONLY);
  if (x.fd == -1) _exit(111);
  x.events = POLLIN;
  if (poll(&x,1,10) == -1) _exit(1);
  if (x.revents != POLLIN) _exit(1);

  /* XXX: try to detect and avoid poll() imitation libraries */

  _exit(0);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_POLL], 1,
[Define this if your OS supports poll()])], [$2
AC_MSG_RESULT([no])])])

# check for kqueue functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_KQUEUE],
[AC_MSG_CHECKING([for kqueue])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main() {
  int kq=kqueue();
  struct kevent kev;
  struct timespec ts;
  if (kq==-1) return 111;
  EV_SET(&kev, 0 /* fd */, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, 0);
  ts.tv_sec=0; ts.tv_nsec=0;
  if (kevent(kq,&kev,1,0,0,&ts)==-1) return 111;

  {
    struct kevent events[100];
    int i,n;
    ts.tv_sec=1; ts.tv_nsec=0;
    switch (n=kevent(kq,0,0,events,100,&ts)) {
    case -1: return 111;
    case 0: puts("no data on fd #0"); break;
    }
    for (i=0; i<n; ++i) {
      printf("ident %d, filter %d, flags %d, fflags %d, data %d\n",
             events[i].ident,events[i].filter,events[i].flags,
             events[i].fflags,events[i].data);
    }
  }
  return 0;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_KQUEUE], 1,
[Define this if your OS supports kqueues])], [AC_MSG_RESULT([no])])])

# check for sigio functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SIGIO],
[AC_MSG_CHECKING([for sigio])
AC_COMPILE_IFELSE([
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/poll.h>
#include <signal.h>
#include <fcntl.h>

int main() {
  int signum=SIGRTMIN+1;
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss,signum);
  sigaddset(&ss,SIGIO);
  sigprocmask(SIG_BLOCK,&ss,0);

  fcntl(0 /* fd */,F_SETOWN,getpid());
  fcntl(0 /* fd */,F_SETSIG,signum);
#if defined(O_ONESIGFD) && defined(F_SETAUXFL)
  fcntl(0 /* fd */, F_SETAUXFL, O_ONESIGFD);
#endif
  fcntl(0 /* fd */,F_SETFL,fcntl(0 /* fd */,F_GETFL)|O_NONBLOCK|O_ASYNC);

  {
    siginfo_t info;
    struct timespec timeout;
    int r;
    timeout.tv_sec=1; timeout.tv_nsec=0;
    switch ((r=sigtimedwait(&ss,&info,&timeout))) {
    case SIGIO:
      /* signal queue overflow */
      signal(signum,SIG_DFL);
      /* do poll */
      break;
    default:
      if (r==signum) {
  printf("event %c%c on fd #%d\n",info.si_band&POLLIN?'r':'-',info.si_band&POLLOUT?'w':'-',info.si_fd);
      }
    }
  }
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SIGIO], 1,
[Define this if your OS supports I/O signales])], [AC_MSG_RESULT([no])])])

# check for /dev/poll i/o multiplexing 
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_DEVPOLL],
[AC_MSG_CHECKING([for /dev/poll])
AC_COMPILE_IFELSE([
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/devpoll.h>

main() {
  int fd=open("/dev/poll",O_RDWR);
  struct pollfd p[100];
  int i,r;
  dvpoll_t timeout;
  p[0].fd=0;
  p[0].events=POLLIN;
  write(fd,p,sizeof(struct pollfd));
  timeout.dp_timeout=100;       /* milliseconds? */
  timeout.dp_nfds=1;
  timeout.dp_fds=p;
  r=ioctl(fd,DP_POLL,&timeout);
  for (i=0; i<r; ++i)
    printf("event %d on fd #%d\n",p[i].revents,p[i].fd);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_DEVPOLL], 1, 
[Define this if your OS supports /dev/poll])], [AC_MSG_RESULT([no])])])

# check for sendfile() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SENDFILE], 
[m4_define([AC_HAVE_SENDFILE], [dnl
AC_DEFINE_UNQUOTED([HAVE_SENDFILE], 1, [Define this if you have the sendfile() system call])])
AC_MSG_CHECKING([for BSD sendfile])
AC_COMPILE_IFELSE([
/* for macos X, dont ask */
#define SENDFILE 1

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int main() {
  struct sf_hdtr hdr;
  struct iovec v[17+23];
  int r,fd=1;
  off_t sbytes;
  hdr.headers=v; hdr.hdr_cnt=17;
  hdr.trailers=v+17; hdr.trl_cnt=23;
  r=sendfile(0,1,37,42,&hdr,&sbytes,0);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_BSDSENDFILE], 1, [Define this if you a BSD style sendfile()])],
[AC_MSG_RESULT([luckyl not])
AC_MSG_CHECKING([for sendfile])
AC_COMPILE_IFELSE([
#ifdef __hpux__
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

int main() {
/*
      sbsize_t sendfile(int s, int fd, off_t offset, bsize_t nbytes,
                    const struct iovec *hdtrl, int flags);
*/
  struct iovec x[2];
  int fd=open("configure",0);
  x[0].iov_base="header";
  x[0].iov_len=6;
  x[1].iov_base="footer";
  x[1].iov_len=6;
  sendfile(1 /* dest socket */,fd /* src file */,
           0 /* offset */, 23 /* nbytes */,
           x, 0);
  perror("sendfile");
  return 0;
}
#elif defined (__sun__) && defined(__svr4__)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <stdio.h>

int main() {
  off_t o;
  o=0;
  sendfile(1 /* dest */, 0 /* src */,&o,23 /* nbytes */);
  perror("sendfile");
  return 0;
}
#elif defined (_AIX)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  struct sf_parms p;
  int destfd=1;
  p.header_data="header";
  p.header_length=6;
  p.file_descriptor=fd;
  p.file_offset=0;
  p.file_bytes=23;
  p.trailer_data="footer";
  p.trailer_length=6;
  if (send_file(&destfd,&p,0)>=0)
    printf("sent %lu bytes.\n",p.bytes_sent);
}
#elif defined(__linux__)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <unistd.h>
#if defined(__GLIBC__)
#include <sys/sendfile.h>
#elif defined(__dietlibc__)
#include <sys/sendfile.h>
#else
#include <linux/unistd.h>
_syscall4(int,sendfile,int,out,int,in,long *,offset,unsigned long,count)
#endif
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  off_t o=0;
  off_t r=sendfile(1,fd,&o,23);
  if (r!=-1)
    printf("sent %llu bytes.\n",r);
}
#endif
], [AC_HAVE_SENDFILE
AC_MSG_RESULT([yes])
])], [AC_MSG_RESULT([no])])])


