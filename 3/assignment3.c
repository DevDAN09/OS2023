#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* 연산 수행 함수 */
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

/* 실제 시간으로 포멧을 수정하는 함수 */
void format_timeval(struct timeval tv, char *buf, size_t sz) {
    struct tm *tm_info = localtime(&tv.tv_sec);
    strftime(buf, sz, "%H:%M:%S", tm_info);
    size_t len = strlen(buf);
    snprintf(buf + len, sz - len, ".%06ld", tv.tv_usec);
}

/* Round Robin 함수의 timeslice 를 수정하는 함수*/
void set_RT_RR_timeslice(int ms) {
    FILE *fp = fopen("/proc/sys/kernel/sched_rr_timeslice_ms", "w"); // 해당 proc 파일 시스템 파일 포인터 설정
    if (!fp) { // 파일 포인터게 지정 되지 않을 시
        perror("Failed to open /proc/sys/kernel/sched_rr_timeslice_ms");
        exit(EXIT_FAILURE); // 에러 반환
    }
    fprintf(fp, "%d", ms); // 함수에 입력받은 값을 해당 프록 파일 시스템에 수정 
    fclose(fp);
}

double total_elapsed_time = 0.0; // 자식 프로그램의 평균 경과 시간을 계산하기 위한 전체 경과시간의 합

int main() {
    struct sched_param param;
	    cpu_set_t set;
    int policy; // 스케줄러 정책을 저장
	int ret; // CPU 코어가 한개로 설정됬는지 확인
    char* endptr;
    char choice[50];
    char policy_str[20];
    /* 파이프 라인 */
    int pipefd[2];
    int timeslice;

    /* 파이프 라인 생성 */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    /* CPU USE only 1*/
    CPU_ZERO(&set);  // cpu_set_t set 자료구조를 zero화
    CPU_SET(0, &set);  // CPU 0번에 코드가 수행하는 세팅

    /* 현재 프로세스[0]에 CPU Affinity를 설정하는 함수 -> CPU 0번만 수행하도록 설정*/
    ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
    if(ret == -1) { // 함수의 오류 발생시
        perror("sched_setaffinity");
        exit(1);
    }
    
    /* 프로그램 메세지 출력 */
    printf("Enter scheduling choice: \n");
    printf("1. CFS_DEFAULT\n2. CFS_NICE\n3. RT_FIFO\n4. RT_RR\n0. Exit\n");

    scanf("%s", choice);

    if(strcmp(choice, "1") == 0) {
        policy = SCHED_OTHER;
        snprintf(policy_str, sizeof(policy_str), "CFS_DEFAULT");
    } else if(strcmp(choice, "2") == 0) {
        policy = SCHED_OTHER;
        snprintf(policy_str, sizeof(policy_str), "CFS_NICE");
    } else if(strcmp(choice, "3") == 0) {
        policy = SCHED_FIFO;
        snprintf(policy_str, sizeof(policy_str), "RT_FIFO");
    } else if(strcmp(choice, "4") == 0) {
        printf("Enter TimeSlice choice(10, 100, 1000): \n");
        scanf("%d", &timeslice);
        if(timeslice == 10 || timeslice == 100 || timeslice == 1000){
            set_RT_RR_timeslice(timeslice);
            policy = SCHED_RR;
            snprintf(policy_str, sizeof(policy_str), "RT_RR");
        }
        else{
            printf("옵션 이외의 time slice를 입력하셨습니다 \n");
            exit(1);
        }

    } else {
        printf("Invalid choice!\n");
        exit(1);
    }

    /* 자식 프로세스 생성 */

    for(int i = 0; i < 21; i++) {
        pid_t pid = fork(); // fork() 통한 자식 프로세스 생성
        if(pid == 0) {
            close(pipefd[0]); //부모 프로세스의 파이프 쓰기 종료
            struct timeval start, end; // 시작 시간, 끝 시간
            char start_str[20], end_str[20], elapsed_str[20]; // 결과 출력에 사용되는 string 
            
            
            /* 연산 전 현재 시간을 저장*/
            gettimeofday(&start, NULL);
            
            /* case2[CFS_NICE] : nice value initialize */
            if(policy == SCHED_OTHER && strcmp(choice, "2") == 0) { 
                int current_pid = getpid();
                if(i < 7) {
                    setpriority(PRIO_PROCESS, current_pid, -20);
                } else if(i < 14) {
                    setpriority(PRIO_PROCESS, current_pid, 0);
                } else {
                    setpriority(PRIO_PROCESS, current_pid, 19);
                }
            }


            /* CFS 를 사용하면 sched_priority를 0으로 설정*/
            param.sched_priority = 0;
            /* RT CLASS 스케줄러를 사용하면 1~99 사이의 값을 설정*/
            if(policy == SCHED_FIFO || policy == SCHED_RR) {
                param.sched_priority = sched_get_priority_max(policy);
            }

            int pidn = getpid();
            sched_setscheduler(0, policy, &param);
            int ret_sched = sched_getscheduler(pidn);

            
            /* 연산을 수행 */
            perform_work();

            /* 연산이 종료된 시간을 저장 */
            gettimeofday(&end, NULL);
            
            /* 시:분:초 형식 */
            format_timeval(start, start_str, sizeof(start_str));
            format_timeval(end, end_str, sizeof(end_str));
            long seconds = end.tv_sec - start.tv_sec;
            long useconds = end.tv_usec - start.tv_usec;
            //long elapsed = seconds + useconds/1000000.0;
            if(useconds < 0) {
                seconds--;
                useconds += 1000000;
            }
            snprintf(elapsed_str, sizeof(elapsed_str), "%ld.%06ld", seconds, useconds);
            double elapsed_time = strtod(elapsed_str,&endptr);
            
            /* CFS_NICE 선택시 수행 */
            if(policy == SCHED_OTHER && strcmp(choice, "2") == 0){ 
                int current_nice = getpriority(PRIO_PROCESS, 0);
                printf("PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                getpid() , current_nice, start_str, end_str, elapsed_str);
                /*
                printf("Policy: %d | PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                ret_sched, getpid() , current_nice, start_str, end_str, elapsed_str);         
                */

            } else if(policy == SCHED_OTHER && strcmp(choice, "1") == 0){ // CFS_DEFAULT
                int current_nice = getpriority(PRIO_PROCESS, 0);
                printf("PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                getpid() , current_nice, start_str, end_str, elapsed_str);

                /*
                printf("Policy : %d | PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                ret_sched, getpid(), current_nice, start_str, end_str, elapsed_str);
                */
            }
            else{ //RT CLASS
                int current_nice = getpriority(PRIO_PROCESS, 0);
                printf("PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                getpid() , current_nice, start_str, end_str, elapsed_str);

                /*
                printf("Policy: %d | PID: %d | NICE : %d | Start time: %s | End time: %s | Elapsed time: %s\n", 
                ret_sched, getpid() ,current_nice, start_str, end_str, elapsed_str);
                */
            }
            /* 각각의 자식프로세스의 경과시간을 파이프라인에 쓰기 */
            write(pipefd[1], &elapsed_time, sizeof(double));
            close(pipefd[1]);  
            exit(0);
        }
    }

    for(int i = 0; i < 21; i++) {
        close(pipefd[1]);  // 자식 프로세스의 파이프 쓰기 종료
        double elapsed_time;

        /* 자식 프로세스의 경과시간을 읽기*/
        read(pipefd[0], &elapsed_time, sizeof(double)); 
        /* 각각의 자식 프로세스의 경과시간들을 누적합 진행*/
        total_elapsed_time += elapsed_time;
        wait(NULL);
    }

    /*최종 결과 출력*/
    if(strcmp(choice, "4") == 0){
        printf("Scheduling Policy: %s | Time Quantum: %d ms | Average elapsed time: %f\n",policy_str,timeslice, total_elapsed_time/21);
    } else{
        printf("Scheduling Policy: %s | Average elapsed time: %f\n",policy_str, total_elapsed_time/21);
    }
    

    return 0;
}
