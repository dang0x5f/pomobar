#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define RUNNING 1
#define TITLE "POMODORO"

typedef enum { START, PRINT, TERMINATE, END } ForkCase_t;

void timetoa(int, char *);
void reverse_time(char *);
void sig_handler(int);
void fork_failed(void);
void fork_handler(ForkCase_t);

int main(int argc, char *argv[]){
    /* TODO: file locking, open() */ 
    /* TODO: lock file content -> pid:seconds */
    /* TODO: math to print timing in notifications & lock file */
    /* TODO: add 5 minute break */

    int seconds, pid;
    double difference;
    time_t now, future;
    char current_time[10]; 

    if(argc < 2 || (seconds = 60 * atoi(argv[1])) <= 0){
        fprintf(stderr, "usage: %s mins \n", argv[0]);
        exit(1);
    }

    pid = getpid();
    printf("pid : %d \n", pid);

    time(&now);
    future = now + seconds;
    signal(SIGTERM, sig_handler);

    fork_handler(START);
    while(RUNNING){
        sleep(1);
        now += 1;

        difference = difftime(future, now);  // TODO: necessary? or is sleep(1)+now+1 enuf?
        /* printf("%.0f \n", difference); */
        timetoa(difference, current_time);
        printf("%s \n", current_time);

        if(difference <= 0)
            break;
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
        case PRINT: // TODO: print warning
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

void timetoa(int time_left, char *buffer){
    int i = 0, extra_zero = 0;
    int m = time_left / 60;
    int s = time_left % 60;

    /* printf("%d : %d \n", m, s); */
    extra_zero = (s < 10 ? 1 : 0);

    do{
        buffer[i++] = (s % 10) + '0';
    }while((s /= 10) != 0);

    if(extra_zero)
        buffer[i++] = '0';

    buffer[i++] = ':';

    do{
        buffer[i++] = (m % 10) + '0';
    }while((m /= 10) != 0);

    buffer[i] = '\0';

    reverse_time(buffer);    
}

void reverse_time(char *b){
    int i, j, c;

    for(i = 0, j = strlen(b)-1; i < j; i++, j--){
        c = b[j];
        b[j] = b[i];
        b[i] = c;
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
