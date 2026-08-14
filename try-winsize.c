#include <sys/ioctl.h>

int main(int argc, char* argv[]) {
  struct winsize sz;

  if(ioctl(0, TIOCGWINSZ, &sz) == 0)
    return 0;

  return 1;
}
