#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
void inttostr(int num,char *s){
    if(num==0){
        s[0]='0';
        s[1]='\0';
        return; }
    char rev[20];
    int l=0;
    while(num>0){
        rev[l++]=(num%10)+'0';
        num/=10; }
    for(int i=0;i<l;i++){
        s[i]=rev[l-1-i]; }
    s[l]='\0'; }
void addlog(const char *m){
    int lf=open("logs.txt",O_WRONLY|O_APPEND|O_CREAT,0666);
    if(lf<0){
        printf("error opening logs.txt\n");
        return; }
    int saveout=dup(1);
    dup2(lf,1);
    printf("%s\n",m);
    fflush(stdout);
    dup2(saveout,1);
    close(saveout);
    close(lf);  }
int countln(const char *fname){
    int fd=open(fname,O_RDONLY);
    if(fd<0) return 0;
    char ch;
    int cnt=0;
    while(read(fd,&ch,1)>0){
        if(ch=='\n') cnt++; }
    close(fd);
    return cnt;}
void showall(const char *fname){
    int fd=open(fname,O_RDONLY);
    if(fd<0) return;
    char b[1024];
    int by;
    while((by=read(fd,b,sizeof(b)))>0){
        write(1,b,by);}
    close(fd); }
void showfirst(const char *fname,int n){
    if(n<=0) return;
    int fd=open(fname,O_RDONLY);
    if(fd<0) return;
    char ch;
    int cnt=0;
    while(read(fd,&ch,1)>0){
        write(1,&ch,1);
        if(ch=='\n'){
            cnt++;
            if(cnt>=n) break; }}
    close(fd); }
void showlast(const char *fname,int n){
    int total=countln(fname);
    if(total==0) return;
    if(n>total) n=total;
    int skip=total-n;
    int fd=open(fname,O_RDONLY);
    if(fd<0) return;
    char ch;
    int cnt=0;
    while(read(fd,&ch,1)>0){
        if(cnt>=skip) write(1,&ch,1);
        if(ch=='\n') cnt++;}
    close(fd); }
int main(){
    char fold[50];
    int pid=getpid();
    strcpy(fold,"folder_");
    char pidstr[20];
    inttostr(pid,pidstr);
    strcat(fold,pidstr);
    mkdir(fold,0777);
    chdir(fold);
    close(open("content.txt",O_CREAT|O_TRUNC|O_WRONLY,0666));
    close(open("logs.txt",O_CREAT|O_TRUNC|O_WRONLY,0666));
    char cm[1024];
    while(1){
        printf("enter command:");
        scanf("%s",cm);
        if(strcmp(cm,"INPUT")==0){
            addlog("INPUT");
            printf("INPUT -> ");
            getchar();
            char t[1024];
            scanf("%1023[^\n]",t);
            int fd=open("content.txt",O_WRONLY|O_APPEND);
            if(fd<0) continue;
            int saveout=dup(1);
            dup2(fd,1);
            printf("%s\n",t);
            fflush(stdout);
            dup2(saveout,1);
            close(saveout);
            close(fd);
        }else if(strcmp(cm,"PRINT")==0){
            addlog("PRINT");
            showall("content.txt");
        }else if(strcmp(cm,"FIRST")==0){
            int n;
            scanf("%d",&n);
            char lb[50];
            strcpy(lb,"FIRST ");
            char num[20];
            inttostr(n,num);
            strcat(lb,num);
            addlog(lb);
            showfirst("content.txt",n);
        }else if(strcmp(cm,"LAST")==0){
            int n;
            scanf("%d",&n);
            char lb[50];
            strcpy(lb,"LAST ");
            char num[20];
            inttostr(n,num);
            strcat(lb,num);
            addlog(lb);
            showlast("content.txt",n); 
        }else if(strcmp(cm,"LOG")==0){
            int n;
            scanf("%d",&n);
            showlast("logs.txt",n); 
        }else if(strcmp(cm,"STOP")==0){
            addlog("STOP");
            break; 
        }else{
            printf("unknown command\n");
        }}
    return 0; }