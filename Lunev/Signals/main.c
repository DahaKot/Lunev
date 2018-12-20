#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void parent(int child_pid);
void child(const char *filename, int parent_pid);
void wait_parent(int parent_pid);

void sigchld_handler(int sig_num);
void sigusr1_handler(int sig_num);
void sigusr1_child_handler(int sig_num);
void sigusr2_handler(int sig_num);
void sigalrm_handler(int sig_num);
void set_parent_actions();
void set_child_actions();

char bit = 0;
char alarma = 0;

int main(int argc, char **argv) {
    if (argv == NULL || argc != 2) {
        printf("Ayayay!\n");
        return 0;
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGCHLD);

    sigprocmask(SIG_BLOCK, &set, NULL);

    int parent_pid = getpid();
    int child_pid = fork();

    if (child_pid != 0) {
        parent(child_pid);
    }
    else {
        child(argv[1], parent_pid);
    }

    return 0;
}

void parent(int child_pid) {
    set_parent_actions();

    sigset_t empty_set;
    sigemptyset(&empty_set);

    //
    kill(child_pid, SIGUSR1);

    int byte = 0;
    while(1) {
        for(int i = 0; i < 8; i++) {
            sigsuspend(&empty_set);

            byte = byte | (bit << i);
            //usleep(1000);
            kill(child_pid, SIGUSR1);
        }

        write(STDOUT_FILENO, &byte, 1);
        fflush(stdout);

        byte = 0;
    }
}

void child(const char *filename, int parent_pid) {
    set_child_actions();

    int input_fd = open(filename, O_RDONLY);
    if (input_fd == -1) {
        printf("Cannot open the file\n");
        return ;
    }
    lseek(input_fd, 0, SEEK_SET);

    int buff = 0;
    int read_s = 0;

    sigset_t empty_set;
    sigemptyset(&empty_set);

    //
    sigsuspend(&empty_set);

    while(1) {
        read_s = read(input_fd, &buff, 1);
        if (read_s == 0) {
            break;
        }

        for(int i = 0; i < 8; i++) {
            if (buff % 2 == 0) {
                kill(parent_pid, SIGUSR1);
            }
            else {
                kill(parent_pid, SIGUSR2);
            }
            //usleep(1000);

            wait_parent(parent_pid);

            buff >>= 1;
        }
    }
}

void wait_parent(int parent_pid) {
    sigset_t empty_set;
    sigemptyset(&empty_set);

    alarma = 1;
    do {
        if (parent_pid != getppid()) {
            exit(1);
        }

        alarm(1);
        sigsuspend(&empty_set);
        alarm(0);
    } while(alarma);
}

void sigchld_handler(int sig_num) {
    exit(0);
}

void sigusr1_handler(int sig_num) {
    bit = 0;
}

void sigusr1_child_handler(int sig_num) {
    alarma = 0;
}

void sigusr2_handler(int sig_num) {
    bit = 1;
}

void sigalrm_handler(int sig_num) {
    //empty handler
}

void set_parent_actions() {
    struct sigaction action = {};

    action.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &action, NULL);

    action.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &action, NULL);

    action.sa_handler = sigusr2_handler;
    sigaction(SIGUSR2, &action, NULL);
}

void set_child_actions() {
    struct sigaction action = {};

    action.sa_handler = sigusr1_child_handler;
    sigaction(SIGUSR1, &action, NULL);

    action.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &action, NULL);
}
