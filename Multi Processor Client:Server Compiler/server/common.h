#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/sysctl.h>

#ifdef HAVE_ST_BIRTHTIME
#define creation(x) x.st_birthtime
#else
#define creation(x) x.st_ctime
#endif

#ifdef __APPLE__
#define cpu() "sysctl -n machdep.cpu.brand_string"
#elif __linux__
#define cpu() "grep 'model name' /proc/cpuinfo | head -n 1"
#endif


#define BUFFSIZE 2000
#define LINESIZE 10000
