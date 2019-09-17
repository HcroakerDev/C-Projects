#include "common.h"
#include "console.h"
#include <time.h>

struct sMem* sharedMem;
int *positions;
int canPrint = 1;

#ifndef WIN32
    #include <termios.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <pthread.h>

    int kbhit(){
      struct timeval tv;
      fd_set fds;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
      return FD_ISSET(STDIN_FILENO, &fds);
    }
#endif

// Shutdown the client and the server safely
void shutDown(int sig){
    printf("Ctrl c detected\n");
    sharedMem->clientFlag = 3;
    shmdt(sharedMem);
    exit(1);
}


// -------  Function ------
// Used to listen for communication between client and server.
// Arguments it the query/slot number passed back from the server
#ifdef _WIN32
DWORD WINAPI listen(LPVOID arg)
#else
void *listen(void *arg)
#endif
{
  // printf("Hey\n");
  int *queryNum = (int*) arg;
  printf("Query num: %d\n",*queryNum);
  int index = *queryNum;
  // printf("%d\n",info->busy);
  clock_t start = clock();
  clock_t init = clock();
  double runningTime = 0;

  while(1){
        if(sharedMem->serverFlag[index]==1){
            while(!canPrint){}
            start = clock();
            // printf("\r");
            canPrint=0;
            printf("Query %d, Factor: %u\n",*queryNum, sharedMem->slots[index]);
            sharedMem->serverFlag[index]=0;
            canPrint=1;
        }else if(sharedMem->serverFlag[index]==2){
            while(!canPrint){}
            canPrint=0;
            printf("\r");
            BarDisplayEx(sharedMem->progress[index],sharedMem->progress[index]/4,25,0,*queryNum);
            printf("\n");
            printf("Query %d done, Total time: %fs\n",*queryNum,((double)(clock() - init)/CLOCKS_PER_SEC));
            // fflush(0);
            sharedMem->serverFlag[index]=0;
            canPrint=1;
            return NULL;
        }
        runningTime = ((double)(clock() - start)/CLOCKS_PER_SEC)*1000;
        if(runningTime>500){
            while(!canPrint){}
            // if(sharedMem->progress[index]==100){
            //     sharedMem->serverFlag[index]=2;
            //     break;
            // }
            canPrint=0;
            start = clock();
            printf("\r");
            BarDisplayEx(sharedMem->progress[index],sharedMem->progress[index]/4,25,0,*queryNum);
            printf("\n");
            canPrint=1;
            // fflush(0);
            // printf("\033[A\033[2K",stdout);
            // printf("Progress %lf\n",sharedMem->progress[pos]);
        }
  }

  return NULL;
}

int main(int argc, char const *argv[]) {



   char message[BUFFSIZE];

   #ifdef _WIN32
   // Initialise shared memory for windows
   TCHAR szName[]=TEXT("Global\\shmfile");
   HANDLE hMapFile;
   hMapFile = CreateFileMapping(
                  INVALID_HANDLE_VALUE,    // use paging file
                  NULL,                    // default security
                  PAGE_READWRITE,          // read/write access
                  0,                       // maximum object size (high-order DWORD)
                  sizeof(struct sMem),                // maximum object size (low-order DWORD)
                  szName);                 // name of mapping object

    if (hMapFile == NULL)
    {
       printf(TEXT("Could not create file mapping object (%d).\n"),GetLastError());
       return 1;
    }
    sharedMem = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct sMem));
     if (sharedMem == NULL)
     {
        printf("Could not map view of file (%d).\n",GetLastError());
        CloseHandle(hMapFile);
        return 1;
     }
     for (size_t i = 0; i < 10; i++) {
       sharedMem->serverFlag[i]=0;
       sharedMem->slots[i]=-1;
     }
   #else
   nonblock(1);
   signal(SIGINT, shutDown);
   // Init shared memory for
   key_t key = ftok("shmfile",65);

   // shmget returns an identifier in shmid
   int shmid = shmget(key,1024,0666|IPC_CREAT);

   // shmat to attach to shared memory
   sharedMem = (struct sMem*) shmat(shmid,(void*)0,0);
   for (size_t i = 0; i < 10; i++) {
     sharedMem->serverFlag[i]=0;
     sharedMem->slots[i]=-1;
   }
   #endif
   sharedMem->clientFlag = 0;

   printf("Enter a message or press (q) to quit\n$: ");
   fgets(message, BUFFSIZE, stdin);
   message[strlen(message)]='\0';
   positions = (int*)calloc(10, sizeof(int));
   pthread_t thread_id[10];
   int pos = 0;


   // Ask user for number, quit on q.
   while (strncmp(message, "q",strlen("q"))!=0) {


       if(sharedMem->clientFlag==0){
         uint32_t num = atoi(message);

         // Copy the data in and set the status to ready
         sharedMem->number = num;
         sharedMem->clientFlag = 1;

         while (1) {

           // Tell the server data is ready to be received.
           if(sharedMem->clientFlag==0){
             pos = sharedMem->number;
             positions[pos]=pos;

             // Start a new thread for communication
             #ifdef _WIN32
             printf("%d\n",CreateThread(
                     NULL,                   // default security attributes
                     0,                      // use default stack size
                     &factor,       // thread function name
                     (LPVOID)(&queryId[i]),          // argument to thread function
                     0,                      // use default creation flags
                     NULL));   // returns the thread identifier
             #else

             pthread_create(&thread_id[pos], NULL, listen, &positions[pos]);
             #endif
             break;
           }
           else if(sharedMem->clientFlag==4){
             printf("\n\nSERVER BUSY! ALL SLOTS TAKEN\n\n");
             sharedMem->clientFlag = 0;
             break;
          }
        }
     }

     // Wait until key is pressed
     while(!kbhit()){}
     // while(1){}
     fgets(message, BUFFSIZE, stdin);
     message[strlen(message)]='\0';
     printf("Got message\n");
     // pthread_join(thread_id[pos], NULL);
     // }


   }

   //detach from shared memory
   #ifdef _WIN32
   UnmapViewOfFile(sharedMem);
   CloseHandle(hMapFile);
   #else
   shmdt(sharedMem);
   shmctl(shmid,IPC_RMID,NULL);
   #endif
   free(positions);


   return 0;
}
