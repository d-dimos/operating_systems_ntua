/**
 * \file name: lab2.c
 * \Operating Systems - Lab 2
 
 * \Dimitris Dimos <dimitris.dimos647@gmail.com>
 * \ID: 03117165
  
 * \Christos Dimopoulos <chrisdim1999@gmail.com>
 * \ID: 03117037
 
 * \date: 2020-03-07
 * \Program that utilises signals sent to father and children processes
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

//Define global flags to use in handlers
int flag1 = 0, flag2 = 0, flag3 = 0, expire = 0, begin = 1;
int daddy1 = 0, daddy2 = 0, daddy3 = 0, dad_expire = 0;

//Handler of signals SIGUSR1 for children
void child_sigusr1();

//Handler of signals SIGUSR2 for children
void child_sigusr2();

//Handler of signals SIGTERM for children
void child_sigterm();

//Handler of signals SIGALRM for children
void child_expire();

//Handler os signals SIGCHLD for father
void father_death();

//Handler of signals SIGUSR1 for father
void father_sigusr1();

//Handler of signals SIGUSR2 for father
void father_sigusr2();

//Handler of signals SIGTERM/SIGINT for father
void father_sigterm();

//Handler of signals SIGALRM for children
void father_expire();

int isNumber(char number[]);


int main(int argc, char *argv[]){
    int n = argc - 1; //number of children to create
    int k = n+1; // temporary variable for specific use
    int d[n]; //Array that contains delays of each child:
    int c[n]; //Array that contains the pids of children born
    int status;
    unsigned int seconds=100;
    struct sigaction f_action1, f_action2, f_terminate, f_expire, f_death;

    //Check for input validity
    if (argc<=1) {
        printf("No delay was given! Invalid input.\n");
        return -1;
    }

    for (int i=1; i<argc; i++){
        if (!isNumber(argv[i])){
            printf("One of the inputs given is not an integer. Try again.\n");
            return -1;
        }
    }

    //Initialize array d[n] of delays
    for (int i=0; i<n; i++){
        d[i] = atoi(argv[i+1]);
    }
    
    //Print maximum execution time of children
    printf("Maximum execution time of children is set to %d secs.\n", seconds);
    
    //Declare fathers pid and number of children born
    printf("[Father Process: %d] Was created and will create %d children!\n", getpid(), n);    




    //Children Creation:
	for(int i=0;i<n;i++){
        int pid = fork();
        pid_t w;
        int status;

        if(pid<0){
	        printf("error");
	        return 0;
        }

        if(pid == 0){
            alarm(seconds);
            struct sigaction c_action1, c_action2, c_terminate, c_alarm;
            printf("[Child Process %d: %d] Was created and will pause!\n", i+1, getpid());
            raise(SIGSTOP); //pause

            //Unpause when SIGCONT is received
            printf("[Child Process %d: %d] Is starting!\n", i+1, getpid());
            int count = 0;
            //Define handler of child in case SIGUSR1 arrives:
            c_action1.sa_handler = child_sigusr1;

            //Define handler of child in case SIGUSR2 arrives:
            c_action2.sa_handler = child_sigusr2;

            //Define handler of child in case SIGTERM arrives:
            c_terminate.sa_handler = child_sigterm;

            //Define handler of child in case SIGALRM arrives:
            c_alarm.sa_handler = child_expire;

            //main body of child
            while(1){
                //alarm(seconds);
                if(sigaction(SIGUSR1, &c_action1, NULL) == -1) {
                	perror("Sigaction SIGUSR1 error\n");
                	exit(-1);
                }
                if(sigaction(SIGUSR2, &c_action2, NULL) == -1) {
                	perror("Sigaction SIGUSR2 error\n");
                	exit(-1);
                }
                if(sigaction(SIGTERM, &c_terminate, NULL) == -1) {
                	perror("Sigaction SIGTERM error\n");
                	exit(-1);
                }
                if(sigaction(SIGALRM, &c_alarm, NULL) == -1) {
                	perror("Sigaction SIGALRM error\n");
                	exit(-1);
                }

                count++;
                sleep(d[i]);
                

                //What to do when SIGUSR1 is received?    
                if (flag1) {
                    flag1 = 0; //reset flag for next signal  
                    printf("[Child Process %d: %d] Value: %d!\n", i+1, getpid(), count); 

                }

                //What to do when SIGUSR2 is received?    
                if (flag2) {
                    flag2 = 0; //reset flag for next signal   
                    printf("[Child Process %d: %d] Echo!\n", i+1, getpid());
                    
                }

                //What to do when SIGTERM is received?    
                if (flag3) {
                    flag3 = 0; //reset flag for next signal   
                    printf("[Child Process %d: %d] Goodbye cruel world!\n", i+1, getpid());
                    exit(0);
                }

                //What to do when SIGALRM is received?    
                if (expire) {
                    printf("[Child Process %d: %d] Time expired! Final Value: %d \n", i+1, getpid(), count);
                    exit(0);
                }
            }
        }

        //wait for child to pause
        w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
        if (w==-1) { 
            perror("Error! A child hasn't stopped!"); 
            exit(-1);
        }
        c[i] = pid; //store pid of child born
       // alarm(seconds); //expiration is for everyone
	}
    alarm(seconds);
    printf("\n");
    //Unpause Children:
    pid_t w;
    for(int i=0;i<n;i++){
        kill(c[i], SIGCONT);
        w = waitpid(c[i], &status, WNOHANG);
        if (w==-1) { 
            perror("Error! A child hasn't woken up!"); 
            exit(-1);
        }
	}

     int eternity = 0;
     while(1){
    
    
            //Define handler of father in case a child dies:
    f_death.sa_handler = father_death;
    if(sigaction(SIGCHLD, &f_death, NULL)==-1)  {
    		perror("Sigaction SIGCHLD error\n");
          exit(-1);
     }

            //Define handler of father in case SIGUSR1 arrives:
    f_action1.sa_handler = father_sigusr1;
    if(sigaction(SIGUSR1, &f_action1, NULL)==-1)  {
    		perror("Sigaction SIGUSR1 error\n");
          exit(-1);
     }


    //Define handler of father in case SIGUSR2 arrives:
    f_action2.sa_handler = father_sigusr2;
    if(sigaction(SIGUSR2, &f_action2, NULL)==-1)   {
    		perror("Sigaction SIGUSR2 error\n");
          exit(-1);
     }

    //Define handler of father in case SIGTERM / SIGINT arrives:
    f_terminate.sa_handler = father_sigterm;
    if(sigaction(SIGTERM, &f_terminate, NULL)==-1)  {
    		perror("Sigaction SIGTERM error\n");
          exit(-1);
     }
    if(sigaction(SIGINT, &f_terminate, NULL)==-1) {
    		perror("Sigaction SIGINT error\n");
          exit(-1);
     }

    //Define handler of father in case SIGALRM arrives:
    f_expire.sa_handler = father_expire;
    if(sigaction(SIGALRM, &f_expire, NULL)==-1)  {
    		perror("Sigaction SIGALRM error\n");
          exit(-1);
     }

        if(begin == 1) { 
          k--;
          printf("\n[Father process: %d] ", getpid());
          if(k == 0) {
            printf("Will exit. All children have been terminated!!!\n\n");
            exit(0);
          }
          printf("Waiting for %d children that are still working!!!\n\n", k);
          begin = 0;
        }


        //What to do when SIGUSR1 is received?    
        if (daddy1) {

            printf("[Father Process: %d] Will ask current values (SIGUSR1) from all active children processes.\n", getpid());
            for (int i=0; i<n; i++) {
                
                daddy1 = 0; //reset flag for next signal  
                pid_t w;
                w = kill(c[i], SIGUSR1);
                if (w == -1) {
                    perror("Error! Signal not sent properly!"); 
                    exit(-1);
	            }
            }
        }

        //What to do when SIGUSR2 is received?    
        if (daddy2) {
            printf("[Father Process: %d] Echo!\n", getpid());
            daddy2 = 0; //reset flag for next signal   
        }

        //What to do when SIGUSR2 is received?    
        if (daddy3) {
            printf("[Father Process: %d] I shall kill all of my children!\n", getpid());
            for (int i=0; i<n; i++) {
                daddy3 = 0;
                printf("[Father Process: %d] Will terminate child process %d: %d\n", getpid(), i+1, c[i]);
                pid_t w;
                w = kill(c[i], SIGTERM);
                if (w!=0){
                    perror("Error! A child was not terminated as expected!");
                    exit(-1);
                }
            }
            printf("[Father Process: %d] No children left! Suicide is the only option now!\n", getpid());
            exit(0);
        }

        if (dad_expire){
            printf("[Father Process: %d] All kids expired! Suicide is the only option now!\n", getpid());
            exit(0);
        }
        eternity++;}
    
	 return 0;  
}


//Handler of signals SIGUSR1 for children
void child_sigusr1(){
    flag1=1; 
}

//Handler of signals SIGUSR2 for children
void child_sigusr2(){
    flag2=1; 
}

//Handler of signals SIGTERM for children
void child_sigterm(){
    flag3=1; 
}

//Handler of signals SIGALRM for children
void child_expire(){
    expire = 1;
}

//Handler of signals SIGCHLD for father
void father_death(){
  begin=1; 
}

//Handler of signals SIGUSR1 for father
void father_sigusr1(){
    daddy1=1; 
}

//Handler of signals SIGUSR2 for father
void father_sigusr2(){
    daddy2=1; 
}

//Handler of signals SIGTERM/SIGINT for father
void father_sigterm(){
    daddy3=1; 
}

//Handler of signals SIGALRM for father
void father_expire(){
    dad_expire=1; 
}

int isNumber(char number[])
{
    int i = 0;
    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++)
    {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}
