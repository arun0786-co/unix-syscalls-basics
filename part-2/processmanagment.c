#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
void task1(){
    printf("TASK 1\n");
    int x=25;
    pid_t pid=fork();
    if(pid<0){
        perror("fork failed");
        exit(1);
    }else if(pid==0){
        x+=10;
        printf("child pid: %d, x: %d\n",getpid(),x);
        exit(0);
    }else{
        x-=5;
        printf("parent pid: %d, x: %d\n",getpid(),x);
        wait(NULL); }}
void task2(){
    printf("TASK 2\n");
    pid_t pid=fork();
    if(pid<0){
        perror("fork failed");
        exit(1);
    }else if(pid==0){
        char *args[]={"./writer",NULL};
        if(execvp(args[0],args)==-1){
            perror("execvp failed");
            exit(1);}
         exit(0);
    }else{
        wait(NULL); }}
void task3(){
    printf("TASK 3\n");
    pid_t pid=fork();
    if(pid<0){
        perror("fork failed");
        exit(1);
    }else if(pid==0){
        sleep(2);
        printf("child pid : %d, parent pid: %d\n",getpid(),getppid());
        exit(0);
    }else{
        printf("parent pid : %d \n",getpid());
        exit(0);}}
int main(){
    task1();
    sleep(1);
    task2();
    sleep(1);
    task3();
    sleep(3);
    return 0;}