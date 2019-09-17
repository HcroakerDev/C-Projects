#include <sys/time.h>
#include <stdio.h>
#define STDIN_FILENO 0
#define NB_ENABLE 1
#define NB_DISABLE 0
#ifndef _WIN32
#include <termios.h>

void nonblock(int state)
{
  struct termios ttystate;
  tcgetattr(STDIN_FILENO, &ttystate);
  if (state == NB_ENABLE)
  {
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_cc[VMIN] = 1;
  }
  else if (state==NB_DISABLE)
  {
    ttystate.c_lflag |= ICANON;
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}
#endif

void BarDisplayEx(float progress,float full,int max,int blocked,int q)
{
  int i;
  printf("Query %d Progress: %d%% ",q,(int)progress);
  for (i = 0; i < full; i++) printf("#");
  for (i = 0; i < max-full; i++) printf("_");
  // if (q)
  // {
  //   printf("     Query: %2d \n", q);
  //   if (blocked) printf("BLOCKED");
  //   else
  //   printf("       ");
  //   for (i = 0; i < 7+16; i++) printf("\b");
  // }
  fflush(0);
}

void BarDisplay(int full, int max)
{
  BarDisplayEx(0,full, max, 0,0);
}

void BarDelete(int max)
{
  for (int i = 0; i < max; i++) printf("\b");
}
