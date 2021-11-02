/* $Header: /home/cvs/rnpl/src/cliser.c,v 1.1.1.1 2013/07/09 00:38:27 cvs Exp $ */
#include <cliser.h>

/************************************************************************/
/*  Copyright 1996  Matthew W Choptuik  UT Austin                       */ 
/*  All rights reserved.                                                */ 
/************************************************************************/
/*                                                                      */
/*                                                                      */
/*  Generic socket based client-server utilities:                       */
/*                                                                      */
/*     ser0_start(int port)                                             */
/*        Binds a socket to specified port on invoking host and returns */
/*        socket for subsequent accepts() etc.  Returns -1 in service   */
/*        can not be started.                                           */
/*     ser0_serve_block(int ss,(int *) service(int,int))                */
/*        Waits for connections (blocks) on previously opened socket    */
/*        and invokes service after connection has been established.    */
/*        Loops until service() returns 0.                              */
/*     ser0_stop(int ss)                                                */
/*        Shuts down socket                                             */
/*     ser0_connect(char *sername,int port)                             */
/*        Connects to specified port on host sername and returns socket */
/*        Returns (-1,-2,-3) for (Unknown host, socket creation error,  */
/*        connect error) respectively.                                  */
/*                                                                      */
/************************************************************************/

/* For checking for connections using select ... */

/* fd_set                  Ser0_fd_set;          */
/* struct timeval          Timeout;              */


/************************************************************************/
/*                                                                      */
/* Starts server and returns socket for subsequent use in accept() ...  */
/* Environment variable HOSTNAME overrides value returned by            */
/* gethostname() (work-around for host machine operating via PPP)       */
/*                                                                      */
/************************************************************************/

int ser0_start(int port) {
   struct sockaddr_in  ser;
   int                 lenser = sizeof(ser);
   struct hostent     *hp = NULL;
   char                hostname[BUFLEN];
   int                 max_l_ss_queue = 5;

   int     ltrace = ON;
   int     ss,     cs,   true = 1,  sockopt,  optlen;

   if( getenv("HOSTNAME") ) {
      strcpy(hostname,getenv("HOSTNAME"));
   } else {
      if( gethostname(hostname,BUFLEN) < 0 ) {
      fprintf(stderr,"ser0_start: gethostname(%s) failed\n",hostname);
         return -1;
      }
   }

   if( (cs = ser0_connect(hostname,port)) > 0 ) {
      fprintf(stderr,"ser0_start: Server already running\n");
      close(cs);
      return -1;
   }

   if( ltrace ) {
      fprintf(stderr,"ser0_start: Attempting to serve on '%s'\n",
              hostname);
   }

   if( (hp = gethostbyname(hostname)) == NULL ) {
         fprintf(stderr,"ser0_start: gethostname(%s) failed\n",hostname);
         return -1;
   }

   if( (ss = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
      perror("ser0_start"); return -1;
   }

/* Allow server to re-use local address ... */

   setsockopt(ss,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(true));

   bzero((char *) &ser, sizeof(ser));
   ser.sin_family = AF_INET;
   bcopy(hp->h_addr,(char *) &ser.sin_addr,hp->h_length);
   ser.sin_port = htons(port);

   if( bind(ss, (struct sockaddr *) &ser, sizeof(ser)) ) {
      perror("bind"); return -1;
   }

   if( listen(ss, max_l_ss_queue) ) {
      perror("listen"); return -1;
   } else {
      if( ltrace ) {
         fprintf(stderr,"ser0_start: Listening for connections on %d\n",ss);
      }
   }

   return ss;
}

/************************************************************************/

int ser0_serve_block(int ss,PFI_II_L service) {
   int                  ltrace = OFF;
   struct sockaddr_in   cli;
   int                  lencli = sizeof(struct sockaddr_in);
   int                  cs;

   while( 1 ) {
      if( ltrace ) {
         fprintf(stderr,"ser0_serve_block: Waiting for a connection on %d\n",
                 ss);
      }
      cs = accept(ss, (struct sockaddr *) &cli, (socklen_t *) &lencli);
      if( ltrace ) {
         fprintf(stderr,"ser0_serve_block: Connection on %d \n",cs);
      }
      if( !(*service)(ss,cs) ) return 1;
   }
}

/************************************************************************/

int ser0_stop(int ss) {
   int     ltrace = OFF;

   if( shutdown(ss,2) ) {
      perror("ser0_stop"); return -1;
   }
   close(ss);

   return 1;
}

/************************************************************************/

int ser0_connect(char *sername,int port) {
   static struct sockaddr_in  ser;
   static int                 lenser = sizeof(ser);
   static struct hostent     *hp = NULL;

   int      ltrace = OFF;
   int      cs,     rc;

   if( hp == NULL ) {
      if( ltrace ) {
         fprintf(stderr,
            "ser0_connect: Initialization for communication with '%s'\n",
             sername);
      }
      if( (hp = gethostbyname(sername)) == NULL ) {
         return -1;
      }

      bzero((char *) &ser, sizeof(ser));
      ser.sin_family = AF_INET;
      bcopy(hp->h_addr,(char *) &ser.sin_addr,hp->h_length);
      ser.sin_port = htons(port);
   }

   if( (cs = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
      return -2;
   }

   rc = connect(cs, (struct sockaddr *) &ser, sizeof(ser));
   if( ltrace ) {
      fprintf(stderr,"ser0_connect: connect(%d,...) returns %d\n",
              cs,rc);
   }
   if( rc ) {
      fprintf(stderr,"ser0_connect: errno: %d\n",errno);
      return -3;
   }

   return cs;
}

/************************************************************************/

int checkenv(char *envvar) {
   if( !getenv(envvar) ) {
      fprintf(stderr,"checkenv: Environment variable %s not set\n",envvar);
      return 0;
   } else {
      return 1;
   }
}
