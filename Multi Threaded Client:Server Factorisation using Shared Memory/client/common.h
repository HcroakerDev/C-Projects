#define BUFFSIZE 1048

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <Windows.h>
  typedef unsigned int uint32_t;
#else
  #include <sys/ipc.h>
  #include <sys/shm.h>
#endif


struct sMem {
  double progress[10];
  char clientFlag;
  uint32_t slots[10];
  uint32_t serverFlag[10];
  uint32_t number;
};
