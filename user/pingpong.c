#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
  int p[2];
  char buf[1];
  char ch = 'd';

  pipe(p);

  int pid = fork();

  if (pid == 0) {
    read(p[0], buf, sizeof(buf));
    if(buf[0]==ch){
      printf("%d: received ping\n", getpid());
    }
    write(p[1], buf, sizeof(buf));
  }
  else {
    write(p[1], &ch, 1);

    wait(0);

    read(p[0], buf, sizeof(buf));
    if(buf[0]==ch){
      printf("%d: received pong\n", getpid());
    }
  }

  exit(0);
}
