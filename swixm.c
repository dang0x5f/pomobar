#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
/* #include <string.h> */

#define RUNNING 1
#define TITLE "POMODORO"

typedef enum { START, PRINT, TERMINATE, END } ForkCase_t;

void sig_handler(int);
void fork_failed(void);
void fork_handler(ForkCase_t x);

/* char current_time[10]; // TODO: write current time in here for output */

int main(int argc, char *argv[]){
    /* TODO: minute argument error handling */
    /* TODO: file locking, open() */ 
    /* TODO: lock file content -> pid:seconds */
    /* TODO: math to print timing in notifications & lock file */
    /* TODO: add switch structure for forks ? */

    int seconds, pid;
    double difference;
    time_t now, future;

    if(argc < 2 || (seconds = 60 * atoi(argv[1])) <= 0){
        fprintf(stderr, "usage: %s mins \n", argv[0]);
        exit(1);
    }

    pid = getpid();
    printf("pid : %d \n", pid);

    time(&now);
    future = now + seconds;
    signal(SIGTERM, sig_handler);

    while(RUNNING){
        sleep(1);
        now += 1;

        difference = difftime(future, now);  // TODO: necessary? or is sleep(1) + now++ enuf?

        printf("%.0f \n", difference);

        if(difference <= 0){
            break;
        }
    }
    fork_handler(END);
    return(0);
}

void fork_handler(ForkCase_t x){
    int rc;
    char *buffer;

    switch(x){
        case START:
            buffer = "Time Starting";
            break;
        case TERMINATE:
            buffer = "Terminated";
            break;
        case END:
            buffer = "Times Up";
            break;
        default:
            break;
    }

    if((rc = fork()) < 0){
        fork_failed();
    }
    else if(rc == 0){
        char *notify[] = {"herbe", TITLE, " ", buffer, NULL};
        execvp(notify[0],notify);
    }
}

void sig_handler(int sig){
    switch(sig){
        case SIGTERM:
            fork_handler(TERMINATE);
            exit(0);
        default:
            fprintf(stderr, "default case\n");
            exit(1);
    }
}

void fork_failed(void){
    fprintf(stderr, "fork failed\n");
    exit(1);
}
