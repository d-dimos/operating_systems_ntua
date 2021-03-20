/**
 * \file lab4.c
 * \Operating Systems - Lab 4
 * \Program that implements a client that connects to a server through a socket
*/


#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

#define STREQUAL(x, y) (strncmp(x, y, strlen(y)) == 0)

#define default_HOST "tcp.akolaitis.os.grnetcloud.net"
#define default_PORT 8080

#define BUFSIZE 1024
#define STD_IN 0

#define GREEN "\033[32m"
#define RED "\033[31;1m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"


int main(int argc, char** argv) {
   char *HOST = default_HOST;
   int PORT = default_PORT;
   int debug = 0;


   /* Check Arguements */
   for(int i = 0; i < argc; ++i) {   
      if(STREQUAL(argv[i], "--host"))
         if(i != argc-1) HOST = argv[i+1];
         else {
            printf(RED"Usage: %s [--host HOST] [--port PORT]\n", argv[0]);
            exit(-1);
         }
      if(STREQUAL(argv[i], "--port"))
         if(i != argc-1) PORT = atoi(argv[i+1]);
         else {
            printf(RED"Usage: %s [--host HOST] [--port PORT]\n", argv[0]);
            exit(-1);
         }
      if(STREQUAL(argv[i], "--debug"))
         debug = 1;
   }
  
   printf(BLUE"Connecting...\n");
  
   /* Socket Creation */
   int sock_fd;
   int domain = AF_INET;
   int type = SOCK_STREAM;

   sock_fd = socket(domain, type, 0);
   if(sock_fd < 0) {
      printf(RED"Socket Error\n");
      exit(-1);
   }
  

   /* Set up struct sockaddr_in */
   struct sockaddr_in sin;
   sin.sin_family = AF_INET;
   sin.sin_port = htons(PORT);
  
  
   /* Get info on host */
   struct hostent *server_host;
   server_host = gethostbyname(HOST);
   if(server_host == NULL) {
      printf(RED"Unknown Host\n");
      exit(1);
   }
   bcopy((char*)server_host->h_addr, (char*)&sin.sin_addr, server_host->h_length);
  

   /* Connect to server */
   if(connect(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) { 
      printf(RED"\nConnection Failed\n"); 
      exit(-1); 
   }
   printf(GREEN"Connected to: %s: %d!\n", HOST, PORT);

   /* Needed to read and write data */
   char buf[BUFSIZE];
   int nread;
   int nwrite;

   /* Loop that stops when client says so */
   while(1) {

      /* Get command from terminal */
      nread = read(STD_IN, buf, BUFSIZE-1);
      if(nread <= 0 ) {
         printf(RED"Failed to get command from terminal\n");
         exit(-1);
      }

      /* STD_IN check */
      if(STREQUAL(buf, "exit"))
         break;

      if(STREQUAL(buf, "help")) {
         printf(YELLOW"Available Commands:\n");
         printf(YELLOW"*'help'                     : Print this help message\n");
         printf(YELLOW"*'exit'                     : Exit\n");
         printf(YELLOW"*'get'                      : Retrieve sensor data\n");
         printf(YELLOW"*'N name surname reason'    : Ask permission to go out\n\n");
         continue;
      }

      if(STREQUAL(buf, "get")) {
         if(debug)
            printf(MAGENTA"[DEBUG] sent 'get'\n");

         nwrite = write(sock_fd, buf, nread);
         if(nwrite <= 0) {
            printf(RED"Failed to send request to %s\n", HOST);
            exit(-1);
         }

         nread = read(sock_fd, buf, BUFSIZE-1);
            if(nread <= 0 ) {
               printf(RED"Failed to get info from server\n");
               exit(-1);
         }
         buf[nread] = 0;
         if(debug)
            printf(MAGENTA"[DEBUG] read: %.*s", nread, buf);
         printf(YELLOW"--------------------\n");

         /* Split data in different arrays */
         char latest_event[1];
         char brightness[4];
         char temperature[5];
         char timestamp[11];

         latest_event[0] = buf[0];
         strncpy (brightness, buf+2, 3); brightness[3] = '\0';
         strncpy (temperature, buf+6, 4); temperature[4] = '\0';
         strncpy (timestamp, buf+11, 10); timestamp[10] = '\0';

         time_t rawtime = atoi(timestamp);
         struct tm *info = localtime(&rawtime);

         switch(latest_event[0]) {
            case '0': printf("Latest event: boot (0)\n");     break;
            case '1': printf("Latest event: setup (1)\n");    break;
            case '2': printf("Latest event: interval (2)\n"); break;
            case '3': printf("Latest event: button (3)\n");   break;
            case '4': printf("Latest event: motion (4)\n");
         }
         
         printf("Temperature is: %d\n", atoi(temperature)/100);
         printf("Light level is: %d\n", atoi(brightness));
         printf("Timestamp is: %s\n", asctime(info));

         continue;
      }


      // If we don't enter another if, then
      // we consider given message to be a "go out" request


      /* Send 'go out' request to server */
      if(debug)
         printf(MAGENTA"[DEBUG] sent: %.*s", nread, buf);
      nwrite = write(sock_fd, buf, nread);
      if(nwrite <= 0) {
         printf(RED"Failed to send 'go out' request to %s\n", HOST);
         exit(-1);
      }


      /* Receive verification code from server */
      nread = read(sock_fd, buf, BUFSIZE-1);
         if(nread <= 0 ) {
            printf(RED"Failed to receive verification code from server\n");
            exit(-1);
      }
      buf[nread] = 0;
      if(debug)
         printf(MAGENTA"[DEBUG] read: %.*s\n", nread, buf);


      /* If client sent sth wrong, server replies "try again" */  
      if(STREQUAL(buf, "try again")) {
         printf(BLUE"Try again or type 'help' to see all possible options\n");
         continue;
      }


      /* Get verification code from terminal */
      printf(BLUE"Send verification code: %s\n", buf);
      nread = read(STD_IN, buf, BUFSIZE-1);
      if(nread <= 0 ) {
         printf(RED"Failed to get verification code from terminal\n");
         exit(-1);
      }


      /* Send verification code to server from terminal */
      nwrite = write(sock_fd, buf, nread);
      if(nwrite <= 0) {
         printf(RED"Failed to send verification code to %s\n", HOST);
         exit(-1);
      }
      if(debug)
         printf(MAGENTA"[DEBUG] sent: %.*s", nread, buf);


      /* Receive the acknowledgement from server */
      nread = read(sock_fd, buf, BUFSIZE-1);
      if(nread <= 0 ) {
         printf(RED"Failed to get acknowledgement from server\n");
         exit(-1);
      }
      if(debug)
         printf(MAGENTA"[DEBUG] read: %.*s", nread, buf);

      /* Server Response */
      printf(GREEN"Response: %.*s", nread, buf);
      if(STREQUAL(buf, "invalid code")) {
         printf(BLUE"Entered invalid code. Try again or type 'help' to see all possible options\n");
         continue;
      }
   }
   printf(BLUE"\nConnection is now over. Exited...\n");
   close(sock_fd);

   return 0;
}
