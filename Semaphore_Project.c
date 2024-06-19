#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_NUM 100
#define NUM_PROCESSES 5

union semun{
    int val;
    struct semid_ds* buf;
    unsigned short *array;
    struct seminfo* __buf;
};

int semid, i; // set of 6 semaphores
union semun arg; 

void initializeSemaphores(){
    // create a set of 6 semaphores
    // semaphore 0 for system-level operations, and from 1 to 5 for application-level operations 
    // application-level operations - like wait and signal
    semid = semget(IPC_PRIVATE, NUM_PROCESSES+1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    
    // Initialize semaphore value to i (0, 1, 2, 3, 4)
    for (i=1 ; i<=NUM_PROCESSES ; i++){
        arg.val = NUM_PROCESSES-i;
        if (semctl(semid,i,SETVAL,arg) == -1){
            perror("semctl");
            exit(EXIT_FAILURE);
        }
    }
}

int waitSemaphore(int sem_index){
    struct sembuf wait = {sem_index, -4, 0}; // {sem_num,sem_op,sem_flg}
    if (semop(semid, &wait, 1) == -1) {
	perror("semop signal");
	exit(EXIT_FAILURE);
    }
}

int signalSemaphore(int sem_index){
    struct sembuf signal = {sem_index, 1, 0}; // {sem_num,sem_op,sem_flg}
    if (semop(semid, &signal, 1) == -1) {
	perror("semop signal");
	exit(EXIT_FAILURE);
    }
}

void destroySemaphore(){
    // removes semaphore set (semid) using IPC_RMID
    if (semctl(semid,0,IPC_RMID,arg) == -1){
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}

int main ()
{
	int pid, fork_index, sem_index;
	
	// ************Initialize semaphores************
	initializeSemaphores();
	
	// ************Print numbers from 1 to 100************
	for (fork_index=1 ; fork_index<=NUM_PROCESSES ; fork_index++){
 	   pid = fork();
	   if (pid == -1) {
               perror("fork");
               exit(EXIT_FAILURE);
               }
	   else if (pid == 0){
	       i=fork_index;
	       while (i <= MAX_NUM){
	           waitSemaphore(fork_index); // delay the current fork and waiting to other forks to work
		   printf("%d\n",i); // print j with jump of 5 for each semaphores
		   for(sem_index=1 ; sem_index<=NUM_PROCESSES ; sem_index++){
		       if (sem_index != fork_index)
			   signalSemaphore(sem_index); // increase all the other semaphores values by 1
		       }
		   i += NUM_PROCESSES;
	       } 
	       exit(EXIT_SUCCESS); //after printing all numbers
	    }
	 }
	
	// ************Wait for all child processes to finish************
	for(i=1 ; i<=NUM_PROCESSES ; i++){
	    wait(NULL);
	}
	
	// ************Remove semaphores************
	destroySemaphore();
	return 0;
}
	
