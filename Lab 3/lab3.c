/**
 * \file lab3.c
 * \Operating Systems - Lab 3
 * \Program that utilises pipes between father and n children tasks
*/

#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#define STREQUAL(x, y) ( strncmp((x), (y), strlen(y) ) == 0 )
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define RED "\033[31;1m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define WHITE "\033[37m"

int isPositiveNumber(char number[]);
int isNumber(char number[]);


int main(int argc, char **argv) {
  int n;    // n children
  int flag;

  /* -------------------- Check Input Validity -------------------- */
    if(argc != 2 && argc != 3) {
      printf("Usage: %s <nChildren> [--random] [--round-robin].\n", argv[0]);
      exit(0);
    }
    if (argc == 2){
      if(!isPositiveNumber(argv[1])){
        printf("Usage: %s <nChildren> [--random] [--round-robin].\n", argv[0]);
        exit(0);
      }
      else {
        n = atoi(argv[1]);
        flag = 1; // round-robin
      }
    }
    if (argc == 3) {
      if(!isPositiveNumber(argv[1]) || !STREQUAL(argv[2],"--random") && !STREQUAL(argv[2],"--round-robin")) {
        printf("Usage: %s <nChildren> [--random] [--round-robin].\n", argv[0]);
        exit(0);
      }
      else {
        n = atoi(argv[1]);
        if(STREQUAL(argv[2],"--random"))
          flag = 0;    // random
        else flag = 1; // round-robin
      }
    }
  /* -------------------------------------------------------------- */

  int c[n];                 // Contains children PIDs
  int pd0[n][2], pd1[n][2]; // a read-end and a write-end pipe for each child

  /* -------- children creation and pipe-ends adjustments --------- */
  for(int i = 0; i < n; ++i) {
    pipe(pd0[i]);
    pipe(pd1[i]);

    if (pipe(pd0[i]) != 0) { perror(RED"pipe\n"); exit(-1); }
    if (pipe(pd1[i]) != 0) { perror(RED"pipe\n"); exit(-1); }

    int pid = fork();        
    if (pid < 0) { printf(RED"Child Bearing Error\n"); exit(-1); }
    c[i] = pid; // store child pid
    
    // children code
    if(pid == 0) {
      int val;  // contains input number to increment

      close (pd0[i][1]); // close write-end of pipe 0
      close (pd1[i][0]); // close read-end  of pipe 1

      while(1) {   // child till-death loop
        pid_t r = read(pd0[i][0], &val, sizeof(int));
        if (r == -1) { perror(RED"Error! Int not read properly!\n"); exit(-1); }

        if (r > 0) {
          printf(YELLOW"[Child %d] [%d] Child received %d!"WHITE"\n", i, getpid(), val);
          val++;
          sleep(5);
          pid_t y = write(pd1[i][1], &val, sizeof(int));
          if (y == -1) { perror(RED"Error! Not written properly!\n"); exit(-1); }
          printf(YELLOW"[Child %d] [%d] Child Finished hard work, writing back %d\n", i, getpid(), val);
        }
      }
    }
  }
  /* -------------------------------------------------------------- */

  int chosen_one = 0; // initialize for round-robin

  // close unused father <--> child[i] pipe ends 
  for (int i=0; i<n; i++) {
      close(pd0[i][0]);
      close(pd1[i][1]);
  }

  // till-death father loop
  while(1) {
    fd_set inset;
    int maxfd;
    

    /* ---------------------- File descriptor SELECTION ----------------------- */
    FD_ZERO(&inset);  // we must initialize before each call to select
    FD_SET(STDIN_FILENO, &inset); // select() will check for input from stdin

    for(int i = 0; i < n; ++i) {
      FD_SET(pd1[i][0], &inset);  // select() will check for input from pipes
      
      maxfd = MAX(STDIN_FILENO, pd1[i][0]);  // select() only considers file descriptors that are smaller than maxfd
    }
    maxfd = maxfd + 1;

    // wait until any of the input file descriptors are ready to receive
    int ready_fds = select(maxfd, &inset, NULL, NULL, NULL);
    if (ready_fds <= 0) { perror(RED"select\n"); continue; } // try again
    /* ------------------------------------------------------------------------ */


    /* ---------------------- Reading from STANDARD INPUT --------------------- */
    if (FD_ISSET(STDIN_FILENO, &inset)) {
      char buffer[101];

      int n_read = read(STDIN_FILENO, buffer, 100);   
      if (n_read < 0) { perror(RED"Error while reading!\n"); exit(-1); } 
      buffer[n_read] = '\0'; // end of string symbol
      
      // Discard new-line (if exists)
      if (n_read > 0 && buffer[n_read-1] == '\n')
        buffer[n_read-1] = '\0';

      printf(BLUE"Got user input: '%s'"WHITE"\n", buffer);
    
      if (n_read >= 4 && strncmp(buffer, "help", 4) == 0)
        printf("Type a number to send job to a child!\n");

      else if (n_read >= 4 && strncmp(buffer, "exit", 4) == 0) {
        // user typed 'exit', kill child and exit properly
        pid_t w;
        int children = n;
        for (int i = 0; i < n; i++){
          w = kill(c[i], SIGTERM);
          if (w == -1) { perror(RED"Error! A child was not terminated as expected!\n"); exit(-1); }

          children--;
          printf(GREEN"Waiting for %d.\n", children);

          pid_t t;
          t = wait(NULL);
          if (t == -1) { perror(RED"Error while waiting!\n"); exit(-1); }
        }
        printf(GREEN"All children terminated!\n");
        for (int i = 0; i < n; i++){
          close(pd0[i][1]);
          close(pd1[i][0]);
        }
        exit(0);
      }

      else if (n_read > 0 && isNumber(buffer)) {
        int k = atoi(buffer);
        // random
        if (flag == 0) {
          chosen_one = rand() % n;
          pid_t w = write(pd0[chosen_one][1], &k, sizeof(int));
          if (w == -1) { perror(RED"Error while writing in pipe!\n"); exit(-1); }
          printf(GREEN"Parent assigned %d to child %d.\n", k, chosen_one);
        }
        
        // round-robin
        if (flag == 1) {
          pid_t w = write(pd0[chosen_one][1], &k, sizeof(int));
          if (w == -1) { perror(RED"Error while writing in pipe!\n"); exit(-1); }
          printf(GREEN"Parent assigned %d to child %d.\n", k, chosen_one);
          
          chosen_one++;
          if (chosen_one == n) chosen_one = 0; //reset
        }
      }

      else if (n_read > 0) 
        printf("Type a number to send job to a child!\n");
    }
    /* ------------------------------------------------------------------------ */

    /* ---------------------- Reading from a Child Pipe ----------------------- */
    for (int i = 0; i < n; i++) {
      if (FD_ISSET(pd1[i][0], &inset)) {
        int val;
        pid_t x = read(pd1[i][0], &val, sizeof(int));
        if (x ==-1) { perror(RED"Can't read pipe. Error!"); exit(-1); }
        printf(GREEN"[Parent] Received result from child %d --> %d\n", i, val);
      }
    }
    /* ------------------------------------------------------------------------ */
  }
  return 0;
}

// checks if given array contains positive numbers
int isPositiveNumber(char number[]) {
  int i = 0;
  if (number[0] == '-')
    return 0;
  for (; number[i] != 0; i++)
    if (!isdigit(number[i]))
      return 0;
  return 1;
}

// checks if given array contains numbers
int isNumber(char number[]) {
  int i = 0;
  if (number[0] == '-') {
    for (; number[i] != 0; i++)
      if (!isdigit(number[i]) && i!=0)
        return 0;
  }
  else
    for (; number[i] != 0; i++)
      if (!isdigit(number[i]))
        return 0;
  return 1;
}
