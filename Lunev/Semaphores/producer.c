#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#define SET_SEMOP(num, s_num, op, flag)     \
    semops[num].sem_num = s_num;            \
    semops[num].sem_op  = op;               \
    semops[num].sem_flg = flag;

enum semaphores {
    READ        = 0,
    WRITE       = 1,
    MUTEX       = 2,
    SEM1        = 3,
    //TRASH       = 4,
    PRODUCER    = 4,
    CONSUMER    = 5
};

void clear_rest (int read_s, char *mem_p);

void freee(void* ptr) {
    free(*(void**)ptr);
}

size_t buff_size = 1024;
int sem_key = 42;
int shm_key = 3802;

int main(int argc, char ** argv) {
    if (argv == NULL || argc != 2) {
        printf("Ayayay!\n");
        return 0;
    }

    int input_fd = open(argv[1], O_RDONLY);     //open file to read and prepare for copy
    if (input_fd == -1) { 
        printf("Cant open file\n");
        return 0;
    }

    int read_s = 1;
    char *buff __attribute__((cleanup(freee))) 
        = calloc(buff_size, sizeof(char));
    int sems = semget(sem_key, 6, IPC_CREAT | 0644);
    int shared_mem = shmget(shm_key, buff_size, IPC_CREAT | 0644);
    char* mem_p = shmat(shared_mem, NULL, 0);

    struct sembuf semops[6];

    SET_SEMOP(0, WRITE, 0,      0);
    SET_SEMOP(1, WRITE, 1,      SEM_UNDO);
    SET_SEMOP(2, PRODUCER, 1,   SEM_UNDO);
    int semop_ = semop(sems, semops, 3);

    SET_SEMOP(0, CONSUMER, -1,  0);
    SET_SEMOP(1, CONSUMER, 1,   0);
    semop_ = semop(sems, semops, 2);

    SET_SEMOP(0, SEM1, 1,   SEM_UNDO);
    SET_SEMOP(1, SEM1, -1,  0);
    semop_ = semop(sems, semops, 2);

    lseek(input_fd, 0, SEEK_SET);

    while (1) {
        read_s = read(input_fd, buff, buff_size);

        errno = 0;
        SET_SEMOP(0, CONSUMER, -1,  IPC_NOWAIT);
        SET_SEMOP(1, CONSUMER, 1,   IPC_NOWAIT);                   
        SET_SEMOP(2, SEM1,  0,      0);
        SET_SEMOP(3, MUTEX, 0,      0);
        SET_SEMOP(4, MUTEX, 1,      SEM_UNDO);
        semop_ = semop(sems, semops, 5);

        if (semop_ == -1 && errno == EAGAIN) {
            printf("consumer has been murdered\n");
            return 0;
        }

        if (read_s > 0) {
            memcpy(mem_p, buff, read_s);
            clear_rest(read_s, mem_p);

            SET_SEMOP(0, MUTEX, -1, SEM_UNDO);
            SET_SEMOP(1, SEM1, 1,   0);
            semop_ = semop(sems, semops, 2);
        }
        else if (read_s == 0) {
            mem_p[0] = EOF;
            clear_rest(1, mem_p);

            SET_SEMOP(0, MUTEX, -1, SEM_UNDO);
            SET_SEMOP(1, SEM1, 1,   0);
            semop_ = semop(sems, semops, 2);

            break;
        }
        else {
            printf("smth went wrong!\n(Do not believe consumer)\n");

            SET_SEMOP(0, MUTEX, -1, SEM_UNDO);
            SET_SEMOP(1, SEM1, 1,   0);
            semop_ = semop(sems, semops, 2);

            return 0;
        }
    }

    return 0;
}

void clear_rest (int read_s, char *mem_p) {
    for ( ; read_s < buff_size; read_s++ ) {
        mem_p[read_s] = '\0';
    }
}