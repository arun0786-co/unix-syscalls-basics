#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
int main(){
    FILE *fp = fopen("newfile.txt", "w");
    if(!fp){
        perror("failed to open file");
        return 1;}
    fprintf(fp, "%d\n", getppid());
    fclose(fp);
    return 0; }