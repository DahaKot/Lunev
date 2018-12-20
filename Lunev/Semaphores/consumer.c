#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>
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
    PRODUCER    = 4,
    CONSUMER    = 5
};

size_t buff_size = 1024;
int sem_key = 42;
int shm_key = 3802;

void freee(void* ptr) {
    free(*(void**)ptr);
}

int main(int argc, char ** argv) {
    if (argv == NULL || argc != 1) {
        printf("Ayayay!\n");
        return 0;
    }
    
    char *buff __attribute__((cleanup(freee))) 
        = calloc(buff_size, sizeof(char));
    int sems = semget(sem_key, 6, IPC_CREAT | 0644); 
    int shared_mem = shmget(shm_key, buff_size, IPC_CREAT | 0644);
    char* mem_p = shmat(shared_mem, NULL, 0);    

    struct sembuf semops[6]; 

    SET_SEMOP(0, READ, 0,       0);
    SET_SEMOP(1, READ, 1,       SEM_UNDO);
    SET_SEMOP(2, CONSUMER, 1,   SEM_UNDO);
    int semop_ = semop(sems, semops, 3);

    SET_SEMOP(0, PRODUCER, -1,  0);
    SET_SEMOP(1, PRODUCER, 1,   0);
    semop_ = semop(sems, semops, 2);

    while (1) {
        errno = 0;
        SET_SEMOP(0, PRODUCER, -1,  IPC_NOWAIT);
        SET_SEMOP(1, PRODUCER, 1,   IPC_NOWAIT);                
        SET_SEMOP(2, SEM1, -1,      0);
        SET_SEMOP(3, MUTEX, 0,      0);
        SET_SEMOP(4, MUTEX, 1,      SEM_UNDO);
        semop_ = semop(sems, semops, 5);

        if (semop_ == -1 && errno == EAGAIN) {
            printf("producer has been murdered\n");
            return 0;
        }

        if (mem_p[0] == EOF) {
            SET_SEMOP(0, MUTEX, -1, SEM_UNDO);
            semop_ = semop(sems, semops, 1);
            break;
        }

        memcpy(buff, mem_p, buff_size);

        SET_SEMOP(0, MUTEX, -1, SEM_UNDO);
        semop_ = semop(sems, semops, 1);

        write(STDOUT_FILENO, buff, buff_size);    
    }

    return 0;
}
