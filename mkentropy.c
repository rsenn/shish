#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
  int fd = open("/dev/urandom", O_RDONLY);
  int i;
  
  for(i = 0; i < 16;)
  {
    int n;
    read(fd, &n, sizeof(n));
    printf("0x%08x\n", n);
    i++;
  }
  
  return 0;
}
