#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
//no way out - film to watch

struct msg {
    long type;
    char mtext[1];
};

long buff[2] = {1, 1};                //buff to get msg
int num = -1;

int main(int argc, char ** argv) {

    if (argv == NULL || argc != 2) {                        //check for right enter
        printf("Ayayay!\n");
        return 0;
    }

    char *end_ptr = NULL;                                   //"n" to n
    long int n = strtol(argv[1], &end_ptr, 10);
    if(end_ptr != NULL && *end_ptr != '\0') {
        printf("It is not a number that i want\n");
        return 0;
    }

    if (n == LONG_MAX || n == LONG_MIN) {
        printf("Your number is not apropriate\n");
        return 0;
    }
    else if (n >= 1) {
        int id = msgget(IPC_PRIVATE, 0644);                 //create a msgqueue, send a msg to the first process
    
        struct msg M;
        M.type = 1;
        //M.mtext[0] = -1;
        int send = msgsnd(id, &M, 1, 0);
        if (send == -1) {
            printf("Error has happend\n");
            return 0;
        }

        pid_t child_pid = 0;
        int status = 0;
        int err = 0;

        for (int i = 0; i < n; i++) {                       //create n children   [parent] 
            child_pid = fork();
            if (child_pid == 0) {
                num = i;
                break;
            }
        }

        if (child_pid == 0) {                               //receive number to print and send to next process [child]
            err = msgrcv(id, buff, 1, num + 1, 0);
            if (err != -1) {
                printf("%d ", /*buff[0] - 1*/ num);
                fflush(stdout);

                M.type = num + 2;
                send = msgsnd(id, &M, 1, 0);
                if (send == -1) {
                    printf("Error has happend\n");
                }

                return 0;
            }
            else {
                printf("error\n");
                return 0;
            }      
        }

        /*for (int i = 0; i < n; i++) {                     //wait for all processes [parent]
            wait(&status);
            if (status == -1) {
                printf("Error has happend\n");
            }
        }*/
        
        err = msgrcv(id, buff, 1, n + 1, 0);
            if (err != -1) {
                printf("\n");
                fflush(stdout);

                return 0;
            }

        if (child_pid == 0) {
            msgctl(id, IPC_RMID, NULL);
        }

    }
    else if (n < 1) {
        printf("I want a positive number\n");
        return 0;
    }

    return 0;
}