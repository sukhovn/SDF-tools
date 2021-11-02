#ifndef CLISER_DEF
#define CLISER_DEF

#include <stdlib.h>
#include <math.h>
#include <limits.h> 
/* #include <values.h> */
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
/* #include <termio.h> */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
/* #include <sys/dir.h> */
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
/* #include <arpa/inet.h> */
#include <netdb.h>

#define                        ON          1
#define                        OFF         0
#define                        BUFLEN      1024

typedef  int                   (* PFI_II_L) (int,int);

extern   int                   ser0_start(int port);
extern   int                   ser0_serve_block(int ss,PFI_II_L service);
extern   int                   ser0_stop(int ss);
extern   int                   ser0_connect(char *sername,int port);

extern   int                   checkenv(char *envvar);
#endif
