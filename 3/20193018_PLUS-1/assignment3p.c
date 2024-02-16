#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <unistd.h>

void perform_work() {
    int A[100][100], B[100][100], result[100][100] = {0};
    int count = 0;
    int k, i, j;
    while(count < 100) {
        for(k = 0; k < 100; k++) {
            for(i = 0; i < 100; i++) {
                for(j = 0; j < 100; j++) {
                    result[k][j] += A[k][i] * B[i][j];
                }
            }
        }
        count++;
    }
}

int main() {
    pid_t pid;
    cpu_set_t set;
    
    CPU_ZERO(&set);
    CPU_SET(0, &set); 

    for(int i = 0; i < 21; i++) {
        pid = fork();
        if(pid == 0) {  // 자식 프로세스
            if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) != 0) {
                perror("sched_setaffinity");
                exit(EXIT_FAILURE);
            }
            perform_work();
            exit(0);
        } else if (pid > 0) {
            // 부모 프로세스: 자식 프로세스가 종료될 때까지 기다림
            waitpid(pid, NULL, 0);
        } else {
            // fork 실패
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
