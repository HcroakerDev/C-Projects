#define BUFFSIZE 1048

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

    #include <Windows.h>
    typedef unsigned int uint32_t;

    typedef HANDLE Mutex;

    typedef struct
    {
        int value;
        Mutex *mutex;
    } Semaphore;

#else

    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <pthread.h>
    typedef pthread_mutex_t Mutex;
    typedef pthread_cond_t Cond;

    typedef struct
    {
        int value;
        Mutex *mutex;
        Cond *cond;
    } Semaphore;


#endif

struct testMode {
   Semaphore* sem;
   Mutex* mut;
   int slot;
   int t;
};

struct INF {
    Semaphore* sem;
    Mutex* mut;
    int busy;
    int threadNum;
    int serverFlag;
    int slot;
    int jobNum;
    double progress;
    uint32_t num;
};

struct sMem {
  double progress[10];
  char clientFlag;
  uint32_t slots[10];
  uint32_t serverFlag[10];
  uint32_t number;
};
