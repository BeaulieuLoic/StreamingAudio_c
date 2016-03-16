#include "audio.h"

int main() {
  int fd,out,x,y,z,sz;
  char buf[1024];

  fd = aud_readinit("test.wav",&x, &y, &z);
  out = aud_writeinit(x,y,z);

  while ((sz=read(fd,buf,1024))>0) {
    write(out,buf,sz);
  }
  return 0;
}
