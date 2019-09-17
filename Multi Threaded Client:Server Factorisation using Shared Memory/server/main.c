#include "common.h"
#include <assert.h>

#ifdef _WIN32
#include <conio.h>
#include <tchar.h>
HANDLE ghSemaphore;
DWORD WINAPI factor( LPVOID );
#else
int shmid; // Share mem variable
#include <unistd.h>
#endif

struct sMem *sharedMem;      // Shared memory struct
struct INF *threadInfo;      // Array of structs, used for holding information used within thread functions
int MAX_THREADS;             // Max number of threads
double progress_all[10][32]; // Progress of all jobs

// Make condition only for linux
#ifndef _WIN32
Cond* make_cond(){
    Cond* cond = (Cond*)malloc(sizeof(Cond));
    pthread_cond_init(cond, NULL);
    return cond;
}
#endif

// * ----- Function --- * //
// * Used to make the mutex.
// * Params:
// * - None
// *
// * Returns metex struct
Mutex* make_mutex(){
    Mutex* mutex = (Mutex*)malloc(sizeof(Mutex));
    // Using win32 and pthreads
    #ifdef _WIN32
    *mutex = CreateMutex(NULL, FALSE, NULL);
    #else
    pthread_mutex_init(mutex, NULL);
    #endif
    return mutex;
}

// * ----- Function --- * //
// * Used to make the semaphore.
// * Params:
// * - Int Value: Sem value
// *
// * Returns Semaphore struct
Semaphore *make_semaphore(int value){
    Semaphore *semaphore = ( Semaphore*)malloc(sizeof(Semaphore));
    semaphore->value = value;
    semaphore->mutex = make_mutex();
    #ifndef _WIN32
    semaphore->cond = make_cond();
    #endif
    return semaphore;
}


// * ----- Function --- * //
// * Used to synchronise access to shared memory.
// * Params:
// * - Semaphore sem: structure for semaphore, includes mutex and semaphore
// * - int Server flag,slot : index of shared memory communication location
// * - uin32_t Fac: The factor
// *
// * Returns nothing
void sema_wait(Semaphore* sem, int serverFlag, int slot, uint32_t fac){

    #ifdef _WIN32

    while (sharedMem->serverFlag[serverFlag]) {
        WaitForSingleObject(ghSemaphore, 0L);
    }
    --sem->value;
    // Write to shared memory
    printf("Write FACTOR: %u to slot \n",fac);
    sharedMem->slots[slot] = fac;
    sharedMem->serverFlag[serverFlag] = 1;

    // Release
    ReleaseMutex(sem->mutex);
    ReleaseSemaphore(ghSemaphore, 1, NULL);
    #else

    // Lock
    pthread_mutex_lock(sem->mutex);
    while(sem->value <= 0)
        pthread_cond_wait(sem->cond, sem->mutex);
    --sem->value;

    // Write to shared memory
    printf("Write FACTOR: %u to slot \n",fac);
    sharedMem->slots[slot] = fac;
    sharedMem->serverFlag[serverFlag] = 1;

    // Release
    pthread_mutex_unlock(sem->mutex);
    #endif
}

// * ----- Function --- * //
// * Used to synchronise access to shared memory.
// * Params:
// * - Semaphore sem: structure for semaphore, includes mutex and semaphore
// *
// * Returns nothing
void sema_signal(Semaphore* sem){
    // Windows
   #ifdef _WIN32
   WaitForSingleObject(ghSemaphore, INFINITE);
   ++sem->value;
   ReleaseSemaphore(ghSemaphore, -1, NULL);
   ReleaseMutex(sem->mutex);

   #else
   // Linux
   pthread_mutex_lock(sem->mutex);
   ++sem->value;
   pthread_cond_signal(sem->cond);
   pthread_mutex_unlock(sem->mutex);
   #endif
}

// * ----- Function --- * //
// * Used to free semaphore memory.
// * Params:
// * - Semaphore sem: structure for semaphore, includes mutex and semaphore
// *
// * Returns nothing
void close_semaphore(Semaphore* s){
    free(s->mutex);
    #ifndef _WIN32
    free(s->cond);
    #endif
    free(s);
}

// * ----- Function --- * //
// * Used to for test mode.
// * Params:
// * - Semaphore sem: structure for semaphore, includes mutex and semaphore
// * - int Server flag,slot : index of shared memory communication location
// * - uin32_t Fac: The factor
// *
// * Returns nothing
#ifdef _WIN32
DWORD WINAPI count(LPVOID arg)
#else
void *count(void *arg)
#endif
{
   // printf("Hey\n");
   struct testMode *info = (struct testMode*) arg;
   int t = info->t;
   // printf("%d\n",info->busy);

   for(int i=10*(t);i<10*(t)+10;i++){
      sema_wait(info->sem, info->slot,info->slot, t);
      #ifndef _WIN32
      sema_signal(info->sem);
      #endif
   }

   return NULL;
}

// * ----- Function --- * //
// * Factorising thread function.
// * Params:
// * - Ptr Arg: A ptr to an array element that stores information about the thread.
// * Information includes:
// * - threadNum
// * - slot/serverflag index
// * - number to factor
// * - semaphore
// * - if its busy or not
// *
// * Returns nothing
#ifdef _WIN32
DWORD WINAPI factor(LPVOID arg) // Windows declartion
#else
void *factor(void *arg) // Unix declaration
#endif
{
  struct INF *info = (struct INF*) arg;
  // printf("%d\n",info->busy);


  // Idle while busy and only check for jobs every 100ms
  while(1){

    // Dont do anything while not busy.
    if(info->busy==1){
      // printf("Waiting\n");
      printf("\nI just got work %d\n",info->threadNum);

      // Semaphore
      Semaphore* s = info->sem;
      int thread_num = info->threadNum;

      //  The number to factor
      uint32_t n = info->num;
      printf("Factoring %d\n",n);

      // Find all the factors and send them back to the client
      for(uint32_t i=1;i<n/2;i++){

         // Calculate progress
         info->progress = ((double)i*100L/(n/2));
         progress_all[info->slot][info->jobNum]= ((double)i*100L/(n/2));

         // If its a factor send it back to client.
         if(n%i == 0){

             // Ensure shared memory access is mutually exclusive.
            sema_wait(s,info->serverFlag, info->slot,i); // Write to shared memory within here
            #ifndef _WIN32
            sema_signal(s);
            #endif
         }

      }

      // Set the progress to 100.
      printf("\nJust finished work... %d\n", thread_num);
      info->progress = 100.0;
      progress_all[info->slot][info->jobNum]= 100.0;

      // Go back to idling
      info->busy=0;

    }

    // Sleep for 100ms
    #ifdef _WIN32
    Sleep(100);
    #else
    usleep(100000);
    #endif

  }

  return NULL;
}

// * ----- Function --- * //
// * Rotate a number right
// * Params:
// * - uint32_t n: number to rotate
// * - unsigned int d: rotate by
// *
// * Returns rotated number
uint32_t rightRotate(uint32_t n, unsigned int d)
{
   return (n >> d)|(n << (32 - d));
}

// * ----- Function --- * //
// * Safely shutdown the server by detatching from and destroying memory
// * Params:
// * - int sig: number
// *
// * Returns rotated number
void shutDown(int sig){

   #ifndef _WIN32
   //detach from shared memory and destroy
   shmdt(sharedMem);
   shmctl(shmid,IPC_RMID,NULL);
   #endif
   printf("Ctrl c detected\n");
   exit(0);
}

// * ----- Function --- * //
// * Main function used to listen for requests.
// * Params:
// * - Command line args: number of threads in thread pool
// *
// * Returns exit status
int main(int argc, char const *argv[]) {

  // Shared memory initalization
  #ifdef _WIN32
  // Windowds file mapping
  HANDLE hMapFile;
  TCHAR szName[]=TEXT("Global\\shmfile");
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
   sharedMem = (struct sMem*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct sMem));
    if (sharedMem == NULL)
    {
       printf("Could not map view of file (%d).\n",GetLastError());
       CloseHandle(hMapFile);
       return 1;
    }
  #else

  // Linux
  signal(SIGINT, shutDown);
  key_t key = ftok("shmfile",65);

  // shmget returns an identifier in shmid
  shmid= shmget(key,1024,0666|IPC_CREAT);

  // shmat to attach to shared memory
  sharedMem = (struct sMem*) shmat(shmid,(void*)0,0);

  #endif

  // Set slots to 0, and progress
  for (size_t i = 0; i < 10; i++) {
    sharedMem->serverFlag[i]=0;
    sharedMem->slots[i]=-1;
    for(int x=0;x<32;x++){
      progress_all[i][x]=0;
    }
  }

  // Start up threads to assign jobs.
  // Use cli arg for max number of threads.
  if(argc == 2){
    MAX_THREADS = atoi(argv[1]);
  }
  else{
    MAX_THREADS = 320;
  }

  // Allocate array of thread_id's.
  #ifdef _WIN32

  // Handle type for windows
  HANDLE thread_id[MAX_THREADS];
  DWORD dwThreadIdArray[MAX_THREADS];
  Mutex mut = make_mutex();
  Semaphore* sem = make_semaphore(10);
  ghSemaphore = CreateSemaphore(
        NULL,           // default security attributes
        10,  // initial count
        10,  // maximum count
        NULL);

  #else

  // Make semaphore for linux and use pthread_t type
  Mutex mut = PTHREAD_MUTEX_INITIALIZER;
  Semaphore* sem = make_semaphore(10);
  pthread_t thread_id[MAX_THREADS];

  #endif

  // Start the thread pool by initializing many threads and have them wait for work.
  printf("%d\n",MAX_THREADS);
  threadInfo = (struct INF *)malloc(MAX_THREADS* sizeof(struct INF) ); // Alocate memory for thread pool.
  for(int t=0;t<MAX_THREADS;t++){
    threadInfo[t].sem = sem;
    threadInfo[t].mut = &mut;
    threadInfo[t].busy = 0;           // Whether the thread is busy. 1 for busy, 0 otherwise.
    threadInfo[t].threadNum = t;     // Thread num
    threadInfo[t].serverFlag = -1;  // Server flag index
    threadInfo[t].slot = -1;       // Slot index
    threadInfo[t].num = -1;       // Number to factor
    threadInfo[t].jobNum = -1;
    threadInfo[t].progress = 0.0;
    #ifdef _WIN32
    // printf("Busy %d ",threadInfo[t].busy);
    // printf("%p\n",(void*)(threadInfo + t));
    printf("%d\n",CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            &factor,       // thread function name
            (LPVOID)(threadInfo + t),          // argument to thread function
            0,                      // use default creation flags
            &dwThreadIdArray[t]) );   // returns the thread identifier
    #else
    pthread_create(&thread_id[t], NULL, factor, threadInfo + t);
    #endif

  }


  // Wait for request using shared memory.
  while (1) {

     // If the client has entered a number and changed the client flag, look for slot and send back index
     if(sharedMem->clientFlag == 1){
        printf("Received Data\n");
        if(sharedMem->number==0){
           printf("Enter test mode");

        }else{
           int found = 0;

           // Attempt to find an open slot
           for(int i=0; i<10; i++){

             // Find a free slot, -1 means free
             if(sharedMem->slots[i]==-1){

                printf("Found slot\n");
                found = 1;

                // Get the number to factor and tell the client the slot.
                sharedMem->slots[i]=1;
                uint32_t factorN = sharedMem->number;
                sharedMem->number = i;
                sharedMem->clientFlag = 0;
                printf("Sent to client\n");

                // For each rotation, find a free thread and factor it.
                for (unsigned int x = 0; x < 32; x++) {
                   // printf("%s\n", );

                   uint32_t n = rightRotate(factorN, x);
                   printf("Factor %u\n",n);

                   if(n==0){
                      printf("Gone to 0\n");
                      break;
                   }
                   int found = 0;
                   while(found==0){
                      // Find a free thread.
                      for(int g=0;g<MAX_THREADS;g++){
                        if(threadInfo[g].busy==0){
                           printf("Found a thread\n");
                           // Give it work, send the info needed
                           threadInfo[g].jobNum = x;
                           threadInfo[g].serverFlag = i;
                           threadInfo[g].slot = i;
                           threadInfo[g].num = n;
                           threadInfo[g].busy = 1;
                           found = 1;
                           break;
                        }
                     }
                     if(found){
                        break;
                     }
                     // printf("Cant find a thread\n");
                  }
               }
               break;
            }
         }
         // If there is no more slots, send back error message to say that it is full.
         if(found==0){
            sharedMem->clientFlag = 4;
         }
      }
   }
   else if(sharedMem->clientFlag==3){
      // Shutdown when the client presses ctrl+c
      printf("Time to shut down...\n");
      break;
   }
   else{

      // For each slot
      for(int i=0;i<10;i++){
         double total = 0;
         double progress = 0;

            // Calculate progress from individual threads.
            for(int x = 0;x<32;x++){
               total += progress_all[i][x];
            }

            // Get overall progress and add it to share memory
            progress = total/32;
            sharedMem->progress[i] = progress;

            // If the progress is finished
            if(progress>=100){

               // Reset that slots progress
               for(int x=0;x<32;x++){
                  progress_all[i][x]=0;
               }

               // Tell the client we are done
               sharedMem->serverFlag[i] = 2;

               // Wait for client response
               while (1) {
                  // When the client responds, allow the slot to be taken and reset progress
                  if (sharedMem->serverFlag[i]==0) {
                     sharedMem->slots[i]=-1;
                     sharedMem->progress[i] = 0;
                     printf("Done\n");
                     break;
                  }
               }
            }
      }
   }
}

// Shut down shared memory.
#ifdef _WIN32
CloseHandle(sem);
#else
close_semaphore(sem);
//detach from shared memory and destroy
shmdt(sharedMem);
shmctl(shmid,IPC_RMID,NULL);
#endif


return 0;
}
