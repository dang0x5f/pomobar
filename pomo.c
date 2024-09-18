#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#define RUNNING 1
#define TITLE "POMODORO"
#define LOCKFILE "/tmp/pomo.lockfile"
#define FIVER (60*5)

const char* study_str = "study";
const char* break_str = "break";

typedef enum { 
    STUDY, 
    BREAK 
} Status_t;
Status_t status = STUDY; // global..

typedef enum { 
    STUDYTIME, 
    WARNING, 
    BREAKTIME,
    TERMINATE, 
    END 
} ForkCase_t;

Status_t get_status(void);
void change_status(void);
void itoa(int, char *);
void timetoa(int, char *);
void reverse(char *);
void sig_handler(int);
void fork_failed(void);
void fork_handler(ForkCase_t);
void write_to_lockfile(int,char*,char*,char*,Status_t);

int main(int argc, char *argv[]){
    /* TODO: add 5 minute break */
    /* TODO: + current_time buffer to herbe message */
    int seconds, quarter, fd;
    double difference;
    time_t now, future;
    char delim = ',';
    char pid_str[10];
    char current_time[10]; 
    char quarter_time[10];

    if(argc < 2 || (seconds = 60 * atoi(argv[1])) <= 0){
        fprintf(stderr, "usage: %s mins \n", argv[0]);
        exit(1);
    }
    if((fd = open(LOCKFILE, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU)) == -1){
        fprintf(stderr, "lock file exists. one instance may already be running \n");
        exit(1);
    }
    else{
        time(&now);
        future = now + seconds;
        difference = difftime(future, now);  

        itoa(getpid(), pid_str);
        timetoa(difference, current_time);

        write_to_lockfile(fd, pid_str, &delim, current_time, get_status());

        close(fd);
    }

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    quarter = seconds / 4;
    timetoa(quarter, quarter_time);

    fork_handler(STUDYTIME);
    while(RUNNING){
        sleep(1);
        now += 1;

        difference = difftime(future, now);  
        timetoa(difference, current_time);
        /* printf("%.0f \n", difference); */
        /* printf("%s \n", current_time); */
        if((fd = open(LOCKFILE, O_WRONLY|O_TRUNC)) == -1){
            fprintf(stderr, "lock file exists. one instance may already be running \n");
            exit(1);
        }
        else{
            write_to_lockfile(fd, pid_str, &delim, current_time, get_status());
            close(fd);
        }

        if(difference <= 0){
            change_status();
            time(&now);
            if(get_status() == STUDY){
                future = now + seconds;
                quarter = seconds / 4;
                fork_handler(STUDYTIME);
            }else{
                future = now + FIVER;
                quarter = FIVER / 4;
                fork_handler(BREAKTIME);
            }
            difference = difftime(future, now);  
            timetoa(difference, current_time);
            write_to_lockfile(fd, pid_str, &delim, current_time, get_status());
            timetoa(quarter, quarter_time);
        }
        else if(difference == quarter)
            fork_handler(WARNING);
    }
    remove(LOCKFILE); 
    fork_handler(END);
    return(0);
}

void fork_handler(ForkCase_t x){
    int rc;
    char *buffer;
    
    switch(x){
        case STUDYTIME:
            buffer = "Time Starting";
            break;
        case WARNING: 
            buffer = "Ending Soon";
            break;
        case BREAKTIME: 
            buffer = "Break Time";
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

void itoa(int nbr, char *arr){
    int i = 0;

    do{
        arr[i++] = (nbr % 10) + '0';
    }while((nbr /= 10) != 0);

    arr[i] = '\0';
    reverse(arr);
}

void timetoa(int time_left, char *buffer){
    int i = 0, extra_zero = 0;
    int m = time_left / 60;
    int s = time_left % 60;

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
    reverse(buffer);    
}

void reverse(char *b){
    int i, j, c;

    for(i = 0, j = strlen(b)-1; i < j; i++, j--){
        c = b[j];
        b[j] = b[i];
        b[i] = c;
    }
}

void write_to_lockfile(int fd, char *pid_str, char *delim, char *current_time, Status_t s){
    write(fd, pid_str, sizeof(char) * strlen(pid_str));
    write(fd, delim, sizeof(char));
    write(fd, current_time, sizeof(char) * strlen(current_time));
    write(fd, delim, sizeof(char));
    if(s == STUDY)
        write(fd, study_str, sizeof(char) * strlen(study_str));
    else
        write(fd, break_str, sizeof(char) * strlen(break_str));
}

void sig_handler(int sig){
    switch(sig){
        case SIGTERM:
            remove(LOCKFILE); 
            fork_handler(TERMINATE);
            exit(0);
        case SIGINT:
            remove(LOCKFILE); 
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

Status_t get_status(){
    return status;
}
void change_status(){
    status = (status == BREAK) ? STUDY : BREAK;
}
